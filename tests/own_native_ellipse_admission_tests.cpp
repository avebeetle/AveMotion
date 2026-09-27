#include "NativeEllipseAdmissionTestData.hpp"
#include "OwnNativeEllipseAdmission.hpp"
#include "avemotion/formats/Tgs.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {
using namespace avemotion::runtime::detail;
using avemotion::formats::detail::OwnJsonReadCode;
using Code = NativeEllipseAdmissionCode;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

NativeEllipseInputResult readThenAdmit(std::string_view json) {
    auto parsed = avemotion::formats::detail::readOwnJson(json);
    if (parsed) return decodeOwnNativeEllipseInput(*parsed.document);
    NativeEllipseInputResult out;
    switch (parsed.code) {
    case OwnJsonReadCode::InvalidJson: out.admission.code = Code::InvalidJson; break;
    case OwnJsonReadCode::ResourceLimit: out.admission.code = Code::ResourceLimit; break;
    case OwnJsonReadCode::Parsed: throw std::logic_error("parsed reader without document");
    }
    out.admission.path = parsed.path;
    return out;
}

NativeEllipseInputResult admitted(std::string_view json, const NativeEllipseInput& expected,
                                 std::string_view label) {
    const auto result = readThenAdmit(json);
    require(static_cast<bool>(result) && result.admission.path.empty() && *result.input == expected,
        label);
    return result;
}

void rejected(std::string_view json, Code code, std::string_view path, std::string_view label) {
    const auto result = readThenAdmit(json);
    if (!(!result && !result.input && result.admission.code == code
          && result.admission.path == path)) {
        throw std::runtime_error(std::string{label} + ": actual code "
            + std::to_string(static_cast<int>(result.admission.code)) + " path " + result.admission.path);
    }
}

std::string nestedArrays(unsigned count) {
    return "{\"\":" + std::string(count, '[') + "0" + std::string(count, ']') + "}";
}

std::string rootArrays(unsigned count) {
    return std::string(count, '[') + "0" + std::string(count, ']');
}

std::string manyNulls(unsigned count) {
    std::string result = R"({"": [)";
    for (unsigned i = 0; i < count; ++i) {
        if (i) result += ',';
        result += "null";
    }
    return result + "]}";
}

void testAcceptedVariants(const std::string& seed) {
    const auto baseline = avemotion::test::expectedEllipseBaseline();
    admitted(seed, baseline, "own document admits ellipse");
    admitted(avemotion::test::replaceEllipseOnce(seed, "\"v\": \"5.7.4\",\n  \"fr\": 60,",
        "\"fr\": 60,\n  \"v\": \"5.7.4\","), baseline, "reordered root");

    auto staticExpected = baseline;
    staticExpected.position = NativeEllipseStaticPosition{{
        avemotion::test::ellipseDecimal(true, "32768"),
        avemotion::test::ellipseDecimal(false, "32768")}};
    admitted(avemotion::test::staticEllipseFixture(seed), staticExpected, "static boundaries");

    auto tiny = baseline;
    tiny.frameRate = avemotion::test::ellipseDecimal(false, "1", true, "9999");
    admitted(avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 1e-9999"), tiny,
        "tiny rate");
    tiny.frameRate = avemotion::test::ellipseDecimal(false, "1", true,
        "999999999999999999999999");
    admitted(avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60",
        "\"fr\": 1e-999999999999999999999999"), tiny, "long tiny rate");
    tiny = baseline;
    tiny.size[0] = avemotion::test::ellipseDecimal(false, "1", true, "9999");
    admitted(avemotion::test::replaceEllipseOnce(seed, "[120, 120]", "[1e-9999, 120]"), tiny,
        "tiny size");
    admitted(avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": 6000e-2"),
        baseline, "equivalent rate");
    admitted(avemotion::test::replaceEllipseOnce(seed, "[-76, 0]", "[-76, -0e+99]"), baseline,
        "signed zero");

    auto variant = avemotion::test::replaceEllipseOnce(seed, "\"ip\": 0,\n      \"op\": 61",
        "\"ip\": 10,\n      \"op\": 20");
    variant = avemotion::test::replaceEllipseOnce(variant, "\"ind\": 1", "\"ind\": 2147483647");
    variant = avemotion::test::replaceEllipseOnce(variant, "[256, 256, 0]", "[-32768, 32768, 0]");
    variant = avemotion::test::replaceEllipseOnce(variant, "[120, 120]", "[16384, 0.5]");
    variant = avemotion::test::replaceEllipseOnce(variant, "[0.08, 0.72, 0.95, 1]", "[0, 1, 0.4, 1]");
    auto varied = baseline;
    varied.layerId = 2147483647; varied.layerInFrame = 10; varied.layerOutFrame = 20;
    varied.layerTranslation = {avemotion::test::ellipseDecimal(true, "32768"),
        avemotion::test::ellipseDecimal(false, "32768")};
    varied.size = {avemotion::test::ellipseDecimal(false, "16384"),
        avemotion::test::ellipseDecimal(false, "5", true, "1")};
    varied.fillColor = {avemotion::test::ellipseDecimal(false, "0"),
        avemotion::test::ellipseDecimal(false, "1"),
        avemotion::test::ellipseDecimal(false, "4", true, "1"),
        avemotion::test::ellipseDecimal(false, "1")};
    admitted(variant, varied, "independent boundary fields");
}

