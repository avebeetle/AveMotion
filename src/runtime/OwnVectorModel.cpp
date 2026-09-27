#include "OwnVectorModel.hpp"
#include "NativeEllipseAdmissionCore.hpp"
#include "OwnVectorPropertyCompiler.hpp"
#include "avemotion/core/Hash.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <set>

namespace avemotion::runtime::detail {
namespace {
using namespace model;
using namespace vector_compile;
class Compiler final {
public:
    explicit Compiler(const formats::detail::OwnJsonDocument& document)
        : root_{&document, 0, ""}, properties_(*model) {}
    std::shared_ptr<MotionAssetModel> model = std::make_shared<MotionAssetModel>();
    std::vector<OwnVectorLayerBinding> layers;
    std::vector<OwnVectorDrawBinding> draws;
    void run() {
        root_.keys({"v", "nm", "tgs", "ddd", "w", "h", "fr", "ip", "op", "assets", "layers"});
        root_.metadata();
        if (root_.get("v").exists())
            (void)root_.get("v").string();
        if (root_.get("tgs").exists())
            (void)root_.get("tgs").number(1, 1, true);
        (void)root_.get("ip").number(0, 0, true);
        model->logicalWidth = static_cast<std::size_t>(root_.get("w").number(1, 4096, true));
        model->logicalHeight = static_cast<std::size_t>(root_.get("h").number(1, 4096, true));
        model->totalFrames = static_cast<std::size_t>(root_.get("op").number(2, 10000, true));
        model->frameRate = root_.get("fr").number(0, 240, false, true);
        model->debugName = root_.get("nm").exists() ? root_.get("nm").string() : "";
        const auto root = node(SourceNodeKind::Composition, {}, root_);
        model->compositions.push_back({true, makeId<CompositionId>(0), root, model->debugName,
                                       model->logicalWidth, model->logicalHeight, 0,
                                       static_cast<double>(model->totalFrames), model->frameRate});
        if (root_.get("assets").exists())
            assets_ = root_.get("assets").array(0, 1);
        for (const auto& asset : assets_) {
            asset.keys({"id", "layers"});
            if (asset.get("id").string().empty())
                asset.get("id").fail("empty asset ID");
            (void)asset.get("layers").array(1, 128);
        }
        composition(root_.get("layers"), root, 0, static_cast<double>(model->totalFrames), false);
        if (!assets_.empty() && !usedAsset_)
            root_.get("assets").fail("unused asset outside admitted subset");
        finalize();
    }

private:
    Value root_;
    PropertyCompiler properties_;
    std::vector<Value> assets_;
    bool usedAsset_ = false;
    std::size_t pathCount_ = 0;
    SourceNodeId node(SourceNodeKind kind, SourceNodeId parent, Value source) {
        MotionSourceNodeRecord n;
        n.present = true;
        n.id = makeId<SourceNodeId>(model->sourceNodes.size());
        n.parent = parent;
        n.kind = kind;
        n.composition = makeId<CompositionId>(0);
        n.debugName = source.get("nm").exists() ? source.get("nm").string() : "";
        core::Fnv1a64 hash;
        hash.appendString(n.debugName);
        n.nameHash = hash.value();
        model->sourceNodes.push_back(n);
        return n.id;
    }
    PropertyId scalar(SourceNodeId owner, Value object, const char* field,
                      PropertySemantic semantic, float fallback, std::int64_t low,
                      std::int64_t high, bool animate = true, bool strictLow = false) {
        const auto p = object.get(field);
        return p.exists() ? properties_.add(owner, semantic, p, PropertyValueType::Scalar, low,
                                            high, animate, 0, strictLow)
                          : properties_.scalar(owner, semantic, fallback);
    }
    PropertyId vec2(SourceNodeId owner, Value object, const char* field, PropertySemantic semantic,
                    MotionVec2Value fallback, std::int64_t low, std::int64_t high,
                    bool animate = true, float z = 0) {
        const auto p = object.get(field);
        return p.exists() ? properties_.add(owner, semantic, p, PropertyValueType::Vec2, low, high,
                                            animate, z)
                          : properties_.vec2(owner, semantic, fallback);
    }
    void identityTranslation(Value object) {
        // Missing position/anchor components use exact zero. Compare authored
        // decimals before conversion so distinct inputs cannot round to identity.
        std::array<std::string_view, 2> position{"0", "0"}, anchor{"0", "0"};
        const auto tokens = [](Value property, auto& output) {
            if (!property.exists())
                return;
            const auto components = property.get("k").array(2, 3);
            for (std::size_t i = 0; i < 2; ++i) {
                components[i].require(formats::detail::OwnJsonKind::Number);
                output[i] = *components[i].document->valueBytes(components[i].id);
            }
        };
        tokens(object.get("p"), position);
        tokens(object.get("a"), anchor);
        for (std::size_t i = 0; i < 2; ++i)
            if (!equalOwnNumericTokens(position[i], anchor[i]))
                object.get("p").fail("precomp position and anchor must be exactly equal");
    }
    void transform(SourceNodeId owner, Value object, bool group, bool precomp = false) {
        if (!object.exists()) {
            properties_.vec2(owner, PropertySemantic::TransformPosition, {});
            properties_.vec2(owner, PropertySemantic::TransformAnchor, {});
            properties_.vec2(owner, PropertySemantic::TransformScale, {100, 100});
            properties_.scalar(owner, PropertySemantic::TransformRotation, 0);
            properties_.scalar(owner, PropertySemantic::TransformOpacity, 100);
            return;
        }
        if (group)
            object.keys({"ty", "nm", "p", "a", "s", "r", "o", "sk", "sa"});
        else
            object.keys({"p", "a", "s", "r", "o"});
        if (group && object.get("nm").exists())
            (void)object.get("nm").string();
        if (precomp)
            identityTranslation(object);
        vec2(owner, object, "p", PropertySemantic::TransformPosition, {}, -32768, 32768,
             !group && !precomp);
        vec2(owner, object, "a", PropertySemantic::TransformAnchor, {}, group ? 0 : -32768,
             group ? 0 : 32768, false);
        vec2(owner, object, "s", PropertySemantic::TransformScale, {100, 100},
             group || precomp ? 100 : -1000, group || precomp ? 100 : 1000, !group && !precomp,
             100);
        scalar(owner, object, "r", PropertySemantic::TransformRotation, 0,
               group || precomp ? 0 : -32768, group || precomp ? 0 : 32768, !group && !precomp);
        scalar(owner, object, "o", PropertySemantic::TransformOpacity, 100,
               group || precomp ? 100 : 0, 100, !group && !precomp);
        if (group) {
            for (auto field : {"sk", "sa"})
                if (object.get(field).exists()) {
                    const auto value = object.get(field);
                    value.keys({"a", "k", "ix"});
                    if (value.get("a").exists())
                        (void)value.get("a").number(0, 0, true);
                    if (value.get("ix").exists())
                        (void)value.get("ix").number(0, 32768, true);
                    (void)value.get("k").number(0, 0);
                }
        }
    }
    SourceNodeId paint(Value input, SourceNodeId group, bool stroke) {
        if (stroke)
            input.keys({"ty", "nm", "c", "o", "w", "lc", "lj", "ml", "bm", "hd"});
        else
            input.keys({"ty", "nm", "c", "o", "r", "bm", "hd"});
        input.metadata();
        const auto id = node(stroke ? SourceNodeKind::Stroke : SourceNodeKind::Fill, group, input);
        properties_.add(id, stroke ? PropertySemantic::StrokeColor : PropertySemantic::FillColor,
                        input.get("c"), PropertyValueType::Color, 0, 1, false);
        scalar(id, input, "o",
               stroke ? PropertySemantic::StrokeOpacity : PropertySemantic::FillOpacity, 100, 0,
               100, false);
        if (stroke) {
            properties_.add(id, PropertySemantic::StrokeWidth, input.get("w"),
                            PropertyValueType::Scalar, 0, 1024, false, 0, true);
            auto& n = model->sourceNodes[id.index()];
            const int cap =
                input.get("lc").exists() ? static_cast<int>(input.get("lc").number(1, 3, true)) : 1;
            const int join =
                input.get("lj").exists() ? static_cast<int>(input.get("lj").number(1, 3, true)) : 1;
            n.strokeCap = cap == 1   ? SourceStrokeCap::Flat
                          : cap == 2 ? SourceStrokeCap::Round
                                     : SourceStrokeCap::Square;
            n.strokeJoin = join == 1   ? SourceStrokeJoin::Miter
                           : join == 2 ? SourceStrokeJoin::Round
                                       : SourceStrokeJoin::Bevel;
            n.miterLimit = input.get("ml").exists() ? input.get("ml").number(1, 1024) : 4;
        } else {
            const auto rule = input.get("r").exists() ? input.get("r").number(1, 2, true) : 1;
            model->sourceNodes[id.index()].fillRule =
                rule == 1 ? SourceFillRule::Winding : SourceFillRule::EvenOdd;
        }
        return id;
    }
    void shapes(Value input, SourceNodeId layer) {
        const auto items = input.array(1, 2);
        const auto groupValue = items[0];
        groupValue.keys({"ty", "nm", "it", "bm", "hd"});
        groupValue.metadata();
        if (groupValue.get("ty").string() != "gr")
            groupValue.fail("one top-level shape group required");
        const auto group = node(SourceNodeKind::ShapeGroup, layer, groupValue);
        const auto contents = groupValue.get("it").array(2, 4);
        if (contents.front().get("ty").string() != "sh" ||
            contents.back().get("ty").string() != "tr")
            groupValue.get("it").fail("expected path, optional stroke/fill, then transform");
        const auto pathValue = contents.front();
        pathValue.keys({"ty", "nm", "ind", "ks", "hd"});
        pathValue.metadata();
        if (pathValue.get("ind").exists())
            (void)pathValue.get("ind").number(0, 32768, true);
        if (pathCount_ >= 128)
            pathValue.fail("path count limit exceeded");
        ++pathCount_;
        const auto path = node(SourceNodeKind::Shape, group, pathValue);
        properties_.add(path, PropertySemantic::ShapePath, pathValue.get("ks"),
                        PropertyValueType::Shape, -32768, 32768);
        std::vector<SourceNodeId> paints;
        bool sawStroke = false, sawFill = false;
        for (std::size_t n = 1; n + 1 < contents.size(); ++n) {
            const auto type = contents[n].get("ty").string();
            if (type == "st" && !sawStroke && !sawFill) {
                paints.push_back(paint(contents[n], group, true));
                sawStroke = true;
            } else if (type == "fl" && !sawFill) {
                paints.push_back(paint(contents[n], group, false));
                sawFill = true;
            } else
                contents[n].fail("expected optional stroke then optional fill");
        }
        transform(group, contents.back(), true);
        std::optional<SourceNodeId> trim;
        if (items.size() == 2) {
            const auto t = items[1];
            t.keys({"ty", "nm", "s", "e", "o", "m", "hd"});
            t.metadata();
            if (t.get("ty").string() != "tm")
                t.fail("only trailing trim supported");
            (void)t.get("m").number(1, 1, true);
            trim = node(SourceNodeKind::Trim, layer, t);
            model->sourceNodes[trim->index()].trimMode = SourceTrimMode::Simultaneous;
            scalar(*trim, t, "s", PropertySemantic::TrimStart, 0, 0, 100);
            scalar(*trim, t, "e", PropertySemantic::TrimEnd, 100, 0, 100);
            scalar(*trim, t, "o", PropertySemantic::TrimOffset, 0, 0, 0, false);
        }
        if (paints.size() > 256 - draws.size())
            input.fail("painted draw limit exceeded");
        for (auto it = paints.rbegin(); it != paints.rend(); ++it)
            draws.push_back({layer, group, path, *it, trim});
    }
    void composition(Value input, SourceNodeId structural, double first, double end, bool nested) {
        const auto entries = input.array(1, 128);
        if (entries.size() > 128 - layers.size())
            input.fail("expanded layer limit exceeded");
        std::map<int, SourceNodeId> ids;
        std::vector<SourceNodeId> nodes;
        for (const auto& source : entries) {
            source.keys({"ty", "nm", "ind", "parent", "ks", "ip", "op", "st", "sr", "bm", "hd",
                         "ddd", "ao", "shapes", "refId", "w", "h"});
            source.metadata();
            const auto authored = static_cast<int>(source.get("ind").number(1, 32768, true));
            const auto kind = source.get("ty").number(0, 4, true);
            if (kind != 0 && kind != 3 && kind != 4)
                source.get("ty").fail("only shape/null/eligible root precomp layers supported");
            if (kind == 0 && (nested || usedAsset_))
                source.get("ty").fail("nested or multiple precomp instances unsupported");
            if (source.get("st").exists())
                (void)source.get("st").number(0, 0);
            if (source.get("sr").exists())
                (void)source.get("sr").number(1, 1);
            const auto in = source.get("ip").number(0, 20000, true),
                       out = source.get("op").number(0, 20000, true);
            if (out <= in)
                source.get("op").fail("layer interval must be nonempty");
            const auto id = node(SourceNodeKind::Layer, structural, source);
            auto& n = model->sourceNodes[id.index()];
            n.authoredLayerId = authored;
            if (source.get("parent").exists())
                n.authoredParentLayerId =
                    static_cast<int>(source.get("parent").number(1, 32768, true));
            n.layerKind = kind == 0   ? SourceLayerKind::Precomposition
                          : kind == 3 ? SourceLayerKind::Null
                                      : SourceLayerKind::Shape;
            n.inFrame = in;
            n.outFrame = out;
            if (!ids.emplace(authored, id).second)
                source.get("ind").fail("duplicate layer ID in composition");
            const auto visibleStart = std::max(first, static_cast<double>(in));
            const auto visibleEnd = std::max(visibleStart, std::min(end, static_cast<double>(out)));
            nodes.push_back(id);
            layers.push_back({id, structural, visibleStart, visibleEnd});
            transform(id, source.get("ks"), false, kind == 0);
        }
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            auto& n = model->sourceNodes[nodes[i].index()];
            n.transformParent = structural;
            if (n.authoredParentLayerId != -1) {
                const auto found = ids.find(n.authoredParentLayerId);
                if (found == ids.end() || found->second == n.id)
                    entries[i].get("parent").fail("missing/self transform parent");
                n.transformParent = found->second;
            }
        }
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            std::set<std::uint32_t> seen;
            auto id = nodes[i];
            while (id != structural) {
                if (!seen.insert(id.value).second)
                    entries[i].get("parent").fail("cyclic transform parent graph");
                id = model->sourceNodes[id.index()].transformParent;
            }
        }
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const auto source = entries[i];
            const auto id = nodes[i];
            const auto kind = model->sourceNodes[id.index()].layerKind;
            if (kind == SourceLayerKind::Shape)
                shapes(source.get("shapes"), id);
            else if (source.get("shapes").exists())
                source.get("shapes").fail("non-shape layer cannot carry shapes");
            if (kind != SourceLayerKind::Precomposition) {
                for (auto key : {"refId", "w", "h"})
                    if (source.get(key).exists())
                        source.get(key).fail("precomp-only field");
                continue;
            }
            if (usedAsset_)
                source.fail("only one root precomp instance supported");
            if (source.get("parent").exists())
                source.get("parent").fail("precomp must have identity world transform");
            if (assets_.size() != 1 ||
                source.get("refId").string() != assets_[0].get("id").string())
                source.get("refId").fail("missing precomp asset");
            (void)source.get("w").number(static_cast<std::int64_t>(model->logicalWidth),
                                         static_cast<std::int64_t>(model->logicalWidth), true);
            (void)source.get("h").number(static_cast<std::int64_t>(model->logicalHeight),
                                         static_cast<std::int64_t>(model->logicalHeight), true);
            model->sourceNodes[id.index()].layerWidth = static_cast<int>(model->logicalWidth);
            model->sourceNodes[id.index()].layerHeight = static_cast<int>(model->logicalHeight);
            usedAsset_ = true;
            const auto in = model->sourceNodes[id.index()].inFrame,
                       out = model->sourceNodes[id.index()].outFrame;
            composition(assets_[0].get("layers"), id, std::max(first, in), std::min(end, out),
                        true);
        }
    }
    void finalize() {
        for (auto& n : model->sourceNodes) {
            n.children.first = static_cast<std::uint32_t>(model->sourceChildIds.size());
            for (const auto& child : model->sourceNodes)
                if (child.parent == n.id)
                    model->sourceChildIds.push_back(child.id);
            n.children.count =
                static_cast<std::uint32_t>(model->sourceChildIds.size()) - n.children.first;
            n.properties.first = static_cast<std::uint32_t>(model->sourcePropertyIds.size());
            n.authoredStatic = true;
            for (const auto& p : model->properties)
                if (p.owner == n.id) {
                    model->sourcePropertyIds.push_back(p.id);
                    if (p.flags & PropertyFlagAnimated)
                        n.authoredStatic = false;
                }
            n.properties.count =
                static_cast<std::uint32_t>(model->sourcePropertyIds.size()) - n.properties.first;
        }
        for (auto it = model->sourceNodes.rbegin(); it != model->sourceNodes.rend(); ++it)
            if (!it->authoredStatic && it->parent.valid())
                model->sourceNodes[it->parent.index()].authoredStatic = false;
        for (auto& n : model->sourceNodes)
            if (!n.authoredStatic)
                n.dependencyBits |= StaticDependencyTimeline;
        auto& s = model->statistics;
        s.directParsedModel = true;
        s.compositionCount = 1;
        s.sourceNodeCount = model->sourceNodes.size();
        s.propertyCount = model->properties.size();
        s.trackCount = model->tracks.size();
        s.animatedPropertyCount = s.trackCount;
        s.staticPropertyCount = s.propertyCount - s.trackCount;
        s.segmentCount = model->segments.size();
        s.scalarValueCount = model->scalarValues.size();
        s.vec2ValueCount = model->vec2Values.size();
        s.colorValueCount = model->colorValues.size();
        s.shapeValueCount = model->shapeValues.size();
        model->revision = 1;
        model->sourceAssetHash = core::fnv1a64(std::as_bytes(std::span(root_.document->source())));
        core::Fnv1a64 hash;
        hash.appendString("AveMotion.OwnVector.Authored.v1");
        hash.appendString(root_.document->source());
        model->parsedModelFingerprint = hash.value();
    }
};
} // namespace
OwnVectorModel::OwnVectorModel(std::string json,
                               std::shared_ptr<const model::MotionAssetModel> authored,
                               std::vector<OwnVectorLayerBinding> layerBindings,
                               std::vector<OwnVectorDrawBinding> drawBindings)
    : exactJson(std::move(json)), model(std::move(authored)), layers(std::move(layerBindings)),
      draws(std::move(drawBindings)) {}
