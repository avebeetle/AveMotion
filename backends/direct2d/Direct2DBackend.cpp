#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/model/AssetModel.hpp"
#include "LinearGradientBackendAccess.hpp"

#if !defined(_WIN32)
#error "The AveMotion Direct2D backend is available only on Windows."
#endif


#include <d2d1_1.h>
#include <d2d1helper.h>
#include <wrl/client.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <utility>

namespace avemotion::backends::direct2d {
namespace {

using Microsoft::WRL::ComPtr;

struct GeometrySlotKey final {
    render::ResourceIdentityScope scope = render::ResourceIdentityScope::Instance;
    std::uint64_t assetHash = 0;
    std::uint64_t assetIdentity = 0;
    std::uint64_t instanceId = 0;
    std::uint64_t instanceIdentity = 0;
    std::uint64_t sourceKey = 0;
    std::uint32_t resourceId = model::kInvalidModelId;

    [[nodiscard]] friend constexpr bool operator==(
        const GeometrySlotKey&,
        const GeometrySlotKey&) noexcept = default;
};

struct GeometrySlotKeyHash final {
    [[nodiscard]] std::size_t operator()(const GeometrySlotKey& key) const noexcept {
        auto value = key.assetHash;
        value ^= key.assetIdentity + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.instanceId + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.instanceIdentity + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.sourceKey + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::uint64_t>(key.resourceId) + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::uint64_t>(key.scope);
        return static_cast<std::size_t>(value);
    }
};

struct GeometryResource final {
    std::uint64_t contentHash = 0;
    std::uint64_t revision = 0;
    ComPtr<ID2D1PathGeometry> geometry;
};

struct StrokeStyleKey final {
    runtime::LineCap cap = runtime::LineCap::Flat;
    runtime::LineJoin join = runtime::LineJoin::Miter;
    std::uint32_t miterBits = 0;

    [[nodiscard]] friend constexpr bool operator==(
        const StrokeStyleKey&,
        const StrokeStyleKey&) noexcept = default;
};

struct StrokeStyleKeyHash final {
    [[nodiscard]] std::size_t operator()(const StrokeStyleKey& key) const noexcept {
        auto value = static_cast<std::uint64_t>(key.miterBits);
        value ^= static_cast<std::uint64_t>(key.cap) << 32U;
        value ^= static_cast<std::uint64_t>(key.join) << 40U;
        return static_cast<std::size_t>(value);
    }
};

[[nodiscard]] GeometrySlotKey slotKey(
    const render::GeometryCacheKey& key) noexcept {
    return {
        .scope = key.scope,
        .assetHash = key.assetHash,
        .assetIdentity = key.assetIdentity,
        .instanceId = key.instanceId,
        .instanceIdentity = key.instanceIdentity,
        .sourceKey = key.sourceKey,
        .resourceId = key.resourceId.value,
    };
}

[[nodiscard]] D2D1::Matrix3x2F toD2D(
    const render::Matrix3x2& value) noexcept {
    return D2D1::Matrix3x2F(
        value.m11,
        value.m12,
        value.m21,
        value.m22,
        value.dx,
        value.dy);
}

class TransformGuard final {
public:
    explicit TransformGuard(ID2D1DeviceContext* context) noexcept
        : context_(context) {
        context_->GetTransform(&original_);
    }

    TransformGuard(const TransformGuard&) = delete;
    TransformGuard& operator=(const TransformGuard&) = delete;

    ~TransformGuard() {
        context_->SetTransform(original_);
    }

