#define NOMINMAX
#include "NativeEllipseCertificate.hpp" // Unchanged oracle's legacy inline type declaration.
#include "OwnNativeEllipseStream.hpp"
#include "support/CaptureCorpus.hpp"
#include "support/NativeEllipseOracle.hpp"
#include "support/NativeEllipseTestAssets.hpp"
#include "support/OwnNativeEllipseStreamTestData.hpp"
#include "support/PixelComparison.hpp"
#include "support/WarpCaptureSurface.hpp"
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/render/RenderPlanner.hpp"

#include <rlottie.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace fs = std::filesystem;
using namespace avemotion;
using avemotion::testsupport::PixelBGRA;
using avemotion::testsupport::WarpCaptureSurface;
constexpr std::uint64_t kOrdinaryOraclePlanId = 0x26A26003ULL;
constexpr std::array<std::size_t, 6> kFrames{0, 10, 19, 20, 30, 60};
constexpr std::array<std::string_view, 6> kAssets{
    "animated", "static-visible", "activity", "nonlinear-easing",
    "nonsquare", "fractional-coordinates"};

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string readFixture() {
    std::ifstream input(fs::path{AVEMOTION_FIXTURE_DIR} / "telegram_sticker_basic.json",
                        std::ios::binary);
    require(bool(input), "baseline fixture readable");
    return {std::istreambuf_iterator<char>{input}, {}};
}

fs::path freshArtifactDirectory() {
    const fs::path root{AVEMOTION_OWN_CAPTURE_ARTIFACT_DIR};
    fs::create_directories(root);
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned suffix = 0; suffix < 1000; ++suffix) {
        const auto directory = root / ("run-" + std::to_string(stamp)
            + "-" + std::to_string(suffix));
        if (fs::create_directory(directory)) return directory;
    }
    throw std::runtime_error("unable to reserve unique capture artifact directory");
}

void writePpm(const fs::path& path, const std::vector<PixelBGRA>& pixels,
              std::size_t width, std::size_t height) {
    require(pixels.size() == width * height, "PPM pixel dimensions");
    std::ofstream output(path, std::ios::binary);
    require(bool(output), "PPM output open: " + path.string());
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto& pixel : pixels) {
        const auto overWhite = [alpha = unsigned(pixel.a)](std::uint8_t value) {
            return static_cast<char>(std::min(255U,
                unsigned(value) + (255U - alpha)));
        };
        const std::array<char, 3> rgb{
            overWhite(pixel.r), overWhite(pixel.g), overWhite(pixel.b)};
        output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
    }
    require(bool(output), "PPM output write: " + path.string());
}

void writeMismatch(const fs::path& root, const std::string& label,
                   const std::vector<PixelBGRA>& expected,
                   const std::vector<PixelBGRA>& actual,
                   std::size_t width, std::size_t height) {
    require(expected.size() == actual.size(), label + " mismatch storage");
    writePpm(root / (label + "-expected.ppm"), expected, width, height);
    writePpm(root / (label + "-actual.ppm"), actual, width, height);
    std::vector<PixelBGRA> difference(expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto delta = [](std::uint8_t a, std::uint8_t b) {
            return static_cast<std::uint8_t>(std::abs(int(a) - int(b)));
        };
        const auto color = std::max({
            delta(expected[i].b, actual[i].b),
            delta(expected[i].g, actual[i].g),
            delta(expected[i].r, actual[i].r)});
        const auto alpha = delta(expected[i].a, actual[i].a);
        difference[i] = {
            static_cast<std::uint8_t>(std::min(255U, 2U * (unsigned(color) + alpha))),
            static_cast<std::uint8_t>(std::min(255U, 4U * unsigned(alpha))),
            static_cast<std::uint8_t>(std::min(255U, 4U * unsigned(color))),
            255};
    }
    writePpm(root / (label + "-diff.ppm"), difference, width, height);
}

std::size_t activePixels(const std::vector<PixelBGRA>& pixels) {
    return static_cast<std::size_t>(std::count_if(pixels.begin(), pixels.end(),
        [](const PixelBGRA& pixel) { return pixel.a > 8; }));
}

std::vector<PixelBGRA> ordinaryCpuPixels(rlottie::Animation& animation,
                                        std::size_t frame,
                                        const testsupport::CaptureProfile& profile) {
    std::vector<std::uint32_t> argb(profile.pixelWidth * profile.pixelHeight);
    rlottie::Surface target(argb.data(), profile.pixelWidth, profile.pixelHeight,
                            profile.pixelWidth * sizeof(std::uint32_t));
    animation.renderSync(frame, target);
    std::vector<PixelBGRA> pixels;
    pixels.reserve(argb.size());
    for (const auto value : argb)
        pixels.push_back(testsupport::pixelFromArgb32(value));
    return pixels;
}

