#include "NativeEllipseAdmissionTestData.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace avemotion::runtime::detail;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

NativeEllipseValueId field(const std::vector<NativeEllipseValue>& values,
                           NativeEllipseValueId object, std::string_view name) {
    for (auto id = values[object].firstChild; id != NativeEllipseNoValue;
         id = values[id].nextSibling)
        if (values[id].key == name) return id;
    throw std::logic_error("test field missing");
}

std::vector<NativeEllipseValue> reversedRows(const std::vector<NativeEllipseValue>& source) {
    const auto remap = [&](NativeEllipseValueId id) {
        return id == NativeEllipseNoValue || id == 0 ? id
            : static_cast<NativeEllipseValueId>(source.size() - id);
    };
    std::vector<NativeEllipseValue> result(source.size());
    for (NativeEllipseValueId id = 0; id < source.size(); ++id) {
        auto row = source[id];
        row.firstChild = remap(row.firstChild);
        row.nextSibling = remap(row.nextSibling);
        result[remap(id)] = row;
    }
    return result;
}
}

int main() {
    try {
        const auto json = avemotion::test::ellipseFixture();
        const auto doc = avemotion::formats::detail::readOwnJson(json);
        require(static_cast<bool>(doc), "core fixture JSON read");
        const auto table = avemotion::test::projectOwnForCore(*doc.document);
        std::shared_ptr<const avemotion::runtime::detail::NativeEllipseInput> input;
        const auto admitted = avemotion::runtime::detail::evaluateNativeEllipseValues(table, &input);
        require(admitted.accepted() && input, "core emits accepted descriptor");
        require(admitted.path.empty() && *input == avemotion::test::expectedEllipseBaseline(),
            "complete literal descriptor");
        require(table.size() > 100 && table[0].firstChild == 1 && table[1].key == "v"
            && table[1].nextSibling == 2 && table[2].key == "fr",
            "literal projected root source order");
        const auto layers = field(table, 0, "layers");
        require(table[layers].nextSibling != layers + 1
            && table[table[layers].nextSibling].key == "markers",
            "nested descendants separate immediate root siblings");
        const auto reordered = reversedRows(table);
        std::shared_ptr<const NativeEllipseInput> permutedInput;
        const auto permutedAdmission = evaluateNativeEllipseValues(reordered, &permutedInput);
        require(permutedAdmission.accepted() && permutedAdmission.path.empty()
            && permutedInput && *permutedInput == avemotion::test::expectedEllipseBaseline(),
            "permuted rows retain ordered links and full descriptor");
        const auto audit = evaluateNativeEllipseValues(reordered);
        require(audit.accepted() && audit.path.empty(), "audit-only accepts without descriptor");

        auto swapped = table;
        const auto layer = swapped[layers].firstChild;
        const auto shapes = field(swapped, layer, "shapes");
        const auto group = swapped[shapes].firstChild;
        const auto items = field(swapped, group, "it");
        const auto ellipse = swapped[items].firstChild;
        const auto fill = swapped[ellipse].nextSibling;
        const auto transform = swapped[fill].nextSibling;
        require(fill != NativeEllipseNoValue && transform != NativeEllipseNoValue,
            "shape siblings exist");
        swapped[items].firstChild = fill;
        swapped[fill].nextSibling = ellipse;
        swapped[ellipse].nextSibling = transform;
        auto prior = input;
        auto rejectedOutput = std::make_shared<const NativeEllipseInput>(
            avemotion::test::expectedEllipseBaseline());
        const auto rejection = evaluateNativeEllipseValues(swapped, &rejectedOutput);
        require(rejection.code == NativeEllipseAdmissionCode::UnsupportedField
            && rejection.path == "/layers/0/shapes/0/it/0/c" && !rejectedOutput,
            "swapped shape sibling rejects first unknown field and clears output");
        require(prior && *prior == avemotion::test::expectedEllipseBaseline(),
            "earlier owned descriptor stays unchanged");

        auto invalid = table;
        invalid[0].firstChild = static_cast<NativeEllipseValueId>(invalid.size());
        bool threw = false;
        try { static_cast<void>(evaluateNativeEllipseValues(invalid)); }
        catch (const std::logic_error&) { threw = true; }
        require(threw, "invalid internal links throw");
        threw = false;
        try { static_cast<void>(evaluateNativeEllipseValues({})); }
        catch (const std::logic_error&) { threw = true; }
        require(threw, "empty internal table throws");
        invalid = table;
        invalid[2].scalar = "01";
        threw = false;
        try { static_cast<void>(evaluateNativeEllipseValues(invalid)); }
        catch (const std::logic_error&) { threw = true; }
        require(threw, "invalid internal number token throws");
        std::cout << "native ellipse admission core tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
