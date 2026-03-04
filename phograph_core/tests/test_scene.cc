#include "pho_scene.h"
#include "pho_draw.h"
#include "pho_prim.h"
#include "pho_value.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>

using namespace pho;

// Helper to check BGRA pixel
struct BGRA {
    uint8_t b, g, r, a;
};

static BGRA get_pixel(const uint8_t* buf, int32_t w, int32_t x, int32_t y) {
    int idx = (y * w + x) * 4;
    return {buf[idx], buf[idx+1], buf[idx+2], buf[idx+3]};
}

static void test_color_conversion() {
    printf("  test_color_conversion... ");
    Color red(1, 0, 0, 1);
    uint32_t bgra = red.to_bgra8();
    // BGRA: B=0, G=0, R=255, A=255
    assert((bgra & 0xFF) == 0);         // B
    assert(((bgra >> 8) & 0xFF) == 0);   // G
    assert(((bgra >> 16) & 0xFF) == 255); // R
    assert(((bgra >> 24) & 0xFF) == 255); // A

    Color blue(0, 0, 1, 1);
    bgra = blue.to_bgra8();
    assert((bgra & 0xFF) == 255);        // B
    assert(((bgra >> 16) & 0xFF) == 0);  // R

    Color blend = Color::blend(red, blue, 0.5f);
    assert(std::abs(blend.r - 0.5f) < 0.01f);
    assert(std::abs(blend.b - 0.5f) < 0.01f);
    printf("OK\n");
}

static void test_transform() {
    printf("  test_transform... ");
    auto t = Transform2D::translate(10, 20);
    Point p = t.apply({5, 5});
    assert(std::abs(p.x - 15.0f) < 0.01f);
    assert(std::abs(p.y - 25.0f) < 0.01f);

    auto s = Transform2D::scale(2, 3);
    p = s.apply({4, 5});
    assert(std::abs(p.x - 8.0f) < 0.01f);
    assert(std::abs(p.y - 15.0f) < 0.01f);

    // Concat: s.concat(t) = s * t, meaning apply t first, then s
    // For point (1,1): t.apply = (11,21), then s.apply = (22,63)
    // But concat means: s.concat(t).apply(p) = s.apply(t.apply(p))
    // Actually concat(o) = this * o matrix multiply
    // s.concat(t).apply({0,0}): m[4]=2*10+0*20+0=20, m[5]=0*10+3*20+0=60
    auto ts = s.concat(t);
    p = ts.apply({0, 0});
    assert(std::abs(p.x - 20.0f) < 0.01f);
    assert(std::abs(p.y - 60.0f) < 0.01f);
    printf("OK\n");
}

static void test_shape_tree() {
    printf("  test_shape_tree... ");
    auto root = make_ref<Shape>();
    root->type = ShapeType::Group;
    root->tag = "root";

    auto child1 = make_ref<Shape>();
    child1->type = ShapeType::Rect;
    child1->bounds = Rect(10, 10, 50, 50);
    child1->tag = "rect1";

    auto child2 = make_ref<Shape>();
    child2->type = ShapeType::Oval;
    child2->bounds = Rect(100, 100, 60, 60);
    child2->tag = "oval1";

    root->add_child(child1);
    root->add_child(child2);
    assert(root->children.size() == 2);

    auto* found = root->find_by_tag("oval1");
    assert(found != nullptr);
    assert(found->type == ShapeType::Oval);

    auto* not_found = root->find_by_tag("nope");
    assert(not_found == nullptr);
    printf("OK\n");
}

static void test_hit_test() {
    printf("  test_hit_test... ");
    Shape rect;
    rect.type = ShapeType::Rect;
    rect.bounds = Rect(10, 10, 100, 100);
    assert(rect.hit_test(50, 50));
    assert(!rect.hit_test(5, 5));
    assert(!rect.hit_test(111, 50));

    Shape oval;
    oval.type = ShapeType::Oval;
    oval.bounds = Rect(0, 0, 100, 100);
    // Center at 50,50 radius 50
    assert(oval.hit_test(50, 50));     // center
    assert(oval.hit_test(50, 1));      // near top
    assert(!oval.hit_test(1, 1));      // corner - outside oval
    printf("OK\n");
}

static void test_canvas_clear() {
    printf("  test_canvas_clear... ");
    auto canvas = make_ref<Canvas>(100, 100);
    canvas->clear(Color(1, 0, 0, 1));

    auto px = get_pixel(canvas->buffer(), 100, 50, 50);
    assert(px.r == 255);
    assert(px.g == 0);
    assert(px.b == 0);
    assert(px.a == 255);
    printf("OK\n");
}

