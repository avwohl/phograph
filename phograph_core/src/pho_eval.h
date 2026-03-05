#pragma once
#include "pho_value.h"
#include "pho_graph.h"
#include "pho_prim.h"
#include "pho_debug.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

namespace pho {

// Evaluation outcome
enum class EvalStatus {
    Success,
    Failure,
    Error,
};

struct EvalResult {
    EvalStatus status = EvalStatus::Success;
    std::vector<Value> outputs;
    Value err_val;

    static EvalResult success(std::vector<Value> out) {
        return {EvalStatus::Success, std::move(out), Value::null_val()};
    }
    static EvalResult failure() {
        return {EvalStatus::Failure, {}, Value::null_val()};
    }
    static EvalResult make_error(const std::string& msg) {
        return {EvalStatus::Error, {}, Value::error(msg)};
    }
    static EvalResult make_error(Value err) {
        return {EvalStatus::Error, {}, std::move(err)};
    }
};

class Evaluator {
public:
    Evaluator();

    // Evaluate a method by name with given inputs
    EvalResult call_method(Project& project, const std::string& method_name,
                           const std::vector<Value>& inputs);

    // Evaluate a single case
    EvalResult eval_case(Project& project, const Case& c,
                         const std::vector<Value>& inputs);

    // Max evaluation steps (to prevent infinite loops)
    uint32_t max_steps = 100000;

    // Debugger integration
    void set_debugger(Debugger* d) { debugger_ = d; }

    // Phase 22: persistent store
    std::unordered_map<std::string, Value> persistent_store;

    // Phase 14: thread-local project/evaluator for HOF callbacks
    static thread_local Project* tl_project;
    static thread_local Evaluator* tl_evaluator;

    // Public eval_method for method-ref calls
    EvalResult eval_method(Project& project, const Method& method,
                           const std::vector<Value>& inputs);

private:
    Debugger* debugger_ = nullptr;
    // Per-node slot storage during case evaluation
    struct NodeSlots {
        std::vector<Value> inputs;
        std::vector<Value> outputs;
        uint32_t inputs_filled = 0;
        bool executed = false;
        bool execution_triggered = false;  // Phase 11
    };

    EvalResult exec_node(Project& project, const Case& c, const Node& node,
                         std::unordered_map<NodeId, NodeSlots>& slots);

    // Phase 15: loop evaluation
    EvalResult eval_loop(Project& project, const Case& c, const Node& node,
                         std::unordered_map<NodeId, NodeSlots>& slots);

    // Phase 20: expression evaluation
    Value eval_expression(const std::string& expr,
                         const std::unordered_map<std::string, Value>& bindings);
};

} // namespace pho
