#pragma once
#include "pho_value.h"
#include "pho_graph.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace pho {

// A trace record for a single wire value during evaluation
struct TraceEntry {
    NodeId source_node;
    uint32_t source_pin;
    NodeId dest_node;
    uint32_t dest_pin;
    Value value;
    uint32_t step_number;
};

// Breakpoint on a specific node
struct Breakpoint {
    NodeId node_id;
    std::string method_name;
    int case_index;
    bool enabled = true;
};

// Execution state snapshot for rollback
struct ExecutionSnapshot {
    uint32_t step_number;
    std::string method_name;
    int case_index;
    std::vector<TraceEntry> traces;
    std::vector<Value> outputs;
    bool completed;
};

enum class DebugAction {
    Continue,
    StepOver,  // Execute next node
    StepInto,  // Enter method call
    StepOut,   // Finish current method
    Rollback,  // Go back to previous snapshot
    Stop,
};

// Callback when debugger hits a breakpoint
using BreakpointHitFn = std::function<void(NodeId node, uint32_t step)>;
// Callback when a wire value is recorded
using TraceRecordFn = std::function<void(const TraceEntry& entry)>;

class Debugger {
public:
    Debugger();

    // Breakpoint management
    void add_breakpoint(NodeId node_id, const std::string& method_name, int case_index);
    void remove_breakpoint(NodeId node_id);
    void toggle_breakpoint(NodeId node_id);
    void clear_breakpoints();
    bool has_breakpoint(NodeId node_id) const;
    const std::vector<Breakpoint>& breakpoints() const { return breakpoints_; }

    // Trace management
    void enable_tracing(bool enabled) { tracing_enabled_ = enabled; }
    bool is_tracing() const { return tracing_enabled_; }
    void record_trace(const TraceEntry& entry);
    const std::vector<TraceEntry>& traces() const { return traces_; }
    void clear_traces();

    // Get trace values for a specific node's outputs
    std::vector<TraceEntry> traces_for_node(NodeId node_id) const;

    // Get the value on a specific wire
    Value wire_value(NodeId source, uint32_t source_pin, NodeId dest, uint32_t dest_pin) const;

    // Snapshot management for rollback
    void save_snapshot(const std::string& method_name, int case_index,
                       const std::vector<Value>& outputs, bool completed);
    bool rollback();
    const std::vector<ExecutionSnapshot>& snapshots() const { return snapshots_; }

    // Step control
    void set_action(DebugAction action) { pending_action_ = action; }
    DebugAction pending_action() const { return pending_action_; }
    bool should_break_at(NodeId node_id) const;
    void step_completed(NodeId node_id);

    // Callbacks
    void set_breakpoint_hit_callback(BreakpointHitFn fn) { on_breakpoint_hit_ = std::move(fn); }
    void set_trace_record_callback(TraceRecordFn fn) { on_trace_record_ = std::move(fn); }

    // Synchronization for blocking eval thread
    void wait_for_resume();   // blocks eval thread until resumed or stopped
    void signal_resume();     // wakes up blocked eval thread
    void request_stop();      // sets stop flag + signals
    bool stop_requested() const { return stop_requested_; }

    // State
    uint32_t current_step() const { return step_count_; }
    bool is_paused() const { return paused_; }
    void set_paused(bool p) { paused_ = p; }
    void reset();

private:
    std::vector<Breakpoint> breakpoints_;
    std::unordered_set<NodeId> breakpoint_nodes_;
    std::vector<TraceEntry> traces_;
    std::vector<ExecutionSnapshot> snapshots_;
    BreakpointHitFn on_breakpoint_hit_;
    TraceRecordFn on_trace_record_;
    DebugAction pending_action_ = DebugAction::Continue;
    uint32_t step_count_ = 0;
    bool tracing_enabled_ = true;
    bool paused_ = false;
    bool step_mode_ = false; // true when stepping one node at a time
    bool stop_requested_ = false;
    int call_depth_ = 0;      // for StepOver vs StepInto
    int step_over_depth_ = 0; // call depth when StepOver was issued
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace pho
