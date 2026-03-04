#pragma once
#include "pho_graph.h"
#include "pho_value.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace pho {

// Code generation: compile a Phograph Project to Swift source code.
//
// Strategy:
// - Each universal Method becomes a Swift function.
// - Each ClassDef becomes a Swift class.
// - Each Case is compiled by topological-sorting nodes and emitting
//   sequential variable assignments.
// - Multiple cases become if/else chains (first success wins).
// - Primitives map to Swift operators/stdlib calls.
// - Control annotations map to guard/continue/break.

struct CodegenOptions {
    bool emit_imports = true;       // emit "import Foundation" etc.
    bool emit_runtime = false;      // embed minimal runtime types inline
    bool emit_main = true;          // emit @main / entry point
    std::string entry_method;       // method name for main(), default "main"
    std::string indent = "    ";    // indent string
};

struct CodegenError {
    std::string message;
    std::string context; // method/class name
};

class SwiftCodegen {
public:
    explicit SwiftCodegen(const CodegenOptions& opts = {}) : opts_(opts) {}

    // Compile entire project to Swift source
    bool compile(const Project& project, std::string& out_source, std::vector<CodegenError>& errors);

    // Compile a single method to Swift function source
    std::string compile_method(const Project& project, const Method& method);

    // Compile a single class to Swift class source
    std::string compile_class(const Project& project, const ClassDef& cls);

private:
    CodegenOptions opts_;

    // State during compilation
    uint32_t temp_counter_ = 0;
    std::unordered_set<std::string> used_runtime_features_;

    // Helpers
    std::string fresh_var(const std::string& hint = "v");
    std::string indent(int level) const;

    // Swift name mangling
    std::string swift_func_name(const std::string& pho_name) const;
    std::string swift_class_name(const std::string& pho_name) const;
    std::string swift_var_name(const std::string& pho_name) const;

    // Primitive -> Swift expression mapping
    std::string emit_prim_call(const std::string& prim_name,
                                const std::vector<std::string>& arg_vars) const;

    // Value literal -> Swift literal
    std::string emit_literal(const Value& val) const;

    // Topological sort of nodes in a case
    std::vector<NodeId> topo_sort(const Case& c) const;

    // Compile one case body; returns list of lines
    std::vector<std::string> compile_case(const Project& project, const Case& c,
                                           const Method& method, int indent_level);

    // Emit runtime helper code
    std::string emit_runtime() const;
};

} // namespace pho
