#include "pho_prim.h"

namespace pho {

void register_type_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("integer?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_integer()));
    });

    reg.register_prim("real?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_real()));
    });

    reg.register_prim("string?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_string()));
    });

    reg.register_prim("list?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_list()));
    });

    reg.register_prim("dict?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_dict()));
    });

    reg.register_prim("boolean?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_boolean()));
    });

    reg.register_prim("null?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_null()));
    });

    reg.register_prim("object?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_object()));
    });

    reg.register_prim("error?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_error()));
    });

    reg.register_prim("data?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_data()));
    });

    reg.register_prim("date?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_date()));
    });

    reg.register_prim("external?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_external()));
    });

    reg.register_prim("enum?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_enum()));
    });

    reg.register_prim("type-of", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::string(tag_name(in[0].tag())));
    });
}

} // namespace pho
