#include "../src/pho_bridge.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>

// ============================================================
// Helpers
// ============================================================

static bool g_inited = false;
static void init_once() {
    if (!g_inited) { pho_engine_init_prims(); g_inited = true; }
}

static std::string run(const char* json, const char* method, const char* inputs) {
    init_once();
    PhoEngineRef e = pho_engine_create();
    int rc = pho_engine_load_json(e, json, strlen(json));
    if (rc != 0) {
        fprintf(stderr, "LOAD ERROR: %s\n", pho_engine_last_error(e));
        pho_engine_destroy(e);
        assert(false && "load_json failed");
        return "";
    }
    const char* raw = pho_engine_call_method(e, method, inputs, inputs ? strlen(inputs) : 0);
    std::string result(raw);
    pho_engine_free_string(raw);
    pho_engine_destroy(e);
    return result;
}

static void assert_success(const std::string& r) {
    if (r.find("\"status\":\"success\"") == std::string::npos) {
        fprintf(stderr, "FAIL expected success: %s\n", r.c_str());
        assert(false);
    }
}

static void assert_failure(const std::string& r) {
    if (r.find("\"status\":\"failure\"") == std::string::npos &&
        r.find("\"status\":\"error\"") == std::string::npos) {
        fprintf(stderr, "FAIL expected failure/error: %s\n", r.c_str());
        assert(false);
    }
}

// Check outputs array is exactly: "outputs":[expected]
static void assert_output(const std::string& r, const std::string& expected) {
    assert_success(r);
    std::string pat = "\"outputs\":[" + expected + "]";
    if (r.find(pat) == std::string::npos) {
        fprintf(stderr, "FAIL expected outputs [%s] in: %s\n", expected.c_str(), r.c_str());
        assert(false);
    }
}

static void assert_output_contains(const std::string& r, const std::string& substr) {
    assert_success(r);
    if (r.find(substr) == std::string::npos) {
        fprintf(stderr, "FAIL expected '%s' in: %s\n", substr.c_str(), r.c_str());
        assert(false);
    }
}

static void assert_output_real(const std::string& r, double expected, double tol = 1e-6) {
    assert_success(r);
    auto pos = r.find("\"outputs\":[");
    assert(pos != std::string::npos);
    pos += strlen("\"outputs\":[");
    double actual = std::stod(r.substr(pos));
    if (std::abs(actual - expected) > tol) {
        fprintf(stderr, "FAIL expected ~%g got %g in: %s\n", expected, actual, r.c_str());
        assert(false);
    }
}

static void assert_console(const std::string& r, const std::string& expected) {
    if (r.find(expected) == std::string::npos) {
        fprintf(stderr, "FAIL expected console '%s' in: %s\n", expected.c_str(), r.c_str());
        assert(false);
    }
}

// JSON escape for primitive names
static std::string jesc(const char* s) {
    std::string out;
    for (; *s; ++s) {
        if (*s == '"') out += "\\\"";
        else if (*s == '\\') out += "\\\\";
        else out += *s;
    }
    return out;
}

// f(a,b) → prim(a,b) → output
static std::string binary_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":2,"num_outputs":1,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":2,"num_outputs":1},)"
        R"({"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},)"
        R"({"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},)"
        R"({"id":3,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0})"
        R"(]}]}]}]})";
}

// f(a) → prim(a) → output
static std::string unary_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":1,"num_outputs":1,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":1,"num_outputs":1},)"
        R"({"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},)"
        R"({"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0})"
        R"(]}]}]}]})";
}

// f() → prim() → output
static std::string nullary_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":0,"num_outputs":1},)"
        R"({"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0})"
        R"(]}]}]}]})";
}

// f(a,b,c) → prim(a,b,c) → output
static std::string ternary_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":3,"num_outputs":1,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":3},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":3,"num_outputs":1},)"
        R"({"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},)"
        R"({"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},)"
        R"({"id":3,"source_node":1,"source_pin":2,"target_node":2,"target_pin":2},)"
        R"({"id":4,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0})"
        R"(]}]}]}]})";
}

// f(a,b) → prim(a,b) → [out0, out1]
static std::string binary_2out_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":2,"num_outputs":2,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":2,"num_outputs":2},)"
        R"({"id":3,"type":"output-bar","num_inputs":2,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},)"
        R"({"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},)"
        R"({"id":3,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},)"
        R"({"id":4,"source_node":2,"source_pin":1,"target_node":3,"target_pin":1})"
        R"(]}]}]}]})";
}

// f(a) → prim(a) → [out0, out1]
static std::string unary_2out_project(const char* prim) {
    return R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":1,"num_outputs":2,"cases":[{"nodes":[)"
        R"({"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},)"
        R"({"id":2,"type":"primitive","name":")" + jesc(prim) + R"(","num_inputs":1,"num_outputs":2},)"
        R"({"id":3,"type":"output-bar","num_inputs":2,"num_outputs":0})"
        R"(],"wires":[)"
        R"({"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},)"
        R"({"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},)"
        R"({"id":3,"source_node":2,"source_pin":1,"target_node":3,"target_pin":1})"
        R"(]}]}]}]})";
}

// ============================================================
// 1. Arithmetic
// ============================================================

static void test_arith_basic() {
    printf("test_arith_basic... ");
    auto bp = binary_project("+");
    auto bm = binary_project("-");
    auto bx = binary_project("*");
    auto bd = binary_project("/");

    // Integer add
    assert_output(run(bp.c_str(), "f", "[3, 4]"), "7");
    // Real add
    assert_output(run(bp.c_str(), "f", "[1.5, 2.5]"), "4");
    // Mixed add (int + real)
    assert_output_real(run(bp.c_str(), "f", "[1, 2.5]"), 3.5);
    // Subtract
    assert_output(run(bm.c_str(), "f", "[10, 3]"), "7");
    // Negative result
    assert_output(run(bm.c_str(), "f", "[3, 10]"), "-7");
    // Multiply
    assert_output(run(bx.c_str(), "f", "[6, 7]"), "42");
    // Divide exact
    assert_output(run(bd.c_str(), "f", "[15, 3]"), "5");
    // Divide real result
    assert_output_real(run(bd.c_str(), "f", "[7, 2]"), 3.5);
    // Division by zero
    assert_failure(run(bd.c_str(), "f", "[1, 0]"));

    printf("OK\n");
}

