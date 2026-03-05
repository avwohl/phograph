#include "pho_prim.h"

namespace pho {

void register_error_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("error-create", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        std::string msg = in[0].is_string() ? in[0].as_string()->str() : in[0].to_display_string();
        return PrimResult::success(Value::error(msg));
    });

    reg.register_prim("error-create-code", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        std::string msg = in[0].is_string() ? in[0].as_string()->str() : in[0].to_display_string();
        std::string code = in[1].is_string() ? in[1].as_string()->str() : in[1].to_display_string();
        return PrimResult::success(Value::error(msg, code));
    });

    reg.register_prim("error-message", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_error()) return PrimResult::fail();
        return PrimResult::success(Value::string(in[0].as_error()->message()));
    });

    reg.register_prim("error-code", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_error()) return PrimResult::fail();
        return PrimResult::success(Value::string(in[0].as_error()->code()));
    });

    reg.register_prim("fail-with", 1, 0, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::fail_with(in[0]);
    });

    // Debug/console
    reg.register_prim("log", 1, 0, [](const std::vector<Value>& in) -> PrimResult {
        for (auto& v : in) {
            pho_console_write("[log] " + v.to_display_string() + "\n");
        }
        return PrimResult::success();
    });

    reg.register_prim("inspect", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        pho_console_write("[inspect] " + in[0].to_display_string() + "\n");
        return PrimResult::success(in[0]);
    });
}

} // namespace pho
