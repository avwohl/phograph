#include "pho_prim.h"
#include "pho_prim_wrappers.h"
#include "pho_thread.h"

namespace pho {

// Global run loop accessible to primitives
static EngineRunLoop* g_run_loop = nullptr;

void set_global_run_loop(EngineRunLoop* loop) {
    g_run_loop = loop;
}

EngineRunLoop* get_global_run_loop() {
    return g_run_loop;
}

void register_input_prims() {
    auto& r = PrimitiveRegistry::instance();

    // input-pointer-position: -> list[x, y]
    // Returns last known pointer position from the run loop.
    // In practice, the host sets this via event dispatch.
    r.register_prim("input-pointer-position", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        // Placeholder - actual implementation depends on event state tracking
        return PrimResult::success(Value::list({Value::real(0), Value::real(0)}));
    });

    // input-event-type: event -> string
    r.register_prim("input-event-type", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        auto type_val = in[0].as_object()->get_attr("type");
        if (type_val.is_string()) return PrimResult::success(type_val);
        return PrimResult::success(Value::string("unknown"));
    });

    // input-event-x: event -> real
    r.register_prim("input-event-x", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        return PrimResult::success(in[0].as_object()->get_attr("x"));
    });

    // input-event-y: event -> real
    r.register_prim("input-event-y", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        return PrimResult::success(in[0].as_object()->get_attr("y"));
    });

    // input-event-key: event -> string
    r.register_prim("input-event-key", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        return PrimResult::success(in[0].as_object()->get_attr("key"));
    });

    // input-modifier-shift?: event -> boolean
    r.register_prim("input-modifier-shift?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        auto mods = in[0].as_object()->get_attr("modifiers");
        if (mods.is_integer())
            return PrimResult::success(Value::boolean((mods.as_integer() & InputEvent::Shift) != 0));
        return PrimResult::success(Value::boolean(false));
    });

    // input-modifier-meta?: event -> boolean
    r.register_prim("input-modifier-meta?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_object()) return PrimResult::fail_with(Value::error("expected event object"));
        auto mods = in[0].as_object()->get_attr("modifiers");
        if (mods.is_integer())
            return PrimResult::success(Value::boolean((mods.as_integer() & InputEvent::Meta) != 0));
        return PrimResult::success(Value::boolean(false));
    });
}

// Convert an InputEvent to a Value (PhoObject) for use in dataflow graphs
Value input_event_to_value(const InputEvent& event) {
    auto obj = make_ref<PhoObject>("InputEvent");

    const char* type_str = "custom";
    switch (event.type) {
        case InputEventType::PointerDown: type_str = "pointer-down"; break;
        case InputEventType::PointerUp:   type_str = "pointer-up"; break;
        case InputEventType::PointerMove: type_str = "pointer-move"; break;
        case InputEventType::PointerDrag: type_str = "pointer-drag"; break;
        case InputEventType::KeyDown:     type_str = "key-down"; break;
        case InputEventType::KeyUp:       type_str = "key-up"; break;
        case InputEventType::Scroll:      type_str = "scroll"; break;
        case InputEventType::Resize:      type_str = "resize"; break;
        case InputEventType::Timer:       type_str = "timer"; break;
        case InputEventType::Custom:      type_str = event.custom_type.c_str(); break;
    }

    obj->set_attr("type", Value::string(type_str));
    obj->set_attr("x", Value::real(event.x));
    obj->set_attr("y", Value::real(event.y));
    obj->set_attr("dx", Value::real(event.dx));
    obj->set_attr("dy", Value::real(event.dy));
    obj->set_attr("button", Value::integer(event.button));
    obj->set_attr("modifiers", Value::integer(event.modifiers));
    obj->set_attr("key", Value::string(event.key));
    obj->set_attr("timestamp", Value::real(event.timestamp));

    return Value::object(std::move(obj));
}

} // namespace pho
