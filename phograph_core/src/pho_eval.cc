#include "pho_eval.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <sstream>

namespace pho {

// Phase 14: thread-local pointers for HOF callbacks
thread_local Project* Evaluator::tl_project = nullptr;
thread_local Evaluator* Evaluator::tl_evaluator = nullptr;

Evaluator::Evaluator() {}

EvalResult Evaluator::eval_method(Project& project, const Method& method,
                                   const std::vector<Value>& inputs) {
    for (auto& c : method.cases) {
        // Phase 23: check case guards
        bool guards_pass = true;
        for (auto& guard : c.guards) {
            if (guard.pin >= inputs.size()) { guards_pass = false; break; }
            const Value& v = inputs[guard.pin];
            switch (guard.kind) {
                case CaseGuard::TypeMatch:
                    if (tag_name(v.tag()) != guard.match_type) guards_pass = false;
                    break;
                case CaseGuard::ValueMatch:
                    if (!v.equals(guard.match_val)) guards_pass = false;
                    break;
                case CaseGuard::Wildcard:
                    break;
            }
            if (!guards_pass) break;
        }
        if (!guards_pass) continue;

        auto result = eval_case(project, c, inputs);
        if (result.status == EvalStatus::Failure) continue;
        return result;
    }
    return EvalResult::failure();
}

EvalResult Evaluator::call_method(Project& project, const std::string& method_name,
                                   const std::vector<Value>& inputs) {
    const Method* method = project.find_method(method_name);
    if (!method) {
        return EvalResult::make_error("method not found: " + method_name);
    }
    return eval_method(project, *method, inputs);
}

EvalResult Evaluator::eval_case(Project& project, const Case& c,
                                 const std::vector<Value>& inputs) {
    // Set thread-local for HOF callbacks
    tl_project = &project;
    tl_evaluator = this;

    // Initialize slot storage for each node
    std::unordered_map<NodeId, NodeSlots> slots;
    for (auto& node : c.nodes) {
        NodeSlots ns;
        ns.inputs.resize(node.num_inputs);
        ns.outputs.resize(node.num_outputs);
        slots[node.id] = std::move(ns);
    }

    // Phase 18: initialize shift registers
    for (auto& sr : c.shift_registers) {
        // Shift registers are initialized before first iteration
        // (handled in loop evaluation)
    }

    // Place inputs on input bar's outputs
    const Node* input_bar = c.find_node(c.input_bar_id);
    if (input_bar) {
        auto& bar_slots = slots[input_bar->id];
        bar_slots.executed = true;
        for (size_t i = 0; i < inputs.size() && i < bar_slots.outputs.size(); i++) {
            bar_slots.outputs[i] = inputs[i];
        }
    }

    // Propagate input bar outputs through wires
    if (input_bar) {
        auto out_wires = c.output_wires(input_bar->id);
        for (auto* w : out_wires) {
            if (w->is_execution) continue;  // Phase 11: skip execution wires for data
            auto& target_slots = slots[w->target.node_id];
            target_slots.inputs[w->target.index] = slots[input_bar->id].outputs[w->source.index];
            target_slots.inputs_filled++;
        }
    }

    // Phase 11: identify nodes with execution inputs
    std::unordered_map<NodeId, bool> has_exec_in;
    for (auto& w : c.wires) {
        if (w.is_execution) {
            has_exec_in[w.target.node_id] = true;
        }
    }

    // Phase 12: compute required input count per node
    auto num_required_inputs = [](const Node& node) -> uint32_t {
        if (!node.input_defs.empty()) {
            uint32_t req = 0;
            for (auto& pd : node.input_defs) {
                if (!pd.is_optional) req++;
            }
            return req;
        }
        return node.num_inputs;
    };

    // Check if a node is ready to fire
    auto is_ready = [&](const Node& node, const NodeSlots& ns) -> bool {
        if (ns.executed) return false;
        if (node.type == NodeType::Constant) return true;
        if (node.num_inputs == 0 && (node.type == NodeType::Primitive ||
            node.type == NodeType::InstanceGenerator)) return true;

        uint32_t required = num_required_inputs(node);
        if (node.num_inputs > 0 && ns.inputs_filled < required) return false;

        // Phase 11: nodes with execution inputs also require trigger
        if (has_exec_in.count(node.id) && !ns.execution_triggered) return false;

        return true;
    };

    // Build ready queue
    std::queue<NodeId> ready;
    for (auto& node : c.nodes) {
        if (node.id == c.input_bar_id) continue;
        if (node.id == c.output_bar_id && node.num_inputs == 0) continue;
        if (is_ready(node, slots[node.id])) {
            ready.push(node.id);
        }
    }

    // Process ready queue
    uint32_t steps = 0;

    while (!ready.empty() && steps < max_steps) {
        NodeId nid = ready.front();
        ready.pop();

        auto& ns = slots[nid];
        if (ns.executed) continue;

        const Node* node = c.find_node(nid);
        if (!node) continue;

        // Phase 12: fill optional defaults before firing
        if (!node->input_defs.empty()) {
            for (size_t i = 0; i < node->input_defs.size() && i < ns.inputs.size(); i++) {
                if (node->input_defs[i].is_optional && ns.inputs[i].is_null()) {
                    // Check if there's actually a wire connected
                    bool has_wire = false;
                    for (auto& w : c.wires) {
                        if (!w.is_execution && w.target.node_id == nid && w.target.index == static_cast<uint32_t>(i)) {
                            has_wire = true;
                            break;
                        }
                    }
                    if (!has_wire) {
                        ns.inputs[i] = node->input_defs[i].default_value;
                    }
                }
            }
        }

        // Phase 16: list map / partition annotation
        EvalResult result = {EvalStatus::Success, {}, Value::null_val()};
        if (node->list_map || node->partition) {
            // Find the list input(s)
            size_t list_len = 0;
            std::vector<bool> is_list_input(ns.inputs.size(), false);
            for (size_t i = 0; i < ns.inputs.size(); i++) {
                if (ns.inputs[i].is_list()) {
                    is_list_input[i] = true;
                    size_t len = ns.inputs[i].as_list()->size();
                    if (list_len == 0) list_len = len;
                    else list_len = std::min(list_len, len);
                }
            }

            if (list_len == 0) {
                // No list inputs or empty lists
                result = exec_node(project, c, *node, slots);
            } else if (node->partition) {
                // Partition: split into pass/fail lists
                std::vector<Value> pass_list, fail_list;
                for (size_t li = 0; li < list_len; li++) {
                    // Build element inputs
                    auto saved_inputs = ns.inputs;
                    for (size_t i = 0; i < ns.inputs.size(); i++) {
                        if (is_list_input[i]) {
                            ns.inputs[i] = ns.inputs[i].as_list()->at(li);
                        }
                    }
                    auto elem_result = exec_node(project, c, *node, slots);
                    ns.inputs = saved_inputs;

                    if (elem_result.status != EvalStatus::Success) continue;
                    // First output is the item, check truthiness for partition
                    if (!elem_result.outputs.empty() && elem_result.outputs[0].is_truthy()) {
                        // Get the original list element
                        for (size_t i = 0; i < ns.inputs.size(); i++) {
                            if (is_list_input[i]) {
                                pass_list.push_back(ns.inputs[i].as_list()->at(li));
                                break;
                            }
                        }
                    } else {
                        for (size_t i = 0; i < ns.inputs.size(); i++) {
                            if (is_list_input[i]) {
                                fail_list.push_back(ns.inputs[i].as_list()->at(li));
                                break;
                            }
                        }
                    }
                }
                result = EvalResult::success({Value::list(std::move(pass_list)),
                                              Value::list(std::move(fail_list))});
            } else {
                // ListMap: collect outputs into lists
                std::vector<std::vector<Value>> output_lists(node->num_outputs);
                for (size_t li = 0; li < list_len; li++) {
                    auto saved_inputs = ns.inputs;
                    for (size_t i = 0; i < ns.inputs.size(); i++) {
                        if (is_list_input[i]) {
                            ns.inputs[i] = ns.inputs[i].as_list()->at(li);
                        }
                    }
                    auto elem_result = exec_node(project, c, *node, slots);
                    ns.inputs = saved_inputs;

                    if (elem_result.status != EvalStatus::Success) continue;
                    for (size_t oi = 0; oi < elem_result.outputs.size() && oi < output_lists.size(); oi++) {
                        output_lists[oi].push_back(elem_result.outputs[oi]);
                    }
                }
                std::vector<Value> list_outputs;
                for (auto& ol : output_lists) {
                    list_outputs.push_back(Value::list(std::move(ol)));
                }
                result = EvalResult::success(std::move(list_outputs));
            }
        } else {
            result = exec_node(project, c, *node, slots);
        }

        ns.executed = true;

        // For nodes with control annotations: if the node produced a boolean
        // output, treat false as failure
        if (result.status == EvalStatus::Success && node->control != ControlType::None) {
            if (result.outputs.size() == 1 && result.outputs[0].is_boolean()) {
                if (!result.outputs[0].as_boolean()) {
                    result.status = EvalStatus::Failure;
                }
            }
        }

        if (result.status == EvalStatus::Failure) {
            switch (node->control) {
                case ControlType::NextCaseOnFailure:
                    return EvalResult::failure();
                case ControlType::ContinueOnFailure:
                    break;
                case ControlType::FailOnFailure:
                    return EvalResult::failure();
                case ControlType::TerminateOnFailure:
                case ControlType::FinishOnFailure:
                    return EvalResult::failure();
                default:
                    return EvalResult::make_error("unhandled failure in node: " + node->name);
            }
            continue;
        }

        if (result.status == EvalStatus::Error) {
            // Phase 17: try annotation
            if (node->has_try && node->error_out_pin < node->num_outputs) {
                // Convert error to output on error pin, null on normal pins
                std::vector<Value> try_outputs(node->num_outputs);
                try_outputs[node->error_out_pin] = result.err_val;
                result = EvalResult::success(std::move(try_outputs));
            } else {
                return result;
            }
        }

        // Success - check success-side controls
        switch (node->control) {
            case ControlType::NextCaseOnSuccess:
                return EvalResult::failure();
            case ControlType::FailOnSuccess:
                return EvalResult::failure();
            case ControlType::TerminateOnSuccess:
            case ControlType::FinishOnSuccess:
                return EvalResult::failure();
            default:
                break;
        }

        // Store outputs
        for (size_t i = 0; i < result.outputs.size() && i < ns.outputs.size(); i++) {
            ns.outputs[i] = result.outputs[i];
        }

        // Propagate outputs through wires
        auto out_wires = c.output_wires(nid);
        for (auto* w : out_wires) {
            if (w->is_execution) {
                // Phase 11: trigger execution on target
                auto& target_ns = slots[w->target.node_id];
                target_ns.execution_triggered = true;
                // Check if target is now ready
                const Node* target_node = c.find_node(w->target.node_id);
                if (target_node && !target_ns.executed && is_ready(*target_node, target_ns)) {
                    ready.push(w->target.node_id);
                }
                continue;
            }

            auto& target_ns = slots[w->target.node_id];
            if (w->source.index < ns.outputs.size()) {
                target_ns.inputs[w->target.index] = ns.outputs[w->source.index];
                target_ns.inputs_filled++;

                // Record trace entry for debugger
                if (debugger_ && debugger_->is_tracing()) {
                    TraceEntry entry;
                    entry.source_node = nid;
                    entry.source_pin = w->source.index;
                    entry.dest_node = w->target.node_id;
                    entry.dest_pin = w->target.index;
                    entry.value = ns.outputs[w->source.index];
                    entry.step_number = steps;
                    debugger_->record_trace(entry);
                }

                // Check if target node is now ready
                const Node* target_node = c.find_node(w->target.node_id);
                if (target_node && !target_ns.executed && is_ready(*target_node, target_ns)) {
                    ready.push(w->target.node_id);
                }
            }
        }

        // Debugger: step completed check
        if (debugger_) {
            debugger_->step_completed(nid);
            if (debugger_->stop_requested()) {
                return EvalResult::make_error("execution stopped by debugger");
            }
        }

        steps++;
    }

    if (steps >= max_steps) {
        return EvalResult::make_error("evaluation exceeded max steps");
    }

    // Collect output bar values
    const Node* output_bar = c.find_node(c.output_bar_id);
    if (output_bar) {
        auto& bar_slots = slots[output_bar->id];
        return EvalResult::success(std::move(bar_slots.inputs));
    }

    return EvalResult::success({});
}

// Phase 20: simple expression evaluator
Value Evaluator::eval_expression(const std::string& expr,
                                 const std::unordered_map<std::string, Value>& bindings) {
    // Simple tokenizer + recursive descent for arithmetic, comparison, ternary
    struct ExprParser {
        const std::string& src;
        const std::unordered_map<std::string, Value>& bindings;
        size_t pos = 0;

        void skip_ws() { while (pos < src.size() && isspace(src[pos])) pos++; }
        char peek() { skip_ws(); return pos < src.size() ? src[pos] : '\0'; }
        char advance() { return pos < src.size() ? src[pos++] : '\0'; }
        bool match(char c) { skip_ws(); if (pos < src.size() && src[pos] == c) { pos++; return true; } return false; }
        bool match2(const char* s) {
            skip_ws();
            if (pos + 1 < src.size() && src[pos] == s[0] && src[pos+1] == s[1]) { pos += 2; return true; }
            return false;
        }

        Value parse_expr() { return parse_ternary(); }

        Value parse_ternary() {
            Value cond = parse_or();
            if (match('?')) {
                Value t = parse_expr();
                if (!match(':')) return Value::error("expected ':' in ternary");
                Value f = parse_expr();
                return cond.is_truthy() ? t : f;
            }
            return cond;
        }

        Value parse_or() {
            Value left = parse_and();
            while (match2("||")) { Value right = parse_and(); left = Value::boolean(left.is_truthy() || right.is_truthy()); }
            return left;
        }

        Value parse_and() {
            Value left = parse_compare();
            while (match2("&&")) { Value right = parse_compare(); left = Value::boolean(left.is_truthy() && right.is_truthy()); }
            return left;
        }

        Value parse_compare() {
            Value left = parse_add();
            skip_ws();
            if (match2("==")) { Value right = parse_add(); return Value::boolean(left.equals(right)); }
            if (match2("!=")) { Value right = parse_add(); return Value::boolean(!left.equals(right)); }
            if (match2("<=")) { Value right = parse_add(); return Value::boolean(left.compare(right) <= 0); }
            if (match2(">=")) { Value right = parse_add(); return Value::boolean(left.compare(right) >= 0); }
            if (match('<')) { Value right = parse_add(); return Value::boolean(left.compare(right) < 0); }
            if (match('>')) { Value right = parse_add(); return Value::boolean(left.compare(right) > 0); }
            return left;
        }

        Value parse_add() {
            Value left = parse_mul();
            while (true) {
                if (match('+')) {
                    Value right = parse_mul();
                    if (left.is_string() || right.is_string()) {
                        left = Value::string(left.to_display_string() + right.to_display_string());
                    } else if (left.is_integer() && right.is_integer()) {
                        left = Value::integer(left.as_integer() + right.as_integer());
                    } else {
                        left = Value::real(left.as_number() + right.as_number());
                    }
                } else if (match('-')) {
                    Value right = parse_mul();
                    if (left.is_integer() && right.is_integer())
                        left = Value::integer(left.as_integer() - right.as_integer());
                    else left = Value::real(left.as_number() - right.as_number());
                } else break;
            }
            return left;
        }

        Value parse_mul() {
            Value left = parse_unary();
            while (true) {
                if (match('*')) {
                    Value right = parse_unary();
                    if (left.is_integer() && right.is_integer())
                        left = Value::integer(left.as_integer() * right.as_integer());
                    else left = Value::real(left.as_number() * right.as_number());
                } else if (match('/')) {
                    Value right = parse_unary();
                    double d = right.as_number();
                    if (d == 0) return Value::error("division by zero");
                    if (left.is_integer() && right.is_integer())
                        left = Value::integer(left.as_integer() / right.as_integer());
                    else left = Value::real(left.as_number() / d);
                } else if (match('%')) {
                    Value right = parse_unary();
                    if (left.is_integer() && right.is_integer() && right.as_integer() != 0)
                        left = Value::integer(left.as_integer() % right.as_integer());
                    else left = Value::real(std::fmod(left.as_number(), right.as_number()));
                } else break;
            }
            return left;
        }

        Value parse_unary() {
            if (match('!')) return Value::boolean(!parse_unary().is_truthy());
            if (match('-')) {
                Value v = parse_unary();
                if (v.is_integer()) return Value::integer(-v.as_integer());
                return Value::real(-v.as_number());
            }
            return parse_primary();
        }

        Value parse_primary() {
            skip_ws();
            if (match('(')) {
                Value v = parse_expr();
                match(')');
                return v;
            }
            // String literal
            if (peek() == '"') {
                advance();
                std::string s;
                while (pos < src.size() && src[pos] != '"') {
                    if (src[pos] == '\\' && pos + 1 < src.size()) { pos++; s += src[pos++]; }
                    else s += src[pos++];
                }
                if (pos < src.size()) pos++; // skip closing "
                return Value::string(s);
            }
            // Number
            if (isdigit(peek()) || (peek() == '.' && pos + 1 < src.size() && isdigit(src[pos + 1]))) {
                size_t start = pos;
                while (pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) pos++;
                std::string num_str = src.substr(start, pos - start);
                if (num_str.find('.') != std::string::npos) return Value::real(std::stod(num_str));
                return Value::integer(std::stoll(num_str));
            }
            // true/false/null
            if (src.compare(pos, 4, "true") == 0 && (pos + 4 >= src.size() || !isalnum(src[pos+4]))) {
                pos += 4; return Value::boolean(true);
            }
            if (src.compare(pos, 5, "false") == 0 && (pos + 5 >= src.size() || !isalnum(src[pos+5]))) {
                pos += 5; return Value::boolean(false);
            }
            if (src.compare(pos, 4, "null") == 0 && (pos + 4 >= src.size() || !isalnum(src[pos+4]))) {
                pos += 4; return Value::null_val();
            }
            // Identifier (variable binding)
            if (isalpha(peek()) || peek() == '_') {
                size_t start = pos;
                while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) pos++;
                std::string name = src.substr(start, pos - start);
                auto it = bindings.find(name);
                if (it != bindings.end()) return it->second;
                return Value::error("undefined: " + name);
            }
            return Value::null_val();
        }
    };