static void test_arith_advanced() {
    printf("test_arith_advanced... ");

    // mod
    assert_output(run(binary_project("mod").c_str(), "f", "[17, 5]"), "2");
    // abs
    assert_output(run(unary_project("abs").c_str(), "f", "[-42]"), "42");
    assert_output(run(unary_project("abs").c_str(), "f", "[42]"), "42");
    // round
    assert_output(run(unary_project("round").c_str(), "f", "[3.7]"), "4");
    assert_output(run(unary_project("round").c_str(), "f", "[3.2]"), "3");
    // floor
    assert_output(run(unary_project("floor").c_str(), "f", "[3.9]"), "3");
    // ceiling
    assert_output(run(unary_project("ceiling").c_str(), "f", "[3.1]"), "4");
    // sqrt
    assert_output(run(unary_project("sqrt").c_str(), "f", "[25]"), "5");
    assert_output_real(run(unary_project("sqrt").c_str(), "f", "[2]"), std::sqrt(2.0));
    // power
    assert_output(run(binary_project("power").c_str(), "f", "[2, 10]"), "1024");
    // min / max
    assert_output(run(binary_project("min").c_str(), "f", "[3, 7]"), "3");
    assert_output(run(binary_project("max").c_str(), "f", "[3, 7]"), "7");
    // clamp
    assert_output(run(ternary_project("clamp").c_str(), "f", "[15, 0, 10]"), "10");
    assert_output(run(ternary_project("clamp").c_str(), "f", "[-5, 0, 10]"), "0");
    assert_output(run(ternary_project("clamp").c_str(), "f", "[5, 0, 10]"), "5");
    // +1
    assert_output(run(unary_project("+1").c_str(), "f", "[41]"), "42");
    // div-mod (2 outputs)
    assert_output(run(binary_2out_project("div-mod").c_str(), "f", "[17, 5]"), "3,2");

    printf("OK\n");
}

static void test_arith_trig() {
    printf("test_arith_trig... ");

    // pi
    assert_output_real(run(nullary_project("pi").c_str(), "f", "[]"), M_PI);
    // sin(0) = 0
    assert_output_real(run(unary_project("sin").c_str(), "f", "[0]"), 0.0);
    // cos(0) = 1
    assert_output_real(run(unary_project("cos").c_str(), "f", "[0]"), 1.0);
    // atan2(1, 1) = pi/4
    assert_output_real(run(binary_project("atan2").c_str(), "f", "[1, 1]"), M_PI / 4.0);
    // ln(1) = 0
    assert_output_real(run(unary_project("ln").c_str(), "f", "[1]"), 0.0);
    // log10(100) = 2
    assert_output_real(run(unary_project("log10").c_str(), "f", "[100]"), 2.0);
    // log2(8) = 3
    assert_output_real(run(unary_project("log2").c_str(), "f", "[8]"), 3.0);

    printf("OK\n");
}

// ============================================================
// 2. Comparison
// ============================================================

static void test_comparison() {
    printf("test_comparison... ");

    // = integers
    assert_output(run(binary_project("=").c_str(), "f", "[3, 3]"), "true");
    assert_output(run(binary_project("=").c_str(), "f", "[3, 4]"), "false");
    // = strings
    assert_output(run(binary_project("=").c_str(), "f", R"(["abc", "abc"])"), "true");
    assert_output(run(binary_project("=").c_str(), "f", R"(["abc", "def"])"), "false");
    // !=
    assert_output(run(binary_project("!=").c_str(), "f", "[3, 4]"), "true");
    assert_output(run(binary_project("!=").c_str(), "f", "[3, 3]"), "false");
    // <
    assert_output(run(binary_project("<").c_str(), "f", "[3, 7]"), "true");
    assert_output(run(binary_project("<").c_str(), "f", "[7, 3]"), "false");
    assert_output(run(binary_project("<").c_str(), "f", "[3, 3]"), "false");
    // >
    assert_output(run(binary_project(">").c_str(), "f", "[7, 3]"), "true");
    // <=
    assert_output(run(binary_project("<=").c_str(), "f", "[3, 3]"), "true");
    assert_output(run(binary_project("<=").c_str(), "f", "[3, 4]"), "true");
    assert_output(run(binary_project("<=").c_str(), "f", "[4, 3]"), "false");
    // >=
    assert_output(run(binary_project(">=").c_str(), "f", "[4, 3]"), "true");
    assert_output(run(binary_project(">=").c_str(), "f", "[3, 3]"), "true");

    printf("OK\n");
}

// ============================================================
// 3. Logic
// ============================================================

static void test_logic() {
    printf("test_logic... ");

    // and
    assert_output(run(binary_project("and").c_str(), "f", "[true, true]"), "true");
    assert_output(run(binary_project("and").c_str(), "f", "[true, false]"), "false");
    assert_output(run(binary_project("and").c_str(), "f", "[false, false]"), "false");
    // or
    assert_output(run(binary_project("or").c_str(), "f", "[true, false]"), "true");
    assert_output(run(binary_project("or").c_str(), "f", "[false, false]"), "false");
    // not
    assert_output(run(unary_project("not").c_str(), "f", "[true]"), "false");
    assert_output(run(unary_project("not").c_str(), "f", "[false]"), "true");

    printf("OK\n");
}

// ============================================================
// 4. String primitives
// ============================================================

static void test_string_basic() {
    printf("test_string_basic... ");

    // concat
    assert_output(run(binary_project("concat").c_str(), "f", R"(["hello", " world"])"), "\"hello world\"");
    // length
    assert_output(run(unary_project("length").c_str(), "f", R"(["hello"])"), "5");
    // to-string (integer)
    assert_output(run(unary_project("to-string").c_str(), "f", "[42]"), "\"42\"");
    // to-string (boolean)
    assert_output(run(unary_project("to-string").c_str(), "f", "[true]"), "\"true\"");
    // uppercase
    assert_output(run(unary_project("uppercase").c_str(), "f", R"(["hello"])"), "\"HELLO\"");
    // lowercase
    assert_output(run(unary_project("lowercase").c_str(), "f", R"(["HELLO"])"), "\"hello\"");
    // string-trim
    assert_output(run(unary_project("string-trim").c_str(), "f", R"(["  hello  "])"), "\"hello\"");

    printf("OK\n");
}

