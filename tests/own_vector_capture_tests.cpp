#define NOMINMAX
#include "OwnNativeEllipseStream.hpp"
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorSceneOracle.hpp"
#include "support/OwnVectorSceneComparison.hpp"
#include "support/OwnVectorGroupTestData.hpp"
#include "support/OwnVectorMetadataPositionTestData.hpp"
#include "support/OwnVectorGradientTestData.hpp"
#include "support/OwnVectorMatteTestData.hpp"
#include "support/OwnVectorMixedTestData.hpp"
#include "../backends/direct2d/LinearGradientBackendAccess.hpp"
#include "../backends/direct2d/AlphaMatteBackendAccess.hpp"
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

struct LiteralMatteRect final { D2D1_RECT_F rect; D2D1_COLOR_F color; };
// This independent raster realizes the rectangle intersection and alpha product
// directly. It consumes authored literals only, never an own scene or backend.
std::vector<PixelBGRA> literalMatteRaster(WarpCaptureSurface& surface,
    const std::vector<LiteralMatteRect>& rects,const D2D1::Matrix3x2F& transform,
    bool opaqueHost,bool clip) {
    surface.begin(); auto* context=surface.context();
    if(opaqueHost) context->Clear(D2D1::ColorF(0,0,1,1));
    if(clip) context->PushAxisAlignedClip({0,0,64,256},D2D1_ANTIALIAS_MODE_ALIASED);
    context->SetTransform(transform);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    require(SUCCEEDED(context->CreateSolidColorBrush({0,0,0,0},&brush)),"literal matte brush");
    for(const auto& rect:rects) { brush->SetColor(rect.color);context->FillRectangle(rect.rect,brush.Get()); }
    if(clip) context->PopAxisAlignedClip();
    surface.end();return surface.readPixels();
}
std::vector<PixelBGRA> renderAlphaPlan(WarpCaptureSurface& surface,backends::direct2d::Backend& backend,
    const render::MotionRenderPlan& plan,const D2D1::Matrix3x2F& host,bool opaqueHost,bool clip,
    std::uint64_t generation) {
    surface.begin();auto* context=surface.context();
    if(opaqueHost) context->Clear(D2D1::ColorF(0,0,1,1));
    if(clip) context->PushAxisAlignedClip({0,0,64,256},D2D1_ANTIALIAS_MODE_ALIASED);
    context->SetTransform(host);
    Microsoft::WRL::ComPtr<ID2D1Image> target,restoredTarget;
    context->GetTarget(&target);
    const auto drawn=backends::direct2d::detail::AlphaMatteBackendAccess::draw(backend,plan,
        {.deviceContext=context,.graphicsDomainId=81,.graphicsGeneration=generation});
    require(bool(drawn),"owned alpha WARP: "+drawn.error.message);
    require(drawn.itemsDrawn==plan.drawItems.size() && drawn.itemsSkipped==0,"owned alpha source primitive coverage");
    context->GetTarget(&restoredTarget);D2D1_MATRIX_3X2_F restored;context->GetTransform(&restored);
    require(target.Get()==restoredTarget.Get() && restored.m11==host.m11 && restored.m12==host.m12 &&
        restored.m21==host.m21 && restored.m22==host.m22 && restored.dx==host.dx && restored.dy==host.dy,
        "native alpha restores original host target/transform");
    if(clip) context->PopAxisAlignedClip();
    surface.end();return surface.readPixels();
}
// Independent original oracle: ordinary paths/paint and parsed paint matrix.
// The optional literal matrix override is used only by the precision witness.
std::vector<PixelBGRA> ordinaryPartyDraw(WarpCaptureSurface &surface,
    const runtime::EvaluatedDrawItem &draw, const VMatrix &paintMatrix, bool local = false,
    const runtime::AffineTransform *literalMatrix = nullptr) {
    using Microsoft::WRL::ComPtr;
    surface.begin(); auto *context=surface.context();
    ComPtr<ID2D1Factory> factory; context->GetFactory(&factory);
    ComPtr<ID2D1PathGeometry> geometry;
    testsupport::captureSurfaceRequireHr(factory->CreatePathGeometry(&geometry),"ordinary Party geometry");
    ComPtr<ID2D1GeometrySink> sink;
    testsupport::captureSurfaceRequireHr(geometry->Open(&sink),"ordinary Party sink");
    sink->SetFillMode(draw.fillRule==runtime::FillRule::EvenOdd?D2D1_FILL_MODE_ALTERNATE:D2D1_FILL_MODE_WINDING);
    const auto &path=local?draw.localPath:draw.path;
    if(local) context->SetTransform(D2D1::Matrix3x2F(paintMatrix.m_11(),paintMatrix.m_12(),paintMatrix.m_21(),paintMatrix.m_22(),paintMatrix.m_tx(),paintMatrix.m_ty()));
    if(literalMatrix) {const auto &m=*literalMatrix;context->SetTransform(D2D1::Matrix3x2F(m.m11,m.m12,m.m21,m.m22,m.dx,m.dy));}
    std::size_t point=0; bool open=false;
    const auto next=[&]() { const auto p=path.points.at(point++);return D2D1::Point2F(p.x,p.y); };
    for(const auto verb:path.verbs) {
        if(verb==runtime::PathVerb::MoveTo) {
            if(open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
            sink->BeginFigure(next(),D2D1_FIGURE_BEGIN_FILLED);open=true;
        } else if(verb==runtime::PathVerb::LineTo) sink->AddLine(next());
        else if(verb==runtime::PathVerb::CubicTo) {
            const auto a=next(),b=next(),c=next();sink->AddBezier(D2D1::BezierSegment(a,b,c));
        } else {sink->EndFigure(D2D1_FIGURE_END_CLOSED);open=false;}
    }
    if(open) sink->EndFigure(D2D1_FIGURE_END_OPEN);
    require(point==path.points.size(),"ordinary Party path consumed");
    testsupport::captureSurfaceRequireHr(sink->Close(),"ordinary Party path close");
    require(!draw.stroke.enabled,"original Party independent fill scope");
    const auto color=[](runtime::Color8 c) {return D2D1::ColorF(c.r/255.F,c.g/255.F,c.b/255.F,c.a/255.F);};
    if(draw.paint.kind==runtime::PaintKind::Solid) {
        ComPtr<ID2D1SolidColorBrush> brush;
        testsupport::captureSurfaceRequireHr(context->CreateSolidColorBrush(color(draw.paint.solid),&brush),"ordinary Party solid");
        context->FillGeometry(geometry.Get(),brush.Get());
    } else {
        require(draw.paint.kind==runtime::PaintKind::Gradient && draw.paint.gradient.kind==runtime::GradientKind::Linear,
                "original Party independent linear scope");
        std::vector<D2D1_GRADIENT_STOP> values;
        for(const auto &stop:draw.paint.gradient.stops) values.push_back({stop.position,color(stop.color)});
        ComPtr<ID2D1GradientStopCollection> stops;
        testsupport::captureSurfaceRequireHr(context->CreateGradientStopCollection(values.data(),static_cast<UINT32>(values.size()),
            D2D1_GAMMA_2_2,D2D1_EXTEND_MODE_CLAMP,&stops),"ordinary Party stops");
        bool invertible=false; const auto inverse=paintMatrix.inverted(&invertible);
        require(invertible,"ordinary Party paint matrix invertible");
        const auto start=inverse.map(draw.paint.gradient.start.x,draw.paint.gradient.start.y);
        const auto end=inverse.map(draw.paint.gradient.end.x,draw.paint.gradient.end.y);
        ComPtr<ID2D1LinearGradientBrush> brush;
        testsupport::captureSurfaceRequireHr(context->CreateLinearGradientBrush(
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES{{start.x(),start.y()},{end.x(),end.y()}},stops.Get(),&brush),"ordinary Party gradient");
        if(!local) brush->SetTransform(D2D1::Matrix3x2F(paintMatrix.m_11(),paintMatrix.m_12(),paintMatrix.m_21(),paintMatrix.m_22(),paintMatrix.m_tx(),paintMatrix.m_ty()));
        context->FillGeometry(geometry.Get(),brush.Get());
    }
    surface.end();return surface.readPixels();
}
std::vector<PixelBGRA> ordinaryPartyRaster(WarpCaptureSurface &surface,
    const runtime::EvaluatedScene &scene,const OwnVectorSceneOracle &oracle,
    const fs::path &root,const std::string &label,bool local = false) {
    std::map<int,std::vector<PixelBGRA>> inputs;
    std::vector<int> order;
    std::ofstream rawScene(root/(label+"-ordinary.tsv"));
    rawScene.precision(12);
    for(const auto &draw:scene.drawItems) {
        const auto &layer=scene.layers.at(draw.layerIndex);
        const auto id=oracle.instances().at(layer.modelLayer.value).data->id();
        auto pixels=ordinaryPartyDraw(surface,draw,oracle.matrices().at(draw.modelNode.value),local);
        const auto [found,inserted]=inputs.try_emplace(id,pixels.size());
        if(inserted) order.push_back(id);
        else for(const auto &pair:originalPartyPairs)
            require(id!=pair.provider && id!=pair.content,"original Party one paired independent paint");
        for(std::size_t i=0;i<pixels.size();++i) {
            const auto source=pixels[i];auto &dst=found->second[i];const auto a=255U-source.a;
            const auto over=[&](unsigned channel,unsigned background) {
                return static_cast<std::uint8_t>(std::min(255U,channel+(background*a+127U)/255U));
            };
            dst={over(source.b,dst.b),over(source.g,dst.g),over(source.r,dst.r),over(source.a,dst.a)};
        }
        rawScene<<"draw\t"<<id<<'\t'<<unsigned(layer.matte)<<'\t'<<layer.visible<<'\t'<<layer.opacity<<'\t'<<oracle.drawRole(scene,draw)<<'\n';
        for(const auto p:draw.path.points) rawScene<<"point\t"<<p.x<<'\t'<<p.y<<'\n';
        rawScene<<"solid\t"<<unsigned(draw.paint.solid.r)<<'\t'<<unsigned(draw.paint.solid.g)<<'\t'<<unsigned(draw.paint.solid.b)<<'\t'<<unsigned(draw.paint.solid.a)<<'\n';
        const auto &g=draw.paint.gradient;
        rawScene<<"gradient\t"<<g.start.x<<'\t'<<g.start.y<<'\t'<<g.end.x<<'\t'<<g.end.y<<'\n';
        for(const auto stop:g.stops) rawScene<<"stop\t"<<stop.position<<'\t'<<unsigned(stop.color.r)<<'\t'<<unsigned(stop.color.g)<<'\t'<<unsigned(stop.color.b)<<'\t'<<unsigned(stop.color.a)<<'\n';
    }
    std::vector<PixelBGRA> expected(scene.viewportWidth*scene.viewportHeight);
    const auto mul=[](unsigned v,unsigned a) {return (v*a+127U)/255U;};
    for(const auto id:order) {
        bool provider=false; int mask=0;
        for(const auto &pair:originalPartyPairs) {
            provider |= id==pair.provider;
            if(id==pair.content) mask=pair.provider;
        }
        if(provider || (mask && !inputs.contains(mask))) continue;
        const auto &input=inputs.at(id);
        for(std::size_t i=0;i<input.size();++i) {
            auto source=input[i];
            if(mask) { const auto a=inputs.at(mask)[i].a; source={static_cast<std::uint8_t>(mul(source.b,a)),static_cast<std::uint8_t>(mul(source.g,a)),static_cast<std::uint8_t>(mul(source.r,a)),static_cast<std::uint8_t>(mul(source.a,a))}; }
            auto &dst=expected[i];const auto a=255U-source.a;
            dst={static_cast<std::uint8_t>(std::min(255U,unsigned(source.b)+mul(dst.b,a))),
                static_cast<std::uint8_t>(std::min(255U,unsigned(source.g)+mul(dst.g,a))),
                static_cast<std::uint8_t>(std::min(255U,unsigned(source.r)+mul(dst.r,a))),
                static_cast<std::uint8_t>(std::min(255U,unsigned(source.a)+mul(dst.a,a)))};
        }
    }
    require(bool(rawScene),"original ordinary raw fields retained");
    return expected;
}
void originalPartyPrecisionWitness(WarpCaptureSurface &surface,
    const runtime::EvaluatedScene &reference,const OwnVectorSceneOracle &oracle,const fs::path &root) {
    // Authored frame0 numeric literals: group G, popper L, null transform-parent P,
    // aspect-fit V. Neither operand construction reads own evaluation output.
    // Independently audited float32 operands. General affine operations make the
    // association explicit; VMatrix's optimized type branches are not this probe.
    using Matrix=runtime::AffineTransform;
    const Matrix group{1,0,0,1,321.80999755859375F,327.3299865722656F};
    const Matrix layer{.7071067690849304F,-.7071067690849304F,.7071067690849304F,.7071067690849304F,-463.310302734375F,-428.4501953125F};
    const Matrix parent{.7071067690849304F,.7071067690849304F,-.7071067690849304F,.7071067690849304F,66,982};
    const Matrix viewport{.09375F,0,0,.09375F,48,0};
    const auto multiply=[](const Matrix &a,const Matrix &b) {
        const auto sum=[](float x,float y) {volatile float rounded=x+y;return rounded;};
        const auto product=[](float x,float y) {volatile float rounded=x*y;return rounded;};
        return Matrix{sum(product(a.m11,b.m11),product(a.m12,b.m21)),
            sum(product(a.m11,b.m12),product(a.m12,b.m22)),
            sum(product(a.m21,b.m11),product(a.m22,b.m21)),
            sum(product(a.m21,b.m12),product(a.m22,b.m22)),
            sum(sum(product(a.dx,b.m11),product(a.dy,b.m21)),b.dx),
            sum(sum(product(a.dx,b.m12),product(a.dy,b.m22)),b.dy)};
    };
    const auto worldThenViewport=multiply(multiply(group,multiply(layer,parent)),viewport);
    const auto viewportBeforeGroup=multiply(group,multiply(multiply(layer,parent),viewport));
    require(worldThenViewport.dy==63.633758544921875F &&
                viewportBeforeGroup.dy==63.633754730224609375F,
            "literal float32 viewport association values");
    const auto &draw=reference.drawItems.at(0);
    require(oracle.drawRole(reference,draw)==
        "root/layer:11[10]|root/layer:11/shapes/5/it/0|root/layer:11/shapes/5/it/1",
        "literal original precision witness role");
    const auto pinned=ordinaryPartyDraw(surface,draw,oracle.matrices().at(draw.modelNode.value),true);
    const auto first=worldThenViewport,second=viewportBeforeGroup;
    require(first.m11==second.m11 && first.m12==second.m12 && first.m21==second.m21 &&
                first.m22==second.m22 && first.dx==second.dx && first.dy!=second.dy,
            "literal native sensitivity changes only dy");
    const auto oldPixels=ordinaryPartyDraw(surface,draw,oracle.matrices().at(draw.modelNode.value),true,&first);
    const auto vendorPixels=ordinaryPartyDraw(surface,draw,oracle.matrices().at(draw.modelNode.value),true,&second);
    rawPixels(root/"literal-world-then-viewport.bgra",oldPixels);
    rawPixels(root/"literal-viewport-before-group.bgra",vendorPixels);
    const auto match=testsupport::comparePixels(pinned,vendorPixels,192,96);
    const auto sensitivity=testsupport::comparePixels(oldPixels,vendorPixels,192,96);
    require(match.maxChannelDifference<=3,"literal vendor association matches independent parsed source native raster");
    require(sensitivity.maxChannelDifference>3,"native AA witnesses sensitivity to literal association delta");
    std::cout.precision(12);
    std::cout<<"literal precision world_dy="<<first.dy<<" viewport_before_group_dy="<<second.dy
        <<" vendor_native_max="<<unsigned(match.maxChannelDifference)
        <<" sensitivity_max="<<unsigned(sensitivity.maxChannelDifference)<<'\n';
}
void originalPartyCaptures(const std::string &json,const fs::path &root) {
    const auto compiled=vectorCompile(json);require(bool(compiled),compiled.path+compiled.message);
    const auto prepared=render::detail::prepareOwnVectorAsset(compiled.prepared);require(bool(prepared),prepared.message);
    auto stream=render::detail::OwnNativeEllipseStream::create(prepared.prepared);require(bool(stream),stream.message);
    OwnVectorSceneOracle oracle(json);render::MotionRenderPlanner planner;
    WarpCaptureSurface ownSurface,nativeSurface;backends::direct2d::Backend backend;
    std::ofstream metrics(root/"party-metrics.tsv");
    metrics<<"width\theight\tframe\tnative_max\tnative_pass\tnative_iou\tnative_alpha\tnative_mean_all\tnative_mean_active\tnative_large_fraction\tnative_bounds\tnative_over3\tcpu_pass\tiou\talpha\tmean_all\tmean_active\tlarge_fraction\tbounds\tdrawn\tskipped\n";
    unsigned failures=0,captures=0;
    for(const auto [width,height]:{std::pair{129U,129U},std::pair{192U,96U}}) {
        testsupport::CaptureProfile profile{"original-Party",width,height,width,height,96,96};
        ownSurface.configure(profile);nativeSurface.configure(profile);
        for(const auto frame:{0U,19U,20U,25U,30U,31U,50U,81U,82U,91U}) {
            auto own=stream.stream->emit(frame,width,height);require(bool(own),own.message);
            assertOriginalParty(*prepared.prepared,*own.scene);
            const auto reference=oracle.freshScene(frame,width,height,true);
            if(width==192 && frame==0) originalPartyPrecisionWitness(nativeSurface,reference,oracle,root);
            auto plan=planner.build(std::move(*own.scene));require(bool(plan),"original Party plan");
            const auto label="party-"+std::to_string(width)+"x"+std::to_string(height)+"-f"+std::to_string(frame);
            auto cpu=rlottie::Animation::loadFromData(json,"fresh-original-Party-"+label,{},false);
            require(bool(cpu),"fresh original Party pinned CPU parse");
            const auto cpuPixels=ordinaryCpuPixels(*cpu,frame,profile);rawPixels(root/(label+"-cpu.bgra"),cpuPixels);
            const auto expected=ordinaryPartyRaster(nativeSurface,reference,oracle,root,label);
            const auto actual=renderAlphaPlan(ownSurface,backend,plan.plan,D2D1::Matrix3x2F::Identity(),false,false,1);
            const auto localExpected=ordinaryPartyRaster(nativeSurface,reference,oracle,root,label+"-local",true);
            rawPixels(root/(label+"-native-local.bgra"),localExpected);
            std::cout<<label<<" diagnostic_local_native_max="<<unsigned(testsupport::comparePixels(localExpected,actual,width,height).maxChannelDifference)<<'\n';
            rawPixels(root/(label+"-own.bgra"),actual);rawPixels(root/(label+"-native.bgra"),expected);
            writeMismatch(root,label,expected,actual,width,height);
            if(frame==0) {
                // Authored Group6 fill is [1,.757000014361,.027000000898,1].
                // Hand-checked interiors stay inside its opaque yellow area.
                const unsigned x=width==129?42U:72U,y=width==129?51U:44U;
                const PixelBGRA literal{6,193,255,255};
                for(const auto &image:{std::cref(actual),std::cref(expected)}) {
                    const auto pixel=image.get().at(y*width+x);
                    require(std::abs(int(pixel.b)-literal.b)<=3 && std::abs(int(pixel.g)-literal.g)<=3 &&
                        std::abs(int(pixel.r)-literal.r)<=3 && std::abs(int(pixel.a)-literal.a)<=3,
                        "original Party authored yellow interior allowance3");
                    require(image.get().front()==PixelBGRA{},"original Party literal transparent corner");
                }
                std::cout<<label<<" literal interior="<<x<<','<<y<<" BGRA=6,193,255,255 allowance=3\n";
            }
            const auto native=testsupport::comparePixels(expected,actual,width,height);
            const auto nativeDecision=testsupport::evaluateComparison(native,{});
            std::size_t over3=0;
            for(std::size_t i=0;i<actual.size();++i)
                over3 += std::abs(int(actual[i].b)-expected[i].b)>3 || std::abs(int(actual[i].g)-expected[i].g)>3 ||
                    std::abs(int(actual[i].r)-expected[i].r)>3 || std::abs(int(actual[i].a)-expected[i].a)>3;
            const auto cpuMetrics=testsupport::comparePixels(cpuPixels,actual,width,height);
            const auto decision=testsupport::evaluateComparison(cpuMetrics,{});
            for(const auto &reason:decision.failures) std::cout<<label<<" CPU failure: "<<reason<<'\n';
            for(const auto &reason:nativeDecision.failures) std::cout<<label<<" native failure: "<<reason<<'\n';
            metrics<<width<<'\t'<<height<<'\t'<<frame<<'\t'<<unsigned(native.maxChannelDifference)<<'\t'<<nativeDecision.passed<<'\t'<<native.activeIoU<<'\t'<<native.alphaRelativeError<<'\t'<<native.meanAbsoluteDifferenceAll<<'\t'<<native.meanAbsoluteDifferenceActive<<'\t'<<native.largeDifferenceFraction<<'\t'<<native.maximumBoundsDelta<<'\t'<<over3<<'\t'<<decision.passed<<'\t'<<cpuMetrics.activeIoU<<'\t'<<cpuMetrics.alphaRelativeError<<'\t'<<cpuMetrics.meanAbsoluteDifferenceAll<<'\t'<<cpuMetrics.meanAbsoluteDifferenceActive<<'\t'<<cpuMetrics.largeDifferenceFraction<<'\t'<<cpuMetrics.maximumBoundsDelta<<'\t'<<plan.plan.drawItems.size()<<"\t0\n";
            std::cout<<label<<" native_max="<<unsigned(native.maxChannelDifference)<<" native_pass="<<nativeDecision.passed<<" native_over3="<<over3<<" cpu_pass="<<decision.passed<<" drawn="<<plan.plan.drawItems.size()<<" skips=0\n";
            failures+=!nativeDecision.passed || !decision.passed;++captures;
        }
    }
    require(failures==0,"original Party capture failures="+std::to_string(failures));
    std::cout<<"original Party captures="<<captures<<" independent native/default policy="<<captures<<" fresh pinned CPU/default policy="<<captures<<" authored interiors allowance=3\n";
}
void alphaCaptures(const fs::path& root) {
    fs::create_directories(root);
    const auto shape=[](int id,int right,const std::string& color,const std::string& role,
                        const std::string& opacity) {
        auto json=clippingShape(24,24,right,72);
        json=vectorReplace(json,"\"ind\":1","\"ind\":"+std::to_string(id));
        json=vectorReplace(json,"\"ks\":{}","\"ks\":{\"o\":{\"k\":"+opacity+"}}");
        json=vectorReplace(json,"[1,0,0,1]",color);
        return vectorReplace(json,"\"ty\":4","\"ty\":4"+role);
    };
    const auto fixture=[&](const std::string& rgb,const std::string& contentOpacity="100",bool lower=false,bool difference=false) {
        const auto provider=shape(1,64,rgb,",\"td\":1","50");
        const auto content=shape(2,72,"[1,0,0,1]",",\"tt\":1",contentOpacity);
        const auto bottom=shape(3,88,"[0,0,1,1]",difference?",\"bm\":10":"","100");
        return vectorReplace(vectorRoot(provider+","+content+(lower?","+bottom:"")),"{\"fr\":","{\"v\":\"5.5.2\",\"fr\":");
    };
    auto inactive=fixture("[0,1,0,1]");
    inactive=vectorReplace(inactive,"\"ind\":1,\"ip\":0,\"op\":180",
        "\"ind\":1,\"ip\":20,\"op\":31");
    auto animated=fixture("[0,1,0,1]");
    const std::string path=R"({"i":[[0,0],[0,0],[0,0],[0,0]],"o":[[0,0],[0,0],[0,0],[0,0]],"v":[[24,24],[64,24],[64,72],[24,72]],"c":true})";
    const auto shifted=vectorReplace(path,"[[24,24],[64,24],[64,72],[24,72]]","[[34,24],[74,24],[74,72],[34,72]]");
    animated=vectorReplace(animated,"\"ks\":{\"k\":"+path+"}","\"ks\":{\"a\":1,\"k\":[{\"t\":0,\"s\":["+path+"] ,\"h\":1},{\"t\":20,\"s\":["+shifted+"],\"h\":1},{\"t\":31,\"s\":["+path+"]}]}" );
    auto repeated=fixture("[0,1,0,1]");
    const auto second=shape(4,64,"[1,1,0,1]",",\"td\":1","50")+","+
        shape(5,72,"[0,0,1,1]",",\"tt\":1","100");
    repeated=vectorReplace(repeated,"\"layers\":[","\"layers\":["+second+",");
    const auto zero=vectorReplace(fixture("[0,1,0,1]"),"\"o\":{\"k\":50}","\"o\":{\"k\":0}");
    const auto staticLinear=vectorReplace(fixture("[0,1,0,1]"),
        R"({"ty":"fl","c":{"k":[1,0,0,1]},"o":{"k":100}})",
        R"({"ty":"gf","t":1,"r":1,"o":{"k":100},"s":{"k":[24,24]},"e":{"k":[72,72]},"g":{"p":2,"k":{"k":[0,1,0,0,1,1,0,0]}}})");
    const auto stroke=vectorReplace(fixture("[0,1,0,1]"),
        R"({"ty":"fl","c":{"k":[1,0,0,1]},"o":{"k":100}})",
        R"({"ty":"st","c":{"k":[1,0,0,1]},"o":{"k":100},"w":{"k":12},"lc":1,"lj":1,"ml":4})");
    struct Case { std::string name,json;unsigned mode; };
    const std::vector<Case> cases{{"half-provider-green",fixture("[0,1,0,1]"),0},
        {"half-provider-blue",fixture("[0,0,1,1]"),0},
        {"half-content",fixture("[0,1,0,1]","50"),1},
        {"lower-artwork",fixture("[0,1,0,1]","100",true),2},
        {"bottom-Difference",fixture("[0,1,0,1]","100",true,true),2},
        {"provider-inactive",inactive,3},{"animated-provider-geometry",animated,4},
        {"source-parent-to-content",vectorReplace(matteFixture(),"{\"fr\":","{\"v\":\"5.5.2\",\"fr\":"),5},
        {"distinct-pair-effects",repeated,6},
        {"presentation-half",fixture("[0,1,0,1]"),7},
        {"host-and-presentation",fixture("[0,1,0,1]"),8},
        {"opaque-host",fixture("[0,1,0,1]","100",true,true),9},
        {"caller-clip",fixture("[0,1,0,1]"),10},{"provider-alpha0",zero,11},
        {"static-linear-content",staticLinear,0},{"solid-stroke-content",stroke,12}};
    std::ofstream metrics(root/"metrics.tsv");metrics<<"case\twidth\theight\tframe\tnative_max\tcpu_pass\tiou\talpha\tmean\n";
    std::size_t captures=0,cpuCaptures=0;
    std::vector<PixelBGRA> greenPixels;
    for(const auto& c:cases) {
        std::ofstream fixtureFile(root/(c.name+".json"),std::ios::binary);fixtureFile<<c.json;
        require(bool(fixtureFile),"retained owned alpha fixture");
        const auto compiled=vectorCompile(c.json);require(bool(compiled),c.name+" compile: "+compiled.path+compiled.message);
        const auto prepared=render::detail::prepareOwnVectorAsset(compiled.prepared);require(bool(prepared),c.name+" prepare");
        auto stream=render::detail::OwnNativeEllipseStream::create(prepared.prepared);require(bool(stream),c.name+" stream");
        render::MotionRenderPlanner planner;backends::direct2d::Backend backend;
        WarpCaptureSurface actualSurface,expectedSurface,providerSurface;
        for(const auto [width,height]:{std::pair{128U,128U},{256U,128U},{128U,256U}}) {
            testsupport::CaptureProfile profile{"alpha",width,height,width,height,96,96};
            actualSurface.configure(profile);expectedSurface.configure(profile);providerSurface.configure(profile);
            const D2D1::Matrix3x2F viewport(1,0,0,1,width>height?64.F:0.F,height>width?64.F:0.F);
            for(const auto frame:{0U,19U,20U,25U,30U,31U,179U,31U,20U,0U,0U}) {
                auto emitted=stream.stream->emit(frame,width,height);require(bool(emitted),c.name+" emit");
                render::PresentationState presentation;
                if(c.mode==7) presentation.opacity=.5F;
                if(c.mode==8) presentation.transform={1,0,0,1,3,5};
                auto plan=planner.build(std::move(*emitted.scene),presentation);require(bool(plan),c.name+" plan");
                const auto host=c.mode==8?D2D1::Matrix3x2F(1.25F,0,0,1.5F,10,6):D2D1::Matrix3x2F::Identity();
                const auto matrix=viewport*D2D1::Matrix3x2F(presentation.transform.m11,presentation.transform.m12,
                    presentation.transform.m21,presentation.transform.m22,presentation.transform.dx,presentation.transform.dy)*host;
                std::vector<LiteralMatteRect> rects;
                if(c.mode==2 || c.mode==9) rects.push_back({{24,24,88,72},{0,0,1,1}});
                const bool active=c.mode!=11 && ((c.mode!=3 && c.mode!=5) || (frame>=20 && frame<31));
                if(active) {
                    D2D1_RECT_F intersection{24,24,64,72};float alpha=c.mode==1 || c.mode==5 || c.mode==7?.25F:.5F;
                    if(c.mode==4 && frame>=20 && frame<31) intersection={34,24,72,72};
                    if(c.mode==5) intersection={41,55,87,99};
                    rects.push_back({intersection,{1,0,0,alpha}});
                    if(c.mode==12) rects={{{24,24,30,72},{1,0,0,.5F}},
                        {{30,24,64,30},{1,0,0,.5F}},{{30,66,64,72},{1,0,0,.5F}}};
                    if(c.mode==6) rects.push_back({intersection,{0,0,1,.5F}});
                }
                const auto actual=renderAlphaPlan(actualSurface,backend,plan.plan,host,c.mode==9,c.mode==10,
                    frame==179?2:1);
                auto expected=literalMatteRaster(expectedSurface,rects,matrix,c.mode==9,c.mode==10);
                const auto label=c.name+"-"+std::to_string(width)+"x"+std::to_string(height)+"-f"+std::to_string(frame)+"-n"+std::to_string(captures++);
                if(c.mode==8) {
                    // The earlier single-intersection oracle differs by32 on
                    // coincident fractional AA edges. Capture each authored
                    // input independently, then derive premultiplied C×Aprovider.
                    // No own scene, plan, effect or backend computes this oracle.
                    rawPixels(root/(label+"-old-intersection.bgra"),expected);
                    const auto old=testsupport::comparePixels(expected,actual,width,height);
                    const auto contentPixels=literalMatteRaster(expectedSurface,
                        {{{24,24,72,72},{1,0,0,1}}},matrix,false,false);
                    const auto providerPixels=literalMatteRaster(providerSurface,
                        {{{24,24,64,72},{0,1,0,.5F}}},matrix,false,false);
                    expected=contentPixels;
                    for(std::size_t i=0;i<expected.size();++i) {
                        const auto multiply=[alpha=unsigned(providerPixels[i].a)](std::uint8_t value) {
                            return static_cast<std::uint8_t>((unsigned(value)*alpha+127)/255);
                        };
                        expected[i]={multiply(contentPixels[i].b),multiply(contentPixels[i].g),
                            multiply(contentPixels[i].r),multiply(contentPixels[i].a)};
                    }
                    rawPixels(root/(label+"-independent-content.bgra"),contentPixels);
                    rawPixels(root/(label+"-independent-provider.bgra"),providerPixels);
                    std::cout<<label<<" old_intersection_max="<<unsigned(old.maxChannelDifference)
                        <<" independent_per_input_product=1\n";
                }
                rawPixels(root/(label+"-own.bgra"),actual);rawPixels(root/(label+"-literal.bgra"),expected);
                const auto native=testsupport::comparePixels(expected,actual,width,height);
                require(native.maxChannelDifference<=3,c.name+" independent literal native alpha raster max="+std::to_string(native.maxChannelDifference));
                bool cpuPassed=true;testsupport::PixelComparisonMetrics cpuMetrics;
                const bool independentCpu=c.mode<=6 || c.mode==11 || c.mode==12;
                if(independentCpu) {
                    auto cpu=rlottie::Animation::loadFromData(c.json,"fresh-alpha-cpu-"+label,{},false);
                    require(bool(cpu),"fresh pinned alpha CPU parse");
                    const auto cpuPixels=ordinaryCpuPixels(*cpu,frame,profile);
                    cpuMetrics=testsupport::comparePixels(cpuPixels,actual,width,height);
                    const auto decision=testsupport::evaluateComparison(cpuMetrics,{});cpuPassed=decision.passed;++cpuCaptures;
                    for(const auto& reason:decision.failures) std::cout<<"alpha CPU failure: "<<reason<<'\n';
                    rawPixels(root/(label+"-cpu.bgra"),cpuPixels);
                    require(cpuPassed,c.name+" fresh pinned CPU unchanged policy");
                }
                metrics<<c.name<<'\t'<<width<<'\t'<<height<<'\t'<<frame<<'\t'<<unsigned(native.maxChannelDifference)<<'\t'
                    <<cpuPassed<<'\t'<<cpuMetrics.activeIoU<<'\t'<<cpuMetrics.alphaRelativeError<<'\t'<<cpuMetrics.meanAbsoluteDifferenceAll<<'\n';
                std::cout<<label<<" native_max="<<unsigned(native.maxChannelDifference)<<" cpu="<<(independentCpu?"fresh":"out-of-scope")
                    <<" drawn="<<plan.plan.drawItems.size()<<" skips=0\n";
                if(width==128 && height==128 && frame==0 && c.name=="half-provider-green") greenPixels=actual;
                if(width==128 && height==128 && frame==0 && c.name=="half-provider-blue")
                    require(actual==greenPixels,"provider RGB has no effect at every pixel");
                if(width==128 && height==128 && frame==0 && (c.mode==0 || c.mode==1 || c.mode==7)) {
                    const auto pixel=actual[48*width+48];const int wanted=c.mode==0?128:64;
                    require(std::abs(int(pixel.r)-wanted)<=3 && pixel.g==0 && pixel.b==0 && std::abs(int(pixel.a)-wanted)<=3,
                        "literal interior RGBA red alpha product with allowance3");
                    std::cout<<c.name<<" interior="<<unsigned(pixel.r)<<",0,0,"<<unsigned(pixel.a)<<" expected="<<wanted<<",0,0,"<<wanted<<" allowance=3\n";
                }
            }
        }
    }
    std::cout<<"alpha native captures="<<captures<<" fresh pinned CPU="<<cpuCaptures<<" max allowance=3\n";
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

void subpixelAlphaThresholdRejection(const std::vector<PixelBGRA>& cpu,
                                    const std::vector<PixelBGRA>& reference,
                                    const std::vector<PixelBGRA>& own,
                                    unsigned width, unsigned height, unsigned frame) {
    require(width == 512 && height == 256 && frame == 60,
            "subpixel threshold witness has exact viewport/frame");
    require(cpu.size() == width * height && reference.size() == cpu.size() && own.size() == cpu.size(),
            "subpixel threshold witness has exact pixel storage");
    require(own == reference, "subpixel own/reference exact WARP");
    const testsupport::PixelComparisonPolicy policy;
    for (const auto* pixels : {&own, &reference}) {
        const auto metrics = testsupport::comparePixels(cpu, *pixels, width, height);
        const auto decision = testsupport::evaluateComparison(metrics, policy);
        require(!decision.passed && decision.failures == std::vector<std::string>{
                    "nontransparent bounds differ beyond policy"},
                "subpixel CPU policy rejects only bounds, including independent ordinary WARP");
        require(metrics.activeIoU >= policy.minimumActiveIoU &&
                    metrics.alphaRelativeError <= policy.maximumAlphaRelativeError &&
                    metrics.meanAbsoluteDifferenceAll <= policy.maximumMeanAbsoluteDifferenceAll &&
                    metrics.meanAbsoluteDifferenceActive <= policy.maximumMeanAbsoluteDifferenceActive &&
                    metrics.largeDifferenceFraction <= policy.maximumLargeDifferenceFraction,
                "subpixel retains all five other default CPU policy criteria");
        const auto& a = metrics.referenceBounds;
        const auto& b = metrics.actualBounds;
        require(a.valid && b.valid && a.left == 252 && a.top == 42 && a.right == 405 && a.bottom == 127 &&
                    b.left == 171 && b.top == 42 && b.right == 405 && b.bottom == 127 &&
                    metrics.maximumBoundsDelta == 81,
                "subpixel literal thresholded bounds");
    }
    // Literal coverage, coordinates, red color and premultiplication come from
    // the retained independent raw audit, not a production path generator.
    const std::array<unsigned, 4> x{171,172,171,172}, y{85,85,86,86};
    const std::array<std::uint8_t, 4> cpuAlpha{7,7,6,6}, warpAlpha{9,9,11,11};
    for (std::size_t i = 0; i < x.size(); ++i) {
        const auto index = y[i] * width + x[i];
        require(cpu[index] == PixelBGRA{0,0,cpuAlpha[i],cpuAlpha[i]} &&
                    own[index] == PixelBGRA{0,0,warpAlpha[i],warpAlpha[i]} &&
                    reference[index] == own[index],
                "subpixel literal nonzero red premultiplied alpha at guarded coordinates");
    }
}

void subpixelMutationWitnesses(const std::vector<PixelBGRA>& cpu,
                              const std::vector<PixelBGRA>& reference,
                              const std::vector<PixelBGRA>& own) {
    const std::array<unsigned, 4> indices{85*512+171,85*512+172,86*512+171,86*512+172};
    for (unsigned mutation : {0U,1U,2U}) {
        auto changed = own;
        for (const auto index : indices)
            changed[index] = {};
        if (mutation == 1)
            for (const auto index : indices)
                changed[index + 1] = own[index];
        if (mutation == 2)
            for (const auto index : indices)
                changed[index] = {0,own[index].a,0,own[index].a};
        bool rejected = false;
        try {
            subpixelAlphaThresholdRejection(cpu, reference, changed, 512, 256, 60);
        } catch (const std::runtime_error& error) {
            rejected = std::string(error.what()).find("subpixel own/reference exact WARP") != std::string::npos;
        }
        require(rejected, "subpixel missing/moved/recolored geometry mutation caught");
    }
    std::cout << "subpixel missing/moved/recolored geometry mutations caught=3\n";
}

void run(const std::string &json, const fs::path &root, bool rectangles, bool groups = false,
         bool positions = false, const std::vector<unsigned> &requestedFrames = {},
         bool fractionalThresholdWitness = false) {
    auto parsed = formats::detail::readOwnJson(json, {65536, 32});
    require(bool(parsed), "vector reader");
    auto prepared = render::detail::prepareOwnMotionAsset(*parsed.document);
    require(bool(prepared), prepared.path + prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    require(bool(stream), stream.message);
    OwnVectorSceneOracle oracle(json);
    std::unique_ptr<vector_scene::OwnRoles> ownRoles;
    if (!requestedFrames.empty())
        ownRoles = std::make_unique<vector_scene::OwnRoles>(*prepared.prepared);
    auto cpu = rlottie::Animation::loadFromData(json, "vector-cpu", {}, false);
    require(bool(cpu), "ordinary CPU parse");
    render::MotionRenderPlanner ownPlanner, refPlanner;
    backends::direct2d::Backend ownBackend, refBackend;
    WarpCaptureSurface ownSurface, refSurface;
    std::ofstream metrics(root / "metrics.tsv");
    metrics << "width\theight\tframe\tdiff_pixels\tmax_channel\tmean_abs\tcpu_"
               "pass\tcpu_"
               "iou\tcpu_alpha\tcpu_mean\tcpu_active_mean\tcpu_large_fraction\tcpu_bounds_delta\n";
    std::size_t failures = 0;
    std::size_t sample = 0;
    std::size_t expectedNegativeSamples = 0;
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
        const std::vector<unsigned> frames = !requestedFrames.empty() ? requestedFrames :
            positions ? std::vector<unsigned>{0,14,15,20,25,40,59,60,70,179,70,60,59,40,25,20,15,14,0,15,15} :
            groups ? std::vector<unsigned>{0,50,68,69,100,110,179,68,69} :
            std::vector<unsigned>{0,10,15,20,33,34,45,67,68,90,110,127,128,135,137,138,179};
        for (const auto frame : frames) {
            auto own = stream.stream->emit(frame, width, height);
            require(bool(own), own.message);
            auto ref = oracle.freshScene(frame, width, height);
            oracle.assertRedundantClips(ref);
            if (ownRoles)
                vector_scene::compare(ref, oracle, *own.scene, *ownRoles);
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
            const bool expectedNegative = fractionalThresholdWitness && width == 512 &&
                                          height == 256 && frame == 60;
            if (expectedNegative) {
                subpixelAlphaThresholdRejection(cpuPixels, expected, actual, width, height, frame);
                if (expectedNegativeSamples == 0)
                    subpixelMutationWitnesses(cpuPixels, expected, actual);
                ++expectedNegativeSamples;
            }
            std::size_t differences = 0;
            for (std::size_t i = 0; i < actual.size(); ++i)
                if (actual[i] != expected[i])
                    ++differences;
            const auto label = "s" + std::to_string(width) +
                               (width == height ? "" : "x" + std::to_string(height)) + "-f" +
                               std::to_string(frame) + (requestedFrames.empty() ? "" :
                                   "-n" + std::to_string(sample++));
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
            if (!requestedFrames.empty())
                rawPixels(root / (label + "-cpu.bgra"), cpuPixels);
            std::ofstream ownRaw(root / (label + "-own.bgra"), std::ios::binary);
            ownRaw.write(reinterpret_cast<const char *>(actual.data()),
                         static_cast<std::streamsize>(actual.size() * sizeof(PixelBGRA)));
            std::ofstream refRaw(root / (label + "-reference.bgra"), std::ios::binary);
            refRaw.write(reinterpret_cast<const char *>(expected.data()),
                         static_cast<std::streamsize>(expected.size() * sizeof(PixelBGRA)));
            require(activePixels(actual) > 0 && activePixels(expected) > 0,
                    "nonempty expected frame");
            if (!requestedFrames.empty()) {
                require(ownPlan.plan.drawItems.size() == 3 && refPlan.plan.drawItems.size() == 3,
                        "mixed capture retains all three authored sibling paints");
                require(actual.front().a == 0 && expected.front().a == 0,
                        "mixed capture has a transparent corner");
                require(std::any_of(actual.begin(), actual.end(), [](const auto& pixel) {
                    return pixel.a > 0 && pixel.a < 255;
                }), "mixed capture retains visible fractional alpha");
            }
            metrics << width << '\t' << height << '\t' << frame << '\t' << differences << '\t'
                    << unsigned(warpMetrics.maxChannelDifference) << '\t'
                    << warpMetrics.meanAbsoluteDifferenceAll << '\t' << cpuDecision.passed << '\t'
                    << cpuMetrics.activeIoU << '\t' << cpuMetrics.alphaRelativeError << '\t'
                    << cpuMetrics.meanAbsoluteDifferenceAll << '\t'
                    << cpuMetrics.meanAbsoluteDifferenceActive << '\t'
                    << cpuMetrics.largeDifferenceFraction << '\t'
                    << cpuMetrics.maximumBoundsDelta << '\n';
            std::cout << label << " diff_pixels=" << differences
                      << " max_channel=" << unsigned(warpMetrics.maxChannelDifference)
                      << " cpu_pass=" << cpuDecision.passed
                      << " drawn=" << ownPlan.plan.drawItems.size() << " skips=0\n";
            if (!cpuDecision.passed) {
                const auto bounds = [](const auto& b) {
                    return std::to_string(b.left) + "," + std::to_string(b.top) + "," +
                           std::to_string(b.right) + "," + std::to_string(b.bottom);
                };
                const auto referenceCpu = testsupport::evaluateComparison(
                    testsupport::comparePixels(cpuPixels, expected, width, height), {});
                std::cout << "cpu reference_scene_pass=" << referenceCpu.passed
                          << " reference_bounds=" << bounds(cpuMetrics.referenceBounds)
                          << " own_bounds=" << bounds(cpuMetrics.actualBounds)
                          << " bounds_delta=" << cpuMetrics.maximumBoundsDelta
                          << " active_mean=" << cpuMetrics.meanAbsoluteDifferenceActive
                          << " large_fraction=" << cpuMetrics.largeDifferenceFraction << '\n';
                for (const auto& reason : cpuDecision.failures)
                    std::cout << "cpu failure: " << reason << '\n';
            }
            if (differences || (!cpuDecision.passed && !expectedNegative))
                ++failures;
        }
    }
    if (fractionalThresholdWitness)
        require(expectedNegativeSamples == 2, "both repeated fractional threshold-negative samples exercised");
    if (!requestedFrames.empty())
        std::cout << "expected_negative_cpu_samples=" << expectedNegativeSamples
                  << (expectedNegativeSamples ? " (CPU policy rejection retained; cpu_pass=0)" : "")
                  << '\n';
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
        if (argc == 2 && (std::string(argv[1]) == "--mixed" ||
                          std::string(argv[1]) == "--mixed-fractional")) {
            for (const auto &[name, json] : vectorMixedFixtures()) {
                if (std::string(argv[1]) == "--mixed-fractional" && name != "fractional")
                    continue;
                std::cout << "mixed case=" << name << '\n';
                fs::create_directories(root / ("mixed-" + name));
                run(json, root / ("mixed-" + name), true, false, false, vectorMixedFrames(), name == "fractional");
            }
            return 0;
        }
        if(argc==3 && std::string(argv[1])=="--party-original") {
            originalPartyCaptures(vectorRead(argv[2]),root); return 0;
        }
        if(argc==2 && std::string(argv[1])=="--alpha-matte") {
            alphaCaptures(root/"own-alpha-matte"); return 0;
        }
        if (argc == 1)
            widthZeroWitness();
        clippingWitnesses(root);
        run(vectorInput(argc, argv), root, argc == 1);
        if (argc == 1) {
            alphaCaptures(root / "own-alpha-matte");
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
            for (const auto &[name, json] : vectorMixedFixtures()) {
                std::cout << "mixed case=" << name << '\n';
                fs::create_directories(root / ("mixed-" + name));
                run(json, root / ("mixed-" + name), true, false, false, vectorMixedFrames(), name == "fractional");
            }
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