std::vector<PixelBGRA> renderPlan(WarpCaptureSurface& surface,
                                  backends::direct2d::Backend& backend,
                                  const render::MotionRenderPlan& plan,
                                  std::uint64_t domain = 1,
                                  std::uint64_t generation = 1) {
    surface.begin();
    const auto drawn = backend.draw(plan, {
        .deviceContext = surface.context(),
        .graphicsDomainId = domain,
        .graphicsGeneration = generation});
    require(bool(drawn), "WARP draw: " + drawn.error.message);
    require(drawn.itemsSkipped == 0 && drawn.itemsDrawn == plan.drawItems.size(),
            "WARP supported draw coverage");
    surface.end();
    return surface.readPixels();
}

render::MotionRenderPlan emitPlan(render::detail::OwnNativeEllipseStream& stream,
                                render::MotionRenderPlanner& planner,
                                std::size_t frame,
                                const testsupport::CaptureProfile& profile) {
    auto emitted = stream.emit(frame, profile.logicalWidth, profile.logicalHeight);
    require(bool(emitted), "own scene emission: " + emitted.message);
    require(!emitted.scene->assetHandle.valid()
                && !emitted.scene->instanceHandle.valid(),
            "own scene uses raw factory identity");
    auto built = planner.build(std::move(*emitted.scene));
    require(bool(built), "own render plan: " + built.error.message);
    require(built.plan.statistics.unsupportedFeatureItemCount == 0,
            "own plan contains no unsupported features");
    return std::move(built.plan);
}

bool expectedEmpty(std::string_view asset, std::size_t frame) {
    return asset == "activity" && (frame < 10 || frame >= 20);
}

