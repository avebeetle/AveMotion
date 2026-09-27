#include "NativeEllipseAdmissionCore.hpp"
#include "NativeEllipseNumericHelpers.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace avemotion::runtime::detail {
namespace {
using Code = NativeEllipseAdmissionCode;
using Value = NativeEllipseValue;
using Range = std::pair<std::int64_t, std::int64_t>;

bool validNumberToken(std::string_view token) {
    std::size_t position = 0;
    if (!token.empty() && token[position] == '-') ++position;
    if (position == token.size()) return false;
    if (token[position] == '0') {
        ++position;
    } else {
        if (token[position] < '1' || token[position] > '9') return false;
        do { ++position; }
        while (position < token.size() && token[position] >= '0' && token[position] <= '9');
    }
    if (position < token.size() && token[position] == '.') {
        ++position;
        const auto first = position;
        while (position < token.size() && token[position] >= '0' && token[position] <= '9')
            ++position;
        if (position == first) return false;
    }
    if (position < token.size() && (token[position] == 'e' || token[position] == 'E')) {
        ++position;
        if (position < token.size() && (token[position] == '+' || token[position] == '-'))
            ++position;
        const auto first = position;
        while (position < token.size() && token[position] >= '0' && token[position] <= '9')
            ++position;
        if (position == first) return false;
    }
    return position == token.size();
}

struct SignedPower {
    bool negative = false;
    std::string magnitude = "0";
};

std::string normalizedMagnitude(std::string_view digits) {
    const auto first = digits.find_first_not_of('0');
    return first == std::string_view::npos ? "0" : std::string{digits.substr(first)};
}

int compareMagnitude(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return left.size() < right.size() ? -1 : 1;
    return left < right ? -1 : (left > right ? 1 : 0);
}

std::string addMagnitude(std::string_view left, std::string_view right) {
    std::string reversed;
    reversed.reserve((std::max)(left.size(), right.size()) + 1);
    std::size_t li = left.size(), ri = right.size();
    int carry = 0;
    while (li || ri || carry) {
        int digit = carry;
        if (li) digit += left[--li] - '0';
        if (ri) digit += right[--ri] - '0';
        reversed.push_back(static_cast<char>('0' + digit % 10));
        carry = digit / 10;
    }
    return {reversed.rbegin(), reversed.rend()};
}

std::string subtractMagnitude(std::string_view larger, std::string_view smaller) {
    std::string reversed;
    reversed.reserve(larger.size());
    std::size_t li = larger.size(), ri = smaller.size();
    int borrow = 0;
    while (li) {
        int digit = larger[--li] - '0' - borrow;
        if (ri) digit -= smaller[--ri] - '0';
        borrow = digit < 0;
        if (borrow) digit += 10;
        reversed.push_back(static_cast<char>('0' + digit));
    }
    while (reversed.size() > 1 && reversed.back() == '0') reversed.pop_back();
    return {reversed.rbegin(), reversed.rend()};
}

SignedPower addPower(SignedPower left, SignedPower right) {
    if (left.magnitude == "0") return right;
    if (right.magnitude == "0") return left;
    if (left.negative == right.negative) {
        return {left.negative, addMagnitude(left.magnitude, right.magnitude)};
    }
    const int ordering = compareMagnitude(left.magnitude, right.magnitude);
    if (ordering == 0) return {};
    if (ordering > 0) {
        return {left.negative, subtractMagnitude(left.magnitude, right.magnitude)};
    }
    return {right.negative, subtractMagnitude(right.magnitude, left.magnitude)};
}

SignedPower powerOffset(std::int64_t offset) {
    if (offset < 0) return {true, std::to_string(-offset)};
    return {false, std::to_string(offset)};
}

int comparePower(const SignedPower& left, const SignedPower& right) {
    if (left.negative != right.negative) return left.negative ? -1 : 1;
    const int magnitude = compareMagnitude(left.magnitude, right.magnitude);
    return left.negative ? -magnitude : magnitude;
}

struct ExactDecimal {
    bool negative = false;
    std::string digits = "0";
    SignedPower power;

