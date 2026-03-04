#pragma once
#include "pho_value.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace pho {

// Forward declarations
class Node;
class Wire;
class Case;
class Method;
class Section;
class Project;

using NodeId = uint32_t;
using WireId = uint32_t;

// Pin: a connection point on a node (input or output)
struct Pin {
    NodeId node_id = 0;
    uint32_t index = 0;   // pin index on the node
    bool is_output = false;

    bool operator==(const Pin& o) const {
        return node_id == o.node_id && index == o.index && is_output == o.is_output;
    }
};

// Wire: connects an output pin to an input pin
struct Wire {
    WireId id = 0;
    Pin source;      // output pin (producer)
    Pin target;      // input pin (consumer)
};

// Control annotation on a node
enum class ControlType : uint8_t {
    None = 0,
    NextCaseOnFailure,    // X - jump to next case on failure
    NextCaseOnSuccess,    // checkmark - jump to next case on success
    ContinueOnFailure,    // continue despite failure
    ContinueOnSuccess,    // continue on success (default)
    FailOnFailure,        // propagate failure to caller
    FailOnSuccess,        // propagate success as failure
    TerminateOnFailure,   // exit loop on failure
    TerminateOnSuccess,   // exit loop on success
    FinishOnFailure,      // finish iteration then exit on failure
    FinishOnSuccess,      // finish iteration then exit on success
};

// Node type
enum class NodeType : uint8_t {
    Primitive,         // built-in operation
    MethodCall,        // call to a user method
    Constant,          // literal value
    InputBar,          // method input bar
    OutputBar,         // method output bar
    Get,               // attribute get
    Set,               // attribute set
    InstanceGenerator, // create class instance
    Persistent,        // global persistent
    LocalMethod,       // inline local method
    Evaluation,        // inline expression
    Inject,            // dynamic dispatch
};

// Node in a dataflow graph
class Node {
public:
    NodeId id = 0;
    NodeType type = NodeType::Primitive;
    std::string name;           // primitive/method name
    uint32_t num_inputs = 0;
    uint32_t num_outputs = 0;
    ControlType control = ControlType::None;
    Value constant_value;       // for Constant nodes
    bool has_execution_in = false;
    bool has_execution_out = false;
};

// Case: a single dataflow diagram within a method
class Case {
public:
    std::vector<Node> nodes;
    std::vector<Wire> wires;

    NodeId input_bar_id = 0;   // node id of the input bar
    NodeId output_bar_id = 0;  // node id of the output bar

    NodeId next_node_id = 1;
    WireId next_wire_id = 1;

    NodeId add_node(Node node) {
        node.id = next_node_id++;
        nodes.push_back(std::move(node));
        return nodes.back().id;
    }

    WireId add_wire(Pin source, Pin target) {
        Wire w;
        w.id = next_wire_id++;
        w.source = source;
        w.target = target;
        wires.push_back(w);
        return w.id;
    }

    const Node* find_node(NodeId id) const {
        for (auto& n : nodes) {
            if (n.id == id) return &n;
        }
        return nullptr;
    }

    // Get all wires feeding into a node's inputs
    std::vector<const Wire*> input_wires(NodeId node_id) const {
        std::vector<const Wire*> result;
        for (auto& w : wires) {
            if (w.target.node_id == node_id) result.push_back(&w);
        }
        return result;
    }

    // Get all wires from a node's outputs
    std::vector<const Wire*> output_wires(NodeId node_id) const {
        std::vector<const Wire*> result;
        for (auto& w : wires) {
            if (w.source.node_id == node_id) result.push_back(&w);
        }
        return result;
    }
};

// Method: one or more cases
class Method {
public:
    std::string name;
    uint32_t num_inputs = 0;
    uint32_t num_outputs = 0;
    std::vector<Case> cases;
    std::string class_name; // empty for universal methods
};

// Attribute definition
struct AttributeDef {
    std::string name;
    Value default_value;
    bool is_class_attr = false; // true = class attribute, false = instance attribute
};

// Class definition
class ClassDef {
public:
    std::string name;
    std::string parent_name;        // empty if no parent
    std::vector<AttributeDef> attributes;
    std::vector<Method> methods;

    Method* find_method(const std::string& mname) {
        for (auto& m : methods) {
            if (m.name == mname) return &m;
        }
        return nullptr;
    }

    const Method* find_method(const std::string& mname) const {
        for (auto& m : methods) {
            if (m.name == mname) return &m;
        }
        return nullptr;
    }

    const AttributeDef* find_attribute(const std::string& aname) const {
        for (auto& a : attributes) {
            if (a.name == aname) return &a;
        }
        return nullptr;
    }
};

// Section: contains classes and universal methods
class Section {
public:
    std::string name;
    std::vector<Method> methods;        // universal methods
    std::vector<ClassDef> classes;

    Method* find_method(const std::string& mname) {
        for (auto& m : methods) {
            if (m.name == mname) return &m;
        }
        return nullptr;
    }

    const Method* find_method(const std::string& mname) const {
        for (auto& m : methods) {
            if (m.name == mname) return &m;
        }
        return nullptr;
    }

    ClassDef* find_class(const std::string& cname) {
        for (auto& c : classes) {
            if (c.name == cname) return &c;
        }
        return nullptr;
    }

    const ClassDef* find_class(const std::string& cname) const {
        for (auto& c : classes) {
            if (c.name == cname) return &c;
        }
        return nullptr;
    }
};

// Project: top-level container
class Project {
public:
    std::string name;
    std::vector<Section> sections;

    Method* find_method(const std::string& mname) {
        for (auto& s : sections) {
            if (auto* m = s.find_method(mname)) return m;
        }
        return nullptr;
    }

    const Method* find_method(const std::string& mname) const {
        for (auto& s : sections) {
            if (auto* m = s.find_method(mname)) return m;
        }
        return nullptr;
    }

    ClassDef* find_class(const std::string& cname) {
        for (auto& s : sections) {
            if (auto* c = s.find_class(cname)) return c;
        }
        return nullptr;
    }

    const ClassDef* find_class(const std::string& cname) const {
        for (auto& s : sections) {
            if (auto* c = s.find_class(cname)) return c;
        }
        return nullptr;
    }

    // Find a method on a class, walking up the inheritance chain
    const Method* find_class_method(const std::string& class_name,
                                     const std::string& method_name) const {
        const ClassDef* cls = find_class(class_name);
        while (cls) {
            if (auto* m = cls->find_method(method_name)) return m;
            if (cls->parent_name.empty()) break;
            cls = find_class(cls->parent_name);
        }
        return nullptr;
    }
};

} // namespace pho
