#include "pho_prim.h"
#include "pho_eval.h"
#include <algorithm>

namespace pho {

void register_list_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("get-nth", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        int64_t idx = in[1].as_integer();
        if (idx < 1 || static_cast<size_t>(idx) > list->size()) return PrimResult::fail();
        return PrimResult::success(list->at(idx - 1));
    });

    reg.register_prim("set-nth", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        int64_t idx = in[1].as_integer();
        if (idx < 1 || static_cast<size_t>(idx) > list->size())
            return PrimResult::fail_with(Value::error("set-nth: index out of bounds"));
        std::vector<Value> elems = list->elems();
        elems[idx - 1] = in[2];
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("first", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        if (list->empty()) return PrimResult::fail();
        return PrimResult::success(list->at(0));
    });

    reg.register_prim("rest", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        if (list->empty()) return PrimResult::fail();
        std::vector<Value> elems(list->elems().begin() + 1, list->elems().end());
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("detach-l", 1, 2, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        if (list->empty()) return PrimResult::fail();
        Value first = list->at(0);
        std::vector<Value> rest(list->elems().begin() + 1, list->elems().end());
        return PrimResult::success({std::move(first), Value::list(std::move(rest))});
    });

    reg.register_prim("detach-r", 1, 2, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        if (list->empty()) return PrimResult::fail();
        std::vector<Value> front(list->elems().begin(), list->elems().end() - 1);
        Value last = list->elems().back();
        return PrimResult::success({Value::list(std::move(front)), std::move(last)});
    });

    reg.register_prim("append", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* a = in[0].as_list();
        auto* b = in[1].as_list();
        std::vector<Value> elems = a->elems();
        elems.insert(elems.end(), b->elems().begin(), b->elems().end());
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("reverse", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        std::vector<Value> elems = in[0].as_list()->elems();
        std::reverse(elems.begin(), elems.end());
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("empty?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_list()) return PrimResult::success(Value::boolean(in[0].as_list()->empty()));
        if (in[0].is_string()) return PrimResult::success(Value::boolean(in[0].as_string()->str().empty()));
        if (in[0].is_dict()) return PrimResult::success(Value::boolean(in[0].as_dict()->size() == 0));
        return PrimResult::success(Value::boolean(in[0].is_null()));
    });

    reg.register_prim("contains?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        for (auto& e : list->elems()) {
            if (e.equals(in[1])) return PrimResult::success(Value::boolean(true));
        }
        return PrimResult::success(Value::boolean(false));
    });

    reg.register_prim("in", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        for (size_t i = 0; i < list->size(); i++) {
            if (list->at(i).equals(in[1]))
                return PrimResult::success(Value::integer(static_cast<int64_t>(i + 1)));
        }
        return PrimResult::fail();
    });

    reg.register_prim("sort", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        std::vector<Value> elems = in[0].as_list()->elems();
        std::sort(elems.begin(), elems.end(), [](const Value& a, const Value& b) {
            return a.compare(b) < 0;
        });
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("make-list", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        int64_t count = in[0].as_integer();
        std::vector<Value> elems(count, in[1]);
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("split-nth", 2, 2, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        int64_t pos = in[1].as_integer();
        if (pos < 1) pos = 1;
        size_t idx = std::min(static_cast<size_t>(pos - 1), list->size());
        std::vector<Value> left(list->elems().begin(), list->elems().begin() + idx);
        std::vector<Value> right(list->elems().begin() + idx, list->elems().end());
        return PrimResult::success({Value::list(std::move(left)), Value::list(std::move(right))});
    });

    reg.register_prim("zip", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* a = in[0].as_list();
        auto* b = in[1].as_list();
        size_t len = std::min(a->size(), b->size());
        std::vector<Value> result;
        for (size_t i = 0; i < len; i++) {
            result.push_back(Value::list({a->at(i), b->at(i)}));
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    reg.register_prim("enumerate", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<Value> result;
        for (size_t i = 0; i < list->size(); i++) {
            result.push_back(Value::list({Value::integer(static_cast<int64_t>(i + 1)), list->at(i)}));
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    reg.register_prim("unique", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<Value> result;
        for (auto& e : list->elems()) {
            bool found = false;
            for (auto& r : result) {
                if (r.equals(e)) { found = true; break; }
            }
            if (!found) result.push_back(e);
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    reg.register_prim("copy", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_list()) {
            std::vector<Value> elems;
            for (auto& e : in[0].as_list()->elems()) elems.push_back(e);
            return PrimResult::success(Value::list(std::move(elems)));
        }
        return PrimResult::success(in[0]);
    });

    // HOF primitives using thread-local evaluator (Phase 14)
    // Helper lambda to call a method-ref on arguments
    auto call_ref = [](const Value& ref, const std::vector<Value>& args) -> Value {
        if (!ref.is_method_ref()) return Value::null_val();
        auto* proj = Evaluator::tl_project;
        auto* eval = Evaluator::tl_evaluator;
        if (!proj || !eval) return Value::null_val();
        auto* mr = ref.as_method_ref();
        std::vector<Value> call_args;
        if (mr->has_bound_object()) {
            call_args.push_back(Value::object(Ref<PhoObject>(mr->bound_object())));
        }
        call_args.insert(call_args.end(), args.begin(), args.end());
        EvalResult result;
        if (!mr->class_name().empty()) {
            const auto* m = proj->find_class_method(mr->class_name(), mr->method_name());
            if (!m) return Value::null_val();
            result = eval->eval_method(*proj, *m, call_args);
        } else {
            result = eval->call_method(*proj, mr->method_name(), call_args);
        }
        if (result.status != EvalStatus::Success || result.outputs.empty()) return Value::null_val();
        return result.outputs[0];
    };

    reg.register_prim("filter", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<Value> result;
        for (auto& e : list->elems()) {
            Value v = call_ref(in[1], {e});
            if (v.is_truthy()) result.push_back(e);
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    reg.register_prim("reduce", 3, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        Value acc = in[1]; // initial value
        for (auto& e : list->elems()) {
            acc = call_ref(in[2], {acc, e});
        }
        return PrimResult::success(acc);
    });

    reg.register_prim("flat-map", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<Value> result;
        for (auto& e : list->elems()) {
            Value v = call_ref(in[1], {e});
            if (v.is_list()) {
                for (auto& sub : v.as_list()->elems()) result.push_back(sub);
            } else {
                result.push_back(v);
            }
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    reg.register_prim("any?", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        for (auto& e : list->elems()) {
            if (call_ref(in[1], {e}).is_truthy()) return PrimResult::success(Value::boolean(true));
        }
        return PrimResult::success(Value::boolean(false));
    });

    reg.register_prim("all?", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        for (auto& e : list->elems()) {
            if (!call_ref(in[1], {e}).is_truthy()) return PrimResult::success(Value::boolean(false));
        }
        return PrimResult::success(Value::boolean(true));
    });

    reg.register_prim("find", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        for (auto& e : list->elems()) {
            if (call_ref(in[1], {e}).is_truthy()) return PrimResult::success(e);
        }
        return PrimResult::fail();
    });

    reg.register_prim("sort-by", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        std::vector<Value> elems = in[0].as_list()->elems();
        Value ref = in[1];
        std::sort(elems.begin(), elems.end(), [&](const Value& a, const Value& b) {
            Value va = call_ref(ref, {a});
            Value vb = call_ref(ref, {b});
            return va.compare(vb) < 0;
        });
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("group-by", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        // Returns a dict: key -> list of values
        auto dict = make_ref<PhoDict>();
        for (auto& e : list->elems()) {
            Value key = call_ref(in[1], {e});
            Value existing = dict->get(key);
            if (existing.is_list()) {
                auto elems = existing.as_list()->elems();
                elems.push_back(e);
                dict->set(key, Value::list(std::move(elems)));
            } else {
                dict->set(key, Value::list({e}));
            }
        }
        return PrimResult::success(Value::dict(dict));
    });

    reg.register_prim("map", 2, 1, [call_ref](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<Value> result;
        for (auto& e : list->elems()) {
            result.push_back(call_ref(in[1], {e}));
        }
        return PrimResult::success(Value::list(std::move(result)));
    });
}

} // namespace pho
