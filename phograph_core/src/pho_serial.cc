#include "pho_serial.h"
#include <cctype>
#include <cassert>
#include <sstream>

namespace pho {

// ---- Minimal JSON parser ----

struct JsonParser {
    const std::string& input;
    size_t pos = 0;
    std::string error;

    JsonParser(const std::string& s) : input(s) {}

    void skip_ws() {
        while (pos < input.size() && std::isspace(input[pos])) pos++;
    }

    bool at_end() const { return pos >= input.size(); }
    char peek() const { return pos < input.size() ? input[pos] : '\0'; }
    char advance() { return pos < input.size() ? input[pos++] : '\0'; }

    bool expect(char c) {
        skip_ws();
        if (peek() == c) { advance(); return true; }
        error = std::string("expected '") + c + "' at position " + std::to_string(pos);
        return false;
    }

    bool match(const char* s) {
        size_t len = std::strlen(s);
        if (pos + len <= input.size() && input.compare(pos, len, s) == 0) {
            pos += len;
            return true;
        }
        return false;
    }

    bool parse_value(JsonValue& out) {
        skip_ws();
        if (at_end()) { error = "unexpected end of input"; return false; }

        char c = peek();
        if (c == '"') return parse_string(out);
        if (c == '{') return parse_object(out);
        if (c == '[') return parse_array(out);
        if (c == 't' || c == 'f') return parse_bool(out);
        if (c == 'n') return parse_null(out);
        if (c == '-' || std::isdigit(c)) return parse_number(out);

        error = std::string("unexpected character '") + c + "' at position " + std::to_string(pos);
        return false;
    }

    bool parse_string(JsonValue& out) {
        if (!expect('"')) return false;
        out.type = JsonType::String;
        out.str.clear();
        while (!at_end() && peek() != '"') {
            if (peek() == '\\') {
                advance();
                char esc = advance();
                switch (esc) {
                    case '"': out.str += '"'; break;
                    case '\\': out.str += '\\'; break;
                    case '/': out.str += '/'; break;
                    case 'n': out.str += '\n'; break;
                    case 'r': out.str += '\r'; break;
                    case 't': out.str += '\t'; break;
                    case 'b': out.str += '\b'; break;
                    case 'f': out.str += '\f'; break;
                    case 'u': {
                        for (int i = 0; i < 4 && !at_end(); i++) advance();
                        out.str += '?';
                        break;
                    }
                    default: out.str += esc; break;
                }
            } else {
                out.str += advance();
            }
        }
        if (!expect('"')) return false;
        return true;
    }

    bool parse_number(JsonValue& out) {
        size_t start = pos;
        if (peek() == '-') advance();
        while (!at_end() && std::isdigit(peek())) advance();
        if (!at_end() && peek() == '.') {
            advance();
            while (!at_end() && std::isdigit(peek())) advance();
        }
        if (!at_end() && (peek() == 'e' || peek() == 'E')) {
            advance();
            if (!at_end() && (peek() == '+' || peek() == '-')) advance();
            while (!at_end() && std::isdigit(peek())) advance();
        }
        out.type = JsonType::Number;
        out.number = std::stod(input.substr(start, pos - start));
        return true;
    }

    bool parse_bool(JsonValue& out) {
        if (match("true")) { out.type = JsonType::Boolean; out.boolean = true; return true; }
        if (match("false")) { out.type = JsonType::Boolean; out.boolean = false; return true; }
        error = "expected true or false at position " + std::to_string(pos);
        return false;
    }

    bool parse_null(JsonValue& out) {
        if (match("null")) { out.type = JsonType::Null; return true; }
        error = "expected null at position " + std::to_string(pos);
        return false;
    }

    bool parse_array(JsonValue& out) {
        if (!expect('[')) return false;
        out.type = JsonType::Array;
        out.array.clear();
        skip_ws();
        if (peek() == ']') { advance(); return true; }
        while (true) {
            JsonValue elem;
            if (!parse_value(elem)) return false;
            out.array.push_back(std::move(elem));
            skip_ws();
            if (peek() == ',') { advance(); continue; }
            break;
        }
        return expect(']');
    }

