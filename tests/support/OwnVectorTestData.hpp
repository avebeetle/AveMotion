#pragma once
#include "OwnVectorModel.hpp"
#include "avemotion/formats/Tgs.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace avemotion::test {
inline void vectorRequire(bool value, const std::string& message) {
    if (!value)
        throw std::runtime_error(message);
}
inline std::string vectorRead(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    vectorRequire(bool(in), "input absent/unreadable: " + path.string());
    return {std::istreambuf_iterator<char>(in), {}};
}
inline std::string vectorInput(int argc, char** argv) {
    if (argc == 1)
        return vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) / "own_vector/parent.json");
    vectorRequire(argc == 3 && std::string(argv[1]) == "--tgs", "usage: --tgs <absolute-file>");
    vectorRequire(std::filesystem::path(argv[2]).is_absolute(), "TGS path must be absolute");
    auto bytes = vectorRead(argv[2]);
    auto decoded = formats::decodeTgs(std::as_bytes(std::span(bytes.data(), bytes.size())));
    vectorRequire(bool(decoded), "production TGS decode failed");
    return decoded.json;
}
inline runtime::detail::OwnVectorModelResult vectorCompile(const std::string& json) {
    auto parsed = formats::detail::readOwnJson(json, {65536, 32});
    vectorRequire(bool(parsed), "explicit vector JSON read failed: " + parsed.path);
    return runtime::detail::buildOwnVectorModel(*parsed.document);
}
inline std::string vectorReplace(std::string json, const std::string& from, const std::string& to) {
    const auto at = json.find(from);
    vectorRequire(at != std::string::npos, "test replacement not found: " + from);
    json.replace(at, from.size(), to);
    return json;
}
inline std::string vectorLayer(const std::string& properties, const std::string& extra = "") {
    return "{\"ty\":3,\"ind\":1,\"ip\":0,\"op\":180,\"ks\":{" + properties + "}" + extra + "}";
}
inline std::string vectorRoot(const std::string& layers) {
    return "{\"fr\":60,\"ip\":0,\"op\":180,\"w\":128,\"h\":128,\"layers\":[" + layers + "]}";
}
inline std::string vectorShape(unsigned count, bool closed, int shift = 0) {
    std::string vertices, handles;
    for (unsigned i = 0; i < count; ++i) {
        if (i) {
            vertices += ",";
            handles += ",";
        }
        vertices += "[" + std::to_string(i * 10 + shift) + ",0]";
        handles += "[0,0]";
    }
    return "{\"i\":[" + handles + "],\"o\":[" + handles + "],\"v\":[" + vertices +
           "],\"c\":" + (closed ? "true" : "false") + "}";
}
inline std::string vectorShapeLayer(const std::string& path) {
    return "{\"ddd\":0,\"ty\":4,\"ind\":1,\"ip\":0,\"op\":180,\"ks\":{},\"shapes\":[{\"ty\":\"gr\","
           "\"it\":[{\"ty\":\"sh\",\"ks\":" +
           path + "},{\"ty\":\"tr\"}]}]}";
}
inline std::string vectorPrecomp(const std::string& layers) {
    return "{\"fr\":60,\"ip\":0,\"op\":180,\"w\":128,\"h\":128,\"assets\":[{\"id\":\"x\","
           "\"layers\":[" +
           layers +
           "]}],\"layers\":[{\"ddd\":0,\"ty\":0,\"ind\":1,\"refId\":\"x\",\"w\":128,\"h\":128,"
           "\"ip\":10,\"op\":170,\"ks\":{}}]}";
}
} // namespace avemotion::test
