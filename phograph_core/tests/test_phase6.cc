#include "pho_thread.h"
#include "pho_prim.h"
#include "pho_value.h"
#include "pho_scene.h"
#include "pho_draw.h"
#include "pho_prim_wrappers.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace pho;

static void test_event_queue() {
    printf("  test_event_queue... ");
    EventQueue q;
    assert(q.empty());
    assert(q.size() == 0);

    InputEvent e1;
    e1.type = InputEventType::PointerDown;
    e1.x = 100; e1.y = 200;
    q.push(e1);

    InputEvent e2;
    e2.type = InputEventType::KeyDown;
    e2.key = "a";
    q.push(e2);

    assert(q.size() == 2);
    assert(!q.empty());

    InputEvent out;
    assert(q.pop(out));
    assert(out.type == InputEventType::PointerDown);
    assert(out.x == 100);

    assert(q.pop(out));
    assert(out.type == InputEventType::KeyDown);
    assert(out.key == "a");

    assert(q.empty());
    assert(!q.pop(out));
    printf("OK\n");
}

static void test_run_loop_events() {
    printf("  test_run_loop_events... ");
    EngineRunLoop loop;
    int event_count = 0;
    InputEventType last_type = InputEventType::Custom;

    loop.set_event_handler([&](const InputEvent& e) {
        event_count++;
        last_type = e.type;
    });

    InputEvent e;
    e.type = InputEventType::PointerDown;
    e.x = 50; e.y = 60;
    loop.post_event(e);

    e.type = InputEventType::PointerUp;
    loop.post_event(e);

    uint32_t processed = loop.tick(1.0);
    assert(processed == 2);
    assert(event_count == 2);
    assert(last_type == InputEventType::PointerUp);
    printf("OK\n");
}

static void test_run_loop_timers() {
    printf("  test_run_loop_timers... ");
    EngineRunLoop loop;
    int fire_count = 0;

    // Schedule a one-shot timer at t=0 with 0.5s delay
    loop.tick(0.0);
    loop.schedule_timer(0.5, 0, false, [&]() { fire_count++; });

    // At t=0.3, timer shouldn't fire
    loop.tick(0.3);
    assert(fire_count == 0);

    // At t=0.6, timer should fire
    loop.tick(0.6);
    assert(fire_count == 1);

    // At t=1.0, shouldn't fire again (one-shot)
    loop.tick(1.0);
    assert(fire_count == 1);

    // Repeating timer
    fire_count = 0;
    loop.tick(2.0);
    uint32_t timer_id = loop.schedule_timer(0.1, 0.1, true, [&]() { fire_count++; });

    loop.tick(2.15);
    assert(fire_count == 1);
    loop.tick(2.25);
    assert(fire_count == 2);

    // Cancel it
    loop.cancel_timer(timer_id);
    loop.tick(2.35);
    assert(fire_count == 2); // Should not fire again

    printf("OK\n");
}

static void test_input_event_to_value() {
    printf("  test_input_event_to_value... ");
    InputEvent e;
    e.type = InputEventType::PointerDown;
    e.x = 100; e.y = 200;
    e.button = 0;
    e.modifiers = InputEvent::Shift | InputEvent::Meta;
    e.timestamp = 12345.678;

    Value v = input_event_to_value(e);
    assert(v.is_object());
    auto* obj = v.as_object();
    assert(obj->get_attr("type").as_string()->str() == "pointer-down");
    assert(std::abs(obj->get_attr("x").as_number() - 100.0) < 0.01);
    assert(std::abs(obj->get_attr("y").as_number() - 200.0) < 0.01);
    assert(obj->get_attr("button").as_integer() == 0);
    printf("OK\n");
}

static void test_easing_prims() {
    printf("  test_easing_prims... ");
    register_all_prims();
    auto& reg = PrimitiveRegistry::instance();

    // ease with linear
    auto* ease = reg.find("ease");
    assert(ease);
    auto res = ease->fn({Value::real(0.5), Value::string("linear")});
    assert(!res.failed);
    assert(std::abs(res.outputs[0].as_number() - 0.5) < 0.01);

    // ease-in-quad at t=0.5 should be 0.25
    res = ease->fn({Value::real(0.5), Value::string("ease-in-quad")});
    assert(std::abs(res.outputs[0].as_number() - 0.25) < 0.01);

    // ease at t=0 and t=1 should be 0 and 1 for any easing
    res = ease->fn({Value::real(0.0), Value::string("ease-out-cubic")});
    assert(std::abs(res.outputs[0].as_number()) < 0.01);
    res = ease->fn({Value::real(1.0), Value::string("ease-out-cubic")});
    assert(std::abs(res.outputs[0].as_number() - 1.0) < 0.01);
    printf("OK\n");
}

