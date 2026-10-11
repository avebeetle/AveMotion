#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/model/AssetModel.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "../backends/direct2d/LinearGradientBackendAccess.hpp"
#include "../backends/direct2d/AlphaMatteBackendAccess.hpp"

#include <d2d1_1.h>
#include <d2d1effects.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

bool near(float left, float right, float epsilon = 1.0e-6F) {
    return std::fabs(left - right) <= epsilon;
}

class RefCounted {
public:
    ULONG addRef() noexcept { return ++references_; }

    template <typename T>
    ULONG release(T* self) noexcept {
        const auto remaining = --references_;
        if (remaining == 0U) {
            delete self;
        }
        return remaining;
    }

protected:
    ~RefCounted() = default;

private:
    ULONG references_ = 1U;
};

enum class FakeVerb : std::uint8_t {
    Move,
    Line,
    Cubic,
    Close,
};

class FakePathGeometry;

class FakeGeometrySink final : public ID2D1GeometrySink, private RefCounted {
public:
    explicit FakeGeometrySink(FakePathGeometry* geometry) noexcept;
    ~FakeGeometrySink() override;

    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    void SetFillMode(D2D1_FILL_MODE fillMode) noexcept override;
    void BeginFigure(
        D2D1_POINT_2F startPoint,
        D2D1_FIGURE_BEGIN figureBegin) noexcept override;
    void AddLine(D2D1_POINT_2F point) noexcept override;
    void AddBezier(const D2D1_BEZIER_SEGMENT& bezier) noexcept override;
    void EndFigure(D2D1_FIGURE_END figureEnd) noexcept override;
    HRESULT Close() noexcept override;

private:
    FakePathGeometry* geometry_ = nullptr;
    bool figureOpen_ = false;
};

class FakePathGeometry final : public ID2D1PathGeometry, private RefCounted {
public:
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    HRESULT Open(ID2D1GeometrySink** geometrySink) noexcept override {
        if (geometrySink == nullptr || opened_) {
            return E_INVALIDARG;
        }
        opened_ = true;
        *geometrySink = new FakeGeometrySink(this);
        return S_OK;
    }

    D2D1_FILL_MODE fillMode = D2D1_FILL_MODE_WINDING;
    std::vector<FakeVerb> verbs;
    std::vector<D2D1_POINT_2F> points;
    bool opened_ = false;
    bool closed_ = false;
};

FakeGeometrySink::FakeGeometrySink(FakePathGeometry* geometry) noexcept
    : geometry_(geometry) {
    geometry_->AddRef();
}

FakeGeometrySink::~FakeGeometrySink() {
    geometry_->Release();
}

void FakeGeometrySink::SetFillMode(D2D1_FILL_MODE fillMode) noexcept {
    geometry_->fillMode = fillMode;
}

void FakeGeometrySink::BeginFigure(
    D2D1_POINT_2F startPoint,
    D2D1_FIGURE_BEGIN /*figureBegin*/) noexcept {
    geometry_->verbs.push_back(FakeVerb::Move);
    geometry_->points.push_back(startPoint);
    figureOpen_ = true;
}

void FakeGeometrySink::AddLine(D2D1_POINT_2F point) noexcept {
    geometry_->verbs.push_back(FakeVerb::Line);
    geometry_->points.push_back(point);
}

void FakeGeometrySink::AddBezier(const D2D1_BEZIER_SEGMENT& bezier) noexcept {
    geometry_->verbs.push_back(FakeVerb::Cubic);
    geometry_->points.push_back(bezier.point1);
    geometry_->points.push_back(bezier.point2);
    geometry_->points.push_back(bezier.point3);
}

void FakeGeometrySink::EndFigure(D2D1_FIGURE_END figureEnd) noexcept {
    if (figureEnd == D2D1_FIGURE_END_CLOSED) {
        geometry_->verbs.push_back(FakeVerb::Close);
    }
    figureOpen_ = false;
}

HRESULT FakeGeometrySink::Close() noexcept {
    if (figureOpen_) {
        return E_FAIL;
    }
    geometry_->closed_ = true;
    return S_OK;
}

class FakeStrokeStyle final : public ID2D1StrokeStyle, private RefCounted {
public:
    explicit FakeStrokeStyle(D2D1_STROKE_STYLE_PROPERTIES value) noexcept
        : properties(value) {
    }

    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    D2D1_STROKE_STYLE_PROPERTIES properties;
};

class FakeSolidColorBrush final
    : public ID2D1SolidColorBrush,
      private RefCounted {
public:
    explicit FakeSolidColorBrush(D2D1_COLOR_F value) noexcept
        : color(value) {
    }

    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    void SetColor(const D2D1_COLOR_F& value) noexcept override { color = value; }
    void SetOpacity(FLOAT value) noexcept override { opacity = value; }

    D2D1_COLOR_F color;
    FLOAT opacity = 1.0F;
};

class FakeFactory final : public ID2D1Factory, private RefCounted {
public:
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    HRESULT CreatePathGeometry(ID2D1PathGeometry** geometry) noexcept override {
        if (geometry == nullptr) {
            return E_INVALIDARG;
        }
        ++pathGeometryCreations;
        *geometry = new FakePathGeometry;
        return S_OK;
    }

    HRESULT CreateStrokeStyle(
        const D2D1_STROKE_STYLE_PROPERTIES& properties,
        const FLOAT* /*dashes*/,
        UINT32 dashCount,
        ID2D1StrokeStyle** strokeStyle) noexcept override {
        if (strokeStyle == nullptr || dashCount != 0U) {
            return E_INVALIDARG;
        }
        ++strokeStyleCreations;
        *strokeStyle = new FakeStrokeStyle(properties);
        return S_OK;
    }

    std::uint64_t pathGeometryCreations = 0;
    std::uint64_t strokeStyleCreations = 0;
};

class FakeStops final : public ID2D1GradientStopCollection, private RefCounted {
public:
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }
    std::vector<D2D1_GRADIENT_STOP> stops;
    D2D1_GAMMA gamma = D2D1_GAMMA_1_0;
    D2D1_EXTEND_MODE extend = D2D1_EXTEND_MODE_WRAP;
};
class FakeLinearBrush final : public ID2D1LinearGradientBrush, private RefCounted {
public:
    FakeLinearBrush(D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES p, FakeStops* s)
        : properties(p), stops(s) { stops->AddRef(); }
    ~FakeLinearBrush() override { stops->Release(); }
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }
    void SetOpacity(FLOAT value) noexcept override { opacity = value; }
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES properties;
    FakeStops* stops;
    FLOAT opacity = 1;
};

struct FakeDrawRecord final {
    bool stroke = false;
    FakePathGeometry* geometry = nullptr;
    D2D1_COLOR_F color;
    FLOAT opacity = 1.0F;
    FLOAT strokeWidth = 0.0F;
    D2D1_STROKE_STYLE_PROPERTIES strokeProperties;
    D2D1_MATRIX_3X2_F transform;
    bool gradient = false;
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES gradientProperties{};
    std::vector<D2D1_GRADIENT_STOP> stops;
};

class FakeImage final : public ID2D1Image, private RefCounted {
public:
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }
};
class FakeCommandList final : public ID2D1CommandList, private RefCounted {
public:
    FakeCommandList(ID2D1Image** target, HRESULT failure) : target_(target), failure_(failure) {}
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }
    HRESULT Close() noexcept override {
        require(*target_ != this, "production detaches command list before Close");
        require(!closed, "production closes each command list once");
        closed = true;
        return failure_;
    }
    bool closed = false;
