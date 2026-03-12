#include "pho_codegen.h"
#include <algorithm>
#include <cassert>

namespace pho {

// --- Name mangling ---

static bool is_identifier_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static std::string sanitize_identifier(const std::string& name) {
    std::string result;
    bool next_upper = false;
    for (size_t i = 0; i < name.size(); i++) {
        char c = name[i];
        if (c == '-' || c == '?' || c == '!' || c == '/' || c == ' ') {
            next_upper = true;
        } else if (is_identifier_char(c)) {
            if (next_upper && !result.empty()) {
                result += static_cast<char>(toupper(c));
                next_upper = false;
            } else {
                result += c;
                next_upper = false;
            }
        }
    }
    if (result.empty()) result = "_unnamed";
    // Don't start with digit
    if (result[0] >= '0' && result[0] <= '9') result = "_" + result;
    return result;
}

std::string SwiftCodegen::swift_func_name(const std::string& pho_name) const {
    return sanitize_identifier(pho_name);
}

std::string SwiftCodegen::swift_class_name(const std::string& pho_name) const {
    std::string s = sanitize_identifier(pho_name);
    if (!s.empty()) s[0] = static_cast<char>(toupper(s[0]));
    return s;
}

std::string SwiftCodegen::swift_var_name(const std::string& pho_name) const {
    std::string s = sanitize_identifier(pho_name);
    if (!s.empty()) s[0] = static_cast<char>(tolower(s[0]));
    return s;
}

std::string SwiftCodegen::fresh_var(const std::string& hint) {
    return hint + std::to_string(temp_counter_++);
}

std::string SwiftCodegen::indent(int level) const {
    std::string result;
    for (int i = 0; i < level; i++) result += opts_.indent;
    return result;
}

// --- Literal emission ---

std::string SwiftCodegen::emit_literal(const Value& val) const {
    switch (val.tag()) {
        case ValueTag::Null: return "PhoValue.null";
        case ValueTag::Integer: return "PhoValue.integer(" + std::to_string(val.as_integer()) + ")";
        case ValueTag::Real: {
            std::ostringstream oss;
            oss << val.as_real();
            return "PhoValue.real(" + oss.str() + ")";
        }
        case ValueTag::Boolean: return val.as_boolean() ? "PhoValue.boolean(true)" : "PhoValue.boolean(false)";
        case ValueTag::String:
            return "PhoValue.string(\"" + val.as_string()->str() + "\")";
        default:
            return "PhoValue.null /* unsupported literal */";
    }
}

// --- Primitive mapping ---

std::string SwiftCodegen::emit_prim_call(const std::string& prim_name,
                                          const std::vector<std::string>& args) const {
    // Arithmetic
    if (prim_name == "+" && args.size() == 2)
        return "phoAdd(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "-" && args.size() == 2)
        return "phoSub(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "*" && args.size() == 2)
        return "phoMul(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "/" && args.size() == 2)
        return "phoDiv(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "mod" && args.size() == 2)
        return "phoMod(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "abs" && args.size() == 1)
        return "phoAbs(" + args[0] + ")";
    if (prim_name == "negate" && args.size() == 1)
        return "phoNegate(" + args[0] + ")";
    if (prim_name == "sqrt" && args.size() == 1)
        return "phoSqrt(" + args[0] + ")";
    if (prim_name == "round" && args.size() == 1)
        return "phoRound(" + args[0] + ")";
    if (prim_name == "floor" && args.size() == 1)
        return "phoFloor(" + args[0] + ")";
    if (prim_name == "ceil" && args.size() == 1)
        return "phoCeil(" + args[0] + ")";
    if (prim_name == "clamp" && args.size() == 3)
        return "phoClamp(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "min" && args.size() == 2)
        return "phoMin(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "max" && args.size() == 2)
        return "phoMax(" + args[0] + ", " + args[1] + ")";

    // Comparison
    if (prim_name == "=" && args.size() == 2)
        return "phoEqual(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "!=" && args.size() == 2)
        return "phoNotEqual(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "<" && args.size() == 2)
        return "phoLessThan(" + args[0] + ", " + args[1] + ")";
    if (prim_name == ">" && args.size() == 2)
        return "phoGreaterThan(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "<=" && args.size() == 2)
        return "phoLessEqual(" + args[0] + ", " + args[1] + ")";
    if (prim_name == ">=" && args.size() == 2)
        return "phoGreaterEqual(" + args[0] + ", " + args[1] + ")";

    // Logic
    if (prim_name == "and" && args.size() == 2)
        return "phoAnd(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "or" && args.size() == 2)
        return "phoOr(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "not" && args.size() == 1)
        return "phoNot(" + args[0] + ")";

    // String
    if (prim_name == "string-concat" && args.size() == 2)
        return "phoStringConcat(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "string-length" && args.size() == 1)
        return "phoStringLength(" + args[0] + ")";
    if (prim_name == "to-string" && args.size() == 1)
        return "phoToString(" + args[0] + ")";

    // List
    if (prim_name == "list-create") {
        std::string s = "phoListCreate([";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) s += ", ";
            s += args[i];
        }
        return s + "])";
    }
    if (prim_name == "list-get" && args.size() == 2)
        return "phoListGet(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "list-set" && args.size() == 3)
        return "phoListSet(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "list-length" && args.size() == 1)
        return "phoListLength(" + args[0] + ")";
    if (prim_name == "list-append" && args.size() == 2)
        return "phoListAppend(" + args[0] + ", " + args[1] + ")";

    // Type checks
    if (prim_name == "integer?" && args.size() == 1)
        return "phoIsInteger(" + args[0] + ")";
    if (prim_name == "real?" && args.size() == 1)
        return "phoIsReal(" + args[0] + ")";
    if (prim_name == "string?" && args.size() == 1)
        return "phoIsString(" + args[0] + ")";
    if (prim_name == "boolean?" && args.size() == 1)
        return "phoIsBoolean(" + args[0] + ")";
    if (prim_name == "list?" && args.size() == 1)
        return "phoIsList(" + args[0] + ")";
    if (prim_name == "null?" && args.size() == 1)
        return "phoIsNull(" + args[0] + ")";
    if (prim_name == "type-of" && args.size() == 1)
        return "phoTypeOf(" + args[0] + ")";

    // Print
    if (prim_name == "print" && args.size() == 1)
        return "phoPrint(" + args[0] + ")";

    // String extras
    if (prim_name == "uppercase" && args.size() == 1)
        return "phoUppercase(" + args[0] + ")";
    if (prim_name == "lowercase" && args.size() == 1)
        return "phoLowercase(" + args[0] + ")";
    if (prim_name == "string-contains" && args.size() == 2)
        return "phoStringContains(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "string-split" && args.size() == 2)
        return "phoStringSplit(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "string-replace" && args.size() == 3)
        return "phoStringReplace(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "string-trim" && args.size() == 1)
        return "phoStringTrim(" + args[0] + ")";
    if (prim_name == "substring" && args.size() == 3)
        return "phoSubstring(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "string-search" && args.size() == 2)
        return "phoStringSearch(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "char-at" && args.size() == 2)
        return "phoCharAt(" + args[0] + ", " + args[1] + ")";

    // List extras
    if (prim_name == "list-first" && args.size() == 1)
        return "phoListFirst(" + args[0] + ")";
    if (prim_name == "list-rest" && args.size() == 1)
        return "phoListRest(" + args[0] + ")";
    if (prim_name == "list-reverse" && args.size() == 1)
        return "phoListReverse(" + args[0] + ")";
    if (prim_name == "list-sort" && args.size() == 1)
        return "phoListSort(" + args[0] + ")";
    if (prim_name == "list-contains" && args.size() == 2)
        return "phoListContains(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "list-empty?" && args.size() == 1)
        return "phoListEmpty(" + args[0] + ")";
    if (prim_name == "list-map" && args.size() == 2)
        return "phoListMap(" + args[0] + ", " + args[1] + ")";

    // Dict
    if (prim_name == "dict-create" && args.size() == 0)
        return "phoDictCreate()";
    if (prim_name == "dict-get" && args.size() == 2)
        return "phoDictGet(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "dict-set" && args.size() == 3)
        return "phoDictSet(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "dict-has" && args.size() == 2)
        return "phoDictHas(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "dict-remove" && args.size() == 2)
        return "phoDictRemove(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "dict-keys" && args.size() == 1)
        return "phoDictKeys(" + args[0] + ")";
    if (prim_name == "dict-values" && args.size() == 1)
        return "phoDictValues(" + args[0] + ")";
    if (prim_name == "dict-merge" && args.size() == 2)
        return "phoDictMerge(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "dict-size" && args.size() == 1)
        return "phoDictSize(" + args[0] + ")";

    // Math extras
    if (prim_name == "power" && args.size() == 2)
        return "phoPower(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "sin" && args.size() == 1)
        return "phoSin(" + args[0] + ")";
    if (prim_name == "cos" && args.size() == 1)
        return "phoCos(" + args[0] + ")";
    if (prim_name == "tan" && args.size() == 1)
        return "phoTan(" + args[0] + ")";
    if (prim_name == "atan2" && args.size() == 2)
        return "phoAtan2(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "pi" && args.size() == 0)
        return "phoPi()";
    if (prim_name == "random" && args.size() == 0)
        return "phoRandom()";

    // Error
    if (prim_name == "error-create" && args.size() == 1)
        return "phoErrorCreate(" + args[0] + ")";
    if (prim_name == "error-message" && args.size() == 1)
        return "phoErrorMessage(" + args[0] + ")";
    if (prim_name == "is-error" && args.size() == 1)
        return "phoIsError(" + args[0] + ")";

    // Canvas
    if (prim_name == "create-canvas" && args.size() == 2)
        return "phoCreateCanvas(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "canvas-width" && args.size() == 1)
        return "phoCanvasWidth(" + args[0] + ")";
    if (prim_name == "canvas-height" && args.size() == 1)
        return "phoCanvasHeight(" + args[0] + ")";
    if (prim_name == "canvas-clear" && args.size() == 1)
        return "phoCanvasClear(" + args[0] + ")";
    if (prim_name == "canvas-render" && args.size() == 1)
        return "phoCanvasRender(" + args[0] + ")";
    if (prim_name == "canvas-pixel-at" && args.size() == 3)
        return "phoCanvasPixelAt(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "canvas-set-root" && args.size() == 2)
        return "phoCanvasSetRoot(" + args[0] + ", " + args[1] + ")";

    // Shapes
    if (prim_name == "shape-rect" && args.size() == 4)
        return "phoShapeRect(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ")";
    if (prim_name == "shape-oval" && args.size() == 4)
        return "phoShapeOval(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ")";
    if (prim_name == "shape-text" && args.size() == 3)
        return "phoShapeText(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "shape-group")
        return "phoShapeGroup()";
    if (prim_name == "shape-set-fill" && args.size() == 2)
        return "phoShapeSetFill(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-set-stroke" && args.size() == 2)
        return "phoShapeSetStroke(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-set-corner-radius" && args.size() == 2)
        return "phoShapeSetCornerRadius(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-set-opacity" && args.size() == 2)
        return "phoShapeSetOpacity(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-set-bounds" && args.size() == 5)
        return "phoShapeSetBounds(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ")";
    if (prim_name == "shape-set-visible" && args.size() == 2)
        return "phoShapeSetVisible(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-set-tag" && args.size() == 2)
        return "phoShapeSetTag(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-add-child" && args.size() == 2)
        return "phoShapeAddChild(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "shape-get-fill" && args.size() == 1)
        return "phoShapeGetFill(" + args[0] + ")";
    if (prim_name == "shape-get-bounds" && args.size() == 1)
        return "phoShapeGetBounds(" + args[0] + ")";
    if (prim_name == "shape-hit-test" && args.size() == 3)
        return "phoShapeHitTest(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "shape-find-by-tag" && args.size() == 2)
        return "phoShapeFindByTag(" + args[0] + ", " + args[1] + ")";

    // Draw
    if (prim_name == "draw-fill-rect" && args.size() == 5)
        return "phoDrawFillRect(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ")";
    if (prim_name == "draw-stroke-rect" && args.size() == 5)
        return "phoDrawStrokeRect(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ")";
    if (prim_name == "draw-fill-oval" && args.size() == 5)
        return "phoDrawFillOval(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ")";
    if (prim_name == "draw-fill-rounded-rect" && args.size() == 6)
        return "phoDrawFillRoundedRect(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ", " + args[5] + ")";
    if (prim_name == "draw-set-pixel" && args.size() == 4)
        return "phoDrawSetPixel(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ")";
    if (prim_name == "draw-clear" && args.size() == 2)
        return "phoDrawClear(" + args[0] + ", " + args[1] + ")";

    // JSON
    if (prim_name == "json-parse" && args.size() == 1)
        return "phoJsonParse(" + args[0] + ")";
    if (prim_name == "json-encode" && args.size() == 1)
        return "phoJsonEncode(" + args[0] + ")";

    // Date
    if (prim_name == "date-now" && args.size() == 0)
        return "phoDateNow()";
    if (prim_name == "date-create" && args.size() == 6)
        return "phoDateCreate(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ", " + args[4] + ", " + args[5] + ")";
    if (prim_name == "date-format" && args.size() == 2)
        return "phoDateFormat(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "date-add" && args.size() == 2)
        return "phoDateAdd(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "date-diff" && args.size() == 2)
        return "phoDateDiff(" + args[0] + ", " + args[1] + ")";

    // String extras
    if (prim_name == "string-repeat" && args.size() == 2)
        return "phoStringRepeat(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "string-starts-with?" && args.size() == 2)
        return "phoStringStartsWith(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "string-ends-with?" && args.size() == 2)
        return "phoStringEndsWith(" + args[0] + ", " + args[1] + ")";

    // List HOFs
    if (prim_name == "filter" && args.size() == 2)
        return "phoFilter(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "reduce" && args.size() == 3)
        return "phoReduce(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "flat-map" && args.size() == 2)
        return "phoFlatMap(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "any?" && args.size() == 2)
        return "phoAny(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "all?" && args.size() == 2)
        return "phoAll(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "find" && args.size() == 2)
        return "phoFind(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "sort-by" && args.size() == 2)
        return "phoSortBy(" + args[0] + ", " + args[1] + ")";

    // Method ref
    if (prim_name == "method-ref" && args.size() == 1)
        return "phoMethodRef(" + args[0] + ")";
    if (prim_name == "call" && args.size() == 2)
        return "phoCall(" + args[0] + ", " + args[1] + ")";

    // Enum
    if (prim_name == "enum-create" && args.size() == 3)
        return "phoEnumCreate(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
    if (prim_name == "enum-variant" && args.size() == 1)
        return "phoEnumVariant(" + args[0] + ")";
    if (prim_name == "enum-data" && args.size() == 1)
        return "phoEnumData(" + args[0] + ")";

    // Phase 26: Observable primitives
    if (prim_name == "observe" && args.size() == 2)
        return "phoObserve(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "unobserve" && args.size() == 2)
        return "phoUnobserve(" + args[0] + ", " + args[1] + ")";
    if (prim_name == "observe-any" && args.size() == 1)
        return "phoObserveAny(" + args[0] + ")";
    if (prim_name == "bind" && args.size() == 4)
        return "phoBind(" + args[0] + ", " + args[1] + ", " + args[2] + ", " + args[3] + ")";
    if (prim_name == "unbind" && args.size() == 2)
        return "phoUnbind(" + args[0] + ", " + args[1] + ")";

    // Fallback: generic runtime call
    std::string s = "phoCallPrim(\"" + prim_name + "\", [";
    for (size_t i = 0; i < args.size(); i++) {
        if (i > 0) s += ", ";
        s += args[i];
    }
    return s + "])";
}