void testNamesAndOwnership(const std::string& seed) {
    const auto baseline = avemotion::test::expectedEllipseBaseline();
    auto absent = avemotion::test::replaceEllipseOnce(seed, "  \"v\": \"5.7.4\",\n", "");
    absent = avemotion::test::replaceEllipseOnce(absent,
        "  \"nm\": \"AveMotion Telegram sticker profile fixture\",\n", "");
    absent = avemotion::test::replaceEllipseOnce(absent, "      \"nm\": \"Moving Circle\",\n", "");
    absent = avemotion::test::replaceEllipseOnce(absent, "          \"nm\": \"Circle Group\",\n", "");
    absent = avemotion::test::replaceEllipseOnce(absent, ",\n              \"nm\": \"Animated Ellipse\"", "");
    absent = avemotion::test::replaceEllipseOnce(absent, ",\n              \"nm\": \"Fill\"", "");
    absent = avemotion::test::replaceEllipseOnce(absent, ",\n              \"nm\": \"Transform\"", "");
    auto noNames = baseline;
    noNames.version.reset(); noNames.name.reset(); noNames.layerName.reset(); noNames.groupName.reset();
    noNames.ellipseName.reset(); noNames.fillName.reset(); noNames.transformName.reset();
    admitted(absent, noNames, "optional names absent");

    auto empty = avemotion::test::replaceEllipseOnce(seed, "\"v\": \"5.7.4\"", "\"v\": \"\"");
    empty = avemotion::test::replaceEllipseOnce(empty, "AveMotion Telegram sticker profile fixture", "");
    empty = avemotion::test::replaceEllipseOnce(empty, "Moving Circle", "");
    empty = avemotion::test::replaceEllipseOnce(empty, "Circle Group", "");
    empty = avemotion::test::replaceEllipseOnce(empty, "Animated Ellipse", "");
    empty = avemotion::test::replaceEllipseOnce(empty, "\"nm\": \"Fill\"", "\"nm\": \"\"");
    empty = avemotion::test::replaceEllipseOnce(empty, "\"nm\": \"Transform\"", "\"nm\": \"\"");
    auto emptyExpected = baseline;
    emptyExpected.version = ""; emptyExpected.name = ""; emptyExpected.layerName = "";
    emptyExpected.groupName = ""; emptyExpected.ellipseName = ""; emptyExpected.fillName = "";
    emptyExpected.transformName = "";
    admitted(empty, emptyExpected, "optional names present empty");
    auto escaped = baseline;
    escaped.layerName = "Moving Circle \xE2\x98\x83";
    admitted(avemotion::test::replaceEllipseOnce(seed, "Moving Circle", "Moving\\u0020Circle \\u2603"),
        escaped, "escaped UTF-8 name");
    auto name256 = baseline;
    name256.layerName = std::string(256, 'a');
    admitted(avemotion::test::replaceEllipseOnce(seed, "Moving Circle", std::string(256, 'a')),
        name256, "256-byte name");
    rejected(avemotion::test::replaceEllipseOnce(seed, "Moving Circle", std::string(257, 'a')),
        Code::UnsupportedValue, "/layers/0/nm", "257-byte name");
    rejected(avemotion::test::replaceEllipseOnce(seed, "Moving Circle", "Moving\\u0000Circle"),
        Code::UnsupportedValue, "/layers/0/nm", "embedded NUL name");

    std::shared_ptr<const NativeEllipseInput> retained;
    {
        auto source = seed;
        auto parsed = avemotion::formats::detail::readOwnJson(source);
        require(static_cast<bool>(parsed), "lifetime reader");
        retained = decodeOwnNativeEllipseInput(*parsed.document).input;
        source.assign(source.size(), 'x');
    }
    require(retained && *retained == baseline, "descriptor survives source and document destruction");
}