void pixelMatrix(const std::string& fixture, const fs::path& artifactRoot) {
    std::ofstream metricsFile(artifactRoot / "pixel-matrix.tsv", std::ios::binary);
    require(bool(metricsFile), "pixel matrix open");
    metricsFile << "asset\tframe\tprofile\tstatus\twarp_exact\tordinary_active"
                   "\town_active\tiou\talpha_relative_error\tmean_abs_all"
                   "\tmean_abs_active\trmse_all\tlarge_diff_fraction"
                   "\tmax_channel_diff\tbounds_delta\tfailures\n";
    testsupport::PixelComparisonPolicy policy;
    std::size_t cases = 0, visibleCases = 0, emptyCases = 0, failures = 0;
    const auto allAssets = test::nativeEllipseTestAssets(fixture);
    for (const auto assetName : kAssets) {
        const auto found = std::find_if(allAssets.begin(), allAssets.end(),
            [assetName](const auto& asset) { return asset.name == assetName; });
        require(found != allAssets.end(), "matrix asset present: " + std::string(assetName));
        auto owner = test::prepareOwnEllipseForTest(found->json);
        auto stream = render::detail::OwnNativeEllipseStream::create(owner);
        require(bool(stream), "own stream: " + stream.message);
        test::NativeEllipseOracle ordinary(found->json);
        const auto seedSamples = ordinary.directSampleCount();
        // This CPU animation is a separate ordinary, cache-disabled parse.
        auto cpu = rlottie::Animation::loadFromData(
            found->json, "own-capture-cpu-" + std::string(assetName), {}, false);
        require(bool(cpu), "ordinary CPU parse: " + std::string(assetName));
        require(ordinary.directParseCount() == 2,
                "ordinary scene oracle uses independent cache-disabled parses");
        render::MotionRenderPlanner ownPlanner, ordinaryPlanner;
        backends::direct2d::Backend ownBackend, ordinaryBackend;
        WarpCaptureSurface ownSurface, ordinarySurface;
        for (const auto& profile : testsupport::kDirect2DCaptureProfiles) {
            ownSurface.configure(profile);
            ordinarySurface.configure(profile);
            for (const auto frame : kFrames) {
                const auto label = std::string(assetName) + "_f" + std::to_string(frame)
                    + "_" + std::string(profile.id);
                auto ownPlan = emitPlan(*stream.stream, ownPlanner, frame, profile);
                auto ordinaryScene = ordinary.freshScene(
                    frame, profile.logicalWidth, profile.logicalHeight);
                require(ordinaryScene.instanceId == 0
                            && ordinaryScene.assetHandle == runtime::AssetHandle{700001, 1}
                            && !ordinaryScene.instanceHandle.valid(),
                        label + " unchanged ordinary oracle identity");
                ordinaryScene.instanceId = kOrdinaryOraclePlanId;
                auto ordinaryBuilt = ordinaryPlanner.build(std::move(ordinaryScene));
                require(bool(ordinaryBuilt), label + " ordinary plan");
                require(ordinaryBuilt.plan.statistics.unsupportedFeatureItemCount == 0,
                        label + " ordinary supported plan");

                const auto ordinaryWARP = renderPlan(
                    ordinarySurface, ordinaryBackend, ordinaryBuilt.plan);
                const auto ownWARP = renderPlan(ownSurface, ownBackend, ownPlan);
                const auto cpuPixels = ordinaryCpuPixels(*cpu, frame, profile);
                const auto comparison = testsupport::comparePixels(
                    cpuPixels, ownWARP, profile.pixelWidth, profile.pixelHeight);
                const auto decision = testsupport::evaluateComparison(comparison, policy);
                const bool exact = ordinaryWARP == ownWARP;
                const bool empty = expectedEmpty(assetName, frame);
                const auto ordinaryActive = activePixels(ordinaryWARP);
                const auto ownActive = activePixels(ownWARP);
                const auto cpuActive = activePixels(cpuPixels);
                const bool visibilityCorrect = empty
                    ? ordinaryBuilt.plan.drawItems.empty() && ownPlan.drawItems.empty()
                        && ordinaryActive == 0 && ownActive == 0 && cpuActive == 0
                    : ordinaryBuilt.plan.drawItems.size() == 1
                        && ownPlan.drawItems.size() == 1
                        && ordinaryActive > 0 && ownActive > 0 && cpuActive > 0;
                if (empty) ++emptyCases; else ++visibleCases;
                const bool passed = exact && decision.passed && visibilityCorrect;
                if (!passed) {
                    ++failures;
                    writeMismatch(artifactRoot, label + "-warp", ordinaryWARP,
                                  ownWARP, profile.pixelWidth, profile.pixelHeight);
                    if (!decision.passed)
                        writeMismatch(artifactRoot, label + "-cpu", cpuPixels,
                                      ownWARP, profile.pixelWidth, profile.pixelHeight);
                }
                metricsFile << assetName << '\t' << frame << '\t' << profile.id
                    << '\t' << (passed ? "PASS" : "FAIL")
                    << '\t' << exact << '\t' << ordinaryActive << '\t' << ownActive
                    << '\t' << std::fixed << std::setprecision(6)
                    << comparison.activeIoU << '\t' << comparison.alphaRelativeError
                    << '\t' << comparison.meanAbsoluteDifferenceAll
                    << '\t' << comparison.meanAbsoluteDifferenceActive
                    << '\t' << comparison.rootMeanSquareDifferenceAll
                    << '\t' << comparison.largeDifferenceFraction
                    << '\t' << unsigned(comparison.maxChannelDifference)
                    << '\t' << comparison.maximumBoundsDelta << '\t';
                if (!visibilityCorrect) metricsFile << "visibility;";
                if (!exact) metricsFile << "WARP bytes;";
                for (const auto& error : decision.failures) metricsFile << error << ';';
                metricsFile << '\n';
                ++cases;
            }
        }
        require(ordinary.directSampleCount() == seedSamples + 30,
                "ordinary scenes sampled independently for every case");
    }
    metricsFile.flush();
    require(bool(metricsFile), "pixel matrix finalize");
    std::cout << "PIXEL_MATRIX cases=" << cases << " visible=" << visibleCases
              << " empty=" << emptyCases << " failures=" << failures
              << " artifactRoot=" << artifactRoot.string() << '\n';
    require(cases == 180 && visibleCases == 160 && emptyCases == 20,
            "six assets times six frames times five profiles and visibility controls");
    require(failures == 0, "own/ordinary WARP or ordinary CPU pixel parity");
}

