#include "pho_debug.h"
#include <algorithm>

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
    if (step_mode_) return true;
    return breakpoint_nodes_.count(node_id) > 0;
}

void Debugger::step_completed(NodeId node_id) {
    step_count_++;

    // Handle step actions first to update step_mode_
    switch (pending_action_) {
        case DebugAction::StepOver:
            // After next node executes, pause
            if (step_mode_) {
                // We're completing the step - pause now
                step_mode_ = false;
                paused_ = true;
                pending_action_ = DebugAction::Continue;
                if (on_breakpoint_hit_) on_breakpoint_hit_(node_id, step_count_);
                return;
            }
            step_mode_ = true;
            break;
        case DebugAction::Continue:
            step_mode_ = false;
            break;
        default:
            break;
    }

    // Check breakpoints
    if (breakpoint_nodes_.count(node_id) > 0) {
        paused_ = true;
        if (on_breakpoint_hit_) on_breakpoint_hit_(node_id, step_count_);
    }
}

void Debugger::reset() {
    clear_traces();
    snapshots_.clear();
    pending_action_ = DebugAction::Continue;
    paused_ = false;
    step_mode_ = false;
}

} // namespace pho