    [[nodiscard]] bool integral() const {
        return digits == "0" || !power.negative;
    }
};

ExactDecimal normalizeNumber(std::string_view token) {
    ExactDecimal result;
    std::size_t position = 0;
    if (token[position] == '-') {
        result.negative = true;
        ++position;
    }
    std::string digits;
    digits.reserve(token.size());
    bool fraction = false;
    std::int64_t fractionDigits = 0;
    for (; position < token.size() && token[position] != 'e' && token[position] != 'E';
         ++position) {
        if (token[position] == '.') {
            fraction = true;
        } else {
            digits.push_back(token[position]);
            if (fraction) ++fractionDigits;
        }
    }
    SignedPower exponent;
    if (position < token.size()) {
        ++position;
        if (token[position] == '-' || token[position] == '+') {
            exponent.negative = token[position] == '-';
            ++position;
        }
        exponent.magnitude = normalizedMagnitude(token.substr(position));
        if (exponent.magnitude == "0") exponent.negative = false;
    }
    result.digits = normalizedMagnitude(digits);
    if (result.digits == "0") return {};
    std::int64_t trailingZeros = 0;
    while (result.digits.back() == '0') {
        result.digits.pop_back();
        ++trailingZeros;
    }
    result.power = addPower(std::move(exponent), powerOffset(trailingZeros - fractionDigits));
    return result;
}

ExactDecimal wholeNumber(std::int64_t number) {
    return normalizeNumber(std::to_string(number));
}

int compareDecimal(const ExactDecimal& left, const ExactDecimal& right) {
    if (left.digits == "0" && right.digits == "0") return 0;
    if (left.digits == "0") return right.negative ? 1 : -1;
    if (right.digits == "0") return left.negative ? -1 : 1;
    if (left.negative != right.negative) return left.negative ? -1 : 1;
    const auto leftOrder = addPower(left.power, powerOffset(static_cast<std::int64_t>(left.digits.size())));
    const auto rightOrder = addPower(right.power, powerOffset(static_cast<std::int64_t>(right.digits.size())));
    int comparison = comparePower(leftOrder, rightOrder);
    if (comparison == 0) {
        const auto size = (std::max)(left.digits.size(), right.digits.size());
        for (std::size_t index = 0; index < size; ++index) {
            const char ld = index < left.digits.size() ? left.digits[index] : '0';
            const char rd = index < right.digits.size() ? right.digits[index] : '0';
            if (ld != rd) {
                comparison = ld < rd ? -1 : 1;
                break;
            }
        }
    }
    return left.negative ? -comparison : comparison;
}

bool boundedInteger(const ExactDecimal& decimal, std::int64_t low,
                    std::int64_t high, std::int64_t& output) {
    const auto magnitude = [](std::int64_t value) -> std::uint64_t {
        return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1
                         : static_cast<std::uint64_t>(value);
    };
    const auto limit = (std::max)(magnitude(low), magnitude(high));
    std::uint64_t value = 0;
    for (const char digitCharacter : decimal.digits) {
        const auto digit = static_cast<std::uint64_t>(digitCharacter - '0');
        if (digit > limit || value > (limit - digit) / 10) return false;
        value = value * 10 + digit;
    }
    const auto maxZeros = std::to_string(limit).size();
    std::size_t zeros = 0;
    for (const char digitCharacter : decimal.power.magnitude) {
        const auto digit = static_cast<std::size_t>(digitCharacter - '0');
        if (digit > maxZeros || zeros > (maxZeros - digit) / 10) return false;
        zeros = zeros * 10 + digit;
        if (zeros > maxZeros) return false;
    }
    for (; zeros != 0; --zeros) {
        if (value > limit / 10) return false;
        value *= 10;
    }
    if (value > static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)())) {
        if (!decimal.negative
            || value != static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)()) + 1) {
            return false;
        }
        output = (std::numeric_limits<std::int64_t>::min)();
    } else {
        const auto signedValue = static_cast<std::int64_t>(value);
        output = decimal.negative ? -signedValue : signedValue;
    }
    return true;
}

std::string pointerChild(std::string_view parent, std::string_view key) {
    std::string result{parent};
    if (result == "/") result.clear();
    result.push_back('/');
    for (char character : key) {
        if (character == '~') result += "~0";
        else if (character == '/') result += "~1";
        else result.push_back(character);
    }
    return result;
}

std::string pointerIndex(std::string_view parent, std::size_t index) {
    return pointerChild(parent, std::to_string(index));
}

class Auditor final {
public:
    explicit Auditor(std::span<const Value> values, bool own = false)
        : values_(values), own_(own) {}

    NativeEllipseAdmission run(std::shared_ptr<const NativeEllipseInput>* output) {
        prepare();
        rootObject(values_[0]);
        if (result_.accepted() && output != nullptr)
            *output = std::make_shared<const NativeEllipseInput>(materialize(values_[0]));
        return result_;
    }

    NativeEllipseAdmission runOwn(std::shared_ptr<const OwnPrimitiveInput>* output) {
        prepare();
        rootObject(values_[0]);
        if (result_.accepted() && output != nullptr)
            *output = std::make_shared<const OwnPrimitiveInput>(materializeOwn(values_[0]));
        return result_;
    }

private:
    void prepare() {
        validateTable();
        numbers_.reserve(values_.size());
        for (const auto& value : values_) {
            numbers_.push_back(value.kind == NativeEllipseValueKind::Number
                ? normalizeNumber(value.scalar) : ExactDecimal{});
        }
    }
    std::span<const Value> values_;
    bool own_ = false;
    NativeEllipseAdmission result_{Code::Accepted, {}};
    std::vector<ExactDecimal> numbers_;

    void validateTable() const {
        if (values_.empty() || values_.size() > 4096 || values_[0].hasKey
            || values_[0].nextSibling != NativeEllipseNoValue)
            throw std::logic_error("invalid internal ellipse root");
        std::vector<bool> seen(values_.size());
        std::vector<NativeEllipseValueId> pending{0};
        while (!pending.empty()) {
            const auto id = pending.back();
            pending.pop_back();
            if (id >= values_.size() || seen[id])
                throw std::logic_error("invalid internal ellipse value link");
            seen[id] = true;
            const auto& value = values_[id];
            if (value.kind == NativeEllipseValueKind::Number && !validNumberToken(value.scalar))
                throw std::logic_error("invalid internal ellipse number token");
            const bool container = value.kind == NativeEllipseValueKind::Object
                || value.kind == NativeEllipseValueKind::Array;
            if (value.childCount > values_.size() - 1)
                throw std::logic_error("invalid internal ellipse child count");
            if (!container && (value.childCount != 0 || value.firstChild != NativeEllipseNoValue))
                throw std::logic_error("invalid internal ellipse scalar children");
            if (container && ((value.childCount == 0) != (value.firstChild == NativeEllipseNoValue)))
                throw std::logic_error("invalid internal ellipse child count");
            auto child = value.firstChild;
            for (std::uint32_t count = 0; count < value.childCount; ++count) {
                if (child == NativeEllipseNoValue || child >= values_.size()
                    || values_[child].hasKey != (value.kind == NativeEllipseValueKind::Object))
                    throw std::logic_error("invalid internal ellipse child index or key");
                pending.push_back(child);
                child = values_[child].nextSibling;
            }
            if (child != NativeEllipseNoValue)
                throw std::logic_error("invalid internal ellipse child chain");
        }
        if (std::find(seen.begin(), seen.end(), false) != seen.end())
            throw std::logic_error("unreachable internal ellipse value");
    }

