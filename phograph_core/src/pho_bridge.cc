#include "pho_bridge.h"
#include "pho_value.h"
#include "pho_graph.h"
#include "pho_eval.h"
#include "pho_prim.h"
#include "pho_serial.h"
#include "pho_thread.h"
#include "pho_debug.h"
#include "pho_codegen.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <thread>

// Thread-local pointer for console capture
static thread_local std::string* tl_console = nullptr;

// Thread-local pointers for display buffer (canvas-render → Metal view)
static thread_local std::vector<uint8_t>* tl_display = nullptr;
static thread_local int32_t* tl_display_w = nullptr;
static thread_local int32_t* tl_display_h = nullptr;

namespace pho {
void pho_console_write(const std::string& text) {
    if (tl_console) tl_console->append(text);
}

void pho_display_blit(const uint8_t* data, int32_t width, int32_t height) {
    if (!tl_display || !data || width <= 0 || height <= 0) return;
    size_t size = static_cast<size_t>(width) * height * 4;
    tl_display->resize(size);
    memcpy(tl_display->data(), data, size);
    *tl_display_w = width;
    *tl_display_h = height;
}
}

struct PhoEngine {
    pho::Project project;
    pho::Evaluator evaluator;
    std::string last_error;
    pho::EngineRunLoop run_loop;
    std::string console_buffer;

    // Debugger
    pho::Debugger debugger;
    PhoDebugCallback debug_cb = nullptr;
    void* debug_ctx = nullptr;
    std::thread debug_thread;

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

    // Capture console output and display buffer during evaluation
    engine->console_buffer.clear();
    tl_console = &engine->console_buffer;
    tl_display = &engine->pixel_buffer;
    tl_display_w = &engine->buffer_width;
    tl_display_h = &engine->buffer_height;

    auto result = engine->evaluator.call_method(engine->project, method_name, inputs);

    tl_console = nullptr;
    tl_display = nullptr;
    tl_display_w = nullptr;
    tl_display_h = nullptr;

    // Encode result as JSON
    std::string out = "{\"status\":";
    switch (result.status) {
        case pho::EvalStatus::Success: out += "\"success\""; break;
        case pho::EvalStatus::Failure: out += "\"failure\""; break;
        case pho::EvalStatus::Error: out += "\"error\""; break;
    }

    // Include error message when status is error or failure
    if (result.status != pho::EvalStatus::Success) {
        std::string msg;
        if (result.status == pho::EvalStatus::Error && !result.err_val.is_null()) {
            msg = result.err_val.to_display_string();
        } else if (result.status == pho::EvalStatus::Failure) {
            msg = "method failed (all cases exhausted)";
        }
        if (!msg.empty()) {
            out += ",\"error\":\"";
            for (char ch : msg) {
                switch (ch) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += ch; break;
                }
            }
            out += "\"";
        }
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
            case pho::ValueTag::String: {
                // Escape string for JSON
                out += "\"";
                for (char ch : v.as_string()->str()) {
                    switch (ch) {
                        case '"': out += "\\\""; break;
                        case '\\': out += "\\\\"; break;
                        case '\n': out += "\\n"; break;
                        case '\r': out += "\\r"; break;
                        case '\t': out += "\\t"; break;
                        default: out += ch; break;
                    }
                }
                out += "\"";
                break;
            }
            default: out += "\"" + v.to_display_string() + "\""; break;
        }
    }
    out += "]";

    // Include console output
    if (!engine->console_buffer.empty()) {
        out += ",\"console\":\"";
        for (char ch : engine->console_buffer) {
            switch (ch) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += ch; break;
            }
        }
        out += "\"";
    }

    out += "}";

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

// ---- Console ----

const char* pho_engine_get_console(PhoEngineRef engine) {
    return engine->console_buffer.c_str();
}

void pho_engine_clear_console(PhoEngineRef engine) {
    engine->console_buffer.clear();
}

// ---- Debug API ----

void pho_engine_debug_set_callback(PhoEngineRef engine, PhoDebugCallback cb, void* ctx) {
    engine->debug_cb = cb;
    engine->debug_ctx = ctx;
}

