#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace pho {

// Input event types
enum class InputEventType : uint8_t {
    PointerDown,
    PointerUp,
    PointerMove,
    PointerDrag,
    KeyDown,
    KeyUp,
    Scroll,
    Resize,
    Timer,
    Custom,
};

struct InputEvent {
    InputEventType type = InputEventType::Custom;
    float x = 0, y = 0;          // pointer/scroll position
    float dx = 0, dy = 0;        // scroll delta or drag delta
    int32_t button = 0;          // mouse button (0=left, 1=right, 2=middle)
    uint32_t modifiers = 0;      // modifier flags
    std::string key;              // key string for key events
    uint32_t key_code = 0;       // platform key code
    std::string custom_type;     // for Custom events
    double timestamp = 0;

    // Modifier flags
    static constexpr uint32_t Shift   = 1 << 0;
    static constexpr uint32_t Control = 1 << 1;
    static constexpr uint32_t Alt     = 1 << 2;
    static constexpr uint32_t Meta    = 1 << 3; // Cmd on macOS
};

// Thread-safe event queue (like ioscpm's pattern)
class EventQueue {
public:
    void push(InputEvent event);
    bool pop(InputEvent& out);
    bool empty() const;
    size_t size() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::queue<InputEvent> queue_;
};

// Timer entry
struct TimerEntry {
    uint32_t id;
    double fire_time;       // absolute time when timer fires
    double interval;        // repeat interval (0 = one-shot)
    bool repeating;
    std::function<void()> callback;
};

// Engine run loop: manages event dispatch and timers
class EngineRunLoop {
public:
    EngineRunLoop();

    // Push an input event (thread-safe)
    void post_event(InputEvent event);

    // Process all pending events, fire ready timers. Called once per frame.
    // Returns number of events processed.
    uint32_t tick(double current_time);

    // Register an event handler
    using EventHandler = std::function<void(const InputEvent&)>;
    void set_event_handler(EventHandler handler);

    // Timer management
    uint32_t schedule_timer(double delay, double interval, bool repeating,
                            std::function<void()> callback);
    void cancel_timer(uint32_t timer_id);

    // Current time (set by tick)
    double current_time() const { return current_time_; }

private:
    EventQueue events_;
    EventHandler event_handler_;
    double current_time_ = 0;

    std::mutex timer_mutex_;
    std::vector<TimerEntry> timers_;
    uint32_t next_timer_id_ = 1;
};

// Global run loop management (for use by primitives)
void set_global_run_loop(EngineRunLoop* loop);
EngineRunLoop* get_global_run_loop();

// Convert InputEvent to a Value for use in dataflow graphs
class Value;
Value input_event_to_value(const InputEvent& event);

} // namespace pho
