#include "pho_prim.h"
#include "pho_value.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace pho;

static void test_future_basic() {
    printf("  test_future_basic... ");
    register_all_prims();
    auto& reg = PrimitiveRegistry::instance();

    // Create resolved future
    auto res = reg.find("future-value")->fn({Value::integer(42)});
    assert(!res.failed);
    Value f = res.outputs[0];

    // Check resolved
    res = reg.find("future-resolved?")->fn({f});
    assert(res.outputs[0].as_boolean() == true);

    // Get value
    res = reg.find("future-get")->fn({f});
    assert(!res.failed);
    assert(res.outputs[0].as_integer() == 42);
    printf("OK\n");
}

static void test_future_create_resolve() {
    printf("  test_future_create_resolve... ");
    auto& reg = PrimitiveRegistry::instance();

    // Create unresolved future
    auto res = reg.find("future-create")->fn({});
    Value f = res.outputs[0];

    // Not yet resolved
    res = reg.find("future-resolved?")->fn({f});
    assert(res.outputs[0].as_boolean() == false);

    // Get should fail
    res = reg.find("future-get")->fn({f});
    assert(res.failed);

    // Resolve it
    reg.find("future-resolve")->fn({f, Value::string("hello")});

    // Now resolved
    res = reg.find("future-resolved?")->fn({f});
    assert(res.outputs[0].as_boolean() == true);

    res = reg.find("future-get")->fn({f});
    assert(!res.failed);
    assert(res.outputs[0].as_string()->str() == "hello");
    printf("OK\n");
}

static void test_future_then() {
    printf("  test_future_then... ");
    auto& reg = PrimitiveRegistry::instance();

    // Create and chain
    auto res = reg.find("future-create")->fn({});
    Value f1 = res.outputs[0];

    res = reg.find("future-then")->fn({f1, Value::string("identity")});
    Value f2 = res.outputs[0];

    // f2 not resolved yet
    res = reg.find("future-resolved?")->fn({f2});
    assert(res.outputs[0].as_boolean() == false);

    // Resolve f1 -> should resolve f2
    reg.find("future-resolve")->fn({f1, Value::integer(99)});

    res = reg.find("future-resolved?")->fn({f2});
    assert(res.outputs[0].as_boolean() == true);

    res = reg.find("future-get")->fn({f2});
    assert(res.outputs[0].as_integer() == 99);
    printf("OK\n");
}

static void test_future_all() {
    printf("  test_future_all... ");
    auto& reg = PrimitiveRegistry::instance();

    auto r1 = reg.find("future-value")->fn({Value::integer(1)});
    auto r2 = reg.find("future-value")->fn({Value::integer(2)});
    auto r3 = reg.find("future-value")->fn({Value::integer(3)});

    Value futures = Value::list({r1.outputs[0], r2.outputs[0], r3.outputs[0]});
    auto res = reg.find("future-all")->fn({futures});
    Value combined = res.outputs[0];

    res = reg.find("future-resolved?")->fn({combined});
    assert(res.outputs[0].as_boolean() == true);

    res = reg.find("future-get")->fn({combined});
    assert(res.outputs[0].is_list());
    assert(res.outputs[0].as_list()->size() == 3);
    assert(res.outputs[0].as_list()->at(0).as_integer() == 1);
    assert(res.outputs[0].as_list()->at(2).as_integer() == 3);
    printf("OK\n");
}

static void test_channel_basic() {
    printf("  test_channel_basic... ");
    auto& reg = PrimitiveRegistry::instance();

    auto res = reg.find("channel-create")->fn({});
    Value ch = res.outputs[0];

    // Empty initially
    res = reg.find("channel-empty?")->fn({ch});
    assert(res.outputs[0].as_boolean() == true);

    // Send values
    reg.find("channel-send")->fn({ch, Value::integer(10)});
    reg.find("channel-send")->fn({ch, Value::integer(20)});
    reg.find("channel-send")->fn({ch, Value::string("hello")});

    res = reg.find("channel-size")->fn({ch});
    assert(res.outputs[0].as_integer() == 3);

    // Receive in order
    res = reg.find("channel-receive")->fn({ch});
    assert(!res.failed);
    assert(res.outputs[0].as_integer() == 10);

    res = reg.find("channel-receive")->fn({ch});
    assert(res.outputs[0].as_integer() == 20);

    res = reg.find("channel-receive")->fn({ch});
    assert(res.outputs[0].as_string()->str() == "hello");

    // Empty now
    res = reg.find("channel-receive")->fn({ch});
    assert(res.failed); // empty channel fails
    printf("OK\n");
}

