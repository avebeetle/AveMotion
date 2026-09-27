#pragma once
#include "OwnVectorTestData.hpp"

namespace avemotion::test {
inline std::string clippingShape(int left, int top, int right, int bottom, bool stroke = false,
                                 int cap = 1, int join = 1) {
    const auto path = "{\"i\":[[0,0],[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0],[0,0]],"
                      "\"v\":[[" +
                      std::to_string(left) + "," + std::to_string(top) + "],[" +
                      std::to_string(right) + "," + std::to_string(top) + "],[" +
                      std::to_string(right) + "," + std::to_string(bottom) + "],[" +
                      std::to_string(left) + "," + std::to_string(bottom) + "]],\"c\":true}";
    auto layer = vectorShapeLayer("{\"k\":" + path + "}");
    const auto paint =
        stroke ? "{\"ty\":\"st\",\"c\":{\"k\":[1,0,0,1]},\"o\":{\"k\":100},\"w\":{\"k\":12},"
                 "\"lc\":" +
                     std::to_string(cap) + ",\"lj\":" + std::to_string(join) + ",\"ml\":4},"
               : "{\"ty\":\"fl\",\"c\":{\"k\":[1,0,0,1]},\"o\":{\"k\":100}},";
    return vectorReplace(layer, "{\"ty\":\"tr\"}", paint + "{\"ty\":\"tr\"}");
}
inline std::string clippingPrecomp(const std::string &child, const std::string &root = "") {
    auto json = vectorPrecomp(child);
    json = vectorReplace(json, "\"ip\":10,\"op\":170", "\"ip\":0,\"op\":180");
    json = vectorReplace(json, "{\"fr\":", "{\"v\":\"5.5.2\",\"fr\":");
    if (!root.empty())
        json = vectorReplace(json, "\"layers\":[{\"ddd\":0,\"ty\":0",
                             "\"layers\":[" + root + ",{\"ddd\":0,\"ty\":0");
    return json;
}
} // namespace avemotion::test
