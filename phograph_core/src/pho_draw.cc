#include "pho_draw.h"
#include <cmath>
#include <algorithm>

namespace pho {

DrawContext::DrawContext(uint8_t* buffer, int32_t width, int32_t height)
    : buffer_(buffer), width_(width), height_(height) {}

void DrawContext::clear(Color c) {
    uint32_t bgra = c.to_bgra8();
    auto* pixels = reinterpret_cast<uint32_t*>(buffer_);
    for (int32_t i = 0; i < width_ * height_; i++) pixels[i] = bgra;
}

void DrawContext::clamp_rect(int32_t& x0, int32_t& y0, int32_t& x1, int32_t& y1) {
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, width_);
    y1 = std::min(y1, height_);
}

void DrawContext::blend_pixel(int32_t x, int32_t y, Color src) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    if (src.a <= 0.0f) return;

    int idx = (y * width_ + x) * 4;
    if (src.a >= 1.0f) {
        // Opaque - write directly in BGRA order
        buffer_[idx + 0] = static_cast<uint8_t>(std::min(std::max(src.b * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 1] = static_cast<uint8_t>(std::min(std::max(src.g * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 2] = static_cast<uint8_t>(std::min(std::max(src.r * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 3] = 255;
    } else {
        // Alpha blend
        float sa = src.a;
        float da = buffer_[idx + 3] / 255.0f;
        float out_a = sa + da * (1.0f - sa);
        if (out_a <= 0.0f) return;

        float db = buffer_[idx + 0] / 255.0f;
        float dg = buffer_[idx + 1] / 255.0f;
        float dr = buffer_[idx + 2] / 255.0f;

        float out_b = (src.b * sa + db * da * (1.0f - sa)) / out_a;
        float out_g = (src.g * sa + dg * da * (1.0f - sa)) / out_a;
        float out_r = (src.r * sa + dr * da * (1.0f - sa)) / out_a;

        buffer_[idx + 0] = static_cast<uint8_t>(std::min(std::max(out_b * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 1] = static_cast<uint8_t>(std::min(std::max(out_g * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 2] = static_cast<uint8_t>(std::min(std::max(out_r * 255.0f, 0.0f), 255.0f));
        buffer_[idx + 3] = static_cast<uint8_t>(std::min(std::max(out_a * 255.0f, 0.0f), 255.0f));
    }
}

void DrawContext::set_pixel(int32_t x, int32_t y, Color c) {
    blend_pixel(x, y, c);
}

void DrawContext::draw_hline(float x1, float x2, float y, Color c) {
    int32_t iy = static_cast<int32_t>(y);
    if (iy < 0 || iy >= height_) return;
    int32_t ix1 = static_cast<int32_t>(std::ceil(x1));
    int32_t ix2 = static_cast<int32_t>(std::floor(x2));
    ix1 = std::max(ix1, 0);
    ix2 = std::min(ix2, width_ - 1);
    for (int32_t x = ix1; x <= ix2; x++) {
        blend_pixel(x, iy, c);
    }
}

void DrawContext::fill_rect(float x, float y, float w, float h, Color c) {
    if (c.a <= 0.0f) return;
    int32_t x0 = static_cast<int32_t>(std::ceil(x));
    int32_t y0 = static_cast<int32_t>(std::ceil(y));
    int32_t x1 = static_cast<int32_t>(std::floor(x + w));
    int32_t y1 = static_cast<int32_t>(std::floor(y + h));
    clamp_rect(x0, y0, x1, y1);

    if (c.a >= 1.0f) {
        // Fast path: opaque fill
        uint32_t bgra = c.to_bgra8();
        for (int32_t py = y0; py < y1; py++) {
            auto* row = reinterpret_cast<uint32_t*>(buffer_ + py * width_ * 4);
            for (int32_t px = x0; px < x1; px++) {
                row[px] = bgra;
            }
        }
    } else {
        for (int32_t py = y0; py < y1; py++) {
            for (int32_t px = x0; px < x1; px++) {
                blend_pixel(px, py, c);
            }
        }
    }
}

void DrawContext::stroke_rect(float x, float y, float w, float h, Color c, float line_w) {
    if (c.a <= 0.0f || line_w <= 0.0f) return;
    // Top
    fill_rect(x, y, w, line_w, c);
    // Bottom
    fill_rect(x, y + h - line_w, w, line_w, c);
    // Left
    fill_rect(x, y + line_w, line_w, h - 2 * line_w, c);
    // Right
    fill_rect(x + w - line_w, y + line_w, line_w, h - 2 * line_w, c);
}

void DrawContext::fill_rounded_rect(float x, float y, float w, float h, float radius, Color c) {
    if (c.a <= 0.0f) return;
    radius = std::min(radius, std::min(w / 2, h / 2));
    if (radius <= 0.5f) {
        fill_rect(x, y, w, h, c);
        return;
    }

    int32_t ix0 = static_cast<int32_t>(std::ceil(x));
    int32_t iy0 = static_cast<int32_t>(std::ceil(y));
    int32_t ix1 = static_cast<int32_t>(std::floor(x + w));
    int32_t iy1 = static_cast<int32_t>(std::floor(y + h));
    clamp_rect(ix0, iy0, ix1, iy1);

    // Four corner centers
    float tl_cx = x + radius, tl_cy = y + radius;
    float tr_cx = x + w - radius, tr_cy = y + radius;
    float bl_cx = x + radius, bl_cy = y + h - radius;
    float br_cx = x + w - radius, br_cy = y + h - radius;
    float r2 = radius * radius;

    for (int32_t py = iy0; py < iy1; py++) {
        for (int32_t px = ix0; px < ix1; px++) {
            float fx = px + 0.5f, fy = py + 0.5f;
            // Check if in a corner region
            bool in_corner = false;
            float dx = 0, dy = 0;
            if (fx < tl_cx && fy < tl_cy) { dx = fx - tl_cx; dy = fy - tl_cy; in_corner = true; }
            else if (fx > tr_cx && fy < tr_cy) { dx = fx - tr_cx; dy = fy - tr_cy; in_corner = true; }
            else if (fx < bl_cx && fy > bl_cy) { dx = fx - bl_cx; dy = fy - bl_cy; in_corner = true; }
            else if (fx > br_cx && fy > br_cy) { dx = fx - br_cx; dy = fy - br_cy; in_corner = true; }

            if (in_corner && dx * dx + dy * dy > r2) continue;
            blend_pixel(px, py, c);
        }
    }
}

void DrawContext::fill_oval(float cx, float cy, float rx, float ry, Color c) {
    if (c.a <= 0.0f || rx <= 0.0f || ry <= 0.0f) return;

    int32_t y0 = static_cast<int32_t>(std::ceil(cy - ry));
    int32_t y1 = static_cast<int32_t>(std::floor(cy + ry));
    y0 = std::max(y0, 0);
    y1 = std::min(y1, height_ - 1);

    for (int32_t py = y0; py <= y1; py++) {
        float dy = (py + 0.5f - cy) / ry;
        if (dy * dy > 1.0f) continue;
        float half_w = rx * std::sqrt(1.0f - dy * dy);
        int32_t x0 = static_cast<int32_t>(std::ceil(cx - half_w));
        int32_t x1 = static_cast<int32_t>(std::floor(cx + half_w));
        x0 = std::max(x0, 0);
        x1 = std::min(x1, width_ - 1);
        for (int32_t px = x0; px <= x1; px++) {
            blend_pixel(px, py, c);
        }
    }
}

void DrawContext::stroke_oval(float cx, float cy, float rx, float ry, Color c, float line_w) {
    if (c.a <= 0.0f || rx <= 0.0f || ry <= 0.0f || line_w <= 0.0f) return;

    float outer_rx = rx, outer_ry = ry;
    float inner_rx = rx - line_w, inner_ry = ry - line_w;

    int32_t y0 = static_cast<int32_t>(std::ceil(cy - outer_ry));
    int32_t y1 = static_cast<int32_t>(std::floor(cy + outer_ry));
    y0 = std::max(y0, 0);
    y1 = std::min(y1, height_ - 1);

    for (int32_t py = y0; py <= y1; py++) {
        float fy = py + 0.5f;
        float ody = (fy - cy) / outer_ry;
        if (ody * ody > 1.0f) continue;

        float outer_half = outer_rx * std::sqrt(1.0f - ody * ody);
        int32_t ox0 = static_cast<int32_t>(std::ceil(cx - outer_half));
        int32_t ox1 = static_cast<int32_t>(std::floor(cx + outer_half));
        ox0 = std::max(ox0, 0);
        ox1 = std::min(ox1, width_ - 1);

        if (inner_rx > 0.0f && inner_ry > 0.0f) {
            float idy = (fy - cy) / inner_ry;
            if (idy * idy < 1.0f) {
                float inner_half = inner_rx * std::sqrt(1.0f - idy * idy);
                int32_t ix0 = static_cast<int32_t>(std::floor(cx - inner_half));
                int32_t ix1 = static_cast<int32_t>(std::ceil(cx + inner_half));
                // Left arc
                for (int32_t px = ox0; px <= std::min(ix0, ox1); px++)
                    blend_pixel(px, py, c);
                // Right arc
                for (int32_t px = std::max(ix1, ox0); px <= ox1; px++)
                    blend_pixel(px, py, c);
                continue;
            }
        }
        // Full scanline (inside inner ellipse doesn't exist)
        for (int32_t px = ox0; px <= ox1; px++)
            blend_pixel(px, py, c);
    }
}

void DrawContext::render_shape(const Shape& shape, Transform2D parent_xform) {
    if (!shape.visible) return;

    Transform2D xform = parent_xform.concat(shape.transform);
    float opacity = shape.style.opacity;

    Color fill = shape.style.fill_color;
    fill.a *= opacity;
    Color stroke = shape.style.stroke_color;
    stroke.a *= opacity;

    // Apply transform to bounds to get screen position
    Point tl = xform.apply({shape.bounds.x, shape.bounds.y});
    Point br = xform.apply({shape.bounds.x + shape.bounds.w, shape.bounds.y + shape.bounds.h});
    float sx = tl.x, sy = tl.y;
    float sw = br.x - tl.x, sh = br.y - tl.y;

    switch (shape.type) {
        case ShapeType::Rect: {
            if (fill.a > 0.0f) {
                if (shape.style.corner_radius > 0.0f) {
                    fill_rounded_rect(sx, sy, sw, sh, shape.style.corner_radius, fill);
                } else {
                    fill_rect(sx, sy, sw, sh, fill);
                }
            }
            if (stroke.a > 0.0f && shape.style.stroke_width > 0.0f) {
                stroke_rect(sx, sy, sw, sh, stroke, shape.style.stroke_width);
            }
            break;
        }
        case ShapeType::Oval: {
            float cx = sx + sw / 2, cy = sy + sh / 2;
            float rx = sw / 2, ry = sh / 2;
            if (fill.a > 0.0f) fill_oval(cx, cy, rx, ry, fill);
            if (stroke.a > 0.0f && shape.style.stroke_width > 0.0f)
                stroke_oval(cx, cy, rx, ry, stroke, shape.style.stroke_width);
            break;
        }
        case ShapeType::Text:
            // Text rendering requires platform support (CoreText/FreeType).
            // For now, just draw a placeholder rect.
            if (fill.a > 0.0f) fill_rect(sx, sy, sw, sh, fill);
            break;
        case ShapeType::Image:
            // Image rendering requires image data loading.
            // Placeholder rect for now.
            if (fill.a > 0.0f) fill_rect(sx, sy, sw, sh, fill);
            break;
        case ShapeType::Group:
        case ShapeType::Path:
            // Group: just render children. Path: not yet implemented.
            if (fill.a > 0.0f) fill_rect(sx, sy, sw, sh, fill);
            break;
    }

    // Render children
    for (auto& child : shape.children) {
        render_shape(*child, xform);
    }
}

void DrawContext::render_canvas(Canvas& canvas) {
    if (!canvas.root) return;
    DrawContext ctx(canvas.buffer(), canvas.width(), canvas.height());
    ctx.clear(colors::clear);
    ctx.render_shape(*canvas.root);
}

} // namespace pho
