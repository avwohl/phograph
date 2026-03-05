#include "pho_eval.h"
#include "pho_serial.h"
#include "pho_codegen.h"
#include "pho_prim.h"
#include "pho_graph.h"
#include "pho_value.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace pho;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Make a minimal project with one section and one universal method.
// The caller fills in the method's case(s) after calling this.
static Project make_project_with_method(const std::string& method_name,
                                         uint32_t num_inputs,
                                         uint32_t num_outputs) {
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = method_name;
    m.num_inputs = num_inputs;
    m.num_outputs = num_outputs;

    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));
    return proj;
}

// Build a case with input bar and output bar already wired.
// Returns references via output parameters so the caller can add more nodes/wires.
static Case make_basic_case(uint32_t num_inputs, uint32_t num_outputs) {
    Case c;

    // Input bar
    Node input_bar;
    input_bar.type = NodeType::InputBar;
    input_bar.name = "input";
    input_bar.num_inputs = 0;
    input_bar.num_outputs = num_inputs;
    c.input_bar_id = c.add_node(input_bar);

    // Output bar
    Node output_bar;
    output_bar.type = NodeType::OutputBar;
    output_bar.name = "output";
    output_bar.num_inputs = num_outputs;
    output_bar.num_outputs = 0;
    c.output_bar_id = c.add_node(output_bar);

    return c;
}

// ---------------------------------------------------------------------------
// Phase 11: Execution Wires
// ---------------------------------------------------------------------------

