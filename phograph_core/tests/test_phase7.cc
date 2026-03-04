#include "pho_fuzzy.h"
#include "pho_ide_render.h"
#include "pho_scene.h"
#include "pho_draw.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace pho;

static void test_fuzzy_score() {
    printf("  test_fuzzy_score... ");
    // Exact match should score highly
    int s1 = fuzzy_score("concat", "concat");
    assert(s1 > 0);

    // Prefix match
    int s2 = fuzzy_score("con", "concat");
    assert(s2 > 0);

    // Subsequence match
    int s3 = fuzzy_score("cnt", "concat");
    assert(s3 > 0);

    // Non-match
    int s4 = fuzzy_score("xyz", "concat");
    assert(s4 == 0);

    // Exact match should score higher than prefix
    assert(s1 > s2);

    // Empty query matches everything
    assert(fuzzy_score("", "anything") > 0);
    printf("OK\n");
}

static void test_fuzzy_filter() {
    printf("  test_fuzzy_filter... ");
    std::vector<std::string> candidates = {
        "+", "-", "*", "/", "concat", "length", "get-nth",
        "dict-create", "dict-get", "dict-set", "sort",
        "shape-rect", "shape-oval", "canvas-render",
    };

    // Filter with "dict"
    auto matches = fuzzy_filter("dict", candidates);
    assert(!matches.empty());
    // All results should contain "dict"
    for (auto& m : matches) {
        assert(m.text.find("dict") != std::string::npos || fuzzy_score("dict", m.text) > 0);
    }
    // dict-create should be first (starts with "dict")
    assert(matches[0].text.find("dict") == 0);

    // Filter with empty query
    auto all = fuzzy_filter("", candidates, 100);
    assert(all.size() == candidates.size());

    // Filter with very specific query
    auto specific = fuzzy_filter("shape-rect", candidates);
    assert(!specific.empty());
    assert(specific[0].text == "shape-rect");

    // max_results limits
    auto limited = fuzzy_filter("", candidates, 3);
    assert(limited.size() == 3);
    printf("OK\n");
}

static void test_ide_node_layout() {
    printf("  test_ide_node_layout... ");
    ide::Theme theme;
    ide::NodeLayout layout;
    layout.id = 1;
    layout.x = 100;
    layout.y = 50;
    layout.width = theme.node_width;
    layout.height = theme.node_header_height + 3 * theme.pin_spacing;
    layout.label = "concat";
    layout.num_inputs = 2;
    layout.num_outputs = 1;

    // Input pin positions
    Point p0 = ide::input_pin_pos(layout, 0, theme);
    assert(std::abs(p0.x - 100.0f) < 0.01f); // left edge
    assert(p0.y > layout.y + theme.node_header_height);

    Point p1 = ide::input_pin_pos(layout, 1, theme);
    assert(p1.y > p0.y); // second pin below first

    // Output pin position
    Point op = ide::output_pin_pos(layout, 0, theme);
    assert(std::abs(op.x - (100.0f + theme.node_width)) < 0.01f); // right edge
    printf("OK\n");
}

static void test_ide_render_node() {
    printf("  test_ide_render_node... ");
    auto canvas = make_ref<Canvas>(400, 300);
    canvas->clear(Color(0.12f, 0.12f, 0.15f, 1.0f));

    DrawContext ctx(canvas->buffer(), canvas->width(), canvas->height());
    ide::Theme theme;

    ide::NodeLayout layout;
    layout.id = 1;
    layout.x = 50;
    layout.y = 30;
    layout.width = 160;
    layout.height = 80;
    layout.label = "add";
    layout.num_inputs = 2;
    layout.num_outputs = 1;
    layout.selected = true;

    ide::draw_node(ctx, layout, theme);

    // Check that the node body area has been drawn (not background color)
    const uint8_t* buf = canvas->buffer();
    int idx = (60 * 400 + 100) * 4; // inside the node
    // Should not be the background color (0.12*255 ~= 30)
    uint8_t bg_r = static_cast<uint8_t>(0.12f * 255);
    assert(buf[idx + 2] != bg_r || buf[idx + 1] != bg_r || buf[idx + 0] != bg_r);
    printf("OK\n");
}

static void test_ide_render_wire() {
    printf("  test_ide_render_wire... ");
    auto canvas = make_ref<Canvas>(400, 300);
    canvas->clear(colors::black);

    DrawContext ctx(canvas->buffer(), canvas->width(), canvas->height());
    ide::Theme theme;

    Point from = {100, 100};
    Point to = {300, 150};
    ide::draw_wire(ctx, from, to, theme.wire_color, 2.0f);

    // Check that some pixels along the wire path are drawn
    // The wire should pass through the midpoint area
    const uint8_t* buf = canvas->buffer();
    bool found_wire_pixel = false;
    for (int y = 90; y < 160; y++) {
        for (int x = 150; x < 250; x++) {
            int idx = (y * 400 + x) * 4;
            if (buf[idx + 3] > 0 && (buf[idx + 0] > 0 || buf[idx + 1] > 0 || buf[idx + 2] > 0)) {
                found_wire_pixel = true;
                break;
            }
        }
        if (found_wire_pixel) break;
    }
    assert(found_wire_pixel);
    printf("OK\n");
}

static void test_ide_grid() {
    printf("  test_ide_grid... ");
    auto canvas = make_ref<Canvas>(200, 200);
    canvas->clear(colors::black);

    DrawContext ctx(canvas->buffer(), canvas->width(), canvas->height());
    ide::Theme theme;
    ide::draw_grid(ctx, 0, 0, 1.0f, theme);

    // Check that grid dots exist at multiples of grid_size
    const uint8_t* buf = canvas->buffer();
    int gs = static_cast<int>(theme.grid_size);
    int idx = (gs * 200 + gs) * 4;
    // Grid pixel should be the grid color (not black)
    assert(buf[idx + 0] > 0 || buf[idx + 1] > 0 || buf[idx + 2] > 0);
    printf("OK\n");
}

int main() {
    printf("=== Phase 7: IDE Graph Editor Tests ===\n");
    test_fuzzy_score();
    test_fuzzy_filter();
    test_ide_node_layout();
    test_ide_render_node();
    test_ide_render_wire();
    test_ide_grid();
    printf("All Phase 7 tests passed!\n");
    return 0;
}