static void test_draw_fill_rect() {
    printf("  test_draw_fill_rect... ");
    auto canvas = make_ref<Canvas>(200, 200);
    canvas->clear(colors::black);

    DrawContext ctx(canvas->buffer(), canvas->width(), canvas->height());
    // Draw red rectangle at (10,10) size 50x50
    ctx.fill_rect(10, 10, 50, 50, Color(1, 0, 0, 1));

    // Check pixel inside rect
    auto px = get_pixel(canvas->buffer(), 200, 30, 30);
    assert(px.r == 255 && px.g == 0 && px.b == 0 && px.a == 255);

    // Check pixel outside rect (should be black)
    px = get_pixel(canvas->buffer(), 200, 5, 5);
    assert(px.r == 0 && px.g == 0 && px.b == 0);
    printf("OK\n");
}

static void test_draw_fill_oval() {
    printf("  test_draw_fill_oval... ");
    auto canvas = make_ref<Canvas>(200, 200);
    canvas->clear(colors::black);

    DrawContext ctx(canvas->buffer(), canvas->width(), canvas->height());
    // Draw blue circle centered at (100,100) radius 40
    ctx.fill_oval(100, 100, 40, 40, Color(0, 0, 1, 1));

    // Check center pixel
    auto px = get_pixel(canvas->buffer(), 200, 100, 100);
    assert(px.r == 0 && px.g == 0 && px.b == 255 && px.a == 255);

    // Check pixel outside circle
    px = get_pixel(canvas->buffer(), 200, 5, 5);
    assert(px.r == 0 && px.g == 0 && px.b == 0);
    printf("OK\n");
}

static void test_scene_graph_render() {
    printf("  test_scene_graph_render... ");
    // Create canvas with root group containing a red rect and blue circle
    auto canvas = make_ref<Canvas>(200, 200);

    auto root = make_ref<Shape>();
    root->type = ShapeType::Group;

    auto rect = make_ref<Shape>();
    rect->type = ShapeType::Rect;
    rect->bounds = Rect(10, 10, 80, 80);
    rect->style.fill_color = Color(1, 0, 0, 1); // red

    auto oval = make_ref<Shape>();
    oval->type = ShapeType::Oval;
    oval->bounds = Rect(100, 100, 60, 60); // center at 130,130 radius 30
    oval->style.fill_color = Color(0, 0, 1, 1); // blue

    root->add_child(rect);
    root->add_child(oval);
    canvas->root = root;

    DrawContext::render_canvas(*canvas);

    // Check red rect interior
    auto px = get_pixel(canvas->buffer(), 200, 30, 30);
    assert(px.r == 255 && px.g == 0 && px.b == 0 && px.a == 255);

    // Check blue oval center
    px = get_pixel(canvas->buffer(), 200, 130, 130);
    assert(px.r == 0 && px.g == 0 && px.b == 255 && px.a == 255);

    // Check undrawn area (should be clear/transparent)
    px = get_pixel(canvas->buffer(), 200, 5, 5);
    assert(px.a == 0);
    printf("OK\n");
}

static void test_alpha_blending() {
    printf("  test_alpha_blending... ");
    auto canvas = make_ref<Canvas>(100, 100);
    canvas->clear(Color(1, 0, 0, 1)); // red background

    DrawContext ctx(canvas->buffer(), 100, 100);
    // Draw semi-transparent blue rect over it
    ctx.fill_rect(0, 0, 100, 100, Color(0, 0, 1, 0.5f));

    auto px = get_pixel(canvas->buffer(), 100, 50, 50);
    // Should be blended: ~50% red + 50% blue
    assert(px.r > 100 && px.r < 200); // some red
    assert(px.b > 100 && px.b < 200); // some blue
    assert(px.a == 255);              // fully opaque result
    printf("OK\n");
}

static void test_rounded_rect() {
    printf("  test_rounded_rect... ");
    auto canvas = make_ref<Canvas>(200, 200);
    canvas->clear(colors::clear);

    DrawContext ctx(canvas->buffer(), 200, 200);
    ctx.fill_rounded_rect(10, 10, 100, 100, 20, Color(0, 1, 0, 1));

    // Center should be filled
    auto px = get_pixel(canvas->buffer(), 200, 60, 60);
    assert(px.g == 255 && px.r == 0 && px.b == 0);

    // Corner of bounding rect should NOT be filled (it's rounded)
    px = get_pixel(canvas->buffer(), 200, 11, 11);
    assert(px.g == 0); // outside the rounded corner
    printf("OK\n");
}

