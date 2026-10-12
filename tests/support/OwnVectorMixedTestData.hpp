#pragma once
#include "OwnVectorTestData.hpp"
#include <utility>
#include <vector>

namespace avemotion::test {
// Authored input variants only. Expected properties, scenes and pixels continue
// to come from the existing independent ordinary Telegram oracle.
inline std::vector<std::pair<std::string, std::string>> vectorMixedFixtures() {
    // The pinned ordinary parser defaults an omitted ddd to true. Explicit 2D
    // avoids adding its neutral 3D properties to this 2D property comparison.
    const auto base = vectorReplace(vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) /
                                 "own/mixed_geometry.json"), R"({"ty":4)", R"({"ddd":0,"ty":4)");
    auto reverse = vectorReplace(base, R"("ty":"el","d":1)", R"("ty":"el","d":3)");
    reverse = vectorReplace(reverse, R"("ty":"rc","d":3)", R"("ty":"rc","d":1)");
    auto animated = vectorReplace(base, R"("p":{"k":[24.5,28.25]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[24.5,28.25],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":30,"s":[30.5,34.25],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[24.5,28.25]}]})");
    animated = vectorReplace(animated, R"("s":{"k":[20.5,12.5]})",
        R"("s":{"a":1,"k":[{"t":0,"s":[20.5,12.5],"o":{"x":0.25,"y":0.1},"i":{"x":0.75,"y":0.9}},{"t":60,"s":[28.5,18.5]}]})");
    animated = vectorReplace(animated, R"("p":{"k":[64,32]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[64,32],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[68,36]}]})");
    animated = vectorReplace(animated, R"("s":{"k":[32,20]})",
        R"("s":{"a":1,"k":[{"t":0,"s":[32,20],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":30,"s":[24.5,16.5],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[32,20]}]})");
    animated = vectorReplace(animated, R"("r":{"k":4})",
        R"("r":{"a":1,"k":[{"t":0,"s":[0],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":30,"s":[20],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[4]}]})");
    auto hold = vectorReplace(base, R"("p":{"k":[24.5,28.25]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[24.5,28.25],"h":1},{"t":15,"s":[30.5,34.25],"h":1},{"t":30,"s":[24.5,28.25]}]})");
    hold = vectorReplace(hold, R"("s":{"k":[32,20]})",
        R"("s":{"a":1,"k":[{"t":0,"s":[32,20],"h":1},{"t":15,"s":[24.5,16.5],"h":1},{"t":30,"s":[32,20]}]})");
    hold = vectorReplace(hold, R"("r":{"k":4})",
        R"("r":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":15,"s":[16384],"h":1},{"t":30,"s":[4]}]})");
    auto fractional = vectorReplace(base, R"("s":{"k":[20.5,12.5]})",
                                     R"("s":{"k":[0.5,0.25]})");
    fractional = vectorReplace(fractional, R"("s":{"k":[32,20]})",
                               R"("s":{"k":[18.75,10.25]})");
    fractional = vectorReplace(fractional, R"("r":{"k":4})", R"("r":{"k":2.125})");
    return {{"static", base}, {"reverse", reverse},
            {"sharp", vectorReplace(base, R"("r":{"k":4})", R"("r":{"k":0})")},
            {"saturated", vectorReplace(base, R"("r":{"k":4})", R"("r":{"k":16384})")},
            {"animated", animated}, {"hold", hold}, {"fractional", fractional}};
}

inline const std::vector<unsigned> &vectorMixedFrames() {
    static const std::vector<unsigned> frames{0,1,14,15,29,30,31,45,59,60,30,0,60};
    return frames;
}
} // namespace avemotion::test