// --- Topological sort ---

std::vector<NodeId> SwiftCodegen::topo_sort(const Case& c) const {
    // Build adjacency: wire source -> wire target (node level)
    std::unordered_map<NodeId, int> in_degree;
    std::unordered_map<NodeId, std::vector<NodeId>> adj;

    for (auto& node : c.nodes) {
        in_degree[node.id] = 0;
    }

    for (auto& w : c.wires) {
        // Skip self-loops
        if (w.source.node_id == w.target.node_id) continue;
        adj[w.source.node_id].push_back(w.target.node_id);
        in_degree[w.target.node_id]++;
    }

    // Kahn's algorithm
    std::queue<NodeId> q;
    for (auto& node : c.nodes) {
        if (in_degree[node.id] == 0) q.push(node.id);
    }

    std::vector<NodeId> order;
    while (!q.empty()) {
        NodeId nid = q.front();
        q.pop();
        order.push_back(nid);
        for (NodeId dep : adj[nid]) {
            in_degree[dep]--;
            if (in_degree[dep] == 0) q.push(dep);
        }
    }

    return order;
}

// --- Case compilation ---

std::vector<std::string> SwiftCodegen::compile_case(
    const Project& project, const Case& c, const Method& method, int ind) {

    std::vector<std::string> lines;
    auto order = topo_sort(c);

    // Map each node output pin to a Swift variable name
    // Key: (node_id, output_index) -> var_name
    std::unordered_map<uint64_t, std::string> pin_vars;

    auto pin_key = [](NodeId nid, uint32_t idx) -> uint64_t {
        return (static_cast<uint64_t>(nid) << 32) | idx;
    };

    // Assign input bar output vars to parameter names
    const Node* input_bar = c.find_node(c.input_bar_id);
    if (input_bar) {
        for (uint32_t i = 0; i < method.num_inputs; i++) {
            pin_vars[pin_key(input_bar->id, i)] = "input" + std::to_string(i);
        }
    }

    for (NodeId nid : order) {
        const Node* node = c.find_node(nid);
        if (!node) continue;
        if (node->id == c.input_bar_id) continue;
        if (node->id == c.output_bar_id) continue;

        // Collect input variable names for this node
        std::vector<std::string> input_vars(node->num_inputs, "PhoValue.null");
        for (auto& w : c.wires) {
            if (w.target.node_id == nid && !w.is_execution) {
                auto it = pin_vars.find(pin_key(w.source.node_id, w.source.index));
                if (it != pin_vars.end() && w.target.index < node->num_inputs) {
                    input_vars[w.target.index] = it->second;
                }
            }
        }

        // Phase 12: fill optional defaults for unwired pins
        if (!node->input_defs.empty()) {
            for (size_t i = 0; i < node->input_defs.size() && i < input_vars.size(); i++) {
                if (node->input_defs[i].is_optional && input_vars[i] == "PhoValue.null") {
                    bool has_wire = false;
                    for (auto& w : c.wires) {
                        if (!w.is_execution && w.target.node_id == nid && w.target.index == static_cast<uint32_t>(i)) {
                            has_wire = true; break;
                        }
                    }
                    if (!has_wire) {
                        input_vars[i] = emit_literal(node->input_defs[i].default_value);
                    }
                }
            }
        }

        // Generate output variables
        std::vector<std::string> out_vars;
        for (uint32_t i = 0; i < node->num_outputs; i++) {
            std::string var = fresh_var("t");
            out_vars.push_back(var);
            pin_vars[pin_key(nid, i)] = var;
        }

        // Emit code based on node type
        switch (node->type) {
            case NodeType::Constant: {
                if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] +
                                    " = " + emit_literal(node->constant_value));
                }
                break;
            }

            case NodeType::Primitive: {
                std::string expr = emit_prim_call(node->name, input_vars);
                if (node->num_outputs == 0) {
                    lines.push_back(indent(ind) + "_ = " + expr);
                } else if (node->num_outputs == 1) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + expr);
                } else {
                    // Multi-output: emit as tuple
                    std::string tuple_var = fresh_var("r");
                    lines.push_back(indent(ind) + "let " + tuple_var + " = " + expr);
                    for (uint32_t i = 0; i < node->num_outputs; i++) {
                        lines.push_back(indent(ind) + "let " + out_vars[i] +
                                        " = " + tuple_var + "." + std::to_string(i));
                    }
                }

                // Control annotation: failure check
                if (node->control == ControlType::NextCaseOnFailure && node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "guard " + out_vars[0] +
                                    ".isTruthy else { break caseBlock }");
                }
                break;
            }

            case NodeType::MethodCall: {
                std::string func;
                if (!node->name.empty() && node->name[0] == '/') {
                    // Data-determined dispatch
                    func = "phoDispatch(" + input_vars[0] + ", \"" +
                           node->name.substr(1) + "\", [" ;
                    for (size_t i = 1; i < input_vars.size(); i++) {
                        if (i > 1) func += ", ";
                        func += input_vars[i];
                    }
                    func += "])";
                } else {
                    func = swift_func_name(node->name) + "(";
                    for (size_t i = 0; i < input_vars.size(); i++) {
                        if (i > 0) func += ", ";
                        func += input_vars[i];
                    }
                    func += ")";
                }

                if (node->num_outputs == 0) {
                    lines.push_back(indent(ind) + "_ = " + func);
                } else if (node->num_outputs == 1) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + func);
                } else {
                    std::string tuple_var = fresh_var("r");
                    lines.push_back(indent(ind) + "let " + tuple_var + " = " + func);
                    for (uint32_t i = 0; i < node->num_outputs; i++) {
                        lines.push_back(indent(ind) + "let " + out_vars[i] +
                                        " = " + tuple_var + "." + std::to_string(i));
                    }
                }
                break;
            }

            case NodeType::InstanceGenerator: {
                std::string cls_name = swift_class_name(node->name);
                std::string expr = cls_name + "(";
                for (size_t i = 0; i < input_vars.size(); i++) {
                    if (i > 0) expr += ", ";
                    expr += input_vars[i];
                }
                expr += ")";
                if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + expr);
                }
                break;
            }

            case NodeType::Get: {
                std::string attr_name = swift_var_name(node->name);
                // Output 0: pass-through object, Output 1: attribute value
                if (out_vars.size() >= 2) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + input_vars[0]);
                    lines.push_back(indent(ind) + "let " + out_vars[1] + " = " +
                                    input_vars[0] + "." + attr_name);
                } else if (out_vars.size() == 1) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " +
                                    input_vars[0] + "." + attr_name);
                }
                break;
            }

            case NodeType::Set: {
                std::string attr_name = swift_var_name(node->name);
                lines.push_back(indent(ind) + input_vars[0] + "." + attr_name +
                                " = " + input_vars[1]);
                if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + input_vars[0]);
                }
                break;
            }

            case NodeType::Evaluation: {
                // Phase 20: emit expression as Swift code
                if (node->num_outputs > 0) {
                    // Substitute input variables into expression
                    std::string expr = node->expression;
                    for (size_t i = 0; i < input_vars.size(); i++) {
                        // Replace positional names: a, b, c, ...
                        if (i < 26) {
                            std::string var_name(1, 'a' + static_cast<char>(i));
                            size_t pos = 0;
                            while ((pos = expr.find(var_name, pos)) != std::string::npos) {
                                bool word_boundary_before = (pos == 0 || !isalnum(expr[pos-1]));
                                bool word_boundary_after = (pos + 1 >= expr.size() || !isalnum(expr[pos+1]));
                                if (word_boundary_before && word_boundary_after) {
                                    expr.replace(pos, 1, input_vars[i]);
                                    pos += input_vars[i].size();
                                } else {
                                    pos++;
                                }
                            }
                        }
                    }
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + expr);
                }
                break;
            }

            case NodeType::Inject: {
                // Phase 21: dynamic dispatch
                std::string func = "phoDispatch(" + input_vars[0] + ", [";
                for (size_t i = 1; i < input_vars.size(); i++) {
                    if (i > 1) func += ", ";
                    func += input_vars[i];
                }
                func += "])";
                if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + func);
                } else {
                    lines.push_back(indent(ind) + "_ = " + func);
                }
                break;
            }

            case NodeType::Persistent: {
                // Phase 22: persistent read/write
                if (node->num_outputs > 0 && !input_vars.empty()) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] +
                                    " = phoPersistent(\"" + node->name + "\", " + input_vars[0] + ")");
                } else if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] +
                                    " = phoPersistentRead(\"" + node->name + "\")");
                }
                break;
            }

            case NodeType::LocalMethod: {
                // Phase 19: local method call
                std::string func = "phoLocal_" + sanitize_identifier(node->name) + "(";
                for (size_t i = 0; i < input_vars.size(); i++) {
                    if (i > 0) func += ", ";
                    func += input_vars[i];
                }
                func += ")";
                if (node->num_outputs > 0) {
                    lines.push_back(indent(ind) + "let " + out_vars[0] + " = " + func);
                } else {
                    lines.push_back(indent(ind) + "_ = " + func);
                }
                break;
            }

            default:
                lines.push_back(indent(ind) + "// unsupported node type: " + node->name);
                break;
        }
    }

    // Collect output bar inputs as return values
    const Node* output_bar = c.find_node(c.output_bar_id);
    if (output_bar && method.num_outputs > 0) {
        std::vector<std::string> ret_vars;
        for (uint32_t i = 0; i < method.num_outputs; i++) {
            std::string var = "PhoValue.null";
            for (auto& w : c.wires) {
                if (w.target.node_id == output_bar->id && w.target.index == i) {
                    auto it = pin_vars.find(pin_key(w.source.node_id, w.source.index));
                    if (it != pin_vars.end()) var = it->second;
                }
            }
            ret_vars.push_back(var);
        }

        if (ret_vars.size() == 1) {
            lines.push_back(indent(ind) + "return " + ret_vars[0]);
        } else {
            std::string ret = "return (";
            for (size_t i = 0; i < ret_vars.size(); i++) {
                if (i > 0) ret += ", ";
                ret += ret_vars[i];
            }
            ret += ")";
            lines.push_back(indent(ind) + ret);
        }
    }

    return lines;
}