static void test_lerp_prims() {
    printf("  test_lerp_prims... ");
    auto& reg = PrimitiveRegistry::instance();

    // lerp
    auto* lerp = reg.find("lerp");
    assert(lerp);
    auto res = lerp->fn({Value::real(0), Value::real(100), Value::real(0.5)});
    assert(std::abs(res.outputs[0].as_number() - 50.0) < 0.01);

    res = lerp->fn({Value::real(10), Value::real(20), Value::real(0.0)});
    assert(std::abs(res.outputs[0].as_number() - 10.0) < 0.01);

    // color-lerp
    auto* clerp = reg.find("color-lerp");
    assert(clerp);
    Value red = Value::list({Value::real(1), Value::real(0), Value::real(0), Value::real(1)});
    Value blue = Value::list({Value::real(0), Value::real(0), Value::real(1), Value::real(1)});
    res = clerp->fn({red, blue, Value::real(0.5)});
    assert(!res.failed && res.outputs[0].is_list());
    auto* c = res.outputs[0].as_list();
    assert(std::abs(c->at(0).as_number() - 0.5) < 0.01); // r
    assert(std::abs(c->at(2).as_number() - 0.5) < 0.01); // b
    printf("OK\n");
}

static void test_animate_progress() {
    printf("  test_animate_progress... ");
    auto& reg = PrimitiveRegistry::instance();

    auto* ap = reg.find("animate-progress");
    assert(ap);
    // Start=0, duration=2, now=1 -> t=0.5
    auto res = ap->fn({Value::real(0), Value::real(2), Value::real(1)});
    assert(std::abs(res.outputs[0].as_number() - 0.5) < 0.01);

    // now=3 (past end) -> t=1.0
    res = ap->fn({Value::real(0), Value::real(2), Value::real(3)});
    assert(std::abs(res.outputs[0].as_number() - 1.0) < 0.01);

    // now=-1 (before start) -> t=0.0
    res = ap->fn({Value::real(0), Value::real(2), Value::real(-1)});
    assert(std::abs(res.outputs[0].as_number()) < 0.01);
    printf("OK\n");
}

static void test_io_prims() {
    printf("  test_io_prims... ");
    auto& reg = PrimitiveRegistry::instance();

    // time-now should return something > 0
    auto* tn = reg.find("time-now");
    assert(tn);
    auto res = tn->fn({});
    assert(!res.failed && res.outputs[0].as_number() > 1000000000.0); // after 2001

    // platform-name
    auto* pn = reg.find("platform-name");
    assert(pn);
    res = pn->fn({});
    assert(!res.failed && res.outputs[0].is_string());

    // file-exists? on a non-existent file
    auto* fe = reg.find("file-exists?");
    assert(fe);
    res = fe->fn({Value::string("/tmp/phograph_nonexistent_test_file_12345")});
    assert(!res.failed && res.outputs[0].as_boolean() == false);
    printf("OK\n");
}

static void test_button_color_change() {
    printf("  test_button_color_change... ");
    // Simulates a button that changes color on tap:
    // 1. Create a canvas with a colored rect
    // 2. Simulate pointer-down event
    // 3. Change the rect color in response
    // 4. Re-render and verify pixel changed

    auto canvas = make_ref<Canvas>(200, 200);
    auto root = make_ref<Shape>();
    root->type = ShapeType::Group;

    auto button = make_ref<Shape>();
    button->type = ShapeType::Rect;
    button->bounds = Rect(50, 50, 100, 100);
    button->style.fill_color = Color(1, 0, 0, 1); // red initially
    button->tag = "button";

    root->add_child(button);
    canvas->root = root;

    // Render initial state
    DrawContext::render_canvas(*canvas);
    const uint8_t* buf = canvas->buffer();
    int idx = (75 * 200 + 75) * 4;
    assert(buf[idx + 2] == 255); // red channel
    assert(buf[idx + 0] == 0);   // blue channel

    // Simulate tap: check hit test, then change color
    InputEvent tap;
    tap.type = InputEventType::PointerDown;
    tap.x = 75; tap.y = 75;

    auto* btn = root->find_by_tag("button");
    assert(btn && btn->hit_test(tap.x, tap.y));

    // Change color to blue (simulating event handler response)
    btn->style.fill_color = Color(0, 0, 1, 1); // blue
    btn->invalidate();

    // Re-render
    DrawContext::render_canvas(*canvas);
    buf = canvas->buffer();
    idx = (75 * 200 + 75) * 4;
    assert(buf[idx + 0] == 255); // blue channel now
    assert(buf[idx + 2] == 0);   // red channel now 0

    printf("OK\n");
}

int main() {
    printf("=== Phase 6: Input + Animation + I/O Tests ===\n");
    test_event_queue();
    test_run_loop_events();
    test_run_loop_timers();
    test_input_event_to_value();
    test_easing_prims();
    test_lerp_prims();
    test_animate_progress();
    test_io_prims();
    test_button_color_change();
    printf("All Phase 6 tests passed!\n");
    return 0;
}
