#pragma once
#include "OwnVectorClippingTestData.hpp"

namespace avemotion::test {
inline std::string vectorGroup(const std::string &opacity = "", unsigned index = 0,
                               bool stroke = false) {
    auto layer =
        clippingShape(24 + static_cast<int>(index), 24, 72 + static_cast<int>(index), 72, stroke);
    const auto start = layer.find("\"shapes\":[") + 10;
    auto group = layer.substr(start, layer.size() - start - 2);
    if (!opacity.empty())
        group = vectorReplace(group, "{\"ty\":\"tr\"}", "{\"ty\":\"tr\",\"o\":" + opacity + "}");
    const std::string colors[]{"[1,0,0,1]",   "[0,1,0,1]",    "[0,0,1,1]",   "[1,1,0,1]",
                               "[0,1,1,1]",   "[1,0,1,1]",    "[0.5,0,0,1]", "[0,0.5,0,1]",
                               "[0,0,0.5,1]", "[0.5,0.5,0,1]"};
    return vectorReplace(group, "[1,0,0,1]", colors[index % 10]);
}
inline std::string vectorGroupsLayer(const std::string &items, int id = 1,
                                     const std::string &transform = "") {
    return "{\"ty\":4,\"ind\":" + std::to_string(id) + ",\"ip\":0,\"op\":180,\"ks\":{" + transform +
           "},\"shapes\":[" + items + "]}";
}
inline std::string vectorGroups(unsigned count) {
    std::string items;
    for (unsigned i = 0; i < count; ++i) {
        if (i)
            items += ",";
        items += vectorGroup("", i);
    }
    return items;
}
inline std::string vectorGroupHeldOpacity() {
    return R"({"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":68.142578125,"s":[50],"h":1},{"t":110,"s":[100]}]})";
}
inline std::string vectorGroupLinearOpacity() {
    return R"({"a":1,"k":[{"t":0,"s":[100],"i":{"x":1,"y":1},"o":{"x":0,"y":0}},{"t":100,"s":[0]}]})";
}
inline std::string vectorGroupFixture() {
    std::string items;
    for (unsigned i = 0; i < 10; ++i) {
        if (i)
            items += ",";
        items += vectorGroup(i == 9  ? ""
                             : i % 2 ? vectorGroupHeldOpacity()
                                     : vectorGroupLinearOpacity(),
                             i);
    }
    return vectorReplace(vectorRoot(vectorGroupsLayer(items, 1, R"("o":{"k":50})")),
                         "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
}
inline std::string vectorOuterGroupFixture(const std::string &opacity) {
    return vectorReplace(
        vectorRoot(vectorGroupsLayer(
            vectorGroup(opacity) +
                R"(,{"ty":"st","c":{"k":[0,0,1,1]},"o":{"k":100},"w":{"k":4},"lc":1,"lj":1,"ml":4})",
            1, R"("o":{"k":50})")),
        "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
}
} // namespace avemotion::test
