#include "OwnNativeEllipseAdmission.hpp"

#include "NativeEllipseAdmissionCore.hpp"
#include "OwnJsonReader.hpp"

#include <stdexcept>
#include <vector>

namespace avemotion::runtime::detail {
namespace {

NativeEllipseValueKind projectedKind(formats::detail::OwnJsonKind kind) {
    using formats::detail::OwnJsonKind;
    switch (kind) {
    case OwnJsonKind::Null: return NativeEllipseValueKind::Null;
    case OwnJsonKind::Boolean: return NativeEllipseValueKind::Boolean;
    case OwnJsonKind::Number: return NativeEllipseValueKind::Number;
    case OwnJsonKind::String: return NativeEllipseValueKind::String;
    case OwnJsonKind::Object: return NativeEllipseValueKind::Object;
    case OwnJsonKind::Array: return NativeEllipseValueKind::Array;
    }
    throw std::logic_error("unknown own JSON kind");
}

std::vector<NativeEllipseValue> projectValues(
    const formats::detail::OwnJsonDocument& document) {
    const auto nodes = document.nodes();
    std::vector<NativeEllipseValue> values;
    values.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const auto id = static_cast<formats::detail::OwnJsonNodeId>(index);
        const auto& node = nodes[index];
        NativeEllipseValue value;
        value.kind = projectedKind(node.kind);
        value.hasKey = node.hasKey;
        if (node.hasKey) {
            const auto name = document.memberName(id);
            if (!name) throw std::logic_error("missing own JSON member name");
            value.key = *name;
        }
        if (node.kind == formats::detail::OwnJsonKind::Number
            || node.kind == formats::detail::OwnJsonKind::String) {
            const auto scalar = document.valueBytes(id);
            if (!scalar) throw std::logic_error("missing own JSON scalar bytes");
            value.scalar = *scalar;
        }
        value.firstChild = node.firstChild;
        value.nextSibling = node.nextSibling;
        value.childCount = node.childCount;
        values.push_back(value);
    }
    return values;
}

} // namespace

NativeEllipseInputResult decodeOwnNativeEllipseInput(
    const formats::detail::OwnJsonDocument& document) {
    NativeEllipseInputResult result;
    const auto nodes = document.nodes();
    if (nodes.empty()) {
        result.admission = {NativeEllipseAdmissionCode::InvalidJson, "/"};
        return result;
    }
    if (nodes.size() > NativeEllipseMaxValues) {
        result.admission = {NativeEllipseAdmissionCode::ResourceLimit, "/"};
        return result;
    }

    const auto values = projectValues(document);
    result.admission = evaluateNativeEllipseValues(values, &result.input);
    return result;
}

OwnPrimitiveInputResult decodeOwnPrimitiveInput(
    const formats::detail::OwnJsonDocument& document) {
    OwnPrimitiveInputResult result;
    const auto nodes = document.nodes();
    if (nodes.empty()) {
        result.admission = {NativeEllipseAdmissionCode::InvalidJson, "/"};
        return result;
    }
    if (nodes.size() > NativeEllipseMaxValues) {
        result.admission = {NativeEllipseAdmissionCode::ResourceLimit, "/"};
        return result;
    }
    const auto values = projectValues(document);
    result.admission = evaluateOwnPrimitiveValues(values, &result.input);
    return result;
}

} // namespace avemotion::runtime::detail