static void test_scene_prims() {
    printf("  test_scene_prims... ");
    register_all_prims();
    auto& reg = PrimitiveRegistry::instance();

    // Create a rect shape
    auto* sr = reg.find("shape-rect");
    assert(sr);
    auto res = sr->fn({Value::real(10), Value::real(20), Value::real(100), Value::real(50)});
    assert(!res.failed && res.outputs.size() == 1);
    Value shape_val = res.outputs[0];
    assert(shape_val.is_object());

    // Set fill color
    auto* sf = reg.find("shape-set-fill");
    assert(sf);
    res = sf->fn({shape_val, Value::list({Value::real(1), Value::real(0), Value::real(0), Value::real(1)})});
    assert(!res.failed);

    // Get bounds
    auto* gb = reg.find("shape-get-bounds");
    assert(gb);
    res = gb->fn({shape_val});
    assert(!res.failed && res.outputs[0].is_list());
    auto* bounds = res.outputs[0].as_list();
    assert(bounds->size() == 4);
    assert(std::abs(bounds->at(0).as_number() - 10.0) < 0.01);

    // Hit test
    auto* ht = reg.find("shape-hit-test");
    assert(ht);
    res = ht->fn({shape_val, Value::real(50), Value::real(40)});
    assert(!res.failed && res.outputs[0].as_boolean() == true);
    res = ht->fn({shape_val, Value::real(5), Value::real(5)});
    assert(!res.failed && res.outputs[0].as_boolean() == false);
    printf("OK\n");
}

static void test_canvas_prims() {
    printf("  test_canvas_prims... ");
    auto& reg = PrimitiveRegistry::instance();

    // Create canvas
    auto* cc = reg.find("create-canvas");
    assert(cc);
    auto res = cc->fn({Value::integer(100), Value::integer(100)});
    assert(!res.failed && res.outputs.size() == 1);
    Value canvas_val = res.outputs[0];

    // Check dimensions
    auto* cw = reg.find("canvas-width");
    res = cw->fn({canvas_val});
    assert(res.outputs[0].as_integer() == 100);

    // Clear with red
    auto* cl = reg.find("canvas-clear");
    res = cl->fn({canvas_val, Value::list({Value::real(1), Value::real(0), Value::real(0), Value::real(1)})});
    assert(!res.failed);

    // Read pixel
    auto* pa = reg.find("canvas-pixel-at");
    res = pa->fn({canvas_val, Value::integer(50), Value::integer(50)});
    assert(!res.failed && res.outputs[0].is_list());
    auto* pixel = res.outputs[0].as_list();
    assert(pixel->size() == 4);
    assert(std::abs(pixel->at(0).as_number() - 1.0) < 0.01); // r=1
    assert(std::abs(pixel->at(1).as_number() - 0.0) < 0.01); // g=0
    assert(std::abs(pixel->at(2).as_number() - 0.0) < 0.01); // b=0
    printf("OK\n");
}

static void test_draw_prims() {
    printf("  test_draw_prims... ");
    auto& reg = PrimitiveRegistry::instance();

    // Create canvas and clear
    auto res = reg.find("create-canvas")->fn({Value::integer(200), Value::integer(200)});
    Value canvas_val = res.outputs[0];
    reg.find("draw-clear")->fn({canvas_val, Value::list({Value::real(0), Value::real(0), Value::real(0), Value::real(1)})});

    // Draw red rect
    auto* dfr = reg.find("draw-fill-rect");
    res = dfr->fn({canvas_val, Value::real(10), Value::real(10), Value::real(50), Value::real(50),
                   Value::list({Value::real(1), Value::real(0), Value::real(0), Value::real(1)})});
    assert(!res.failed);

    // Check pixel
    res = reg.find("canvas-pixel-at")->fn({canvas_val, Value::integer(30), Value::integer(30)});
    auto* pixel = res.outputs[0].as_list();
    assert(std::abs(pixel->at(0).as_number() - 1.0) < 0.01); // r=1

    // Draw blue oval
    auto* dfo = reg.find("draw-fill-oval");
    res = dfo->fn({canvas_val, Value::real(150), Value::real(150), Value::real(30), Value::real(30),
                   Value::list({Value::real(0), Value::real(0), Value::real(1), Value::real(1)})});
    assert(!res.failed);

    // Check oval center
    res = reg.find("canvas-pixel-at")->fn({canvas_val, Value::integer(150), Value::integer(150)});
    pixel = res.outputs[0].as_list();
    assert(std::abs(pixel->at(2).as_number() - 1.0) < 0.01); // b=1
    printf("OK\n");
}

int main() {
    printf("=== Phase 5: Scene Graph + Drawing Tests ===\n");
    test_color_conversion();
    test_transform();
    test_shape_tree();
    test_hit_test();
    test_canvas_clear();
    test_draw_fill_rect();
    test_draw_fill_oval();
    test_scene_graph_render();
    test_alpha_blending();
    test_rounded_rect();
    test_scene_prims();
    test_canvas_prims();
    test_draw_prims();
    printf("All Phase 5 tests passed!\n");
    return 0;
}
