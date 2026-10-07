#include "OwnPrimitiveTestData.hpp"
#include "OwnNativeEllipseAdmission.hpp"
#include "OwnPrimitiveNumeric.hpp"
#include "OwnJsonReader.hpp"
#include "avemotion/formats/Tgs.hpp"

#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace avemotion::runtime::detail;
namespace model = avemotion::model;
using Code = NativeEllipseAdmissionCode;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

OwnPrimitiveInputResult admitted(std::string_view json) {
    const auto parsed = avemotion::formats::detail::readOwnJson(json);
    require(static_cast<bool>(parsed), "mutation JSON parses");
    return decodeOwnPrimitiveInput(*parsed.document);
}

void rejected(std::string_view json, Code code, std::string_view path) {
    const auto result = admitted(json);
    if (result || result.input || result.admission.code != code || result.admission.path != path)
        throw std::runtime_error("wrong rejection: code "
            + std::to_string(static_cast<int>(result.admission.code)) + " path "
            + result.admission.path + " expected " + std::string{path});
}

void widerReaderAdmissionBoundary() {
    unsigned failures = 0;
    for (const std::size_t nodeCount : {4096U, 4097U}) {
        // One root object, one unknown-member array, and nodeCount - 2 scalars.
        // This authored fixture does not depend on the compiler's private limit.
        std::string json = "{\"boundaryPadding\":[0";
        for (std::size_t index = 1; index < nodeCount - 2; ++index)
            json += ",0";
        json += "]}";
        const auto parsed = avemotion::formats::detail::readOwnJson(json, {65536, 32});
        require(static_cast<bool>(parsed), "wider reader parses boundary fixture");
        require(parsed.document->nodes().size() == nodeCount,
                "boundary fixture has the independently requested node count");
        const auto check = [&](auto decode, std::string_view adapter) {
            try {
                const auto result = decode(*parsed.document);
                const auto code = nodeCount == 4096 ? Code::UnsupportedField : Code::ResourceLimit;
                const auto path = nodeCount == 4096 ? "/boundaryPadding" : "/";
                require(!result && !result.input && result.admission.code == code
                        && result.admission.path == path,
                        "boundary rejection has expected code, path, and no owner");
            } catch (const std::exception& error) {
                ++failures;
                std::cerr << adapter << " with " << nodeCount << " nodes: "
                          << error.what() << '\n';
            }
        };
        check(decodeOwnNativeEllipseInput, "own ellipse adapter");
        check(decodeOwnPrimitiveInput, "own primitive adapter");
    }
    require(failures == 0, "wider-reader boundary cases return typed rejections without exceptions");
}

NativeEllipseDecimal d(bool negative, std::string digits, bool powerNegative = false,
                       std::string power = "0") {
    return {negative, std::move(digits), {powerNegative, std::move(power)}};
}

NativeEllipseVec2 v(int x, int y) {
    const auto integer = [](int value) {
        auto digits = std::to_string(std::abs(value));
        unsigned zeroes = 0;
        while (digits.size() > 1 && digits.back() == '0') {
            digits.pop_back(); ++zeroes;
        }
        return d(value < 0, std::move(digits), false, std::to_string(zeroes));
    };
    return {integer(x), integer(y)};
}

NativeEllipsePosition fixed(int x, int y) {
    return NativeEllipseStaticPosition{v(x, y)};
}

NativeEllipsePosition moving(int sx, int sy, int ex, int ey) {
    NativeEllipseAnimatedPosition result;
    result.firstFrame = 0; result.lastFrame = 60;
    result.start = v(sx, sy); result.end = v(ex, ey);
    result.incoming = {d(false, "667", true, "3"), d(false, "1")};
    result.outgoing = {d(false, "333", true, "3"), d(false, "0")};
    return result;
}

OwnPrimitiveGroupInput group(OwnPrimitiveKind kind, model::SourcePathDirection direction,
                             NativeEllipsePosition position, NativeEllipsePosition size,
                             std::optional<OwnPrimitiveScalar> radius,
                             std::array<NativeEllipseDecimal, 4> color,
                             std::string title) {
    OwnPrimitiveGroupInput result;
    result.kind = kind; result.direction = direction;
    result.position = std::move(position); result.size = std::move(size);
    result.roundness = std::move(radius); result.fillColor = std::move(color);
    result.groupName = title; result.primitiveName = title;
    result.fillName = "Fill"; result.transformName = "Transform";
    return result;
}

