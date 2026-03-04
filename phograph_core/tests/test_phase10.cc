#include "pho_codegen.h"
#include "pho_graph.h"
#include "pho_value.h"
#include "pho_prim.h"
#include "pho_eval.h"
#include <cassert>
#include <cstdio>
#include <string>

using namespace pho;

// Helper to build a simple graph: (a + b) * c
static Method make_add_mul_method() {
    Method m;
    m.name = "add-mul";
    m.num_inputs = 3;
    m.num_outputs = 1;

    Case c;

    // Input bar: 3 outputs (a, b, c)
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.name = "inputs";
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 3;
    c.input_bar_id = c.add_node(input_bar);

    // + node: 2 inputs, 1 output
    Node add_node;
    add_node.type = NodeType::Primitive;
    add_node.name = "+";
    add_node.num_inputs = 2;
    add_node.num_outputs = 1;
    NodeId add_id = c.add_node(add_node);

    // * node: 2 inputs, 1 output
    Node mul_node;
    mul_node.type = NodeType::Primitive;
    mul_node.name = "*";
    mul_node.num_inputs = 2;
    mul_node.num_outputs = 1;
    NodeId mul_id = c.add_node(mul_node);

    // Output bar: 1 input
    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.name = "outputs";
    output_bar.num_inputs = 1;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    // Wires: input[0] -> add[0], input[1] -> add[1]
    c.add_wire({c.input_bar_id, 0, true}, {add_id, 0, false});
    c.add_wire({c.input_bar_id, 1, true}, {add_id, 1, false});
    // add[0] -> mul[0], input[2] -> mul[1]
    c.add_wire({add_id, 0, true}, {mul_id, 0, false});
    c.add_wire({c.input_bar_id, 2, true}, {mul_id, 1, false});
    // mul[0] -> output[0]
    c.add_wire({mul_id, 0, true}, {c.output_bar_id, 0, false});

    m.cases.push_back(std::move(c));
    return m;
}

static void test_name_mangling() {
    printf("  test_name_mangling... ");
    SwiftCodegen gen;
    // Use compile_method to test name mangling indirectly:
    // method name "add-mul" should become "addMul"
    Method m = make_add_mul_method();
    Project p;
    p.name = "test";
    std::string src = gen.compile_method(p, m);
    assert(src.find("func addMul(") != std::string::npos);
    printf("OK\n");
}

static void test_compile_simple_method() {
    printf("  test_compile_simple_method... ");
    SwiftCodegen gen;
    Method m = make_add_mul_method();
    Project p;
    p.name = "test";

    std::string src = gen.compile_method(p, m);

    // Should contain function header
    assert(src.find("func addMul(") != std::string::npos);
    assert(src.find("PhoValue") != std::string::npos);

    // Should contain phoAdd and phoMul calls
    assert(src.find("phoAdd(") != std::string::npos);
    assert(src.find("phoMul(") != std::string::npos);

    // Should return
    assert(src.find("return ") != std::string::npos);
    printf("OK\n");
}

static void test_compile_constant_node() {
    printf("  test_compile_constant_node... ");
    SwiftCodegen gen;

    Method m;
    m.name = "get-42";
    m.num_inputs = 0;
    m.num_outputs = 1;

    Case c;
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 0;
    c.input_bar_id = c.add_node(input_bar);

    Node const_node;
    const_node.type = NodeType::Constant;
    const_node.name = "42";
    const_node.constant_value = Value::integer(42);
    const_node.num_inputs = 0;
    const_node.num_outputs = 1;
    NodeId const_id = c.add_node(const_node);

    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.num_inputs = 1;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    c.add_wire({const_id, 0, true}, {c.output_bar_id, 0, false});
    m.cases.push_back(std::move(c));

    Project p;
    p.name = "test";
    std::string src = gen.compile_method(p, m);

    assert(src.find("PhoValue.integer(42)") != std::string::npos);
    assert(src.find("return ") != std::string::npos);
    printf("OK\n");
}

static void test_compile_method_call() {
    printf("  test_compile_method_call... ");
    SwiftCodegen gen;

    Method m;
    m.name = "caller";
    m.num_inputs = 1;
    m.num_outputs = 1;

    Case c;
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 1;
    c.input_bar_id = c.add_node(input_bar);

    Node call_node;
    call_node.type = NodeType::MethodCall;
    call_node.name = "double-it";
    call_node.num_inputs = 1;
    call_node.num_outputs = 1;
    NodeId call_id = c.add_node(call_node);

    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.num_inputs = 1;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    c.add_wire({c.input_bar_id, 0, true}, {call_id, 0, false});
    c.add_wire({call_id, 0, true}, {c.output_bar_id, 0, false});
    m.cases.push_back(std::move(c));

    Project p;
    p.name = "test";
    std::string src = gen.compile_method(p, m);

    assert(src.find("doubleIt(") != std::string::npos);
    printf("OK\n");
}