    bool parse_object(JsonValue& out) {
        if (!expect('{')) return false;
        out.type = JsonType::Object;
        out.object.clear();
        skip_ws();
        if (peek() == '}') { advance(); return true; }
        while (true) {
            JsonValue key;
            if (!parse_string(key)) return false;
            skip_ws();
            if (!expect(':')) return false;
            JsonValue val;
            if (!parse_value(val)) return false;
            out.object.emplace_back(key.str, std::move(val));
            skip_ws();
            if (peek() == ',') { advance(); continue; }
            break;
        }
        return expect('}');
    }
};

bool parse_json(const std::string& input, JsonValue& out, std::string& error) {
    JsonParser parser(input);
    if (!parser.parse_value(out)) {
        error = parser.error;
        return false;
    }
    return true;
}

// ---- Graph loading from JSON ----

static ControlType parse_control(const std::string& s) {
    if (s == "next-case-on-failure") return ControlType::NextCaseOnFailure;
    if (s == "next-case-on-success") return ControlType::NextCaseOnSuccess;
    if (s == "continue-on-failure") return ControlType::ContinueOnFailure;
    if (s == "continue-on-success") return ControlType::ContinueOnSuccess;
    if (s == "fail-on-failure") return ControlType::FailOnFailure;
    if (s == "fail-on-success") return ControlType::FailOnSuccess;
    if (s == "terminate-on-failure") return ControlType::TerminateOnFailure;
    if (s == "terminate-on-success") return ControlType::TerminateOnSuccess;
    if (s == "finish-on-failure") return ControlType::FinishOnFailure;
    if (s == "finish-on-success") return ControlType::FinishOnSuccess;
    return ControlType::None;
}

static NodeType parse_node_type(const std::string& s) {
    if (s == "primitive") return NodeType::Primitive;
    if (s == "method-call" || s == "method_call") return NodeType::MethodCall;
    if (s == "constant") return NodeType::Constant;
    if (s == "input-bar" || s == "input_bar") return NodeType::InputBar;
    if (s == "output-bar" || s == "output_bar") return NodeType::OutputBar;
    if (s == "get") return NodeType::Get;
    if (s == "set") return NodeType::Set;
    if (s == "instance-generator" || s == "instance_generator") return NodeType::InstanceGenerator;
    if (s == "persistent") return NodeType::Persistent;
    if (s == "local-method" || s == "local_method") return NodeType::LocalMethod;
    if (s == "evaluation") return NodeType::Evaluation;
    if (s == "inject") return NodeType::Inject;
    return NodeType::Primitive;
}

static Access parse_access(const std::string& s) {
    if (s == "private") return Access::Private;
    if (s == "protected") return Access::Protected;
    return Access::Public;
}

static Value parse_constant_value(const JsonValue& jv) {
    if (jv.is_null()) return Value::null_val();
    if (jv.type == JsonType::Boolean) return Value::boolean(jv.boolean);
    if (jv.type == JsonType::Number) {
        double n = jv.number;
        if (n == std::floor(n) && n >= -9007199254740992.0 && n <= 9007199254740992.0) {
            return Value::integer(static_cast<int64_t>(n));
        }
        return Value::real(n);
    }
    if (jv.type == JsonType::String) return Value::string(jv.str);
    if (jv.type == JsonType::Array) {
        std::vector<Value> elems;
        for (auto& e : jv.array) {
            elems.push_back(parse_constant_value(e));
        }
        return Value::list(std::move(elems));
    }
    if (jv.type == JsonType::Object) {
        auto type_str = jv.get_string("type");
        auto* val = jv.get("value");
        if (!type_str.empty() && val) {
            return parse_constant_value(*val);
        }
        return Value::null_val();
    }
    return Value::null_val();
}

// Forward declaration for recursive local method loading
static bool load_case(const JsonValue& jc, Case& out_case, std::string& error);

static bool load_method_from_json(const JsonValue& jm, Method& method, std::string& error) {
    method.name = jm.get_string("name");
    method.num_inputs = static_cast<uint32_t>(jm.get_int("num_inputs"));
    method.num_outputs = static_cast<uint32_t>(jm.get_int("num_outputs"));
    auto access_str = jm.get_string("access", "");
    if (!access_str.empty()) method.access = parse_access(access_str);

    auto* cases = jm.get("cases");
    if (cases && cases->is_array()) {
        for (auto& jc : cases->array) {
            Case c;
            if (!load_case(jc, c, error)) return false;
            method.cases.push_back(std::move(c));
        }
    }
    return true;
}

static bool load_case(const JsonValue& jc, Case& out_case, std::string& error) {
    auto* nodes = jc.get("nodes");
    if (!nodes || !nodes->is_array()) { error = "case missing nodes array"; return false; }

    for (auto& jn : nodes->array) {
        Node node;
        node.id = static_cast<NodeId>(jn.get_int("id"));
        node.type = parse_node_type(jn.get_string("type"));
        node.name = jn.get_string("name");
        node.num_inputs = static_cast<uint32_t>(jn.get_int("num_inputs"));
        node.num_outputs = static_cast<uint32_t>(jn.get_int("num_outputs"));

        auto ctrl_str = jn.get_string("control");
        if (!ctrl_str.empty()) node.control = parse_control(ctrl_str);

        auto* cv = jn.get("value");
        if (cv) node.constant_value = parse_constant_value(*cv);

        // Phase 11: execution flags
        node.has_execution_in = jn.get_bool("has_execution_in", false);
        node.has_execution_out = jn.get_bool("has_execution_out", false);

        // Phase 12: input definitions
        auto* input_defs = jn.get("input_defs");
        if (input_defs && input_defs->is_array()) {
            for (auto& jpd : input_defs->array) {
                PinDef pd;
                pd.name = jpd.get_string("name");
                pd.is_optional = jpd.get_bool("optional", false);
                pd.is_hot = jpd.get_bool("hot", true);
                auto* def_val = jpd.get("default");
                if (def_val) pd.default_value = parse_constant_value(*def_val);
                node.input_defs.push_back(std::move(pd));
            }
        }

        // Phase 15/16: annotations
        auto ann_str = jn.get_string("annotation");
        if (ann_str == "loop") node.is_loop = true;
        if (ann_str == "listMap") node.list_map = true;
        if (ann_str == "partition") node.partition = true;

        // Phase 17: try
        node.has_try = jn.get_bool("has_try", false);
        auto error_pin = jn.get_int("error_out_pin", -1);
        if (error_pin >= 0) node.error_out_pin = static_cast<uint32_t>(error_pin);

        // Phase 20: expression
        auto expr_str = jn.get_string("expression");
        if (!expr_str.empty()) node.expression = expr_str;

        // Phase 19: local method body
        auto* local_method_json = jn.get("local_method");
        if (local_method_json && local_method_json->is_object()) {
            auto lm = std::make_shared<Method>();
            if (!load_method_from_json(*local_method_json, *lm, error)) return false;
            node.local_method = lm;
        }

        if (node.id >= out_case.next_node_id) out_case.next_node_id = node.id + 1;

        if (node.type == NodeType::InputBar) out_case.input_bar_id = node.id;
        if (node.type == NodeType::OutputBar) out_case.output_bar_id = node.id;

        out_case.nodes.push_back(std::move(node));
    }

    auto* wires = jc.get("wires");
    if (wires && wires->is_array()) {
        for (auto& jw : wires->array) {
            Wire w;
            w.id = static_cast<WireId>(jw.get_int("id"));
            w.source.node_id = static_cast<NodeId>(jw.get_int("source_node"));
            w.source.index = static_cast<uint32_t>(jw.get_int("source_pin"));
            w.source.is_output = true;
            w.target.node_id = static_cast<NodeId>(jw.get_int("target_node"));
            w.target.index = static_cast<uint32_t>(jw.get_int("target_pin"));
            w.target.is_output = false;
            // Phase 11: execution wire flag
            w.is_execution = jw.get_bool("is_execution", false);

            if (w.id >= out_case.next_wire_id) out_case.next_wire_id = w.id + 1;
            out_case.wires.push_back(w);
        }
    }

    // Phase 18: shift registers
    auto* sr_arr = jc.get("shift_registers");
    if (sr_arr && sr_arr->is_array()) {
        for (auto& jsr : sr_arr->array) {
            ShiftRegister sr;
            sr.input_pin = static_cast<uint32_t>(jsr.get_int("input_pin"));
            sr.output_pin = static_cast<uint32_t>(jsr.get_int("output_pin"));
            auto* init_val = jsr.get("initial");
            if (init_val) sr.initial = parse_constant_value(*init_val);
            out_case.shift_registers.push_back(std::move(sr));
        }
    }

    // Phase 23: case guards
    auto* guards_arr = jc.get("guards");
    if (guards_arr && guards_arr->is_array()) {
        for (auto& jg : guards_arr->array) {
            CaseGuard guard;
            guard.pin = static_cast<uint32_t>(jg.get_int("pin"));
            auto kind_str = jg.get_string("kind", "wildcard");
            if (kind_str == "type") guard.kind = CaseGuard::TypeMatch;
            else if (kind_str == "value") guard.kind = CaseGuard::ValueMatch;
            else guard.kind = CaseGuard::Wildcard;
            guard.match_type = jg.get_string("match_type");
            auto* mv = jg.get("match_value");
            if (mv) guard.match_val = parse_constant_value(*mv);
            out_case.guards.push_back(std::move(guard));
        }
    }

    return true;
}

bool load_project_from_json(const std::string& json, Project& out_project, std::string& out_error) {
    JsonValue root;
    if (!parse_json(json, root, out_error)) return false;

    if (!root.is_object()) { out_error = "root must be a JSON object"; return false; }

    out_project.name = root.get_string("name", "untitled");

    auto* sections = root.get("sections");
    if (!sections || !sections->is_array()) {
        out_error = "project missing sections array";
        return false;
    }

    for (auto& js : sections->array) {
        Section section;
        section.name = js.get_string("name", "main");

        auto* methods = js.get("methods");
        if (methods && methods->is_array()) {
            for (auto& jm : methods->array) {
                Method method;
                if (!load_method_from_json(jm, method, out_error)) return false;
                section.methods.push_back(std::move(method));
            }
        }

        // Load classes
        auto* classes = js.get("classes");
        if (classes && classes->is_array()) {
            for (auto& jcls : classes->array) {
                ClassDef cls;
                cls.name = jcls.get_string("name");
                cls.parent_name = jcls.get_string("parent", "");
                cls.is_actor = jcls.get_bool("is_actor", false);

                // Phase 25: protocol conformance
                auto* conforms = jcls.get("conforms_to");
                if (conforms && conforms->is_array()) {
                    for (auto& cp : conforms->array) {
                        if (cp.is_string()) cls.conforms_to.push_back(cp.str);
                    }
                }

                auto* attrs = jcls.get("attributes");
                if (attrs && attrs->is_array()) {
                    for (auto& ja : attrs->array) {
                        AttributeDef attr;
                        attr.name = ja.get_string("name");
                        attr.is_class_attr = ja.get_bool("class_attr", false);
                        auto access_str = ja.get_string("access", "");
                        if (!access_str.empty()) attr.access = parse_access(access_str);
                        auto* def_val = ja.get("default");
                        if (def_val) attr.default_value = parse_constant_value(*def_val);
                        cls.attributes.push_back(std::move(attr));
                    }
                }

                auto* cls_methods = jcls.get("methods");
                if (cls_methods && cls_methods->is_array()) {
                    for (auto& jm : cls_methods->array) {
                        Method method;
                        if (!load_method_from_json(jm, method, out_error)) return false;
                        method.class_name = cls.name;
                        cls.methods.push_back(std::move(method));
                    }
                }

                section.classes.push_back(std::move(cls));
            }
        }

        // Phase 25: protocols
        auto* protocols = js.get("protocols");
        if (protocols && protocols->is_array()) {
            for (auto& jp : protocols->array) {
                ProtocolDef pdef;
                pdef.name = jp.get_string("name");
                auto* req_methods = jp.get("required_methods");
                if (req_methods && req_methods->is_array()) {
                    for (auto& rm : req_methods->array) {
                        if (rm.is_string()) pdef.required_methods.push_back(rm.str);
                    }
                }
                section.protocols.push_back(std::move(pdef));
            }
        }

        out_project.sections.push_back(std::move(section));
    }

    return true;
}

} // namespace pho
