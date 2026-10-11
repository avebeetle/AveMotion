#define NOMINMAX
#include "OwnNativeEllipseStream.hpp"
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorSceneOracle.hpp"
#include "support/OwnVectorGroupTestData.hpp"
#include "support/OwnVectorMetadataPositionTestData.hpp"
#include "support/OwnVectorGradientTestData.hpp"
#include "../backends/direct2d/LinearGradientBackendAccess.hpp"
#include "support/WarpCaptureSurface.hpp"
#include <array>
#include <chrono>
#include <cstdlib>
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

std::vector<PixelBGRA> renderLinearPlan(WarpCaptureSurface& surface,
    backends::direct2d::Backend& backend, const render::MotionRenderPlan& plan,
    const D2D1::Matrix3x2F& host = D2D1::Matrix3x2F::Identity(),
    std::uint64_t generation = 1) {
    surface.begin();
    surface.context()->SetTransform(host); // begin resets the host transform.
    const auto drawn = backends::direct2d::detail::LinearGradientBackendAccess::draw(backend,plan,
        {.deviceContext=surface.context(),.graphicsDomainId=7,.graphicsGeneration=generation});
    require(bool(drawn), "private WARP draw: " + drawn.error.message);
    require(drawn.itemsSkipped==0 && drawn.itemsDrawn==plan.drawItems.size(), "private WARP complete coverage");
    D2D1_MATRIX_3X2_F restored;
    surface.context()->GetTransform(&restored);
    require(restored.m11==host.m11 && restored.m12==host.m12 && restored.m21==host.m21 &&
            restored.m22==host.m22 && restored.dx==host.dx && restored.dy==host.dy,
            "private WARP restores saved host transform");
    surface.end(); return surface.readPixels();
}