private:
    ID2D1Image** target_;
    HRESULT failure_;
};
class FakeEffect final : public ID2D1Effect, private RefCounted {
public:
    explicit FakeEffect(HRESULT failure) : failure_(failure) {}
    ~FakeEffect() override { for(auto* input : inputs) if(input) input->Release(); }
    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }
    void SetInput(UINT32 index, ID2D1Image* image) noexcept override {
        require(index<2 && image && dynamic_cast<FakeCommandList*>(image)->closed,
                "production binds closed content/provider lists");
        if(inputs[index]) inputs[index]->Release();
        inputs[index]=image; image->AddRef();
    }
    HRESULT SetValue(UINT32 property, D2D1_COMPOSITE_MODE value) noexcept override {
        require(property == D2D1_COMPOSITE_PROP_MODE && value == D2D1_COMPOSITE_MODE_DESTINATION_IN,
                "production requests destination-in");
        configured=SUCCEEDED(failure_); return failure_;
    }
    ID2D1Image* inputs[2]{};
    bool configured=false;
private:
    HRESULT failure_;
};

class FakeDeviceContext final
    : public ID2D1DeviceContext,
      private RefCounted {
public:
    explicit FakeDeviceContext(FakeFactory* factory) noexcept
        : factory_(factory) {
        factory_->AddRef();
    }

    ~FakeDeviceContext() override {
        SetTarget(nullptr);
        factory_->Release();
    }

    void GetTarget(ID2D1Image** image) noexcept override {
        *image=target; if(target) target->AddRef();
    }
    void SetTarget(ID2D1Image* image) noexcept override {
        if(image) image->AddRef();
        if(target) target->Release();
        target=image;
    }
    HRESULT CreateCommandList(ID2D1CommandList** list) noexcept override {
        ++listCreations;
        if(listCreations==failListAt) return nativeFailure;
        *list=new FakeCommandList(&target,listCreations==failCloseAt?nativeFailure:S_OK);
        return S_OK;
    }
    HRESULT CreateEffect(int effect,ID2D1Effect** value) noexcept override {
        require(effect==CLSID_D2D1Composite,"production uses native Composite");
        ++effectCreations;
        if(failEffect) return nativeFailure;
        *value=new FakeEffect(failProperty?nativeFailure:S_OK); return S_OK;
    }
    void DrawImage(ID2D1Image* image,const D2D1_POINT_2F*,const void*,
        D2D1_INTERPOLATION_MODE,D2D1_COMPOSITE_MODE mode) noexcept override {
        require(dynamic_cast<FakeCommandList*>(image)->closed && mode==D2D1_COMPOSITE_MODE_SOURCE_OVER,
                "production replays closed frame source-over");
        require(sameIdentity(),"frame replay uses identity after captured transforms");
        ++frameReplays;
    }
    void DrawImage(ID2D1Effect* effect,const D2D1_POINT_2F*,const void*,
        D2D1_INTERPOLATION_MODE,D2D1_COMPOSITE_MODE mode) noexcept override {
        const auto* value=dynamic_cast<FakeEffect*>(effect);
        require(value->configured && value->inputs[0] && value->inputs[1] &&
                value->inputs[0]!=value->inputs[1] && mode==D2D1_COMPOSITE_MODE_SOURCE_OVER,
                "production replays distinct configured content/provider inputs");
        require(sameIdentity(),"pair replay uses identity"); ++pairReplays;
    }
    bool sameIdentity() const noexcept {
        return transform_.m11==1 && transform_.m12==0 && transform_.m21==0 &&
            transform_.m22==1 && transform_.dx==0 && transform_.dy==0;
    }
    ID2D1Image* target=nullptr;
    unsigned listCreations=0,effectCreations=0,frameReplays=0,pairReplays=0;
    unsigned failListAt=0,failCloseAt=0;
    bool failEffect=false,failProperty=false;
    HRESULT nativeFailure=E_FAIL;

    ULONG AddRef() noexcept override { return addRef(); }
    ULONG Release() noexcept override { return release(this); }

    void GetFactory(ID2D1Factory** factory) noexcept override {
        if (factory == nullptr) {
            return;
        }
        factory_->AddRef();
        *factory = factory_;
    }

    HRESULT CreateSolidColorBrush(
        const D2D1_COLOR_F& color,
        ID2D1SolidColorBrush** brush) noexcept override {
        if (brush == nullptr) {
            return E_INVALIDARG;
        }
        ++solidBrushCreations;
        *brush = new FakeSolidColorBrush(color);
        return S_OK;
    }

    void GetTransform(D2D1_MATRIX_3X2_F* transform) const noexcept override {
        if (transform != nullptr) {
            *transform = transform_;
        }
    }

    void SetTransform(const D2D1_MATRIX_3X2_F& transform) noexcept override {
        transform_ = transform;
    }

    void FillGeometry(
        ID2D1PathGeometry* geometry,
        ID2D1Brush* brush) noexcept override {
        record(false, geometry, brush, 0.0F, nullptr);
    }

    void DrawGeometry(
        ID2D1PathGeometry* geometry,
        ID2D1Brush* brush,
        FLOAT strokeWidth,
        ID2D1StrokeStyle* strokeStyle) noexcept override {
        record(true, geometry, brush, strokeWidth, strokeStyle);
    }

    [[nodiscard]] const D2D1_MATRIX_3X2_F& transform() const noexcept {
        return transform_;
    }

    std::uint64_t solidBrushCreations = 0;
    std::uint64_t stopCreations = 0, linearBrushCreations = 0;
    HRESULT stopFailure = S_OK, linearFailure = S_OK;
    HRESULT CreateGradientStopCollection(const D2D1_GRADIENT_STOP* stops, UINT32 count,
        D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend,
        ID2D1GradientStopCollection** collection) noexcept override {
        ++stopCreations;
        if (FAILED(stopFailure)) return stopFailure;
        auto* value = new FakeStops;
        value->stops.assign(stops, stops + count);
        value->gamma = gamma; value->extend = extend;
        require(gamma == D2D1_GAMMA_2_2 && extend == D2D1_EXTEND_MODE_CLAMP,
                "gradient uses encoded interpolation and clamp");
        *collection = value;
        return S_OK;
    }
    HRESULT CreateLinearGradientBrush(const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES& properties,
        ID2D1GradientStopCollection* collection, ID2D1LinearGradientBrush** brush) noexcept override {
        ++linearBrushCreations;
        if (FAILED(linearFailure)) return linearFailure;
        *brush = new FakeLinearBrush(properties, static_cast<FakeStops*>(collection));
        return S_OK;
    }
    std::vector<FakeDrawRecord> records;

private:
    void record(
        bool stroke,
        ID2D1PathGeometry* geometry,
        ID2D1Brush* brush,
        FLOAT strokeWidth,
        ID2D1StrokeStyle* strokeStyle) noexcept {
        auto* fakeGeometry = dynamic_cast<FakePathGeometry*>(geometry);
        auto* fakeBrush = dynamic_cast<FakeSolidColorBrush*>(brush);
        auto* fakeStroke = dynamic_cast<FakeStrokeStyle*>(strokeStyle);
        FakeDrawRecord value;
        value.stroke = stroke;
        value.geometry = fakeGeometry;
        value.color = fakeBrush != nullptr ? fakeBrush->color : D2D1_COLOR_F{};
        value.opacity = fakeBrush != nullptr ? fakeBrush->opacity : 0.0F;
        value.strokeWidth = strokeWidth;
        value.strokeProperties = fakeStroke != nullptr
            ? fakeStroke->properties
            : D2D1_STROKE_STYLE_PROPERTIES{};
        value.transform = transform_;
        if (auto* linear = dynamic_cast<FakeLinearBrush*>(brush)) {
            value.gradient = true; value.opacity = linear->opacity;
            value.gradientProperties = linear->properties; value.stops = linear->stops->stops;
        }
        records.push_back(value);
    }

    FakeFactory* factory_ = nullptr;
    D2D1_MATRIX_3X2_F transform_{};
};

