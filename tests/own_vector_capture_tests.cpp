#define NOMINMAX
#include "OwnNativeEllipseStream.hpp"
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorSceneOracle.hpp"
#include "support/WarpCaptureSurface.hpp"
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
namespace {
using namespace avemotion;
using namespace avemotion::test;
using testsupport::PixelBGRA;
using testsupport::WarpCaptureSurface;
namespace fs = std::filesystem;
void require(bool b, const std::string &message) { vectorRequire(b, message); }
void writePpm(const fs::path &path, const std::vector<PixelBGRA> &pixels, std::size_t width,
              std::size_t height) {
    require(pixels.size() == width * height, "PPM pixel dimensions");
    std::ofstream output(path, std::ios::binary);
    require(bool(output), "PPM output open: " + path.string());
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto &pixel : pixels) {
        const auto overWhite = [alpha = unsigned(pixel.a)](std::uint8_t value) {
            return static_cast<char>(std::min(255U, unsigned(value) + (255U - alpha)));
        };
        const std::array<char, 3> rgb{overWhite(pixel.r), overWhite(pixel.g), overWhite(pixel.b)};
        output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
    }
    require(bool(output), "PPM output write: " + path.string());
}

void writeMismatch(const fs::path &root, const std::string &label,
                   const std::vector<PixelBGRA> &expected, const std::vector<PixelBGRA> &actual,
                   std::size_t width, std::size_t height) {
    require(expected.size() == actual.size(), label + " mismatch storage");
    writePpm(root / (label + "-expected.ppm"), expected, width, height);
    writePpm(root / (label + "-actual.ppm"), actual, width, height);
    std::vector<PixelBGRA> difference(expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto delta = [](std::uint8_t a, std::uint8_t b) {
            return static_cast<std::uint8_t>(std::abs(int(a) - int(b)));
        };
        const auto color =
            std::max({delta(expected[i].b, actual[i].b), delta(expected[i].g, actual[i].g),
                      delta(expected[i].r, actual[i].r)});
        const auto alpha = delta(expected[i].a, actual[i].a);
        difference[i] = {static_cast<std::uint8_t>(std::min(255U, 2U * (unsigned(color) + alpha))),
                         static_cast<std::uint8_t>(std::min(255U, 4U * unsigned(alpha))),
                         static_cast<std::uint8_t>(std::min(255U, 4U * unsigned(color))), 255};
    }
    writePpm(root / (label + "-diff.ppm"), difference, width, height);
}

std::size_t activePixels(const std::vector<PixelBGRA> &pixels) {
    return static_cast<std::size_t>(std::count_if(
        pixels.begin(), pixels.end(), [](const PixelBGRA &pixel) { return pixel.a > 8; }));
}