struct LiteralGradient final {
    D2D1::Matrix3x2F matrix;
    D2D1_POINT_2F start{2,3}, end{18,3};
    std::vector<D2D1_GRADIENT_STOP> stops{{0,{1,0,0,1}},
        {0.5F,{127.F*(1.F/255),63.F*(1.F/255),31.F*(1.F/255),1}}, {1,{0,0,1,1}}};
    float opacity=1, x=0, height=10;
    D2D1_FILL_MODE rule=D2D1_FILL_MODE_WINDING;
};
// Expected construction uses authored literal data only. It never consumes an
// own scene, plan, paint, or backend resource.
std::vector<PixelBGRA> literalNative(WarpCaptureSurface& surface,
    const std::vector<LiteralGradient>& items, const D2D1::Matrix3x2F& viewport,
    const D2D1::Matrix3x2F& presentation=D2D1::Matrix3x2F::Identity(),
    const D2D1::Matrix3x2F& host=D2D1::Matrix3x2F::Identity()) {
    using Microsoft::WRL::ComPtr;
    surface.begin();
    ComPtr<ID2D1Factory> factory; surface.context()->GetFactory(&factory);
    for(const auto& item:items) {
        ComPtr<ID2D1PathGeometry> path;
        testsupport::captureSurfaceRequireHr(factory->CreatePathGeometry(&path),"literal path");
        ComPtr<ID2D1GeometrySink> sink;
        testsupport::captureSurfaceRequireHr(path->Open(&sink),"literal sink");
        sink->SetFillMode(item.rule);
        sink->BeginFigure({item.x,0},D2D1_FIGURE_BEGIN_FILLED);
        // Authored sh vertices have zero incoming/outgoing handles: preserve
        // their independent literal cubic command stream in native expected.
        sink->AddBezier(D2D1::BezierSegment({item.x,0},{item.x+20,0},{item.x+20,0}));
        sink->AddBezier(D2D1::BezierSegment({item.x+20,0},{item.x+20,item.height},{item.x+20,item.height}));
        sink->AddBezier(D2D1::BezierSegment({item.x+20,item.height},{item.x,item.height},{item.x,item.height}));
        sink->AddBezier(D2D1::BezierSegment({item.x,item.height},{item.x,0},{item.x,0}));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        testsupport::captureSurfaceRequireHr(sink->Close(),"literal close");
        ComPtr<ID2D1GradientStopCollection> stops;
        testsupport::captureSurfaceRequireHr(surface.context()->CreateGradientStopCollection(
            item.stops.data(),static_cast<UINT32>(item.stops.size()),D2D1_GAMMA_2_2,D2D1_EXTEND_MODE_CLAMP,&stops),"literal stops");
        ComPtr<ID2D1LinearGradientBrush> brush;
        testsupport::captureSurfaceRequireHr(surface.context()->CreateLinearGradientBrush(
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES{item.start,item.end},stops.Get(),&brush),"literal brush");
        brush->SetOpacity(item.opacity);
        surface.context()->SetTransform(item.matrix*viewport*presentation*host);
        surface.context()->FillGeometry(path.Get(),brush.Get());
    }
    surface.end(); return surface.readPixels();
}
void rawPixels(const fs::path& path,const std::vector<PixelBGRA>& pixels) {
    std::ofstream output(path,std::ios::binary);
    output.write(reinterpret_cast<const char*>(pixels.data()),static_cast<std::streamsize>(pixels.size()*sizeof(PixelBGRA)));
    require(bool(output),"raw gradient pixels written");
}
void gradientCaptures(const fs::path& root) {
    fs::create_directories(root);
    const std::string translated=R"(,"p":{"k":[40,40]})";
    const std::string bw=R"({"ty":"gf","t":1,"r":1,"s":{"k":[0,0]},"e":{"k":[20,20]},"g":{"p":2,"k":{"k":[0,0,0,0,1,1,1,1]}}})";
    struct Case {std::string name,json;unsigned mode;};
    const auto fractional=vectorReplace(gradientFill(),"\"k\":100","\"k\":50.5");
    const auto diagonal=vectorReplace(gradientFixture(bw,"",R"("s":{"k":[200,50]})"),
        "[20,10],[0,10]","[20,20],[0,20]");
    const auto midpoint=gradientFixture(vectorReplace(vectorReplace(bw,"[0,0]","[0.5,3]"),"[20,20]","[20.5,3]"),translated);
    const auto ordered=vectorReplace(vectorRoot(vectorGroupsLayer(
        gradientGroup(gradientFill(),translated)+","+gradientGroup(bw,translated))),"{\"fr\":","{\"v\":\"5.5.2\",\"fr\":");
    const std::vector<Case> cases{
        {"three",gradientFixture(gradientFill(),translated),0},
        {"two",gradientFixture(gradientPacked(2),translated),1},
        {"sixteen",gradientFixture(gradientPacked(16),translated),2},
        {"fractional",gradientFixture(fractional,translated+R"(,"o":{"k":75.25})",R"("o":{"k":66.5})"),3},
        {"animated-parent",gradientAnimatedFixture(),4},
        {"rotated-nonuniform",gradientFixture(gradientFill(),"",R"("p":{"k":[70,40]},"s":{"k":[200,50]},"r":{"k":90})"),5},
        {"diagonal-affine",diagonal,6}, {"encoded-midpoint",midpoint,7},
        {"evenodd",gradientFixture(vectorReplace(gradientFill(),"\"r\":1","\"r\":2"),translated),8},
        {"draw-order",ordered,9}};
    std::size_t failures=0, captures=0;
    std::ofstream metrics(root/"metrics.tsv");
    metrics << "case\twidth\theight\tframe\tnative_diff\tcpu_pass\tiou\talpha\tmean\n";
    for(const auto& c:cases) {
        std::ofstream ownedJson(root/(c.name+".json"),std::ios::binary);
        ownedJson<<c.json; require(bool(ownedJson),"owned gradient fixture bytes retained");
        const auto compiled=vectorCompile(c.json);
        require(bool(compiled),c.name+" admission: "+compiled.path+compiled.message);
        const auto prepared=render::detail::prepareOwnVectorAsset(compiled.prepared);
        require(bool(prepared),c.name+" preparation");
        auto stream=render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        require(bool(stream),c.name+" stream");
        auto cpu=rlottie::Animation::loadFromData(c.json,"gradient-cpu-"+c.name,{},false);
        require(bool(cpu),c.name+" independent pinned CPU");
        render::MotionRenderPlanner planner;
        backends::direct2d::Backend backend;
        WarpCaptureSurface actualSurface,expectedSurface;
        for(const auto [width,height]: {std::pair{128U,128U},{256U,256U},{256U,128U},{128U,256U}}) {
            testsupport::CaptureProfile profile{"gradient",width,height,width,height,96,96};
            actualSurface.configure(profile); expectedSurface.configure(profile);
            const float scale=width==256 && height==256 ? 2.F:1.F;
            const D2D1::Matrix3x2F viewport(scale,0,0,scale,width>height?64.F:0.F,height>width?64.F:0.F);
            for(const auto frame:{0U,10U,90U,179U,10U,0U,0U}) {
                auto emitted=stream.stream->emit(frame,width,height);
                require(bool(emitted),c.name+" sample");
                auto plan=planner.build(std::move(*emitted.scene)); require(bool(plan),c.name+" plan");
                require(plan.plan.statistics.unsupportedFeatureItemCount==plan.plan.drawItems.size(),
                    "public planner gradient accounting remains unsupported");
                LiteralGradient literal; literal.matrix={1,0,0,1,40,40};
                if(c.mode==1 || c.mode==2) {
                    literal.stops.clear(); const unsigned count=c.mode==1?2:16;
                    // Packed fixture positions are serialized to six decimals.
                    const float positions[]{0,.066667F,.133333F,.2F,.266667F,.333333F,.4F,.466667F,
                        .533333F,.6F,.666667F,.733333F,.8F,.866667F,.933333F,1};
                    for(unsigned i=0;i<count;++i) literal.stops.push_back({count==2?float(i):positions[i],{1,0,0,1}});
                }
                if(c.mode==3) { for(auto& stop:literal.stops) stop.color.a=128.F*(1.F/255); literal.opacity=.7525F*.665F; }
                if(c.mode==4) {const bool shifted=frame>=10 && frame<90;literal.matrix={3,0,0,.75F,shifted?13.F:9.F,shifted?13.F:9.F};literal.x=shifted?2.F:0.F;}
                if(c.mode==5) literal.matrix={-8.74227766e-8F,2,-.5F,-2.18556941e-8F,70,40};
                if(c.mode==6 || c.mode==7 || c.mode==9) {
                    literal.stops={{0,{0,0,0,1}},{1,{1,1,1,1}}};literal.start={0,0};literal.end={20,20};
                    if(c.mode==6) {literal.matrix={2,0,0,.5F,0,0};literal.height=20;}
                    if(c.mode==7) {literal.start={.5F,3};literal.end={20.5F,3};}
                }
                if(c.mode==8) literal.rule=D2D1_FILL_MODE_ALTERNATE;
                std::vector<LiteralGradient> expectedItems{literal};
                if(c.mode==9) {LiteralGradient top;top.matrix={1,0,0,1,40,40};expectedItems.push_back(top);}
                require(plan.plan.drawItems.size()==expectedItems.size(),"literal expected complete draw order");
                auto actual=renderLinearPlan(actualSurface,backend,plan.plan);
                auto expected=literalNative(expectedSurface,expectedItems,viewport);
                auto cpuPixels=ordinaryCpuPixels(*cpu,frame,profile);
                const auto cpuMetrics=testsupport::comparePixels(cpuPixels,actual,width,height);
                const auto decision=testsupport::evaluateComparison(cpuMetrics,{});
                std::size_t differences=0;
                for(std::size_t i=0;i<actual.size();++i) differences+=actual[i]!=expected[i];
                const auto label=c.name+"-"+std::to_string(width)+"x"+std::to_string(height)+"-f"+std::to_string(frame)+"-n"+std::to_string(captures++);
                writeMismatch(root,label,expected,actual,width,height);writePpm(root/(label+"-cpu.ppm"),cpuPixels,width,height);
                rawPixels(root/(label+"-own.bgra"),actual);rawPixels(root/(label+"-native.bgra"),expected);rawPixels(root/(label+"-cpu.bgra"),cpuPixels);
                metrics<<c.name<<'\t'<<width<<'\t'<<height<<'\t'<<frame<<'\t'<<differences<<'\t'<<decision.passed<<'\t'<<cpuMetrics.activeIoU<<'\t'<<cpuMetrics.alphaRelativeError<<'\t'<<cpuMetrics.meanAbsoluteDifferenceAll<<'\n';
                std::cout<<label<<" native_diff="<<differences<<" cpu_pass="<<decision.passed<<" drawn="<<plan.plan.drawItems.size()<<" skips=0\n";
                for(const auto& reason:decision.failures) std::cout<<"cpu failure: "<<reason<<'\n';
                if(differences || !decision.passed) ++failures;
                if(width==128 && height==128 && frame==0 && (c.mode==6 || c.mode==7)) {
                    // Affine center(20.5,2.5) inverse maps to(10.25,5),
                    // t=(10.25+5)/40=.38125 -> encoded RGB97.21875.
                    // World-endpoint projection instead gives t=.4970588.
                    const auto pixel=c.mode==6?actual[2*width+20]:actual[45*width+50];
                    const int wanted=c.mode==6?97:128;
                    std::cout<<c.name<<" interior_rgba="<<unsigned(pixel.r)<<','<<unsigned(pixel.g)<<','<<unsigned(pixel.b)<<','<<unsigned(pixel.a)
                        <<" expected="<<wanted<<","<<wanted<<","<<wanted<<",255 allowance=3\n";
                    require(std::abs(int(pixel.r)-wanted)<=3 && std::abs(int(pixel.g)-wanted)<=3 &&
                        std::abs(int(pixel.b)-wanted)<=3 && pixel.a==255,"literal inverse-local / encoded-gamma interior pixel");
                }
                if(width==128 && height==128 && frame==0) {
                    // Independent host/presentation witness; CPU remains viewport-space above.
                    auto presented=plan.plan;
                    for(auto& item:presented.drawItems) item.presentationTransform={1,0,0,1,3,5};
                    const D2D1::Matrix3x2F presentation(1,0,0,1,3,5),host(1,0,0,1,7,9);
                    const auto hostActual=renderLinearPlan(actualSurface,backend,presented,host,2);
                    const auto hostExpected=literalNative(expectedSurface,expectedItems,viewport,presentation,host);
                    writeMismatch(root,label+"-host",hostExpected,hostActual,width,height);
                    rawPixels(root/(label+"-host-own.bgra"),hostActual);rawPixels(root/(label+"-host-native.bgra"),hostExpected);
                    if(hostActual!=hostExpected) ++failures;
                }
            }
        }
    }
    require(failures==0,"gradient capture failures="+std::to_string(failures));
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
                    "zero-handle sharp fixtures characterize conservative rejection, "
                    "not spill");
    }
}
void widthZeroWitness() {
    const auto path = std::string("{\"a\":0,\"k\":") + vectorShape(2, false) + "}";
    const std::string stroke =
        R"({"ty":"st","c":{"a":0,"k":[1,0,0,1]},"w":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":10,"s":[8],"h":1},{"t":20,"s":[0]}]}})";
    const auto json =
        vectorReplace(vectorRoot(vectorReplace(vectorShapeLayer(path), "{\"ty\":\"tr\"}",
                                               stroke + R"(,{"ty":"tr","p":{"a":0,"k":[40,64]}})")),
                      "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
    const auto compiled = vectorCompile(json);
    require(bool(compiled), "width0 fixture compiles: " + compiled.path + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    require(bool(prepared), "width0 fixture prepares: " + prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    require(bool(stream), "width0 stream");
    auto cpu = rlottie::Animation::loadFromData(json, "width0-cpu", {}, false);
    require(bool(cpu), "width0 ordinary CPU");
    testsupport::CaptureProfile profile{"width0", 128, 128, 128, 128, 96, 96};
    WarpCaptureSurface surface;
    surface.configure(profile);
    backends::direct2d::Backend backend;
    render::MotionRenderPlanner planner;
    for (const auto &[frame, expectedWidth] : {std::pair{0U, 0.F}, {10U, 8.F}, {20U, 0.F}}) {
        auto emitted = stream.stream->emit(frame, 128, 128);
        require(bool(emitted), "width0 scene: " + emitted.message);
        require(emitted.scene->drawItems.size() == 1 &&
                    emitted.scene->drawItems[0].stroke.enabled &&
                    std::abs(emitted.scene->drawItems[0].stroke.width - expectedWidth) < 0.001F,
                "width0 enabled scene draw retains sampled width");
        auto plan = planner.build(std::move(*emitted.scene));
        require(bool(plan), "width0 render plan");
        const auto pixels = renderPlan(surface, backend, plan.plan);
        const auto cpuPixels = ordinaryCpuPixels(*cpu, frame, profile);
        std::cout << "width0 frame=" << frame << " cpu_active=" << activePixels(cpuPixels)
                  << " warp_active=" << activePixels(pixels) << '\n';
        require((activePixels(cpuPixels) == 0) == (expectedWidth == 0),
                "ordinary CPU width0 raster characterization");
        require((activePixels(pixels) == 0) == (expectedWidth == 0),
                "WARP width0 raster follows enabled stroke coverage");
    }
}

void run(const std::string &json, const fs::path &root, bool rectangles, bool groups = false,
         bool positions = false) {
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
    metrics << "width\theight\tframe\tdiff_pixels\tmax_channel\tmean_abs\tcpu_"
               "pass\tcpu_"
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
        const std::vector<unsigned> frames = positions ? std::vector<unsigned>{0,14,15,20,25,40,59,60,70,179,70,60,59,40,25,20,15,14,0,15,15} :
            groups ? std::vector<unsigned>{0,50,68,69,100,110,179,68,69} :
            std::vector<unsigned>{0,10,15,20,33,34,45,67,68,90,110,127,128,135,137,138,179};
        for (const auto frame : frames) {
            auto own = stream.stream->emit(frame, width, height);
            require(bool(own), own.message);
            auto ref = oracle.freshScene(frame, width, height);
            oracle.assertRedundantClips(ref);
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
            if (differences && frame == 0) {
                std::cout.precision(12);
                for (std::size_t i = 0; i < ownPlan.plan.drawItems.size(); ++i) {
                    auto isolatedOwn = ownPlan.plan, isolatedReference = refPlan.plan;
                    isolatedOwn.drawItems = {ownPlan.plan.drawItems[i]};
                    isolatedReference.drawItems = {refPlan.plan.drawItems[i]};
                    const auto op = renderPlan(ownSurface, ownBackend, isolatedOwn);
                    const auto rp = renderPlan(refSurface, refBackend, isolatedReference);
                    const auto delta = testsupport::comparePixels(rp, op, width, height);
                    if (delta.maxChannelDifference != 0)
                        std::cout << "isolated mismatch draw=" << i
                                  << " max_channel=" << unsigned(delta.maxChannelDifference)
                                  << " own_width="
                                  << ownPlan.plan.sourceScene->drawItems[i].stroke.width
                                  << " ref_width="
                                  << refPlan.plan.sourceScene->drawItems[i].stroke.width << '\n';
                }
            }
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
    char *environmentRoot = nullptr;
    std::size_t environmentLength = 0;
    _dupenv_s(&environmentRoot, &environmentLength, "AVEMOTION_TASK_CAPTURE_ROOT");
    std::unique_ptr<char, decltype(&std::free)> requestedRoot(environmentRoot, &std::free);
    const fs::path root =
        fs::path(requestedRoot && *requestedRoot ? requestedRoot.get()
                                                 : AVEMOTION_VECTOR_CAPTURE_DIR) /
        ("run-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        fs::create_directories(root);
        std::cout << "captures=" << root.string() << '\n';
        if (argc == 1)
            widthZeroWitness();
        clippingWitnesses(root);
        run(vectorInput(argc, argv), root, argc == 1);
        if (argc == 1) {
            gradientCaptures(root / "static-linear-gradient");
            fs::create_directories(root / "metadata-position");
            run(metadataPositionFixture(), root / "metadata-position", true, false, true);
            fs::create_directories(root / "metadata-position-occurrences");
            run(metadataPositionFixture(true,true), root / "metadata-position-occurrences", true, false, true);
            fs::create_directories(root / "bounded-groups");
            run(vectorGroupFixture(), root / "bounded-groups", true, true);
            fs::create_directories(root / "contained-precomp");
            run(clippingPrecomp(clippingShape(56, 56, 72, 72, true)), root / "contained-precomp",
                true);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