static void test_string_search() {
    printf("test_string_search... ");

    // string-search (1-indexed)
    assert_output(run(binary_project("string-search").c_str(), "f", R"(["hello world", "world"])"), "7");
    // string-contains?
    assert_output(run(binary_project("string-contains?").c_str(), "f", R"(["hello world", "world"])"), "true");
    assert_output(run(binary_project("string-contains?").c_str(), "f", R"(["hello world", "xyz"])"), "false");
    // string-starts-with?
    assert_output(run(binary_project("string-starts-with?").c_str(), "f", R"(["hello", "hel"])"), "true");
    assert_output(run(binary_project("string-starts-with?").c_str(), "f", R"(["hello", "xyz"])"), "false");
    // string-ends-with?
    assert_output(run(binary_project("string-ends-with?").c_str(), "f", R"(["hello", "llo"])"), "true");

    printf("OK\n");
}

static void test_string_transform() {
    printf("test_string_transform... ");

    // string-replace
    assert_output(run(ternary_project("string-replace").c_str(), "f",
        R"(["hello world", "world", "there"])"), "\"hello there\"");
    // char-at (1-indexed)
    assert_output(run(binary_project("char-at").c_str(), "f", R"(["abc", 2])"), "\"b\"");
    // prefix
    assert_output(run(binary_project("prefix").c_str(), "f", R"(["hello", 3])"), "\"hel\"");
    // suffix
    assert_output(run(binary_project("suffix").c_str(), "f", R"(["hello", 3])"), "\"llo\"");
    // middle
    assert_output(run(ternary_project("middle").c_str(), "f", R"(["hello world", 2, 5])"), "\"ello \"");
    // string-repeat
    assert_output(run(binary_project("string-repeat").c_str(), "f", R"(["ab", 3])"), "\"ababab\"");

    printf("OK\n");
}

// ============================================================
// 5. List primitives (tested via graph chains)
// ============================================================

static void test_list_basic() {
    printf("test_list_basic... ");

    // make-list(3, 0) → length → 3
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":2,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},
            {"id":2,"type":"primitive","name":"make-list","num_inputs":2,"num_outputs":1},
            {"id":3,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
            {"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},
            {"id":3,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":4,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[3, 0]"), "3");
    }

    // List constant [10, 20, 30] → get-nth(_, 2) → 20
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":4,"type":"primitive","name":"get-nth","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "20");
    }

    // first([10, 20, 30]) → 10
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"primitive","name":"first","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "10");
    }

    // rest([10, 20, 30]) → length → 2
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"primitive","name":"rest","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "2");
    }

    printf("OK\n");
}

static void test_list_advanced() {
    printf("test_list_advanced... ");

    // set-nth: [10,20,30] → set-nth(_, 2, 99) → get-nth(_, 2) → 99
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":99},
            {"id":5,"type":"primitive","name":"set-nth","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":7,"type":"primitive","name":"get-nth","num_inputs":2,"num_outputs":1},
            {"id":8,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":7,"target_pin":0},
            {"id":5,"source_node":6,"source_pin":0,"target_node":7,"target_pin":1},
            {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "99");
    }

    // append([1,2], [3,4]) → length → 4
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[1,2]},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":[3,4]},
            {"id":4,"type":"primitive","name":"append","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "4");
    }

    // reverse([10,20,30]) → first → 30
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"primitive","name":"reverse","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"primitive","name":"first","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "30");
    }

    // sort([3,1,2]) → first → 1
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[3,1,2]},
            {"id":3,"type":"primitive","name":"sort","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"primitive","name":"first","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "1");
    }

    // contains?([10,20,30], 20) → true
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":20},
            {"id":4,"type":"primitive","name":"contains?","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    // empty?([]) → true
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[]},
            {"id":3,"type":"primitive","name":"empty?","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    // unique([1,2,1,3]) → length → 3
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[1,2,1,3]},
            {"id":3,"type":"primitive","name":"unique","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "3");
    }

    printf("OK\n");
}

static void test_list_split() {
    printf("test_list_split... ");

    // detach-l([10,20,30]) → [first=10, rest_length=2]
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":2,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"primitive","name":"detach-l","num_inputs":1,"num_outputs":2},
            {"id":4,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":2,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":3,"source_node":3,"source_pin":1,"target_node":4,"target_pin":0},
            {"id":4,"source_node":4,"source_pin":0,"target_node":5,"target_pin":1}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "10,2");
    }

    // split-nth([10,20,30,40], 3) → [left_len, right_len] = [2, 2]
    // split-nth splits before position n (1-indexed)
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":2,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30,40]},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":3},
            {"id":4,"type":"primitive","name":"split-nth","num_inputs":2,"num_outputs":2},
            {"id":5,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":6,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":7,"type":"output-bar","num_inputs":2,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":4,"source_node":4,"source_pin":1,"target_node":6,"target_pin":0},
            {"id":5,"source_node":5,"source_pin":0,"target_node":7,"target_pin":0},
            {"id":6,"source_node":6,"source_pin":0,"target_node":7,"target_pin":1}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "2,2");
    }

    // enumerate([10,20,30]) → first → first of that pair → 1
    // enumerate returns [[1,10],[2,20],[3,30]]
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[10,20,30]},
            {"id":3,"type":"primitive","name":"enumerate","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "3");
    }

    printf("OK\n");
}

// ============================================================
// 6. Dict primitives (tested via graph chains)
// ============================================================

