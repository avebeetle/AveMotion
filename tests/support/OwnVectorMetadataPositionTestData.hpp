#pragma once
#include "OwnVectorClippingTestData.hpp"

namespace avemotion::test {
// Owned rectangles and literal independent axis clocks; no corpus artwork.
inline std::string metadataPositionAxes() {
    return R"({"s":true,"x":{"a":1,"k":[{"t":0,"s":[0],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":40,"s":[20]}]},"y":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":15,"s":[10],"h":1},{"t":60,"s":[-5]}]}})";
}
inline std::string metadataPositionAnnotate(std::string json) {
    json = vectorReplace(json, R"("ty":"gr")", R"("ty":"gr","np":32768,"cix":0,"ix":7,"mn":"owned group")");
    json = vectorReplace(json, R"("ty":"sh")", R"("ty":"sh","ix":32768,"mn":"owned path")");
    json = vectorReplace(json, R"("ty":"fl")", R"("ty":"fl","mn":"owned fill")");
    return vectorReplace(json, R"("fr":60)", R"("markers":[{"tm":25.5,"cm":"owned marker","dr":0.25}],"fr":60)");
}
inline std::string metadataPositionFixture(bool annotated = true, bool occurrences = false) {
    auto shape = vectorReplace(clippingShape(24, 24, 48, 48), R"("ks":{})",
                               R"("parent":2,"ks":{"p":)" + metadataPositionAxes() + "}");
    auto parent = vectorReplace(vectorLayer(R"("p":{"s":true,"x":{"k":3},"y":{"k":4}},"o":{"k":0})"),
                                R"("ind":1)", R"("ind":2)");
    auto json = occurrences
        ? vectorReplace(vectorPrecomp(shape + "," + parent),
                        R"("ip":10,"op":170,"ks":{})",
                        R"("ip":0,"op":180,"st":0,"ks":{}},{"ty":0,"ind":2,"refId":"x","w":128,"h":128,"ip":0,"op":180,"st":10,"ks":{})")
        : vectorRoot(shape + "," + parent);
    json = vectorReplace(json, R"({"fr":)", R"({"v":"5.5.2","fr":)");
    return annotated ? metadataPositionAnnotate(json) : json;
}
inline void metadataPositionReject(const std::string& json, const std::string& label) {
    const auto result = vectorCompile(json);
    vectorRequire(!result && !result.prepared && !result.path.empty() && !result.message.empty(),
                  "atomic metadata/position rejection: " + label);
}
} // namespace avemotion::test
