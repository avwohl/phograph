#include "pho_eval.h"
#include <algorithm>
#include <cassert>

namespace pho {

Evaluator::Evaluator() {}

EvalResult Evaluator::eval_method(Project& project, const Method& method,
                                   const std::vector<Value>& inputs) {
    for (auto& c : method.cases) {
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
    // Initialize slot storage for each node
    std::unordered_map<NodeId, NodeSlots> slots;
    for (auto& node : c.nodes) {
        NodeSlots ns;
        ns.inputs.resize(node.num_inputs);
        ns.outputs.resize(node.num_outputs);
        slots[node.id] = std::move(ns);
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
            auto& target_slots = slots[w->target.node_id];
            target_slots.inputs[w->target.index] = slots[input_bar->id].outputs[w->source.index];
            target_slots.inputs_filled++;
        }
    }

    // Build ready queue: nodes with all inputs filled
    std::queue<NodeId> ready;
    for (auto& node : c.nodes) {
        if (node.id == c.input_bar_id) continue;
        if (node.id == c.output_bar_id && node.num_inputs == 0) continue;
        auto& ns = slots[node.id];
        if (node.type == NodeType::Constant) {
            ready.push(node.id);
        } else if (node.num_inputs == 0 && (node.type == NodeType::Primitive || node.type == NodeType::InstanceGenerator)) {
            ready.push(node.id); // zero-input primitives or instance generators
        } else if (node.num_inputs > 0 && ns.inputs_filled >= node.num_inputs) {
            ready.push(node.id);
        }
    }

    // Process ready queue
    uint32_t steps = 0;
    bool case_failed = false;

    while (!ready.empty() && steps < max_steps) {
        NodeId nid = ready.front();
        ready.pop();

        auto& ns = slots[nid];
        if (ns.executed) continue;

        const Node* node = c.find_node(nid);
        if (!node) continue;

        auto result = exec_node(project, c, *node, slots);

        ns.executed = true;

        // For nodes with control annotations: if the node produced a boolean
        // output, treat false as failure (this is how Prograph match operations work).
        if (result.status == EvalStatus::Success && node->control != ControlType::None) {
            if (result.outputs.size() == 1 && result.outputs[0].is_boolean()) {
                if (!result.outputs[0].as_boolean()) {
                    result.status = EvalStatus::Failure;
                }
            }
        }

        if (result.status == EvalStatus::Failure) {
            // Handle control annotation
            switch (node->control) {
                case ControlType::NextCaseOnFailure:
                    return EvalResult::failure(); // try next case
                case ControlType::ContinueOnFailure:
                    break; // continue evaluation despite failure
                case ControlType::FailOnFailure:
                    return EvalResult::failure(); // propagate failure
                case ControlType::TerminateOnFailure:
                case ControlType::FinishOnFailure:
                    return EvalResult::failure(); // loop termination (phase 2)
                default:
                    // No failure handler - this is an error
                    return EvalResult::make_error("unhandled failure in node: " + node->name);
            }
            continue;
        }

        if (result.status == EvalStatus::Error) {
            return result;
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
                if (target_node && !target_ns.executed) {
                    if (target_node->type == NodeType::Constant) {
                        ready.push(w->target.node_id);
                    } else if (target_ns.inputs_filled >= target_node->num_inputs) {
                        ready.push(w->target.node_id);
                    }
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
            // Check for data-determined dispatch: /MethodName
            // First input is the object, method dispatched on object's class
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
            // node.name is the class name
            const ClassDef* cls = project.find_class(node.name);
            if (!cls) {
                return EvalResult::make_error("class not found: " + node.name);
            }
            auto obj = make_ref<PhoObject>(node.name);
            // Set default attribute values, walking up inheritance
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
            // Call init if it exists
            const auto* init = project.find_class_method(node.name, "init");
            if (init) {
                std::vector<Value> init_args = {obj_val};
                // Pass through any inputs to init
                for (auto& inp : ns.inputs) init_args.push_back(inp);
                auto init_result = eval_method(project, *init, init_args);
                if (init_result.status != EvalStatus::Success) return init_result;
                // init may return the object
                if (!init_result.outputs.empty() && init_result.outputs[0].is_object())
                    return EvalResult::success({init_result.outputs[0]});
            }
            return EvalResult::success({obj_val});
        }

        case NodeType::Get: {
            // Input 0: object, node.name = attribute name
            if (ns.inputs.empty() || !ns.inputs[0].is_object()) {
                return EvalResult::make_error("get requires object input");
            }
            auto* obj = ns.inputs[0].as_object();
            // Output 0: the object (pass-through), Output 1: attribute value
            Value attr_val = obj->get_attr(node.name);
            return EvalResult::success({ns.inputs[0], attr_val});
        }

        case NodeType::Set: {
            // Input 0: object, Input 1: new value, node.name = attribute name
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

        default:
            return EvalResult::make_error("unimplemented node type: " + node.name);
    }
}

} // namespace pho