    [[nodiscard]] const D2D1::Matrix3x2F& original() const noexcept {
        return original_;
    }

private:
    ID2D1DeviceContext* context_ = nullptr;
    D2D1::Matrix3x2F original_{};
};

[[nodiscard]] D2D1_COLOR_F toD2D(const runtime::Color8& value) noexcept {
    constexpr auto scale = 1.0F / 255.0F;
    return D2D1::ColorF(
        static_cast<float>(value.r) * scale,
        static_cast<float>(value.g) * scale,
        static_cast<float>(value.b) * scale,
        static_cast<float>(value.a) * scale);
}

[[nodiscard]] D2D1_CAP_STYLE mapCap(runtime::LineCap cap) noexcept {
    switch (cap) {
    case runtime::LineCap::Square: return D2D1_CAP_STYLE_SQUARE;
    case runtime::LineCap::Round: return D2D1_CAP_STYLE_ROUND;
    case runtime::LineCap::Flat: return D2D1_CAP_STYLE_FLAT;
    }
    return D2D1_CAP_STYLE_FLAT;
}

[[nodiscard]] D2D1_LINE_JOIN mapJoin(runtime::LineJoin join) noexcept {
    switch (join) {
    case runtime::LineJoin::Bevel: return D2D1_LINE_JOIN_BEVEL;
    case runtime::LineJoin::Round: return D2D1_LINE_JOIN_ROUND;
    case runtime::LineJoin::Miter: return D2D1_LINE_JOIN_MITER;
    }
    return D2D1_LINE_JOIN_MITER;
}

[[nodiscard]] const runtime::CanonicalGeometry* staticGeometry(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    const auto* record = scene.assetModel && item.modelGeometry.valid()
        ? scene.assetModel->geometry(item.modelGeometry)
        : nullptr;
    if (record != nullptr
        && record->resourceClass == model::ResourceClass::AssetStatic
        && record->staticValue) {
        return &*record->staticValue;
    }
    return item.canonicalGeometry.get();
}

[[nodiscard]] const runtime::CanonicalPaint* staticPaint(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    const auto* record = scene.assetModel && item.modelPaint.valid()
        ? scene.assetModel->paint(item.modelPaint)
        : nullptr;
    if (record != nullptr
        && record->resourceClass == model::ResourceClass::AssetStatic
        && record->staticValue) {
        return &*record->staticValue;
    }
    return item.canonicalPaint.get();
}

[[nodiscard]] bool usesLocalRenderingSpace(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    return staticGeometry(scene, item) != nullptr
        || (item.localGeometryAvailable && item.localPaintAvailable);
}

[[nodiscard]] const runtime::EvaluatedPath& selectedPath(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticGeometry(scene, item)) return value->path;
    return usesLocalRenderingSpace(scene, item) ? item.localPath : item.path;
}

[[nodiscard]] runtime::FillRule selectedFillRule(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticGeometry(scene, item)) return value->fillRule;
    return item.fillRule;
}

[[nodiscard]] const runtime::EvaluatedStroke& selectedStroke(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticPaint(scene, item)) return value->stroke;
    return item.localPaintAvailable ? item.localStroke : item.stroke;
}

[[nodiscard]] const runtime::EvaluatedPaint& selectedPaint(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticPaint(scene, item)) return value->paint;
    return item.localPaintAvailable ? item.localPaint : item.paint;
}

