#pragma once
#include "pho_scene.h"
#include "pho_draw.h"
#include "pho_graph.h"
#include <cmath>

namespace pho {

// IDE-specific rendering: graph nodes, wires, pins.
// These render directly to a DrawContext for the graph canvas.

namespace ide {

// Visual constants
struct Theme {
    Color node_bg{0.18f, 0.18f, 0.22f, 1.0f};
    Color node_border{0.4f, 0.4f, 0.5f, 1.0f};
    Color node_selected_border{0.3f, 0.6f, 1.0f, 1.0f};
    Color node_header_bg{0.25f, 0.25f, 0.32f, 1.0f};
    Color pin_input{0.4f, 0.8f, 0.4f, 1.0f};
    Color pin_output{0.8f, 0.4f, 0.4f, 1.0f};
    Color wire_color{0.6f, 0.6f, 0.7f, 0.8f};
    Color wire_active{0.3f, 0.7f, 1.0f, 1.0f};
    Color text_color{0.9f, 0.9f, 0.9f, 1.0f};
    Color canvas_bg{0.12f, 0.12f, 0.15f, 1.0f};
    Color grid_color{0.16f, 0.16f, 0.2f, 1.0f};
    Color selection_rect{0.3f, 0.6f, 1.0f, 0.2f};

    float node_width = 160;
    float node_header_height = 28;
    float pin_radius = 5;
    float pin_spacing = 22;
    float wire_thickness = 2;
    float node_corner_radius = 6;
    float grid_size = 20;
};

// Layout information for a node in the graph canvas
struct NodeLayout {
    NodeId id;
    float x, y;           // top-left position
    float width, height;   // dimensions
    std::string label;
    bool selected = false;
    int num_inputs = 0;
    int num_outputs = 0;
};

// Get pin position (center of pin circle)
inline Point input_pin_pos(const NodeLayout& layout, int pin_index, const Theme& theme) {
    float x = layout.x;
    float y = layout.y + theme.node_header_height + theme.pin_spacing * (pin_index + 0.5f);
    return {x, y};
}

inline Point output_pin_pos(const NodeLayout& layout, int pin_index, const Theme& theme) {
    float x = layout.x + layout.width;
    float y = layout.y + theme.node_header_height + theme.pin_spacing * (pin_index + 0.5f);
    return {x, y};
}

// Draw background grid
inline void draw_grid(DrawContext& ctx, float offset_x, float offset_y, float scale, const Theme& theme) {
    float gs = theme.grid_size * scale;
    if (gs < 4) return; // too small to draw

    float sx = std::fmod(offset_x, gs);
    float sy = std::fmod(offset_y, gs);

    for (float x = sx; x < ctx.width(); x += gs) {
        for (float y = sy; y < ctx.height(); y += gs) {
            ctx.set_pixel(static_cast<int32_t>(x), static_cast<int32_t>(y), theme.grid_color);
        }
    }
}

// Draw a single node
inline void draw_node(DrawContext& ctx, const NodeLayout& layout, const Theme& theme) {
    float x = layout.x, y = layout.y;
    float w = layout.width, h = layout.height;

    // Node body
    ctx.fill_rounded_rect(x, y, w, h, theme.node_corner_radius, theme.node_bg);

    // Header
    ctx.fill_rounded_rect(x, y, w, theme.node_header_height, theme.node_corner_radius, theme.node_header_bg);
    // Bottom of header (square corners)
    ctx.fill_rect(x, y + theme.node_header_height - theme.node_corner_radius,
                  w, theme.node_corner_radius, theme.node_header_bg);

    // Border
    Color border = layout.selected ? theme.node_selected_border : theme.node_border;
    ctx.stroke_rect(x, y, w, h, border, 1.0f);

    // Input pins
    for (int i = 0; i < layout.num_inputs; i++) {
        Point p = input_pin_pos(layout, i, theme);
        ctx.fill_oval(p.x, p.y, theme.pin_radius, theme.pin_radius, theme.pin_input);
    }

    // Output pins
    for (int i = 0; i < layout.num_outputs; i++) {
        Point p = output_pin_pos(layout, i, theme);
        ctx.fill_oval(p.x, p.y, theme.pin_radius, theme.pin_radius, theme.pin_output);
    }
}

// Draw a wire as a bezier-like curve (approximated with line segments)
inline void draw_wire(DrawContext& ctx, Point from, Point to, Color color, float thickness) {
    // Cubic bezier: from -> cp1 -> cp2 -> to
    float dx = std::abs(to.x - from.x) * 0.5f;
    float cp1x = from.x + dx, cp1y = from.y;
    float cp2x = to.x - dx, cp2y = to.y;

    const int segments = 32;
    float prev_x = from.x, prev_y = from.y;

    for (int i = 1; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        float u = 1 - t;
        float x = u*u*u*from.x + 3*u*u*t*cp1x + 3*u*t*t*cp2x + t*t*t*to.x;
        float y = u*u*u*from.y + 3*u*u*t*cp1y + 3*u*t*t*cp2y + t*t*t*to.y;

        // Draw thick line segment
        float half_t = thickness / 2;
        float seg_dx = x - prev_x;
        float seg_dy = y - prev_y;
        float len = std::sqrt(seg_dx * seg_dx + seg_dy * seg_dy);
        if (len > 0.001f) {
            // Normal perpendicular
            float nx = -seg_dy / len * half_t;
            float ny = seg_dx / len * half_t;
            // Fill quad as two triangles (simplified: just draw fat hline for each y)
            float min_y = std::min({prev_y - half_t, prev_y + half_t, y - half_t, y + half_t});
            float max_y = std::max({prev_y - half_t, prev_y + half_t, y - half_t, y + half_t});
            for (float py = min_y; py <= max_y; py += 1.0f) {
                float min_x = std::min(prev_x, x) - half_t;
                float max_x = std::max(prev_x, x) + half_t;
                ctx.draw_hline(min_x, max_x, py, color);
            }
        }

        prev_x = x;
        prev_y = y;
    }
}

} // namespace ide
} // namespace pho
