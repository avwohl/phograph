#include "pho_prim.h"

namespace pho {

void register_compare_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("=", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        bool eq = in[0].equals(in[1]);
        return PrimResult::success(Value::boolean(eq));
    });

    reg.register_prim("!=", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(!in[0].equals(in[1])));
    });

    reg.register_prim("<", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].compare(in[1]) < 0));
    });

    reg.register_prim(">", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].compare(in[1]) > 0));
    });

    reg.register_prim("<=", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].compare(in[1]) <= 0));
    });

    reg.register_prim(">=", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].compare(in[1]) >= 0));
    });
}

} // namespace pho
