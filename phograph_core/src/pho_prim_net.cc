#include "pho_prim.h"
#include "pho_serial.h"
#include "pho_platform.h"
#include <cstring>
#include <sstream>

namespace pho {

// Convert JsonValue to pho::Value
static Value json_to_value(const JsonValue& jv) {
    switch (jv.type) {
        case JsonType::Null: return Value::null_val();
        case JsonType::Boolean: return Value::boolean(jv.boolean);
        case JsonType::Number: {
            double n = jv.number;
            if (n == std::floor(n) && n >= -9007199254740992.0 && n <= 9007199254740992.0) {
                return Value::integer(static_cast<int64_t>(n));
            }
            return Value::real(n);
        }
        case JsonType::String: return Value::string(jv.str);
        case JsonType::Array: {
            std::vector<Value> elems;
            for (auto& e : jv.array) elems.push_back(json_to_value(e));
            return Value::list(std::move(elems));
        }
        case JsonType::Object: {
            auto dict = make_ref<PhoDict>();
            for (auto& [k, v] : jv.object) {
                dict->set(Value::string(k), json_to_value(v));
            }
            return Value::dict(dict);
        }
    }
    return Value::null_val();
}

// Convert pho::Value to JSON string
static std::string value_to_json(const Value& v) {
    switch (v.tag()) {
        case ValueTag::Null: return "null";
        case ValueTag::Boolean: return v.as_boolean() ? "true" : "false";
        case ValueTag::Integer: return std::to_string(v.as_integer());
        case ValueTag::Real: {
            std::ostringstream oss;
            oss << v.as_real();
            return oss.str();
        }
        case ValueTag::String: {
            std::string s = "\"";
            for (char c : v.as_string()->str()) {
                switch (c) {
                    case '"': s += "\\\""; break;
                    case '\\': s += "\\\\"; break;
                    case '\n': s += "\\n"; break;
                    case '\r': s += "\\r"; break;
                    case '\t': s += "\\t"; break;
                    default: s += c;
                }
            }
            s += "\"";
            return s;
        }
        case ValueTag::List: {
            std::string s = "[";
            auto* l = v.as_list();
            for (size_t i = 0; i < l->size(); i++) {
                if (i > 0) s += ",";
                s += value_to_json(l->at(i));
            }
            s += "]";
            return s;
        }
        case ValueTag::Dict: {
            std::string s = "{";
            auto* d = v.as_dict();
            bool first = true;
            for (auto& [k, val] : d->entries()) {
                if (!first) s += ",";
                // Key must be string
                if (k.is_string()) {
                    s += value_to_json(k);
                } else {
                    s += "\"" + k.to_display_string() + "\"";
                }
                s += ":";
                s += value_to_json(val);
                first = false;
            }
            s += "}";
            return s;
        }
        default: return "null";
    }
}

void register_net_prims() {
    auto& r = PrimitiveRegistry::instance();

    // http-get: url -> string (response body)
    r.register_prim("http-get", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("http-get: expected string URL"));
        std::string body;
        int status = pho_platform_http_get(in[0].as_string()->str(), body);
        if (status < 0) return PrimResult::fail_with(Value::error("http-get: request failed"));
        return PrimResult::success(Value::string(body));
    });

    // http-post: url body content-type -> string (response body)
    r.register_prim("http-post", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string() || !in[2].is_string())
            return PrimResult::fail_with(Value::error("http-post: expected url, body, content-type strings"));
        std::string body;
        int status = pho_platform_http_post(in[0].as_string()->str(),
                                             in[1].as_string()->str(),
                                             in[2].as_string()->str(), body);
        if (status < 0) return PrimResult::fail_with(Value::error("http-post: request failed"));
        return PrimResult::success(Value::string(body));
    });

    // json-parse: string -> value (dict or list)
    r.register_prim("json-parse", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("json-parse: expected string"));
        JsonValue jv;
        std::string error;
        if (!parse_json(in[0].as_string()->str(), jv, error)) {
            return PrimResult::fail_with(Value::error("json-parse: " + error));
        }
        return PrimResult::success(json_to_value(jv));
    });

    // json-encode: value -> string
    r.register_prim("json-encode", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::string(value_to_json(in[0])));
    });
}

} // namespace pho