OwnPrimitiveInput expected() {
    using K = OwnPrimitiveKind;
    using D = model::SourcePathDirection;
    const auto c = [](std::string r, std::string g, std::string b) {
        const auto channel = [](std::string value) {
            return value == "1" ? d(false, "1") : d(false, value.substr(2), true, "1");
        };
        return std::array{channel(std::move(r)), channel(std::move(g)),
                          channel(std::move(b)), d(false, "1")};
    };
    // The literal colors below use normalized decimal powers, independent of the decoder.
    OwnPrimitiveInput result;
    result.width = 280; result.height = 230; result.endFrame = 61;
    result.frameRate = d(false, "6", false, "1");
    result.layerId = 1; result.layerInFrame = 0; result.layerOutFrame = 61;
    result.layerTranslation = v(0, 0);
    result.version = "5.7.4";
    result.name = "AveMotion generated primitive geometry fixture";
    result.layerName = "Primitive Geometry Layer";
    result.groups = {
        group(K::Rectangle, D::Clockwise, fixed(45,42), fixed(54,34),
              OwnPrimitiveStaticScalar{d(false,"0")}, c("1","0.2","0.2"), "Sharp CW"),
        group(K::Rectangle, D::Clockwise, fixed(125,42), fixed(64,42),
              OwnPrimitiveStaticScalar{d(false,"11")}, c("0.2","1","0.2"), "Rounded CW"),
        group(K::Rectangle, D::CounterClockwise, fixed(215,42), fixed(64,42),
              OwnPrimitiveStaticScalar{d(false,"8",false,"1")}, c("0.2","0.4","1"), "Rounded CCW Clamp"),
        group(K::Ellipse, D::Clockwise, fixed(45,115), fixed(58,38),
              std::nullopt, c("1","0.7","0.1"), "Ellipse CW"),
        group(K::Ellipse, D::CounterClockwise, fixed(125,115), fixed(58,38),
              std::nullopt, c("0.8","0.2","1"), "Ellipse CCW"),
        group(K::Rectangle, D::Clockwise, moving(205,105,225,132), moving(44,28,76,52),
              OwnPrimitiveAnimatedScalar{0,60,d(false,"0"),d(false,"2",false,"1"),
                  {d(false,"667",true,"3"),d(false,"1")},
                  {d(false,"333",true,"3"),d(false,"0")}},
              c("0.1","0.9","0.9"), "Animated Rounded CW"),
        group(K::Ellipse, D::CounterClockwise, moving(75,185,110,190), moving(38,28,72,50),
              std::nullopt, c("0.9","0.1","0.7"), "Animated Ellipse CCW")
    };
    return result;
}

std::string mutateGroup(std::string source, std::size_t ordinal,
                        std::string_view from, std::string_view to) {
    const std::string marker = "        {\n          \"ty\": \"gr\"";
    std::size_t begin = source.find(marker);
    for (std::size_t index = 0; index < ordinal && begin != std::string::npos; ++index)
        begin = source.find(marker, begin + marker.size());
    if (begin == std::string::npos) throw std::logic_error("missing group");
    const auto end = source.find(marker, begin + marker.size());
    const auto limit = end == std::string::npos ? source.find("\n      ],", begin) : end;
    const auto at = source.find(from, begin);
    if (limit == std::string::npos || at == std::string::npos || at >= limit)
        throw std::logic_error("missing group mutation fragment");
    source.replace(at, from.size(), to);
    return source;
}

std::string repeatedGroups(std::string source, std::size_t count) {
    const auto begin = source.find("        {\n          \"ty\": \"gr\"");
    const auto second = source.find("        {\n          \"ty\": \"gr\"", begin + 1);
    const auto end = source.find("\n      ],", second);
    if (begin == std::string::npos || second == std::string::npos || end == std::string::npos)
        throw std::logic_error("fixture group delimiters missing");
    auto first = source.substr(begin, second - begin);
    while (!first.empty() && (first.back() == ' ' || first.back() == '\n' || first.back() == ','))
        first.pop_back();
    std::string replacement;
    for (std::size_t index = 0; index < count; ++index) {
        if (index) replacement += ",\n";
        replacement += first;
    }
    source.replace(begin, end - begin, replacement);
    return source;
}
}