static void test_dict() {
    printf("test_dict... ");

    // dict-create → dict-set("name", "Alice") → dict-get("name") → "Alice"
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":"name"},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":"Alice"},
            {"id":5,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"constant","num_inputs":0,"num_outputs":1,"value":"name"},
            {"id":7,"type":"primitive","name":"dict-get","num_inputs":2,"num_outputs":1},
            {"id":8,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":7,"target_pin":0},
            {"id":5,"source_node":6,"source_pin":0,"target_node":7,"target_pin":1},
            {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "\"Alice\"");
    }

    // dict-has? → true for existing key
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":"x"},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":42},
            {"id":5,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"constant","num_inputs":0,"num_outputs":1,"value":"x"},
            {"id":7,"type":"primitive","name":"dict-has?","num_inputs":2,"num_outputs":1},
            {"id":8,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":7,"target_pin":0},
            {"id":5,"source_node":6,"source_pin":0,"target_node":7,"target_pin":1},
            {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    // dict-size after two sets → 2
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":"a"},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
            {"id":5,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"constant","num_inputs":0,"num_outputs":1,"value":"b"},
            {"id":7,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":8,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":9,"type":"primitive","name":"dict-size","num_inputs":1,"num_outputs":1},
            {"id":10,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":8,"target_pin":0},
            {"id":5,"source_node":6,"source_pin":0,"target_node":8,"target_pin":1},
            {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":2},
            {"id":7,"source_node":8,"source_pin":0,"target_node":9,"target_pin":0},
            {"id":8,"source_node":9,"source_pin":0,"target_node":10,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "2");
    }

    // dict-get-default for missing key → default value
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":"missing"},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":99},
            {"id":5,"type":"primitive","name":"dict-get-default","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "99");
    }

    // dict-remove: create {a:1, b:2}, remove "a" → size = 1
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":"a"},
            {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
            {"id":5,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":6,"type":"constant","num_inputs":0,"num_outputs":1,"value":"b"},
            {"id":7,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":8,"type":"primitive","name":"dict-set","num_inputs":3,"num_outputs":1},
            {"id":9,"type":"constant","num_inputs":0,"num_outputs":1,"value":"a"},
            {"id":10,"type":"primitive","name":"dict-remove","num_inputs":2,"num_outputs":1},
            {"id":11,"type":"primitive","name":"dict-size","num_inputs":1,"num_outputs":1},
            {"id":12,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":2},
            {"id":4,"source_node":5,"source_pin":0,"target_node":8,"target_pin":0},
            {"id":5,"source_node":6,"source_pin":0,"target_node":8,"target_pin":1},
            {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":2},
            {"id":7,"source_node":8,"source_pin":0,"target_node":10,"target_pin":0},
            {"id":8,"source_node":9,"source_pin":0,"target_node":10,"target_pin":1},
            {"id":9,"source_node":10,"source_pin":0,"target_node":11,"target_pin":0},
            {"id":10,"source_node":11,"source_pin":0,"target_node":12,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "1");
    }

    printf("OK\n");
}

// ============================================================
// 7. Type checks
// ============================================================

static void test_type_checks() {
    printf("test_type_checks... ");

    assert_output(run(unary_project("integer?").c_str(), "f", "[42]"), "true");
    assert_output(run(unary_project("integer?").c_str(), "f", "[3.14]"), "false");
    assert_output(run(unary_project("real?").c_str(), "f", "[3.14]"), "true");
    assert_output(run(unary_project("real?").c_str(), "f", "[42]"), "false");
    assert_output(run(unary_project("boolean?").c_str(), "f", "[true]"), "true");
    assert_output(run(unary_project("boolean?").c_str(), "f", "[42]"), "false");
    assert_output(run(unary_project("string?").c_str(), "f", R"(["hello"])"), "true");
    assert_output(run(unary_project("string?").c_str(), "f", "[42]"), "false");
    assert_output(run(unary_project("null?").c_str(), "f", "[null]"), "true");
    assert_output(run(unary_project("null?").c_str(), "f", "[42]"), "false");

    // type-of
    assert_output(run(unary_project("type-of").c_str(), "f", "[42]"), "\"integer\"");
    assert_output(run(unary_project("type-of").c_str(), "f", "[3.14]"), "\"real\"");
    assert_output(run(unary_project("type-of").c_str(), "f", R"(["hello"])"), "\"string\"");
    assert_output(run(unary_project("type-of").c_str(), "f", "[true]"), "\"boolean\"");
    assert_output(run(unary_project("type-of").c_str(), "f", "[null]"), "\"null\"");

    // list? via constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[1,2,3]},
            {"id":3,"type":"primitive","name":"list?","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    // dict? via dict-create
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"primitive","name":"dict-create","num_inputs":0,"num_outputs":1},
            {"id":3,"type":"primitive","name":"dict?","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    printf("OK\n");
}

// ============================================================
// 8. Data type constants
// ============================================================

static void test_constants() {
    printf("test_constants... ");

    // Integer constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":42},
            {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "42");
    }

    // Real constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":3.14},
            {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
        ]}]}]}]})";
        assert_output_real(run(json, "f", "[]"), 3.14);
    }

    // String constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":"hello"},
            {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "\"hello\"");
    }

    // Boolean constants
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":2,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":true},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":false},
            {"id":4,"type":"output-bar","num_inputs":2,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true,false");
    }

    // Null constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":null},
            {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "null");
    }

    // List constant
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":[1,2,3]},
            {"id":3,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
            {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
        ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "3");
    }

    printf("OK\n");
}

// ============================================================
// 9. Control flow
// ============================================================

static void test_multi_case_dispatch() {
    printf("test_multi_case_dispatch... ");

    // abs-val: 2 cases
    // Case 1: if n >= 0, return n
    // Case 2: return 0 - n
    auto json = R"({
        "name": "t","sections": [{"name": "main","methods": [{
            "name": "abs-val","num_inputs": 1,"num_outputs": 1,
            "cases": [{
                "nodes": [
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":0},
                    {"id":3,"type":"primitive","name":">=","num_inputs":2,"num_outputs":1,"control":"next-case-on-failure"},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires": [
                    {"id":1,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":1,"source_pin":0,"target_node":4,"target_pin":0}
                ]
            },{
                "nodes": [
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":0},
                    {"id":3,"type":"primitive","name":"-","num_inputs":2,"num_outputs":1},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires": [
                    {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":1,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
                ]
            }]
        }]}]
    })";

    assert_output(run(json, "abs-val", "[5]"), "5");
    assert_output(run(json, "abs-val", "[-3]"), "3");
    assert_output(run(json, "abs-val", "[0]"), "0");

    printf("OK\n");
}