static void test_execution_wire_ordering() {
    printf("  test_execution_wire_ordering... ");

    // Build: inputBar(0: value 5) -> "+" node A (adds 5+5=10) -> outputBar
    //        constant(5) -> A.input1
    //        A --exec--> B (log node, which is a no-op sink)
    //        constant(5) -> B.input0
    // Because B has an execution input from A, B must not fire until A completes,
    // even though B's data input is filled first.
    // We verify by checking that the output bar receives A's result (10).

    Case c = make_basic_case(0, 1);

    // Constant 5 (node for A's first input)
    Node const5a;
    const5a.type = NodeType::Constant;
    const5a.name = "5a";
    const5a.num_inputs = 0;
    const5a.num_outputs = 1;
    const5a.constant_value = Value::integer(5);
    NodeId const5a_id = c.add_node(const5a);

    // Constant 5 (node for A's second input)
    Node const5b;
    const5b.type = NodeType::Constant;
    const5b.name = "5b";
    const5b.num_inputs = 0;
    const5b.num_outputs = 1;
    const5b.constant_value = Value::integer(5);
    NodeId const5b_id = c.add_node(const5b);

    // Node A: "+" primitive, 2 inputs, 1 output, has_execution_out
    Node nodeA;
    nodeA.type = NodeType::Primitive;
    nodeA.name = "+";
    nodeA.num_inputs = 2;
    nodeA.num_outputs = 1;
    nodeA.has_execution_out = true;
    NodeId a_id = c.add_node(nodeA);

    // Node B: "log" primitive (no outputs), has_execution_in via wire
    Node nodeB;
    nodeB.type = NodeType::Primitive;
    nodeB.name = "log";
    nodeB.num_inputs = 1;
    nodeB.num_outputs = 0;
    nodeB.has_execution_in = true;
    NodeId b_id = c.add_node(nodeB);

    // Wire const5a -> A.input0
    c.add_wire({const5a_id, 0, true}, {a_id, 0, false});
    // Wire const5b -> A.input1
    c.add_wire({const5b_id, 0, true}, {a_id, 1, false});
    // Wire const5a -> B.input0 (data)
    c.add_wire({const5a_id, 0, true}, {b_id, 0, false});
    // Execution wire: A -> B
    c.add_wire({a_id, 0, true}, {b_id, 0, false}, true);
    // Wire A.output0 -> output bar
    c.add_wire({a_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_exec_wire", 0, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_exec_wire", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 10);

    printf("OK\n");
}

static void test_execution_wire_blocks_without_trigger() {
    printf("  test_execution_wire_blocks_without_trigger... ");

    // Node B has an execution input (is marked via exec wire from A),
    // but A never fires (A's data inputs are never satisfied).
    // B has its data inputs filled from a constant, but should NOT fire
    // because the exec trigger never arrives.
    // The output bar is wired from B, so it should remain null.

    Case c = make_basic_case(0, 1);

    // A: a "+" node with 2 inputs but only 1 wired -> never fires
    Node nodeA;
    nodeA.type = NodeType::Primitive;
    nodeA.name = "+";
    nodeA.num_inputs = 2;
    nodeA.num_outputs = 1;
    nodeA.has_execution_out = true;
    NodeId a_id = c.add_node(nodeA);

    // Constant 42
    Node const42;
    const42.type = NodeType::Constant;
    const42.name = "42";
    const42.num_inputs = 0;
    const42.num_outputs = 1;
    const42.constant_value = Value::integer(42);
    NodeId c42_id = c.add_node(const42);

    // Wire constant -> A.input0 (but A.input1 is unwired, so A never fires)
    c.add_wire({c42_id, 0, true}, {a_id, 0, false});

    // B: "inspect" (1 in, 1 out), has execution input from A
    Node nodeB;
    nodeB.type = NodeType::Primitive;
    nodeB.name = "inspect";
    nodeB.num_inputs = 1;
    nodeB.num_outputs = 1;
    nodeB.has_execution_in = true;
    NodeId b_id = c.add_node(nodeB);

    // Wire constant -> B.input0
    c.add_wire({c42_id, 0, true}, {b_id, 0, false});
    // Execution wire A -> B
    c.add_wire({a_id, 0, true}, {b_id, 0, false}, true);
    // Wire B.output0 -> output bar
    c.add_wire({b_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_exec_block", 0, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_exec_block", {});
    // B never fired, so output bar input remains at default (null)
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_null());

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 12: Optional Inputs
// ---------------------------------------------------------------------------

static void test_optional_input_default() {
    printf("  test_optional_input_default... ");

    // Create a "+" node with 2 inputs, where input1 is optional with default 100.
    // Only wire input0 from constant 7. Expect 7+100 = 107.

    Case c = make_basic_case(0, 1);

    Node const7;
    const7.type = NodeType::Constant;
    const7.name = "7";
    const7.num_inputs = 0;
    const7.num_outputs = 1;
    const7.constant_value = Value::integer(7);
    NodeId c7_id = c.add_node(const7);

    Node addNode;
    addNode.type = NodeType::Primitive;
    addNode.name = "+";
    addNode.num_inputs = 2;
    addNode.num_outputs = 1;
    // input_defs: pin0 required, pin1 optional with default 100
    PinDef pin0;
    pin0.name = "a";
    pin0.is_optional = false;
    PinDef pin1;
    pin1.name = "b";
    pin1.is_optional = true;
    pin1.default_value = Value::integer(100);
    addNode.input_defs = {pin0, pin1};
    NodeId add_id = c.add_node(addNode);

    // Wire const7 -> add.input0 (but NOT input1 -- it should get default 100)
    c.add_wire({c7_id, 0, true}, {add_id, 0, false});
    // Wire add.output0 -> output bar
    c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_optional", 0, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_optional", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 107);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 13: JSON, Dates, String Prims
// ---------------------------------------------------------------------------

static void test_json_parse() {
    printf("  test_json_parse... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* jp = reg.find("json-parse");
    assert(jp);

    auto result = jp->fn({Value::string("{\"name\":\"Alice\",\"age\":30,\"items\":[1,2,3]}")});
    assert(!result.failed);
    assert(result.outputs.size() == 1);

    Value v = result.outputs[0];
    assert(v.is_dict());
    auto* d = v.as_dict();

    Value name = d->get(Value::string("name"));
    assert(name.is_string());
    assert(name.as_string()->str() == "Alice");

    Value age = d->get(Value::string("age"));
    assert(age.is_integer());
    assert(age.as_integer() == 30);

    Value items = d->get(Value::string("items"));
    assert(items.is_list());
    assert(items.as_list()->size() == 3);
    assert(items.as_list()->at(0).as_integer() == 1);
    assert(items.as_list()->at(2).as_integer() == 3);

    printf("OK\n");
}

static void test_json_encode() {
    printf("  test_json_encode... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* je = reg.find("json-encode");
    assert(je);

    auto dict = make_ref<PhoDict>();
    dict->set(Value::string("x"), Value::integer(42));
    Value dv = Value::dict(dict);

    auto result = je->fn({dv});
    assert(!result.failed);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_string());

    std::string json = result.outputs[0].as_string()->str();
    // Should contain "x":42
    assert(json.find("\"x\"") != std::string::npos);
    assert(json.find("42") != std::string::npos);

    printf("OK\n");
}

static void test_date_now() {
    printf("  test_date_now... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* dn = reg.find("date-now");
    assert(dn);

    auto result = dn->fn({});
    assert(!result.failed);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_date());
    // Timestamp should be a reasonable number (after 2020 epoch ~1.577e9)
    double ts = result.outputs[0].as_date();
    assert(ts > 1577836800.0);

    printf("OK\n");
}

static void test_date_create_components_roundtrip() {
    printf("  test_date_create_components_roundtrip... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* dc = reg.find("date-create");
    auto* dcomp = reg.find("date-components");
    assert(dc && dcomp);

    // Create date: 2025-03-15 10:30:45
    auto cr = dc->fn({
        Value::integer(2025), Value::integer(3), Value::integer(15),
        Value::integer(10), Value::integer(30), Value::integer(45)
    });
    assert(!cr.failed);
    assert(cr.outputs[0].is_date());

    // Extract components back
    auto comp = dcomp->fn({cr.outputs[0]});
    assert(!comp.failed);
    assert(comp.outputs.size() == 6);
    assert(comp.outputs[0].as_integer() == 2025);  // year
    assert(comp.outputs[1].as_integer() == 3);      // month
    assert(comp.outputs[2].as_integer() == 15);     // day
    assert(comp.outputs[3].as_integer() == 10);     // hour
    assert(comp.outputs[4].as_integer() == 30);     // minute
    assert(comp.outputs[5].as_integer() == 45);     // second

    printf("OK\n");
}

static void test_string_repeat() {
    printf("  test_string_repeat... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* sr = reg.find("string-repeat");
    assert(sr);

    auto result = sr->fn({Value::string("ab"), Value::integer(3)});
    assert(!result.failed);
    assert(result.outputs[0].as_string()->str() == "ababab");

    printf("OK\n");
}

static void test_string_starts_with() {
    printf("  test_string_starts_with... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* sw = reg.find("string-starts-with?");
    assert(sw);

    auto r1 = sw->fn({Value::string("hello world"), Value::string("hello")});
    assert(r1.outputs[0].as_boolean() == true);

    auto r2 = sw->fn({Value::string("hello world"), Value::string("world")});
    assert(r2.outputs[0].as_boolean() == false);

    printf("OK\n");
}

static void test_string_ends_with() {
    printf("  test_string_ends_with... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* ew = reg.find("string-ends-with?");
    assert(ew);

    auto r1 = ew->fn({Value::string("hello world"), Value::string("world")});
    assert(r1.outputs[0].as_boolean() == true);

    auto r2 = ew->fn({Value::string("hello world"), Value::string("hello")});
    assert(r2.outputs[0].as_boolean() == false);

    printf("OK\n");
}

static void test_to_from_codepoints() {
    printf("  test_to_from_codepoints... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* tcp = reg.find("to-codepoints");
    auto* fcp = reg.find("from-codepoints");
    assert(tcp && fcp);

    // to-codepoints("ABC") -> [65, 66, 67]
    auto r1 = tcp->fn({Value::string("ABC")});
    assert(!r1.failed);
    auto* list = r1.outputs[0].as_list();
    assert(list->size() == 3);
    assert(list->at(0).as_integer() == 65);
    assert(list->at(1).as_integer() == 66);
    assert(list->at(2).as_integer() == 67);

    // from-codepoints([65, 66, 67]) -> "ABC"
    auto r2 = fcp->fn({r1.outputs[0]});
    assert(!r2.failed);
    assert(r2.outputs[0].as_string()->str() == "ABC");

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 14: Method Refs, filter/reduce, enums
// ---------------------------------------------------------------------------

static void test_method_ref_call() {
    printf("  test_method_ref_call... ");

    // Create a project with a method "double" that returns input*2
    Project proj = make_project_with_method("double", 1, 1);
    {
        Case c = make_basic_case(1, 1);

        Node mul;
        mul.type = NodeType::Primitive;
        mul.name = "*";
        mul.num_inputs = 2;
        mul.num_outputs = 1;
        NodeId mul_id = c.add_node(mul);

        Node const2;
        const2.type = NodeType::Constant;
        const2.name = "2";
        const2.num_inputs = 0;
        const2.num_outputs = 1;
        const2.constant_value = Value::integer(2);
        NodeId c2_id = c.add_node(const2);

        c.add_wire({c.input_bar_id, 0, true}, {mul_id, 0, false});
        c.add_wire({c2_id, 0, true}, {mul_id, 1, false});
        c.add_wire({mul_id, 0, true}, {c.output_bar_id, 0, false});

        proj.sections[0].methods[0].cases.push_back(std::move(c));
    }

    // Create method-ref and call it
    auto& reg = PrimitiveRegistry::instance();
    auto* mr_prim = reg.find("method-ref");
    auto* call_prim = reg.find("call");
    assert(mr_prim && call_prim);

    Evaluator eval;
    // Set thread-locals so the call primitive can find the project/evaluator
    Evaluator::tl_project = &proj;
    Evaluator::tl_evaluator = &eval;

    auto mr_result = mr_prim->fn({Value::string("double")});
    assert(!mr_result.failed);
    Value mref = mr_result.outputs[0];
    assert(mref.is_method_ref());

    // call(method-ref, [7]) -> 14
    auto call_result = call_prim->fn({mref, Value::list({Value::integer(7)})});
    assert(!call_result.failed);
    assert(call_result.outputs[0].as_integer() == 14);

    Evaluator::tl_project = nullptr;
    Evaluator::tl_evaluator = nullptr;

    printf("OK\n");
}

static void test_filter_with_method_ref() {
    printf("  test_filter_with_method_ref... ");

    // Create a method "is-positive?" that returns input > 0
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = "is-positive?";
    m.num_inputs = 1;
    m.num_outputs = 1;
    {
        Case c = make_basic_case(1, 1);

        Node gt;
        gt.type = NodeType::Primitive;
        gt.name = ">";
        gt.num_inputs = 2;
        gt.num_outputs = 1;
        NodeId gt_id = c.add_node(gt);

        Node zero;
        zero.type = NodeType::Constant;
        zero.name = "0";
        zero.num_inputs = 0;
        zero.num_outputs = 1;
        zero.constant_value = Value::integer(0);
        NodeId z_id = c.add_node(zero);

        c.add_wire({c.input_bar_id, 0, true}, {gt_id, 0, false});
        c.add_wire({z_id, 0, true}, {gt_id, 1, false});
        c.add_wire({gt_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }
    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));

    Evaluator eval;
    Evaluator::tl_project = &proj;
    Evaluator::tl_evaluator = &eval;

    auto& reg = PrimitiveRegistry::instance();
    auto* mr_prim = reg.find("method-ref");
    auto mr_result = mr_prim->fn({Value::string("is-positive?")});
    Value mref = mr_result.outputs[0];

    auto* filter_prim = reg.find("filter");
    assert(filter_prim);

    Value input_list = Value::list({
        Value::integer(-3), Value::integer(5), Value::integer(-1),
        Value::integer(10), Value::integer(0)
    });
    auto result = filter_prim->fn({input_list, mref});
    assert(!result.failed);
    auto* out = result.outputs[0].as_list();
    assert(out->size() == 2);
    assert(out->at(0).as_integer() == 5);
    assert(out->at(1).as_integer() == 10);

    Evaluator::tl_project = nullptr;
    Evaluator::tl_evaluator = nullptr;

    printf("OK\n");
}

static void test_reduce_with_method_ref() {
    printf("  test_reduce_with_method_ref... ");

    // Create a method "add-two" that takes (a, b) and returns a + b
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = "add-two";
    m.num_inputs = 2;
    m.num_outputs = 1;
    {
        Case c = make_basic_case(2, 1);

        Node addN;
        addN.type = NodeType::Primitive;
        addN.name = "+";
        addN.num_inputs = 2;
        addN.num_outputs = 1;
        NodeId add_id = c.add_node(addN);

        c.add_wire({c.input_bar_id, 0, true}, {add_id, 0, false});
        c.add_wire({c.input_bar_id, 1, true}, {add_id, 1, false});
        c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }
    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));

    Evaluator eval;
    Evaluator::tl_project = &proj;
    Evaluator::tl_evaluator = &eval;

    auto& reg = PrimitiveRegistry::instance();
    auto mr_result = reg.find("method-ref")->fn({Value::string("add-two")});
    Value mref = mr_result.outputs[0];

    // reduce([1, 2, 3, 4], 0, add-two) -> 10
    auto* reduce_prim = reg.find("reduce");
    assert(reduce_prim);
    Value input_list = Value::list({
        Value::integer(1), Value::integer(2),
        Value::integer(3), Value::integer(4)
    });
    auto result = reduce_prim->fn({input_list, Value::integer(0), mref});
    assert(!result.failed);
    assert(result.outputs[0].as_integer() == 10);

    Evaluator::tl_project = nullptr;
    Evaluator::tl_evaluator = nullptr;

    printf("OK\n");
}

static void test_enum_create_variant_data() {
    printf("  test_enum_create_variant_data... ");

    auto& reg = PrimitiveRegistry::instance();
    auto* ec = reg.find("enum-create");
    auto* ev = reg.find("enum-variant");
    auto* ed = reg.find("enum-data");
    assert(ec && ev && ed);

    // Create an enum: Color/Red with data [255, 0, 0]
    auto cr = ec->fn({
        Value::string("Color"), Value::string("Red"),
        Value::list({Value::integer(255), Value::integer(0), Value::integer(0)})
    });
    assert(!cr.failed);
    assert(cr.outputs[0].is_enum());

    // Get variant name
    auto vr = ev->fn({cr.outputs[0]});
    assert(!vr.failed);
    assert(vr.outputs[0].as_string()->str() == "Red");

    // Get data
    auto dr = ed->fn({cr.outputs[0]});
    assert(!dr.failed);
    assert(dr.outputs[0].is_list());
    assert(dr.outputs[0].as_list()->size() == 3);
    assert(dr.outputs[0].as_list()->at(0).as_integer() == 255);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 15: Loops (TerminateOnFailure)
// ---------------------------------------------------------------------------

static void test_loop_count_to_10() {
    printf("  test_loop_count_to_10... ");

    // We simulate a "count to 10" loop using the evaluator's pattern-matching
    // multi-case approach:
    //   Case 1: check if input >= 10 -> TerminateOnFailure control
    //           (when input >= 10, the ">=" returns true, but we actually want
    //            to terminate -- so we use a node that checks >= 10 and has
    //            TerminateOnSuccess, meaning "stop when this succeeds")
    //   Case 2: return input + 1 (the loop body)
    //
    // But since we don't have explicit loop infra in the test, let's do a
    // simpler test: use multiple calls to simulate a loop manually,
    // verifying the control annotation behavior.

    // Build a method with one case that:
    //   input -> ">=" with constant 10 -> TerminateOnFailure control
    //   input -> "+1" -> output
    // When input < 10, ">=" yields false -> failure -> TerminateOnFailure -> propagate failure
    // When input >= 10, ">=" yields true -> success -> continue -> output input+1
    // Wait, that's inverted. Let's think again:
    //   TerminateOnFailure: on failure, terminate (return failure)
    //   We want to continue while < 10 and stop at >= 10.
    //   Use ">=" with NextCaseOnSuccess: if >= 10, jump to next case (which we
    //   make return the value). If < 10, failure falls through via ContinueOnFailure.
    //   Then increment and return.

    // Actually, let's just test that TerminateOnFailure works:
    // A case where a comparison node has TerminateOnFailure.
    // If comparison fails, the whole case returns failure.

    Project proj = make_project_with_method("check-limit", 1, 1);
    {
        Case c = make_basic_case(1, 1);

        // ">=" node: input >= 10
        Node geq;
        geq.type = NodeType::Primitive;
        geq.name = ">=";
        geq.num_inputs = 2;
        geq.num_outputs = 1;
        geq.control = ControlType::TerminateOnFailure;
        NodeId geq_id = c.add_node(geq);

        Node const10;
        const10.type = NodeType::Constant;
        const10.name = "10";
        const10.num_inputs = 0;
        const10.num_outputs = 1;
        const10.constant_value = Value::integer(10);
        NodeId c10_id = c.add_node(const10);

        c.add_wire({c.input_bar_id, 0, true}, {geq_id, 0, false});
        c.add_wire({c10_id, 0, true}, {geq_id, 1, false});
        // Also pass input through to output
        c.add_wire({c.input_bar_id, 0, true}, {c.output_bar_id, 0, false});

        proj.sections[0].methods[0].cases.push_back(std::move(c));
    }

    Evaluator eval;

    // Input 5: ">=" returns false -> failure -> TerminateOnFailure -> returns Failure
    auto r1 = eval.call_method(proj, "check-limit", {Value::integer(5)});
    assert(r1.status == EvalStatus::Failure);

    // Input 10: ">=" returns true -> success -> pass through -> output is 10
    auto r2 = eval.call_method(proj, "check-limit", {Value::integer(10)});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].as_integer() == 10);

    // Input 15: same as 10, succeeds
    auto r3 = eval.call_method(proj, "check-limit", {Value::integer(15)});
    assert(r3.status == EvalStatus::Success);
    assert(r3.outputs[0].as_integer() == 15);

    printf("OK\n");
}

// Test eval_loop via is_loop=true on a method call node
static void test_loop_is_loop_annotation() {
    printf("  test_loop_is_loop_annotation... ");

    // Create a "step" method that increments input by 1.
    // Case 1 has a ">=" check with TerminateOnFailure: if input >= 10,
    //   the ">=" succeeds, but we want to stop, so we use NextCaseOnSuccess
    //   to force moving to the next case which doesn't exist => overall failure.
    // Actually, simpler approach: step method has 2 cases.
    //   Case 1: ">=" 10 with NextCaseOnSuccess. If input >= 10, this succeeds
    //           -> NextCaseOnSuccess -> method returns failure (loop termination).
    //   Case 2: increment and return.

    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    // "step" method: check >= 10 (fail if true), otherwise return n+1
    {
        Method step;
        step.name = "step";
        step.num_inputs = 1;
        step.num_outputs = 1;

        // Case 1: check n < 10 with FailOnFailure. If n < 10 passes,
        // continue to increment. If n >= 10, < fails => FailOnFailure => case fails.
        {
            Case c = make_basic_case(1, 1);

            // "<" node: input < 10
            Node lt;
            lt.type = NodeType::Primitive;
            lt.name = "<";
            lt.num_inputs = 2;
            lt.num_outputs = 1;
            lt.control = ControlType::FailOnFailure; // if < 10 fails (n>=10) => fail case
            NodeId lt_id = c.add_node(lt);

            Node c10;
            c10.type = NodeType::Constant;
            c10.num_inputs = 0;
            c10.num_outputs = 1;
            c10.constant_value = Value::integer(10);
            NodeId c10_id = c.add_node(c10);

            // Wire input->lt pin 0, const 10->lt pin 1
            c.add_wire({c.input_bar_id, 0, true}, {lt_id, 0, false});
            c.add_wire({c10_id, 0, true}, {lt_id, 1, false});

            // Use exec wire from lt to add, so add only fires after check passes
            Node add;
            add.type = NodeType::Primitive;
            add.name = "+";
            add.num_inputs = 2;
            add.num_outputs = 1;
            NodeId add_id = c.add_node(add);

            Node c1;
            c1.type = NodeType::Constant;
            c1.num_inputs = 0;
            c1.num_outputs = 1;
            c1.constant_value = Value::integer(1);
            NodeId c1_id = c.add_node(c1);

            c.add_wire({c.input_bar_id, 0, true}, {add_id, 0, false});
            c.add_wire({c1_id, 0, true}, {add_id, 1, false});
            c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});

            step.cases.push_back(std::move(c));
        }

        sec.methods.push_back(std::move(step));
    }

    // "f" method: calls step in a loop starting from 0
    {
        Method f;
        f.name = "f";
        f.num_inputs = 0;
        f.num_outputs = 1;

        Case c = make_basic_case(0, 1);

        // Constant 0 (start value)
        Node c0;
        c0.type = NodeType::Constant;
        c0.num_inputs = 0;
        c0.num_outputs = 1;
        c0.constant_value = Value::integer(0);
        NodeId c0_id = c.add_node(c0);

        // Loop node: calls "step" repeatedly with is_loop=true
        Node loop;
        loop.type = NodeType::MethodCall;
        loop.name = "step";
        loop.num_inputs = 1;
        loop.num_outputs = 1;
        loop.is_loop = true;
        NodeId loop_id = c.add_node(loop);

        c.add_wire({c0_id, 0, true}, {loop_id, 0, false});
        c.add_wire({loop_id, 0, true}, {c.output_bar_id, 0, false});

        f.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(f));
    }

    proj.sections.push_back(std::move(sec));

    Evaluator eval;
    auto r = eval.call_method(proj, "f", {});
    assert(r.status == EvalStatus::Success);
    // Loop starts at 0, step returns 1,2,...,9,10.
    // When input=10, >=10 succeeds, FailOnSuccess -> method fails -> loop returns last input.
    // Last input was 10 (the value passed to the step that failed).
    assert(r.outputs[0].as_integer() == 10);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 16: List Annotations (ListMap)
// ---------------------------------------------------------------------------

static void test_list_map_annotation() {
    printf("  test_list_map_annotation... ");

    // Node with list_map=true, input list [1,2,3], primitive "+", scalar 10
    // -> output [11, 12, 13]

    Case c = make_basic_case(0, 1);

    // Constant list [1, 2, 3]
    Node constList;
    constList.type = NodeType::Constant;
    constList.name = "list";
    constList.num_inputs = 0;
    constList.num_outputs = 1;
    constList.constant_value = Value::list({
        Value::integer(1), Value::integer(2), Value::integer(3)
    });
    NodeId list_id = c.add_node(constList);

    // Constant scalar 10
    Node const10;
    const10.type = NodeType::Constant;
    const10.name = "10";
    const10.num_inputs = 0;
    const10.num_outputs = 1;
    const10.constant_value = Value::integer(10);
    NodeId c10_id = c.add_node(const10);

    // "+" node with list_map=true
    Node addNode;
    addNode.type = NodeType::Primitive;
    addNode.name = "+";
    addNode.num_inputs = 2;
    addNode.num_outputs = 1;
    addNode.list_map = true;
    NodeId add_id = c.add_node(addNode);

    // Wire list -> add.input0 (this is the list input)
    c.add_wire({list_id, 0, true}, {add_id, 0, false});
    // Wire scalar 10 -> add.input1
    c.add_wire({c10_id, 0, true}, {add_id, 1, false});
    // Wire add.output0 -> output bar
    c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_list_map", 0, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_list_map", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_list());

    auto* out = result.outputs[0].as_list();
    assert(out->size() == 3);
    assert(out->at(0).as_integer() == 11);
    assert(out->at(1).as_integer() == 12);
    assert(out->at(2).as_integer() == 13);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 17: Try / Error
// ---------------------------------------------------------------------------

static void test_try_catches_error() {
    printf("  test_try_catches_error... ");

    // Use an unknown-primitive node to trigger EvalStatus::Error.
    // exec_node returns make_error("unknown primitive: ...") for unknown prims.
    // has_try catches it and routes the error to the error_out_pin.

    Case c = make_basic_case(0, 2);

    Node constVal;
    constVal.type = NodeType::Constant;
    constVal.name = "val";
    constVal.num_inputs = 0;
    constVal.num_outputs = 1;
    constVal.constant_value = Value::integer(42);
    NodeId cv_id = c.add_node(constVal);

    // A primitive node referencing a nonexistent primitive triggers make_error
    Node badNode;
    badNode.type = NodeType::Primitive;
    badNode.name = "no-such-prim-xyz";
    badNode.num_inputs = 1;
    badNode.num_outputs = 2;
    badNode.has_try = true;
    badNode.error_out_pin = 1;
    NodeId bad_id = c.add_node(badNode);

    c.add_wire({cv_id, 0, true}, {bad_id, 0, false});
    c.add_wire({bad_id, 0, true}, {c.output_bar_id, 0, false});
    c.add_wire({bad_id, 1, true}, {c.output_bar_id, 1, false});

    Project proj = make_project_with_method("test_try", 0, 2);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_try", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 2);
    // Pin 0 should be null (no normal output on error)
    assert(result.outputs[0].is_null());
    // Pin 1 should be the error value
    assert(result.outputs[1].is_error());
    assert(result.outputs[1].as_error()->message().find("unknown primitive") != std::string::npos);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 20: Evaluation Nodes (Inline Expressions)
// ---------------------------------------------------------------------------

static void test_evaluation_node() {
    printf("  test_evaluation_node... ");

    // Evaluation node with expression "a + b * 2"
    // inputs: a=3, b=4 -> output 11

    Case c = make_basic_case(2, 1);

    Node evalNode;
    evalNode.type = NodeType::Evaluation;
    evalNode.name = "eval";
    evalNode.num_inputs = 2;
    evalNode.num_outputs = 1;
    evalNode.expression = "a + b * 2";
    NodeId eval_id = c.add_node(evalNode);

    // Wire input bar pin 0 -> eval.input0 (bound as "a")
    c.add_wire({c.input_bar_id, 0, true}, {eval_id, 0, false});
    // Wire input bar pin 1 -> eval.input1 (bound as "b")
    c.add_wire({c.input_bar_id, 1, true}, {eval_id, 1, false});
    // Wire eval.output0 -> output bar
    c.add_wire({eval_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_eval_node", 2, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_eval_node", {Value::integer(3), Value::integer(4)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].is_integer());
    assert(result.outputs[0].as_integer() == 11);

    printf("OK\n");
}

static void test_evaluation_node_boolean_logic() {
    printf("  test_evaluation_node_boolean_logic... ");

    // Expression: a > 5 && b < 20
    // inputs: a=10, b=15 -> true
    Case c = make_basic_case(2, 1);

    Node evalNode;
    evalNode.type = NodeType::Evaluation;
    evalNode.name = "eval";
    evalNode.num_inputs = 2;
    evalNode.num_outputs = 1;
    evalNode.expression = "a > 5 && b < 20";
    NodeId eval_id = c.add_node(evalNode);

    c.add_wire({c.input_bar_id, 0, true}, {eval_id, 0, false});
    c.add_wire({c.input_bar_id, 1, true}, {eval_id, 1, false});
    c.add_wire({eval_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_eval_bool", 2, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;

    // Both conditions true: 10 > 5 && 15 < 20 -> true
    auto r1 = eval.call_method(proj, "test_eval_bool", {Value::integer(10), Value::integer(15)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].is_boolean());
    assert(r1.outputs[0].as_boolean() == true);

    // First false: 3 > 5 is false -> false
    auto r2 = eval.call_method(proj, "test_eval_bool", {Value::integer(3), Value::integer(15)});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].as_boolean() == false);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 21: Inject (Dynamic Dispatch)
// ---------------------------------------------------------------------------

static void test_inject_dispatches_to_method() {
    printf("  test_inject_dispatches_to_method... ");

    // Create a project with a method "square" that returns x*x
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    {
        Method m;
        m.name = "square";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c = make_basic_case(1, 1);

        Node mul;
        mul.type = NodeType::Primitive;
        mul.name = "*";
        mul.num_inputs = 2;
        mul.num_outputs = 1;
        NodeId mul_id = c.add_node(mul);

        c.add_wire({c.input_bar_id, 0, true}, {mul_id, 0, false});
        c.add_wire({c.input_bar_id, 0, true}, {mul_id, 1, false});
        c.add_wire({mul_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    // Create method "test_inject" that uses an Inject node
    {
        Method m;
        m.name = "test_inject";
        m.num_inputs = 1;  // the value to square
        m.num_outputs = 1;

        Case c = make_basic_case(1, 1);

        // Constant string "square" (the method name)
        Node constName;
        constName.type = NodeType::Constant;
        constName.name = "name";
        constName.num_inputs = 0;
        constName.num_outputs = 1;
        constName.constant_value = Value::string("square");
        NodeId name_id = c.add_node(constName);

        // Inject node: input0 = method name, input1 = arg
        Node injectNode;
        injectNode.type = NodeType::Inject;
        injectNode.name = "inject";
        injectNode.num_inputs = 2;
        injectNode.num_outputs = 1;
        NodeId inj_id = c.add_node(injectNode);

        c.add_wire({name_id, 0, true}, {inj_id, 0, false});
        c.add_wire({c.input_bar_id, 0, true}, {inj_id, 1, false});
        c.add_wire({inj_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    proj.sections.push_back(std::move(sec));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_inject", {Value::integer(7)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 1);
    assert(result.outputs[0].as_integer() == 49);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 22: Persistent Nodes
// ---------------------------------------------------------------------------

static void test_persistent_write_read() {
    printf("  test_persistent_write_read... ");

    // Method 1: write to persistent "counter" with value from input
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    {
        Method m;
        m.name = "write_persist";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c = make_basic_case(1, 1);

        Node pNode;
        pNode.type = NodeType::Persistent;
        pNode.name = "counter";
        pNode.num_inputs = 1;
        pNode.num_outputs = 1;
        pNode.constant_value = Value::integer(0); // default
        NodeId p_id = c.add_node(pNode);

        c.add_wire({c.input_bar_id, 0, true}, {p_id, 0, false});
        c.add_wire({p_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    // Method 2: read from persistent "counter" (no input wired -> read mode)
    {
        Method m;
        m.name = "read_persist";
        m.num_inputs = 0;
        m.num_outputs = 1;

        Case c = make_basic_case(0, 1);

        Node pNode;
        pNode.type = NodeType::Persistent;
        pNode.name = "counter";
        pNode.num_inputs = 1;
        pNode.num_outputs = 1;
        pNode.constant_value = Value::integer(0); // default

        // No input wired -> read mode. But we need num_inputs=1 with
        // an optional setup so the node can fire with 0 filled.
        PinDef pd;
        pd.name = "value";
        pd.is_optional = true;
        pd.default_value = Value::null_val();
        pNode.input_defs = {pd};

        NodeId p_id = c.add_node(pNode);

        c.add_wire({p_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    proj.sections.push_back(std::move(sec));

    Evaluator eval;

    // Write 42
    auto r1 = eval.call_method(proj, "write_persist", {Value::integer(42)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].as_integer() == 42);

    // Read back
    auto r2 = eval.call_method(proj, "read_persist", {});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs.size() == 1);
    assert(r2.outputs[0].as_integer() == 42);

    // Write 99, read back
    eval.call_method(proj, "write_persist", {Value::integer(99)});
    auto r3 = eval.call_method(proj, "read_persist", {});
    assert(r3.status == EvalStatus::Success);
    assert(r3.outputs[0].as_integer() == 99);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 23: Pattern Matching (Case Guards)
// ---------------------------------------------------------------------------

static void test_pattern_matching_type_guard() {
    printf("  test_pattern_matching_type_guard... ");

    // Method "describe" with two cases:
    //   Case 1: guard on pin 0 type=integer -> returns "int:<value>"
    //   Case 2: guard on pin 0 type=string  -> returns "str:<value>"

    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = "describe";
    m.num_inputs = 1;
    m.num_outputs = 1;

    // Case 1: integer guard
    {
        Case c = make_basic_case(1, 1);

        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::TypeMatch;
        guard.match_type = "integer";
        c.guards.push_back(guard);

        // Use "to-string" to convert, then concat with "int:"
        Node toStr;
        toStr.type = NodeType::Primitive;
        toStr.name = "to-string";
        toStr.num_inputs = 1;
        toStr.num_outputs = 1;
        NodeId ts_id = c.add_node(toStr);

        Node prefix;
        prefix.type = NodeType::Constant;
        prefix.name = "prefix";
        prefix.num_inputs = 0;
        prefix.num_outputs = 1;
        prefix.constant_value = Value::string("int:");
        NodeId pf_id = c.add_node(prefix);

        Node concatNode;
        concatNode.type = NodeType::Primitive;
        concatNode.name = "concat";
        concatNode.num_inputs = 2;
        concatNode.num_outputs = 1;
        NodeId cat_id = c.add_node(concatNode);

        c.add_wire({c.input_bar_id, 0, true}, {ts_id, 0, false});
        c.add_wire({pf_id, 0, true}, {cat_id, 0, false});
        c.add_wire({ts_id, 0, true}, {cat_id, 1, false});
        c.add_wire({cat_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }

    // Case 2: string guard
    {
        Case c = make_basic_case(1, 1);

        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::TypeMatch;
        guard.match_type = "string";
        c.guards.push_back(guard);

        Node prefix;
        prefix.type = NodeType::Constant;
        prefix.name = "prefix";
        prefix.num_inputs = 0;
        prefix.num_outputs = 1;
        prefix.constant_value = Value::string("str:");
        NodeId pf_id = c.add_node(prefix);

        Node concatNode;
        concatNode.type = NodeType::Primitive;
        concatNode.name = "concat";
        concatNode.num_inputs = 2;
        concatNode.num_outputs = 1;
        NodeId cat_id = c.add_node(concatNode);

        c.add_wire({pf_id, 0, true}, {cat_id, 0, false});
        c.add_wire({c.input_bar_id, 0, true}, {cat_id, 1, false});
        c.add_wire({cat_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
    }

    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));

    Evaluator eval;

    // Pass integer
    auto r1 = eval.call_method(proj, "describe", {Value::integer(42)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].is_string());
    assert(r1.outputs[0].as_string()->str() == "int:42");

    // Pass string
    auto r2 = eval.call_method(proj, "describe", {Value::string("hello")});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].is_string());
    assert(r2.outputs[0].as_string()->str() == "str:hello");

    // Pass boolean (neither guard matches) -> should fail
    auto r3 = eval.call_method(proj, "describe", {Value::boolean(true)});
    assert(r3.status == EvalStatus::Failure);

    printf("OK\n");
}

static void test_pattern_matching_value_guard() {
    printf("  test_pattern_matching_value_guard... ");

    // Method "check-code" with two cases:
    //   Case 1: guard pin 0 value match = 200 -> returns "ok"
    //   Case 2: guard pin 0 value match = 404 -> returns "not-found"

    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = "check-code";
    m.num_inputs = 1;
    m.num_outputs = 1;

    // Case 1: value 200
    {
        Case c = make_basic_case(1, 1);
        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::ValueMatch;
        guard.match_val = Value::integer(200);
        c.guards.push_back(guard);

        Node constOK;
        constOK.type = NodeType::Constant;
        constOK.name = "ok";
        constOK.num_inputs = 0;
        constOK.num_outputs = 1;
        constOK.constant_value = Value::string("ok");
        NodeId ok_id = c.add_node(constOK);

        c.add_wire({ok_id, 0, true}, {c.output_bar_id, 0, false});
        m.cases.push_back(std::move(c));
    }

    // Case 2: value 404
    {
        Case c = make_basic_case(1, 1);
        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::ValueMatch;
        guard.match_val = Value::integer(404);
        c.guards.push_back(guard);

        Node constNF;
        constNF.type = NodeType::Constant;
        constNF.name = "nf";
        constNF.num_inputs = 0;
        constNF.num_outputs = 1;
        constNF.constant_value = Value::string("not-found");
        NodeId nf_id = c.add_node(constNF);

        c.add_wire({nf_id, 0, true}, {c.output_bar_id, 0, false});
        m.cases.push_back(std::move(c));
    }

    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));

    Evaluator eval;

    auto r1 = eval.call_method(proj, "check-code", {Value::integer(200)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].as_string()->str() == "ok");

    auto r2 = eval.call_method(proj, "check-code", {Value::integer(404)});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].as_string()->str() == "not-found");

    // 500: no guard matches -> failure
    auto r3 = eval.call_method(proj, "check-code", {Value::integer(500)});
    assert(r3.status == EvalStatus::Failure);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Additional Phase 20 tests: expression with comparison and ternary
// ---------------------------------------------------------------------------

static void test_evaluation_node_ternary() {
    printf("  test_evaluation_node_ternary... ");

    // Expression: a > 0 ? a : -a   (absolute value via ternary)
    Case c = make_basic_case(1, 1);

    Node evalNode;
    evalNode.type = NodeType::Evaluation;
    evalNode.name = "abs_eval";
    evalNode.num_inputs = 1;
    evalNode.num_outputs = 1;
    evalNode.expression = "a > 0 ? a : -a";
    NodeId eval_id = c.add_node(evalNode);

    c.add_wire({c.input_bar_id, 0, true}, {eval_id, 0, false});
    c.add_wire({eval_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_abs_eval", 1, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;

    auto r1 = eval.call_method(proj, "test_abs_eval", {Value::integer(5)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].as_integer() == 5);

    auto r2 = eval.call_method(proj, "test_abs_eval", {Value::integer(-3)});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].as_integer() == 3);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Additional Phase 12 test: optional input WITH wire (wire takes precedence)
// ---------------------------------------------------------------------------

static void test_optional_input_with_wire_overrides_default() {
    printf("  test_optional_input_with_wire_overrides_default... ");

    // Same setup as test_optional_input_default, but now we also wire input1.
    // The wired value (3) should override the default (100).
    // So 7 + 3 = 10.

    Case c = make_basic_case(0, 1);

    Node const7;
    const7.type = NodeType::Constant;
    const7.name = "7";
    const7.num_inputs = 0;
    const7.num_outputs = 1;
    const7.constant_value = Value::integer(7);
    NodeId c7_id = c.add_node(const7);

    Node const3;
    const3.type = NodeType::Constant;
    const3.name = "3";
    const3.num_inputs = 0;
    const3.num_outputs = 1;
    const3.constant_value = Value::integer(3);
    NodeId c3_id = c.add_node(const3);

    Node addNode;
    addNode.type = NodeType::Primitive;
    addNode.name = "+";
    addNode.num_inputs = 2;
    addNode.num_outputs = 1;
    PinDef pin0;
    pin0.name = "a";
    pin0.is_optional = false;
    PinDef pin1;
    pin1.name = "b";
    pin1.is_optional = true;
    pin1.default_value = Value::integer(100);
    addNode.input_defs = {pin0, pin1};
    NodeId add_id = c.add_node(addNode);

    c.add_wire({c7_id, 0, true}, {add_id, 0, false});
    c.add_wire({c3_id, 0, true}, {add_id, 1, false});
    c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});

    Project proj = make_project_with_method("test_opt_override", 0, 1);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_opt_override", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs[0].as_integer() == 10);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Additional Phase 17: Try with successful operation (no error)
// ---------------------------------------------------------------------------

static void test_try_no_error() {
    printf("  test_try_no_error... ");

    // A "+" node with has_try that succeeds normally.
    // Output pin 0 = result, pin 1 = error (should be null on success).

    Case c = make_basic_case(0, 2);

    Node const3;
    const3.type = NodeType::Constant;
    const3.name = "3";
    const3.num_inputs = 0;
    const3.num_outputs = 1;
    const3.constant_value = Value::integer(3);
    NodeId c3_id = c.add_node(const3);

    Node const4;
    const4.type = NodeType::Constant;
    const4.name = "4";
    const4.num_inputs = 0;
    const4.num_outputs = 1;
    const4.constant_value = Value::integer(4);
    NodeId c4_id = c.add_node(const4);

    Node addNode;
    addNode.type = NodeType::Primitive;
    addNode.name = "+";
    addNode.num_inputs = 2;
    addNode.num_outputs = 2;
    addNode.has_try = true;
    addNode.error_out_pin = 1;
    NodeId add_id = c.add_node(addNode);

    c.add_wire({c3_id, 0, true}, {add_id, 0, false});
    c.add_wire({c4_id, 0, true}, {add_id, 1, false});
    c.add_wire({add_id, 0, true}, {c.output_bar_id, 0, false});
    c.add_wire({add_id, 1, true}, {c.output_bar_id, 1, false});

    Project proj = make_project_with_method("test_try_ok", 0, 2);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_try_ok", {});
    assert(result.status == EvalStatus::Success);
    // Pin 0 = normal result (3+4=7)
    assert(result.outputs[0].as_integer() == 7);
    // Pin 1 = error, should be null since operation succeeded
    assert(result.outputs[1].is_null());

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 16: Partition annotation
// ---------------------------------------------------------------------------

static void test_partition_annotation() {
    printf("  test_partition_annotation... ");

    // Partition [1,2,3,4,5,6] through ">" with threshold 3
    // Pass list: [4,5,6], Fail list: [1,2,3]

    Case c = make_basic_case(0, 2);

    Node constList;
    constList.type = NodeType::Constant;
    constList.name = "list";
    constList.num_inputs = 0;
    constList.num_outputs = 1;
    constList.constant_value = Value::list({
        Value::integer(1), Value::integer(2), Value::integer(3),
        Value::integer(4), Value::integer(5), Value::integer(6)
    });
    NodeId list_id = c.add_node(constList);

    Node const3;
    const3.type = NodeType::Constant;
    const3.name = "3";
    const3.num_inputs = 0;
    const3.num_outputs = 1;
    const3.constant_value = Value::integer(3);
    NodeId c3_id = c.add_node(const3);

    Node gtNode;
    gtNode.type = NodeType::Primitive;
    gtNode.name = ">";
    gtNode.num_inputs = 2;
    gtNode.num_outputs = 2;  // partition produces 2 outputs: pass list and fail list
    gtNode.partition = true;
    NodeId gt_id = c.add_node(gtNode);

    c.add_wire({list_id, 0, true}, {gt_id, 0, false});
    c.add_wire({c3_id, 0, true}, {gt_id, 1, false});
    c.add_wire({gt_id, 0, true}, {c.output_bar_id, 0, false});
    c.add_wire({gt_id, 1, true}, {c.output_bar_id, 1, false});

    Project proj = make_project_with_method("test_partition", 0, 2);
    proj.sections[0].methods[0].cases.push_back(std::move(c));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_partition", {});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs.size() == 2);

    // Pass list (elements where > 3 is true)
    auto* pass = result.outputs[0].as_list();
    assert(pass->size() == 3);
    assert(pass->at(0).as_integer() == 4);
    assert(pass->at(1).as_integer() == 5);
    assert(pass->at(2).as_integer() == 6);

    // Fail list (elements where > 3 is false)
    auto* fail = result.outputs[1].as_list();
    assert(fail->size() == 3);
    assert(fail->at(0).as_integer() == 1);
    assert(fail->at(1).as_integer() == 2);
    assert(fail->at(2).as_integer() == 3);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 21: Inject with method-ref input
// ---------------------------------------------------------------------------

static void test_inject_with_method_ref() {
    printf("  test_inject_with_method_ref... ");

    // Create "negate" method that returns -x
    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    {
        Method m;
        m.name = "negate";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c = make_basic_case(1, 1);

        Node const0;
        const0.type = NodeType::Constant;
        const0.name = "0";
        const0.num_inputs = 0;
        const0.num_outputs = 1;
        const0.constant_value = Value::integer(0);
        NodeId c0_id = c.add_node(const0);

        Node sub;
        sub.type = NodeType::Primitive;
        sub.name = "-";
        sub.num_inputs = 2;
        sub.num_outputs = 1;
        NodeId sub_id = c.add_node(sub);

        c.add_wire({c0_id, 0, true}, {sub_id, 0, false});
        c.add_wire({c.input_bar_id, 0, true}, {sub_id, 1, false});
        c.add_wire({sub_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    // Create method that uses inject with a method-ref value
    {
        Method m;
        m.name = "test_inject_mr";
        m.num_inputs = 1;
        m.num_outputs = 1;

        Case c = make_basic_case(1, 1);

        // method-ref("negate") node
        Node mrNode;
        mrNode.type = NodeType::Primitive;
        mrNode.name = "method-ref";
        mrNode.num_inputs = 1;
        mrNode.num_outputs = 1;
        NodeId mr_id = c.add_node(mrNode);

        Node constName;
        constName.type = NodeType::Constant;
        constName.name = "name";
        constName.num_inputs = 0;
        constName.num_outputs = 1;
        constName.constant_value = Value::string("negate");
        NodeId name_id = c.add_node(constName);

        // Inject node
        Node injectNode;
        injectNode.type = NodeType::Inject;
        injectNode.name = "inject";
        injectNode.num_inputs = 2;
        injectNode.num_outputs = 1;
        NodeId inj_id = c.add_node(injectNode);

        c.add_wire({name_id, 0, true}, {mr_id, 0, false});
        c.add_wire({mr_id, 0, true}, {inj_id, 0, false});
        c.add_wire({c.input_bar_id, 0, true}, {inj_id, 1, false});
        c.add_wire({inj_id, 0, true}, {c.output_bar_id, 0, false});

        m.cases.push_back(std::move(c));
        sec.methods.push_back(std::move(m));
    }

    proj.sections.push_back(std::move(sec));

    Evaluator eval;
    auto result = eval.call_method(proj, "test_inject_mr", {Value::integer(7)});
    assert(result.status == EvalStatus::Success);
    assert(result.outputs[0].as_integer() == -7);

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 23: Wildcard guard
// ---------------------------------------------------------------------------

static void test_pattern_matching_wildcard_fallback() {
    printf("  test_pattern_matching_wildcard_fallback... ");

    // Method with:
    //   Case 1: TypeMatch integer -> "integer"
    //   Case 2: Wildcard -> "other"

    Project proj;
    proj.name = "test";
    Section sec;
    sec.name = "main";

    Method m;
    m.name = "classify";
    m.num_inputs = 1;
    m.num_outputs = 1;

    // Case 1: integer
    {
        Case c = make_basic_case(1, 1);
        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::TypeMatch;
        guard.match_type = "integer";
        c.guards.push_back(guard);

        Node constStr;
        constStr.type = NodeType::Constant;
        constStr.name = "result";
        constStr.num_inputs = 0;
        constStr.num_outputs = 1;
        constStr.constant_value = Value::string("integer");
        NodeId cs_id = c.add_node(constStr);
        c.add_wire({cs_id, 0, true}, {c.output_bar_id, 0, false});
        m.cases.push_back(std::move(c));
    }

    // Case 2: wildcard
    {
        Case c = make_basic_case(1, 1);
        CaseGuard guard;
        guard.pin = 0;
        guard.kind = CaseGuard::Wildcard;
        c.guards.push_back(guard);

        Node constStr;
        constStr.type = NodeType::Constant;
        constStr.name = "result";
        constStr.num_inputs = 0;
        constStr.num_outputs = 1;
        constStr.constant_value = Value::string("other");
        NodeId cs_id = c.add_node(constStr);
        c.add_wire({cs_id, 0, true}, {c.output_bar_id, 0, false});
        m.cases.push_back(std::move(c));
    }

    sec.methods.push_back(std::move(m));
    proj.sections.push_back(std::move(sec));

    Evaluator eval;

    auto r1 = eval.call_method(proj, "classify", {Value::integer(42)});
    assert(r1.status == EvalStatus::Success);
    assert(r1.outputs[0].as_string()->str() == "integer");

    auto r2 = eval.call_method(proj, "classify", {Value::string("hi")});
    assert(r2.status == EvalStatus::Success);
    assert(r2.outputs[0].as_string()->str() == "other");

    auto r3 = eval.call_method(proj, "classify", {Value::boolean(false)});
    assert(r3.status == EvalStatus::Success);
    assert(r3.outputs[0].as_string()->str() == "other");

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// Phase 26: Observable Attributes
// ---------------------------------------------------------------------------

static void test_observe_bind() {
    printf("  test_observe_bind... ");

    // Create two objects. Bind src.x -> tgt.y. Set src.x, verify tgt.y updates.
    auto src = make_ref<PhoObject>("Src");
    auto tgt = make_ref<PhoObject>("Tgt");
    src->set_attr("x", Value::integer(0));
    tgt->set_attr("y", Value::integer(0));

    // Bind: when src.x changes, tgt.y is set to new value
    PhoObject* tgt_ptr = tgt.get();
    ObserverId bind_id = src->add_observer("x",
        [tgt_ptr](const std::string&, const Value&, const Value& new_val) {
            tgt_ptr->set_attr("y", new_val);
        });

    // Set src.x = 42, tgt.y should become 42
    src->set_attr("x", Value::integer(42));
    assert(tgt->get_attr("y").as_integer() == 42);

    // Set src.x = 99, tgt.y should become 99
    src->set_attr("x", Value::integer(99));
    assert(tgt->get_attr("y").as_integer() == 99);

    // Unbind, then set — tgt.y should NOT change
    src->remove_observer(bind_id);
    src->set_attr("x", Value::integer(200));
    assert(tgt->get_attr("y").as_integer() == 99);  // unchanged

    printf("OK\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    register_all_prims();

    printf("Phase 11-28 tests:\n");

    // Phase 11: Execution wires
    test_execution_wire_ordering();
    test_execution_wire_blocks_without_trigger();

    // Phase 12: Optional inputs
    test_optional_input_default();
    test_optional_input_with_wire_overrides_default();

    // Phase 13: JSON, Dates, String prims
    test_json_parse();
    test_json_encode();
    test_date_now();
    test_date_create_components_roundtrip();
    test_string_repeat();
    test_string_starts_with();
    test_string_ends_with();
    test_to_from_codepoints();

    // Phase 14: Method refs, filter/reduce, enums
    test_method_ref_call();
    test_filter_with_method_ref();
    test_reduce_with_method_ref();
    test_enum_create_variant_data();

    // Phase 15: Loops (control annotations + eval_loop)
    test_loop_count_to_10();
    test_loop_is_loop_annotation();

    // Phase 16: List annotations
    test_list_map_annotation();
    test_partition_annotation();

    // Phase 17: Try / Error
    test_try_catches_error();
    test_try_no_error();

    // Phase 20: Evaluation nodes
    test_evaluation_node();
    test_evaluation_node_boolean_logic();
    test_evaluation_node_ternary();

    // Phase 21: Inject
    test_inject_dispatches_to_method();
    test_inject_with_method_ref();

    // Phase 22: Persistent
    test_persistent_write_read();

    // Phase 23: Pattern matching
    test_pattern_matching_type_guard();
    test_pattern_matching_value_guard();
    test_pattern_matching_wildcard_fallback();

    // Phase 26: Observables
    test_observe_bind();

    printf("All phase 11-28 tests passed!\n");
    return 0;
}