int main() {
    try {
        widerReaderAdmissionBoundary();
        const auto parsed = avemotion::formats::detail::readOwnJson(avemotion::test::primitiveFixture());
        require(static_cast<bool>(parsed), "fixture parses");
        const auto baseline = decodeOwnPrimitiveInput(*parsed.document);
        require(static_cast<bool>(baseline), "unchanged primitive JSON admits an owner");
        const auto want = expected();
        require(baseline.input->width == want.width && baseline.input->height == want.height
                && baseline.input->endFrame == want.endFrame
                && baseline.input->frameRate == want.frameRate
                && baseline.input->layerId == want.layerId
                && baseline.input->layerInFrame == want.layerInFrame
                && baseline.input->layerOutFrame == want.layerOutFrame
                && baseline.input->layerTranslation == want.layerTranslation
                && baseline.input->version == want.version && baseline.input->name == want.name
                && baseline.input->layerName == want.layerName, "exact common fixture fields");
        require(baseline.input->groups.size() == want.groups.size(), "seven ordered groups");
        for (std::size_t index = 0; index < want.groups.size(); ++index)
            if (!(baseline.input->groups[index] == want.groups[index])) {
                const auto& got = baseline.input->groups[index];
                const auto& exp = want.groups[index];
                throw std::runtime_error("exact group fields differ at " + std::to_string(index)
                    + " position=" + std::to_string(got.position == exp.position)
                    + " size=" + std::to_string(got.size == exp.size)
                    + " color=" + std::to_string(got.fillColor == exp.fillColor)
                    + " names=" + std::to_string(got.groupName == exp.groupName)
                    + std::to_string(got.primitiveName == exp.primitiveName));
            }
        const auto tgs = avemotion::formats::decodeTgsFile(
            std::filesystem::path{AVEMOTION_TGS_DIR} / "primitive_geometry.tgs");
        require(static_cast<bool>(tgs), "manifest-pinned TGS transport decodes");
        const auto transported = admitted(tgs.json);
        require(static_cast<bool>(transported) && *transported.input == *baseline.input,
                "JSON and TGS own exact payloads agree");
        const auto numeric = interpretOwnPrimitiveInput(*baseline.input);
        require(numeric.has_value(), "admitted primitive payload converts to numeric values");
        require(numeric->frameRate == 60.0F && numeric->translation.x == 0.0F
                && numeric->translation.y == 0.0F && numeric->groups.size() == 7,
                "numeric common fields and group count");
        require(numeric->groups[0].position.start.x == 45.0F
                && numeric->groups[0].size.start.y == 34.0F
                && !numeric->groups[0].position.animated
                && !numeric->groups[0].size.animated
                && numeric->groups[0].roundness && !numeric->groups[0].roundness->animated
                && numeric->groups[0].roundness->start == 0.0F,
                "static rectangle numeric values");
        require(numeric->groups[2].roundness && numeric->groups[2].roundness->start == 80.0F
                && !numeric->groups[3].roundness,
                "unclamped radius and absent ellipse radius");
        require(numeric->groups[5].position.animated && numeric->groups[5].size.animated
                && numeric->groups[5].roundness && numeric->groups[5].roundness->animated
                && numeric->groups[5].position.firstFrame == 0
                && numeric->groups[5].position.lastFrame == 60
                && numeric->groups[5].position.start.x == 205.0F
                && numeric->groups[5].position.end.y == 132.0F
                && numeric->groups[5].size.start.x == 44.0F
                && numeric->groups[5].size.end.y == 52.0F
                && numeric->groups[5].roundness->start == 0.0F
                && numeric->groups[5].roundness->end == 20.0F
                && numeric->groups[5].roundness->incoming.x == 0.667F
                && numeric->groups[5].roundness->outgoing.x == 0.333F
                && numeric->groups[6].position.animated
                && numeric->groups[6].size.animated
                && numeric->groups[6].position.end.x == 110.0F,
                "five animated segments retain literal endpoints and easing");
        const auto seed = avemotion::test::primitiveFixture();
        const auto one = admitted(repeatedGroups(seed, 1));
        const auto sixteen = admitted(repeatedGroups(seed, 16));
        require(one && one.input->groups.size() == 1, "one group admitted");
        require(sixteen && sixteen.input->groups.size() == 16, "sixteen groups admitted");
        rejected(repeatedGroups(seed, 0), Code::UnsupportedStructure, "/layers/0/shapes");
        rejected(repeatedGroups(seed, 17), Code::UnsupportedStructure, "/layers/0/shapes");
        rejected(mutateGroup(seed, 6, "\"ty\": \"el\"", "\"ty\": \"zz\""),
                 Code::UnsupportedValue, "/layers/0/shapes/6/it/0/ty");
        rejected(mutateGroup(seed, 6, "\"ty\": \"el\"", "\"ty\": \"rc\""),
                 Code::UnsupportedStructure, "/layers/0/shapes/6/it/0/r");
        rejected(mutateGroup(seed, 6, "\"d\": 3", "\"d\": 2"),
                 Code::UnsupportedValue, "/layers/0/shapes/6/it/0/d");
        rejected(mutateGroup(seed, 6, "\"d\": 3", "\"d\": 4"),
                 Code::UnsupportedValue, "/layers/0/shapes/6/it/0/d");
        rejected(mutateGroup(seed, 6, "\"nm\": \"Animated Ellipse CCW\"",
                             "\"bad\": 1, \"nm\": \"Animated Ellipse CCW\""),
                 Code::UnsupportedField, "/layers/0/shapes/6/bad");
        rejected(mutateGroup(seed, 6, "\"it\": [", "\"it\": [null,"),
                 Code::UnsupportedStructure, "/layers/0/shapes/6/it");
        rejected(mutateGroup(seed, 6, "\"ty\": \"el\"", "\"ty\": \"fl\""),
                 Code::UnsupportedValue, "/layers/0/shapes/6/it/0/ty");
        rejected(mutateGroup(seed, 5, "0.667,\n                        0.667",
                             "0.667,\n                        0.668"),
                 Code::UnsupportedValue, "/layers/0/shapes/5/it/0/s/k/0/i/x/1");
        rejected(mutateGroup(seed, 5, "0.667,\n                        0.667",
                             "0.667"),
                 Code::UnsupportedStructure, "/layers/0/shapes/5/it/0/s/k/0/i/x");
        require(static_cast<bool>(admitted(mutateGroup(seed, 5,
            "0.667,\n                        0.667", "0.667,\n                        667e-3"))),
            "equal Vec2 easing axes use exact decimal equality");
        require(static_cast<bool>(admitted(mutateGroup(seed, 5,
            "\"x\": [\n                        0.667,\n                        0.667\n                      ]",
            "\"x\": 0.667"))), "numeric Vec2 easing axis accepted");
        rejected(mutateGroup(seed, 5, "\"t\": 60,\n                    \"s\": [\n                      76,",
                             "\"t\": 60,\n                    \"s\": [\n                      77,"),
                 Code::UnsupportedValue, "/layers/0/shapes/5/it/0/s/k/1/s");
        rejected(mutateGroup(seed, 1, "\"k\": 11", "\"k\": -1"),
                 Code::UnsupportedValue, "/layers/0/shapes/1/it/0/r/k");
        rejected(mutateGroup(seed, 0, "54,\n                  34", "0,\n                  34"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/0/s/k/0");
        rejected(mutateGroup(seed, 0, "54,\n                  34", "16385,\n                  34"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/0/s/k/0");
        rejected(mutateGroup(seed, 0, "45,\n                  42", "32769,\n                  42"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/0/p/k/0");
        rejected(mutateGroup(seed, 5, "44,\n                      28", "0,\n                      28"),
                 Code::UnsupportedValue, "/layers/0/shapes/5/it/0/s/k/0/s/0");
        rejected(mutateGroup(seed, 5, "\"e\": [\n                      20",
                             "\"e\": [\n                      16385"),
                 Code::UnsupportedValue, "/layers/0/shapes/5/it/0/r/k/0/e/0");
        rejected(mutateGroup(seed, 5, "\"s\": [\n                      0\n                    ],",
                             "\"s\": [\n                      -1\n                    ],"),
                 Code::UnsupportedValue, "/layers/0/shapes/5/it/0/r/k/0/s/0");
        rejected(mutateGroup(seed, 0, "1,\n                  0.2", "1.1,\n                  0.2"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/1/c/k/0");
        rejected(mutateGroup(seed, 0, "\"r\": 1,", "\"r\": 2,"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/1/r");
        rejected(mutateGroup(seed, 0, "\"k\": 100\n              },\n              \"r\": 1",
                             "\"k\": 99\n              },\n              \"r\": 1"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/1/o/k");
        rejected(mutateGroup(seed, 0,
                 "\"p\": {\n                \"a\": 0,\n                \"k\": [\n                  0,\n                  0",
                 "\"p\": {\n                \"a\": 0,\n                \"k\": [\n                  1,\n                  0"),
                 Code::UnsupportedValue, "/layers/0/shapes/0/it/2/p/k/0");
        rejected(avemotion::test::replacePrimitiveOnce(seed,
                 "\"p\": {\n          \"a\": 0,\n          \"k\": [\n            0,\n            0,\n            0",
                 "\"p\": {\n          \"a\": 0,\n          \"k\": [\n            0,\n            0,\n            1"),
                 Code::UnsupportedValue, "/layers/0/ks/p/k/2");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"w\": 280", "\"w\": 0"),
                 Code::UnsupportedValue, "/w");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"h\": 230", "\"h\": 8193"),
                 Code::UnsupportedValue, "/h");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"fr\": 60", "\"fr\": 241"),
                 Code::UnsupportedValue, "/fr");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"fr\": 60", "\"fr\": 0"),
                 Code::UnsupportedValue, "/fr");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"ip\": 0,\n  \"op\": 61",
                 "\"ip\": 0,\n  \"op\": 1"),
                 Code::UnsupportedValue, "/op");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"sr\": 1", "\"sr\": 2"),
                 Code::UnsupportedValue, "/layers/0/sr");
        rejected(avemotion::test::replacePrimitiveOnce(seed, "\"sr\": 1,", "\"sr\": 1, \"ao\": 1,"),
                 Code::UnsupportedValue, "/layers/0/ao");
        require(static_cast<bool>(admitted(avemotion::test::replacePrimitiveOnce(seed,
                "\"sr\": 1,", "\"sr\": 1, \"ao\": 0,"))),
                "explicit zero auto orientation accepted");
        const auto absent = admitted(avemotion::test::replacePrimitiveOnce(seed,
            "\"v\": \"5.7.4\",\n  ", ""));
        require(absent && !absent.input->version, "absent optional version preserved");
        const auto emptyName = admitted(mutateGroup(seed, 0,
            "\"nm\": \"Sharp CW\"", "\"nm\": \"\""));
        require(emptyName && emptyName.input->groups[0].groupName == "",
                "empty optional group name preserved");
        const auto absentName = admitted(mutateGroup(seed, 0,
            "\"nm\": \"Sharp CW\",\n          \"it\"", "\"it\""));
        require(absentName && !absentName.input->groups[0].groupName,
                "absent optional group name preserved");
        auto tiny = admitted(avemotion::test::replacePrimitiveOnce(seed, "\"fr\": 60",
            "\"fr\": 1e-9999"));
        require(tiny && !interpretOwnPrimitiveInput(*tiny.input),
                "exact tiny nonzero rate admits then fails numeric conversion");
        auto lexicalZero = admitted(mutateGroup(seed, 0, "\"k\": 0", "\"k\": -0e+99"));
        require(lexicalZero && interpretOwnPrimitiveInput(*lexicalZero.input),
                "lexical zero radius accepts and converts");
        auto invalid = *baseline.input;
        invalid.groups[0].size = NativeEllipseStaticPosition{{d(false,"1",true,"9999"), d(false,"34")}};
        require(!interpretOwnPrimitiveInput(invalid), "tiny positive size underflow fails");
        invalid = *baseline.input;
        invalid.frameRate.digits = "06";
        require(!interpretOwnPrimitiveInput(invalid), "malformed typed decimal fails");
        invalid = *baseline.input;
        invalid.groups[1].roundness = OwnPrimitiveStaticScalar{d(false,"16385")};
        require(!interpretOwnPrimitiveInput(invalid), "typed radius above bound fails");
        invalid = *baseline.input;
        invalid.groups[6].size = NativeEllipseAnimatedPosition{0,59,v(38,28),v(72,50),
            {d(false,"667",true,"3"),d(false,"1")},
            {d(false,"333",true,"3"),d(false,"0")}};
        require(!interpretOwnPrimitiveInput(invalid), "typed segment frame bound fails");
        const auto aboveRate = d(false, "2400000001", true, "7");
        const auto aboveSize = d(false, "1638400001", true, "5");
        const auto abovePosition = d(false, "3276800001", true, "5");
        const auto aboveUnit = d(false, "100000001", true, "8");
        invalid = *baseline.input;
        invalid.frameRate = aboveRate;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact frame rate over 240 rejects before float rounding");
        invalid = *baseline.input;
        invalid.layerTranslation[0] = abovePosition;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact translation over 32768 rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseStaticPosition>(invalid.groups[0].position).value[0] = abovePosition;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact static position over 32768 rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseStaticPosition>(invalid.groups[0].size).value[0] = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact static size over 16384 rejects");
        invalid = *baseline.input;
        std::get<OwnPrimitiveStaticScalar>(*invalid.groups[0].roundness).value = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact static radius over 16384 rejects");
        invalid = *baseline.input;
        invalid.groups[0].fillColor[0] = aboveUnit;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact RGB over one rejects");
        invalid = *baseline.input;
        invalid.groups[0].fillColor[3] = d(false, "99999999", true, "8");
        require(!interpretOwnPrimitiveInput(invalid), "typed exact alpha below one rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseAnimatedPosition>(invalid.groups[5].position).end[1] = abovePosition;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact animated position endpoint over bound rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseAnimatedPosition>(invalid.groups[5].size).start[0] = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact animated size start over bound rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseAnimatedPosition>(invalid.groups[5].size).end[1] = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact animated size end over bound rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseAnimatedPosition>(invalid.groups[5].position).incoming[0] = aboveUnit;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact Vec2 incoming easing over one rejects");
        invalid = *baseline.input;
        std::get<NativeEllipseAnimatedPosition>(invalid.groups[5].position).outgoing[1] = aboveUnit;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact Vec2 outgoing easing over one rejects");
        invalid = *baseline.input;
        std::get<OwnPrimitiveAnimatedScalar>(*invalid.groups[5].roundness).start = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact animated radius start over bound rejects");
        invalid = *baseline.input;
        std::get<OwnPrimitiveAnimatedScalar>(*invalid.groups[5].roundness).end = aboveSize;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact animated radius end over bound rejects");
        invalid = *baseline.input;
        std::get<OwnPrimitiveAnimatedScalar>(*invalid.groups[5].roundness).incoming[0] = aboveUnit;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact scalar incoming easing over one rejects");
        invalid = *baseline.input;
        std::get<OwnPrimitiveAnimatedScalar>(*invalid.groups[5].roundness).outgoing[1] = aboveUnit;
        require(!interpretOwnPrimitiveInput(invalid), "typed exact scalar outgoing easing over one rejects");
        auto atBounds = *baseline.input;
        atBounds.frameRate = d(false, "24", false, "1");
        atBounds.layerTranslation = v(-32768,32768);
        std::get<NativeEllipseStaticPosition>(atBounds.groups[0].position).value = v(-32768,32768);
        std::get<NativeEllipseStaticPosition>(atBounds.groups[0].size).value = v(16384,1);
        std::get<OwnPrimitiveStaticScalar>(*atBounds.groups[0].roundness).value = d(false,"16384");
        atBounds.groups[0].fillColor = {d(false,"0"),d(false,"1"),d(false,"0"),d(false,"1")};
        auto& boundaryPosition = std::get<NativeEllipseAnimatedPosition>(atBounds.groups[5].position);
        boundaryPosition.start = v(-32768,32768);
        boundaryPosition.end = v(32768,-32768);
        boundaryPosition.incoming = v(0,1);
        boundaryPosition.outgoing = v(1,0);
        auto& boundarySize = std::get<NativeEllipseAnimatedPosition>(atBounds.groups[5].size);
        boundarySize.start = v(1,16384);
        boundarySize.end = v(16384,1);
        boundarySize.incoming = v(0,1);
        boundarySize.outgoing = v(1,0);
        auto& boundaryRadius = std::get<OwnPrimitiveAnimatedScalar>(*atBounds.groups[5].roundness);
        boundaryRadius.start = d(false,"0"); boundaryRadius.end = d(false,"16384");
        boundaryRadius.incoming = v(0,1); boundaryRadius.outgoing = v(1,0);
        const auto inclusive = interpretOwnPrimitiveInput(atBounds);
        require(inclusive && inclusive->frameRate == 240.0F
                && inclusive->translation.x == -32768.0F
                && inclusive->groups[0].size.start.x == 16384.0F
                && inclusive->groups[5].roundness->end == 16384.0F,
                "typed exact inclusive limits remain valid");
        const auto failed = admitted(mutateGroup(seed, 6, "\"d\": 3", "\"d\": 2"));
        const auto retained = baseline.input;
        require(!failed && !failed.input && retained && retained->groups.size() == 7,
                "failed admission publishes no owner and prior owner remains valid");
        std::cout << "own primitive admission passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
