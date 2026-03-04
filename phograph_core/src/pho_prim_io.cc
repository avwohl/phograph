#include "pho_prim.h"
#include "pho_platform.h"
#include "pho_thread.h"
#include <ctime>

namespace pho {

void register_io_prims() {
    auto& r = PrimitiveRegistry::instance();

    // file-read: path -> string (or fail)
    r.register_prim("file-read", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("file-read: expected string path"));
        const char* path = in[0].as_string()->c_str();
        size_t len = 0;
        char* data = pho_platform_read_file(path, &len);
        if (!data) return PrimResult::fail_with(Value::error("file-read: could not read file"));
        std::string content(data, len);
        pho_platform_free(data);
        return PrimResult::success(Value::string(std::move(content)));
    });

    // file-write: path content -> boolean
    r.register_prim("file-write", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("file-write: expected string path and content"));
        int rc = pho_platform_write_file(
            in[0].as_string()->c_str(),
            in[1].as_string()->c_str(),
            in[1].as_string()->length()
        );
        return PrimResult::success(Value::boolean(rc == 0));
    });

    // file-exists?: path -> boolean
    r.register_prim("file-exists?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("file-exists?: expected string"));
        return PrimResult::success(Value::boolean(pho_platform_file_exists(in[0].as_string()->c_str())));
    });

    // time-now: -> real (seconds since epoch)
    r.register_prim("time-now", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::real(pho_platform_time_now()));
    });

    // timer-schedule: delay -> timer-id
    r.register_prim("timer-schedule", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("timer-schedule: expected delay"));
        auto* loop = get_global_run_loop();
        if (!loop) return PrimResult::fail_with(Value::error("timer-schedule: no run loop"));
        double delay = in[0].as_number();
        uint32_t id = loop->schedule_timer(delay, 0, false, [loop]() {
            InputEvent evt;
            evt.type = InputEventType::Timer;
            loop->post_event(std::move(evt));
        });
        return PrimResult::success(Value::integer(id));
    });

    // timer-repeat: delay interval -> timer-id
    r.register_prim("timer-repeat", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric())
            return PrimResult::fail_with(Value::error("timer-repeat: expected delay and interval"));
        auto* loop = get_global_run_loop();
        if (!loop) return PrimResult::fail_with(Value::error("timer-repeat: no run loop"));
        double delay = in[0].as_number();
        double interval = in[1].as_number();
        uint32_t id = loop->schedule_timer(delay, interval, true, [loop]() {
            InputEvent evt;
            evt.type = InputEventType::Timer;
            loop->post_event(std::move(evt));
        });
        return PrimResult::success(Value::integer(id));
    });

    // timer-cancel: timer-id -> boolean
    r.register_prim("timer-cancel", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("timer-cancel: expected integer"));
        auto* loop = get_global_run_loop();
        if (!loop) return PrimResult::fail_with(Value::error("timer-cancel: no run loop"));
        loop->cancel_timer(static_cast<uint32_t>(in[0].as_integer()));
        return PrimResult::success(Value::boolean(true));
    });

    // clipboard-get: -> string (or fail)
    r.register_prim("clipboard-get", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        char* text = pho_platform_clipboard_get();
        if (!text) return PrimResult::fail();
        std::string result(text);
        pho_platform_free(text);
        return PrimResult::success(Value::string(std::move(result)));
    });

    // clipboard-set: text -> boolean
    r.register_prim("clipboard-set", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("clipboard-set: expected string"));
        pho_platform_clipboard_set(in[0].as_string()->c_str());
        return PrimResult::success(Value::boolean(true));
    });

    // platform-name: -> string
    r.register_prim("platform-name", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        const char* name = pho_platform_name();
        return PrimResult::success(Value::string(name ? name : "unknown"));
    });

    // log: value -> value (pass-through, prints to platform log)
    // Already registered in pho_prim_error.cc, skip duplicate
}

} // namespace pho
