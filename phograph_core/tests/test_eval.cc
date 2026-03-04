#include "../src/pho_value.h"
#include "../src/pho_graph.h"
#include "../src/pho_eval.h"
#include "../src/pho_prim.h"
#include "../src/pho_serial.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>

using namespace pho;

// Helper: build graph programmatically
static void test_simple_add() {
    printf("test_simple_add... ");

    register_all_phase1_prims();

    // Build: 3 + 4 = 7
    Project project;
    Section section;
    section.name = "main";

    Method method;
    method.name = "test-add";
    method.num_inputs = 0;
    method.num_outputs = 1;

    Case c;

    // Input bar (no inputs for this method)
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.name = "input";
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 0;
    c.input_bar_id = c.add_node(input_bar);

    // Constant 3
    Node const3;
    const3.type = NodeType::Constant;
    const3.name = "3";
    const3.num_inputs = 0;
    const3.num_outputs = 1;
    const3.constant_value = Value::integer(3);
    NodeId id3 = c.add_node(const3);

    // Constant 4
    Node const4;
    const4.type = NodeType::Constant;
    const4.name = "4";
    const4.num_inputs = 0;
    const4.num_outputs = 1;
    const4.constant_value = Value::integer(4);
    NodeId id4 = c.add_node(const4);

    // + node
    Node add;
    add.type = NodeType::Primitive;
    add.name = "+";
    add.num_inputs = 2;
    add.num_outputs = 1;
    NodeId id_add = c.add_node(add);

    // Output bar
    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.name = "output";
    output_bar.num_inputs = 1;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    // Wire: const3 -> add input 0
    c.add_wire({id3, 0, true}, {id_add, 0, false});
    // Wire: const4 -> add input 1
    c.add_wire({id4, 0, true}, {id_add, 1, false});
    // Wire: add output -> output bar input 0
    c.add_wire({id_add, 0, true}, {c.output_bar_id, 0, false});

    method.cases.push_back(std::move(c));
    section.methods.push_back(std::move(method));
    project.sections.push_back(std::move(section));

    Evaluator eval;
    auto result = eval.call_method(project, "test-add", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 7);

    printf("OK\n");
}

// Test: (3 + 4) * 2 = 14
static void test_3_plus_4_times_2() {
    printf("test_3_plus_4_times_2... ");

    Project project;
    Section section;
    section.name = "main";

    Method method;
    method.name = "test-expr";
    method.num_inputs = 0;
    method.num_outputs = 1;

    Case c;

    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 0;
    c.input_bar_id = c.add_node(input_bar);

    Node n3;
    n3.type = NodeType::Constant;
    n3.num_inputs = 0; n3.num_outputs = 1;
    n3.constant_value = Value::integer(3);
    NodeId id3 = c.add_node(n3);

    Node n4;
    n4.type = NodeType::Constant;
    n4.num_inputs = 0; n4.num_outputs = 1;
    n4.constant_value = Value::integer(4);
    NodeId id4 = c.add_node(n4);

    Node n2;
    n2.type = NodeType::Constant;
    n2.num_inputs = 0; n2.num_outputs = 1;
    n2.constant_value = Value::integer(2);
    NodeId id2 = c.add_node(n2);

    Node add;
    add.type = NodeType::Primitive;
    add.name = "+";
    add.num_inputs = 2; add.num_outputs = 1;
    NodeId id_add = c.add_node(add);

    Node mul;
    mul.type = NodeType::Primitive;
    mul.name = "*";
    mul.num_inputs = 2; mul.num_outputs = 1;
    NodeId id_mul = c.add_node(mul);

    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.num_inputs = 1; output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    c.add_wire({id3, 0, true}, {id_add, 0, false});
    c.add_wire({id4, 0, true}, {id_add, 1, false});
    c.add_wire({id_add, 0, true}, {id_mul, 0, false});
    c.add_wire({id2, 0, true}, {id_mul, 1, false});
    c.add_wire({id_mul, 0, true}, {c.output_bar_id, 0, false});

    method.cases.push_back(std::move(c));
    section.methods.push_back(std::move(method));
    project.sections.push_back(std::move(section));

    Evaluator eval;
    auto result = eval.call_method(project, "test-expr", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 14);

    printf("OK\n");
}

