#include "pho_prim.h"
#include "pho_eval.h"

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

    // Phase 13: class-of, instance-of?, responds-to?
    reg.register_prim("class-of", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_object()) return PrimResult::success(Value::string(in[0].as_object()->class_name()));
        return PrimResult::success(Value::string(tag_name(in[0].tag())));
    });

    reg.register_prim("instance-of?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string()) return PrimResult::success(Value::boolean(false));
        auto* proj = Evaluator::tl_project;
        if (!proj) return PrimResult::success(Value::boolean(in[0].as_object()->class_name() == in[1].as_string()->str()));
        // Walk inheritance chain
        std::string cls_name = in[0].as_object()->class_name();
        std::string target = in[1].as_string()->str();
        while (!cls_name.empty()) {
            if (cls_name == target) return PrimResult::success(Value::boolean(true));
            const auto* cls = proj->find_class(cls_name);
            if (!cls) break;
            cls_name = cls->parent_name;
        }
        return PrimResult::success(Value::boolean(false));
    });

    reg.register_prim("responds-to?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string()) return PrimResult::success(Value::boolean(false));
        auto* proj = Evaluator::tl_project;
        if (!proj) return PrimResult::success(Value::boolean(false));
        auto* m = proj->find_class_method(in[0].as_object()->class_name(), in[1].as_string()->str());
        return PrimResult::success(Value::boolean(m != nullptr));
    });

    // Phase 25: conforms-to?
    reg.register_prim("conforms-to?", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string()) return PrimResult::success(Value::boolean(false));
        auto* proj = Evaluator::tl_project;
        if (!proj) return PrimResult::success(Value::boolean(false));
        auto* cls = proj->find_class(in[0].as_object()->class_name());
        if (!cls) return PrimResult::success(Value::boolean(false));
        std::string protocol_name = in[1].as_string()->str();
        for (auto& p : cls->conforms_to) {
            if (p == protocol_name) return PrimResult::success(Value::boolean(true));
        }
        return PrimResult::success(Value::boolean(false));
    });

    // Phase 14: enum introspection
    reg.register_prim("enum-variant", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_enum()) return PrimResult::fail();
        return PrimResult::success(Value::string(in[0].as_enum()->variant()));
    });

    reg.register_prim("enum-type", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_enum()) return PrimResult::fail();
        return PrimResult::success(Value::string(in[0].as_enum()->type_name()));
    });

    reg.register_prim("enum-create", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string()) return PrimResult::fail();
        std::vector<Value> data;
        if (in[2].is_list()) {
            data = in[2].as_list()->elems();
        }
        auto e = make_ref<PhoEnum>(in[0].as_string()->str(), in[1].as_string()->str(), std::move(data));
        return PrimResult::success(Value::enum_val(e));
    });

    reg.register_prim("enum-data", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_enum()) return PrimResult::fail();
        return PrimResult::success(Value::list(in[0].as_enum()->data()));
    });

    // Phase 26: Observable attribute primitives

    // observe: (object, attr_name) -> observer_id
    // Registers a no-op observer that logs changes. In a full implementation,
    // the third input would be a method-ref callback.
    reg.register_prim("observe", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string()) return PrimResult::fail();
        auto* obj = in[0].as_object();
        std::string attr = in[1].as_string()->str();
        ObserverId id = obj->add_observer(attr, [](const std::string&, const Value&, const Value&) {});
        return PrimResult::success(Value::integer(static_cast<int64_t>(id)));
    });

    // unobserve: (object, observer_id) -> null
    reg.register_prim("unobserve", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_integer()) return PrimResult::fail();
        auto* obj = in[0].as_object();
        obj->remove_observer(static_cast<ObserverId>(in[1].as_integer()));
        return PrimResult::success(Value::null_val());
    });

    // observe-any: (object) -> observer_id
    reg.register_prim("observe-any", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail();
        auto* obj = in[0].as_object();
        ObserverId id = obj->add_any_observer([](const std::string&, const Value&, const Value&) {});
        return PrimResult::success(Value::integer(static_cast<int64_t>(id)));
    });

    // bind: (source_obj, source_attr, target_obj, target_attr) -> observer_id
    // When source.attr changes, target.attr is set to the new value.
    reg.register_prim("bind", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_string() ||
            !in[2].is_object() || !in[3].is_string()) return PrimResult::fail();
        auto* src = in[0].as_object();
        std::string src_attr = in[1].as_string()->str();
        // Capture target as a raw pointer (weak reference) — in a real impl we'd use Ref<>
        PhoObject* tgt = in[2].as_object();
        std::string tgt_attr = in[3].as_string()->str();
        ObserverId id = src->add_observer(src_attr,
            [tgt, tgt_attr](const std::string&, const Value&, const Value& new_val) {
                tgt->set_attr(tgt_attr, new_val);
            });
        return PrimResult::success(Value::integer(static_cast<int64_t>(id)));
    });

    // unbind: alias for unobserve
    reg.register_prim("unbind", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object() || !in[1].is_integer()) return PrimResult::fail();
        auto* obj = in[0].as_object();
        obj->remove_observer(static_cast<ObserverId>(in[1].as_integer()));
        return PrimResult::success(Value::null_val());
    });
}

} // namespace pho