    ExprParser parser{expr, bindings, 0};
    return parser.parse_expr();
}

// Phase 15: Loop evaluation
// Loops work by re-evaluating a case body iteratively.
// The is_loop annotation on a node marks it as a loop node.
// In Prograph, a loop method has cases that execute repeatedly.
// We implement this at the case level: if a case is_loop, we re-run it.
// For the simpler in-graph loop: a node with is_loop=true calls its method
// repeatedly, feeding outputs back as inputs each iteration.
EvalResult Evaluator::eval_loop(Project& project, const Case& c, const Node& node,
                                 std::unordered_map<NodeId, NodeSlots>& slots) {
    auto& ns = slots[node.id];

    // The loop node calls its method repeatedly.
    // Inputs: initial values. After each iteration, outputs become next inputs.
    // Termination: controlled by ControlType annotations inside the called method.
    // TerminateOnFailure/Success: stop immediately.
    // FinishOnFailure/Success: complete iteration, then stop.

    std::string method_name = node.name;
    std::vector<Value> current_inputs = ns.inputs;

    // Initialize shift registers from the case
    std::unordered_map<uint32_t, Value> shift_vals;
    for (auto& sr : c.shift_registers) {
        shift_vals[sr.output_pin] = sr.initial;
    }

    uint32_t iteration = 0;
    uint32_t max_iterations = max_steps;

    while (iteration < max_iterations) {
        // Add shift register values to inputs
        for (auto& sr : c.shift_registers) {
            if (sr.input_pin < current_inputs.size()) {
                current_inputs[sr.input_pin] = shift_vals[sr.output_pin];
            }
        }

        EvalResult iter_result;
        if (node.local_method) {
            iter_result = eval_method(project, *node.local_method, current_inputs);
        } else {
            iter_result = call_method(project, method_name, current_inputs);
        }

        // Update shift register values from outputs
        for (auto& sr : c.shift_registers) {
            if (sr.output_pin < iter_result.outputs.size()) {
                shift_vals[sr.output_pin] = iter_result.outputs[sr.output_pin];
            }
        }

        if (iter_result.status == EvalStatus::Failure) {
            // Loop body failed = termination condition met
            // Return the last successful outputs (current_inputs)
            return EvalResult::success(std::move(current_inputs));
        }

        if (iter_result.status == EvalStatus::Error) {
            return iter_result;
        }

        // Feed outputs back as next iteration's inputs
        current_inputs = std::move(iter_result.outputs);
        iteration++;
    }

    return EvalResult::make_error("loop exceeded max iterations");
}

