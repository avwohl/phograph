#include "pho_bridge.h"
#include "pho_value.h"
#include "pho_graph.h"
#include "pho_eval.h"
#include "pho_prim.h"
#include "pho_serial.h"
#include "pho_thread.h"
#include <string>
#include <cstring>
#include <cstdlib>

struct PhoEngine {
    pho::Project project;
    pho::Evaluator evaluator;
    std::string last_error;
    pho::EngineRunLoop run_loop;

    // Pixel buffer (Phase 5+)
    std::vector<uint8_t> pixel_buffer;
    int32_t buffer_width = 0;
    int32_t buffer_height = 0;
};

extern "C" {

PhoEngineRef pho_engine_create(void) {
    auto* engine = new PhoEngine();
    pho::set_global_run_loop(&engine->run_loop);
    return engine;
}

void pho_engine_destroy(PhoEngineRef engine) {
    delete engine;
}

void pho_engine_init_prims(void) {
    pho::register_all_prims();
}

int pho_engine_load_json(PhoEngineRef engine, const char* json, size_t json_len) {
    std::string json_str(json, json_len);
    std::string error;
    engine->project = pho::Project{}; // reset
    if (!pho::load_project_from_json(json_str, engine->project, error)) {
        engine->last_error = error;
        return -1;
    }
    return 0;
}

const char* pho_engine_call_method(PhoEngineRef engine, const char* method_name,
                                     const char* inputs_json, size_t inputs_len) {
    // Parse inputs from JSON
    std::vector<pho::Value> inputs;
    if (inputs_json && inputs_len > 0) {
        std::string json_str(inputs_json, inputs_len);
        pho::JsonValue jv;
        std::string error;
        if (pho::parse_json(json_str, jv, error) && jv.is_array()) {
            for (auto& elem : jv.array) {
                if (elem.type == pho::JsonType::Number) {
                    double n = elem.number;
                    if (n == std::floor(n) && n >= -9007199254740992.0 && n <= 9007199254740992.0)
                        inputs.push_back(pho::Value::integer(static_cast<int64_t>(n)));
                    else
                        inputs.push_back(pho::Value::real(n));
                } else if (elem.type == pho::JsonType::String) {
                    inputs.push_back(pho::Value::string(elem.str));
                } else if (elem.type == pho::JsonType::Boolean) {
                    inputs.push_back(pho::Value::boolean(elem.boolean));
                } else {
                    inputs.push_back(pho::Value::null_val());
                }
            }
        }
    }

    auto result = engine->evaluator.call_method(engine->project, method_name, inputs);

    // Encode result as JSON
    std::string out = "{\"status\":";
    switch (result.status) {
        case pho::EvalStatus::Success: out += "\"success\""; break;
        case pho::EvalStatus::Failure: out += "\"failure\""; break;
        case pho::EvalStatus::Error: out += "\"error\""; break;
    }

    out += ",\"outputs\":[";
    for (size_t i = 0; i < result.outputs.size(); i++) {
        if (i > 0) out += ",";
        auto& v = result.outputs[i];
        switch (v.tag()) {
            case pho::ValueTag::Null: out += "null"; break;
            case pho::ValueTag::Integer: out += std::to_string(v.as_integer()); break;
            case pho::ValueTag::Real: {
                char buf[64];
                snprintf(buf, sizeof(buf), "%.17g", v.as_real());
                out += buf;
                break;
            }
            case pho::ValueTag::Boolean: out += v.as_boolean() ? "true" : "false"; break;
            case pho::ValueTag::String: out += "\"" + v.as_string()->str() + "\""; break;
            default: out += "\"" + v.to_display_string() + "\""; break;
        }
    }
    out += "]}";

    char* result_str = static_cast<char*>(malloc(out.size() + 1));
    memcpy(result_str, out.c_str(), out.size() + 1);
    return result_str;
}

const char* pho_engine_last_error(PhoEngineRef engine) {
    return engine->last_error.c_str();
}

void pho_engine_free_string(const char* str) {
    free(const_cast<char*>(str));
}

const uint8_t* pho_engine_pixel_buffer(PhoEngineRef engine, int32_t* out_width, int32_t* out_height) {
    *out_width = engine->buffer_width;
    *out_height = engine->buffer_height;
    if (engine->pixel_buffer.empty()) return nullptr;
    return engine->pixel_buffer.data();
}

void pho_engine_resize(PhoEngineRef engine, int32_t width, int32_t height) {
    engine->buffer_width = width;
    engine->buffer_height = height;
    engine->pixel_buffer.resize(width * height * 4, 0);
}

void pho_engine_tick(PhoEngineRef engine, double dt) {
    engine->run_loop.tick(engine->run_loop.current_time() + dt);
}

// ---- Input events ----

static void post_input(PhoEngineRef engine, pho::InputEventType type,
                        float x, float y, float dx = 0, float dy = 0,
                        int32_t button = 0, uint32_t modifiers = 0,
                        const char* key = nullptr, uint32_t key_code = 0) {
    pho::InputEvent evt;
    evt.type = type;
    evt.x = x; evt.y = y;
    evt.dx = dx; evt.dy = dy;
    evt.button = button;
    evt.modifiers = modifiers;
    if (key) evt.key = key;
    evt.key_code = key_code;
    evt.timestamp = engine->run_loop.current_time();
    engine->run_loop.post_event(std::move(evt));
}

void pho_engine_send_pointer_down(PhoEngineRef engine, float x, float y, int32_t button) {
    post_input(engine, pho::InputEventType::PointerDown, x, y, 0, 0, button);
}

void pho_engine_send_pointer_up(PhoEngineRef engine, float x, float y, int32_t button) {
    post_input(engine, pho::InputEventType::PointerUp, x, y, 0, 0, button);
}

void pho_engine_send_pointer_move(PhoEngineRef engine, float x, float y) {
    post_input(engine, pho::InputEventType::PointerMove, x, y);
}

void pho_engine_send_pointer_drag(PhoEngineRef engine, float x, float y, float dx, float dy) {
    post_input(engine, pho::InputEventType::PointerDrag, x, y, dx, dy);
}

void pho_engine_send_scroll(PhoEngineRef engine, float x, float y, float dx, float dy) {
    post_input(engine, pho::InputEventType::Scroll, x, y, dx, dy);
}

void pho_engine_send_key_down(PhoEngineRef engine, const char* key, uint32_t key_code, uint32_t modifiers) {
    post_input(engine, pho::InputEventType::KeyDown, 0, 0, 0, 0, 0, modifiers, key, key_code);
}

void pho_engine_send_key_up(PhoEngineRef engine, const char* key, uint32_t key_code, uint32_t modifiers) {
    post_input(engine, pho::InputEventType::KeyUp, 0, 0, 0, 0, 0, modifiers, key, key_code);
}

} // extern "C"