void testFailuresAndReaderComposition(const std::string& seed) {
    avemotion::formats::detail::OwnJsonDocument empty;
    const auto direct = decodeOwnNativeEllipseInput(empty);
    require(!direct && !direct.input && direct.admission.code == Code::InvalidJson
        && direct.admission.path == "/", "empty own document");
    rejected("0", Code::InvalidType, "/", "scalar root");
    rejected("{}", Code::UnsupportedStructure, "/fr", "missing root field");
    rejected("{\"x\":1}", Code::UnsupportedField, "/x", "unknown field");
    rejected(avemotion::test::replaceEllipseOnce(seed, "\"fr\": 60", "\"fr\": \"60\""),
        Code::InvalidType, "/fr", "bad scalar grammar");
    rejected(avemotion::test::replaceEllipseOnce(seed, "\"e\": [76, 0]",
        "\"e\": [76.00000000000001, 0]"), Code::UnsupportedValue,
        "/layers/0/shapes/0/it/0/p/k/1/s", "continuity");
    rejected(seed + "{", Code::InvalidJson, "/", "malformed suffix");
    rejected(std::string(1'048'577, ' ') + "{", Code::ResourceLimit, "/", "byte limit before syntax");
    auto padded = seed;
    padded.append(1'048'576 - padded.size(), ' ');
    admitted(padded, avemotion::test::expectedEllipseBaseline(), "exact byte limit");
    rejected(std::string{"{\"x\":\"a\0b\"}", 12}, Code::InvalidJson, "/", "literal NUL");

    std::string depthPath;
    for (int i = 0; i < 32; ++i) depthPath += "/0";
    rejected(rootArrays(33), Code::ResourceLimit, depthPath, "33 arrays");
    std::string emptyDepthPath = "/";
    for (int i = 0; i < 31; ++i) emptyDepthPath += "/0";
    rejected(nestedArrays(32), Code::ResourceLimit, emptyDepthPath, "empty-member 32 arrays");
    rejected(manyNulls(4095), Code::ResourceLimit, "//4094", "empty-member node limit");
    rejected("{\"\":{\"a\":1,\"a\":2}}", Code::InvalidJson, "//a", "duplicate empty ancestor");
    rejected("{\"\":{\"\":1,\"\":2}}", Code::InvalidJson, "//", "duplicate empty key");
    rejected("{\"a/b~\":0}", Code::UnsupportedField, "/a~1b~0", "slash tilde key");
    const std::string nulPath{"/a\0b", 4};
    rejected("{\"a\\u0000b\":0}", Code::UnsupportedField, nulPath, "decoded NUL key");
    rejected(nestedArrays(33) + "{", Code::InvalidJson, "/", "malformed overrides resource");
    rejected("{\"\":{\"a\":1,\"a\":2}}{", Code::InvalidJson, "/", "malformed overrides duplicate");
}

void testTgsAndConcurrency(const std::string& seed) {
    const auto tgs = avemotion::formats::decodeTgsFile(
        std::filesystem::path{AVEMOTION_TGS_DIR} / "telegram_sticker_basic.tgs");
    require(static_cast<bool>(tgs), "TGS transport decode");
    admitted(tgs.json, avemotion::test::expectedEllipseBaseline(), "TGS descriptor");

    std::array<std::exception_ptr, 2> errors;
    std::array<std::thread, 2> workers;
    for (std::size_t index = 0; index < workers.size(); ++index) {
        workers[index] = std::thread([&, index] {
            try {
                auto expected = avemotion::test::expectedEllipseBaseline();
                expected.layerName = index == 0 ? "Thread A" : "Thread B";
                expected.layerTranslation = {avemotion::test::ellipseDecimal(false, index == 0 ? "11" : "33"),
                    avemotion::test::ellipseDecimal(false, index == 0 ? "22" : "44")};
                auto json = avemotion::test::replaceEllipseOnce(seed, "Moving Circle", *expected.layerName);
                json = avemotion::test::replaceEllipseOnce(json, "[256, 256, 0]",
                    index == 0 ? "[11, 22, 0]" : "[33, 44, 0]");
                for (int iteration = 0; iteration < 64; ++iteration)
                    admitted(json, expected, "concurrent independent descriptor");
            } catch (...) { errors[index] = std::current_exception(); }
        });
    }
    for (auto& worker : workers) worker.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    admitted(seed, avemotion::test::expectedEllipseBaseline(), "serial after threads");
}
}

int main() {
    try {
        const auto seed = avemotion::test::ellipseFixture();
        testAcceptedVariants(seed);
        testNamesAndOwnership(seed);
        testFailuresAndReaderComposition(seed);
        testTgsAndConcurrency(seed);
        std::cout << "own native ellipse admission tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