// --- Method compilation ---

std::string SwiftCodegen::compile_method(const Project& project, const Method& method) {
    std::ostringstream out;

    std::string func_name = swift_func_name(method.name);

    // Function signature
    out << "func " << func_name << "(";
    for (uint32_t i = 0; i < method.num_inputs; i++) {
        if (i > 0) out << ", ";
        out << "_ input" << i << ": PhoValue";
    }
    out << ")";

    // Return type
    if (method.num_outputs == 0) {
        // void
    } else if (method.num_outputs == 1) {
        out << " -> PhoValue";
    } else {
        out << " -> (";
        for (uint32_t i = 0; i < method.num_outputs; i++) {
            if (i > 0) out << ", ";
            out << "PhoValue";
        }
        out << ")";
    }

    out << " {\n";

    if (method.cases.size() == 1) {
        // Single case: emit directly
        auto lines = compile_case(project, method.cases[0], method, 1);
        for (auto& line : lines) out << line << "\n";
    } else {
        // Multiple cases: try each in order
        // Use labeled block + break for case chaining
        for (size_t ci = 0; ci < method.cases.size(); ci++) {
            out << indent(1) << "caseBlock: do {\n";
            auto lines = compile_case(project, method.cases[ci], method, 2);
            for (auto& line : lines) out << line << "\n";
            out << indent(1) << "}\n";
            if (ci < method.cases.size() - 1) {
                out << indent(1) << "// fall through to next case\n";
            }
        }

        // If all cases fall through, return null
        if (method.num_outputs > 0) {
            if (method.num_outputs == 1) {
                out << indent(1) << "return PhoValue.null\n";
            } else {
                out << indent(1) << "return (";
                for (uint32_t i = 0; i < method.num_outputs; i++) {
                    if (i > 0) out << ", ";
                    out << "PhoValue.null";
                }
                out << ")\n";
            }
        }
    }

    out << "}\n";
    return out.str();
}