static void test_compile_class() {
    printf("  test_compile_class... ");
    SwiftCodegen gen;

    ClassDef cls;
    cls.name = "Point";
    cls.attributes.push_back({"x", Value::integer(0), false});
    cls.attributes.push_back({"y", Value::integer(0), false});

    // Add a simple method: get-x (self -> x)
    Method get_x;
    get_x.name = "get-x";
    get_x.num_inputs = 1; // self
    get_x.num_outputs = 1;
    get_x.class_name = "Point";

    Case c;
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.num_inputs = 0;
    input_bar.num_outputs = 1;
    c.input_bar_id = c.add_node(input_bar);

    Node get_node;
    get_node.type = NodeType::Get;
    get_node.name = "x";
    get_node.num_inputs = 1;
    get_node.num_outputs = 2;
    NodeId get_id = c.add_node(get_node);

    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.num_inputs = 1;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    c.add_wire({c.input_bar_id, 0, true}, {get_id, 0, false});
    c.add_wire({get_id, 1, true}, {c.output_bar_id, 0, false});

    get_x.cases.push_back(std::move(c));
    cls.methods.push_back(std::move(get_x));

    Project p;
    p.name = "test";
    std::string src = gen.compile_class(p, cls);

    assert(src.find("class Point") != std::string::npos);
    assert(src.find("var x: PhoValue") != std::string::npos);
    assert(src.find("var y: PhoValue") != std::string::npos);
    assert(src.find("func getX(") != std::string::npos);
    printf("OK\n");
}

