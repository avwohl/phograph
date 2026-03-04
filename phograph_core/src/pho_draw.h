#pragma once
#include "pho_scene.h"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace pho {

// CPU rasterizer that draws shapes to a BGRA8 pixel buffer.
class DrawContext {
public:
    DrawContext(uint8_t* buffer, int32_t width, int32_t height);

    int32_t width() const { return width_; }
    int32_t height() const { return height_; }

    // Clear entire buffer
    void clear(Color c = colors::clear);

    // Draw a filled rectangle
    void fill_rect(float x, float y, float w, float h, Color c);

    // Draw a stroked rectangle
    void stroke_rect(float x, float y, float w, float h, Color c, float line_w);

    // Draw a filled rounded rectangle
    void fill_rounded_rect(float x, float y, float w, float h, float radius, Color c);

    // Draw a filled oval
    void fill_oval(float cx, float cy, float rx, float ry, Color c);

    // Draw a stroked oval
    void stroke_oval(float cx, float cy, float rx, float ry, Color c, float line_w);

    // Draw a horizontal line
    void draw_hline(float x1, float x2, float y, Color c);

    // Set a single pixel (with alpha blending)
    void set_pixel(int32_t x, int32_t y, Color c);

    // Render the full scene graph
    void render_shape(const Shape& shape, Transform2D parent_xform = Transform2D::identity());

    // Render a canvas's scene graph into its own buffer
    static void render_canvas(Canvas& canvas);

private:
    // Blend src color over dst pixel at buffer position
    void blend_pixel(int32_t x, int32_t y, Color src);

    // Clamp coordinates to buffer bounds
    void clamp_rect(int32_t& x0, int32_t& y0, int32_t& x1, int32_t& y1);

    uint8_t* buffer_;
    int32_t width_;
    int32_t height_;
};

} // namespace pho