// --- Class compilation ---

std::string SwiftCodegen::compile_class(const Project& project, const ClassDef& cls) {
    std::ostringstream out;

    std::string cls_name = swift_class_name(cls.name);
    std::string parent = cls.parent_name.empty() ? "" : swift_class_name(cls.parent_name);

    // Phase 27: actor keyword
    if (cls.is_actor) {
        out << "actor " << cls_name;
    } else {
        out << "class " << cls_name;
    }
    if (!parent.empty()) out << ": " << parent;
    // Phase 25: protocol conformance
    for (size_t i = 0; i < cls.conforms_to.size(); i++) {
        out << ((!parent.empty() || i > 0) ? ", " : ": ");
        out << swift_class_name(cls.conforms_to[i]);
    }
    out << " {\n";

    // Instance attributes as properties
    for (auto& attr : cls.attributes) {
        if (attr.is_class_attr) continue;
        std::string var_name = swift_var_name(attr.name);
        std::string access_str;
        if (attr.access == Access::Private) access_str = "private ";
        else if (attr.access == Access::Protected) access_str = "internal "; // Swift has no 'protected'
        out << indent(1) << access_str << "var " << var_name << ": PhoValue = " << emit_literal(attr.default_value) << "\n";
    }

    // Class attributes as static properties
    for (auto& attr : cls.attributes) {
        if (!attr.is_class_attr) continue;
        std::string var_name = swift_var_name(attr.name);
        out << indent(1) << "static var " << var_name << ": PhoValue = " << emit_literal(attr.default_value) << "\n";
    }

    if (!cls.attributes.empty()) out << "\n";

    // Init
    bool has_init = false;
    for (auto& m : cls.methods) {
        if (m.name == "init") { has_init = true; break; }
    }
    if (!has_init) {
        // Default init
        if (parent.empty()) {
            out << indent(1) << "init() {}\n\n";
        } else {
            out << indent(1) << "override init() { super.init() }\n\n";
        }
    }

    // Methods
    for (auto& m : cls.methods) {
        bool is_override = false;
        if (!cls.parent_name.empty()) {
            const ClassDef* p = project.find_class(cls.parent_name);
            if (p && p->find_method(m.name)) is_override = true;
        }

        std::string func_name = swift_func_name(m.name);
        if (m.name == "init") func_name = "init";

        out << indent(1);
        if (is_override) out << "override ";
        out << "func " << func_name << "(";

        // Skip first input (self) for class methods
        uint32_t start_input = 1; // first input is self
        for (uint32_t i = start_input; i < m.num_inputs; i++) {
            if (i > start_input) out << ", ";
            out << "_ input" << i << ": PhoValue";
        }
        out << ")";

        if (m.num_outputs == 0) {
            // void
        } else if (m.num_outputs == 1) {
            out << " -> PhoValue";
        } else {
            out << " -> (";
            for (uint32_t i = 0; i < m.num_outputs; i++) {
                if (i > 0) out << ", ";
                out << "PhoValue";
            }
            out << ")";
        }

        out << " {\n";

        if (m.cases.size() == 1) {
            auto lines = compile_case(project, m.cases[0], m, 2);
            for (auto& line : lines) out << line << "\n";
        } else {
            for (size_t ci = 0; ci < m.cases.size(); ci++) {
                out << indent(2) << "caseBlock: do {\n";
                auto lines = compile_case(project, m.cases[ci], m, 3);
                for (auto& line : lines) out << line << "\n";
                out << indent(2) << "}\n";
            }
            if (m.num_outputs == 1) {
                out << indent(2) << "return PhoValue.null\n";
            }
        }

        out << indent(1) << "}\n\n";
    }

    out << "}\n";
    return out.str();
}