static void test_fibonacci() {
    printf("test_fibonacci... ");

    auto json = R"({
        "name":"t","sections":[{"name":"main","methods":[{
            "name":"fib","num_inputs":1,"num_outputs":1,
            "cases":[{
                "nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
                    {"id":3,"type":"primitive","name":"<=","num_inputs":2,"num_outputs":1,"control":"next-case-on-failure"},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":1,"source_pin":0,"target_node":4,"target_pin":0}
                ]
            },{
                "nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
                    {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
                    {"id":4,"type":"primitive","name":"-","num_inputs":2,"num_outputs":1},
                    {"id":5,"type":"primitive","name":"-","num_inputs":2,"num_outputs":1},
                    {"id":6,"type":"method-call","name":"fib","num_inputs":1,"num_outputs":1},
                    {"id":7,"type":"method-call","name":"fib","num_inputs":1,"num_outputs":1},
                    {"id":8,"type":"primitive","name":"+","num_inputs":2,"num_outputs":1},
                    {"id":9,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":4,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":4,"target_pin":1},
                    {"id":3,"source_node":1,"source_pin":0,"target_node":5,"target_pin":0},
                    {"id":4,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
                    {"id":5,"source_node":4,"source_pin":0,"target_node":6,"target_pin":0},
                    {"id":6,"source_node":5,"source_pin":0,"target_node":7,"target_pin":0},
                    {"id":7,"source_node":6,"source_pin":0,"target_node":8,"target_pin":0},
                    {"id":8,"source_node":7,"source_pin":0,"target_node":8,"target_pin":1},
                    {"id":9,"source_node":8,"source_pin":0,"target_node":9,"target_pin":0}
                ]
            }]
        }]}]
    })";

    assert_output(run(json, "fib", "[0]"), "0");
    assert_output(run(json, "fib", "[1]"), "1");
    assert_output(run(json, "fib", "[10]"), "55");

    printf("OK\n");
}

static void test_factorial() {
    printf("test_factorial... ");

    // factorial(n): case 1: n <= 0 → 1; case 2: n * factorial(n-1)
    auto json = R"({
        "name":"t","sections":[{"name":"main","methods":[{
            "name":"fact","num_inputs":1,"num_outputs":1,
            "cases":[{
                "nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":0},
                    {"id":3,"type":"primitive","name":"<=","num_inputs":2,"num_outputs":1,"control":"next-case-on-failure"},
                    {"id":4,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
                    {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
                ]
            },{
                "nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":1},
                    {"id":3,"type":"primitive","name":"-","num_inputs":2,"num_outputs":1},
                    {"id":4,"type":"method-call","name":"fact","num_inputs":1,"num_outputs":1},
                    {"id":5,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                    {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],
                "wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
                    {"id":4,"source_node":1,"source_pin":0,"target_node":5,"target_pin":0},
                    {"id":5,"source_node":4,"source_pin":0,"target_node":5,"target_pin":1},
                    {"id":6,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
                ]
            }]
        }]}]
    })";

    assert_output(run(json, "fact", "[0]"), "1");
    assert_output(run(json, "fact", "[1]"), "1");
    assert_output(run(json, "fact", "[5]"), "120");
    assert_output(run(json, "fact", "[10]"), "3628800");

    printf("OK\n");
}

// ============================================================
// 10. Method calls
// ============================================================

static void test_method_calls() {
    printf("test_method_calls... ");

    // Simple: double(x) = x * 2, quadruple(x) = double(double(x))
    {
        auto json = R"({
            "name":"t","sections":[{"name":"main","methods":[
                {"name":"double","num_inputs":1,"num_outputs":1,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
                    {"id":3,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":1},
                    {"id":3,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
                ]}]},
                {"name":"quadruple","num_inputs":1,"num_outputs":1,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"method-call","name":"double","num_inputs":1,"num_outputs":1},
                    {"id":3,"type":"method-call","name":"double","num_inputs":1,"num_outputs":1},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":3,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
                ]}]}
            ]}]
        })";
        assert_output(run(json, "quadruple", "[5]"), "20");
    }

    // Multiple outputs: swap(a,b) → [b, a]
    {
        auto json = R"({
            "name":"t","sections":[{"name":"main","methods":[
                {"name":"swap","num_inputs":2,"num_outputs":2,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},
                    {"id":2,"type":"output-bar","num_inputs":2,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":1},
                    {"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":0}
                ]}]}
            ]}]
        })";
        assert_output(run(json, "swap", "[10, 20]"), "20,10");
    }

    // Zero inputs: constant producer
    {
        auto json = R"({
            "name":"t","sections":[{"name":"main","methods":[
                {"name":"answer","num_inputs":0,"num_outputs":1,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                    {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":42},
                    {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
                ]}]}
            ]}]
        })";
        assert_output(run(json, "answer", "[]"), "42");
    }

    // Chained: a → b → c
    {
        auto json = R"({
            "name":"t","sections":[{"name":"main","methods":[
                {"name":"add1","num_inputs":1,"num_outputs":1,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"primitive","name":"+1","num_inputs":1,"num_outputs":1},
                    {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
                ]}]},
                {"name":"add3","num_inputs":1,"num_outputs":1,"cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                    {"id":2,"type":"method-call","name":"add1","num_inputs":1,"num_outputs":1},
                    {"id":3,"type":"method-call","name":"add1","num_inputs":1,"num_outputs":1},
                    {"id":4,"type":"method-call","name":"add1","num_inputs":1,"num_outputs":1},
                    {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                    {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":3,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
                    {"id":4,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
                ]}]}
            ]}]
        })";
        assert_output(run(json, "add3", "[10]"), "13");
    }

    printf("OK\n");
}

// ============================================================
// 11. OOP — Classes
// ============================================================