[[nodiscard]] bool finite(float value) noexcept { return std::isfinite(value); }
template <typename Matrix>
[[nodiscard]] bool finiteMatrix(const Matrix& m) noexcept {
    return finite(m.m11) && finite(m.m12) && finite(m.m21) && finite(m.m22)
        && finite(m.dx) && finite(m.dy);
}
template <typename Left, typename Right>
[[nodiscard]] bool sameMatrix(const Left& a, const Right& b) noexcept {
    return a.m11 == b.m11 && a.m12 == b.m12 && a.m21 == b.m21
        && a.m22 == b.m22 && a.dx == b.dx && a.dy == b.dy;
}
[[nodiscard]] bool validPath(const runtime::EvaluatedPath& path) noexcept {
    std::size_t count = 0;
    bool open = false;
    for (auto verb : path.verbs) {
        switch (verb) {
        case runtime::PathVerb::MoveTo: ++count; open = true; break;
        case runtime::PathVerb::LineTo: if (!open) return false; ++count; break;
        case runtime::PathVerb::CubicTo: if (!open) return false; count += 3; break;
        case runtime::PathVerb::Close: if (!open) return false; open = false; break;
        default: return false;
        }
    }
    if (count != path.points.size()) return false;
    for (auto point : path.points) if (!finite(point.x) || !finite(point.y)) return false;
    return true;
}
[[nodiscard]] BackendErrorCode preflight(const render::MotionRenderPlan& plan) noexcept {
    using Error = BackendErrorCode;
    const auto& scene = *plan.sourceScene;
    if (!scene.masks.empty()) return Error::UnsupportedFeature;
    for (const auto& layer : scene.layers) {
        if (layer.maskCount != 0 || layer.matte != runtime::MatteMode::None
            || !layer.clipPath.verbs.empty() || !layer.clipPath.points.empty())
            return Error::UnsupportedFeature;
    }
    for (const auto& item : plan.drawItems) {
        if (item.sourceDrawItemIndex >= scene.drawItems.size()) return Error::InvalidPlan;
        const auto& source = scene.drawItems[item.sourceDrawItemIndex];
        if (source.layerIndex >= scene.layers.size()
            || !finiteMatrix(item.geometryTransform) || !finiteMatrix(item.presentationTransform)
            || !finite(item.effectiveOpacity) || item.effectiveOpacity < 0 || item.effectiveOpacity > 1
            || source.localGeometryAvailable != source.localPaintAvailable)
            return Error::InvalidPlan;
        // The executor's frozen-model selection must be precisely the alias
        // which the planner selected. A missing alias cannot repair a plan.
        if (scene.assetModel && source.modelGeometry.valid()) {
            const auto* record = scene.assetModel->geometry(source.modelGeometry);
            if (!record || (record->resourceClass == model::ResourceClass::AssetStatic
                && record->staticValue && source.canonicalGeometry.get() != &*record->staticValue))
                return Error::InvalidPlan;
        }
        if (scene.assetModel && source.modelPaint.valid()) {
            const auto* record = scene.assetModel->paint(source.modelPaint);
            if (!record || (record->resourceClass == model::ResourceClass::AssetStatic
                && record->staticValue && source.canonicalPaint.get() != &*record->staticValue))
                return Error::InvalidPlan;
        }
        if ((source.canonicalGeometry || source.canonicalPaint)
            && !(source.localGeometryAvailable && source.localPaintAvailable)) return Error::InvalidPlan;
        const auto local = usesLocalRenderingSpace(scene, source);
        if ((local && (!finiteMatrix(source.localToViewport)
                      || !sameMatrix(item.geometryTransform, source.localToViewport)))
            || (!local && !sameMatrix(item.geometryTransform, render::Matrix3x2::identity())))
            return Error::InvalidPlan;
        const auto& paint = selectedPaint(scene, source);
        const auto& stroke = selectedStroke(scene, source);
        if (!validPath(selectedPath(scene, source))
            || (selectedFillRule(scene, source) != runtime::FillRule::EvenOdd
                && selectedFillRule(scene, source) != runtime::FillRule::Winding)
            || !finite(stroke.width) || stroke.width < 0 || !finite(stroke.miterLimit)
            || stroke.miterLimit < 0
            || (stroke.cap != runtime::LineCap::Flat && stroke.cap != runtime::LineCap::Square
                && stroke.cap != runtime::LineCap::Round)
            || (stroke.join != runtime::LineJoin::Miter && stroke.join != runtime::LineJoin::Bevel
                && stroke.join != runtime::LineJoin::Round)) return Error::InvalidPlan;
        if (!stroke.dashArray.empty()) return Error::UnsupportedFeature;
        std::uint32_t bits = stroke.enabled ? render::RenderFeatureStroke : 0U;
        if (paint.kind == runtime::PaintKind::Solid) bits |= render::RenderFeatureSolidPaint;
        else if (paint.kind == runtime::PaintKind::Gradient) bits |= render::RenderFeatureGradientPaint;
        else return Error::UnsupportedFeature;
        if (bits != item.featureBits) return Error::UnsupportedFeature;
        if (paint.kind == runtime::PaintKind::Gradient) {
            if (stroke.enabled || paint.gradient.kind != runtime::GradientKind::Linear)
                return Error::UnsupportedFeature;
            if (!local || !source.localGeometryAvailable || !source.localPaintAvailable)
                return Error::InvalidPlan;
            const auto& g = paint.gradient;
            if (!finite(g.start.x) || !finite(g.start.y) || !finite(g.end.x) || !finite(g.end.y)
                || (g.start.x == g.end.x && g.start.y == g.end.y)
                || g.stops.size() < 2 || g.stops.size() > 16
                || g.center.x != 0 || g.center.y != 0 || g.focal.x != 0 || g.focal.y != 0
                || g.centerRadius != 0 || g.focalRadius != 0
                || paint.solid.r != 0 || paint.solid.g != 0 || paint.solid.b != 0 || paint.solid.a != 0
                || paint.image.present || paint.image.width != 0 || paint.image.height != 0)
                return Error::UnsupportedFeature;
            for (const auto value : paint.image.matrix) if (value != 0) return Error::UnsupportedFeature;
            float previous = -1;
            const auto alpha = g.stops.front().color.a;
            for (const auto& stop : g.stops) {
                if (!finite(stop.position) || stop.position < 0 || stop.position > 1
                    || stop.position <= previous || stop.color.a != alpha) return Error::UnsupportedFeature;
                previous = stop.position;
            }
        }
    }
    return Error::None;
}

