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

// Test: PhoObject basic operations
static void test_object_basic() {
    printf("test_object_basic... ");

    auto obj = make_ref<PhoObject>("Point");
    obj->set_attr("x", Value::real(3.0));
    obj->set_attr("y", Value::real(4.0));

    assert(obj->class_name() == "Point");
    assert(obj->get_attr("x").as_real() == 3.0);
    assert(obj->get_attr("y").as_real() == 4.0);
    assert(obj->get_attr("z").is_null()); // not set

    Value v = Value::object(obj);
    assert(v.is_object());
    assert(v.as_object()->class_name() == "Point");

    printf("OK\n");
}

// Test: instance generator + get/set in graph
static void test_class_in_graph() {
    printf("test_class_in_graph... ");
    ensure_prims();

    // Define a Point class with x,y attributes and a distance method
    // Create an instance, set x=3, y=4, compute distance = sqrt(x*x + y*y) = 5
    std::string json = R"({
        "name": "test-class",
        "sections": [{
            "name": "main",
            "classes": [{
                "name": "Point",
                "attributes": [
                    {"name": "x", "default": 0.0},
                    {"name": "y", "default": 0.0}
                ],
                "methods": [{
                    "name": "distance",
                    "num_inputs": 1,
                    "num_outputs": 1,
                    "cases": [{
                        "nodes": [
                            {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                            {"id": 2, "type": "get", "name": "x", "num_inputs": 1, "num_outputs": 2},
                            {"id": 3, "type": "get", "name": "y", "num_inputs": 1, "num_outputs": 2},
                            {"id": 4, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                            {"id": 5, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                            {"id": 6, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1},
                            {"id": 7, "type": "primitive", "name": "sqrt", "num_inputs": 1, "num_outputs": 1},
                            {"id": 8, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                        ],
                        "wires": [
                            {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0},
                            {"id": 2, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0},
                            {"id": 3, "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 0},
                            {"id": 4, "source_node": 2, "source_pin": 1, "target_node": 4, "target_pin": 1},
                            {"id": 5, "source_node": 3, "source_pin": 1, "target_node": 5, "target_pin": 0},
                            {"id": 6, "source_node": 3, "source_pin": 1, "target_node": 5, "target_pin": 1},
                            {"id": 7, "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0},
                            {"id": 8, "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1},
                            {"id": 9, "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0},
                            {"id": 10, "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0}
                        ]
                    }]
                }]
            }],
            "methods": [{
                "name": "main",
                "num_inputs": 0,
                "num_outputs": 1,
                "cases": [{
                    "nodes": [
                        {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 0},
                        {"id": 2, "type": "instance-generator", "name": "Point", "num_inputs": 0, "num_outputs": 1},
                        {"id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 3.0},
                        {"id": 4, "type": "set", "name": "x", "num_inputs": 2, "num_outputs": 1},
                        {"id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 4.0},
                        {"id": 6, "type": "set", "name": "y", "num_inputs": 2, "num_outputs": 1},
                        {"id": 7, "type": "method-call", "name": "/distance", "num_inputs": 1, "num_outputs": 1},
                        {"id": 8, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                    ],
                    "wires": [
                        {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0},
                        {"id": 2, "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1},
                        {"id": 3, "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0},
                        {"id": 4, "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1},
                        {"id": 5, "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0},
                        {"id": 6, "source_node": 7, "source_pin": 0, "target_node": 8, "target_pin": 0}
                    ]
                }]
            }]
        }]
    })";

    Project project;
    std::string error;
    bool ok = load_project_from_json(json, project, error);
    if (!ok) { printf("FAIL: %s\n", error.c_str()); assert(false); }

    Evaluator eval;
    auto result = eval.call_method(project, "main", {});
    if (result.status != EvalStatus::Success) {
        printf("FAIL: status=%d, err=%s\n", (int)result.status,
               result.err_val.to_display_string().c_str());
        assert(false);
    }
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_real());
    assert(std::abs(result.outputs[0].as_real() - 5.0) < 1e-10);

    printf("OK\n");
}

// Test: inheritance + polymorphic dispatch
static void test_inheritance() {
    printf("test_inheritance... ");
    ensure_prims();

    // Shape class with area method (returns 0)
    // Circle subclass overrides area (returns pi*r*r)
    std::string json = R"({
        "name": "test-inherit",
        "sections": [{
            "name": "main",
            "classes": [
                {
                    "name": "Shape",
                    "attributes": [],
                    "methods": [{
                        "name": "area",
                        "num_inputs": 1,
                        "num_outputs": 1,
                        "cases": [{
                            "nodes": [
                                {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                                {"id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 0.0},
                                {"id": 3, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                            ],
                            "wires": [
                                {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0}
                            ]
                        }]
                    }]
                },
                {
                    "name": "Circle",
                    "parent": "Shape",
                    "attributes": [
                        {"name": "radius", "default": 1.0}
                    ],
                    "methods": [{
                        "name": "area",
                        "num_inputs": 1,
                        "num_outputs": 1,
                        "cases": [{
                            "nodes": [
                                {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 1},
                                {"id": 2, "type": "get", "name": "radius", "num_inputs": 1, "num_outputs": 2},
                                {"id": 3, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                                {"id": 4, "type": "primitive", "name": "pi", "num_inputs": 0, "num_outputs": 1},
                                {"id": 5, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1},
                                {"id": 6, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                            ],
                            "wires": [
                                {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0},
                                {"id": 2, "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 0},
                                {"id": 3, "source_node": 2, "source_pin": 1, "target_node": 3, "target_pin": 1},
                                {"id": 4, "source_node": 3, "source_pin": 0, "target_node": 5, "target_pin": 0},
                                {"id": 5, "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 1},
                                {"id": 6, "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0}
                            ]
                        }]
                    }]
                }
            ],
            "methods": [{
                "name": "main",
                "num_inputs": 0,
                "num_outputs": 1,
                "cases": [{
                    "nodes": [
                        {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 0},
                        {"id": 2, "type": "instance-generator", "name": "Circle", "num_inputs": 0, "num_outputs": 1},
                        {"id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": 5.0},
                        {"id": 4, "type": "set", "name": "radius", "num_inputs": 2, "num_outputs": 1},
                        {"id": 5, "type": "method-call", "name": "/area", "num_inputs": 1, "num_outputs": 1},
                        {"id": 6, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                    ],
                    "wires": [
                        {"id": 1, "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0},
                        {"id": 2, "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1},
                        {"id": 3, "source_node": 4, "source_pin": 0, "target_node": 5, "target_pin": 0},
                        {"id": 4, "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 0}
                    ]
                }]
            }]
        }]
    })";

    Project project;
    std::string error;
    assert(load_project_from_json(json, project, error));

    Evaluator eval;
    auto result = eval.call_method(project, "main", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    // Circle area = pi * 5 * 5 = 78.539...
    double expected = M_PI * 25.0;
    assert(std::abs(result.outputs[0].as_real() - expected) < 1e-10);

    printf("OK\n");
}

int main() {
    printf("=== Phograph Phase 3 Tests ===\n");

    test_object_basic();
    test_class_in_graph();
    test_inheritance();

    printf("\n=== All Phase 3 tests passed! ===\n");
    return 0;
}