avemotion::runtime::EvaluatedPath rectanglePath() {
    using namespace avemotion::runtime;
    EvaluatedPath path;
    path.verbs = {
        PathVerb::MoveTo,
        PathVerb::LineTo,
        PathVerb::LineTo,
        PathVerb::LineTo,
        PathVerb::Close,
    };
    path.points = {
        {0.0F, 0.0F},
        {24.0F, 0.0F},
        {24.0F, 24.0F},
        {0.0F, 24.0F},
    };
    path.controlBounds = {true, 0.0F, 0.0F, 24.0F, 24.0F};
    path.hash = 0xAA55AA55ULL;
    return path;
}

avemotion::runtime::EvaluatedDrawItem sourceItem(
    avemotion::runtime::Color8 color,
    bool stroke) {
    using namespace avemotion::runtime;
    EvaluatedDrawItem item;
    item.localGeometryAvailable = true;
    item.localGeometryStaticCandidate = true;
    item.localPath = rectanglePath();
    item.localPaintAvailable = true;
    item.localPaintStaticCandidate = true;
    item.localPaint.kind = PaintKind::Solid;
    item.localPaint.solid = color;
    item.localStroke.enabled = stroke;
    item.localStroke.width = stroke ? 3.0F : 0.0F;
    item.localStroke.miterLimit = 4.0F;
    item.localStroke.cap = LineCap::Round;
    item.localStroke.join = LineJoin::Bevel;
    item.fillRule = FillRule::Winding;
    return item;
}

avemotion::render::GeometryCacheKey sharedGeometryKey() {
    using namespace avemotion::render;
    return {
        .scope = ResourceIdentityScope::Asset,
        .assetHash = 0x1234ULL,
        .assetIdentity = 7ULL,
        .instanceId = 0ULL,
        .instanceIdentity = 0ULL,
        .sourceKey = 0x88ULL,
        .resourceId = {},
        .contentHash = 0xABCDULL,
        .revision = 0ULL,
    };
}

avemotion::render::PaintCacheKey paintKey(std::uint64_t sourceKey) {
    using namespace avemotion::render;
    return {
        .scope = ResourceIdentityScope::Asset,
        .assetHash = 0x1234ULL,
        .assetIdentity = 7ULL,
        .instanceId = 0ULL,
        .instanceIdentity = 0ULL,
        .sourceKey = sourceKey,
        .resourceId = {},
        .contentHash = sourceKey * 17ULL,
        .revision = 0ULL,
    };
}

avemotion::render::MotionDrawItem planItem(
    std::uint32_t sourceIndex,
    float x,
    float y,
    float opacity,
    std::uint32_t features,
    std::uint64_t paintSourceKey) {
    avemotion::render::MotionDrawItem item;
    item.sourceDrawItemIndex = sourceIndex;
    item.geometry = sharedGeometryKey();
    item.paint = paintKey(paintSourceKey);
    item.presentationTransform.dx = x;
    item.presentationTransform.dy = y;
    item.effectiveOpacity = opacity;
    item.featureBits = features;
    return item;
}

avemotion::render::MotionRenderPlan makePlan() {
    using namespace avemotion;
    auto scene = std::make_shared<runtime::EvaluatedScene>();
    scene->drawItems.push_back(sourceItem(
        runtime::Color8{255U, 0U, 0U, 255U}, false));
    scene->drawItems.push_back(sourceItem(
        runtime::Color8{0U, 255U, 0U, 255U}, true));

    render::MotionRenderPlan plan;
    plan.sourceScene = std::move(scene);
    plan.drawItems.push_back(planItem(
        0U, 10.0F, 12.0F, 1.0F,
        render::RenderFeatureSolidPaint, 0x101ULL));
    plan.drawItems.push_back(planItem(
        0U, 50.0F, 12.0F, 0.5F,
        render::RenderFeatureSolidPaint, 0x101ULL));
    plan.drawItems.push_back(planItem(
        1U, 90.0F, 12.0F, 1.0F,
        render::RenderFeatureSolidPaint | render::RenderFeatureStroke,
        0x202ULL));
    return plan;
}

bool sameMatrix(
    const D2D1_MATRIX_3X2_F& left,
    const D2D1_MATRIX_3X2_F& right) {
    return near(left.m11, right.m11)
        && near(left.m12, right.m12)
        && near(left.m21, right.m21)
        && near(left.m22, right.m22)
        && near(left.dx, right.dx)
        && near(left.dy, right.dy);
}

// Migrated unconditionally after the recorded public-route behavioral RED.
avemotion::backends::direct2d::DrawResult drawLinear(
    avemotion::backends::direct2d::Backend& backend,
    const avemotion::render::MotionRenderPlan& plan,
    const avemotion::backends::direct2d::RenderSession& session) {
    return avemotion::backends::direct2d::detail::LinearGradientBackendAccess::draw(backend, plan, session);
}

avemotion::render::MotionRenderPlan linearPlan() {
    using namespace avemotion;
    auto scene = std::make_shared<runtime::EvaluatedScene>();
    scene->layers.emplace_back();
    auto item = sourceItem({}, false);
    item.layerIndex = 0;
    item.localPaint.kind = runtime::PaintKind::Gradient;
    item.localPaint.gradient.end = {20, 20};
    item.localPaint.gradient.stops = {{0, {0,0,0,255}}, {1, {255,255,255,255}}};
    scene->drawItems.push_back(item);
    render::MotionRenderPlan plan;
    plan.sourceScene = scene;
    plan.drawItems.push_back(planItem(0, 10, 12, 1, render::RenderFeatureGradientPaint, 0x303));
    return plan;
}

// Switched only this seam after the recorded public-route functional RED.
avemotion::backends::direct2d::DrawResult drawAlpha(
    avemotion::backends::direct2d::Backend& backend,
    const avemotion::render::MotionRenderPlan& plan,
    const avemotion::backends::direct2d::RenderSession& session) {
    return avemotion::backends::direct2d::detail::AlphaMatteBackendAccess::draw(backend, plan, session);
}

avemotion::render::MotionRenderPlan alphaPlan() {
    using namespace avemotion;
    runtime::EvaluatedScene scene;
    scene.sourceAssetHash = 17; scene.instanceId = 19;
    // The synthetic root is followed by root artwork in bottom-up order.
    scene.layers.resize(4);
    for (unsigned i=1; i<4; ++i) {
        scene.layers[i].visible = true; scene.layers[i].drawItemCount = 1;
        scene.layers[i].parentLayer = 0;
    }
    scene.layers[3].matteTargetLayer = 2;
    scene.layers[2].matteSourceLayer = 3;
    scene.layers[2].matte = runtime::MatteMode::Alpha;
    scene.layers[1].isolatedBottomDifference = true;
    for (unsigned layer : {1U, 2U, 3U}) {
        auto item = sourceItem(layer == 3 ? runtime::Color8{0,255,0,128}
                                         : runtime::Color8{255,0,0,255}, false);
        item.layerIndex = layer;
        scene.layers[layer].firstDrawItem = static_cast<std::uint32_t>(scene.drawItems.size());
        scene.drawItems.push_back(item);
    }
    render::MotionRenderPlanner planner;
    auto plan = planner.build(std::move(scene));
    require(bool(plan), "owned alpha contract planner");
    return std::move(plan.plan);
}