struct GradientResource final {
    std::uint64_t contentHash = 0;
    std::uint64_t revision = 0;
    ComPtr<ID2D1GradientStopCollection> stops;
    ComPtr<ID2D1LinearGradientBrush> brush;
};
[[nodiscard]] GeometrySlotKey slotKey(const render::PaintCacheKey& key) noexcept {
    return {key.scope, key.assetHash, key.assetIdentity, key.instanceId,
        key.instanceIdentity, key.sourceKey, key.resourceId.value};
}

[[nodiscard]] HRESULT buildPathGeometry(
    ID2D1Factory* factory,
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item,
    ID2D1PathGeometry** output) noexcept {
    if (factory == nullptr || output == nullptr) {
        return E_INVALIDARG;
    }
    *output = nullptr;
    ComPtr<ID2D1PathGeometry> geometry;
    auto hr = factory->CreatePathGeometry(&geometry);
    if (FAILED(hr)) {
        return hr;
    }
    ComPtr<ID2D1GeometrySink> sink;
    hr = geometry->Open(&sink);
    if (FAILED(hr)) {
        return hr;
    }
    const auto& path = selectedPath(scene, item);
    sink->SetFillMode(selectedFillRule(scene, item) == runtime::FillRule::EvenOdd
        ? D2D1_FILL_MODE_ALTERNATE
        : D2D1_FILL_MODE_WINDING);

    std::size_t pointIndex = 0;
    bool figureOpen = false;
    for (const auto verb : path.verbs) {
        switch (verb) {
        case runtime::PathVerb::MoveTo:
            if (pointIndex >= path.points.size()) {
                return E_INVALIDARG;
            }
            if (figureOpen) {
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
            }
            sink->BeginFigure(
                D2D1::Point2F(
                    path.points[pointIndex].x,
                    path.points[pointIndex].y),
                D2D1_FIGURE_BEGIN_FILLED);
            ++pointIndex;
            figureOpen = true;
            break;
        case runtime::PathVerb::LineTo:
            if (!figureOpen || pointIndex >= path.points.size()) {
                return E_INVALIDARG;
            }
            sink->AddLine(D2D1::Point2F(
                path.points[pointIndex].x,
                path.points[pointIndex].y));
            ++pointIndex;
            break;
        case runtime::PathVerb::CubicTo:
            if (!figureOpen || pointIndex + 2U >= path.points.size()) {
                return E_INVALIDARG;
            }
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(
                    path.points[pointIndex].x,
                    path.points[pointIndex].y),
                D2D1::Point2F(
                    path.points[pointIndex + 1U].x,
                    path.points[pointIndex + 1U].y),
                D2D1::Point2F(
                    path.points[pointIndex + 2U].x,
                    path.points[pointIndex + 2U].y)));
            pointIndex += 3U;
            break;
        case runtime::PathVerb::Close:
            if (!figureOpen) {
                return E_INVALIDARG;
            }
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            figureOpen = false;
            break;
        }
    }
    if (figureOpen) {
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    if (pointIndex != path.points.size()) {
        return E_INVALIDARG;
    }
    hr = sink->Close();
    if (FAILED(hr)) {
        return hr;
    }
    *output = geometry.Detach();
    return S_OK;
}

} // namespace

