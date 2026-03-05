#include "pho_prim.h"
#include "pho_eval.h"

namespace pho {

void register_methodref_prims() {
    auto& reg = PrimitiveRegistry::instance();

    // method-ref: string -> method-ref (universal method)
    reg.register_prim("method-ref", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail();
        auto mr = make_ref<PhoMethodRef>("", in[0].as_string()->str());
        return PrimResult::success(Value::method_ref(mr));
    });

    // method-ref-class: (class_name, method_name) -> method-ref
    reg.register_prim("method-ref-class", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string()) return PrimResult::fail();
        auto mr = make_ref<PhoMethodRef>(in[0].as_string()->str(), in[1].as_string()->str());
        return PrimResult::success(Value::method_ref(mr));
    });

    // method-ref-bound: (object, method_name) -> method-ref
    reg.register_prim("method-ref-bound", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string()) return PrimResult::fail();
        auto* obj = in[0].as_object();
        auto mr = make_ref<PhoMethodRef>(
            obj->class_name(), in[1].as_string()->str(),
            Ref<PhoObject>(obj));
        return PrimResult::success(Value::method_ref(mr));
    });

    // call: (method-ref, args...) -> result
    // Uses thread-local project/evaluator
    reg.register_prim("call", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_method_ref()) return PrimResult::fail();
        auto* mr = in[0].as_method_ref();

        auto* proj = Evaluator::tl_project;
        auto* eval = Evaluator::tl_evaluator;
        if (!proj || !eval) return PrimResult::fail();

        // Build args: if bound, prepend bound object
        std::vector<Value> args;
        if (mr->has_bound_object()) {
            args.push_back(Value::object(Ref<PhoObject>(mr->bound_object())));
        }
        // Second input should be a list of args
        if (in[1].is_list()) {
            for (auto& v : in[1].as_list()->elems()) {
                args.push_back(v);
            }
        }

        EvalResult result;
        if (!mr->class_name().empty()) {
            const auto* m = proj->find_class_method(mr->class_name(), mr->method_name());
            if (!m) return PrimResult::fail();
            result = eval->eval_method(*proj, *m, args);
        } else {
            result = eval->call_method(*proj, mr->method_name(), args);
        }

        if (result.status != EvalStatus::Success) return PrimResult::fail();
        if (result.outputs.empty()) return PrimResult::success(Value::null_val());
        return PrimResult::success(result.outputs[0]);
    });

    // is-method-ref?
    reg.register_prim("is-method-ref?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::boolean(in[0].is_method_ref()));
    });

    // method-ref-name
    reg.register_prim("method-ref-name", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_method_ref()) return PrimResult::fail();
        return PrimResult::success(Value::string(in[0].as_method_ref()->method_name()));
    });
}

} // namespace pho
