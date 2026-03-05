#include "pho_debug.h"
#include "pho_value.h"
#include <cassert>
#include <cstdio>

using namespace pho;

static void test_breakpoints() {
    printf("  test_breakpoints... ");
    Debugger dbg;

    dbg.add_breakpoint(10, "main", 0);
    dbg.add_breakpoint(20, "main", 0);

    assert(dbg.has_breakpoint(10));
    assert(dbg.has_breakpoint(20));
    assert(!dbg.has_breakpoint(30));
    assert(dbg.breakpoints().size() == 2);

    dbg.toggle_breakpoint(10);
    assert(!dbg.has_breakpoint(10));

    dbg.toggle_breakpoint(10);
    assert(dbg.has_breakpoint(10));

    dbg.remove_breakpoint(10);
    assert(!dbg.has_breakpoint(10));
    assert(dbg.breakpoints().size() == 1);

    dbg.clear_breakpoints();
    assert(dbg.breakpoints().empty());
    printf("OK\n");
}

static void test_trace_recording() {
    printf("  test_trace_recording... ");
    Debugger dbg;
    dbg.enable_tracing(true);

    TraceEntry t1;
    t1.source_node = 1;
    t1.source_pin = 0;
    t1.dest_node = 2;
    t1.dest_pin = 0;
    t1.value = Value::integer(42);
    t1.step_number = 1;
    dbg.record_trace(t1);

    TraceEntry t2;
    t2.source_node = 1;
    t2.source_pin = 1;
    t2.dest_node = 3;
    t2.dest_pin = 0;
    t2.value = Value::string("hello");
    t2.step_number = 2;
    dbg.record_trace(t2);

    assert(dbg.traces().size() == 2);

    // Query traces for node 1
    auto node1_traces = dbg.traces_for_node(1);
    assert(node1_traces.size() == 2);

    // Query specific wire value
    Value wv = dbg.wire_value(1, 0, 2, 0);
    assert(wv.is_integer() && wv.as_integer() == 42);

    Value wv2 = dbg.wire_value(1, 1, 3, 0);
    assert(wv2.is_string() && wv2.as_string()->str() == "hello");

    // Non-existent wire
    Value wv3 = dbg.wire_value(99, 0, 100, 0);
    assert(wv3.is_null());

    dbg.clear_traces();
    assert(dbg.traces().empty());
    printf("OK\n");
}

static void test_trace_callback() {
    printf("  test_trace_callback... ");
    Debugger dbg;
    int callback_count = 0;
    Value last_value;

    dbg.set_trace_record_callback([&](const TraceEntry& entry) {
        callback_count++;
        last_value = entry.value;
    });

    TraceEntry t;
    t.source_node = 1; t.source_pin = 0;
    t.dest_node = 2; t.dest_pin = 0;
    t.value = Value::integer(7);
    t.step_number = 1;
    dbg.record_trace(t);

    assert(callback_count == 1);
    assert(last_value.as_integer() == 7);
    printf("OK\n");
}

static void test_snapshots_rollback() {
    printf("  test_snapshots_rollback... ");
    Debugger dbg;

    // Record some traces and save snapshot
    TraceEntry t1;
    t1.source_node = 1; t1.source_pin = 0;
    t1.dest_node = 2; t1.dest_pin = 0;
    t1.value = Value::integer(10);
    t1.step_number = 1;
    dbg.record_trace(t1);

    dbg.save_snapshot("main", 0, {Value::integer(10)}, false);
    assert(dbg.snapshots().size() == 1);

    // More traces
    TraceEntry t2;
    t2.source_node = 2; t2.source_pin = 0;
    t2.dest_node = 3; t2.dest_pin = 0;
    t2.value = Value::integer(20);
    t2.step_number = 2;
    dbg.record_trace(t2);

    dbg.save_snapshot("main", 0, {Value::integer(20)}, true);
    assert(dbg.snapshots().size() == 2);
    assert(dbg.traces().size() == 2);

    // Rollback to previous snapshot
    bool ok = dbg.rollback();
    assert(ok);
    assert(dbg.snapshots().size() == 1);
    assert(dbg.traces().size() == 1); // Only first trace remains
    assert(dbg.traces()[0].value.as_integer() == 10);

    // Can't rollback further (only 1 snapshot)
    ok = dbg.rollback();
    assert(!ok);
    printf("OK\n");
}

static void test_step_control() {
    printf("  test_step_control... ");
    Debugger dbg;

    // Add breakpoint and simulate stepping
    dbg.add_breakpoint(5, "main", 0);

    int hit_count = 0;
    NodeId hit_node = 0;
    dbg.set_breakpoint_hit_callback([&](NodeId node, uint32_t step) {
        hit_count++;
        hit_node = node;
        // Auto-resume so step_completed doesn't block in wait_for_resume
        dbg.signal_resume();
    });

    // Simulate node executions
    dbg.step_completed(1);
    assert(hit_count == 0); // No breakpoint on node 1

    dbg.step_completed(5);
    assert(hit_count == 1); // Hit breakpoint on node 5
    assert(hit_node == 5);

    // Step over: first call arms step mode, second call pauses
    dbg.set_action(DebugAction::StepOver);
    dbg.step_completed(6); // arms step_mode_
    // Note: node 6 doesn't have breakpoint, but step_mode is armed
    // The next step_completed will pause
    dbg.step_completed(7); // should pause here (step_mode was armed), then auto-resume
    assert(hit_count == 2);
    assert(hit_node == 7);

    // Continue: should not pause until next breakpoint
    dbg.set_action(DebugAction::Continue);
    dbg.step_completed(8);
    assert(hit_count == 2); // no breakpoint on node 8

    printf("OK\n");
}

static void test_reset() {
    printf("  test_reset... ");
    Debugger dbg;

    dbg.add_breakpoint(1, "main", 0);
    TraceEntry t;
    t.source_node = 1; t.source_pin = 0;
    t.dest_node = 2; t.dest_pin = 0;
    t.value = Value::integer(42);
    t.step_number = 1;
    dbg.record_trace(t);
    dbg.save_snapshot("main", 0, {}, true);
    dbg.set_paused(true);

    dbg.reset();
    assert(dbg.traces().empty());
    assert(dbg.snapshots().empty());
    assert(!dbg.is_paused());
    assert(dbg.current_step() == 0);
    // Breakpoints should persist through reset
    assert(dbg.has_breakpoint(1));
    printf("OK\n");
}

int main() {
    printf("=== Phase 8: Debugger + Trace Tests ===\n");
    test_breakpoints();
    test_trace_recording();
    test_trace_callback();
    test_snapshots_rollback();
    test_step_control();
    test_reset();
    printf("All Phase 8 tests passed!\n");
    return 0;
}