void pho_engine_debug_run(PhoEngineRef engine, const char* method_name) {
    // Stop any existing debug session
    if (engine->debug_thread.joinable()) {
        engine->debugger.request_stop();
        engine->debug_thread.join();
    }

    engine->debugger.reset();
    engine->debugger.enable_tracing(true);
    engine->evaluator.set_debugger(&engine->debugger);

    // Set up pause callback that fires debug events
    engine->debugger.set_breakpoint_hit_callback([engine](pho::NodeId node_id, uint32_t step) {
        if (engine->debug_cb) {
            // Build JSON event
            std::string json = "{\"event\":\"paused\",\"node_id\":";
            json += std::to_string(node_id);
            json += ",\"step\":";
            json += std::to_string(step);

            // Include recent trace entries
            json += ",\"traces\":[";
            auto& traces = engine->debugger.traces();
            size_t start = traces.size() > 20 ? traces.size() - 20 : 0;
            for (size_t i = start; i < traces.size(); i++) {
                if (i > start) json += ",";
                auto& t = traces[i];
                json += "{\"src\":";
                json += std::to_string(t.source_node);
                json += ",\"sp\":";
                json += std::to_string(t.source_pin);
                json += ",\"dst\":";
                json += std::to_string(t.dest_node);
                json += ",\"dp\":";
                json += std::to_string(t.dest_pin);
                json += ",\"val\":\"";
                json += t.value.to_display_string();
                json += "\",\"step\":";
                json += std::to_string(t.step_number);
                json += "}";
            }
            json += "]}";

            engine->debug_cb(engine->debug_ctx, json.c_str());
        }
    });

    std::string mname(method_name);
    engine->debug_thread = std::thread([engine, mname]() {
        engine->console_buffer.clear();
        tl_console = &engine->console_buffer;
        tl_display = &engine->pixel_buffer;
        tl_display_w = &engine->buffer_width;
        tl_display_h = &engine->buffer_height;

        auto result = engine->evaluator.call_method(engine->project, mname, {});

        tl_console = nullptr;
        tl_display = nullptr;
        tl_display_w = nullptr;
        tl_display_h = nullptr;
        engine->evaluator.set_debugger(nullptr);

        // Fire completed event
        if (engine->debug_cb) {
            std::string json = "{\"event\":\"completed\",\"status\":\"";
            switch (result.status) {
                case pho::EvalStatus::Success: json += "success"; break;
                case pho::EvalStatus::Failure: json += "failure"; break;
                case pho::EvalStatus::Error: json += "error"; break;
            }
            json += "\"";
            // Include error message
            if (result.status != pho::EvalStatus::Success) {
                std::string msg;
                if (result.status == pho::EvalStatus::Error && !result.err_val.is_null()) {
                    msg = result.err_val.to_display_string();
                } else if (result.status == pho::EvalStatus::Failure) {
                    msg = "method failed (all cases exhausted)";
                }
                if (!msg.empty()) {
                    json += ",\"error\":\"";
                    for (char ch : msg) {
                        switch (ch) {
                            case '"': json += "\\\""; break;
                            case '\\': json += "\\\\"; break;
                            case '\n': json += "\\n"; break;
                            case '\r': json += "\\r"; break;
                            case '\t': json += "\\t"; break;
                            default: json += ch; break;
                        }
                    }
                    json += "\"";
                }
            }
            json += "}";
            engine->debug_cb(engine->debug_ctx, json.c_str());
        }
    });
}

void pho_engine_debug_continue(PhoEngineRef engine) {
    engine->debugger.set_action(pho::DebugAction::Continue);
    engine->debugger.signal_resume();
}

void pho_engine_debug_step_over(PhoEngineRef engine) {
    engine->debugger.set_action(pho::DebugAction::StepOver);
    engine->debugger.signal_resume();
}

void pho_engine_debug_step_into(PhoEngineRef engine) {
    engine->debugger.set_action(pho::DebugAction::StepInto);
    engine->debugger.signal_resume();
}

void pho_engine_debug_stop(PhoEngineRef engine) {
    engine->debugger.request_stop();
    if (engine->debug_thread.joinable()) {
        engine->debug_thread.join();
    }
    engine->evaluator.set_debugger(nullptr);
}

void pho_engine_debug_add_breakpoint(PhoEngineRef engine, uint32_t node_id,
                                      const char* method, int case_idx) {
    engine->debugger.add_breakpoint(node_id, method ? method : "", case_idx);
}

void pho_engine_debug_remove_breakpoint(PhoEngineRef engine, uint32_t node_id) {
    engine->debugger.remove_breakpoint(node_id);
}

// ---- Codegen / Compile ----

const char* pho_engine_compile(PhoEngineRef engine, const char* entry_method, int emit_main) {
    pho::CodegenOptions opts;
    opts.emit_imports = true;
    opts.emit_runtime = false;  // runtime is provided as a separate file
    opts.emit_main = (emit_main != 0);
    if (entry_method) opts.entry_method = entry_method;

    pho::SwiftCodegen codegen(opts);
    std::string source;
    std::vector<pho::CodegenError> errors;
    codegen.compile(engine->project, source, errors);

    if (!errors.empty()) {
        std::string err_msg;
        for (auto& e : errors) {
            if (!err_msg.empty()) err_msg += "\n";
            err_msg += e.context + ": " + e.message;
        }
        engine->last_error = err_msg;
    }

    char* result = static_cast<char*>(malloc(source.size() + 1));
    memcpy(result, source.c_str(), source.size() + 1);
    return result;
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
