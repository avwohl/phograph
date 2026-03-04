#include "pho_prim.h"
#include <algorithm>
#include <sstream>

namespace pho {

void register_string_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("concat", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        std::string result;
        for (auto& v : in) {
            if (v.is_string()) result += v.as_string()->str();
            else result += v.to_display_string();
        }
        return PrimResult::success(Value::string(result));
    });

    reg.register_prim("length", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_string())
            return PrimResult::success(Value::integer(static_cast<int64_t>(in[0].as_string()->length())));
        if (in[0].is_list())
            return PrimResult::success(Value::integer(static_cast<int64_t>(in[0].as_list()->size())));
        if (in[0].is_data())
            return PrimResult::success(Value::integer(static_cast<int64_t>(in[0].as_data()->length())));
        return PrimResult::fail_with(Value::error("length: unsupported type"));
    });

    reg.register_prim("to-string", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_string()) return PrimResult::success(in[0]);
        return PrimResult::success(Value::string(in[0].to_display_string()));
    });

    reg.register_prim("from-string", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        // Try integer
        try {
            size_t pos;
            int64_t i = std::stoll(s, &pos);
            if (pos == s.size()) return PrimResult::success(Value::integer(i));
        } catch (...) {}
        // Try real
        try {
            size_t pos;
            double d = std::stod(s, &pos);
            if (pos == s.size()) return PrimResult::success(Value::real(d));
        } catch (...) {}
        if (s == "true") return PrimResult::success(Value::boolean(true));
        if (s == "false") return PrimResult::success(Value::boolean(false));
        if (s == "null") return PrimResult::success(Value::null_val());
        return PrimResult::fail();
    });

    reg.register_prim("prefix", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        int64_t n = in[1].as_integer();
        if (n < 0) n = 0;
        size_t count = std::min(static_cast<size_t>(n), s.size());
        return PrimResult::success(Value::string(s.substr(0, count)));
    });

    reg.register_prim("suffix", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        int64_t n = in[1].as_integer();
        if (n < 0) n = 0;
        size_t count = std::min(static_cast<size_t>(n), s.size());
        return PrimResult::success(Value::string(s.substr(s.size() - count)));
    });

    reg.register_prim("middle", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        int64_t start = in[1].as_integer();
        int64_t count = in[2].as_integer();
        if (start < 1) start = 1;
        size_t idx = static_cast<size_t>(start - 1);
        if (idx >= s.size()) return PrimResult::success(Value::string(""));
        size_t len = std::min(static_cast<size_t>(count), s.size() - idx);
        return PrimResult::success(Value::string(s.substr(idx, len)));
    });

    reg.register_prim("string-search", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& hay = in[0].as_string()->str();
        auto& needle = in[1].as_string()->str();
        auto pos = hay.find(needle);
        if (pos == std::string::npos) return PrimResult::fail();
        return PrimResult::success(Value::integer(static_cast<int64_t>(pos + 1)));
    });

    reg.register_prim("string-contains?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& hay = in[0].as_string()->str();
        auto& needle = in[1].as_string()->str();
        return PrimResult::success(Value::boolean(hay.find(needle) != std::string::npos));
    });

    reg.register_prim("string-replace", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = in[0].as_string()->str();
        auto& target = in[1].as_string()->str();
        auto& replacement = in[2].as_string()->str();
        if (target.empty()) return PrimResult::success(Value::string(s));
        size_t pos = 0;
        while ((pos = s.find(target, pos)) != std::string::npos) {
            s.replace(pos, target.size(), replacement);
            pos += replacement.size();
        }
        return PrimResult::success(Value::string(s));
    });

    reg.register_prim("string-replace-first", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = in[0].as_string()->str();
        auto& target = in[1].as_string()->str();
        auto& replacement = in[2].as_string()->str();
        auto pos = s.find(target);
        if (pos != std::string::npos) s.replace(pos, target.size(), replacement);
        return PrimResult::success(Value::string(s));
    });

    reg.register_prim("string-split", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        auto& sep = in[1].as_string()->str();
        std::vector<Value> parts;
        if (sep.empty()) {
            for (char c : s) parts.push_back(Value::string(std::string(1, c)));
        } else {
            size_t start = 0, end;
            while ((end = s.find(sep, start)) != std::string::npos) {
                parts.push_back(Value::string(s.substr(start, end - start)));
                start = end + sep.size();
            }
            parts.push_back(Value::string(s.substr(start)));
        }
        return PrimResult::success(Value::list(std::move(parts)));
    });

    reg.register_prim("string-trim", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = in[0].as_string()->str();
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return PrimResult::success(Value::string(""));
        auto end = s.find_last_not_of(" \t\n\r");
        return PrimResult::success(Value::string(s.substr(start, end - start + 1)));
    });

    reg.register_prim("string-starts-with?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        auto& prefix = in[1].as_string()->str();
        return PrimResult::success(Value::boolean(s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0));
    });

    reg.register_prim("string-ends-with?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        auto& suffix = in[1].as_string()->str();
        return PrimResult::success(Value::boolean(s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0));
    });

    reg.register_prim("uppercase", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = in[0].as_string()->str();
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return PrimResult::success(Value::string(s));
    });

    reg.register_prim("lowercase", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = in[0].as_string()->str();
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return PrimResult::success(Value::string(s));
    });

    reg.register_prim("string-repeat", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        int64_t count = in[1].as_integer();
        std::string result;
        for (int64_t i = 0; i < count; i++) result += s;
        return PrimResult::success(Value::string(result));
    });

    reg.register_prim("char-at", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        int64_t idx = in[1].as_integer();
        if (idx < 1 || static_cast<size_t>(idx) > s.size()) return PrimResult::fail();
        return PrimResult::success(Value::string(std::string(1, s[idx - 1])));
    });

    reg.register_prim("format", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        // Simple format: just to-string for now
        return PrimResult::success(Value::string(in[0].to_display_string()));
    });

    reg.register_prim("to-codepoints", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        std::vector<Value> cps;
        for (unsigned char c : s) cps.push_back(Value::integer(c));
        return PrimResult::success(Value::list(std::move(cps)));
    });

    reg.register_prim("from-codepoints", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::string s;
        for (auto& v : list->elems()) s += static_cast<char>(v.as_integer());
        return PrimResult::success(Value::string(s));
    });
}

} // namespace pho