EvalResult Evaluator::exec_node(Project& project, const Case& c, const Node& node,
                                 std::unordered_map<NodeId, NodeSlots>& slots) {
    auto& ns = slots[node.id];

    switch (node.type) {
        case NodeType::Constant: {
            return EvalResult::success({node.constant_value});
        }

        case NodeType::Primitive: {
            auto* prim = PrimitiveRegistry::instance().find(node.name);
            if (!prim) {
                return EvalResult::make_error("unknown primitive: " + node.name);
            }
            auto result = prim->fn(ns.inputs);
            if (result.failed) {
                return EvalResult::failure();
            }
            return EvalResult::success(std::move(result.outputs));
        }

        case NodeType::MethodCall: {
            // Phase 15: loop node
            if (node.is_loop) {
                return eval_loop(project, c, node, slots);
            }
            // Check for data-determined dispatch: /MethodName
            if (!node.name.empty() && node.name[0] == '/') {
                std::string method_name = node.name.substr(1);
                if (ns.inputs.empty() || !ns.inputs[0].is_object()) {
                    return EvalResult::make_error("data-determined dispatch requires object as first input");
                }
                auto* obj = ns.inputs[0].as_object();
                const auto* m = project.find_class_method(obj->class_name(), method_name);
                if (!m) {
                    return EvalResult::make_error("method " + method_name + " not found on class " + obj->class_name());
                }
                return eval_method(project, *m, ns.inputs);
            }
            // Check for ClassName/MethodName syntax
            auto slash_pos = node.name.find('/');
            if (slash_pos != std::string::npos && slash_pos > 0) {
                std::string class_name = node.name.substr(0, slash_pos);
                std::string method_name = node.name.substr(slash_pos + 1);
                const auto* m = project.find_class_method(class_name, method_name);
                if (!m) {
                    return EvalResult::make_error("method " + class_name + "/" + method_name + " not found");
                }
                return eval_method(project, *m, ns.inputs);
            }
            return call_method(project, node.name, ns.inputs);
        }

        case NodeType::InstanceGenerator: {
            const ClassDef* cls = project.find_class(node.name);
            if (!cls) {
                return EvalResult::make_error("class not found: " + node.name);
            }
            auto obj = make_ref<PhoObject>(node.name);
            const ClassDef* c2 = cls;
            while (c2) {
                for (auto& attr : c2->attributes) {
                    if (!attr.is_class_attr && !obj->has_attr(attr.name)) {
                        obj->set_attr(attr.name, attr.default_value);
                    }
                }
                if (c2->parent_name.empty()) break;
                c2 = project.find_class(c2->parent_name);
            }
            Value obj_val = Value::object(obj);
            const auto* init = project.find_class_method(node.name, "init");
            if (init) {
                std::vector<Value> init_args = {obj_val};
                for (auto& inp : ns.inputs) init_args.push_back(inp);
                auto init_result = eval_method(project, *init, init_args);
                if (init_result.status != EvalStatus::Success) return init_result;
                if (!init_result.outputs.empty() && init_result.outputs[0].is_object())
                    return EvalResult::success({init_result.outputs[0]});
            }
            return EvalResult::success({obj_val});
        }

        case NodeType::Get: {
            if (ns.inputs.empty() || !ns.inputs[0].is_object()) {
                return EvalResult::make_error("get requires object input");
            }
            auto* obj = ns.inputs[0].as_object();
            Value attr_val = obj->get_attr(node.name);
            return EvalResult::success({ns.inputs[0], attr_val});
        }

        case NodeType::Set: {
            if (ns.inputs.size() < 2 || !ns.inputs[0].is_object()) {
                return EvalResult::make_error("set requires object and value inputs");
            }
            auto* obj = ns.inputs[0].as_object();
            obj->set_attr(node.name, ns.inputs[1]);
            return EvalResult::success({ns.inputs[0]});
        }

        case NodeType::InputBar:
        case NodeType::OutputBar:
            return EvalResult::success({});

        // Phase 19: Local method
        case NodeType::LocalMethod: {
            if (!node.local_method) {
                return EvalResult::make_error("local method has no body");
            }
            return eval_method(project, *node.local_method, ns.inputs);
        }

        // Phase 20: Evaluation (inline expression)
        case NodeType::Evaluation: {
            // Build bindings from wire names -> input values
            std::unordered_map<std::string, Value> bindings;
            // Use input pin names or wire names from the case wires
            for (auto& w : c.wires) {
                if (w.target.node_id == node.id && !w.is_execution) {
                    // Use wire name or "input0", "input1", etc.
                    std::string name = !w.target.is_output ? "input" + std::to_string(w.target.index) : "";
                    // Also look for wire names from the case
                    for (auto& w2 : c.wires) {
                        if (w2.target.node_id == node.id && w2.target.index == w.target.index) {
                            // Use source pin name as binding name
                            break;
                        }
                    }
                    bindings[name] = ns.inputs[w.target.index];
                    // Also bind by index alias: a, b, c, ...
                    if (w.target.index < 26) {
                        bindings[std::string(1, 'a' + static_cast<char>(w.target.index))] = ns.inputs[w.target.index];
                    }
                }
            }
            // Always bind positional names
            for (size_t i = 0; i < ns.inputs.size(); i++) {
                bindings["input" + std::to_string(i)] = ns.inputs[i];
                if (i < 26) {
                    bindings[std::string(1, 'a' + static_cast<char>(i))] = ns.inputs[i];
                }
            }
            Value result = eval_expression(node.expression, bindings);
            return EvalResult::success({result});
        }

        // Phase 21: Inject (dynamic dispatch)
        case NodeType::Inject: {
            if (ns.inputs.empty()) {
                return EvalResult::make_error("inject requires method name input");
            }
            std::string method_name;
            if (ns.inputs[0].is_string()) {
                method_name = ns.inputs[0].as_string()->str();
            } else if (ns.inputs[0].is_method_ref()) {
                auto* mr = ns.inputs[0].as_method_ref();
                method_name = mr->method_name();
                // If bound, use the class method
                if (!mr->class_name().empty()) {
                    const auto* m = project.find_class_method(mr->class_name(), method_name);
                    if (m) {
                        std::vector<Value> args(ns.inputs.begin() + 1, ns.inputs.end());
                        if (mr->has_bound_object()) {
                            args.insert(args.begin(), Value::object(
                                Ref<PhoObject>(mr->bound_object())));
                        }
                        return eval_method(project, *m, args);
                    }
                }
            } else {
                return EvalResult::make_error("inject: first input must be string or method-ref");
            }

            std::vector<Value> args(ns.inputs.begin() + 1, ns.inputs.end());
            return call_method(project, method_name, args);
        }

        // Phase 22: Persistent
        case NodeType::Persistent: {
            std::string key = node.name;
            if (ns.inputs.size() > 0 && !ns.inputs[0].is_null()) {
                // Write mode: store the value
                persistent_store[key] = ns.inputs[0];
                return EvalResult::success({ns.inputs[0]});
            }
            // Read mode: retrieve stored value or default
            auto it = persistent_store.find(key);
            if (it != persistent_store.end()) {
                return EvalResult::success({it->second});
            }
            return EvalResult::success({node.constant_value}); // default
        }

        default:
            return EvalResult::make_error("unimplemented node type: " + node.name);
    }
}

} // namespace pho
