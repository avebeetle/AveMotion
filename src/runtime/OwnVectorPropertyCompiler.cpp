#include "OwnVectorPropertyCompiler.hpp"
#include "NativeEllipseAdmissionCore.hpp"
#include <algorithm>
#include <cmath>

namespace avemotion::runtime::detail::vector_compile {
using namespace model;
using namespace formats::detail;
bool Value::exists() const {
    return document->node(id) != nullptr;
}
void Value::require(OwnJsonKind kind) const {
    if (!exists() || document->node(id)->kind != kind)
        fail("missing value or wrong JSON type");
}
void Value::fail(std::string message) const {
    throw Failure{path.empty() ? "/" : path, std::move(message)};
}
Value Value::get(std::string_view key) const {
    require(OwnJsonKind::Object);
    auto child = document->node(id)->firstChild;
    while (child != OwnJsonNoNode) {
        if (document->memberName(child) == key)
            return {document, child, path + "/" + std::string(key)};
        child = document->node(child)->nextSibling;
    }
    return {document, OwnJsonNoNode, path + "/" + std::string(key)};
}
std::vector<Value> Value::array(std::size_t minimum, std::size_t maximum) const {
    require(OwnJsonKind::Array);
    const auto count = document->node(id)->childCount;
    if (count < minimum || count > maximum)
        fail("array count outside admitted bounds");
    std::vector<Value> result;
    result.reserve(count);
    for (auto child = document->node(id)->firstChild; child != OwnJsonNoNode;
         child = document->node(child)->nextSibling)
        result.push_back({document, child, path + "/" + std::to_string(result.size())});
    return result;
}
void Value::keys(std::initializer_list<std::string_view> allowed) const {
    require(OwnJsonKind::Object);
    for (auto child = document->node(id)->firstChild; child != OwnJsonNoNode;
         child = document->node(child)->nextSibling) {
        const auto key = *document->memberName(child);
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            std::string escaped;
            for (char c : key)
                escaped += c == '~' ? "~0" : c == '/' ? "~1" : std::string(1, c);
            Value{document, child, path + "/" + escaped}.fail("unsupported field in this context");
        }
    }
}
float Value::number(std::int64_t low, std::int64_t high, bool integral, bool strictLow) const {
    require(OwnJsonKind::Number);
    float output = 0;
    if (!convertOwnNumericToken(*document->valueBytes(id), low, high, integral, strictLow, output))
        fail("number outside exact admitted domain or finite float representation");
    return output;
}
std::string Value::string() const {
    require(OwnJsonKind::String);
    return std::string(*document->valueBytes(id));
}
bool Value::boolean() const {
    require(OwnJsonKind::Boolean);
    return document->node(id)->boolean;
}
void Value::metadata() const {
    if (get("nm").exists())
        (void)get("nm").string();
    for (auto key : {"bm", "ddd", "ao"})
        if (get(key).exists())
            (void)get(key).number(0, 0, true);
    if (get("hd").exists() && get("hd").boolean())
        get("hd").fail("hidden content is unsupported");
}
namespace {
MotionVec2Value point(Value input, std::int64_t low, std::int64_t high, float z = 0) {
    auto a = input.array(2, 3);
    if (a.size() == 3 &&
        a[2].number(static_cast<std::int64_t>(z), static_cast<std::int64_t>(z)) != z)
        a[2].fail("non-neutral Z component");
    return {a[0].number(low, high), a[1].number(low, high)};
}
float channel(Value input) {
    if (input.exists() && input.document->node(input.id)->kind == OwnJsonKind::Number)
        return input.number(0, 1);
    auto values = input.array(1, 3);
    values.front().require(OwnJsonKind::Number);
    const auto firstToken = *values.front().document->valueBytes(values.front().id);
    for (const auto& v : values) {
        v.require(OwnJsonKind::Number);
        if (!equalOwnNumericTokens(firstToken, *v.document->valueBytes(v.id)))
            v.fail("unequal channel easing is unsupported");
    }
    return values.front().number(0, 1);
}
MotionVec2Value ease(Value input) {
    input.keys({"x", "y"});
    return {channel(input.get("x")), channel(input.get("y"))};
}
} // namespace
MotionValueRef PropertyCompiler::shape(Value input) {
    input.keys({"i", "o", "v", "c"});
    const auto vertices = input.get("v").array(2, 256);
    const auto incoming = input.get("i").array(vertices.size(), vertices.size());
    const auto outgoing = input.get("o").array(vertices.size(), vertices.size());
    const bool closed = input.get("c").boolean();
    const auto count = 1 + 3 * (vertices.size() - 1 + (closed ? 1 : 0));
    if (count > 65536 - model_.shapePoints.size())
        input.fail("canonical shape point limit exceeded");
    const auto first = static_cast<std::uint32_t>(model_.shapePoints.size());
    std::vector<MotionVec2Value> v, i, o;
    for (std::size_t n = 0; n < vertices.size(); ++n) {
        // Path points are strictly 2D; transforms alone allow a neutral Z.
        (void)vertices[n].array(2, 2);
        (void)incoming[n].array(2, 2);
        (void)outgoing[n].array(2, 2);
        v.push_back(point(vertices[n], -32768, 32768));
        i.push_back(point(incoming[n], -32768, 32768));
        o.push_back(point(outgoing[n], -32768, 32768));
    }
    model_.shapePoints.push_back(v[0]);
    for (std::size_t n = 1; n < vertices.size() + (closed ? 1 : 0); ++n) {
        const auto previous = n - 1, next = n % vertices.size();
        model_.shapePoints.push_back(
            {v[previous].x + o[previous].x, v[previous].y + o[previous].y});
        model_.shapePoints.push_back({v[next].x + i[next].x, v[next].y + i[next].y});
        model_.shapePoints.push_back(v[next]);
    }
    const auto index = static_cast<std::uint32_t>(model_.shapeValues.size());
    model_.shapeValues.push_back({{first, static_cast<std::uint32_t>(count)}, closed});
    return {PropertyValueType::Shape, index};
}
MotionValueRef PropertyCompiler::value(Value input, PropertyValueType type, std::int64_t low,
                                       std::int64_t high, float z, bool strictLow, bool keyValue) {
    if (keyValue && (type == PropertyValueType::Scalar || type == PropertyValueType::Shape))
        input = input.array(1, 1)[0];
    switch (type) {
    case PropertyValueType::Scalar: {
        auto index = static_cast<std::uint32_t>(model_.scalarValues.size());
        model_.scalarValues.push_back(input.number(low, high, false, strictLow));
        return {type, index};
    }
    case PropertyValueType::Vec2: {
        auto index = static_cast<std::uint32_t>(model_.vec2Values.size());
        model_.vec2Values.push_back(point(input, low, high, z));
        return {type, index};
    }
    case PropertyValueType::Color: {
        auto a = input.array(4, 4);
        auto index = static_cast<std::uint32_t>(model_.colorValues.size());
        model_.colorValues.push_back(
            {a[0].number(0, 1), a[1].number(0, 1), a[2].number(0, 1), a[3].number(1, 1)});
        return {type, index};
    }
    case PropertyValueType::Shape:
        return shape(input);
    default:
        input.fail("unsupported property type");
    }
}
PropertyId PropertyCompiler::append(SourceNodeId owner, PropertySemantic semantic,
                                    MotionValueRef initial) {
    if (model_.properties.size() >= 4096)
        throw Failure{"/", "property limit exceeded"};
    MotionPropertyRecord p;
    p.present = true;
    p.id = makeId<PropertyId>(model_.properties.size());
    p.owner = owner;
    p.semantic = semantic;
    p.valueType = initial.type;
    p.flags = PropertyFlagStatic;
    p.staticValue = initial;
    model_.properties.push_back(p);
    return p.id;
}
PropertyId PropertyCompiler::scalar(SourceNodeId owner, PropertySemantic semantic, float value) {
    auto index = static_cast<std::uint32_t>(model_.scalarValues.size());
    model_.scalarValues.push_back(value);
    return append(owner, semantic, {PropertyValueType::Scalar, index});
}
PropertyId PropertyCompiler::vec2(SourceNodeId owner, PropertySemantic semantic,
                                  MotionVec2Value value) {
    auto index = static_cast<std::uint32_t>(model_.vec2Values.size());
    model_.vec2Values.push_back(value);
    return append(owner, semantic, {PropertyValueType::Vec2, index});
}
PropertyId PropertyCompiler::add(SourceNodeId owner, PropertySemantic semantic, Value property,
                                 PropertyValueType type, std::int64_t low, std::int64_t high,
                                 bool animated, float z, bool strictLow) {
    property.keys({"a", "k", "ix"});
    if (property.get("ix").exists())
        (void)property.get("ix").number(0, 32768, true);
    const auto a = property.get("a");
    const bool keyed = a.exists() && a.number(0, 1, true) == 1;
    if (!keyed)
        return append(owner, semantic,
                      value(property.get("k"), type, low, high, z, strictLow, false));
    if (!animated || type == PropertyValueType::Color)
        property.fail("this property must be static");
    const auto keys = property.get("k").array(2, 256);
    struct Key {
        float time;
        MotionValueRef value;
        bool hold;
        MotionVec2Value in{}, out{}, ti{}, to{};
        bool spatial = false;
    };
    std::vector<Key> parsed;
    parsed.reserve(keys.size());
    for (std::size_t n = 0; n < keys.size(); ++n) {
        const auto& k = keys[n];
        k.keys({"t", "s", "h", "i", "o", "ti", "to"});
        Key entry{k.get("t").number(0, 20000),
                  value(k.get("s"), type, low, high, z, strictLow, true), false};
        if (!parsed.empty() && entry.time <= parsed.back().time)
            k.get("t").fail("key times must strictly increase");
        if (k.get("h").exists())
            entry.hold = k.get("h").number(1, 1, true) == 1;
        const bool terminal = n + 1 == keys.size();
        if (terminal || entry.hold) {
            for (auto field : {"i", "o", "ti", "to"})
                if (k.get(field).exists())
                    k.get(field).fail("held/terminal key cannot carry interpolation controls");
        } else {
            entry.in = ease(k.get("i"));
            entry.out = ease(k.get("o"));
            entry.spatial = k.get("ti").exists() || k.get("to").exists();
            if (entry.spatial) {
                if (semantic != PropertySemantic::TransformPosition)
                    k.fail("spatial tangents require position");
                entry.ti = point(k.get("ti"), -32768, 32768);
                entry.to = point(k.get("to"), -32768, 32768);
            }
        }
        if (type == PropertyValueType::Shape && !parsed.empty()) {
            const auto& first = model_.shapeValues[parsed[0].value.index];
            const auto& current = model_.shapeValues[entry.value.index];
            if (first.closed != current.closed || first.points.count != current.points.count)
                k.get("s").fail("morph topology and closure must remain fixed");
        }
        parsed.push_back(entry);
    }
    const auto segmentCount = parsed.size() - 1 + (parsed.back().hold ? 1 : 0);
    if (segmentCount > 8192 - model_.segments.size())
        property.fail("segment limit exceeded");
    const auto id = append(owner, semantic, parsed[0].value);
    auto& p = model_.properties[id.index()];
    p.flags = PropertyFlagAnimated;
    p.staticValue = {};
    p.track = makeId<TrackId>(model_.tracks.size());
    MotionTrackRecord track{true,
                            p.track,
                            id,
                            {static_cast<std::uint32_t>(model_.segments.size()),
                             static_cast<std::uint32_t>(segmentCount)},
                            parsed.front().time,
                            parsed.back().time};
    for (std::size_t n = 0; n < segmentCount; ++n) {
        const auto& key = parsed[n];
        const auto& next = parsed[std::min(n + 1, parsed.size() - 1)];
        MotionSegmentRecord s;
        s.present = true;
        s.id = makeId<SegmentId>(model_.segments.size());
        s.track = p.track;
        s.firstFrame = key.time;
        s.endFrame = next.time;
        s.startValue = key.value;
        // Hold interpolation selects start inside the interval; its endpoint is
        // still the next authored value. This also retains an ordinary terminal
        // after a hold. A final zero-length hold has identical start/end values.
        s.endValue = next.value;
        if (!key.hold) {
            s.temporalControl1 = key.out;
            s.temporalControl2 = key.in;
            s.interpolation = key.out == MotionVec2Value{0, 0} && key.in == MotionVec2Value{1, 1}
                                  ? SegmentInterpolation::Linear
                                  : SegmentInterpolation::CubicBezier;
        }
        if (key.spatial) {
            s.spatialInterpolation = SpatialInterpolation::CubicBezier;
            s.spatialInTangent = key.ti;
            s.spatialOutTangent = key.to;
            p.flags |= PropertyFlagSpatial;
        }
        model_.segments.push_back(s);
    }
    model_.tracks.push_back(track);
    return id;
}
} // namespace avemotion::runtime::detail::vector_compile
