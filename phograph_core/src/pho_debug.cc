#include "pho_debug.h"
#include <algorithm>
#include <mutex>

namespace pho {

Debugger::Debugger() = default;

void Debugger::add_breakpoint(NodeId node_id, const std::string& method_name, int case_index) {
    // Check for duplicate
    for (auto& bp : breakpoints_) {
        if (bp.node_id == node_id) {
            bp.enabled = true;
            breakpoint_nodes_.insert(node_id);
            return;
        }
    }
    breakpoints_.push_back({node_id, method_name, case_index, true});
    breakpoint_nodes_.insert(node_id);
}

void Debugger::remove_breakpoint(NodeId node_id) {
    breakpoints_.erase(
        std::remove_if(breakpoints_.begin(), breakpoints_.end(),
                        [node_id](const Breakpoint& bp) { return bp.node_id == node_id; }),
        breakpoints_.end());
    breakpoint_nodes_.erase(node_id);
}

void Debugger::toggle_breakpoint(NodeId node_id) {
    for (auto& bp : breakpoints_) {
        if (bp.node_id == node_id) {
            bp.enabled = !bp.enabled;
            if (bp.enabled)
                breakpoint_nodes_.insert(node_id);
            else
                breakpoint_nodes_.erase(node_id);
            return;
        }
    }
}

void Debugger::clear_breakpoints() {
    breakpoints_.clear();
    breakpoint_nodes_.clear();
}

bool Debugger::has_breakpoint(NodeId node_id) const {
    return breakpoint_nodes_.count(node_id) > 0;
}

void Debugger::record_trace(const TraceEntry& entry) {
    traces_.push_back(entry);
    if (on_trace_record_) on_trace_record_(entry);
}

void Debugger::clear_traces() {
    traces_.clear();
    step_count_ = 0;
}

std::vector<TraceEntry> Debugger::traces_for_node(NodeId node_id) const {
    std::vector<TraceEntry> result;
    for (auto& t : traces_) {
        if (t.source_node == node_id) result.push_back(t);
    }
    return result;
}

Value Debugger::wire_value(NodeId source, uint32_t source_pin,
                            NodeId dest, uint32_t dest_pin) const {
    // Return the most recent trace value for this wire
    for (auto it = traces_.rbegin(); it != traces_.rend(); ++it) {
        if (it->source_node == source && it->source_pin == source_pin &&
            it->dest_node == dest && it->dest_pin == dest_pin) {
            return it->value;
        }
    }
    return Value::null_val();
}

void Debugger::save_snapshot(const std::string& method_name, int case_index,
                              const std::vector<Value>& outputs, bool completed) {
    ExecutionSnapshot snap;
    snap.step_number = step_count_;
    snap.method_name = method_name;
    snap.case_index = case_index;
    snap.traces = traces_; // copy all traces up to this point
    snap.outputs = outputs;
    snap.completed = completed;
    snapshots_.push_back(std::move(snap));
}

bool Debugger::rollback() {
    if (snapshots_.size() < 2) return false;
    snapshots_.pop_back(); // remove current
    auto& prev = snapshots_.back();
    traces_ = prev.traces;
    step_count_ = prev.step_number;
    return true;
}

bool Debugger::should_break_at(NodeId node_id) const {
    if (step_mode_) {
        // StepOver: only break at same or shallower call depth
        if (pending_action_ == DebugAction::StepOver && call_depth_ > step_over_depth_) {
            return false;
        }
        return true;
    }
    return breakpoint_nodes_.count(node_id) > 0;
}

void Debugger::step_completed(NodeId node_id) {
    step_count_++;

    if (stop_requested_) return;

    bool should_pause = false;

    // Handle step actions
    switch (pending_action_) {
        case DebugAction::StepOver:
            if (step_mode_) {
                // Only pause if we're at the same or shallower depth
                if (call_depth_ <= step_over_depth_) {
                    step_mode_ = false;
                    should_pause = true;
                    pending_action_ = DebugAction::Continue;
                }
            } else {
                step_mode_ = true;
                step_over_depth_ = call_depth_;
            }
            break;
        case DebugAction::StepInto:
            if (step_mode_) {
                step_mode_ = false;
                should_pause = true;
                pending_action_ = DebugAction::Continue;
            } else {
                step_mode_ = true;
            }
            break;
        case DebugAction::Continue:
            step_mode_ = false;
            break;
        default:
            break;
    }

    // Check breakpoints
    if (!should_pause && breakpoint_nodes_.count(node_id) > 0) {
        should_pause = true;
    }

    if (should_pause) {
        paused_ = true;
        if (on_breakpoint_hit_) on_breakpoint_hit_(node_id, step_count_);
        wait_for_resume();
    }
}

void Debugger::wait_for_resume() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !paused_ || stop_requested_; });
}

void Debugger::signal_resume() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        paused_ = false;
    }
    cv_.notify_one();
}

void Debugger::request_stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_requested_ = true;
        paused_ = false;
    }
    cv_.notify_one();
}

void Debugger::reset() {
    clear_traces();
    snapshots_.clear();
    pending_action_ = DebugAction::Continue;
    paused_ = false;
    step_mode_ = false;
    stop_requested_ = false;
    call_depth_ = 0;
    step_over_depth_ = 0;
}

} // namespace pho