static void test_oop_basic() {
    printf("test_oop_basic... ");

    // Point class with x,y. Create, set x=3 y=4, compute distance = sqrt(x*x+y*y) = 5
    auto json = R"({
        "name":"t","sections":[{"name":"main",
            "classes":[{
                "name":"Point",
                "attributes":[{"name":"x","default":0.0},{"name":"y","default":0.0}],
                "methods":[{
                    "name":"distance","num_inputs":1,"num_outputs":1,
                    "cases":[{"nodes":[
                        {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                        {"id":2,"type":"get","name":"x","num_inputs":1,"num_outputs":2},
                        {"id":3,"type":"get","name":"y","num_inputs":1,"num_outputs":2},
                        {"id":4,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                        {"id":5,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                        {"id":6,"type":"primitive","name":"+","num_inputs":2,"num_outputs":1},
                        {"id":7,"type":"primitive","name":"sqrt","num_inputs":1,"num_outputs":1},
                        {"id":8,"type":"output-bar","num_inputs":1,"num_outputs":0}
                    ],"wires":[
                        {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                        {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                        {"id":3,"source_node":2,"source_pin":1,"target_node":4,"target_pin":0},
                        {"id":4,"source_node":2,"source_pin":1,"target_node":4,"target_pin":1},
                        {"id":5,"source_node":3,"source_pin":1,"target_node":5,"target_pin":0},
                        {"id":6,"source_node":3,"source_pin":1,"target_node":5,"target_pin":1},
                        {"id":7,"source_node":4,"source_pin":0,"target_node":6,"target_pin":0},
                        {"id":8,"source_node":5,"source_pin":0,"target_node":6,"target_pin":1},
                        {"id":9,"source_node":6,"source_pin":0,"target_node":7,"target_pin":0},
                        {"id":10,"source_node":7,"source_pin":0,"target_node":8,"target_pin":0}
                    ]}]
                }]
            }],
            "methods":[{
                "name":"main","num_inputs":0,"num_outputs":1,
                "cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                    {"id":2,"type":"instance-generator","name":"Point","num_inputs":0,"num_outputs":1},
                    {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":3.0},
                    {"id":4,"type":"set","name":"x","num_inputs":2,"num_outputs":1},
                    {"id":5,"type":"constant","num_inputs":0,"num_outputs":1,"value":4.0},
                    {"id":6,"type":"set","name":"y","num_inputs":2,"num_outputs":1},
                    {"id":7,"type":"method-call","name":"/distance","num_inputs":1,"num_outputs":1},
                    {"id":8,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
                    {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
                    {"id":3,"source_node":4,"source_pin":0,"target_node":6,"target_pin":0},
                    {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":1},
                    {"id":5,"source_node":6,"source_pin":0,"target_node":7,"target_pin":0},
                    {"id":6,"source_node":7,"source_pin":0,"target_node":8,"target_pin":0}
                ]}]
            }]
        }]
    })";

    assert_output_real(run(json, "main", "[]"), 5.0);

    printf("OK\n");
}

static void test_oop_inheritance() {
    printf("test_oop_inheritance... ");

    // Shape base class: area returns 0
    // Circle subclass: area returns pi*r*r
    auto json = R"({
        "name":"t","sections":[{"name":"main",
            "classes":[
                {"name":"Shape","attributes":[],"methods":[{
                    "name":"area","num_inputs":1,"num_outputs":1,
                    "cases":[{"nodes":[
                        {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                        {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":0.0},
                        {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
                    ],"wires":[
                        {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
                    ]}]
                }]},
                {"name":"Circle","parent":"Shape",
                 "attributes":[{"name":"radius","default":1.0}],
                 "methods":[{
                    "name":"area","num_inputs":1,"num_outputs":1,
                    "cases":[{"nodes":[
                        {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                        {"id":2,"type":"get","name":"radius","num_inputs":1,"num_outputs":2},
                        {"id":3,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                        {"id":4,"type":"primitive","name":"pi","num_inputs":0,"num_outputs":1},
                        {"id":5,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
                        {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
                    ],"wires":[
                        {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                        {"id":2,"source_node":2,"source_pin":1,"target_node":3,"target_pin":0},
                        {"id":3,"source_node":2,"source_pin":1,"target_node":3,"target_pin":1},
                        {"id":4,"source_node":3,"source_pin":0,"target_node":5,"target_pin":0},
                        {"id":5,"source_node":4,"source_pin":0,"target_node":5,"target_pin":1},
                        {"id":6,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
                    ]}]
                }]}
            ],
            "methods":[{
                "name":"main","num_inputs":0,"num_outputs":1,
                "cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                    {"id":2,"type":"instance-generator","name":"Circle","num_inputs":0,"num_outputs":1},
                    {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":5.0},
                    {"id":4,"type":"set","name":"radius","num_inputs":2,"num_outputs":1},
                    {"id":5,"type":"method-call","name":"/area","num_inputs":1,"num_outputs":1},
                    {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
                    {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
                    {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0},
                    {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
                ]}]
            }]
        }]
    })";

    // Circle area = pi * 5 * 5 = 78.539...
    assert_output_real(run(json, "main", "[]"), M_PI * 25.0);

    printf("OK\n");
}

static void test_oop_default_attrs() {
    printf("test_oop_default_attrs... ");

    // Counter class with default count=0, get default value
    auto json = R"({
        "name":"t","sections":[{"name":"main",
            "classes":[{
                "name":"Counter",
                "attributes":[{"name":"count","default":0}],
                "methods":[]
            }],
            "methods":[{
                "name":"main","num_inputs":0,"num_outputs":1,
                "cases":[{"nodes":[
                    {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                    {"id":2,"type":"instance-generator","name":"Counter","num_inputs":0,"num_outputs":1},
                    {"id":3,"type":"get","name":"count","num_inputs":1,"num_outputs":2},
                    {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
                ],"wires":[
                    {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                    {"id":2,"source_node":3,"source_pin":1,"target_node":4,"target_pin":0}
                ]}]
            }]
        }]
    })";

    assert_output(run(json, "main", "[]"), "0");

    printf("OK\n");
}

// ============================================================
// 12. Console / Log
// ============================================================

static void test_console() {
    printf("test_console... ");

    // log: writes to console, input passes to output via separate wire
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":1,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                {"id":2,"type":"primitive","name":"log","num_inputs":1,"num_outputs":0},
                {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                {"id":2,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0}
            ]}]}]}]})";
        auto r = run(json, "f", "[42]");
        assert_output(r, "42");
        assert_console(r, "42");
    }

    // inspect: writes to console AND passes through
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":1,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
                {"id":2,"type":"primitive","name":"inspect","num_inputs":1,"num_outputs":1},
                {"id":3,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                {"id":2,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0}
            ]}]}]}]})";
        auto r = run(json, "f", "[99]");
        assert_output(r, "99");
        assert_console(r, "99");
    }

    printf("OK\n");
}