    bool reject(Code code, std::string path) {
        result_ = {code, std::move(path)};
        return false;
    }

    const Value* field(const Value& object, std::string_view key) const {
        for (auto id = object.firstChild; id != NativeEllipseNoValue; id = values_[id].nextSibling)
            if (values_[id].hasKey && values_[id].key == key) return &values_[id];
        return nullptr;
    }

    const Value& element(const Value& array, std::size_t ordinal) const {
        auto id = array.firstChild;
        while (ordinal-- != 0 && id != NativeEllipseNoValue) id = values_[id].nextSibling;
        if (id == NativeEllipseNoValue)
            throw std::logic_error("invalid internal ellipse child index");
        return values_[id];
    }

    const ExactDecimal* exact(const Value& value) const {
        return &numbers_[static_cast<std::size_t>(&value - values_.data())];
    }

    bool inertString(const Value& value, std::string path) {
        if (value.kind != NativeEllipseValueKind::String) return reject(Code::InvalidType, std::move(path));
        if (value.scalar.size() > 256) return reject(Code::UnsupportedValue, std::move(path));
        for (std::size_t index = 0; index < value.scalar.size(); ++index) {
            if (value.scalar[index] == '\0') {
                return reject(Code::UnsupportedValue, std::move(path));
            }
        }
        return true;
    }

    bool object(const Value& value, std::string_view path,
                std::initializer_list<std::string_view> required,
                std::initializer_list<std::string_view> optional = {}) {
        if (value.kind != NativeEllipseValueKind::Object) return reject(Code::InvalidType, std::string{path});
        for (auto id = value.firstChild; id != NativeEllipseNoValue; id = values_[id].nextSibling) {
            const auto name = values_[id].key;
            bool known = false;
            for (const auto allowed : required) known |= name == allowed;
            for (const auto allowed : optional) known |= name == allowed;
            if (!known) return reject(Code::UnsupportedField, pointerChild(path, name));
        }
        for (const auto needed : required) {
            if (!field(value, needed)) {
                return reject(Code::UnsupportedStructure, pointerChild(path, needed));
            }
        }
        if (const auto* name = field(value, "nm")) {
            if (!inertString(*name, pointerChild(path, "nm"))) return false;
        }
        return true;
    }

    bool number(const Value& value, std::string path, std::int64_t low,
                std::int64_t high, ExactDecimal* output = nullptr) {
        if (value.kind != NativeEllipseValueKind::Number) return reject(Code::InvalidType, std::move(path));
        const auto* numeric = exact(value);
        if (!numeric) return reject(Code::InvalidJson, "/");
        if (compareDecimal(*numeric, wholeNumber(low)) < 0
            || compareDecimal(*numeric, wholeNumber(high)) > 0) {
            return reject(Code::UnsupportedValue, std::move(path));
        }
        if (output) *output = *numeric;
        return true;
    }

    bool integer(const Value& value, std::string path, std::int64_t low,
                 std::int64_t high, std::int64_t* output = nullptr) {
        ExactDecimal numeric;
        if (!number(value, path, low, high, &numeric)) return false;
        if (!numeric.integral()) return reject(Code::UnsupportedValue, std::move(path));
        if (output && !boundedInteger(numeric, low, high, *output)) {
            return reject(Code::InvalidJson, "/");
        }
        return true;
    }

    bool literal(const Value& value, std::string path, std::string_view expected) {
        if (value.kind != NativeEllipseValueKind::String) return reject(Code::InvalidType, std::move(path));
        if (value.scalar != expected) {
            return reject(Code::UnsupportedValue, std::move(path));
        }
        return true;
    }

    bool arraySize(const Value& value, std::string path, std::size_t size) {
        if (value.kind != NativeEllipseValueKind::Array) return reject(Code::InvalidType, std::move(path));
        if (value.childCount != size) return reject(Code::UnsupportedStructure, std::move(path));
        return true;
    }

    bool vector(const Value& value, std::string_view path,
                std::initializer_list<Range> ranges, ExactDecimal* output = nullptr) {
        if (!arraySize(value, std::string{path}, static_cast<std::size_t>(ranges.size()))) {
            return false;
        }
        std::size_t index = 0;
        for (const auto [low, high] : ranges) {
            ExactDecimal numeric;
            if (!number(element(value, index), pointerIndex(path, index), low, high, &numeric)) return false;
            if (output) output[index] = std::move(numeric);
            ++index;
        }
        return true;
    }

    bool staticScalar(const Value& value, std::string_view path, std::int64_t expected) {
        if (!object(value, path, {"a", "k"})) return false;
        return integer(*field(value, "a"), pointerChild(path, "a"), 0, 0)
            && number(*field(value, "k"), pointerChild(path, "k"), expected, expected);
    }

