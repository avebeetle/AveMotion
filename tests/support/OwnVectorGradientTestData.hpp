#pragma once
#include "OwnVectorGroupTestData.hpp"

namespace avemotion::test {
inline std::string gradientFill() {
    return R"({"ty":"gf","t":1,"r":1,"o":{"a":0,"k":100},"s":{"a":0,"k":[2,3]},"e":{"a":0,"k":[18,3]},"g":{"p":3,"k":{"a":0,"k":[0,1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]}}})";
}
inline std::string gradientGroup(const std::string &fill = gradientFill(),
                                 const std::string &transform = "") {
    // Owned rectangle, independent of corpus artwork.
    return R"({"ty":"gr","it":[{"ty":"sh","ks":{"a":0,"k":{"i":[[0,0],[0,0],[0,0],[0,0]],"o":[[0,0],[0,0],[0,0],[0,0]],"v":[[0,0],[20,0],[20,10],[0,10]],"c":true}}},)" +
           fill + R"(,{"ty":"tr")" + transform + "}]}";
}
inline std::string gradientFixture(const std::string &fill = gradientFill(),
                                   const std::string &groupTransform = "",
                                   const std::string &layerTransform = "") {
    return vectorReplace(vectorRoot(vectorReplace(vectorGroupsLayer(gradientGroup(fill, groupTransform), 1,
                                                      layerTransform), "{\"ty\":4", "{\"ddd\":0,\"ty\":4")),
                         "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
}
inline std::string gradientParentFixture() {
    const auto parent = vectorReplace(vectorLayer(
        R"("p":{"a":0,"k":[5,7]},"s":{"a":0,"k":[200,50]},"o":{"a":0,"k":0})"),
        "{\"ty\":3", "{\"ddd\":0,\"ty\":3");
    auto child = vectorGroupsLayer(gradientGroup(gradientFill(),
        R"(,"p":{"a":0,"k":[2,4]},"s":{"a":0,"k":[150,150]})"), 2,
        R"("o":{"a":0,"k":100})");
    child = vectorReplace(vectorReplace(child, "\"ind\":2", "\"ind\":2,\"parent\":1"),
                          "{\"ty\":4", "{\"ddd\":0,\"ty\":4");
    return vectorReplace(vectorRoot(parent + "," + child), "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
}
inline std::string gradientPacked(unsigned stops) {
    std::string packed;
    for (unsigned i = 0; i < stops; ++i) {
        if (i) packed += ",";
        packed += std::to_string(static_cast<double>(i) / (stops - 1)) + ",1,0,0";
    }
    return vectorReplace(vectorReplace(gradientFill(), "\"p\":3", "\"p\":" + std::to_string(stops)),
        "[0,1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]", "[" + packed + "]");
}
inline std::string gradientAnimatedFixture() {
    auto fixture = vectorReplace(gradientParentFixture(),
        R"("p":{"a":0,"k":[5,7]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[5,7],"h":1},{"t":10,"s":[9,11],"h":1},{"t":90,"s":[5,7]}]})");
    const std::string a = R"({"i":[[0,0],[0,0],[0,0],[0,0]],"o":[[0,0],[0,0],[0,0],[0,0]],"v":[[0,0],[20,0],[20,10],[0,10]],"c":true})";
    const std::string b = vectorReplace(a, "[[0,0],[20,0],[20,10],[0,10]]", "[[2,0],[22,0],[22,10],[2,10]]");
    return vectorReplace(fixture, "\"ks\":{\"a\":0,\"k\":" + a + "}",
        "\"ks\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[" + a + "],\"h\":1},{\"t\":10,\"s\":[" + b +
        "],\"h\":1},{\"t\":90,\"s\":[" + a + "]}]}" );
}
inline std::string gradientOccurrencesFixture() {
    const auto leaf = vectorReplace(vectorGroupsLayer(gradientGroup(),1,
        R"("p":{"a":1,"k":[{"t":0,"s":[20,20],"h":1},{"t":10,"s":[30,20]}]})"),
        "{\"ty\":4", "{\"ddd\":0,\"ty\":4");
    const auto occurrences = vectorRoot(
        R"({"ddd":0,"ty":0,"ind":1,"refId":"gradient","w":128,"h":128,"ip":0,"op":180,"st":0,"ks":{}},{"ddd":0,"ty":0,"ind":2,"refId":"gradient","w":128,"h":128,"ip":10,"op":180,"st":10,"ks":{}})");
    return vectorReplace(vectorReplace(occurrences, "\"layers\":",
        "\"assets\":[{\"id\":\"gradient\",\"layers\":[" + leaf + "]}],\"layers\":"),
        "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
}
} // namespace avemotion::test
