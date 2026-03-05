#include "pho_prim.h"

namespace pho {

void register_dict_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("dict-create", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::dict(make_ref<PhoDict>()));
    });

    reg.register_prim("dict-get", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_dict();
        if (!d->has(in[1])) return PrimResult::fail();
        return PrimResult::success(d->get(in[1]));
    });

    reg.register_prim("dict-get-default", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_dict();
        if (!d->has(in[1])) return PrimResult::success(in[2]);
        return PrimResult::success(d->get(in[1]));
    });

    reg.register_prim("dict-set", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto new_dict = make_ref<PhoDict>();
        auto* d = in[0].as_dict();
        for (auto& [k, v] : d->entries()) new_dict->set(k, v);
        new_dict->set(in[1], in[2]);
        return PrimResult::success(Value::dict(new_dict));
    });

    reg.register_prim("dict-remove", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto new_dict = make_ref<PhoDict>();
        auto* d = in[0].as_dict();
        for (auto& [k, v] : d->entries()) {
            if (!k.equals(in[1])) new_dict->set(k, v);
        }
        return PrimResult::success(Value::dict(new_dict));
    });

    reg.register_prim("dict-has?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].as_dict()->has(in[1])));
    });

    reg.register_prim("dict-keys", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_dict();
        std::vector<Value> keys;
        for (auto& [k, v] : d->entries()) keys.push_back(k);
        return PrimResult::success(Value::list(std::move(keys)));
    });

    reg.register_prim("dict-values", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_dict();
        std::vector<Value> vals;
        for (auto& [k, v] : d->entries()) vals.push_back(v);
        return PrimResult::success(Value::list(std::move(vals)));
    });

    reg.register_prim("dict-pairs", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_dict();
        std::vector<Value> pairs;
        for (auto& [k, v] : d->entries()) {
            pairs.push_back(Value::list({k, v}));
        }
        return PrimResult::success(Value::list(std::move(pairs)));
    });

    reg.register_prim("dict-size", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(in[0].as_dict()->size())));
    });

    reg.register_prim("dict-set!", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        // Mutating set - modifies the dict in-place
        auto* d = in[0].as_dict();
        d->set(in[1], in[2]);
        return PrimResult::success(in[0]);
    });

    reg.register_prim("dict-merge", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto new_dict = make_ref<PhoDict>();
        for (auto& [k, v] : in[0].as_dict()->entries()) new_dict->set(k, v);
        for (auto& [k, v] : in[1].as_dict()->entries()) new_dict->set(k, v);
        return PrimResult::success(Value::dict(new_dict));
    });
}

} // namespace pho
