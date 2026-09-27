#define NOMINMAX
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/core/Hash.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include "avemotion/reference/ReferenceRuntime.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "avemotion/render/SourceGeometryProjector.hpp"
#include "avemotion/runtime/Runtime.hpp"
#include "support/CaptureCorpus.hpp"
#include "support/PixelComparison.hpp"
#include "support/WarpCaptureSurface.hpp"

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef AVEMOTION_CAPTURE_ARTIFACT_DIR
#define AVEMOTION_CAPTURE_ARTIFACT_DIR "."
#endif
#ifndef AVEMOTION_CORPUS_DIR
#error "AVEMOTION_CORPUS_DIR is required"
#endif
#ifndef AVEMOTION_FIXTURE_DIR
#error "AVEMOTION_FIXTURE_DIR is required"
#endif

namespace {

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;
using avemotion::testsupport::PixelBGRA;

[[noreturn]] void fail(const std::string& message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, const std::string& message) {
    if (!condition) fail(message);
}

void requireHr(HRESULT value, const std::string& operation) {
    if (FAILED(value)) {
        std::ostringstream stream;
        stream << operation << " failed with HRESULT 0x"
               << std::hex << std::uppercase
               << static_cast<std::uint32_t>(value);
        fail(stream.str());
    }
}

std::string readText(const fs::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) fail("unable to open " + path.string());
    return {
        std::istreambuf_iterator<char>{stream},
        std::istreambuf_iterator<char>{}};
}

fs::path assetPath(const avemotion::testsupport::CaptureAssetSpec& asset) {
    return (asset.location
            == avemotion::testsupport::CaptureAssetLocation::Corpus
        ? fs::path{AVEMOTION_CORPUS_DIR}
        : fs::path{AVEMOTION_FIXTURE_DIR})
        / asset.fileName;
}

using avemotion::testsupport::WarpCaptureSurface;

std::vector<PixelBGRA> convertReference(
    const avemotion::reference::RenderedFrame& frame) {
    std::vector<PixelBGRA> result;
    result.reserve(frame.argbPremultiplied.size());
    for (const auto pixel : frame.argbPremultiplied) {
        result.push_back(avemotion::testsupport::pixelFromArgb32(pixel));
    }
    return result;
}

std::uint64_t hashPixels(const std::vector<PixelBGRA>& pixels) {
    avemotion::core::Fnv1a64 hash;
    for (const auto pixel : pixels) {
        hash.appendU8(pixel.b);
        hash.appendU8(pixel.g);
        hash.appendU8(pixel.r);
        hash.appendU8(pixel.a);
    }
    return hash.value();
}

std::uint8_t compositeChannel(
    std::uint8_t premultiplied,
    std::uint8_t alpha,
    std::uint8_t background) noexcept {
    const auto backgroundContribution =
        static_cast<unsigned>(background)
        * static_cast<unsigned>(255U - alpha);
    return static_cast<std::uint8_t>(std::min(
        255U,
        static_cast<unsigned>(premultiplied)
            + (backgroundContribution + 127U) / 255U));
}