avemotion::render::MotionRenderPlan frozenAlphaPlan() {
    using namespace avemotion;
    auto scene=std::make_shared<runtime::EvaluatedScene>(*alphaPlan().sourceScene);
    auto frozen=std::make_shared<model::MotionAssetModel>();frozen->sourceAssetHash=scene->sourceAssetHash;
    for(unsigned i=0;i<scene->layers.size();++i) {
        auto& layer=scene->layers[i];layer.modelLayer={i};
        model::MotionLayerRecord record;record.present=true;record.id={i};record.parent=layer.parentLayer==runtime::kInvalidSceneIndex
            ?model::LayerId{}:model::LayerId{layer.parentLayer};record.nodes={layer.firstDrawItem,layer.drawItemCount};record.matte=layer.matte;
        if(layer.matteSourceLayer!=runtime::kInvalidSceneIndex || layer.matteTargetLayer!=runtime::kInvalidSceneIndex)
            record.dependencyBits=model::StaticDependencyMatte;
        frozen->layers.push_back(record);
    }
    for(unsigned i=0;i<scene->drawItems.size();++i) {
        auto& item=scene->drawItems[i];item.modelDrawItem={i};item.modelNode={i};item.modelGeometry={i};item.modelPaint={i};
        item.sourceGeometryId=i+1;item.sourcePaintId=i+1;
        frozen->nodes.push_back({true,{i},{i},{item.layerIndex},{i},{i},i});
        model::MotionGeometryRecord geometry;geometry.present=true;geometry.id={i};geometry.resourceClass=model::ResourceClass::InstanceEvaluated;
        frozen->geometries.push_back(geometry);
        model::MotionPaintRecord paint;paint.present=true;paint.id={i};paint.resourceClass=model::ResourceClass::InstanceEvaluated;
        frozen->paints.push_back(paint);
    }
    scene->assetModel=frozen;scene->assetModelApplied=true;
    render::MotionRenderPlanner planner;auto plan=planner.build(scene);require(bool(plan),"frozen alpha fixture plan");return std::move(plan.plan);
}

void alphaCanonicalProvenanceContracts() {
    using namespace avemotion;
    using Class = model::ResourceClass;
    struct Case { const char* name; Class resourceClass; bool value; bool ownedAlias; bool alias; };
    const Case corrupt[]{
        {"instance foreign alias",Class::InstanceEvaluated,false,false,true},
        {"unknown foreign alias",Class::Unknown,false,false,true},
        {"static absent value alias",Class::AssetStatic,false,false,true},
        {"instance value owned alias",Class::InstanceEvaluated,true,true,true},
        {"unknown value owned alias",Class::Unknown,true,true,true},
        {"instance value foreign alias",Class::InstanceEvaluated,true,false,true},
        {"unknown value foreign alias",Class::Unknown,true,false,true},
        {"static value foreign alias",Class::AssetStatic,true,false,true},
        {"static value missing alias",Class::AssetStatic,true,false,false},
    };
    const Case valid[]{
        {"instance no alias",Class::InstanceEvaluated,false,false,false},
        {"unknown no alias",Class::Unknown,false,false,false},
        {"static absent value no alias",Class::AssetStatic,false,false,false},
        {"instance value no alias",Class::InstanceEvaluated,true,false,false},
        {"unknown value no alias",Class::Unknown,true,false,false},
        {"static canonical aliases",Class::AssetStatic,true,true,true},
    };
    const auto base=frozenAlphaPlan();
    const auto makePlan=[&](const Case& test,bool geometry,bool legitimate) {
        auto plan=base;auto scene=std::make_shared<runtime::EvaluatedScene>(*base.sourceScene);
        auto frozen=std::make_shared<model::MotionAssetModel>(*scene->assetModel);
        scene->assetModel=frozen;plan.sourceScene=scene;
        // Corrupt only the last source, retaining a complete valid prefix and
        // every original resource key/hash. Finite foreign pixels differ.
        auto& item=scene->drawItems[2];
        const auto bind=[&](auto& record,auto& alias,auto payload) {
            using Alias=std::remove_reference_t<decltype(alias)>;
            record.resourceClass=test.resourceClass;
            if(legitimate && test.alias) record.contentHash=payload.contentHash;
            if(test.value) record.staticValue=payload;
            if(test.alias) alias=test.ownedAlias
                ?Alias(frozen,&*record.staticValue)
                :std::make_shared<std::remove_const_t<typename Alias::element_type>>(payload);
        };
        if(geometry || legitimate) {
            runtime::CanonicalGeometry payload;
            payload.sourceKey=item.sourceGeometryId;
            payload.path=item.localPath;payload.fillRule=item.fillRule;
            payload.contentHash=base.drawItems[2].geometry.contentHash;
            if(!legitimate) payload.path.points[0].x+=7;
            bind(frozen->geometries[2],item.canonicalGeometry,payload);
        }
        if(!geometry || legitimate) {
            runtime::CanonicalPaint payload;
            payload.sourceKey=item.sourcePaintId;
            payload.paint=item.localPaint;payload.stroke=item.localStroke;
            payload.contentHash=base.drawItems[2].paint.contentHash;
            if(!legitimate) payload.paint.solid={0,0,255,255};
            bind(frozen->paints[2],item.canonicalPaint,payload);
        }
        if(legitimate && test.alias) {
            item.geometryOrigin=item.paintOrigin=runtime::EvaluatedValueOrigin::AssetStatic;
            render::MotionRenderPlanner planner;auto built=planner.build(scene);
            require(bool(built),"legitimate canonical alpha plan");plan=std::move(built.plan);
        }
        return plan;
    };
    bool allRejected=true;unsigned rejectionChecks=0,acceptedChecks=0;
    for(bool warm:{false,true}) for(bool geometry:{true,false}) for(const auto& test:corrupt) {
        auto* factory=new FakeFactory;auto* context=new FakeDeviceContext(factory);auto* host=new FakeImage;
        context->SetTarget(host);const auto transform=context->transform();
        backends::direct2d::Backend backend;backends::direct2d::RenderSession session{context,199,1};
        if(warm) require(bool(drawAlpha(backend,base,session)),"warm canonical rejection baseline");
        const auto resources=[&] {return factory->pathGeometryCreations+factory->strokeStyleCreations+
            context->listCreations+context->effectCreations+context->solidBrushCreations+
            context->stopCreations+context->linearBrushCreations;};
        const auto beforeResources=resources(),beforeDraws=context->records.size();
        const auto failed=drawAlpha(backend,makePlan(test,geometry,false),session);
        const bool rejected=!failed && failed.error.code==backends::direct2d::BackendErrorCode::InvalidPlan &&
            failed.itemsDrawn==0 && failed.itemsSkipped==0 && resources()==beforeResources &&
            context->records.size()==beforeDraws && context->target==host && sameMatrix(context->transform(),transform);
        if(!rejected) std::cout<<"canonical provenance bypass: "<<(geometry?"geometry ":"paint ")<<test.name
            <<" warm="<<warm<<" drawn="<<failed.itemsDrawn<<" native_resources="<<resources()-beforeResources<<'\n';
        allRejected=allRejected && rejected;++rejectionChecks;
        context->Release();host->Release();factory->Release();
    }
    for(bool warm:{false,true}) for(const auto& test:valid) {
        auto* factory=new FakeFactory;auto* context=new FakeDeviceContext(factory);
        backends::direct2d::Backend backend;backends::direct2d::RenderSession session{context,199,1};
        if(warm) require(bool(drawAlpha(backend,base,session)),"warm canonical acceptance baseline");
        const auto accepted=drawAlpha(backend,makePlan(test,true,true),session);
        require(bool(accepted) && accepted.itemsDrawn==3 && accepted.itemsSkipped==0,
            std::string(test.name)+" remains admitted cold/warm");++acceptedChecks;
        context->Release();factory->Release();
    }
    std::cout<<"alpha canonical rejection checks="<<rejectionChecks<<" legitimate acceptance checks="<<acceptedChecks<<'\n';
    require(allRejected,"alpha rejects forged frozen geometry/paint aliases before native work cold/warm");
}