std::vector<PixelBGRA> ordinaryCpuPixels(rlottie::Animation &animation, std::size_t frame,
                                         const testsupport::CaptureProfile &profile) {
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

std::vector<PixelBGRA> renderPlan(WarpCaptureSurface &surface, backends::direct2d::Backend &backend,
                                  const render::MotionRenderPlan &plan, std::uint64_t domain = 1,
                                  std::uint64_t generation = 1) {
    surface.begin();
    const auto drawn = backend.draw(plan, {.deviceContext = surface.context(),
                                           .graphicsDomainId = domain,
                                           .graphicsGeneration = generation});
    require(bool(drawn), "WARP draw: " + drawn.error.message);
    require(drawn.itemsSkipped == 0 && drawn.itemsDrawn == plan.drawItems.size(),
            "WARP supported draw coverage");
    surface.end();
    return surface.readPixels();
}

void clippingWitnesses(const fs::path &root) {
    // The ordinary inherited precomp clip is not executed by the WARP MVP.
    // Demonstrate that comparing only two unconstrained WARP scenes would miss
    // I1: the ordinary CPU respects this clip, while the bridge WARP does not.
    for (unsigned testCase = 0; testCase < 5; ++testCase) {
        const bool tall = testCase == 1;
        const unsigned width = tall ? 128U : 256U, height = tall ? 256U : 128U;
        auto child = testCase == 0 ? clippingShape(-8, 20, 40, 100)
                     : tall        ? clippingShape(20, -8, 100, 40)
                                   : clippingShape(2, 30, 60, 90, true, 3, 2);
        if (testCase >= 3) {
            child = clippingShape(20, 60, 64, 68, true, 1, 1);
            child = vectorReplace(child, "[[20,60],[64,60],[64,68],[20,68]]",
                                  testCase == 3 ? "[[20,64],[64,60],[64,68],[20,64]]"
                                                : "[[8,64],[64,60],[64,68],[8,64]]");
            child = vectorReplace(child, "\"ml\":4", testCase == 3 ? "\"ml\":16" : "\"ml\":2");
        }
        const auto json = clippingPrecomp(child);
        OwnVectorSceneOracle oracle(json);
        auto scene = oracle.freshScene(0, width, height);
        const auto compiled = vectorCompile(json);
        const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        auto own = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        const auto rejected = own.stream->emit(0, width, height);
        require(!rejected && !rejected.scene &&
                    rejected.code == render::detail::OwnNativeEllipseFrameCode::UnsupportedClipping,
                "crossing own frame explicitly rejected");
        auto cpu = rlottie::Animation::loadFromData(json, "clipping-cpu", {}, false);
        require(bool(cpu), "clipping CPU parse");
        testsupport::CaptureProfile profile{"clip-witness", width, height, width, height, 96, 96};
        auto clipped = ordinaryCpuPixels(*cpu, 0, profile);
        render::MotionRenderPlanner planner;
        auto plan = planner.build(std::move(scene));
        require(bool(plan) && plan.plan.statistics.unsupportedFeatureItemCount == 0,
                "witness ordinary plan");
        WarpCaptureSurface surface;
        surface.configure(profile);
        backends::direct2d::Backend backend;
        auto unconstrained = renderPlan(surface, backend, plan.plan);
        if (testCase >= 3) {
            // Both sharp and clipped miter witnesses must remain inside the
            // conservative envelope, measured from literal final coordinates.
            const double left = testCase == 3 ? 84.0 : 72.0;
            // Literal width12, radius6: ml16 gives102, ml2 gives18.
            const double padding = (testCase == 3 ? 102.0 : 18.0) + 2.0;
            for (unsigned y = 0; y < height; ++y)
                for (unsigned x = 0; x < width; ++x)
                    if (unconstrained[y * width + x].a != 0)
                        require(double(x) + 1 >= left - padding && double(x) <= 128 + padding &&
                                    double(y) + 1 >= 60 - padding && double(y) <= 68 + padding,
                                "sharp/clipped miter pixels bounded by radius envelope");
        }
        std::size_t cpuLetterbox = 0, warpLetterbox = 0;
        for (unsigned y = 0; y < height; ++y)
            for (unsigned x = 0; x < width; ++x)
                if (tall ? y < 63 : x < 63) {
                    cpuLetterbox += clipped[y * width + x].a != 0;
                    warpLetterbox += unconstrained[y * width + x].a != 0;
                }
        const auto label = "clip-witness-" + std::to_string(testCase);
        writeMismatch(root, label, clipped, unconstrained, width, height);
        std::cout << label << " cpu_letterbox=" << cpuLetterbox
                  << " unconstrained_warp_letterbox=" << warpLetterbox << " own=rejected\n";
        if (testCase < 3)
            require(cpuLetterbox == 0 && warpLetterbox > 0,
                    "independent CPU proves letterbox clipping is observable");
        else
            require(cpuLetterbox == 0 && warpLetterbox == 0,
                    "zero-handle sharp fixtures characterize conservative rejection, not spill");
    }
}

void run(const std::string &json, const fs::path &root, bool rectangles) {
    auto parsed = formats::detail::readOwnJson(json, {65536, 32});
    require(bool(parsed), "vector reader");
    auto prepared = render::detail::prepareOwnMotionAsset(*parsed.document);
    require(bool(prepared), prepared.path + prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    require(bool(stream), stream.message);
    OwnVectorSceneOracle oracle(json);
    auto cpu = rlottie::Animation::loadFromData(json, "vector-cpu", {}, false);
    require(bool(cpu), "ordinary CPU parse");
    render::MotionRenderPlanner ownPlanner, refPlanner;
    backends::direct2d::Backend ownBackend, refBackend;
    WarpCaptureSurface ownSurface, refSurface;
    std::ofstream metrics(root / "metrics.tsv");
    metrics << "width\theight\tframe\tdiff_pixels\tmax_channel\tmean_abs\tcpu_pass\tcpu_"
               "iou\tcpu_alpha\tcpu_mean\n";
    std::size_t failures = 0;
    std::vector<std::pair<unsigned, unsigned>> viewports{{128, 128}, {256, 256}, {512, 512}};
    if (rectangles) {
        viewports.emplace_back(512, 256);
        viewports.emplace_back(256, 512);
    } else {
        viewports.emplace_back(129, 129);
    }
    for (const auto [width, height] : viewports) {
        testsupport::CaptureProfile profile{"vector", width, height, width, height, 96, 96};
        ownSurface.configure(profile);
        refSurface.configure(profile);
        for (const auto frame : {0U, 10U, 15U, 20U, 45U, 90U, 110U, 135U, 179U}) {
            auto own = stream.stream->emit(frame, width, height);
            require(bool(own), own.message);
            auto ref = oracle.freshScene(frame, width, height);
            auto ownPlan = ownPlanner.build(std::move(*own.scene));
            auto refPlan = refPlanner.build(std::move(ref));
            require(bool(ownPlan) && bool(refPlan), "plans built");
            require(ownPlan.plan.statistics.unsupportedFeatureItemCount == 0 &&
                        refPlan.plan.statistics.unsupportedFeatureItemCount == 0,
                    "zero unsupported plan items");
            auto actual = renderPlan(ownSurface, ownBackend, ownPlan.plan);
            auto expected = renderPlan(refSurface, refBackend, refPlan.plan);
            auto cpuPixels = ordinaryCpuPixels(*cpu, frame, profile);
            const auto cpuMetrics = testsupport::comparePixels(cpuPixels, actual, width, height);
            const auto cpuDecision = testsupport::evaluateComparison(cpuMetrics, {});
            const auto warpMetrics = testsupport::comparePixels(expected, actual, width, height);
            std::size_t differences = 0;
            for (std::size_t i = 0; i < actual.size(); ++i)
                if (actual[i] != expected[i])
                    ++differences;
            const auto label = "s" + std::to_string(width) +
                               (width == height ? "" : "x" + std::to_string(height)) + "-f" +
                               std::to_string(frame);
            writeMismatch(root, label, expected, actual, width, height);
            writePpm(root / (label + "-cpu.ppm"), cpuPixels, width, height);
            std::ofstream ownRaw(root / (label + "-own.bgra"), std::ios::binary);
            ownRaw.write(reinterpret_cast<const char *>(actual.data()),
                         static_cast<std::streamsize>(actual.size() * sizeof(PixelBGRA)));
            std::ofstream refRaw(root / (label + "-reference.bgra"), std::ios::binary);
            refRaw.write(reinterpret_cast<const char *>(expected.data()),
                         static_cast<std::streamsize>(expected.size() * sizeof(PixelBGRA)));
            require(activePixels(actual) > 0 && activePixels(expected) > 0,
                    "nonempty expected frame");
            metrics << width << '\t' << height << '\t' << frame << '\t' << differences << '\t'
                    << unsigned(warpMetrics.maxChannelDifference) << '\t'
                    << warpMetrics.meanAbsoluteDifferenceAll << '\t' << cpuDecision.passed << '\t'
                    << cpuMetrics.activeIoU << '\t' << cpuMetrics.alphaRelativeError << '\t'
                    << cpuMetrics.meanAbsoluteDifferenceAll << '\n';
            std::cout << label << " diff_pixels=" << differences
                      << " max_channel=" << unsigned(warpMetrics.maxChannelDifference)
                      << " cpu_pass=" << cpuDecision.passed
                      << " drawn=" << ownPlan.plan.drawItems.size() << " skips=0\n";
            if (differences || !cpuDecision.passed)
                ++failures;
        }
    }
    require(failures == 0, "capture matrix failures=" + std::to_string(failures));
}
} // namespace
int main(int argc, char **argv) {
    const fs::path root =
        fs::path(AVEMOTION_VECTOR_CAPTURE_DIR) /
        ("run-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        fs::create_directories(root);
        std::cout << "captures=" << root.string() << '\n';
        clippingWitnesses(root);
        run(vectorInput(argc, argv), root, argc == 1);
        if (argc == 1) {
            fs::create_directories(root / "contained-precomp");
            run(clippingPrecomp(clippingShape(56, 56, 72, 72, true)), root / "contained-precomp",
                true);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