void writeU16(std::ostream& output, std::uint16_t value) {
    const std::uint8_t bytes[] = {
        static_cast<std::uint8_t>(value & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
    };
    output.write(
        reinterpret_cast<const char*>(bytes),
        static_cast<std::streamsize>(sizeof(bytes)));
}

void writeU32(std::ostream& output, std::uint32_t value) {
    const std::uint8_t bytes[] = {
        static_cast<std::uint8_t>(value & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
    };
    output.write(
        reinterpret_cast<const char*>(bytes),
        static_cast<std::streamsize>(sizeof(bytes)));
}

void writeRgbBmp(
    const fs::path& path,
    const std::vector<std::array<std::uint8_t, 3>>& rgb,
    std::size_t width,
    std::size_t height) {
    require(rgb.size() == width * height, "invalid BMP pixel storage");
    require(width <= std::numeric_limits<std::uint32_t>::max()
                && height <= std::numeric_limits<std::uint32_t>::max(),
            "BMP dimensions are too large");
    fs::create_directories(path.parent_path());
    std::ofstream output{path, std::ios::binary};
    require(static_cast<bool>(output), "unable to create " + path.string());

    const auto rowBytes = width * 3U;
    const auto rowStride = (rowBytes + 3U) & ~std::size_t{3U};
    const auto pixelBytes = rowStride * height;
    require(pixelBytes <= std::numeric_limits<std::uint32_t>::max() - 54U,
            "BMP payload is too large");
    const auto fileSize = static_cast<std::uint32_t>(54U + pixelBytes);

    output.put('B');
    output.put('M');
    writeU32(output, fileSize);
    writeU16(output, 0U);
    writeU16(output, 0U);
    writeU32(output, 54U);
    writeU32(output, 40U);
    writeU32(output, static_cast<std::uint32_t>(width));
    writeU32(output, static_cast<std::uint32_t>(height));
    writeU16(output, 1U);
    writeU16(output, 24U);
    writeU32(output, 0U);
    writeU32(output, static_cast<std::uint32_t>(pixelBytes));
    writeU32(output, 3780U);
    writeU32(output, 3780U);
    writeU32(output, 0U);
    writeU32(output, 0U);

    const std::array<char, 3> padding{{0, 0, 0}};
    for (std::size_t sourceY = height; sourceY-- > 0U;) {
        for (std::size_t x = 0U; x < width; ++x) {
            const auto& pixel = rgb[sourceY * width + x];
            const std::uint8_t bgr[] = {pixel[2], pixel[1], pixel[0]};
            output.write(
                reinterpret_cast<const char*>(bgr),
                static_cast<std::streamsize>(sizeof(bgr)));
        }
        const auto paddingBytes = rowStride - rowBytes;
        output.write(padding.data(), static_cast<std::streamsize>(paddingBytes));
    }
    require(static_cast<bool>(output), "unable to write " + path.string());
}

void writeCompositeBmp(
    const fs::path& path,
    const std::vector<PixelBGRA>& pixels,
    std::size_t width,
    std::size_t height) {
    std::vector<std::array<std::uint8_t, 3>> rgb(width * height);
    for (std::size_t y = 0U; y < height; ++y) {
        for (std::size_t x = 0U; x < width; ++x) {
            const auto& pixel = pixels[y * width + x];
            const auto check = ((x / 8U) + (y / 8U)) % 2U;
            const std::uint8_t background = check == 0U ? 224U : 184U;
            rgb[y * width + x] = {
                compositeChannel(pixel.r, pixel.a, background),
                compositeChannel(pixel.g, pixel.a, background),
                compositeChannel(pixel.b, pixel.a, background),
            };
        }
    }
    writeRgbBmp(path, rgb, width, height);
}

void writeDiffBmp(
    const fs::path& path,
    const std::vector<PixelBGRA>& reference,
    const std::vector<PixelBGRA>& actual,
    std::size_t width,
    std::size_t height) {
    std::vector<std::array<std::uint8_t, 3>> rgb(width * height);
    for (std::size_t index = 0U; index < width * height; ++index) {
        const auto& expected = reference[index];
        const auto& observed = actual[index];
        const auto channelDifference = [](std::uint8_t left, std::uint8_t right) {
            return static_cast<unsigned>(std::abs(
                static_cast<int>(left) - static_cast<int>(right)));
        };
        const auto color = std::max({
            channelDifference(expected.r, observed.r),
            channelDifference(expected.g, observed.g),
            channelDifference(expected.b, observed.b),
        });
        const auto alpha = channelDifference(expected.a, observed.a);
        rgb[index] = {
            static_cast<std::uint8_t>(std::min(255U, color * 4U)),
            static_cast<std::uint8_t>(std::min(255U, alpha * 4U)),
            static_cast<std::uint8_t>(std::min(255U, (color + alpha) * 2U)),
        };
    }
    writeRgbBmp(path, rgb, width, height);
}

std::string formatBounds(const avemotion::testsupport::PixelBounds& bounds) {
    if (!bounds.valid) return "none";
    std::ostringstream stream;
    stream << bounds.left << ',' << bounds.top << ','
           << bounds.right << ',' << bounds.bottom;
    return stream.str();
}

struct AssetContext final {
    avemotion::testsupport::CaptureAssetSpec spec;
    fs::path path;
    std::string json;
    avemotion::runtime::Runtime runtime;
    std::shared_ptr<const avemotion::runtime::Asset> asset;
    std::unique_ptr<avemotion::runtime::Instance> instance;
    std::unique_ptr<avemotion::evaluation::PropertyEvaluator> evaluator;
    avemotion::evaluation::PropertyEvaluationWorkspace propertyWorkspace;
    std::unique_ptr<avemotion::render::SourceGeometryProjector> projector;
    avemotion::render::SourceGeometryProjectionWorkspace projectionWorkspace;
    std::unique_ptr<avemotion::reference::ReferenceAnimation> reference;
    std::uint64_t propertyStorageGeneration = 0;
    std::uint64_t projectionStorageGeneration = 0;
};

std::unique_ptr<AssetContext> loadAssetContext(
    const avemotion::testsupport::CaptureAssetSpec& spec,
    const avemotion::reference::ReferenceRuntime& referenceRuntime) {
    auto context = std::make_unique<AssetContext>();
    context->spec = spec;
    context->path = assetPath(spec);
    context->json = readText(context->path);

    auto loaded = context->runtime.loadLottieJson(
        context->json, context->path.filename().string());
    require(static_cast<bool>(loaded),
            "runtime load failed for " + context->path.string());
    context->asset = std::move(loaded.asset);
    const auto prepared = context->asset->prepareModel();
    require(static_cast<bool>(prepared),
            "canonical model preparation failed for " + context->path.string());
    auto created = context->runtime.createInstance(context->asset);
    require(static_cast<bool>(created),
            "instance creation failed for " + context->path.string());
    context->instance = std::move(created.instance);

    context->evaluator =
        std::make_unique<avemotion::evaluation::PropertyEvaluator>(prepared.model);
    require(context->evaluator->valid(),
            "property evaluator invalid for " + context->path.string());
    context->evaluator->prepare(context->propertyWorkspace);
    context->projector =
        std::make_unique<avemotion::render::SourceGeometryProjector>(
            prepared.model);
    require(context->projector->valid(),
            "source projector invalid for " + context->path.string());
    require(context->projector->prepare(context->projectionWorkspace),
            "source projection workspace preparation failed");
    context->propertyStorageGeneration =
        context->propertyWorkspace.storageGeneration();
    context->projectionStorageGeneration =
        context->projectionWorkspace.storageGeneration();

    auto referenceLoaded = referenceRuntime.loadJson(
        context->json,
        std::string{"avemotion-part19-"} + std::string{spec.id},
        false);
    require(static_cast<bool>(referenceLoaded),
            "reference load failed for " + context->path.string());
    context->reference = std::move(referenceLoaded.animation);
    require(context->reference->metadata().totalFrames
                == context->asset->metadata().totalFrames,
            "runtime/reference frame count mismatch");
    return context;
}

void writeManifestHeader(std::ostream& out) {
    out << "asset\tsample\tframe\tprofile\tlogical_size\tpixel_size\tdpi"
           "\tdraw_items\tprojected_items\titems_drawn\titems_skipped"
           "\tgeometry_created_delta\tgeometry_replaced_delta"
           "\trepaint_geometry_created_delta\trepaint_cache_hits_delta"
           "\treference_hash\tdirect2d_hash\tactive_iou"
           "\talpha_relative_error\tmean_abs_all\tmean_abs_active"
           "\trmse_all\tlarge_diff_fraction\tmax_channel_diff"
           "\tbounds_delta\treference_bounds\tdirect2d_bounds\tstatus\n";
}

std::string caseId(
    const avemotion::testsupport::CaptureAssetSpec& asset,
    const avemotion::testsupport::CaptureSample& sample,
    const avemotion::testsupport::CaptureProfile& profile) {
    return std::string{asset.id} + "_" + std::string{sample.id}
        + "_" + std::string{profile.id};
}

} // namespace

int main() {
    using namespace avemotion;
    using namespace avemotion::testsupport;

    require(reference::ReferenceRuntime::compiledWithRlottie(),
            "Part 19 capture requires the Telegram rlottie reference engine");

    const fs::path artifactRoot{AVEMOTION_CAPTURE_ARTIFACT_DIR};
    // A capture run is evidence, not an append-only cache. Remove stale
    // artifacts so a repeated or interrupted run cannot satisfy the launcher
    // with files that were produced by a previous corpus definition.
    fs::remove_all(artifactRoot);
    fs::create_directories(artifactRoot);
    std::ofstream manifest{artifactRoot / "capture_manifest.tsv", std::ios::binary};
    require(static_cast<bool>(manifest), "unable to create capture manifest");
    writeManifestHeader(manifest);

    reference::ReferenceRuntime referenceRuntime;
    std::vector<std::unique_ptr<AssetContext>> assets;
    assets.reserve(kDirect2DCaptureAssets.size());
    for (const auto& spec : kDirect2DCaptureAssets) {
        assets.push_back(loadAssetContext(spec, referenceRuntime));
    }

    WarpCaptureSurface surface;
    backends::direct2d::Backend backend;
    PixelComparisonPolicy policy;
    std::vector<std::string> failures;
    std::size_t completedCases = 0U;

    for (const auto& profile : kDirect2DCaptureProfiles) {
        surface.configure(profile);
        for (auto& assetContext : assets) {
            for (const auto& sample : kDirect2DCaptureSamples) {
                const auto frame = assetContext->instance->frameAtPosition(
                    sample.normalizedPosition);
                require(frame == assetContext->reference->frameAtPosition(
                                    sample.normalizedPosition),
                        "runtime/reference selected different frames");

                const auto properties = assetContext->evaluator->evaluate(
                    static_cast<double>(frame),
                    assetContext->propertyWorkspace);
                require(static_cast<bool>(properties),
                        "property evaluation failed for capture case");
                auto evaluated = assetContext->instance->evaluateModelFrame(
                    frame, profile.logicalWidth, profile.logicalHeight);
                require(static_cast<bool>(evaluated),
                        "scene evaluation failed for capture case");
                const auto projected = assetContext->projector->project(
                    evaluated.scene,
                    properties,
                    assetContext->projectionWorkspace);
                require(static_cast<bool>(projected),
                        "source geometry projection failed for capture case");
                require(projected.statistics.projected != 0U,
                        "capture case did not use AveMotion-owned geometry");
                require(assetContext->propertyWorkspace.storageGeneration()
                            == assetContext->propertyStorageGeneration,
                        "property workspace grew during capture");
                require(assetContext->projectionWorkspace.storageGeneration()
                            == assetContext->projectionStorageGeneration,
                        "projection workspace grew during capture");

                render::MotionRenderPlanner planner;
                auto planned = planner.build(std::move(evaluated.scene));
                require(static_cast<bool>(planned),
                        "render plan build failed for capture case");
                require(planned.plan.statistics.unsupportedFeatureItemCount == 0U,
                        "capture plan contains unsupported Direct2D features");

                const auto referenceFrame = assetContext->reference->renderFrame(
                    frame,
                    profile.pixelWidth,
                    profile.pixelHeight,
                    true);
                require(referenceFrame.width == profile.pixelWidth
                            && referenceFrame.height == profile.pixelHeight,
                        "reference frame size mismatch");
                const auto referencePixels = convertReference(referenceFrame);

                const auto before = backend.diagnostics();
                surface.begin();
                const auto firstDraw = backend.draw(planned.plan, {
                    .deviceContext = surface.context(),
                    .graphicsDomainId = 1ULL,
                    .graphicsGeneration = 1ULL,
                });
                require(static_cast<bool>(firstDraw),
                        "Direct2D capture draw failed");
                require(firstDraw.itemsSkipped == 0U
                            && firstDraw.itemsDrawn == planned.plan.drawItems.size(),
                        "Direct2D capture skipped supported draw items");
                surface.end();
                const auto firstPixels = surface.readPixels();
                const auto afterFirst = backend.diagnostics();

                surface.begin();
                const auto repaint = backend.draw(planned.plan, {
                    .deviceContext = surface.context(),
                    .graphicsDomainId = 1ULL,
                    .graphicsGeneration = 1ULL,
                });
                require(static_cast<bool>(repaint),
                        "Direct2D capture repaint failed");
                surface.end();
                const auto direct2dPixels = surface.readPixels();
                const auto afterRepaint = backend.diagnostics();
                require(firstPixels == direct2dPixels,
                        "Direct2D repaint changed pixels for an unchanged plan");
                require(afterRepaint.geometryResourcesCreated
                            == afterFirst.geometryResourcesCreated
                            && afterRepaint.geometryResourcesReplaced
                                == afterFirst.geometryResourcesReplaced,
                        "Direct2D repaint rebuilt unchanged geometry");

                const auto metrics = comparePixels(
                    referencePixels,
                    direct2dPixels,
                    profile.pixelWidth,
                    profile.pixelHeight);
                const auto decision = evaluateComparison(metrics, policy);
                const auto id = caseId(assetContext->spec, sample, profile);
                const auto caseDirectory = artifactRoot / id;
                writeCompositeBmp(
                    caseDirectory / "telegram.bmp",
                    referencePixels,
                    profile.pixelWidth,
                    profile.pixelHeight);
                writeCompositeBmp(
                    caseDirectory / "direct2d.bmp",
                    direct2dPixels,
                    profile.pixelWidth,
                    profile.pixelHeight);
                writeDiffBmp(
                    caseDirectory / "diff.bmp",
                    referencePixels,
                    direct2dPixels,
                    profile.pixelWidth,
                    profile.pixelHeight);

                manifest << assetContext->spec.id << '\t'
                         << sample.id << '\t'
                         << frame << '\t'
                         << profile.id << '\t'
                         << profile.logicalWidth << 'x' << profile.logicalHeight
                         << '\t'
                         << profile.pixelWidth << 'x' << profile.pixelHeight
                         << '\t'
                         << profile.dpiX << 'x' << profile.dpiY << '\t'
                         << planned.plan.drawItems.size() << '\t'
                         << projected.statistics.projected << '\t'
                         << firstDraw.itemsDrawn << '\t'
                         << firstDraw.itemsSkipped << '\t'
                         << (afterFirst.geometryResourcesCreated
                                - before.geometryResourcesCreated) << '\t'
                         << (afterFirst.geometryResourcesReplaced
                                - before.geometryResourcesReplaced) << '\t'
                         << (afterRepaint.geometryResourcesCreated
                                - afterFirst.geometryResourcesCreated) << '\t'
                         << (afterRepaint.geometryCacheHits
                                - afterFirst.geometryCacheHits) << '\t'
                         << core::formatHash(hashPixels(referencePixels)) << '\t'
                         << core::formatHash(hashPixels(direct2dPixels)) << '\t'
                         << std::fixed << std::setprecision(6)
                         << metrics.activeIoU << '\t'
                         << metrics.alphaRelativeError << '\t'
                         << metrics.meanAbsoluteDifferenceAll << '\t'
                         << metrics.meanAbsoluteDifferenceActive << '\t'
                         << metrics.rootMeanSquareDifferenceAll << '\t'
                         << metrics.largeDifferenceFraction << '\t'
                         << static_cast<unsigned>(metrics.maxChannelDifference)
                         << '\t'
                         << metrics.maximumBoundsDelta << '\t'
                         << formatBounds(metrics.referenceBounds) << '\t'
                         << formatBounds(metrics.actualBounds) << '\t'
                         << (decision.passed ? "PASS" : "FAIL") << '\n';

                if (!decision.passed) {
                    std::ostringstream message;
                    message << id << ": ";
                    for (std::size_t index = 0U;
                         index < decision.failures.size(); ++index) {
                        if (index != 0U) message << "; ";
                        message << decision.failures[index];
                    }
                    message << " [IoU=" << metrics.activeIoU
                            << ", alphaError=" << metrics.alphaRelativeError
                            << ", meanActive="
                            << metrics.meanAbsoluteDifferenceActive
                            << ", largeFraction="
                            << metrics.largeDifferenceFraction
                            << ", boundsDelta="
                            << metrics.maximumBoundsDelta << ']';
                    failures.push_back(message.str());
                }
                ++completedCases;
            }
        }
    }

    require(completedCases == kDirect2DCaptureCaseCount,
            "Direct2D capture case count changed");
    manifest.flush();
    require(static_cast<bool>(manifest), "unable to finalize capture manifest");

    const auto diagnostics = backend.diagnostics();
    std::ofstream summary{artifactRoot / "README.txt", std::ios::binary};
    summary << "AveMotion Part 19 Direct2D capture corpus\n"
            << "cases=" << completedCases << '\n'
            << "policy.minimumActiveIoU=" << policy.minimumActiveIoU << '\n'
            << "policy.maximumAlphaRelativeError="
            << policy.maximumAlphaRelativeError << '\n'
            << "policy.maximumMeanAbsoluteDifferenceAll="
            << policy.maximumMeanAbsoluteDifferenceAll << '\n'
            << "policy.maximumMeanAbsoluteDifferenceActive="
            << policy.maximumMeanAbsoluteDifferenceActive << '\n'
            << "policy.maximumLargeDifferenceFraction="
            << policy.maximumLargeDifferenceFraction << '\n'
            << "policy.maximumBoundsDelta="
            << policy.maximumBoundsDelta << '\n'
            << "drawCalls=" << diagnostics.drawCalls << '\n'
            << "drawItemsVisited=" << diagnostics.drawItemsVisited << '\n'
            << "geometryCacheHits=" << diagnostics.geometryCacheHits << '\n'
            << "geometryCacheMisses=" << diagnostics.geometryCacheMisses << '\n'
            << "geometryResourcesCreated="
            << diagnostics.geometryResourcesCreated << '\n'
            << "geometryResourcesReplaced="
            << diagnostics.geometryResourcesReplaced << '\n'
            << "failures=" << failures.size() << '\n';
    for (const auto& failure : failures) summary << failure << '\n';
    summary.flush();

    if (!failures.empty()) {
        for (const auto& failure : failures) {
            std::cerr << "CAPTURE MISMATCH: " << failure << '\n';
        }
        fail("Direct2D capture corpus exceeded the raster parity policy");
    }

    std::cout << "AveMotion Direct2D capture corpus passed\n"
              << "cases=" << completedCases << '\n'
              << "artifactRoot=" << artifactRoot.string() << '\n'
              << "drawCalls=" << diagnostics.drawCalls << '\n'
              << "geometryCacheHits=" << diagnostics.geometryCacheHits << '\n'
              << "geometryCacheMisses=" << diagnostics.geometryCacheMisses
              << '\n';
    return EXIT_SUCCESS;
}
