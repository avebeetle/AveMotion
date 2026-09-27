#pragma once
#include "OwnVectorModel.hpp"
#include <initializer_list>
#include <stdexcept>

namespace avemotion::runtime::detail::vector_compile {
// All traversal is over the bounded reader arena. This cursor supplies contextual
// diagnostics; it does not parse JSON or evaluate animation.
struct Failure final {
    std::string path, message;
};
struct Value final {
    const formats::detail::OwnJsonDocument* document;
    formats::detail::OwnJsonNodeId id;
    std::string path;
    bool exists() const;
    void require(formats::detail::OwnJsonKind kind) const;
    [[noreturn]] void fail(std::string message) const;
    Value get(std::string_view key) const;
    std::vector<Value> array(std::size_t minimum, std::size_t maximum) const;
    void keys(std::initializer_list<std::string_view> allowed) const;
    float number(std::int64_t low, std::int64_t high, bool integral = false,
                 bool strictLow = false) const;
    std::string string() const;
    bool boolean() const;
    void metadata() const;
};
class PropertyCompiler final {
public:
    explicit PropertyCompiler(model::MotionAssetModel& model) : model_(model) {}
    model::PropertyId add(model::SourceNodeId owner, model::PropertySemantic semantic,
                          Value property, model::PropertyValueType type, std::int64_t low,
                          std::int64_t high, bool animated = true, float z = 0,
                          bool strictLow = false);
    model::PropertyId scalar(model::SourceNodeId owner, model::PropertySemantic semantic,
                             float value);
    model::PropertyId vec2(model::SourceNodeId owner, model::PropertySemantic semantic,
                           model::MotionVec2Value value);

private:
    model::MotionAssetModel& model_;
    model::MotionValueRef value(Value input, model::PropertyValueType type, std::int64_t low,
                                std::int64_t high, float z, bool strictLow, bool keyValue);
    model::MotionValueRef shape(Value input);
    model::PropertyId append(model::SourceNodeId owner, model::PropertySemantic semantic,
                             model::MotionValueRef value);
};
} // namespace avemotion::runtime::detail::vector_compile