// --- Runtime ---

std::string SwiftCodegen::emit_runtime() const {
    std::ostringstream out;

    out << R"SWIFT(// PhographRuntime - minimal runtime for compiled Phograph programs
import Foundation

enum PhoValue {
    case null
    case integer(Int64)
    case real(Double)
    case boolean(Bool)
    case string(String)
    case list([PhoValue])
    case dict([String: PhoValue])
    case object(PhoObject)
    case error(String)

    var isTruthy: Bool {
        switch self {
        case .null: return false
        case .boolean(let b): return b
        case .integer(let i): return i != 0
        case .string(let s): return !s.isEmpty
        case .list(let l): return !l.isEmpty
        case .error: return false
        default: return true
        }
    }

    var asInteger: Int64 {
        switch self {
        case .integer(let i): return i
        case .real(let r): return Int64(r)
        default: return 0
        }
    }

    var asReal: Double {
        switch self {
        case .real(let r): return r
        case .integer(let i): return Double(i)
        default: return 0
        }
    }

    var asString: String {
        switch self {
        case .string(let s): return s
        case .integer(let i): return "\(i)"
        case .real(let r): return "\(r)"
        case .boolean(let b): return b ? "true" : "false"
        case .null: return "null"
        default: return "<\(self)>"
        }
    }

    var asBool: Bool {
        switch self {
        case .boolean(let b): return b
        default: return isTruthy
        }
    }
}

class PhoObject {
    var className: String
    var attrs: [String: PhoValue] = [:]
    init(_ name: String) { className = name }
}

// Arithmetic
func phoAdd(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x + y)
    case (.real(let x), .real(let y)): return .real(x + y)
    case (.integer(let x), .real(let y)): return .real(Double(x) + y)
    case (.real(let x), .integer(let y)): return .real(x + Double(y))
    default: return .error("add: type mismatch")
    }
}