    bool staticVector(const Value& value, std::string_view path,
                      std::initializer_list<Range> ranges) {
        if (!object(value, path, {"a", "k"})) return false;
        return integer(*field(value, "a"), pointerChild(path, "a"), 0, 0)
            && vector(*field(value, "k"), pointerChild(path, "k"), ranges);
    }

    bool easing(const Value& value, std::string_view path) {
        if (!object(value, path, {"x", "y"})) return false;
        return number(*field(value, "x"), pointerChild(path, "x"), 0, 1)
            && number(*field(value, "y"), pointerChild(path, "y"), 0, 1);
    }

    bool rootObject(const Value& root);
    bool layer(const Value& value, std::string_view path, std::int64_t rootOp);
    bool layerTransform(const Value& value, std::string_view path);
    bool group(const Value& value, std::string_view path, std::int64_t rootOp);
    bool ellipse(const Value& value, std::string_view path, std::int64_t rootOp);
    bool fill(const Value& value, std::string_view path);
    bool groupTransform(const Value& value, std::string_view path);
    bool position(const Value& value, std::string_view path, std::int64_t rootOp);
    bool ownProperty(const Value& value, std::string_view path,
                     std::int64_t rootOp, bool scalar, bool positive);
    bool ownEasing(const Value& value, std::string_view path, bool scalar);
    NativeEllipseInput materialize(const Value& root) const;
    OwnPrimitiveInput materializeOwn(const Value& root) const;
    NativeEllipsePosition ownVectorValue(const Value& value, std::uint32_t last) const;
    OwnPrimitiveScalar ownScalarValue(const Value& value, std::uint32_t last) const;
    NativeEllipseVec2 ownEasingValue(const Value& value) const;
    NativeEllipseDecimal decimal(const Value& value) const;
    std::int64_t structural(const Value& value, std::int64_t low, std::int64_t high) const;
    NativeEllipseVec2 vec2(const Value& array) const;
    std::optional<std::string> name(const Value& object, std::string_view key) const;
};

NativeEllipseDecimal Auditor::decimal(const Value& value) const {
    const auto& source = *exact(value);
    return {source.negative, source.digits,
        {source.power.negative, source.power.magnitude}};
}

std::int64_t Auditor::structural(const Value& value, std::int64_t low,
                                std::int64_t high) const {
    std::int64_t result = 0;
    if (!boundedInteger(*exact(value), low, high, result)) {
        throw std::logic_error("validated native ellipse integer could not be materialized");
    }
    return result;
}

NativeEllipseVec2 Auditor::vec2(const Value& array) const {
    return {decimal(element(array, 0)), decimal(element(array, 1))};
}

std::optional<std::string> Auditor::name(const Value& object, std::string_view key) const {
    const auto* value = field(object, key);
    if (!value) return std::nullopt;
    return std::string{value->scalar};
}

NativeEllipseInput Auditor::materialize(const Value& root) const {
    NativeEllipseInput input;
    input.width = static_cast<std::uint32_t>(structural(*field(root, "w"), 1, 8192));
    input.height = static_cast<std::uint32_t>(structural(*field(root, "h"), 1, 8192));
    input.endFrame = static_cast<std::uint32_t>(structural(*field(root, "op"), 2, 10000));
    input.frameRate = decimal(*field(root, "fr"));
    input.version = name(root, "v");
    input.name = name(root, "nm");

    const auto& layer = element(*field(root, "layers"), 0);
    input.layerId = static_cast<std::int32_t>(structural(*field(layer, "ind"), 1, 2147483647));
    input.layerInFrame = static_cast<std::uint32_t>(structural(*field(layer, "ip"), 0, input.endFrame));
    input.layerOutFrame = static_cast<std::uint32_t>(structural(*field(layer, "op"), 0, input.endFrame));
    input.layerName = name(layer, "nm");
    const auto& layerPosition = *field(*field(*field(layer, "ks"), "p"), "k");
    input.layerTranslation = vec2(layerPosition);

    const auto& group = element(*field(layer, "shapes"), 0);
    input.groupName = name(group, "nm");
    const auto& items = *field(group, "it");
    const auto& ellipse = element(items, 0);
    input.ellipseName = name(ellipse, "nm");
    input.size = vec2(*field(*field(ellipse, "s"), "k"));
    const auto& position = *field(ellipse, "p");
    const auto animated = structural(*field(position, "a"), 0, 1);
    const auto& key = *field(position, "k");
    if (animated == 0) {
        input.position = NativeEllipseStaticPosition{vec2(key)};
    } else {
        const auto& first = element(key, 0);
        const auto& last = element(key, 1);
        NativeEllipseAnimatedPosition motion;
        motion.firstFrame = static_cast<std::uint32_t>(structural(*field(first, "t"), 0, 0));
        motion.lastFrame = static_cast<std::uint32_t>(structural(*field(last, "t"), input.endFrame - 1, input.endFrame - 1));
        motion.start = vec2(*field(first, "s"));
        motion.end = vec2(*field(first, "e"));
        const auto& incoming = *field(first, "i");
        const auto& outgoing = *field(first, "o");
        motion.incoming = {decimal(*field(incoming, "x")), decimal(*field(incoming, "y"))};
        motion.outgoing = {decimal(*field(outgoing, "x")), decimal(*field(outgoing, "y"))};
        input.position = std::move(motion);
    }
    const auto& fill = element(items, 1);
    input.fillName = name(fill, "nm");
    const auto& color = *field(*field(fill, "c"), "k");
    for (std::size_t index = 0; index < 4; ++index) {
        input.fillColor[index] = decimal(element(color, index));
    }
    input.transformName = name(element(items, 2), "nm");
    return input;
}

