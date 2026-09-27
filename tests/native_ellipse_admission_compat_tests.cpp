#include "NativeEllipseAdmissionTestData.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace avemotion::runtime::detail;
using avemotion::test::ellipseFixture;
using avemotion::test::expectedEllipseBaseline;
using avemotion::test::replaceEllipseOnce;
using Code = NativeEllipseAdmissionCode;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

struct Observation {
    std::string label, source, path;
    Code code;
};

void check(const Observation& row) {
    const auto audit = auditNativeEllipseInput(row.source);
    const auto decode = decodeNativeEllipseInput(row.source);
    require(audit.code == row.code && audit.path == row.path, row.label + " audit");
    require(decode.admission.code == row.code && decode.admission.path == row.path,
        row.label + " decode");
    require(static_cast<bool>(decode.input) == (row.code == Code::Accepted),
        row.label + " publication");
}

std::string nestedArrays(unsigned count) {
    return "{\"\":" + std::string(count, '[') + "0" + std::string(count, ']') + "}";
}

std::string manyNulls(unsigned count) {
    std::string result = R"({"": [)";
    for (unsigned i = 0; i < count; ++i) {
        if (i) result += ',';
        result += "null";
    }
    return result + "]}";
}

void run() {
    const auto seed = ellipseFixture();
    const auto deep = nestedArrays(32);
    const auto wide = manyNulls(4095);
    const std::string zeroKeyPath{"/a\0b", 4};
    auto absentNames = replaceEllipseOnce(seed, "  \"v\": \"5.7.4\",\n", "");
    absentNames = replaceEllipseOnce(absentNames,
        "  \"nm\": \"AveMotion Telegram sticker profile fixture\",\n", "");
    absentNames = replaceEllipseOnce(absentNames, "      \"nm\": \"Moving Circle\",\n", "");
    absentNames = replaceEllipseOnce(absentNames, "          \"nm\": \"Circle Group\",\n", "");
    absentNames = replaceEllipseOnce(absentNames, ",\n              \"nm\": \"Animated Ellipse\"", "");
    absentNames = replaceEllipseOnce(absentNames, ",\n              \"nm\": \"Fill\"", "");
    absentNames = replaceEllipseOnce(absentNames, ",\n              \"nm\": \"Transform\"", "");
    auto emptyNames = replaceEllipseOnce(seed, "\"v\": \"5.7.4\"", "\"v\": \"\"");
    emptyNames = replaceEllipseOnce(emptyNames, "AveMotion Telegram sticker profile fixture", "");
    emptyNames = replaceEllipseOnce(emptyNames, "Moving Circle", "");
    emptyNames = replaceEllipseOnce(emptyNames, "Circle Group", "");
    emptyNames = replaceEllipseOnce(emptyNames, "Animated Ellipse", "");
    emptyNames = replaceEllipseOnce(emptyNames, "\"nm\": \"Fill\"", "\"nm\": \"\"");
    emptyNames = replaceEllipseOnce(emptyNames, "\"nm\": \"Transform\"", "\"nm\": \"\"");
    const std::vector<Observation> cases{
        {"missing fr", "{}", "/fr", Code::UnsupportedStructure},
        {"unknown x", "{\"x\":1,\"y\":2}", "/x", Code::UnsupportedField},
        {"unknown y", "{\"y\":2,\"x\":1}", "/y", Code::UnsupportedField},
        {"missing ip", "{\"fr\":0}", "/ip", Code::UnsupportedStructure},
        {"escaped pointer", "{\"a/b~\":0}", "/a~1b~0", Code::UnsupportedField},
        {"decoded nul key", "{\"a\\u0000b\":0}", zeroKeyPath, Code::UnsupportedField},
        {"duplicate nested", "{\"\":{\"a\":1,\"a\":2}}", "/a", Code::InvalidJson},
        {"duplicate empty", "{\"\":{\"\":1,\"\":2}}", "/", Code::InvalidJson},
        {"depth empty ancestor", deep, std::string(31, 'x'), Code::ResourceLimit},
        {"count empty ancestor", wide, "/4094", Code::ResourceLimit},
        {"depth malformed suffix", deep + "{", "/", Code::InvalidJson},
        {"count malformed suffix", wide + "{", "/", Code::InvalidJson},
        {"duplicate malformed suffix", "{\"\":{\"a\":1,\"a\":2}}{", "/", Code::InvalidJson},
        {"overflow zero", replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 0e309"), "/", Code::InvalidJson},
        {"overflow compensated", replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 1" + std::string(309, '0') + "e-309"), "/", Code::InvalidJson},
        {"overflow rounding", replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 1.7976931348623158e308"), "/", Code::InvalidJson},
        {"lexical high", replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 1.79769313486231580e308"), "/fr", Code::UnsupportedValue},
        {"near fr bound", replaceEllipseOnce(seed, "\"fr\": 60",
            "\"fr\": 240.00000000000001"), "/fr", Code::UnsupportedValue},
        {"near continuity", replaceEllipseOnce(seed, "\"e\": [76, 0]",
            "\"e\": [76.00000000000001, 0]"),
            "/layers/0/shapes/0/it/0/p/k/1/s", Code::UnsupportedValue},
        {"baseline", seed, "", Code::Accepted},
        {"reordered root", replaceEllipseOnce(seed, "\"v\": \"5.7.4\",\n  \"fr\": 60,",
            "\"fr\": 60,\n  \"v\": \"5.7.4\","), "", Code::Accepted},
        {"tiny fr", replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 1e-9999"), "", Code::Accepted},
        {"lone surrogate", replaceEllipseOnce(seed, "\"nm\": \"Moving Circle\"",
            "\"nm\": \"\\uDC00\""), "", Code::Accepted},
        {"absent names", absentNames, "", Code::Accepted},
        {"empty names", emptyNames, "", Code::Accepted},
        {"static", avemotion::test::staticEllipseFixture(seed), "", Code::Accepted},
    };
    for (const auto& row : cases) {
        if (row.label == "depth empty ancestor") {
            std::string path;
            for (int i = 0; i < 31; ++i) path += "/0";
            check({row.label, row.source, path, row.code});
        } else check(row);
    }
    const auto baseline = decodeNativeEllipseInput(seed);
    require(baseline.input && *baseline.input == expectedEllipseBaseline(), "baseline full literal");
    const auto reordered = decodeNativeEllipseInput(replaceEllipseOnce(seed,
        "\"v\": \"5.7.4\",\n  \"fr\": 60,", "\"fr\": 60,\n  \"v\": \"5.7.4\","));
    require(reordered.input && *reordered.input == expectedEllipseBaseline(),
        "reordered full literal");
    auto tinyExpected = expectedEllipseBaseline();
    tinyExpected.frameRate = avemotion::test::ellipseDecimal(false, "1", true, "9999");
    const auto tiny = decodeNativeEllipseInput(replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 1e-9999"));
    require(tiny.input && *tiny.input == tinyExpected, "tiny full literal");
    auto staticExpected = expectedEllipseBaseline();
    staticExpected.position = NativeEllipseStaticPosition{{
        avemotion::test::ellipseDecimal(true, "32768"),
        avemotion::test::ellipseDecimal(false, "32768")}};
    const auto fixed = decodeNativeEllipseInput(avemotion::test::staticEllipseFixture(seed));
    require(fixed.input && *fixed.input == staticExpected, "static full literal");
    auto surrogateExpected = expectedEllipseBaseline();
    surrogateExpected.layerName = std::string{"\xED\xB0\x80", 3};
    const auto surrogate = decodeNativeEllipseInput(replaceEllipseOnce(seed,
        "\"nm\": \"Moving Circle\"", "\"nm\": \"\\uDC00\""));
    require(surrogate.input && *surrogate.input == surrogateExpected, "surrogate full literal");
    auto absentExpected = expectedEllipseBaseline();
    absentExpected.version.reset(); absentExpected.name.reset();
    absentExpected.layerName.reset(); absentExpected.groupName.reset();
    absentExpected.ellipseName.reset(); absentExpected.fillName.reset();
    absentExpected.transformName.reset();
    const auto absent = decodeNativeEllipseInput(absentNames);
    require(absent.input && *absent.input == absentExpected, "absent names full literal");
    auto emptyExpected = expectedEllipseBaseline();
    emptyExpected.version = ""; emptyExpected.name = "";
    emptyExpected.layerName = ""; emptyExpected.groupName = "";
    emptyExpected.ellipseName = ""; emptyExpected.fillName = "";
    emptyExpected.transformName = "";
    const auto empty = decodeNativeEllipseInput(emptyNames);
    require(empty.input && *empty.input == emptyExpected, "empty names full literal");
}
}

int main() {
    try {
        run();
        std::cout << "native ellipse admission compat tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
