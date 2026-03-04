#include "pho_prim.h"
#include "pho_value.h"
#include "pho_platform.h"

namespace pho {

// Effects are commands that describe side effects without executing them.
// They are represented as PhoObject with class_name "Effect" and a "kind" attribute.
// The "perform" primitive executes an effect and returns its result.

static Value make_effect(const std::string& kind, const std::unordered_map<std::string, Value>& attrs) {
    auto obj = make_ref<PhoObject>("Effect");
    obj->set_attr("kind", Value::string(kind));
    for (auto& [k, v] : attrs) {
        obj->set_attr(k, v);
    }
    return Value::object(std::move(obj));
}

static PhoObject* unwrap_effect(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Effect") return nullptr;
    return obj;
}

void register_effect_prims() {
    auto& r = PrimitiveRegistry::instance();

    // cmd-read-file: path -> effect
    r.register_prim("cmd-read-file", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("cmd-read-file: expected string"));
        return PrimResult::success(make_effect("read-file", {{"path", in[0]}}));
    });

    // cmd-write-file: path content -> effect
    r.register_prim("cmd-write-file", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("cmd-write-file: expected strings"));
        return PrimResult::success(make_effect("write-file", {{"path", in[0]}, {"content", in[1]}}));
    });

    // cmd-http-get: url -> effect
    r.register_prim("cmd-http-get", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("cmd-http-get: expected string"));
        return PrimResult::success(make_effect("http-get", {{"url", in[0]}}));
    });

    // cmd-http-post: url body -> effect
    r.register_prim("cmd-http-post", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("cmd-http-post: expected strings"));
        return PrimResult::success(make_effect("http-post", {{"url", in[0]}, {"body", in[1]}}));
    });

    // cmd-log: message -> effect
    r.register_prim("cmd-log", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(make_effect("log", {{"message", in[0]}}));
    });

    // cmd-delay: seconds -> effect
    r.register_prim("cmd-delay", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("cmd-delay: expected number"));
        return PrimResult::success(make_effect("delay", {{"seconds", in[0]}}));
    });

    // cmd-none: -> effect (no-op effect)
    r.register_prim("cmd-none", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(make_effect("none", {}));
    });

    // cmd-batch: list-of-effects -> effect
    r.register_prim("cmd-batch", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list()) return PrimResult::fail_with(Value::error("cmd-batch: expected list"));
        return PrimResult::success(make_effect("batch", {{"effects", in[0]}}));
    });

    // perform: effect -> value (executes the effect)
    r.register_prim("perform", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* eff = unwrap_effect(in[0]);
        if (!eff) return PrimResult::fail_with(Value::error("perform: expected effect"));

        auto kind = eff->get_attr("kind");
        if (!kind.is_string()) return PrimResult::fail_with(Value::error("perform: invalid effect"));
        const std::string& k = kind.as_string()->str();

        if (k == "read-file") {
            auto path = eff->get_attr("path");
            if (!path.is_string()) return PrimResult::fail_with(Value::error("perform: missing path"));
            size_t len = 0;
            char* data = pho_platform_read_file(path.as_string()->c_str(), &len);
            if (!data) return PrimResult::fail_with(Value::error("perform: could not read file"));
            std::string content(data, len);
            pho_platform_free(data);
            return PrimResult::success(Value::string(std::move(content)));
        }

        if (k == "write-file") {
            auto path = eff->get_attr("path");
            auto content = eff->get_attr("content");
            if (!path.is_string() || !content.is_string())
                return PrimResult::fail_with(Value::error("perform: missing path/content"));
            int rc = pho_platform_write_file(
                path.as_string()->c_str(),
                content.as_string()->c_str(),
                content.as_string()->length()
            );
            return PrimResult::success(Value::boolean(rc == 0));
        }

        if (k == "log") {
            auto msg = eff->get_attr("message");
            pho_platform_log(msg.to_display_string().c_str());
            return PrimResult::success(Value::null_val());
        }

        if (k == "none") {
            return PrimResult::success(Value::null_val());
        }

        if (k == "http-get" || k == "http-post") {
            // Placeholder: HTTP effects require platform async implementation
            return PrimResult::fail_with(Value::error("perform: HTTP effects not yet implemented in stub"));
        }

        if (k == "delay") {
            // Placeholder: delay effects need async runtime
            return PrimResult::success(Value::null_val());
        }

        if (k == "batch") {
            // Execute all effects in sequence
            auto effects = eff->get_attr("effects");
            if (!effects.is_list()) return PrimResult::fail_with(Value::error("perform: batch needs list"));
            auto* list = effects.as_list();
            std::vector<Value> results;
            for (size_t i = 0; i < list->size(); i++) {
                auto* sub_eff = unwrap_effect(list->at(i));
                if (!sub_eff) continue;
                // Recursive perform
                auto& reg = PrimitiveRegistry::instance();
                auto* perform = reg.find("perform");
                if (perform) {
                    auto res = perform->fn({list->at(i)});
                    if (!res.failed && !res.outputs.empty()) {
                        results.push_back(res.outputs[0]);
                    }
                }
            }
            return PrimResult::success(Value::list(std::move(results)));
        }

        return PrimResult::fail_with(Value::error("perform: unknown effect kind: " + k));
    });

    // effect?: value -> boolean
    r.register_prim("effect?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(unwrap_effect(in[0]) != nullptr));
    });
}

} // namespace pho