NativeEllipseVec2 Auditor::ownEasingValue(const Value& value) const {
    const auto& x = *field(value, "x");
    const auto& y = *field(value, "y");
    return {decimal(x.kind == NativeEllipseValueKind::Array ? element(x, 0) : x),
            decimal(y.kind == NativeEllipseValueKind::Array ? element(y, 0) : y)};
}

NativeEllipsePosition Auditor::ownVectorValue(const Value& value, std::uint32_t last) const {
    const auto& key = *field(value, "k");
    if (structural(*field(value, "a"), 0, 1) == 0)
        return NativeEllipseStaticPosition{vec2(key)};
    const auto& first = element(key, 0);
    NativeEllipseAnimatedPosition motion;
    motion.firstFrame = 0;
    motion.lastFrame = last;
    motion.start = vec2(*field(first, "s"));
    motion.end = vec2(*field(first, "e"));
    motion.incoming = ownEasingValue(*field(first, "i"));
    motion.outgoing = ownEasingValue(*field(first, "o"));
    return motion;
}

OwnPrimitiveScalar Auditor::ownScalarValue(const Value& value, std::uint32_t last) const {
    const auto& key = *field(value, "k");
    if (structural(*field(value, "a"), 0, 1) == 0)
        return OwnPrimitiveStaticScalar{decimal(key)};
    const auto& first = element(key, 0);
    OwnPrimitiveAnimatedScalar motion;
    motion.firstFrame = 0;
    motion.lastFrame = last;
    motion.start = decimal(element(*field(first, "s"), 0));
    motion.end = decimal(element(*field(first, "e"), 0));
    motion.incoming = ownEasingValue(*field(first, "i"));
    motion.outgoing = ownEasingValue(*field(first, "o"));
    return motion;
}

OwnPrimitiveInput Auditor::materializeOwn(const Value& root) const {
    OwnPrimitiveInput input;
    input.width = static_cast<std::uint32_t>(structural(*field(root, "w"), 1, 8192));
    input.height = static_cast<std::uint32_t>(structural(*field(root, "h"), 1, 8192));
    input.endFrame = static_cast<std::uint32_t>(structural(*field(root, "op"), 2, 10000));
    input.frameRate = decimal(*field(root, "fr"));
    input.version = name(root, "v");
    input.name = name(root, "nm");
    const auto& layer = element(*field(root, "layers"), 0);
    input.layerId = static_cast<std::int32_t>(structural(*field(layer, "ind"), 1, 2147483647));
    input.layerInFrame = static_cast<std::uint32_t>(structural(*field(layer, "ip"), 0, input.endFrame));
    input.layerOutFrame = static_cast<std::uint32_t>(structural(*field(layer, "op"), 0, input.endFrame));
    input.layerName = name(layer, "nm");
    input.layerTranslation = vec2(*field(*field(*field(layer, "ks"), "p"), "k"));
    const auto& shapes = *field(layer, "shapes");
    input.groups.reserve(shapes.childCount);
    for (std::size_t index = 0; index < shapes.childCount; ++index) {
        const auto& group = element(shapes, index);
        const auto& items = *field(group, "it");
        const auto& primitive = element(items, 0);
        const auto& fill = element(items, 1);
        OwnPrimitiveGroupInput item;
        item.kind = field(primitive, "ty")->scalar == "rc"
            ? OwnPrimitiveKind::Rectangle : OwnPrimitiveKind::Ellipse;
        item.direction = structural(*field(primitive, "d"), 1, 3) == 3
            ? model::SourcePathDirection::CounterClockwise
            : model::SourcePathDirection::Clockwise;
        item.position = ownVectorValue(*field(primitive, "p"), input.endFrame - 1);
        item.size = ownVectorValue(*field(primitive, "s"), input.endFrame - 1);
        if (item.kind == OwnPrimitiveKind::Rectangle)
            item.roundness = ownScalarValue(*field(primitive, "r"), input.endFrame - 1);
        const auto& color = *field(*field(fill, "c"), "k");
        for (std::size_t channel = 0; channel < 4; ++channel)
            item.fillColor[channel] = decimal(element(color, channel));
        item.groupName = name(group, "nm");
        item.primitiveName = name(primitive, "nm");
        item.fillName = name(fill, "nm");
        item.transformName = name(element(items, 2), "nm");
        input.groups.push_back(std::move(item));
    }
    return input;
}