static void test_channel_bounded() {
    printf("  test_channel_bounded... ");
    auto& reg = PrimitiveRegistry::instance();

    auto res = reg.find("channel-create-bounded")->fn({Value::integer(2)});
    Value ch = res.outputs[0];

    // Send up to capacity
    res = reg.find("channel-send")->fn({ch, Value::integer(1)});
    assert(res.outputs[0].as_boolean() == true);
    res = reg.find("channel-send")->fn({ch, Value::integer(2)});
    assert(res.outputs[0].as_boolean() == true);

    // Third send should fail (at capacity)
    res = reg.find("channel-send")->fn({ch, Value::integer(3)});
    assert(res.outputs[0].as_boolean() == false);

    // Receive one, then send should work
    reg.find("channel-receive")->fn({ch});
    res = reg.find("channel-send")->fn({ch, Value::integer(3)});
    assert(res.outputs[0].as_boolean() == true);
    printf("OK\n");
}

static void test_channel_close() {
    printf("  test_channel_close... ");
    auto& reg = PrimitiveRegistry::instance();

    auto res = reg.find("channel-create")->fn({});
    Value ch = res.outputs[0];

    reg.find("channel-send")->fn({ch, Value::integer(42)});
    reg.find("channel-close")->fn({ch});

    res = reg.find("channel-closed?")->fn({ch});
    assert(res.outputs[0].as_boolean() == true);

    // Send on closed channel fails
    res = reg.find("channel-send")->fn({ch, Value::integer(99)});
    assert(res.outputs[0].as_boolean() == false);

    // But can still receive buffered values
    res = reg.find("channel-receive")->fn({ch});
    assert(!res.failed);
    assert(res.outputs[0].as_integer() == 42);
    printf("OK\n");
}

static void test_effect_basic() {
    printf("  test_effect_basic... ");
    auto& reg = PrimitiveRegistry::instance();

    // Create log effect
    auto res = reg.find("cmd-log")->fn({Value::string("test message")});
    assert(!res.failed);
    Value eff = res.outputs[0];

    res = reg.find("effect?")->fn({eff});
    assert(res.outputs[0].as_boolean() == true);

    // Non-effect
    res = reg.find("effect?")->fn({Value::integer(42)});
    assert(res.outputs[0].as_boolean() == false);

    // Perform log effect
    res = reg.find("perform")->fn({eff});
    assert(!res.failed);
    printf("OK\n");
}

static void test_effect_batch() {
    printf("  test_effect_batch... ");
    auto& reg = PrimitiveRegistry::instance();

    auto e1 = reg.find("cmd-log")->fn({Value::string("msg1")}).outputs[0];
    auto e2 = reg.find("cmd-log")->fn({Value::string("msg2")}).outputs[0];
    auto e3 = reg.find("cmd-none")->fn({}).outputs[0];

    auto batch = reg.find("cmd-batch")->fn({Value::list({e1, e2, e3})});
    assert(!batch.failed);

    auto res = reg.find("perform")->fn({batch.outputs[0]});
    assert(!res.failed);
    assert(res.outputs[0].is_list());
    printf("OK\n");
}

int main() {
    printf("=== Phase 9: Async + Concurrency Tests ===\n");
    test_future_basic();
    test_future_create_resolve();
    test_future_then();
    test_future_all();
    test_channel_basic();
    test_channel_bounded();
    test_channel_close();
    test_effect_basic();
    test_effect_batch();
    printf("All Phase 9 tests passed!\n");
    return 0;
}