void alphaContracts() {
    using namespace avemotion;
    using Error = backends::direct2d::BackendErrorCode;
    auto* factory=new FakeFactory;
    auto* context=new FakeDeviceContext(factory);
    auto* host=new FakeImage;
    context->SetTarget(host);
    const D2D1_MATRIX_3X2_F transform{1.25F,.1F,.2F,1.5F,10,6};
    context->SetTransform(transform);
    backends::direct2d::Backend backend;
    backends::direct2d::RenderSession session{context,99,1};
    const auto plan=alphaPlan();
    auto result=drawAlpha(backend,plan,session);
    require(bool(result) && result.itemsDrawn==3 && result.itemsSkipped==0 &&
        context->listCreations==3 && context->effectCreations==1 && context->pairReplays==1 && context->frameReplays==1,
        "alpha records each primitive, suppresses provider, replays one isolated frame");
    require(context->target==host && sameMatrix(context->transform(),transform),"alpha restores host target and transform");
    result=backend.draw(plan,session);
    require(bool(result) && result.itemsDrawn==0 && result.itemsSkipped==3,"public skips both matte roles and Difference");
    auto partial=plan; partial.drawItems.resize(1);
    result=backend.draw(partial,session);
    require(bool(result) && result.itemsDrawn==0 && result.itemsSkipped==1,"public valid prefix keeps skip contract");
    auto solid=plan;
    auto solidScene=std::make_shared<runtime::EvaluatedScene>(*plan.sourceScene);
    for(auto& layer:solidScene->layers) { layer.matte=runtime::MatteMode::None;
        layer.matteSourceLayer=layer.matteTargetLayer=runtime::kInvalidSceneIndex;layer.isolatedBottomDifference=false; }
    render::MotionRenderPlanner planner;
    solid=planner.build(solidScene).plan;
    const auto lists=context->listCreations,effects=context->effectCreations;
    require(bool(drawAlpha(backend,solid,session)) && context->listCreations==lists && context->effectCreations==effects,
        "private no-pair no-Difference route creates zero lists/effects");
    std::size_t rejected=0;
    const auto reject=[&](const std::string& name,auto mutate) {
        auto bad=plan; auto source=std::make_shared<runtime::EvaluatedScene>(*plan.sourceScene);
        bad.sourceScene=source; mutate(*source,bad);
        const auto draws=context->records.size();
        const auto resources=context->listCreations+context->effectCreations+context->solidBrushCreations+
            context->stopCreations+context->linearBrushCreations+factory->pathGeometryCreations;
        const auto failed=drawAlpha(backend,bad,session);
        require(!failed && failed.itemsDrawn==0 && failed.itemsSkipped==0,name+" whole-plan error before valid prefix");
        require(context->records.size()==draws && resources==context->listCreations+context->effectCreations+
            context->solidBrushCreations+context->stopCreations+context->linearBrushCreations+factory->pathGeometryCreations,
            name+" rejection precedes native resources/draws");
        require(context->target==host && sameMatrix(context->transform(),transform),name+" preserves host state");
        ++rejected;
    };
    reject("reciprocity",[](auto& s,auto&){s.layers[3].matteTargetLayer=1;});
    reject("provider role",[](auto& s,auto&){s.layers[3].matte=runtime::MatteMode::Alpha;});
    reject("nonroot",[](auto& s,auto&){s.layers[2].parentLayer=1;s.layers[3].parentLayer=1;});
    reject("multidraw",[](auto& s,auto&){s.layers[2].drawItemCount=2;});
    reject("partial",[](auto&,auto& p){p.drawItems.pop_back();});
    reject("reordered",[](auto&,auto& p){std::swap(p.drawItems[1],p.drawItems[2]);});
    reject("duplicate",[](auto&,auto& p){p.drawItems[2]=p.drawItems[1];});
    reject("other matte",[](auto& s,auto&){s.layers[2].matte=runtime::MatteMode::AlphaInverted;});
    reject("mask",[](auto& s,auto&){s.layers[3].maskCount=1;});
    reject("clip",[](auto& s,auto&){s.layers[3].clipPath=rectanglePath();});
    reject("dash",[](auto& s,auto&){s.drawItems[2].localStroke.dashArray={1,2};});
    reject("radial",[](auto& s,auto& p){s.drawItems[2].localPaint.kind=runtime::PaintKind::Gradient;
        s.drawItems[2].localPaint.gradient.kind=runtime::GradientKind::Radial;
        p.drawItems[2].featureBits=render::RenderFeatureGradientPaint|render::RenderFeatureMatte;});
    reject("other blend",[](auto&,auto& p){p.drawItems[2].featureBits|=render::RenderFeatureBlend;});
    reject("nonbottom Difference",[](auto& s,auto&){s.layers[1].isolatedBottomDifference=false;s.layers[3].isolatedBottomDifference=true;});
    reject("canonical identity",[](auto& s,auto&){auto c=std::make_shared<runtime::CanonicalGeometry>();
        c->path=rectanglePath();s.drawItems[2].canonicalGeometry=c;});
    reject("unowned canonical alias",[](auto& s,auto& p){auto c=std::make_shared<runtime::CanonicalGeometry>();
        c->path=rectanglePath();c->contentHash=p.drawItems[2].geometry.contentHash;
        s.drawItems[2].canonicalGeometry=c;});
    reject("orphan model identity",[](auto& s,auto&){s.drawItems[2].modelNode={0};});
    const auto frozenPlan=frozenAlphaPlan();
    require(bool(drawAlpha(backend,frozenPlan,session)),"alpha accepts complete frozen resource bindings");
    const auto rejectFrozen=[&](const std::string& name,auto mutate) {
        auto bad=frozenPlan;auto scene=std::make_shared<runtime::EvaluatedScene>(*bad.sourceScene);
        auto frozen=std::make_shared<model::MotionAssetModel>(*scene->assetModel);scene->assetModel=frozen;bad.sourceScene=scene;
        mutate(*frozen,*scene,bad);const auto before=context->records.size();
        const auto failed=drawAlpha(backend,bad,session);
        require(!failed && failed.itemsDrawn==0 && context->records.size()==before,name+" frozen binding rejection");++rejected;
    };
    rejectFrozen("node layer",[](auto& m,auto&,auto&){m.nodes[2].layer={1};});
    rejectFrozen("record identity",[](auto& m,auto&,auto&){m.paints[2].id={1};});
    rejectFrozen("frozen pair cardinality",[](auto& m,auto&,auto&){m.layers[3].nodes.count=2;});
    rejectFrozen("frozen matte",[](auto& m,auto&,auto&){m.layers[2].matte=runtime::MatteMode::None;});
    reject("resource identity",[](auto&,auto& p){p.drawItems[2].geometry.sourceKey++;});
    reject("paint identity",[](auto&,auto& p){p.drawItems[2].paint.contentHash++;});
    reject("source identity",[](auto&,auto& p){p.drawItems[2].sourceItemKey++;});
    reject("nonfinite path",[](auto& s,auto&){s.drawItems[2].localPath.points[0].x=std::numeric_limits<float>::quiet_NaN();});
    reject("nonfinite transform",[](auto&,auto& p){p.drawItems[2].geometryTransform.dx=std::numeric_limits<float>::infinity();});
    reject("opacity range",[](auto&,auto& p){p.drawItems[2].effectiveOpacity=1.1F;});
    reject("presentation range",[](auto&,auto& p){p.presentationOpacity=-.1F;});
    reject("source opacity range",[](auto& s,auto&){s.drawItems[2].separatedOpacity=1.1F;});
    reject("invalid resource origin",[](auto& s,auto&){s.drawItems[2].geometryOrigin=static_cast<runtime::EvaluatedValueOrigin>(99);});
    reject("source range",[](auto& s,auto&){s.layers[3].firstDrawItem=999;});
    reject("layer range",[](auto& s,auto&){s.drawItems[2].layerIndex=999;});
    for(unsigned boundary=0;boundary<8;++boundary) for(auto native:{E_FAIL,D2DERR_RECREATE_TARGET}) {
        auto* failedContext=new FakeDeviceContext(factory);failedContext->SetTarget(host);failedContext->SetTransform(transform);
        failedContext->nativeFailure=native;
        if(boundary<3) failedContext->failListAt=boundary+1;
        else if(boundary<6) failedContext->failCloseAt=boundary-2;
        else if(boundary==6) failedContext->failEffect=true;
        else failedContext->failProperty=true;
        backends::direct2d::Backend failureBackend;
        const auto failed=drawAlpha(failureBackend,plan,{failedContext,99,1});
        require(!failed && failed.error.nativeCode==native && failed.error.code==
            (native==D2DERR_RECREATE_TARGET?Error::DeviceLost:Error::ResourceCreationFailed),
            "native boundary failure propagates exact code");
        const unsigned issued[]{0,1,2,3,2,3,3,3};
        require(failed.itemsDrawn==issued[boundary] && failed.itemsSkipped==0,
            "native failure counts source primitives actually issued before failure");
        require(failedContext->target==host && sameMatrix(failedContext->transform(),transform) && failedContext->frameReplays==0,
            "every native failure restores host and discards incomplete frame");
        failedContext->Release();
    }
    for(unsigned variant=0;variant<4;++variant) {
        auto hidden=std::make_shared<runtime::EvaluatedScene>(*plan.sourceScene);
        if(variant==0) hidden->layers[3].visible=false;
        if(variant==1) hidden->layers[3].opacity=0;
        render::MotionRenderPlanner hiddenPlanner;
        const auto built=hiddenPlanner.build(hidden, {.opacity=variant==2?0.F:1.F,.visible=variant!=3});
        require(bool(built),"inactive alpha plan");
        const auto drawn=drawAlpha(backend,built.plan,session);
        require(bool(drawn) && drawn.itemsDrawn==built.plan.drawItems.size() && drawn.itemsSkipped==0,
            "inactive/alpha0 pair keeps source issuance counts");
    }
    const auto creations=factory->pathGeometryCreations;
    backend.invalidateGraphicsDomain(99,1);
    require(bool(drawAlpha(backend,plan,session)) && factory->pathGeometryCreations>creations,"alpha domain invalidation recreates resources");
    ++session.graphicsGeneration;require(bool(drawAlpha(backend,plan,session)),"alpha generation change");
    auto* otherFactory=new FakeFactory;auto* other=new FakeDeviceContext(otherFactory);other->SetTarget(host);
    session.deviceContext=other;
    require(bool(drawAlpha(backend,plan,session)) && otherFactory->pathGeometryCreations>0,"alpha factory/context change");
    other->Release();otherFactory->Release();
    std::cout<<"alpha contracts rejections="<<rejected<<" injected_native_failures=16 fast_path_zero_composition=1\n";
    context->Release();host->Release();factory->Release();
}

} // namespace