bool Auditor::position(const Value& value, std::string_view path, std::int64_t rootOp) {
    if (!object(value, path, {"a", "k"})) return false;
    std::int64_t animated = 0;
    if (!integer(*field(value, "a"), pointerChild(path, "a"), 0, 1, &animated)) return false;
    const auto kPath = pointerChild(path, "k");
    const auto& k = *field(value, "k");
    if (animated == 0) {
        return vector(k, kPath, {{-32768, 32768}, {-32768, 32768}});
    }
    if (!arraySize(k, kPath, 2)) return false;
    const auto firstPath = pointerIndex(kPath, 0);
    const auto lastPath = pointerIndex(kPath, 1);
    const auto& first = element(k, 0);
    const auto& last = element(k, 1);
    if (!object(first, firstPath, {"t", "s", "e", "i", "o"})) return false;
    if (!integer(*field(first, "t"), pointerChild(firstPath, "t"), 0, 0)) return false;
    if (!vector(*field(first, "s"), pointerChild(firstPath, "s"),
                {{-32768, 32768}, {-32768, 32768}})) return false;
    ExactDecimal end[2]{};
    if (!vector(*field(first, "e"), pointerChild(firstPath, "e"),
                {{-32768, 32768}, {-32768, 32768}}, end)) return false;
    if (!easing(*field(first, "i"), pointerChild(firstPath, "i"))) return false;
    if (!easing(*field(first, "o"), pointerChild(firstPath, "o"))) return false;
    if (!object(last, lastPath, {"t", "s"})) return false;
    if (!integer(*field(last, "t"), pointerChild(lastPath, "t"), rootOp - 1, rootOp - 1)) {
        return false;
    }
    ExactDecimal start[2]{};
    if (!vector(*field(last, "s"), pointerChild(lastPath, "s"),
                {{-32768, 32768}, {-32768, 32768}}, start)) return false;
    if (compareDecimal(start[0], end[0]) != 0 || compareDecimal(start[1], end[1]) != 0) {
        return reject(Code::UnsupportedValue, pointerChild(lastPath, "s"));
    }
    return true;
}

bool Auditor::ownEasing(const Value& value, std::string_view path, bool scalar) {
    if (!object(value, path, {"x", "y"})) return false;
    for (const auto key : {"x", "y"}) {
        const auto componentPath = pointerChild(path, key);
        const auto& component = *field(value, key);
        if (component.kind == NativeEllipseValueKind::Number) {
            if (!number(component, componentPath, 0, 1)) return false;
        } else {
            const auto count = scalar ? 1U : 2U;
            if (!arraySize(component, componentPath, count)) return false;
            ExactDecimal first;
            for (std::size_t index = 0; index < count; ++index) {
                ExactDecimal current;
                if (!number(element(component, index), pointerIndex(componentPath, index),
                            0, 1, &current)) return false;
                if (index == 0) first = current;
                else if (compareDecimal(first, current) != 0)
                    return reject(Code::UnsupportedValue, pointerIndex(componentPath, index));
            }
        }
    }
    return true;
}

bool Auditor::ownProperty(const Value& value, std::string_view path,
                          std::int64_t rootOp, bool scalar, bool positive) {
    if (!object(value, path, {"a", "k"})) return false;
    std::int64_t animated = 0;
    if (!integer(*field(value, "a"), pointerChild(path, "a"), 0, 1, &animated)) return false;
    const auto kPath = pointerChild(path, "k");
    const auto& key = *field(value, "k");
    const auto low = positive || scalar ? 0 : -32768;
    const auto high = positive || scalar ? 16384 : 32768;
    const auto values = [&](const Value& source, const std::string& sourcePath,
                            std::array<ExactDecimal, 2>* output = nullptr) {
        if (scalar) {
            if (!arraySize(source, sourcePath, 1)) return false;
        } else if (!arraySize(source, sourcePath, 2)) return false;
        for (std::size_t index = 0; index < (scalar ? 1U : 2U); ++index) {
            ExactDecimal current;
            if (!number(element(source, index), pointerIndex(sourcePath, index),
                        low, high, &current)) return false;
            if (positive && compareDecimal(current, wholeNumber(0)) <= 0)
                return reject(Code::UnsupportedValue, pointerIndex(sourcePath, index));
            if (output) (*output)[index] = std::move(current);
        }
        return true;
    };
    if (animated == 0) {
        if (scalar) return number(key, kPath, 0, 16384);
        return values(key, kPath);
    }
    if (!arraySize(key, kPath, 2)) return false;
    const auto firstPath = pointerIndex(kPath, 0);
    const auto lastPath = pointerIndex(kPath, 1);
    const auto& first = element(key, 0);
    const auto& last = element(key, 1);
    if (!object(first, firstPath, {"t", "s", "e", "i", "o"})
        || !integer(*field(first, "t"), pointerChild(firstPath, "t"), 0, 0)
        || !values(*field(first, "s"), pointerChild(firstPath, "s"))) return false;
    std::array<ExactDecimal, 2> end{};
    if (!values(*field(first, "e"), pointerChild(firstPath, "e"), &end)
        || !ownEasing(*field(first, "i"), pointerChild(firstPath, "i"), scalar)
        || !ownEasing(*field(first, "o"), pointerChild(firstPath, "o"), scalar)
        || !object(last, lastPath, {"t", "s"})
        || !integer(*field(last, "t"), pointerChild(lastPath, "t"), rootOp - 1, rootOp - 1))
        return false;
    std::array<ExactDecimal, 2> start{};
    if (!values(*field(last, "s"), pointerChild(lastPath, "s"), &start)) return false;
    for (std::size_t index = 0; index < (scalar ? 1U : 2U); ++index)
        if (compareDecimal(start[index], end[index]) != 0)
            return reject(Code::UnsupportedValue, pointerChild(lastPath, "s"));
    return true;
}

