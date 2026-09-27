#include "NativeEllipseAdmission.hpp"
#include "NativeEllipseInput.hpp"
#include "NativeEllipseAdmissionCore.hpp"

#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/reader.h>

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace avemotion::runtime::detail {
namespace {

using Code = NativeEllipseAdmissionCode;
using Value = rapidjson::Value;
enum class EventKind { Null, Boolean, Number, String, Object, Array };

struct ValueEvent {
    EventKind kind;
    std::string rawNumber;
};

struct RawNumberHandler final : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, RawNumberHandler> {
    std::vector<ValueEvent> events;

    bool add(EventKind kind, const char* raw = nullptr, rapidjson::SizeType length = 0) {
        if (events.size() == 4096) return false;
        events.push_back({kind, raw ? std::string{raw, length} : std::string{}});
        return true;
    }
    bool Null() { return add(EventKind::Null); }
    bool Bool(bool) { return add(EventKind::Boolean); }
    bool RawNumber(const char* raw, rapidjson::SizeType length, bool) {
        return add(EventKind::Number, raw, length);
    }
    bool String(const char*, rapidjson::SizeType, bool) { return add(EventKind::String); }
    bool Key(const char*, rapidjson::SizeType, bool) { return true; }
    bool StartObject() { return add(EventKind::Object); }
    bool StartArray() { return add(EventKind::Array); }
};

EventKind kindOf(const Value& value) {
    if (value.IsNull()) return EventKind::Null;
    if (value.IsBool()) return EventKind::Boolean;
    if (value.IsNumber()) return EventKind::Number;
    if (value.IsString()) return EventKind::String;
    return value.IsObject() ? EventKind::Object : EventKind::Array;
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

std::string pointerIndex(std::string_view parent, rapidjson::SizeType index) {
    return pointerChild(parent, std::to_string(index));
}

std::string_view memberName(const Value::ConstMemberIterator& member) {
    return {member->name.GetString(), member->name.GetStringLength()};
}

class Frontend final {
public:
    NativeEllipseAdmission run(const Value& root, std::string_view json,
                               std::shared_ptr<const NativeEllipseInput>* output) {
        if (!resources(root)) return result_;
        if (!captureNumbers(root, json)) return result_;
        std::vector<NativeEllipseValue> values;
        values.reserve(handler_.events.size());
        project(root, {}, false, values);
        return evaluateNativeEllipseValues(values, output);
    }

private:
    NativeEllipseAdmission result_{Code::Accepted, {}};
    RawNumberHandler handler_;
    std::vector<std::pair<const Value*, std::size_t>> numbers_;

    bool reject(Code code, std::string path) {
        result_ = {code, std::move(path)};
        return false;
    }

    bool resources(const Value& root) {
        struct Node {
            const Value* value;
            std::size_t parent;
            std::string_view name;
            rapidjson::SizeType index;
            bool indexed;
            std::size_t depth;
        };
        std::vector<Node> nodes;
        nodes.reserve(4096);
        nodes.push_back({&root, 0, {}, 0, false, 1});
        const auto pathOf = [&nodes](std::size_t id) {
            std::vector<std::size_t> ancestors;
            while (id != 0) {
                ancestors.push_back(id);
                id = nodes[id].parent;
            }
            std::string path = "/";
            for (auto ancestor = ancestors.rbegin(); ancestor != ancestors.rend(); ++ancestor) {
                const auto& node = nodes[*ancestor];
                path = node.indexed ? pointerIndex(path, node.index)
                                    : pointerChild(path, node.name);
            }
            return path;
        };
        for (std::size_t currentIndex = 0; currentIndex < nodes.size(); ++currentIndex) {
            const Node current = nodes[currentIndex];
            if ((current.value->IsObject() || current.value->IsArray())
                && current.depth > 32) {
                return reject(Code::ResourceLimit, pathOf(currentIndex));
            }
            if (current.value->IsObject()) {
                std::vector<std::string_view> names;
                names.reserve(current.value->MemberCount());
                for (auto member = current.value->MemberBegin();
                     member != current.value->MemberEnd(); ++member) {
                    const auto name = memberName(member);
                    for (const auto prior : names) {
                        if (prior == name) {
                            return reject(Code::InvalidJson, pointerChild(pathOf(currentIndex), name));
                        }
                    }
                    names.push_back(name);
                    if (nodes.size() == 4096) {
                        return reject(Code::ResourceLimit, pointerChild(pathOf(currentIndex), name));
                    }
                    nodes.push_back({&member->value, currentIndex, name, 0, false,
                        current.depth + (member->value.IsObject() || member->value.IsArray())});
                }
            } else if (current.value->IsArray()) {
                for (rapidjson::SizeType index = 0; index < current.value->Size(); ++index) {
                    const auto& child = (*current.value)[index];
                    if (nodes.size() == 4096) {
                        return reject(Code::ResourceLimit, pointerIndex(pathOf(currentIndex), index));
                    }
                    nodes.push_back({&child, currentIndex, {}, index, true,
                        current.depth + (child.IsObject() || child.IsArray())});
                }
            }
        }
        return true;
    }

    bool captureNumbers(const Value& root, std::string_view json) {
        rapidjson::MemoryStream stream{json.data(), json.size()};
        rapidjson::Reader reader;
                reader.Parse<rapidjson::kParseIterativeFlag |
                     rapidjson::kParseValidateEncodingFlag |
                     rapidjson::kParseNumbersAsStringsFlag>(stream, handler_);
        if (reader.HasParseError()) return reject(Code::InvalidJson, "/");

        std::vector<const Value*> pending{&root};
        std::size_t eventIndex = 0;
        while (!pending.empty()) {
            const auto* value = pending.back();
            pending.pop_back();
            if (eventIndex == handler_.events.size()
                || kindOf(*value) != handler_.events[eventIndex].kind) {
                return reject(Code::InvalidJson, "/");
            }
            if (value->IsNumber()) {
                numbers_.emplace_back(value, eventIndex);
            }
            ++eventIndex;
            if (value->IsObject()) {
                for (auto member = value->MemberEnd(); member != value->MemberBegin();) {
                    --member;
                    pending.push_back(&member->value);
                }
            } else if (value->IsArray()) {
                for (rapidjson::SizeType index = value->Size(); index > 0; --index) {
                    pending.push_back(&(*value)[index - 1]);
                }
            }
        }
        if (eventIndex != handler_.events.size()) return reject(Code::InvalidJson, "/");
        return true;
    }

    NativeEllipseValueId project(const Value& value, std::string_view key, bool hasKey,
                                 std::vector<NativeEllipseValue>& values) const {
        const auto id = static_cast<NativeEllipseValueId>(values.size());
        NativeEllipseValue item;
        item.hasKey = hasKey;
        item.key = key;
        switch (kindOf(value)) {
        case EventKind::Null: item.kind = NativeEllipseValueKind::Null; break;
        case EventKind::Boolean: item.kind = NativeEllipseValueKind::Boolean; break;
        case EventKind::Number:
            item.kind = NativeEllipseValueKind::Number;
            {
            bool found = false;
            for (const auto [number, event] : numbers_) {
                if (number == &value) {
                    item.scalar = handler_.events[event].rawNumber;
                    found = true;
                    break;
                }
            }
            if (!found) throw std::logic_error("missing aligned ellipse number event");
            }
            break;
        case EventKind::String:
            item.kind = NativeEllipseValueKind::String;
            item.scalar = {value.GetString(), value.GetStringLength()};
            break;
        case EventKind::Object: item.kind = NativeEllipseValueKind::Object; break;
        case EventKind::Array: item.kind = NativeEllipseValueKind::Array; break;
        }
        values.push_back(item);
        NativeEllipseValueId prior = NativeEllipseNoValue;
        const auto addChild = [&](const Value& child, std::string_view childKey, bool keyed) {
            const auto childId = project(child, childKey, keyed, values);
            if (prior == NativeEllipseNoValue) values[id].firstChild = childId;
            else values[prior].nextSibling = childId;
            prior = childId;
            ++values[id].childCount;
        };
        if (value.IsObject()) {
            for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
                addChild(member->value, memberName(member), true);
        } else if (value.IsArray()) {
            for (rapidjson::SizeType index = 0; index < value.Size(); ++index)
                addChild(value[index], {}, false);
        }
        return id;
    }
};
} // namespace

namespace {
NativeEllipseAdmission parseNativeEllipseInput(
    std::string_view json, std::shared_ptr<const NativeEllipseInput>* output) {
    if (json.size() > 1'048'576) return {Code::ResourceLimit, "/"};
    if (json.empty() || json.find('\0') != std::string_view::npos) {
        return {Code::InvalidJson, "/"};
    }
    rapidjson::Document document;
    document.Parse<rapidjson::kParseIterativeFlag |
                   rapidjson::kParseValidateEncodingFlag>(json.data(), json.size());
    if (document.HasParseError()) return {Code::InvalidJson, "/"};
    return Frontend{}.run(document, json, output);
}
} // namespace

NativeEllipseAdmission auditNativeEllipseInput(std::string_view json) {
    return parseNativeEllipseInput(json, nullptr);
}

NativeEllipseInputResult decodeNativeEllipseInput(std::string_view json) {
    NativeEllipseInputResult result;
    result.admission = parseNativeEllipseInput(json, &result.input);
    return result;
}

} // namespace avemotion::runtime::detail
