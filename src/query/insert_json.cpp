#include "query/insert_json.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_set>

#include <nlohmann/json.hpp>

using Json = nlohmann::ordered_json;

// SAX callbacks keep the input order and detect duplicate keys before they are
// collapsed into a JSON object.
class InsertJsonHandler : public nlohmann::json_sax<Json> {
    std::unique_ptr<Tuple> tuple = std::make_unique<Tuple>();
    std::unordered_set<std::string> names;
    std::string currentName;
    std::uint64_t id = 0;
    bool hasId = false;
    bool objectStarted = false;
    bool objectFinished = false;

    bool addInteger(int value) {
        if (currentName == "id") {
            if (value < 0) return false;
            id = static_cast<std::uint64_t>(value);
            hasId = true;
        }
        tuple->addField(std::make_unique<Field>(value, currentName));
        return true;
    }

public:
    bool null() override { return false; }
    bool boolean(bool) override { return false; }

    bool number_integer(number_integer_t value) override {
        // Negative JSON integers, including -0, cannot be used as an id.
        if (currentName == "id") return false;
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) return false;
        return addInteger(static_cast<int>(value));
    }

    bool number_unsigned(number_unsigned_t value) override {
        if (value > static_cast<number_unsigned_t>(std::numeric_limits<int>::max())) return false;
        return addInteger(static_cast<int>(value));
    }

    bool number_float(number_float_t value, const string_t &token) override {
        // JSON integers outside the library's integer range are reported as
        // floating-point values; they cannot fit in an integer Field.
        if (currentName == "id" || token.find_first_of(".eE") == std::string::npos || !std::isfinite(value) ||
            std::abs(value) > std::numeric_limits<float>::max()) return false;

        const float narrowed = static_cast<float>(value);
        if (narrowed == 0) {
            const auto exponent = token.find_first_of("eE");
            const auto significand = token.substr(0, exponent);
            if (std::any_of(significand.begin(), significand.end(),
                            [](char c) { return c >= '1' && c <= '9'; })) return false;
        }
        tuple->addField(std::make_unique<Field>(narrowed, currentName));
        return true;
    }

    bool string(string_t &value) override {
        if (currentName == "id" || value.size() > std::numeric_limits<std::uint16_t>::max() ||
            value.find('\0') != std::string::npos) return false;
        tuple->addField(std::make_unique<Field>(value, currentName));
        return true;
    }

    bool binary(binary_t &) override { return false; }

    bool start_object(std::size_t) override {
        if (objectStarted) return false;
        objectStarted = true;
        return true;
    }

    bool key(string_t &name) override {
        if (name.empty() || name.size() > std::numeric_limits<std::uint8_t>::max() ||
            !names.insert(name).second) return false;
        currentName = name;
        return true;
    }

    bool end_object() override {
        objectFinished = true;
        return true;
    }

    bool start_array(std::size_t) override { return false; }
    bool end_array() override { return false; }

    bool parse_error(std::size_t, const std::string &, const nlohmann::detail::exception &) override {
        return false;
    }

    std::optional<ParsedInsertJson> result() {
        if (!objectFinished || !hasId) return std::nullopt;
        return ParsedInsertJson{id, std::move(tuple)};
    }
};

std::optional<ParsedInsertJson> parseInsertJson(std::string_view json) {
    InsertJsonHandler handler;
    if (!Json::sax_parse(json.begin(), json.end(), &handler)) return std::nullopt;
    return handler.result();
}