func phoSub(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x - y)
    case (.real(let x), .real(let y)): return .real(x - y)
    case (.integer(let x), .real(let y)): return .real(Double(x) - y)
    case (.real(let x), .integer(let y)): return .real(x - Double(y))
    default: return .error("sub: type mismatch")
    }
}

func phoMul(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x * y)
    case (.real(let x), .real(let y)): return .real(x * y)
    case (.integer(let x), .real(let y)): return .real(Double(x) * y)
    case (.real(let x), .integer(let y)): return .real(x * Double(y))
    default: return .error("mul: type mismatch")
    }
}

func phoDiv(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)):
        if y == 0 { return .error("division by zero") }
        return .integer(x / y)
    case (.real(let x), .real(let y)):
        if y == 0 { return .error("division by zero") }
        return .real(x / y)
    case (.integer(let x), .real(let y)): return .real(Double(x) / y)
    case (.real(let x), .integer(let y)): return .real(x / Double(y))
    default: return .error("div: type mismatch")
    }
}

func phoMod(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)):
        if y == 0 { return .error("mod by zero") }
        return .integer(x % y)
    default: return .error("mod: type mismatch")
    }
}

func phoAbs(_ a: PhoValue) -> PhoValue {
    switch a {
    case .integer(let x): return .integer(abs(x))
    case .real(let x): return .real(abs(x))
    default: return .error("abs: type mismatch")
    }
}