struct Backend::Impl final {
    std::uint64_t graphicsDomainId = 0;
    std::uint64_t graphicsGeneration = 0;
    ID2D1Factory* factoryIdentity = nullptr;
    ComPtr<ID2D1Factory> factory;
    ID2D1DeviceContext* brushContextIdentity = nullptr;
    ComPtr<ID2D1SolidColorBrush> solidBrush;
    std::unordered_map<
        GeometrySlotKey,
        GeometryResource,
        GeometrySlotKeyHash> geometryCache;
    std::unordered_map<
        StrokeStyleKey,
        ComPtr<ID2D1StrokeStyle>,
        StrokeStyleKeyHash> strokeStyleCache;
    BackendDiagnostics diagnostics;
    std::unordered_map<GeometrySlotKey, GradientResource, GeometrySlotKeyHash> gradientCache;

    void resetResources() noexcept {
        geometryCache.clear();
        strokeStyleCache.clear();
        gradientCache.clear();
        solidBrush.Reset();
        brushContextIdentity = nullptr;
        factory.Reset();
        factoryIdentity = nullptr;
        ++diagnostics.resourceDomainResets;
    }

    [[nodiscard]] HRESULT ensureDomain(const RenderSession& session) noexcept {
        const auto domainChanged = graphicsDomainId != session.graphicsDomainId
            || graphicsGeneration != session.graphicsGeneration;
        if (domainChanged) {
            resetResources();
            graphicsDomainId = session.graphicsDomainId;
            graphicsGeneration = session.graphicsGeneration;
        }
        ComPtr<ID2D1Factory> currentFactory;
        session.deviceContext->GetFactory(&currentFactory);
        if (!currentFactory) {
            return E_FAIL;
        }
        if (factoryIdentity != currentFactory.Get()) {
            // A graphics-domain transition already cleared the resources.
            // Avoid counting and performing a second reset merely because the
            // new factory is being adopted for the first time.
            if (!domainChanged && factoryIdentity != nullptr) {
                resetResources();
            }
            graphicsDomainId = session.graphicsDomainId;
            graphicsGeneration = session.graphicsGeneration;
            factory = std::move(currentFactory);
            factoryIdentity = factory.Get();
        }
        if (brushContextIdentity != session.deviceContext || !solidBrush) {
            gradientCache.clear();
            solidBrush.Reset();
            const auto hr = session.deviceContext->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &solidBrush);
            if (FAILED(hr)) {
                return hr;
            }
            brushContextIdentity = session.deviceContext;
        }
        return S_OK;
    }
};

Backend::Backend()
    : impl_(std::make_unique<Impl>()) {
}

Backend::Backend(Backend&&) noexcept = default;
Backend& Backend::operator=(Backend&&) noexcept = default;
Backend::~Backend() = default;

DrawResult Backend::draw(
    const render::MotionRenderPlan& plan,
    const RenderSession& session) {
    return execute(plan, session, false);
}

DrawResult detail::LinearGradientBackendAccess::draw(Backend& backend,
    const render::MotionRenderPlan& plan, const RenderSession& session) {
    return backend.execute(plan, session, true);
}

