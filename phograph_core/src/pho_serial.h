#pragma once
#include "pho_graph.h"
#include "pho_value.h"
#include <string>

namespace pho {

// Minimal JSON parser for loading graph definitions.
// Uses a simple recursive-descent parser (no external dependency).

class JsonValue;

// Load a project from a JSON string
bool load_project_from_json(const std::string& json, Project& out_project, std::string& out_error);

// Minimal JSON parser types
enum class JsonType {
    Null, Boolean, Number, String, Array, Object
};

class JsonValue {
public:
    JsonType type = JsonType::Null;
    double number = 0;
    bool boolean = false;
    std::string str;
    std::vector<JsonValue> array;
    std::vector<std::pair<std::string, JsonValue>> object;

    bool is_null() const { return type == JsonType::Null; }
    bool is_object() const { return type == JsonType::Object; }
    bool is_array() const { return type == JsonType::Array; }
    bool is_string() const { return type == JsonType::String; }
    bool is_number() const { return type == JsonType::Number; }

    const JsonValue* get(const std::string& key) const {
        if (type != JsonType::Object) return nullptr;
        for (auto& [k, v] : object) {
            if (k == key) return &v;
        }
        return nullptr;
    }

    std::string get_string(const std::string& key, const std::string& def = "") const {
        auto* v = get(key);
        if (v && v->type == JsonType::String) return v->str;
        return def;
    }

    int64_t get_int(const std::string& key, int64_t def = 0) const {
        auto* v = get(key);
        if (v && v->type == JsonType::Number) return static_cast<int64_t>(v->number);
        return def;
    }

    double get_number(const std::string& key, double def = 0) const {
        auto* v = get(key);
        if (v && v->type == JsonType::Number) return v->number;
        return def;
    }

    bool get_bool(const std::string& key, bool def = false) const {
        auto* v = get(key);
        if (v && v->type == JsonType::Boolean) return v->boolean;
        return def;
    }
};

// Parse JSON string
bool parse_json(const std::string& input, JsonValue& out, std::string& error);

} // namespace pho
