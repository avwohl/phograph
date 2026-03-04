#include "../src/pho_value.h"
#include "../src/pho_graph.h"
#include "../src/pho_eval.h"
#include "../src/pho_prim.h"
#include "../src/pho_serial.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace pho;

static bool prims_registered = false;
static void ensure_prims() {
    if (!prims_registered) {
        register_all_prims();
        prims_registered = true;
    }
}

// Test string primitives
static void test_string_prims() {
    printf("test_string_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    // concat
    auto r = reg.find("concat")->fn({Value::string("hello"), Value::string(" world")});
    assert(r.outputs[0].as_string()->str() == "hello world");

    // length
    r = reg.find("length")->fn({Value::string("hello")});
    assert(r.outputs[0].as_integer() == 5);

    // uppercase/lowercase
    r = reg.find("uppercase")->fn({Value::string("hello")});
    assert(r.outputs[0].as_string()->str() == "HELLO");

    r = reg.find("lowercase")->fn({Value::string("HELLO")});
    assert(r.outputs[0].as_string()->str() == "hello");

    // split
    r = reg.find("string-split")->fn({Value::string("a,b,c"), Value::string(",")});
    assert(r.outputs[0].as_list()->size() == 3);
    assert(r.outputs[0].as_list()->at(0).as_string()->str() == "a");

    // trim
    r = reg.find("string-trim")->fn({Value::string("  hello  ")});
    assert(r.outputs[0].as_string()->str() == "hello");

    // replace
    r = reg.find("string-replace")->fn({
        Value::string("hello world"), Value::string("world"), Value::string("there")
    });
    assert(r.outputs[0].as_string()->str() == "hello there");

    // starts-with / ends-with
    r = reg.find("string-starts-with?")->fn({Value::string("hello"), Value::string("hel")});
    assert(r.outputs[0].as_boolean() == true);

    r = reg.find("string-ends-with?")->fn({Value::string("hello"), Value::string("llo")});
    assert(r.outputs[0].as_boolean() == true);

    // search
    r = reg.find("string-search")->fn({Value::string("hello world"), Value::string("world")});
    assert(r.outputs[0].as_integer() == 7);

    // char-at (1-indexed)
    r = reg.find("char-at")->fn({Value::string("abc"), Value::integer(2)});
    assert(r.outputs[0].as_string()->str() == "b");

    printf("OK\n");
}

// Test list primitives
static void test_list_prims() {
    printf("test_list_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    auto list = Value::list({Value::integer(10), Value::integer(20), Value::integer(30)});

    // get-nth (1-indexed)
    auto r = reg.find("get-nth")->fn({list, Value::integer(2)});
    assert(r.outputs[0].as_integer() == 20);

    // out of bounds -> failure
    r = reg.find("get-nth")->fn({list, Value::integer(5)});
    assert(r.failed);

    // first/rest
    r = reg.find("first")->fn({list});
    assert(r.outputs[0].as_integer() == 10);

    r = reg.find("rest")->fn({list});
    assert(r.outputs[0].as_list()->size() == 2);

    // length
    r = reg.find("length")->fn({list});
    assert(r.outputs[0].as_integer() == 3);

    // append
    auto list2 = Value::list({Value::integer(40), Value::integer(50)});
    r = reg.find("append")->fn({list, list2});
    assert(r.outputs[0].as_list()->size() == 5);

    // reverse
    r = reg.find("reverse")->fn({list});
    assert(r.outputs[0].as_list()->at(0).as_integer() == 30);

    // sort
    auto unsorted = Value::list({Value::integer(3), Value::integer(1), Value::integer(2)});
    r = reg.find("sort")->fn({unsorted});
    assert(r.outputs[0].as_list()->at(0).as_integer() == 1);
    assert(r.outputs[0].as_list()->at(1).as_integer() == 2);
    assert(r.outputs[0].as_list()->at(2).as_integer() == 3);

    // contains?
    r = reg.find("contains?")->fn({list, Value::integer(20)});
    assert(r.outputs[0].as_boolean() == true);
    r = reg.find("contains?")->fn({list, Value::integer(99)});
    assert(r.outputs[0].as_boolean() == false);

    // zip
    auto a = Value::list({Value::integer(1), Value::integer(2)});
    auto b = Value::list({Value::string("a"), Value::string("b")});
    r = reg.find("zip")->fn({a, b});
    assert(r.outputs[0].as_list()->size() == 2);
    assert(r.outputs[0].as_list()->at(0).as_list()->at(0).as_integer() == 1);

    // unique
    auto dup = Value::list({Value::integer(1), Value::integer(2), Value::integer(1), Value::integer(3)});
    r = reg.find("unique")->fn({dup});
    assert(r.outputs[0].as_list()->size() == 3);

    printf("OK\n");
}

// Test dict primitives
static void test_dict_prims() {
    printf("test_dict_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    // Create and set
    auto r = reg.find("dict-create")->fn({});
    auto dict = r.outputs[0];

    r = reg.find("dict-set")->fn({dict, Value::string("name"), Value::string("Alice")});
    dict = r.outputs[0];

    r = reg.find("dict-set")->fn({dict, Value::string("age"), Value::integer(30)});
    dict = r.outputs[0];

    // Get
    r = reg.find("dict-get")->fn({dict, Value::string("name")});
    assert(r.outputs[0].as_string()->str() == "Alice");

    // Has
    r = reg.find("dict-has?")->fn({dict, Value::string("name")});
    assert(r.outputs[0].as_boolean() == true);

    r = reg.find("dict-has?")->fn({dict, Value::string("missing")});
    assert(r.outputs[0].as_boolean() == false);

    // Size
    r = reg.find("dict-size")->fn({dict});
    assert(r.outputs[0].as_integer() == 2);

    // Keys
    r = reg.find("dict-keys")->fn({dict});
    assert(r.outputs[0].as_list()->size() == 2);

    // Get missing -> failure
    r = reg.find("dict-get")->fn({dict, Value::string("missing")});
    assert(r.failed);

    // Get with default
    r = reg.find("dict-get-default")->fn({dict, Value::string("missing"), Value::integer(0)});
    assert(r.outputs[0].as_integer() == 0);

    // Remove
    r = reg.find("dict-remove")->fn({dict, Value::string("age")});
    r = reg.find("dict-size")->fn({r.outputs[0]});
    assert(r.outputs[0].as_integer() == 1);

    printf("OK\n");
}

// Test type primitives
static void test_type_prims() {
    printf("test_type_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    auto r = reg.find("integer?")->fn({Value::integer(42)});
    assert(r.outputs[0].as_boolean() == true);

    r = reg.find("integer?")->fn({Value::real(3.14)});
    assert(r.outputs[0].as_boolean() == false);

    r = reg.find("type-of")->fn({Value::string("hello")});
    assert(r.outputs[0].as_string()->str() == "string");

    r = reg.find("type-of")->fn({Value::null_val()});
    assert(r.outputs[0].as_string()->str() == "null");

    r = reg.find("null?")->fn({Value::null_val()});
    assert(r.outputs[0].as_boolean() == true);

    printf("OK\n");
}

// Test data primitives
static void test_data_prims() {
    printf("test_data_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    auto r = reg.find("data-create")->fn({Value::integer(4)});
    auto data = r.outputs[0];
    assert(data.as_data()->length() == 4);

    // From list
    auto bytes = Value::list({Value::integer(72), Value::integer(101), Value::integer(108)});
    r = reg.find("data-from-list")->fn({bytes});
    data = r.outputs[0];
    assert(data.as_data()->length() == 3);

    // To string
    r = reg.find("data-to-string")->fn({data, Value::string("utf8")});
    assert(r.outputs[0].as_string()->str() == "Hel");

    // Get byte
    r = reg.find("data-get-byte")->fn({data, Value::integer(0)});
    assert(r.outputs[0].as_integer() == 72);

    printf("OK\n");
}

// Test error primitives
static void test_error_prims() {
    printf("test_error_prims... ");
    ensure_prims();
    auto& reg = PrimitiveRegistry::instance();

    auto r = reg.find("error-create")->fn({Value::string("something failed")});
    assert(r.outputs[0].is_error());
    assert(r.outputs[0].as_error()->message() == "something failed");

    r = reg.find("error-message")->fn({r.outputs[0]});
    assert(r.outputs[0].as_string()->str() == "something failed");

    r = reg.find("error?")->fn({Value::error("test")});
    assert(r.outputs[0].as_boolean() == true);

    r = reg.find("error?")->fn({Value::integer(42)});
    assert(r.outputs[0].as_boolean() == false);

    printf("OK\n");
}

// Test: multiple cases (if-then-else pattern)
// Method "abs-val" with 2 cases:
//   Case 1: if n >= 0, return n
//   Case 2: return -n (negate)
static void test_multi_case() {
    printf("test_multi_case... ");
    ensure_prims();

    std::string json = R"({
        "name": "test-cases",
        "sections": [{
            "name": "main",
            "methods": [{
                "name": "abs-val",
                "num_inputs": 1,
                "num_outputs": 1,
                "cases": [
                    {
                        "nodes": [
                            {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 0},
                            {"id": 3, "type": "primitive", "name": ">=", "num_inputs": 2, "num_outputs": 1, "control": "next-case-on-failure"},
                            {"id": 4, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 2, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1},
                            {"id": 3, "source_node": 1, "source_pin": 0, "target_node": 4, "target_pin": 0}
                        ]
                    },
                    {
                        "nodes": [
                            {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 0},
                            {"id": 3, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1},
                            {"id": 4, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 2, "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 1},
                            {"id": 3, "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 0}
                        ]
                    }
                ]
            }]
        }]
    })";

    Project project;
    std::string error;
    assert(load_project_from_json(json, project, error));

    Evaluator eval;

    // Positive input: should return as-is
    auto result = eval.call_method(project, "abs-val", {Value::integer(5)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs[0].as_integer() == 5);

    // Negative input: should negate
    result = eval.call_method(project, "abs-val", {Value::integer(-3)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs[0].as_integer() == 3);

    printf("OK\n");
}

// Test: evaluator handles comparison controls
// The >= primitive returns boolean. We need to check that when the boolean is false,
// the control annotation "next-case-on-failure" triggers.
// Actually, comparison prims return true/false, and the control should be triggered
// based on the truthiness of the result. Let me adjust the evaluator to handle this.
// For now, this tests the multi-case pattern with the >= check.

// Test: fibonacci via method recursion + cases
static void test_fibonacci_recursive() {
    printf("test_fibonacci_recursive... ");
    ensure_prims();

    // fib(n):
    //   Case 1: if n <= 1, return n
    //   Case 2: return fib(n-1) + fib(n-2)
    std::string json = R"({
        "name": "test-fib",
        "sections": [{
            "name": "main",
            "methods": [{
                "name": "fib",
                "num_inputs": 1,
                "num_outputs": 1,
                "cases": [
                    {
                        "nodes": [
                            {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 1},
                            {"id": 3, "type": "primitive", "name": "<=", "num_inputs": 2, "num_outputs": 1, "control": "next-case-on-failure"},
                            {"id": 4, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 2, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 1},
                            {"id": 3, "source_node": 1, "source_pin": 0, "target_node": 4, "target_pin": 0}
                        ]
                    },
                    {
                        "nodes": [
                            {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 1},
                            {"id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 2},
                            {"id": 4, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1},
                            {"id": 5, "type": "primitive", "name": "-", "num_inputs": 2, "num_outputs": 1},
                            {"id": 6, "type": "method-call", "name": "fib", "num_inputs": 1, "num_outputs": 1},
                            {"id": 7, "type": "method-call", "name": "fib", "num_inputs": 1, "num_outputs": 1},
                            {"id": 8, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1},
                            {"id": 9, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 4, "target_pin": 0},
                            {"id": 2, "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 1},
                            {"id": 3, "source_node": 1, "source_pin": 0, "target_node": 5, "target_pin": 0},
                            {"id": 4, "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1},
                            {"id": 5, "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0},
                            {"id": 6, "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0},
                            {"id": 7, "source_node": 6, "source_pin": 0, "target_node": 8, "target_pin": 0},
                            {"id": 8, "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 1},
                            {"id": 9, "source_node": 8, "source_pin": 0, "target_node": 9, "target_pin": 0}
                        ]
                    }
                ]
            }]
        }]
    })";

    Project project;
    std::string error;
    assert(load_project_from_json(json, project, error));

    Evaluator eval;

    // fib(0) = 0
    auto r = eval.call_method(project, "fib", {Value::integer(0)});
    assert(r.status == EvalStatus::Success);
    assert(r.outputs[0].as_integer() == 0);

    // fib(1) = 1
    r = eval.call_method(project, "fib", {Value::integer(1)});
    assert(r.status == EvalStatus::Success);
    assert(r.outputs[0].as_integer() == 1);

    // fib(10) = 55
    r = eval.call_method(project, "fib", {Value::integer(10)});
    assert(r.status == EvalStatus::Success);
    assert(r.outputs[0].as_integer() == 55);

    printf("OK\n");
}

// Test: dict operations in a graph
static void test_dict_in_graph() {
    printf("test_dict_in_graph... ");
    ensure_prims();

    // Create a dict, set a key, get the key
    std::string json = R"({
        "name": "test-dict-graph",
        "sections": [{
            "name": "main",
            "methods": [{
                "name": "test-dict",
                "num_inputs": 0,
                "num_outputs": 1,
                "cases": [{
                    "nodes": [
                        {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 0},
                        {"id": 2, "type": "primitive", "name": "dict-create", "num_inputs": 0, "num_outputs": 1},
                        {"id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": "x"},
                        {"id": 4, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 42},
                        {"id": 5, "type": "primitive", "name": "dict-set", "num_inputs": 3, "num_outputs": 1},
                        {"id": 6, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": "x"},
                        {"id": 7, "type": "primitive", "name": "dict-get", "num_inputs": 2, "num_outputs": 1},
                        {"id": 8, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                    ],
                    "wires": [
                        {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 5, "target_pin": 0},
                        {"id": 2, "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 1},
                        {"id": 3, "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 2},
                        {"id": 4, "source_node": 5, "source_pin": 0, "target_node": 7, "target_pin": 0},
                        {"id": 5, "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 1},
                        {"id": 6, "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0}
                    ]
                }]
            }]
        }]
    })";

    Project project;
    std::string error;
    assert(load_project_from_json(json, project, error));

    Evaluator eval;
    auto r = eval.call_method(project, "test-dict", {});
    assert(r.status == EvalStatus::Success);
    assert(r.outputs[0].as_integer() == 42);

    printf("OK\n");
}

int main() {
    printf("=== Phograph Phase 2 Tests ===\n");

    test_string_prims();
    test_list_prims();
    test_dict_prims();
    test_type_prims();
    test_data_prims();
    test_error_prims();
    test_multi_case();
    test_fibonacci_recursive();
    test_dict_in_graph();

    printf("\n=== All Phase 2 tests passed! ===\n");
    return 0;
}