// ============================================================
// 13. Error handling
// ============================================================

static void test_error_handling() {
    printf("test_error_handling... ");

    // Division by zero → failure
    assert_failure(run(binary_project("/").c_str(), "f", "[1, 0]"));

    // error-create produces an error value (method still succeeds since we pass the value through)
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":0,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":"something failed"},
                {"id":3,"type":"primitive","name":"error-create","num_inputs":1,"num_outputs":1},
                {"id":4,"type":"primitive","name":"error?","num_inputs":1,"num_outputs":1},
                {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
                {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "true");
    }

    // error-message extracts message from error
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":0,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":"oops"},
                {"id":3,"type":"primitive","name":"error-create","num_inputs":1,"num_outputs":1},
                {"id":4,"type":"primitive","name":"error-message","num_inputs":1,"num_outputs":1},
                {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0},
                {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "\"oops\"");
    }

    // error? returns false for non-error
    assert_output(run(unary_project("error?").c_str(), "f", "[42]"), "false");

    printf("OK\n");
}

// ============================================================
// 14. Canvas & Shapes
// ============================================================

static void test_canvas() {
    printf("test_canvas... ");

    // create-canvas(100, 50) → canvas-width → 100
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":2,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},
                {"id":2,"type":"primitive","name":"create-canvas","num_inputs":2,"num_outputs":1},
                {"id":3,"type":"primitive","name":"canvas-width","num_inputs":1,"num_outputs":1},
                {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                {"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},
                {"id":3,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                {"id":4,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[100, 50]"), "100");
    }

    // canvas-height
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":2,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":2},
                {"id":2,"type":"primitive","name":"create-canvas","num_inputs":2,"num_outputs":1},
                {"id":3,"type":"primitive","name":"canvas-height","num_inputs":1,"num_outputs":1},
                {"id":4,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
                {"id":2,"source_node":1,"source_pin":1,"target_node":2,"target_pin":1},
                {"id":3,"source_node":2,"source_pin":0,"target_node":3,"target_pin":0},
                {"id":4,"source_node":3,"source_pin":0,"target_node":4,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[100, 50]"), "50");
    }

    printf("OK\n");
}

// ============================================================
// 15. Math library prims
// ============================================================

static void test_math_library() {
    printf("test_math_library... ");

    // factorial (library prim)
    assert_output(run(unary_project("factorial").c_str(), "f", "[5]"), "120");
    // fibonacci (library prim, iterative)
    assert_output(run(unary_project("fibonacci").c_str(), "f", "[10]"), "55");
    // gcd
    assert_output(run(binary_project("gcd").c_str(), "f", "[12, 8]"), "4");
    // lcm
    assert_output(run(binary_project("lcm").c_str(), "f", "[4, 6]"), "12");
    // is-prime
    assert_output(run(unary_project("is-prime").c_str(), "f", "[7]"), "true");
    assert_output(run(unary_project("is-prime").c_str(), "f", "[4]"), "false");

    printf("OK\n");
}

// ============================================================
// 16. Edge cases
// ============================================================

static void test_edge_empty_method() {
    printf("test_edge_empty_method... ");

    // Method with no nodes except bars, no wires — 0 inputs, 0 outputs
    auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
        "name":"f","num_inputs":0,"num_outputs":0,
        "cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"output-bar","num_inputs":0,"num_outputs":0}
        ],"wires":[]}]}]}]})";
    auto r = run(json, "f", "[]");
    assert_success(r);

    printf("OK\n");
}

static void test_edge_fan_out() {
    printf("test_edge_fan_out... ");

    // One input feeds 4 separate + nodes, all sum to output
    // result = (x+x) + (x+x) = 4x
    auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
        "name":"f","num_inputs":1,"num_outputs":1,
        "cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
            {"id":2,"type":"primitive","name":"+","num_inputs":2,"num_outputs":1},
            {"id":3,"type":"primitive","name":"+","num_inputs":2,"num_outputs":1},
            {"id":4,"type":"primitive","name":"+","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":1,"source_pin":0,"target_node":2,"target_pin":0},
            {"id":2,"source_node":1,"source_pin":0,"target_node":2,"target_pin":1},
            {"id":3,"source_node":1,"source_pin":0,"target_node":3,"target_pin":0},
            {"id":4,"source_node":1,"source_pin":0,"target_node":3,"target_pin":1},
            {"id":5,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":6,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":7,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
    assert_output(run(json, "f", "[10]"), "40");

    printf("OK\n");
}

static void test_edge_deep_chain() {
    printf("test_edge_deep_chain... ");

    // Chain of 10 +1 nodes: result = input + 10
    std::string nodes = R"([{"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1})";
    std::string wires = "[";
    int wire_id = 1;
    for (int i = 0; i < 10; i++) {
        int nid = i + 2;
        nodes += R"(,{"id":)" + std::to_string(nid) + R"(,"type":"primitive","name":"+1","num_inputs":1,"num_outputs":1})";
        int src = (i == 0) ? 1 : (nid - 1);
        if (wire_id > 1) wires += ",";
        wires += R"({"id":)" + std::to_string(wire_id++) + R"(,"source_node":)" + std::to_string(src)
              + R"(,"source_pin":0,"target_node":)" + std::to_string(nid) + R"(,"target_pin":0})";
    }
    int out_id = 12;
    nodes += R"(,{"id":)" + std::to_string(out_id) + R"(,"type":"output-bar","num_inputs":1,"num_outputs":0}])";
    wires += R"(,{"id":)" + std::to_string(wire_id) + R"(,"source_node":11,"source_pin":0,"target_node":)"
          + std::to_string(out_id) + R"(,"target_pin":0}])";

    std::string json = R"({"name":"t","sections":[{"name":"main","methods":[{"name":"f","num_inputs":1,"num_outputs":1,"cases":[{"nodes":)"
        + nodes + R"(,"wires":)" + wires + R"(}]}]}]})";
    assert_output(run(json.c_str(), "f", "[0]"), "10");
    assert_output(run(json.c_str(), "f", "[100]"), "110");

    printf("OK\n");
}