// Test: load from JSON and evaluate
static void test_json_load_eval() {
    printf("test_json_load_eval... ");

    // JSON for (3 + 4) * 2
    std::string json = R"({
        "name": "test-project",
        "sections": [{
            "name": "main",
            "methods": [{
                "name": "compute",
                "num_inputs": 0,
                "num_outputs": 1,
                "cases": [{
                    "nodes": [
                        {"id": 1, "type": "input-bar", "name": "input", "num_inputs": 0, "num_outputs": 0},
                        {"id": 2, "type": "constant", "name": "3", "num_inputs": 0, "num_outputs": 1, "value": 3},
                        {"id": 3, "type": "constant", "name": "4", "num_inputs": 0, "num_outputs": 1, "value": 4},
                        {"id": 4, "type": "constant", "name": "2", "num_inputs": 0, "num_outputs": 1, "value": 2},
                        {"id": 5, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1},
                        {"id": 6, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                        {"id": 7, "type": "output-bar", "name": "output", "num_inputs": 1, "num_outputs": 0}
                    ],
                    "wires": [
                        {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0},
                        {"id": 2, "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1},
                        {"id": 3, "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0},
                        {"id": 4, "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 1},
                        {"id": 5, "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0}
                    ]
                }]
            }]
        }]
    })";

    Project project;
    std::string error;
    bool ok = load_project_from_json(json, project, error);
    assert(ok);

    Evaluator eval;
    auto result = eval.call_method(project, "compute", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 14);

    printf("OK\n");
}

// Test: method calling method
static void test_method_call() {
    printf("test_method_call... ");

    std::string json = R"({
        "name": "test-method-call",
        "sections": [{
            "name": "main",
            "methods": [
                {
                    "name": "double-it",
                    "num_inputs": 1,
                    "num_outputs": 1,
                    "cases": [{
                        "nodes": [
                            {"id": 1, "type": "input-bar", "name": "input", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "constant", "name": "2", "num_inputs": 0, "num_outputs": 1, "value": 2},
                            {"id": 3, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                            {"id": 4, "type": "output-bar", "name": "output", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 2, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1},
                            {"id": 3, "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0}
                        ]
                    }]
                },
                {
                    "name": "main",
                    "num_inputs": 0,
                    "num_outputs": 1,
                    "cases": [{
                        "nodes": [
                            {"id": 1, "type": "input-bar", "name": "input", "num_inputs": 0, "num_outputs": 0},
                            {"id": 2, "type": "constant", "name": "5", "num_inputs": 0, "num_outputs": 1, "value": 5},
                            {"id": 3, "type": "method-call", "name": "double-it", "num_inputs": 1, "num_outputs": 1},
                            {"id": 4, "type": "output-bar", "name": "output", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 2, "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0}
                        ]
                    }]
                }
            ]
        }]
    })";

    Project project;
    std::string error;
    bool ok = load_project_from_json(json, project, error);
    assert(ok);

    Evaluator eval;
    auto result = eval.call_method(project, "main", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 10);

    printf("OK\n");
}

// Test: value types
static void test_value_types() {
    printf("test_value_types... ");

    // Null
    Value v = Value::null_val();
    assert(v.is_null());
    assert(!v.is_truthy());

    // Integer
    v = Value::integer(42);
    assert(v.is_integer());
    assert(v.as_integer() == 42);
    assert(v.is_truthy());

    // Real
    v = Value::real(3.14);
    assert(v.is_real());
    assert(std::abs(v.as_real() - 3.14) < 1e-10);

    // Boolean
    v = Value::boolean(true);
    assert(v.is_boolean());
    assert(v.as_boolean() == true);

    // String
    v = Value::string("hello");
    assert(v.is_string());
    assert(v.as_string()->str() == "hello");

    // List
    v = Value::list({Value::integer(1), Value::integer(2), Value::integer(3)});
    assert(v.is_list());
    assert(v.as_list()->size() == 3);
    assert(v.as_list()->at(0).as_integer() == 1);

    // Equality
    assert(Value::integer(5).equals(Value::integer(5)));
    assert(!Value::integer(5).equals(Value::integer(6)));
    assert(Value::integer(5).equals(Value::real(5.0))); // cross-type
    assert(Value::string("abc").equals(Value::string("abc")));

    // Comparison
    assert(Value::integer(3).compare(Value::integer(5)) < 0);
    assert(Value::integer(5).compare(Value::integer(3)) > 0);
    assert(Value::integer(5).compare(Value::integer(5)) == 0);

    // Error
    v = Value::error("something broke");
    assert(v.is_error());
    assert(v.as_error()->message() == "something broke");
    assert(!v.is_truthy());

    // Copy semantics
    Value a = Value::string("test");
    Value b = a;
    assert(b.is_string());
    assert(b.as_string()->str() == "test");

    // Move semantics
    Value c = std::move(a);
    assert(c.is_string());
    assert(c.as_string()->str() == "test");
    assert(a.is_null()); // moved-from

    printf("OK\n");
}

// Test: arithmetic primitives
static void test_arith_prims() {
    printf("test_arith_prims... ");

    auto& reg = PrimitiveRegistry::instance();

    // Division
    auto* div = reg.find("/");
    assert(div);
    auto r = div->fn({Value::integer(10), Value::integer(3)});
    assert(!r.failed);
    assert(r.outputs[0].is_real()); // 10/3 is not exact

    r = div->fn({Value::integer(10), Value::integer(2)});
    assert(r.outputs[0].is_integer());
    assert(r.outputs[0].as_integer() == 5);

    // div by zero
    r = div->fn({Value::integer(10), Value::integer(0)});
    assert(r.failed);

    // div-mod
    auto* dm = reg.find("div-mod");
    r = dm->fn({Value::integer(17), Value::integer(5)});
    assert(r.outputs.size() == 2);
    assert(r.outputs[0].as_integer() == 3);
    assert(r.outputs[1].as_integer() == 2);

    // sqrt
    auto* sq = reg.find("sqrt");
    r = sq->fn({Value::integer(9)});
    assert(std::abs(r.outputs[0].as_real() - 3.0) < 1e-10);

    // clamp
    auto* cl = reg.find("clamp");
    r = cl->fn({Value::integer(15), Value::integer(0), Value::integer(10)});
    assert(r.outputs[0].as_integer() == 10);

    printf("OK\n");
}

// Test: comparison and logic
static void test_compare_logic() {
    printf("test_compare_logic... ");

    auto& reg = PrimitiveRegistry::instance();

    auto* eq = reg.find("=");
    auto r = eq->fn({Value::integer(5), Value::integer(5)});
    assert(r.outputs[0].as_boolean() == true);

    r = eq->fn({Value::integer(5), Value::integer(6)});
    assert(r.outputs[0].as_boolean() == false);

    auto* lt = reg.find("<");
    r = lt->fn({Value::integer(3), Value::integer(5)});
    assert(r.outputs[0].as_boolean() == true);

    auto* and_op = reg.find("and");
    r = and_op->fn({Value::boolean(true), Value::boolean(false)});
    assert(r.outputs[0].as_boolean() == false);

    auto* not_op = reg.find("not");
    r = not_op->fn({Value::boolean(false)});
    assert(r.outputs[0].as_boolean() == true);

    printf("OK\n");
}

// Test: JSON parser
static void test_json_parser() {
    printf("test_json_parser... ");

    JsonValue val;
    std::string err;

    assert(parse_json("42", val, err));
    assert(val.type == JsonType::Number);
    assert(val.number == 42.0);

    assert(parse_json("\"hello\"", val, err));
    assert(val.str == "hello");

    assert(parse_json("[1, 2, 3]", val, err));
    assert(val.array.size() == 3);

    assert(parse_json("{\"a\": 1, \"b\": true}", val, err));
    assert(val.get_int("a") == 1);
    assert(val.get_bool("b") == true);

    assert(parse_json("null", val, err));
    assert(val.is_null());

    printf("OK\n");
}

// Test: display string
static void test_display() {
    printf("test_display... ");

    assert(Value::null_val().to_display_string() == "null");
    assert(Value::integer(42).to_display_string() == "42");
    assert(Value::boolean(true).to_display_string() == "true");
    assert(Value::string("hi").to_display_string() == "\"hi\"");

    auto list_val = Value::list({Value::integer(1), Value::integer(2)});
    assert(list_val.to_display_string() == "(1 2)");

    printf("OK\n");
}

int main() {
    printf("=== Phograph Phase 1 Tests ===\n");

    test_value_types();
    test_json_parser();
    test_display();
    test_simple_add();
    test_3_plus_4_times_2();
    test_json_load_eval();
    test_method_call();
    test_arith_prims();
    test_compare_logic();

    printf("\n=== All Phase 1 tests passed! ===\n");
    return 0;
}
