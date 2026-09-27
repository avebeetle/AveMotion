#pragma once

#include "NativeEllipseAdmissionCore.hpp"
#include "OwnJsonReader.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace avemotion::test {

using namespace runtime::detail;

inline NativeEllipseDecimal ellipseDecimal(bool n, std::string s, bool pn = false,
                                           std::string p = "0") {
    return {n, std::move(s), {pn, std::move(p)}};
}

inline NativeEllipseInput expectedEllipseBaseline() {
    auto d = [](bool n, std::string s, bool pn=false, std::string p="0") {
        return NativeEllipseDecimal{n,std::move(s),{pn,std::move(p)}};
    };
    NativeEllipseInput x;
    x.width=512; x.height=512; x.endFrame=61; x.frameRate=d(false,"6",false,"1");
    x.layerId=1; x.layerInFrame=0; x.layerOutFrame=61;
    x.layerTranslation={d(false,"256"),d(false,"256")};
    x.size={d(false,"12",false,"1"),d(false,"12",false,"1")};
    NativeEllipseAnimatedPosition p;
    p.firstFrame=0; p.lastFrame=60;
    p.start={d(true,"76"),d(false,"0")}; p.end={d(false,"76"),d(false,"0")};
    p.incoming={d(false,"667",true,"3"),d(false,"1")};
    p.outgoing={d(false,"333",true,"3"),d(false,"0")}; x.position=p;
    x.fillColor={d(false,"8",true,"2"),d(false,"72",true,"2"),d(false,"95",true,"2"),d(false,"1")};
    x.version="5.7.4"; x.name="AveMotion Telegram sticker profile fixture";
    x.layerName="Moving Circle"; x.groupName="Circle Group";
    x.ellipseName="Animated Ellipse"; x.fillName="Fill"; x.transformName="Transform";
    return x;
}

inline std::string ellipseFixture() {
    std::ifstream input{std::filesystem::path{AVEMOTION_FIXTURE_DIR} /
        "telegram_sticker_basic.json", std::ios::binary};
    if (!input) throw std::runtime_error("cannot open ellipse fixture");
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

inline std::string replaceEllipseOnce(std::string source, std::string_view from,
                                      std::string_view to) {
    const auto position = source.find(from);
    if (position == std::string::npos || source.find(from, position + from.size()) != std::string::npos)
        throw std::logic_error("ellipse mutation fragment must be unique");
    source.replace(position, from.size(), to);
    return source;
}

inline std::string staticEllipseFixture(std::string_view seed) {
    const std::string_view first = "\"p\": {\n                \"a\": 1,";
    const std::string_view last = "\n              },\n              \"nm\": \"Animated Ellipse\"";
    const auto begin = seed.find(first);
    const auto end = seed.find(last, begin == std::string_view::npos ? 0 : begin + first.size());
    if (begin == std::string_view::npos || end == std::string_view::npos
        || seed.find(first, begin + first.size()) != std::string_view::npos
        || seed.find(last, end + last.size()) != std::string_view::npos)
        throw std::logic_error("ellipse static mutation fragments must be unique");
    std::string result{seed};
    result.replace(begin, end + last.size() - begin,
        "\"p\": {\"a\":0,\"k\":[-32768,32768]},\n              \"nm\": \"Animated Ellipse\"");
    return result;
}

inline std::vector<NativeEllipseValue> projectOwnForCore(
    const formats::detail::OwnJsonDocument& document) {
    using formats::detail::OwnJsonKind;
    const auto nodes = document.nodes();
    std::vector<NativeEllipseValue> result;
    result.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const auto& node = nodes[index];
        NativeEllipseValue value;
        switch (node.kind) {
        case OwnJsonKind::Null: value.kind = NativeEllipseValueKind::Null; break;
        case OwnJsonKind::Boolean: value.kind = NativeEllipseValueKind::Boolean; break;
        case OwnJsonKind::Number: value.kind = NativeEllipseValueKind::Number; break;
        case OwnJsonKind::String: value.kind = NativeEllipseValueKind::String; break;
        case OwnJsonKind::Object: value.kind = NativeEllipseValueKind::Object; break;
        case OwnJsonKind::Array: value.kind = NativeEllipseValueKind::Array; break;
        }
        value.hasKey = node.hasKey;
        if (node.hasKey) value.key = *document.memberName(static_cast<formats::detail::OwnJsonNodeId>(index));
        if (node.kind == OwnJsonKind::Number || node.kind == OwnJsonKind::String)
            value.scalar = *document.valueBytes(static_cast<formats::detail::OwnJsonNodeId>(index));
        value.firstChild = node.firstChild;
        value.nextSibling = node.nextSibling;
        value.childCount = node.childCount;
        result.push_back(value);
    }
    return result;
}

} // namespace avemotion::test