static void test_compile_multiple_cases() {
    printf("  test_compile_multiple_cases... ");
    SwiftCodegen gen;

    Method m;
    m.name = "abs-val";
    m.num_inputs = 1;
    m.num_outputs = 1;

    // Case 1: guard x >= 0, return x
    {
        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 1;
        c.input_bar_id = c.add_node(input_bar);

        Node const_zero;
        const_zero.type = NodeType::Constant;
        const_zero.constant_value = Value::integer(0);
        const_zero.num_inputs = 0;
        const_zero.num_outputs = 1;
        NodeId zero_id = c.add_node(const_zero);

        Node gte_node;
        gte_node.type = NodeType::Primitive;
        gte_node.name = ">=";
        gte_node.num_inputs = 2;
        gte_node.num_outputs = 1;
        gte_node.control = ControlType::NextCaseOnFailure;
        NodeId gte_id = c.add_node(gte_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 1;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c.input_bar_id, 0, true}, {gte_id, 0, false});
        c.add_wire({zero_id, 0, true}, {gte_id, 1, false});
        c.add_wire({c.input_bar_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }

    // Case 2: return negate(x)
    {
        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 1;
        c.input_bar_id = c.add_node(input_bar);

        Node neg_node;
        neg_node.type = NodeType::Primitive;
        neg_node.name = "negate";
        neg_node.num_inputs = 1;
        neg_node.num_outputs = 1;
        NodeId neg_id = c.add_node(neg_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 1;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c.input_bar_id, 0, true}, {neg_id, 0, false});
        c.add_wire({neg_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }

    Project p;
    p.name = "test";
    std::string src = gen.compile_method(p, m);

    // Should contain caseBlock for multiple cases
    assert(src.find("caseBlock") != std::string::npos);
    assert(src.find("guard") != std::string::npos);
    assert(src.find("phoNegate(") != std::string::npos);
    printf("OK\n");
}

static void test_compile_full_project() {
    printf("  test_compile_full_project... ");

    CodegenOptions opts;
    opts.emit_runtime = true;
    opts.emit_main = true;
    opts.entry_method = "main";
    SwiftCodegen gen(opts);

    Project p;
    p.name = "TestProject";

    Section s;
    s.name = "Main";

    // Add "double-it" method: x -> x * 2
    {
        Method m;
        m.name = "double-it";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 1;
        c.input_bar_id = c.add_node(input_bar);

        Node const_2;
        const_2.type = NodeType::Constant;
        const_2.constant_value = Value::integer(2);
        const_2.num_inputs = 0;
        const_2.num_outputs = 1;
        NodeId c2_id = c.add_node(const_2);

        Node mul_node;
        mul_node.type = NodeType::Primitive;
        mul_node.name = "*";
        mul_node.num_inputs = 2;
        mul_node.num_outputs = 1;
        NodeId mul_id = c.add_node(mul_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 1;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c.input_bar_id, 0, true}, {mul_id, 0, false});
        c.add_wire({c2_id, 0, true}, {mul_id, 1, false});
        c.add_wire({mul_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        s.methods.push_back(std::move(m));
    }

    // Add "main" method: () -> print(double-it(21))
    {
        Method m;
        m.name = "main";
        m.num_inputs = 0;
        m.num_outputs = 1;

        Case c;
        Node input_bar;
        input_bar.type = NodeType::InputBar;
        input_bar.num_inputs = 0;
        input_bar.num_outputs = 0;
        c.input_bar_id = c.add_node(input_bar);

        Node const_21;
        const_21.type = NodeType::Constant;
        const_21.constant_value = Value::integer(21);
        const_21.num_inputs = 0;
        const_21.num_outputs = 1;
        NodeId c21_id = c.add_node(const_21);

        Node call_node;
        call_node.type = NodeType::MethodCall;
        call_node.name = "double-it";
        call_node.num_inputs = 1;
        call_node.num_outputs = 1;
        NodeId call_id = c.add_node(call_node);

        Node print_node;
        print_node.type = NodeType::Primitive;
        print_node.name = "print";
        print_node.num_inputs = 1;
        print_node.num_outputs = 1;
        NodeId print_id = c.add_node(print_node);

        Node output_bar;
        output_bar.type = NodeType::OutputBar;
        output_bar.num_inputs = 1;
        output_bar.num_outputs = 0;
        c.output_bar_id = c.add_node(output_bar);

        c.add_wire({c21_id, 0, true}, {call_id, 0, false});
        c.add_wire({call_id, 0, true}, {print_id, 0, false});
        c.add_wire({print_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        s.methods.push_back(std::move(m));
    }

    p.sections.push_back(std::move(s));

    std::string source;
    std::vector<CodegenError> errors;
    bool ok = gen.compile(p, source, errors);
    assert(ok);

    // Verify source contains expected content
    assert(source.find("Generated by Phograph") != std::string::npos);
    assert(source.find("enum PhoValue") != std::string::npos);
    assert(source.find("func doubleIt(") != std::string::npos);
    assert(source.find("func pho_entry_main(") != std::string::npos);
    assert(source.find("phoMul(") != std::string::npos);
    assert(source.find("phoPrint(") != std::string::npos);

    // Verify the interpreter also gives correct result
    register_all_prims();
    Evaluator eval;
    auto result = eval.call_method(p, "double-it", {Value::integer(21)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs[0].as_integer() == 42);

    printf("OK\n");
}

static void test_compile_class_inheritance() {
    printf("  test_compile_class_inheritance... ");
    SwiftCodegen gen;

    ClassDef base;
    base.name = "Shape";
    base.attributes.push_back({"color", Value::string("red"), false});

    ClassDef child;
    child.name = "Circle";
    child.parent_name = "Shape";
    child.attributes.push_back({"radius", Value::integer(10), false});

    Project p;
    p.name = "test";
    Section s;
    s.name = "Main";
    s.classes.push_back(base);
    s.classes.push_back(child);
    p.sections.push_back(std::move(s));

    std::string base_src = gen.compile_class(p, *p.find_class("Shape"));
    std::string child_src = gen.compile_class(p, *p.find_class("Circle"));

    assert(base_src.find("class Shape") != std::string::npos);
    assert(child_src.find("class Circle: Shape") != std::string::npos);
    assert(child_src.find("var radius") != std::string::npos);
    printf("OK\n");
}

static void test_topo_sort_correctness() {
    printf("  test_topo_sort_correctness... ");

    // Verify the generated code has operations in correct order
    // by checking that phoAdd appears before phoMul in the output
    SwiftCodegen gen;
    Method m = make_add_mul_method();
    Project p;
    p.name = "test";
    std::string src = gen.compile_method(p, m);

    size_t add_pos = src.find("phoAdd(");
    size_t mul_pos = src.find("phoMul(");
    assert(add_pos != std::string::npos);
    assert(mul_pos != std::string::npos);
    assert(add_pos < mul_pos); // add must come before mul
    printf("OK\n");
}

static void test_runtime_contains_essentials() {
    printf("  test_runtime_contains_essentials... ");

    CodegenOptions opts;
    opts.emit_runtime = true;
    opts.emit_main = false;
    SwiftCodegen gen(opts);

    Project p;
    p.name = "empty";
    std::string source;
    std::vector<CodegenError> errors;
    gen.compile(p, source, errors);

    // Runtime should contain core types and functions
    assert(source.find("enum PhoValue") != std::string::npos);
    assert(source.find("class PhoObject") != std::string::npos);
    assert(source.find("func phoAdd") != std::string::npos);
    assert(source.find("func phoSub") != std::string::npos);
    assert(source.find("func phoMul") != std::string::npos);
    assert(source.find("func phoDiv") != std::string::npos);
    assert(source.find("func phoEqual") != std::string::npos);
    assert(source.find("func phoListCreate") != std::string::npos);
    assert(source.find("func phoPrint") != std::string::npos);
    printf("OK\n");
}

int main() {
    printf("=== Phase 10: Compiler (Graph -> Swift) Tests ===\n");
    test_name_mangling();
    test_compile_simple_method();
    test_compile_constant_node();
    test_compile_method_call();
    test_compile_class();
    test_compile_multiple_cases();
    test_compile_full_project();
    test_compile_class_inheritance();
    test_topo_sort_correctness();
    test_runtime_contains_essentials();
    printf("All Phase 10 tests passed!\n");
    return 0;
}
