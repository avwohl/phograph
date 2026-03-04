#include "../src/pho_bridge.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

// Test: C bridge API works end-to-end
static void test_bridge_basic() {
    printf("test_bridge_basic... ");

    pho_engine_init_prims();

    PhoEngineRef engine = pho_engine_create();
    assert(engine != nullptr);

    const char* json = R"({
        "name": "bridge-test",
        "sections": [{
            "name": "main",
            "methods": [{
                "name": "add",
                "num_inputs": 2,
                "num_outputs": 1,
                "cases": [{
                    "nodes": [
                        {"id": 1, "type": "input-bar", "num_inputs": 0, "num_outputs": 2},
                        {"id": 2, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1},
                        {"id": 3, "type": "output-bar", "num_inputs": 1, "num_outputs": 0}
                    ],
                    "wires": [
                        {"id": 1, "source_node": 1, "source_pin": 0, "target_node": 2, "target_pin": 0},
                        {"id": 2, "source_node": 1, "source_pin": 1, "target_node": 2, "target_pin": 1},
                        {"id": 3, "source_node": 2, "source_pin": 0, "target_node": 3, "target_pin": 0}
                    ]
                }]
            }]
        }]
    })";

    int rc = pho_engine_load_json(engine, json, strlen(json));
    assert(rc == 0);

    // Call with inputs [3, 4]
    const char* inputs = "[3, 4]";
    const char* result = pho_engine_call_method(engine, "add", inputs, strlen(inputs));
    assert(result != nullptr);

    std::string result_str(result);
    pho_engine_free_string(result);

    // Should contain "success" and "7"
    assert(result_str.find("\"success\"") != std::string::npos);
    assert(result_str.find("7") != std::string::npos);

    pho_engine_destroy(engine);

    printf("OK\n");
}

// Test: pixel buffer operations
static void test_bridge_pixel_buffer() {
    printf("test_bridge_pixel_buffer... ");

    PhoEngineRef engine = pho_engine_create();

    pho_engine_resize(engine, 100, 50);

    int32_t w, h;
    const uint8_t* buf = pho_engine_pixel_buffer(engine, &w, &h);
    assert(w == 100);
    assert(h == 50);
    assert(buf != nullptr);

    pho_engine_tick(engine, 1.0 / 60.0);

    pho_engine_destroy(engine);

    printf("OK\n");
}

// Test: error handling
static void test_bridge_errors() {
    printf("test_bridge_errors... ");

    PhoEngineRef engine = pho_engine_create();

    // Load invalid JSON
    int rc = pho_engine_load_json(engine, "not json", 8);
    assert(rc != 0);
    assert(pho_engine_last_error(engine) != nullptr);
    assert(strlen(pho_engine_last_error(engine)) > 0);

    pho_engine_destroy(engine);

    printf("OK\n");
}

int main() {
    printf("=== Phograph Phase 4 Bridge Tests ===\n");

    test_bridge_basic();
    test_bridge_pixel_buffer();
    test_bridge_errors();

    printf("\n=== All Phase 4 tests passed! ===\n");
    return 0;
}