DrawResult Backend::execute(const render::MotionRenderPlan& plan,
    const RenderSession& session, bool linearFillEnabled) {
    DrawResult result;
    if (session.deviceContext == nullptr || !plan.sourceScene) {
        result.error = {
            BackendErrorCode::InvalidArgument,
            static_cast<long>(E_INVALIDARG),
            "a valid device context and render plan source scene are required"};
        return result;
    }
    if (linearFillEnabled) {
        const auto error = preflight(plan);
        if (error != BackendErrorCode::None) {
            result.error = {error, static_cast<long>(E_INVALIDARG),
                "private linear-fill plan failed whole-plan preflight"};
            return result;
        }
    }
    const auto domainResult = impl_->ensureDomain(session);
    if (FAILED(domainResult)) {
        result.error = {
            domainResult == D2DERR_RECREATE_TARGET
                ? BackendErrorCode::DeviceLost
                : BackendErrorCode::ResourceCreationFailed,
            static_cast<long>(domainResult),
            "unable to initialize the Direct2D resource domain"};
        return result;
    }

    const TransformGuard transformGuard(session.deviceContext);
    ++impl_->diagnostics.drawCalls;

    for (const auto& planItem : plan.drawItems) {
        ++impl_->diagnostics.drawItemsVisited;
        if (planItem.sourceDrawItemIndex >= plan.sourceScene->drawItems.size()) {
            result.error = {
                BackendErrorCode::InvalidPlan,
                static_cast<long>(E_INVALIDARG),
                "draw item references an invalid source item"};
            return result;
        }
        const auto& source = plan.sourceScene->drawItems[planItem.sourceDrawItemIndex];
        const auto& paint = selectedPaint(*plan.sourceScene, source);
        const auto& stroke = selectedStroke(*plan.sourceScene, source);
        const auto unsupported = planItem.featureBits
            & ~(render::RenderFeatureSolidPaint | render::RenderFeatureStroke);
        if (!linearFillEnabled && (unsupported != 0U || paint.kind != runtime::PaintKind::Solid)) {
            ++result.itemsSkipped;
            ++impl_->diagnostics.unsupportedItemsSkipped;
            continue;
        }

        const auto nativeSlot = slotKey(planItem.geometry);
        auto found = impl_->geometryCache.find(nativeSlot);
        ComPtr<ID2D1PathGeometry> geometry;
        if (found != impl_->geometryCache.end()
            && found->second.contentHash == planItem.geometry.contentHash
            && found->second.revision == planItem.geometry.revision) {
            geometry = found->second.geometry;
            ++impl_->diagnostics.geometryCacheHits;
        } else {
            ++impl_->diagnostics.geometryCacheMisses;
            const auto hr = buildPathGeometry(
                impl_->factory.Get(), *plan.sourceScene, source, &geometry);
            if (FAILED(hr)) {
                result.error = {
                    BackendErrorCode::ResourceCreationFailed,
                    static_cast<long>(hr),
                    "unable to create a Direct2D path geometry"};
                return result;
            }
            GeometryResource resource{
                .contentHash = planItem.geometry.contentHash,
                .revision = planItem.geometry.revision,
                .geometry = geometry,
            };
            if (found == impl_->geometryCache.end()) {
                impl_->geometryCache.emplace(nativeSlot, std::move(resource));
                ++impl_->diagnostics.geometryResourcesCreated;
            } else {
                found->second = std::move(resource);
                ++impl_->diagnostics.geometryResourcesReplaced;
            }
        }

        ID2D1Brush* brush = impl_->solidBrush.Get();
        if (paint.kind == runtime::PaintKind::Gradient) {
            const auto key = slotKey(planItem.paint);
            auto gradient = impl_->gradientCache.find(key);
            if (gradient == impl_->gradientCache.end()
                || gradient->second.contentHash != planItem.paint.contentHash
                || gradient->second.revision != planItem.paint.revision) {
                GradientResource resource;
                resource.contentHash = planItem.paint.contentHash;
                resource.revision = planItem.paint.revision;
                std::vector<D2D1_GRADIENT_STOP> stops;
                stops.reserve(paint.gradient.stops.size());
                for (const auto& stop : paint.gradient.stops) stops.push_back({stop.position, toD2D(stop.color)});
                auto hr = session.deviceContext->CreateGradientStopCollection(stops.data(),
                    static_cast<UINT32>(stops.size()), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &resource.stops);
                if (SUCCEEDED(hr)) {
                    const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES properties{
                        D2D1::Point2F(paint.gradient.start.x, paint.gradient.start.y),
                        D2D1::Point2F(paint.gradient.end.x, paint.gradient.end.y)};
                    hr = session.deviceContext->CreateLinearGradientBrush(properties, resource.stops.Get(), &resource.brush);
                }
                if (FAILED(hr)) {
                    result.error = {hr == D2DERR_RECREATE_TARGET ? BackendErrorCode::DeviceLost
                        : BackendErrorCode::ResourceCreationFailed, static_cast<long>(hr),
                        "unable to create a Direct2D linear gradient"};
                    return result;
                }
                gradient = impl_->gradientCache.insert_or_assign(key, std::move(resource)).first;
            }
            brush = gradient->second.brush.Get();
        } else {
            impl_->solidBrush->SetColor(toD2D(paint.solid));
        }
        brush->SetOpacity(planItem.effectiveOpacity);
        session.deviceContext->SetTransform(
            toD2D(planItem.geometryTransform)
            * toD2D(planItem.presentationTransform)
            * transformGuard.original());
        if (stroke.enabled) {
            const StrokeStyleKey styleKey{
                .cap = stroke.cap,
                .join = stroke.join,
                .miterBits = std::bit_cast<std::uint32_t>(stroke.miterLimit),
            };
            ComPtr<ID2D1StrokeStyle> strokeStyle;
            const auto styleFound = impl_->strokeStyleCache.find(styleKey);
            if (styleFound != impl_->strokeStyleCache.end()) {
                strokeStyle = styleFound->second;
                ++impl_->diagnostics.strokeStyleCacheHits;
            } else {
                ++impl_->diagnostics.strokeStyleCacheMisses;
                const auto properties = D2D1::StrokeStyleProperties(
                    mapCap(stroke.cap),
                    mapCap(stroke.cap),
                    mapCap(stroke.cap),
                    mapJoin(stroke.join),
                    stroke.miterLimit,
                    D2D1_DASH_STYLE_SOLID,
                    0.0F);
                const auto hr = impl_->factory->CreateStrokeStyle(
                    properties,
                    nullptr,
                    0U,
                    &strokeStyle);
                if (FAILED(hr)) {
                    result.error = {
                        BackendErrorCode::ResourceCreationFailed,
                        static_cast<long>(hr),
                        "unable to create a Direct2D stroke style"};
                    return result;
                }
                impl_->strokeStyleCache.emplace(styleKey, strokeStyle);
                ++impl_->diagnostics.strokeStylesCreated;
            }
            session.deviceContext->DrawGeometry(
                geometry.Get(),
                brush,
                stroke.width,
                strokeStyle.Get());
        } else {
            session.deviceContext->FillGeometry(
                geometry.Get(),
                brush);
        }
        ++result.itemsDrawn;
        if (paint.kind == runtime::PaintKind::Solid) ++impl_->diagnostics.solidItemsDrawn;
    }
    return result;
}

void Backend::invalidateGraphicsDomain(
    std::uint64_t graphicsDomainId,
    std::uint64_t graphicsGeneration) noexcept {
    if (impl_->graphicsDomainId == graphicsDomainId
        && impl_->graphicsGeneration == graphicsGeneration) {
        impl_->resetResources();
    }
}

void Backend::clear() noexcept {
    impl_->resetResources();
    impl_->graphicsDomainId = 0;
    impl_->graphicsGeneration = 0;
}

BackendDiagnostics Backend::diagnostics() const noexcept {
    return impl_->diagnostics;
}

} // namespace avemotion::backends::direct2d