func phoNegate(_ a: PhoValue) -> PhoValue {
    switch a {
    case .integer(let x): return .integer(-x)
    case .real(let x): return .real(-x)
    default: return .error("negate: type mismatch")
    }
}

func phoSqrt(_ a: PhoValue) -> PhoValue { .real(sqrt(a.asReal)) }
func phoRound(_ a: PhoValue) -> PhoValue { .integer(Int64(a.asReal.rounded())) }
func phoFloor(_ a: PhoValue) -> PhoValue { .integer(Int64(floor(a.asReal))) }
func phoCeil(_ a: PhoValue) -> PhoValue { .integer(Int64(ceil(a.asReal))) }

func phoClamp(_ v: PhoValue, _ lo: PhoValue, _ hi: PhoValue) -> PhoValue {
    let val = v.asReal, low = lo.asReal, high = hi.asReal
    return .real(Swift.min(Swift.max(val, low), high))
}

func phoMin(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(Swift.min(x, y))
    default: return .real(Swift.min(a.asReal, b.asReal))
    }
}

func phoMax(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(Swift.max(x, y))
    default: return .real(Swift.max(a.asReal, b.asReal))
    }
}

// Comparison
func phoEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .boolean(x == y)
    case (.real(let x), .real(let y)): return .boolean(x == y)
    case (.boolean(let x), .boolean(let y)): return .boolean(x == y)
    case (.string(let x), .string(let y)): return .boolean(x == y)
    case (.null, .null): return .boolean(true)
    default: return .boolean(false)
    }
}

func phoNotEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    if case .boolean(let b) = phoEqual(a, b) { return .boolean(!b) }
    return .boolean(true)
}

func phoLessThan(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal < b.asReal) }
func phoGreaterThan(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal > b.asReal) }
func phoLessEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal <= b.asReal) }
func phoGreaterEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal >= b.asReal) }

