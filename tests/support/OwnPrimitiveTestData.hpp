#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace avemotion::test {

inline std::string primitiveFixture() {
    std::ifstream input{std::filesystem::path{AVEMOTION_FIXTURE_DIR} /
        "primitive_geometry.json", std::ios::binary};
    if (!input) throw std::runtime_error("cannot open primitive fixture");
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

inline std::string replacePrimitiveOnce(std::string source, std::string_view from,
                                        std::string_view to) {
    const auto position = source.find(from);
    if (position == std::string::npos || source.find(from, position + from.size()) != std::string::npos)
        throw std::logic_error("primitive mutation fragment must be unique");
    source.replace(position, from.size(), to);
    return source;
}

} // namespace avemotion::test
