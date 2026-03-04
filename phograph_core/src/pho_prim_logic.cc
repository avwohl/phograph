#include "pho_prim.h"

namespace pho {

void register_logic_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("and", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_truthy() && in[1].is_truthy()));
    });

    reg.register_prim("or", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_truthy() || in[1].is_truthy()));
    });

    reg.register_prim("not", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(!in[0].is_truthy()));
    });

    // Bitwise ops (integers only)
    reg.register_prim("bit-and", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(in[0].as_integer() & in[1].as_integer()));
    });

    reg.register_prim("bit-or", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(in[0].as_integer() | in[1].as_integer()));
    });

    reg.register_prim("bit-not", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(~in[0].as_integer()));
    });

    reg.register_prim("bit-xor", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(in[0].as_integer() ^ in[1].as_integer()));
    });

    reg.register_prim("bit-shift-left", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(in[0].as_integer() << in[1].as_integer()));
    });

    reg.register_prim("bit-shift-right", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(in[0].as_integer() >> in[1].as_integer()));
    });

    reg.register_prim("bit-test", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        int64_t val = in[0].as_integer();
        int64_t bit = in[1].as_integer();
        return PrimResult::success(Value::boolean((val & (1LL << bit)) != 0));
    });
}

} // namespace pho