// Logic
func phoAnd(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asBool && b.asBool) }
func phoOr(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asBool || b.asBool) }
func phoNot(_ a: PhoValue) -> PhoValue { .boolean(!a.asBool) }

// String
func phoStringConcat(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .string(a.asString + b.asString) }
func phoStringLength(_ a: PhoValue) -> PhoValue { .integer(Int64(a.asString.count)) }
func phoToString(_ a: PhoValue) -> PhoValue { .string(a.asString) }

// List
func phoListCreate(_ elems: [PhoValue]) -> PhoValue { .list(elems) }
func phoListGet(_ list: PhoValue, _ idx: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .error("list-get: not a list") }
    let i = Int(idx.asInteger)
    guard i >= 0 && i < arr.count else { return .error("list-get: index out of bounds") }
    return arr[i]
}
func phoListSet(_ list: PhoValue, _ idx: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .list(var arr) = list else { return .error("list-set: not a list") }
    let i = Int(idx.asInteger)
    guard i >= 0 && i < arr.count else { return .error("list-set: index out of bounds") }
    arr[i] = val
    return .list(arr)
}
func phoListLength(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .integer(0) }
    return .integer(Int64(arr.count))
}
func phoListAppend(_ list: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .list(var arr) = list else { return .list([val]) }
    arr.append(val)
    return .list(arr)
}

// Type checks
func phoIsInteger(_ a: PhoValue) -> PhoValue { if case .integer = a { return .boolean(true) }; return .boolean(false) }
func phoIsReal(_ a: PhoValue) -> PhoValue { if case .real = a { return .boolean(true) }; return .boolean(false) }
func phoIsString(_ a: PhoValue) -> PhoValue { if case .string = a { return .boolean(true) }; return .boolean(false) }
func phoIsBoolean(_ a: PhoValue) -> PhoValue { if case .boolean = a { return .boolean(true) }; return .boolean(false) }
func phoIsList(_ a: PhoValue) -> PhoValue { if case .list = a { return .boolean(true) }; return .boolean(false) }
func phoIsNull(_ a: PhoValue) -> PhoValue { if case .null = a { return .boolean(true) }; return .boolean(false) }
func phoTypeOf(_ a: PhoValue) -> PhoValue {
    switch a {
    case .null: return .string("null")
    case .integer: return .string("integer")
    case .real: return .string("real")
    case .boolean: return .string("boolean")
    case .string: return .string("string")
    case .list: return .string("list")
    case .dict: return .string("dict")
    case .object: return .string("object")
    case .error: return .string("error")
    }
}

// I/O
func phoPrint(_ a: PhoValue) -> PhoValue {
    print(a.asString)
    return .null
}

// Generic prim call (fallback)
func phoCallPrim(_ name: String, _ args: [PhoValue]) -> PhoValue {
    print("WARNING: uncompiled primitive '\(name)' called at runtime")
    return .null
}

// Dynamic dispatch (fallback)
func phoDispatch(_ obj: PhoValue, _ method: String, _ args: [PhoValue]) -> PhoValue {
    print("WARNING: dynamic dispatch '\(method)' not compiled")
    return .null
}
)SWIFT";

    return out.str();
}

// --- Full project compilation ---

bool SwiftCodegen::compile(const Project& project, std::string& out_source,
                            std::vector<CodegenError>& errors) {
    temp_counter_ = 0;
    std::ostringstream out;

    // Header
    out << "// Generated by Phograph Compiler\n";
    out << "// Project: " << project.name << "\n\n";

    if (opts_.emit_imports) {
        out << "import Foundation\n\n";
    }

    // Runtime
    if (opts_.emit_runtime) {
        out << emit_runtime() << "\n";
    }

    // Classes
    for (auto& section : project.sections) {
        for (auto& cls : section.classes) {
            out << compile_class(project, cls) << "\n";
        }
    }

    // Universal methods
    std::string entry_name = opts_.emit_main ? (opts_.entry_method.empty() ? "main" : opts_.entry_method) : "";
    for (auto& section : project.sections) {
        for (auto& method : section.methods) {
            // Rename entry method to avoid conflict with @main struct
            if (opts_.emit_main && method.name == entry_name) {
                Method renamed = method;
                renamed.name = "pho_entry_" + method.name;
                out << compile_method(project, renamed) << "\n";
            } else {
                out << compile_method(project, method) << "\n";
            }
        }
    }

    // Entry point
    if (opts_.emit_main) {
        std::string entry = opts_.entry_method.empty() ? "main" : opts_.entry_method;
        const Method* main_method = project.find_method(entry);
        if (main_method) {
            std::string func = swift_func_name("pho_entry_" + entry);
            // Use @main struct for multi-file Swift compatibility
            out << "@main\nstruct PhographMain {\n";
            out << indent(1) << "static func main() {\n";
            if (main_method->num_outputs > 0) {
                out << indent(2) << "let _result = " << func << "()\n";
                out << indent(2) << "if case .integer(let code) = _result { exit(Int32(code)) }\n";
            } else {
                out << indent(2) << func << "()\n";
            }
            out << indent(1) << "}\n";
            out << "}\n";
        }
    }

    out_source = out.str();
    return errors.empty();
}

} // namespace pho
