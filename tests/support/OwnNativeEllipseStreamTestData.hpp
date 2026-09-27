#pragma once

#include "OwnJsonReader.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnNativeEllipsePreparedAsset.hpp"

#include <memory>
#include <stdexcept>
#include <string>

namespace avemotion::test {

inline std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset>
prepareOwnEllipseForTest(const std::string& source) {
    const auto parsed = formats::detail::readOwnJson(source);
    if (!parsed) throw std::runtime_error("own JSON reader rejected test input");
    const auto authored = runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    if (!authored) throw std::runtime_error("own model rejected test input");
    const auto resources = render::detail::prepareOwnNativeEllipseAsset(authored.prepared);
    if (!resources) throw std::runtime_error("own resources rejected test input");
    return resources.prepared;
}

} // namespace avemotion::test