static void test_edge_constant_only() {
    printf("test_edge_constant_only... ");

    // Method with no input connections, just constants
    auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
        "name":"f","num_inputs":0,"num_outputs":1,
        "cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":6},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":7},
            {"id":4,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"output-bar","num_inputs":1,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0}
        ]}]}]}]})";
    assert_output(run(json, "f", "[]"), "42");

    printf("OK\n");
}

static void test_edge_multiple_outputs() {
    printf("test_edge_multiple_outputs... ");

    // Method returns 3 outputs: input, input*2, input*3
    auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
        "name":"f","num_inputs":1,"num_outputs":3,
        "cases":[{"nodes":[
            {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":1},
            {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":2},
            {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":3},
            {"id":4,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
            {"id":5,"type":"primitive","name":"*","num_inputs":2,"num_outputs":1},
            {"id":6,"type":"output-bar","num_inputs":3,"num_outputs":0}
        ],"wires":[
            {"id":1,"source_node":1,"source_pin":0,"target_node":4,"target_pin":0},
            {"id":2,"source_node":2,"source_pin":0,"target_node":4,"target_pin":1},
            {"id":3,"source_node":1,"source_pin":0,"target_node":5,"target_pin":0},
            {"id":4,"source_node":3,"source_pin":0,"target_node":5,"target_pin":1},
            {"id":5,"source_node":1,"source_pin":0,"target_node":6,"target_pin":0},
            {"id":6,"source_node":4,"source_pin":0,"target_node":6,"target_pin":1},
            {"id":7,"source_node":5,"source_pin":0,"target_node":6,"target_pin":2}
        ]}]}]}]})";
    assert_output(run(json, "f", "[5]"), "5,10,15");

    printf("OK\n");
}

// ============================================================
// 17. String-split based list operations
// ============================================================

static void test_string_split_list() {
    printf("test_string_split_list... ");

    // string-split("a,b,c", ",") → length → 3
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":0,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":"a,b,c"},
                {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":","},
                {"id":4,"type":"primitive","name":"string-split","num_inputs":2,"num_outputs":1},
                {"id":5,"type":"primitive","name":"length","num_inputs":1,"num_outputs":1},
                {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
                {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
                {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0},
                {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "3");
    }

    // string-split → first → "a"
    {
        auto json = R"({"name":"t","sections":[{"name":"main","methods":[{
            "name":"f","num_inputs":0,"num_outputs":1,
            "cases":[{"nodes":[
                {"id":1,"type":"input-bar","num_inputs":0,"num_outputs":0},
                {"id":2,"type":"constant","num_inputs":0,"num_outputs":1,"value":"a,b,c"},
                {"id":3,"type":"constant","num_inputs":0,"num_outputs":1,"value":","},
                {"id":4,"type":"primitive","name":"string-split","num_inputs":2,"num_outputs":1},
                {"id":5,"type":"primitive","name":"first","num_inputs":1,"num_outputs":1},
                {"id":6,"type":"output-bar","num_inputs":1,"num_outputs":0}
            ],"wires":[
                {"id":1,"source_node":2,"source_pin":0,"target_node":4,"target_pin":0},
                {"id":2,"source_node":3,"source_pin":0,"target_node":4,"target_pin":1},
                {"id":3,"source_node":4,"source_pin":0,"target_node":5,"target_pin":0},
                {"id":4,"source_node":5,"source_pin":0,"target_node":6,"target_pin":0}
            ]}]}]}]})";
        assert_output(run(json, "f", "[]"), "\"a\"");
    }

    printf("OK\n");
}

// ============================================================
// 18. Bitwise logic
// ============================================================

static void test_bitwise() {
    printf("test_bitwise... ");

    assert_output(run(binary_project("bit-and").c_str(), "f", "[12, 10]"), "8");
    assert_output(run(binary_project("bit-or").c_str(), "f", "[12, 10]"), "14");
    assert_output(run(binary_project("bit-xor").c_str(), "f", "[12, 10]"), "6");
    assert_output(run(binary_project("bit-shift-left").c_str(), "f", "[1, 3]"), "8");
    assert_output(run(binary_project("bit-shift-right").c_str(), "f", "[8, 2]"), "2");

    printf("OK\n");
}

// ============================================================
// Main
// ============================================================

int main() {
    printf("=== Phograph Comprehensive Tests ===\n");

    // Arithmetic
    test_arith_basic();
    test_arith_advanced();
    test_arith_trig();

    // Comparison & logic
    test_comparison();
    test_logic();
    test_bitwise();

    // Strings
    test_string_basic();
    test_string_search();
    test_string_transform();

    // Lists
    test_list_basic();
    test_list_advanced();
    test_list_split();
    test_string_split_list();

    // Dicts
    test_dict();

    // Types
    test_type_checks();
    test_constants();

    // Control flow
    test_multi_case_dispatch();
    test_fibonacci();
    test_factorial();

    // Method calls
    test_method_calls();

    // OOP
    test_oop_basic();
    test_oop_inheritance();
    test_oop_default_attrs();

    // Console
    test_console();

    // Errors
    test_error_handling();

    // Canvas
    test_canvas();

    // Math library
    test_math_library();

    // Edge cases
    test_edge_empty_method();
    test_edge_fan_out();
    test_edge_deep_chain();
    test_edge_constant_only();
    test_edge_multiple_outputs();

    printf("\n=== All comprehensive tests passed! ===\n");
    return 0;
}