OwnVectorModelResult buildOwnVectorModel(const formats::detail::OwnJsonDocument& document) {
    try {
        Compiler compiler(document);
        compiler.run();
        auto owner = std::shared_ptr<const OwnVectorModel>(
            new OwnVectorModel(std::string(document.source()), compiler.model,
                               std::move(compiler.layers), std::move(compiler.draws)));
        if (!validateOwnVectorModel(*owner))
            return {nullptr, "/", "canonical vector validation failed"};
        return {std::move(owner), {}, {}};
    } catch (const vector_compile::Failure& e) {
        return {nullptr, e.path, e.message};
    }
}
bool validateOwnVectorModel(const OwnVectorModel& owner) {
    if (!owner.model || owner.exactJson.empty() || owner.layers.empty() ||
        owner.layers.size() > 128 || owner.draws.size() > 256)
        return false;
    const auto& m = *owner.model;
    if (m.compositions.size() != 1 || m.properties.size() > 4096 || m.segments.size() > 8192 ||
        m.shapePoints.size() > 65536)
        return false;
    evaluation::PropertyEvaluator evaluator(owner.model);
    if (!evaluator.valid())
        return false;
    for (const auto& b : owner.layers) {
        const auto* n = m.sourceNode(b.layer);
        if (!n || n->kind != model::SourceNodeKind::Layer || n->parent != b.structuralParent ||
            !m.sourceNode(b.structuralParent) || b.outFrame < b.inFrame)
            return false;
    }
    for (const auto& b : owner.draws) {
        const auto* l = m.sourceNode(b.layer);
        const auto* g = m.sourceNode(b.group);
        const auto* p = m.sourceNode(b.path);
        const auto* paint = m.sourceNode(b.paint);
        if (!l || !g || !p || !paint || l->kind != model::SourceNodeKind::Layer ||
            g->kind != model::SourceNodeKind::ShapeGroup || g->parent != l->id ||
            p->kind != model::SourceNodeKind::Shape || p->parent != g->id ||
            paint->parent != g->id ||
            (paint->kind != model::SourceNodeKind::Fill &&
             paint->kind != model::SourceNodeKind::Stroke))
            return false;
        if (b.trim) {
            const auto* t = m.sourceNode(*b.trim);
            if (!t || t->kind != model::SourceNodeKind::Trim || t->parent != l->id ||
                t->trimMode != model::SourceTrimMode::Simultaneous)
                return false;
        }
    }
    return true;
}
} // namespace avemotion::runtime::detail