int main() {
    alphaCanonicalProvenanceContracts();
    using namespace avemotion;
    alphaContracts();

    auto* factory = new FakeFactory;
    auto* context = new FakeDeviceContext(factory);
    const D2D1_MATRIX_3X2_F hostTransform{
        1.0F, 0.0F, 0.0F, 1.0F, 2.0F, 3.0F};
    context->SetTransform(hostTransform);

    backends::direct2d::Backend backend;
    const backends::direct2d::RenderSession session{
        .deviceContext = context,
        .graphicsDomainId = 42ULL,
        .graphicsGeneration = 1ULL,
    };
    const auto plan = makePlan();


    const auto first = backend.draw(plan, session);
    require(static_cast<bool>(first), "first Direct2D contract draw failed");
    require(first.itemsDrawn == 3U && first.itemsSkipped == 0U,
            "first draw reported incorrect item counts");
    require(context->records.size() == 3U,
            "fake context did not receive all draw calls");
    require(factory->pathGeometryCreations == 1U,
            "shared geometry was not reused inside the first draw");
    require(factory->strokeStyleCreations == 1U,
            "stroke style was not created exactly once");
    require(context->solidBrushCreations == 1U,
            "solid brush was not retained per device context");
    require(context->records[0].geometry == context->records[1].geometry
                && context->records[1].geometry == context->records[2].geometry,
            "fill and stroke applications did not share native geometry");
    require(context->records[0].geometry != nullptr
                && context->records[0].geometry->closed_
                && context->records[0].geometry->verbs.size() == 5U
                && context->records[0].geometry->points.size() == 4U,
            "path geometry command stream was not translated correctly");
    require(!context->records[0].stroke && !context->records[1].stroke
                && context->records[2].stroke,
            "fill/stroke dispatch is incorrect");
    require(near(context->records[0].color.r, 1.0F)
                && near(context->records[0].color.g, 0.0F)
                && near(context->records[0].opacity, 1.0F),
            "solid fill brush state is incorrect");
    require(near(context->records[1].opacity, 0.5F),
            "per-copy opacity was not applied");
    require(near(context->records[2].color.g, 1.0F)
                && near(context->records[2].strokeWidth, 3.0F)
                && context->records[2].strokeProperties.lineJoin
                    == D2D1_LINE_JOIN_BEVEL
                && context->records[2].strokeProperties.startCap
                    == D2D1_CAP_STYLE_ROUND,
            "stroke state is incorrect");
    require(near(context->records[0].transform.dx, 12.0F)
                && near(context->records[0].transform.dy, 15.0F)
                && near(context->records[1].transform.dx, 52.0F)
                && near(context->records[2].transform.dx, 92.0F),
            "geometry/presentation/host transform composition is incorrect");
    require(sameMatrix(context->transform(), hostTransform),
            "backend did not restore the host transform");

    auto diagnostics = backend.diagnostics();
    require(diagnostics.drawCalls == 1U
                && diagnostics.drawItemsVisited == 3U
                && diagnostics.solidItemsDrawn == 3U,
            "draw diagnostics are incorrect");
    require(diagnostics.geometryCacheMisses == 1U
                && diagnostics.geometryCacheHits == 2U
                && diagnostics.geometryResourcesCreated == 1U,
            "geometry cache diagnostics are incorrect");
    require(diagnostics.strokeStyleCacheMisses == 1U
                && diagnostics.strokeStylesCreated == 1U,
            "stroke-style cache diagnostics are incorrect");
    require(diagnostics.resourceDomainResets == 1U,
            "initial graphics-domain adoption performed redundant resets");

    context->records.clear();
    const auto second = backend.draw(plan, session);
    require(static_cast<bool>(second), "second Direct2D contract draw failed");
    diagnostics = backend.diagnostics();
    require(factory->pathGeometryCreations == 1U,
            "repaint recreated unchanged native geometry");
    require(factory->strokeStyleCreations == 1U,
            "repaint recreated unchanged stroke style");
    require(diagnostics.geometryCacheHits == 5U
                && diagnostics.geometryCacheMisses == 1U,
            "repaint did not hit the native geometry cache");
    require(diagnostics.strokeStyleCacheHits == 1U,
            "repaint did not hit the stroke-style cache");

    backend.invalidateGraphicsDomain(42ULL, 1ULL);
    context->records.clear();
    const auto afterInvalidation = backend.draw(plan, session);
    require(static_cast<bool>(afterInvalidation),
            "draw after graphics-domain invalidation failed");
    diagnostics = backend.diagnostics();
    require(factory->pathGeometryCreations == 2U,
            "graphics-domain invalidation did not rebuild native geometry");
    require(factory->strokeStyleCreations == 2U,
            "graphics-domain invalidation did not rebuild stroke style");
    require(context->solidBrushCreations == 2U,
            "graphics-domain invalidation did not rebuild device brush");
    require(diagnostics.resourceDomainResets == 2U,
            "explicit invalidation caused an unexpected reset count");

    auto invalidPlan = plan;
    invalidPlan.drawItems[1].sourceDrawItemIndex = 999U;
    context->SetTransform(hostTransform);
    const auto invalidResult = backend.draw(invalidPlan, session);
    require(!invalidResult
                && invalidResult.error.code
                    == backends::direct2d::BackendErrorCode::InvalidPlan,
            "invalid source index was not rejected");
    require(sameMatrix(context->transform(), hostTransform),
            "host transform was not restored on an error path");

    auto unsupportedPlan = plan;
    unsupportedPlan.drawItems.resize(1U);
    unsupportedPlan.drawItems[0].featureBits
        = render::RenderFeatureSolidPaint | render::RenderFeatureGradientPaint;
    const auto unsupported = backend.draw(unsupportedPlan, session);
    require(static_cast<bool>(unsupported)
                && unsupported.itemsDrawn == 0U
                && unsupported.itemsSkipped == 1U,
            "unsupported feature was not skipped fail-closed");

    const auto gradient = linearPlan();
    const auto publicGradient = backend.draw(gradient, session);
    require(bool(publicGradient) && publicGradient.itemsDrawn == 0 && publicGradient.itemsSkipped == 1,
            "ordinary public gradient retains skip policy");
    const auto linear = drawLinear(backend, gradient, session);
    std::cout << "private linear baseline drawn=" << linear.itemsDrawn
              << " skips=" << linear.itemsSkipped << '\n';
    require(bool(linear) && linear.itemsDrawn == 1 && linear.itemsSkipped == 0,
            "private linear fill must draw once with zero skips");
    require(context->records.back().gradient && context->records.back().stops.size() == 2 &&
            near(context->records.back().gradientProperties.endPoint.x, 20),
            "native linear fill preserves local endpoints and stops");
    const auto solidsBefore=backend.diagnostics().solidItemsDrawn;
    auto alternate=gradient;
    auto alternateScene=std::make_shared<runtime::EvaluatedScene>(*gradient.sourceScene);
    alternateScene->drawItems[0].fillRule=runtime::FillRule::EvenOdd;
    alternate.sourceScene=alternateScene;
    ++alternate.drawItems[0].geometry.contentHash;
    require(bool(drawLinear(backend,alternate,session)) && context->records.back().geometry->fillMode==D2D1_FILL_MODE_ALTERNATE
            && backend.diagnostics().solidItemsDrawn==solidsBefore, "gradient evenodd dispatch and solid-only diagnostics");

    const auto reject = [&](const std::string& name, auto mutate,
                            backends::direct2d::BackendErrorCode expected) {
        auto bad = gradient;
        auto scene = std::make_shared<runtime::EvaluatedScene>(*gradient.sourceScene);
        // Valid solid prefix must never reach the native context.
        scene->drawItems.insert(scene->drawItems.begin(), sourceItem({255,0,0,255}, false));
        scene->drawItems[0].layerIndex = 0;
        bad.sourceScene = scene;
        bad.drawItems[0].sourceDrawItemIndex = 1;
        bad.drawItems.insert(bad.drawItems.begin(), planItem(0,0,0,1,render::RenderFeatureSolidPaint,0x101));
        mutate(*scene, bad);
        const auto draws = context->records.size();
        const auto resources = factory->pathGeometryCreations + context->stopCreations + context->solidBrushCreations;
        const auto result = drawLinear(backend, bad, session);
        require(!result && result.error.code == expected && result.itemsDrawn == 0 && result.itemsSkipped == 0,
                name + " preflight error/counts");
        require(context->records.size() == draws && resources == factory->pathGeometryCreations + context->stopCreations + context->solidBrushCreations,
                name + " preflight precedes native resources and valid prefix draws");
        require(sameMatrix(context->transform(), hostTransform), name + " restores host transform");
    };
    using Error = backends::direct2d::BackendErrorCode;
    reject("source bounds", [](auto&, auto& p){p.drawItems[1].sourceDrawItemIndex=999;}, Error::InvalidPlan);
    reject("layer bounds", [](auto& s, auto&){s.drawItems[1].layerIndex=999;}, Error::InvalidPlan);
    reject("opacity", [](auto&, auto& p){p.drawItems[1].effectiveOpacity=1.01F;}, Error::InvalidPlan);
    reject("nonfinite opacity", [](auto&, auto& p){p.drawItems[1].effectiveOpacity=std::numeric_limits<float>::quiet_NaN();}, Error::InvalidPlan);
    reject("presentation", [](auto&, auto& p){p.drawItems[1].presentationTransform.dx=std::numeric_limits<float>::infinity();}, Error::InvalidPlan);
    reject("geometry matrix", [](auto&, auto& p){p.drawItems[1].geometryTransform.dx=1;}, Error::InvalidPlan);
    reject("nonfinite geometry matrix", [](auto&, auto& p){p.drawItems[1].geometryTransform.m11=std::numeric_limits<float>::infinity();}, Error::InvalidPlan);
    reject("final-space matrix", [](auto& s, auto& p){auto& i=s.drawItems[0];i.path=i.localPath;i.paint=i.localPaint;i.localPaintAvailable=false;i.localGeometryAvailable=false;p.drawItems[0].geometryTransform.dx=1;}, Error::InvalidPlan);
    reject("partial local", [](auto& s, auto&){s.drawItems[1].localGeometryAvailable=false;}, Error::InvalidPlan);
    reject("missing local", [](auto& s, auto&){auto& i=s.drawItems[1]; i.paint=i.localPaint;i.localGeometryAvailable=false;i.localPaintAvailable=false;}, Error::InvalidPlan);
    reject("cleared features", [](auto&, auto& p){p.drawItems[1].featureBits=0;}, Error::UnsupportedFeature);
    reject("radial", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.kind=runtime::GradientKind::Radial;}, Error::UnsupportedFeature);
    reject("image", [](auto& s, auto&){s.drawItems[1].localPaint.kind=runtime::PaintKind::Image;}, Error::UnsupportedFeature);
    reject("none", [](auto& s, auto&){s.drawItems[1].localPaint.kind=runtime::PaintKind::None;}, Error::UnsupportedFeature);
    reject("gradient stroke", [](auto& s, auto& p){s.drawItems[1].localStroke.enabled=true;p.drawItems[1].featureBits|=render::RenderFeatureStroke;}, Error::UnsupportedFeature);
    reject("disabled dash", [](auto& s, auto&){s.drawItems[1].localStroke.dashArray={1};}, Error::UnsupportedFeature);
    reject("clip verbs", [](auto& s, auto&){s.layers[0].clipPath.verbs={runtime::PathVerb::Close};}, Error::UnsupportedFeature);
    reject("clip points", [](auto& s, auto&){s.layers[0].clipPath.points={{1,1}};}, Error::UnsupportedFeature);
    reject("unplanned matte", [](auto& s, auto&){s.layers.emplace_back().matte=runtime::MatteMode::Alpha;}, Error::UnsupportedFeature);
    reject("unplanned layer mask", [](auto& s, auto&){s.layers.emplace_back().maskCount=1;}, Error::UnsupportedFeature);
    reject("scene mask", [](auto& s, auto&){s.masks.emplace_back();}, Error::UnsupportedFeature);
    reject("duplicate stops", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops[1].position=0;}, Error::UnsupportedFeature);
    reject("alpha tail", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops[1].color.a=128;}, Error::UnsupportedFeature);
    reject("inactive fields", [](auto& s, auto&){s.drawItems[1].localPaint.solid.r=1;}, Error::UnsupportedFeature);
    reject("radial fields", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.focalRadius=1;}, Error::UnsupportedFeature);
    reject("coincident endpoints", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.end={};}, Error::UnsupportedFeature);
    reject("nonfinite stop", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops[1].position=std::numeric_limits<float>::quiet_NaN();}, Error::UnsupportedFeature);
    reject("nonfinite endpoint", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.end.x=std::numeric_limits<float>::quiet_NaN();}, Error::UnsupportedFeature);
    reject("one stop", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops.resize(1);}, Error::UnsupportedFeature);
    reject("seventeen stops", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops.resize(17);}, Error::UnsupportedFeature);
    reject("stop beyond one", [](auto& s, auto&){s.drawItems[1].localPaint.gradient.stops[1].position=1.01F;}, Error::UnsupportedFeature);
    reject("invalid cap", [](auto& s, auto&){s.drawItems[1].localStroke.cap=static_cast<runtime::LineCap>(99);}, Error::InvalidPlan);
    reject("nonfinite geometry", [](auto& s, auto&){s.drawItems[1].localPath.points[0].x=std::numeric_limits<float>::infinity();}, Error::InvalidPlan);
    reject("nonfinite stroke", [](auto& s, auto&){s.drawItems[1].localStroke.width=std::numeric_limits<float>::infinity();}, Error::InvalidPlan);
    const auto frozen = [](auto& s, bool alias) {
        auto model = std::make_shared<model::MotionAssetModel>();
        model::MotionPaintRecord record;
        record.present=true; record.id={0}; record.resourceClass=model::ResourceClass::AssetStatic;
        record.staticValue=runtime::CanonicalPaint{};
        record.staticValue->paint=s.drawItems[1].localPaint;
        record.staticValue->stroke=s.drawItems[1].localStroke;
        model->paints.push_back(record); s.assetModel=model;
        s.drawItems[1].modelPaint={0};
        if(alias) s.drawItems[1].canonicalPaint=std::shared_ptr<const runtime::CanonicalPaint>(model,&*model->paints[0].staticValue);
        return model;
    };
    reject("absent frozen alias", [&](auto& s, auto&){frozen(s,false);}, Error::InvalidPlan);
    reject("mismatched frozen alias", [&](auto& s, auto&){frozen(s,true);s.drawItems[1].canonicalPaint=std::make_shared<runtime::CanonicalPaint>(*s.drawItems[1].canonicalPaint);}, Error::InvalidPlan);
    reject("cached radial shadows claimed solid", [&](auto& s, auto& p){auto m=frozen(s,true);m->paints[0].staticValue->paint.gradient.kind=runtime::GradientKind::Radial;s.drawItems[1].localPaint.kind=runtime::PaintKind::Solid;p.drawItems[1].featureBits=render::RenderFeatureSolidPaint;}, Error::UnsupportedFeature);
    reject("corrupt paint reference", [](auto& s, auto&){s.assetModel=std::make_shared<model::MotionAssetModel>();s.drawItems[1].modelPaint={99};}, Error::InvalidPlan);

    const auto creations = context->stopCreations;
    auto moved = gradient;
    moved.drawItems[0].presentationTransform.dx = 35;
    moved.drawItems[0].effectiveOpacity = 0.25F;
    require(bool(drawLinear(backend,moved,session)) && context->stopCreations == creations &&
            near(context->records.back().opacity,0.25F), "presentation and opacity reuse static gradient");
    auto changed = gradient;
    auto changedScene = std::make_shared<runtime::EvaluatedScene>(*gradient.sourceScene);
    changedScene->drawItems[0].localPaint.gradient.stops[0].color.r=50;
    changed.sourceScene=changedScene;
    ++changed.drawItems[0].paint.contentHash;
    require(bool(drawLinear(backend,changed,session)) && context->stopCreations == creations+1 &&
            near(context->records.back().stops[0].color.r,50.F/255.F), "paint changes replace gradient resources");
    auto distinct = gradient;
    ++distinct.drawItems[0].paint.assetIdentity;
    require(bool(drawLinear(backend,distinct,session)) && context->stopCreations == creations+2,
            "equal content distinct asset identities retain distinct resources");
    auto revised=distinct; ++revised.drawItems[0].paint.revision;
    require(bool(drawLinear(backend,revised,session)) && context->stopCreations==creations+3,
            "paint revision replaces gradient resources even with equal content");
    backend.clear();
    context->stopFailure=E_FAIL;
    const auto stopFailed=drawLinear(backend,gradient,session);
    require(!stopFailed && stopFailed.error.code==Error::ResourceCreationFailed && sameMatrix(context->transform(),hostTransform), "stop collection failure restores host");
    context->stopFailure=S_OK; context->linearFailure=D2DERR_RECREATE_TARGET;
    const auto brushFailed=drawLinear(backend,gradient,session);
    require(!brushFailed && brushFailed.error.code==Error::DeviceLost && sameMatrix(context->transform(),hostTransform), "linear brush device loss restores host");
    context->linearFailure=S_OK;
    require(bool(drawLinear(backend,gradient,session)), "gradient recovers after resource failure");
    auto prior=context->stopCreations;
    backend.invalidateGraphicsDomain(42,1);
    require(bool(drawLinear(backend,gradient,session)) && context->stopCreations==prior+1, "invalidation recreates gradient");
    prior=context->stopCreations;
    auto generation=session; ++generation.graphicsGeneration;
    require(bool(drawLinear(backend,gradient,generation)) && context->stopCreations==prior+1, "generation recreates gradient");
    prior=context->stopCreations;
    ++generation.graphicsDomainId;
    require(bool(drawLinear(backend,gradient,generation)) && context->stopCreations==prior+1, "domain recreates gradient");
    auto* nextContext=new FakeDeviceContext(factory);
    auto nextSession=generation; nextSession.deviceContext=nextContext;
    require(bool(drawLinear(backend,gradient,nextSession)) && nextContext->stopCreations==1, "context change recreates gradient");
    auto* nextFactory=new FakeFactory;
    auto* thirdContext=new FakeDeviceContext(nextFactory);
    nextSession.deviceContext=thirdContext;
    require(bool(drawLinear(backend,gradient,nextSession)) && thirdContext->stopCreations==1, "factory change recreates gradient");
    thirdContext->Release(); nextFactory->Release(); nextContext->Release();

    const auto alpha = drawAlpha(backend, alphaPlan(), session);
    std::cout << "private alpha baseline drawn=" << alpha.itemsDrawn
              << " skips=" << alpha.itemsSkipped << '\n';
    require(bool(alpha) && alpha.itemsDrawn == 3 && alpha.itemsSkipped == 0,
            "private alpha pair and isolated bottom Difference issue all three source primitives");
    context->Release();
    factory->Release();

    std::cout << "AveMotion Direct2D contract tests passed\n";
    return EXIT_SUCCESS;
}
