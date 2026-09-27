#include "NativeEllipseAdmissionTestData.hpp"
#include "OwnNativeEllipseAdmission.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace avemotion::runtime::detail;
using avemotion::formats::detail::OwnJsonReadCode;
using Code = NativeEllipseAdmissionCode;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

NativeEllipseInputResult readThenAdmit(std::string_view json) {
    const auto parsed = avemotion::formats::detail::readOwnJson(json);
    if (parsed) return decodeOwnNativeEllipseInput(*parsed.document);
    NativeEllipseInputResult result;
    switch (parsed.code) {
    case OwnJsonReadCode::InvalidJson: result.admission.code = Code::InvalidJson; break;
    case OwnJsonReadCode::ResourceLimit: result.admission.code = Code::ResourceLimit; break;
    case OwnJsonReadCode::Parsed: throw std::logic_error("parsed reader without document");
    }
    result.admission.path = parsed.path;
    return result;
}

bool equal(const NativeEllipseInputResult& left, const NativeEllipseInputResult& right) {
    return left.admission.code == right.admission.code
        && left.admission.path == right.admission.path
        && static_cast<bool>(left.input) == static_cast<bool>(right.input)
        && (!left.input || *left.input == *right.input);
}

struct Expected final {
    std::string label, json, oldPath, ownPath;
    Code oldCode, ownCode;
    std::shared_ptr<const NativeEllipseInput> ownInput;
};

void check(const Expected& expected) {
    const auto oldAudit = auditNativeEllipseInput(expected.json);
    const auto old = decodeNativeEllipseInput(expected.json);
    const auto own = readThenAdmit(expected.json);
    require(oldAudit.code == expected.oldCode && oldAudit.path == expected.oldPath,
        expected.label + " old audit");
    require(old.admission.code == expected.oldCode && old.admission.path == expected.oldPath
        && static_cast<bool>(old.input) == (expected.oldCode == Code::Accepted),
        expected.label + " old decode");
    require(own.admission.code == expected.ownCode && own.admission.path == expected.ownPath
        && static_cast<bool>(own.input) == static_cast<bool>(expected.ownInput),
        expected.label + " own composition");
    if (expected.ownInput) require(*own.input == *expected.ownInput,
        expected.label + " literal own descriptor");
}

std::string rootArrays(unsigned count) {
    return std::string(count, '[') + "0" + std::string(count, ']');
}

std::string nestedArrays(unsigned count) {
    return "{\"\":" + std::string(count, '[') + "0" + std::string(count, ']') + "}";
}

std::string manyNulls(unsigned count) {
    std::string result = R"({"": [)";
    for (unsigned i = 0; i < count; ++i) result += (i ? ",null" : "null");
    return result + "]}";
}

void comparatorNoticesChanges(const std::string& seed) {
    const auto observed = readThenAdmit(seed);
    require(observed && observed.input, "baseline observation");
    auto changedPath = observed;
    changedPath.admission.path = "/changed";
    require(!equal(observed, changedPath), "comparator sees path");
    auto changedName = observed;
    auto named = std::make_shared<NativeEllipseInput>(*observed.input);
    named->layerName = "changed";
    changedName.input = named;
    require(!equal(observed, changedName), "comparator sees name");
    auto changedPower = observed;
    auto decimal = std::make_shared<NativeEllipseInput>(*observed.input);
    decimal->frameRate.power.magnitude = "9";
    changedPower.input = decimal;
    require(!equal(observed, changedPower), "comparator sees decimal power");
    auto changedVariant = observed;
    auto variant = std::make_shared<NativeEllipseInput>(*observed.input);
    variant->position = NativeEllipseStaticPosition{};
    changedVariant.input = variant;
    require(!equal(observed, changedVariant), "comparator sees variant");
    auto unpublished = observed;
    unpublished.input.reset();
    require(!equal(observed, unpublished), "comparator sees publication");
}

void run() {
    const auto seed = avemotion::test::ellipseFixture();
    const auto baseline = std::make_shared<const NativeEllipseInput>(
        avemotion::test::expectedEllipseBaseline());
    auto oneRate = std::make_shared<NativeEllipseInput>(*baseline);
    oneRate->frameRate = avemotion::test::ellipseDecimal(false, "1");
    const auto ipTiny = avemotion::test::replaceEllipseOnce(seed, "  \"ip\": 0,\n  \"op\": 61",
        "  \"ip\": 0e999999999999999999999999,\n  \"op\": 61");
    const auto compensated = avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60",
        "\"fr\": 1" + std::string(309, '0') + "e-309");
    std::string rootPath;
    for (int i = 0; i < 32; ++i) rootPath += "/0";
    std::string nestedPath = "/";
    for (int i = 0; i < 31; ++i) nestedPath += "/0";
    std::string oldNestedPath;
    for (int i = 0; i < 31; ++i) oldNestedPath += "/0";
    const std::vector<Expected> rows{
        {"fr zero overflow", avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 0e309"),
            "/", "/fr", Code::InvalidJson, Code::UnsupportedValue, {}},
        {"root ip enormous zero", ipTiny, "/", "", Code::InvalidJson, Code::Accepted, baseline},
        {"compensated huge rate", compensated, "/", "", Code::InvalidJson, Code::Accepted, oneRate},
        {"rounded max overflow", avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 1.7976931348623158e308"), "/", "/fr", Code::InvalidJson, Code::UnsupportedValue, {}},
        {"lexical max high", avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 1.79769313486231580e308"), "/fr", "/fr", Code::UnsupportedValue, Code::UnsupportedValue, {}},
        {"lone low surrogate", avemotion::test::replaceEllipseOnce(seed, "Moving Circle", "\\uDC00"),
            "", "/", Code::Accepted, Code::InvalidJson, {}},
        {"unknown huge number", "{\"x\":1e309}", "/", "/x", Code::InvalidJson, Code::UnsupportedField, {}},
        {"duplicate before compensated", "{\"x\":1,\"x\":2,\"y\":1" + std::string(309, '0') + "e-309}",
            "/", "/x", Code::InvalidJson, Code::InvalidJson, {}},
        {"duplicate empty ancestor", "{\"\":{\"a\":1,\"a\":2}}", "/a", "//a",
            Code::InvalidJson, Code::InvalidJson, {}},
        {"root arrays resource", rootArrays(33), rootPath, rootPath,
            Code::ResourceLimit, Code::ResourceLimit, {}},
        {"empty member arrays resource", nestedArrays(32), oldNestedPath, nestedPath,
            Code::ResourceLimit, Code::ResourceLimit, {}},
        {"empty member nodes resource", manyNulls(4095), "/4094", "//4094",
            Code::ResourceLimit, Code::ResourceLimit, {}},
        {"trailing suffix", seed + "{", "/", "/", Code::InvalidJson, Code::InvalidJson, {}},
    };
    for (const auto& row : rows) check(row);
    comparatorNoticesChanges(seed);
}
}

int main() {
    try {
        run();
        std::cout << "own native ellipse admission differential tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