void lifetimeAndCache(const std::string& fixture, const fs::path& artifactRoot) {
    const auto allAssets = test::nativeEllipseTestAssets(fixture);
    const auto find = [&](std::string_view name) -> const std::string& {
        const auto item = std::find_if(allAssets.begin(), allAssets.end(),
            [name](const auto& asset) { return asset.name == name; });
        require(item != allAssets.end(), "lifetime asset present");
        return item->json;
    };
    const auto& profile = testsupport::kDirect2DCaptureProfiles[1];
    WarpCaptureSurface surface;
    surface.configure(profile);
    render::MotionRenderPlanner planner;
    backends::direct2d::Backend backend;
    auto owner = test::prepareOwnEllipseForTest(find("animated"));
    auto a = render::detail::OwnNativeEllipseStream::create(owner);
    auto b = render::detail::OwnNativeEllipseStream::create(owner);
    require(bool(a) && bool(b), "own A/B streams");
    auto a0 = emitPlan(*a.stream, planner, 0, profile);
    auto b0 = emitPlan(*b.stream, planner, 0, profile);
    require(a0.firstPlan && b0.firstPlan && a0.drawItems.size() == 1
                && b0.drawItems.size() == 1,
            "interleaved A/B first plans");
    require(a0.drawItems[0].geometry.scope == render::ResourceIdentityScope::Instance
                && b0.drawItems[0].geometry.scope == render::ResourceIdentityScope::Instance
                && a0.drawItems[0].geometry != b0.drawItems[0].geometry
                && a0.drawItems[0].paint.scope == render::ResourceIdentityScope::Asset
                && a0.drawItems[0].paint == b0.drawItems[0].paint,
            "animated geometry separated and static paint shared");
    const auto aid = a0.stamp.instanceId;
    const auto first = renderPlan(surface, backend, a0);
    const auto afterA = backend.diagnostics();
    const auto second = renderPlan(surface, backend, b0);
    const auto afterB = backend.diagnostics();
    require(first == second && activePixels(first) > 0,
            "A/B independent streams have equal visible pixels");
    require(afterB.geometryResourcesCreated == afterA.geometryResourcesCreated + 1,
            "distinct animated slot created for B");
    auto aRepeat = emitPlan(*a.stream, planner, 0, profile);
    require(!aRepeat.firstPlan && aRepeat.geometryUpdates.empty(),
            "repeated A frame preserves planner history");
    const auto repeated = renderPlan(surface, backend, aRepeat);
    const auto afterRepeat = backend.diagnostics();
    require(repeated == first
                && afterRepeat.geometryCacheHits > afterB.geometryCacheHits
                && afterRepeat.geometryResourcesCreated == afterB.geometryResourcesCreated,
            "A/B/A replay hits existing geometry and keeps pixels");
    auto newer = emitPlan(*a.stream, planner, 30, profile);
    require(!newer.firstPlan && !newer.geometryUpdates.empty(),
            "A newer frame updates geometry");
    const auto newerPixels = renderPlan(surface, backend, newer);
    require(newerPixels != first, "newer frame changes visible pixels");
    auto inactiveOwner = test::prepareOwnEllipseForTest(find("activity"));
    auto inactiveStream = render::detail::OwnNativeEllipseStream::create(inactiveOwner);
    require(bool(inactiveStream), "activity stream");
    auto activityVisible = emitPlan(*inactiveStream.stream, planner, 10, profile);
    require(activityVisible.drawItems.size() == 1,
            "activity stream begins with a visible plan");
    const auto activityPixels = renderPlan(surface, backend, activityVisible);
    require(activePixels(activityPixels) > 0,
            "retained activity plan has visible pixels");
    const auto afterActivityVisible = backend.diagnostics();
    auto inactive = emitPlan(*inactiveStream.stream, planner, 20, profile);
    require(inactive.drawItems.empty()
                && activePixels(renderPlan(surface, backend, inactive)) == 0,
            "inactive own plan draws transparent pixels");
    const auto afterInactive = backend.diagnostics();
    require(afterInactive.geometryResourcesCreated == afterActivityVisible.geometryResourcesCreated,
            "inactive draw creates no geometry");
    require(renderPlan(surface, backend, activityVisible) == activityPixels,
            "same-stream retained visible plan replays after inactive frame");
    const auto afterActivityReplay = backend.diagnostics();
    require(afterActivityReplay.geometryCacheHits > afterInactive.geometryCacheHits
                && afterActivityReplay.geometryResourcesCreated
                    == afterInactive.geometryResourcesCreated,
            "same-stream retained activity replay hits cached geometry");
    require(renderPlan(surface, backend, a0) == first,
            "retained old A plan replays after newer and inactive plans");
    planner.forgetInstance(aid);
    const auto afterForget = planner.diagnostics();
    require(afterForget.forgottenInstances >= 1,
            "planner records forgotten own stream history");
    auto rebuilt = planner.build(a0.sourceScene);
    require(bool(rebuilt) && rebuilt.plan.firstPlan
                && rebuilt.plan.stamp.planSequence == 1,
            "forgotten own stream can rebuild retained scene");
    a.stream.reset(); b.stream.reset(); owner.reset();
    inactiveStream.stream.reset(); inactiveOwner.reset();
    require(a0.sourceScene && a0.sourceScene->assetModel
                && a0.sourceScene->drawItems[0].canonicalPaint,
            "retained plan owns model and canonical paint after source destruction");
    require(renderPlan(surface, backend, activityVisible) == activityPixels,
            "activity visible pixels survive source and stream destruction");
    require(renderPlan(surface, backend, rebuilt.plan) == first,
            "rebuilt plan pixels survive owner and stream destruction");
    const auto beforeClear = backend.diagnostics();
    backend.clear();
    WarpCaptureSurface recreated;
    recreated.configure(profile);
    require(renderPlan(recreated, backend, a0, 2, 1) == first,
            "retained old plan replays after backend clear and surface recreation");
    const auto afterRecreate = backend.diagnostics();
    require(afterRecreate.resourceDomainResets > beforeClear.resourceDomainResets
                && afterRecreate.geometryResourcesCreated > beforeClear.geometryResourcesCreated,
            "domain recreation rebuilds geometry resources");
    backend.invalidateGraphicsDomain(2, 1);
    const auto afterInvalidate = backend.diagnostics();
    require(afterInvalidate.resourceDomainResets > afterRecreate.resourceDomainResets,
            "explicit active domain invalidation recorded");
    require(renderPlan(recreated, backend, a0, 2, 2) == first,
            "retained plan replays after domain generation advance");

    auto staticOwner = test::prepareOwnEllipseForTest(find("static-visible"));
    auto staticA = render::detail::OwnNativeEllipseStream::create(staticOwner);
    auto staticB = render::detail::OwnNativeEllipseStream::create(staticOwner);
    require(bool(staticA) && bool(staticB), "static streams");
    auto staticPlanA = emitPlan(*staticA.stream, planner, 0, profile);
    auto staticPlanB = emitPlan(*staticB.stream, planner, 0, profile);
    require(staticPlanA.drawItems.size() == 1 && staticPlanB.drawItems.size() == 1
                && staticPlanA.drawItems[0].geometry.scope == render::ResourceIdentityScope::Asset
                && staticPlanA.drawItems[0].geometry == staticPlanB.drawItems[0].geometry
                && staticPlanA.drawItems[0].paint == staticPlanB.drawItems[0].paint,
            "same-source static geometry and paint keys share");
    const auto staticPixels = renderPlan(recreated, backend, staticPlanA, 2, 2);
    const auto afterStaticA = backend.diagnostics();
    require(renderPlan(recreated, backend, staticPlanB, 2, 2) == staticPixels,
            "static sharing retains equal visible pixels");
    const auto afterStaticB = backend.diagnostics();
    require(activePixels(staticPixels) > 0
                && afterStaticB.geometryCacheHits > afterStaticA.geometryCacheHits
                && afterStaticB.geometryResourcesCreated == afterStaticA.geometryResourcesCreated,
            "static B uses shared backend geometry slot");

    std::ofstream evidence(artifactRoot / "cache-lifetime.tsv", std::ios::binary);
    require(bool(evidence), "cache evidence open");
    evidence << "stage\tdraw_calls\tgeometry_hits\tgeometry_misses"
                "\tgeometry_created\tgeometry_replaced\tdomain_resets\n";
    const auto row = [&](std::string_view label,
                         const backends::direct2d::BackendDiagnostics& d) {
        evidence << label << '\t' << d.drawCalls << '\t' << d.geometryCacheHits
                 << '\t' << d.geometryCacheMisses << '\t' << d.geometryResourcesCreated
                 << '\t' << d.geometryResourcesReplaced << '\t'
                 << d.resourceDomainResets << '\n';
    };
    row("A", afterA); row("B", afterB); row("A-repeat", afterRepeat);
    row("activity-visible", afterActivityVisible);
    row("activity-inactive", afterInactive);
    row("activity-retained", afterActivityReplay);
    row("clear", beforeClear); row("recreated", afterRecreate);
    row("invalidated", afterInvalidate); row("static-A", afterStaticA);
    row("static-B", afterStaticB); row("final", backend.diagnostics());
    evidence.flush();
    require(bool(evidence), "cache evidence finalize");
    std::cout << "CACHE_LIFETIME A/B/A=3 sameStreamInactiveReplay=1"
              << " retained=1 forget=1 domain=2 static=2"
              << " finalGeometryHits=" << backend.diagnostics().geometryCacheHits << '\n';
}
} // namespace

int main() {
    try {
        const auto artifactRoot = freshArtifactDirectory();
        const auto fixture = readFixture();
        pixelMatrix(fixture, artifactRoot);
        lifetimeAndCache(fixture, artifactRoot);
        std::cout << "own native ellipse WARP capture passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