bool Auditor::layerTransform(const Value& value, std::string_view path) {
    if (!object(value, path, {"o", "r", "p", "a", "s"})) return false;
    return staticScalar(*field(value, "o"), pointerChild(path, "o"), 100)
        && staticScalar(*field(value, "r"), pointerChild(path, "r"), 0)
        && staticVector(*field(value, "p"), pointerChild(path, "p"),
            {{-32768, 32768}, {-32768, 32768}, {0, 0}})
        && staticVector(*field(value, "a"), pointerChild(path, "a"),
            {{0, 0}, {0, 0}, {0, 0}})
        && staticVector(*field(value, "s"), pointerChild(path, "s"),
            {{100, 100}, {100, 100}, {100, 100}});
}

bool Auditor::ellipse(const Value& value, std::string_view path, std::int64_t rootOp) {
    if (own_) {
        if (value.kind != NativeEllipseValueKind::Object)
            return reject(Code::InvalidType, std::string{path});
        const auto* type = field(value, "ty");
        if (!type) return reject(Code::UnsupportedStructure, pointerChild(path, "ty"));
        if (type->kind != NativeEllipseValueKind::String)
            return reject(Code::InvalidType, pointerChild(path, "ty"));
        const bool rectangle = type->scalar == "rc";
        if (!rectangle && type->scalar != "el")
            return reject(Code::UnsupportedValue, pointerChild(path, "ty"));
        if (!object(value, path, rectangle
                ? std::initializer_list<std::string_view>{"ty", "d", "s", "p", "r"}
                : std::initializer_list<std::string_view>{"ty", "d", "s", "p"}, {"nm"})) return false;
        std::int64_t direction = 0;
        if (!integer(*field(value, "d"), pointerChild(path, "d"), 1, 3, &direction)) return false;
        if (direction == 2) return reject(Code::UnsupportedValue, pointerChild(path, "d"));
        if (!ownProperty(*field(value, "s"), pointerChild(path, "s"), rootOp, false, true)
            || !ownProperty(*field(value, "p"), pointerChild(path, "p"), rootOp, false, false))
            return false;
        return !rectangle || ownProperty(*field(value, "r"), pointerChild(path, "r"),
                                         rootOp, true, false);
    }
    if (!object(value, path, {"ty", "d", "s", "p"}, {"nm"})) return false;
    if (!literal(*field(value, "ty"), pointerChild(path, "ty"), "el")) return false;
    if (!integer(*field(value, "d"), pointerChild(path, "d"), 1, 1)) return false;
    const auto sPath = pointerChild(path, "s");
    const auto& s = *field(value, "s");
    if (!object(s, sPath, {"a", "k"})) return false;
    if (!integer(*field(s, "a"), pointerChild(sPath, "a"), 0, 0)) return false;
    const auto kPath = pointerChild(sPath, "k");
    const auto& k = *field(s, "k");
    ExactDecimal size[2]{};
    if (!vector(k, kPath, {{0, 16384}, {0, 16384}}, size)) return false;
    for (std::size_t index = 0; index < 2; ++index) {
        if (compareDecimal(size[index], wholeNumber(0)) <= 0) {
            return reject(Code::UnsupportedValue, pointerIndex(kPath, index));
        }
    }
    return position(*field(value, "p"), pointerChild(path, "p"), rootOp);
}

bool Auditor::fill(const Value& value, std::string_view path) {
    if (!object(value, path, {"ty", "c", "o", "r"}, {"nm"})) return false;
    if (!literal(*field(value, "ty"), pointerChild(path, "ty"), "fl")) return false;
    return staticVector(*field(value, "c"), pointerChild(path, "c"),
            {{0, 1}, {0, 1}, {0, 1}, {1, 1}})
        && staticScalar(*field(value, "o"), pointerChild(path, "o"), 100)
        && integer(*field(value, "r"), pointerChild(path, "r"), 1, 1);
}

bool Auditor::groupTransform(const Value& value, std::string_view path) {
    if (!object(value, path, {"ty", "p", "a", "s", "r", "sk", "sa", "o"}, {"nm"})) {
        return false;
    }
    if (!literal(*field(value, "ty"), pointerChild(path, "ty"), "tr")) return false;
    return staticVector(*field(value, "p"), pointerChild(path, "p"), {{0, 0}, {0, 0}})
        && staticVector(*field(value, "a"), pointerChild(path, "a"), {{0, 0}, {0, 0}})
        && staticVector(*field(value, "s"), pointerChild(path, "s"), {{100, 100}, {100, 100}})
        && staticScalar(*field(value, "r"), pointerChild(path, "r"), 0)
        && staticScalar(*field(value, "sk"), pointerChild(path, "sk"), 0)
        && staticScalar(*field(value, "sa"), pointerChild(path, "sa"), 0)
        && staticScalar(*field(value, "o"), pointerChild(path, "o"), 100);
}

bool Auditor::group(const Value& value, std::string_view path, std::int64_t rootOp) {
    if (!object(value, path, {"ty", "it"}, {"nm"})) return false;
    if (!literal(*field(value, "ty"), pointerChild(path, "ty"), "gr")) return false;
    const auto itPath = pointerChild(path, "it");
    const auto& items = *field(value, "it");
    if (!arraySize(items, itPath, 3)) return false;
    return ellipse(element(items, 0), pointerIndex(itPath, 0), rootOp)
        && fill(element(items, 1), pointerIndex(itPath, 1))
        && groupTransform(element(items, 2), pointerIndex(itPath, 2));
}

bool Auditor::layer(const Value& value, std::string_view path, std::int64_t rootOp) {
    if (!object(value, path,
                own_ ? std::initializer_list<std::string_view>{"ddd", "ind", "ty", "sr", "ks", "shapes", "ip", "op", "st", "bm"}
                     : std::initializer_list<std::string_view>{"ddd", "ind", "ty", "sr", "ks", "ao", "shapes", "ip", "op", "st", "bm"},
                own_ ? std::initializer_list<std::string_view>{"nm", "ao"}
                     : std::initializer_list<std::string_view>{"nm"})) return false;
    if (!integer(*field(value, "ddd"), pointerChild(path, "ddd"), 0, 0)
        || !integer(*field(value, "ind"), pointerChild(path, "ind"), 1, 2147483647)
        || !integer(*field(value, "ty"), pointerChild(path, "ty"), 4, 4)
        || !number(*field(value, "sr"), pointerChild(path, "sr"), 1, 1)
        || !layerTransform(*field(value, "ks"), pointerChild(path, "ks"))) return false;
    if (const auto* ao = field(value, "ao"))
        if (!integer(*ao, pointerChild(path, "ao"), 0, 0)) return false;
    const auto shapesPath = pointerChild(path, "shapes");
    const auto& shapes = *field(value, "shapes");
    if (!own_) {
        if (!arraySize(shapes, shapesPath, 1)) return false;
    } else {
        if (shapes.kind != NativeEllipseValueKind::Array)
            return reject(Code::InvalidType, shapesPath);
        if (shapes.childCount < 1 || shapes.childCount > 16)
            return reject(Code::UnsupportedStructure, shapesPath);
    }
    for (std::size_t index = 0; index < shapes.childCount; ++index)
        if (!group(element(shapes, index), pointerIndex(shapesPath, index), rootOp)) return false;
    std::int64_t ip = 0;
    std::int64_t op = 0;
    if (!integer(*field(value, "ip"), pointerChild(path, "ip"), 0, rootOp, &ip)
        || !integer(*field(value, "op"), pointerChild(path, "op"), 0, rootOp, &op)) return false;
    if (ip >= op) return reject(Code::UnsupportedValue, pointerChild(path, "op"));
    return integer(*field(value, "st"), pointerChild(path, "st"), 0, 0)
        && integer(*field(value, "bm"), pointerChild(path, "bm"), 0, 0);
}

bool Auditor::rootObject(const Value& root) {
    constexpr std::string_view path = "/";
    if (!object(root, path,
                {"fr", "ip", "op", "w", "h", "ddd", "assets", "markers", "layers"},
                {"v", "nm"})) return false;
    if (const auto* version = field(root, "v")) {
        if (!inertString(*version, pointerChild(path, "v"))) return false;
    }
    std::int64_t rootOp = 0;
    ExactDecimal frameRate;
    if (!number(*field(root, "fr"), "/fr", 0, 240, &frameRate)) return false;
    if (compareDecimal(frameRate, wholeNumber(0)) <= 0) {
        return reject(Code::UnsupportedValue, "/fr");
    }
    if (!integer(*field(root, "ip"), "/ip", 0, 0)
        || !integer(*field(root, "op"), "/op", 2, 10000, &rootOp)
        || !integer(*field(root, "w"), "/w", 1, 8192)
        || !integer(*field(root, "h"), "/h", 1, 8192)
        || !integer(*field(root, "ddd"), "/ddd", 0, 0)
        || !arraySize(*field(root, "assets"), "/assets", 0)
        || !arraySize(*field(root, "markers"), "/markers", 0)) return false;
    const auto& layers = *field(root, "layers");
    if (!arraySize(layers, "/layers", 1)) return false;
    return layer(element(layers, 0), "/layers/0", rootOp);
}


} // namespace

NativeEllipseAdmission evaluateNativeEllipseValues(
    std::span<const NativeEllipseValue> values,
    std::shared_ptr<const NativeEllipseInput>* output) {
    if (output) output->reset();
    return Auditor{values}.run(output);
}

NativeEllipseAdmission evaluateOwnPrimitiveValues(
    std::span<const NativeEllipseValue> values, std::shared_ptr<const OwnPrimitiveInput>* output) {
    if (output) output->reset();
    return Auditor{values, true}.runOwn(output);
}

bool exactOwnPrimitiveBound(const NativeEllipseDecimal& value,
                            std::int64_t low, std::int64_t high, bool strictLow) {
    if (!numeric::canonical(value)) return false;
    const ExactDecimal exact{value.negative, value.digits,
        {value.power.negative, value.power.magnitude}};
    const auto lower = compareDecimal(exact, wholeNumber(low));
    return (strictLow ? lower > 0 : lower >= 0)
        && compareDecimal(exact, wholeNumber(high)) <= 0;
}

bool convertOwnNumericToken(std::string_view token, std::int64_t low, std::int64_t high,
                            bool integral, bool strictLow, float& output) {
    if (!validNumberToken(token) || low > high) return false;
    const auto exact = normalizeNumber(token);
    if (integral && !exact.integral()) return false;
    const NativeEllipseDecimal value{exact.negative, exact.digits,
        {exact.power.negative, exact.power.magnitude}};
    return exactOwnPrimitiveBound(value, low, high, strictLow) && numeric::convert(value, output);
}

bool equalOwnNumericTokens(std::string_view left, std::string_view right) {
    return validNumberToken(left) && validNumberToken(right)
        && compareDecimal(normalizeNumber(left), normalizeNumber(right)) == 0;
}

} // namespace avemotion::runtime::detail
