#pragma once
#include "pho_mem.h"
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>

namespace pho {

struct Color {
    float r = 0, g = 0, b = 0, a = 1;
    Color() = default;
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    uint32_t to_bgra8() const {
        uint8_t br = static_cast<uint8_t>(std::min(std::max(b * 255.0f, 0.0f), 255.0f));
        uint8_t bg = static_cast<uint8_t>(std::min(std::max(g * 255.0f, 0.0f), 255.0f));
        uint8_t bb = static_cast<uint8_t>(std::min(std::max(r * 255.0f, 0.0f), 255.0f));
        uint8_t ba = static_cast<uint8_t>(std::min(std::max(a * 255.0f, 0.0f), 255.0f));
        return (ba << 24) | (bb << 16) | (bg << 8) | br;
    }

    static Color blend(const Color& c1, const Color& c2, float t) {
        return Color(
            c1.r + (c2.r - c1.r) * t,
            c1.g + (c2.g - c1.g) * t,
            c1.b + (c2.b - c1.b) * t,
            c1.a + (c2.a - c1.a) * t
        );
    }
};

// Named colors
namespace colors {
    static const Color black{0, 0, 0};
    static const Color white{1, 1, 1};
    static const Color red{1, 0, 0};
    static const Color green{0, 1, 0};
    static const Color blue{0, 0, 1};
    static const Color yellow{1, 1, 0};
    static const Color cyan{0, 1, 1};
    static const Color magenta{1, 0, 1};
    static const Color orange{1, 0.647f, 0};
    static const Color gray{0.5f, 0.5f, 0.5f};
    static const Color clear{0, 0, 0, 0};
}

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

struct Point {
    float x = 0, y = 0;
    Point() = default;
    Point(float x, float y) : x(x), y(y) {}
};

struct Transform2D {
    float m[6] = {1, 0, 0, 1, 0, 0}; // a,b,c,d,tx,ty

    static Transform2D identity() { return Transform2D(); }
    static Transform2D translate(float dx, float dy) {
        Transform2D t;
        t.m[4] = dx; t.m[5] = dy;
        return t;
    }
    static Transform2D scale(float sx, float sy) {
        Transform2D t;
        t.m[0] = sx; t.m[3] = sy;
        return t;
    }
    static Transform2D rotate(float radians) {
        Transform2D t;
        float c = std::cos(radians), s = std::sin(radians);
        t.m[0] = c; t.m[1] = s; t.m[2] = -s; t.m[3] = c;
        return t;
    }

    Point apply(Point p) const {
        return {m[0]*p.x + m[2]*p.y + m[4], m[1]*p.x + m[3]*p.y + m[5]};
    }

    Transform2D concat(const Transform2D& o) const {
        Transform2D r;
        r.m[0] = m[0]*o.m[0] + m[2]*o.m[1];
        r.m[1] = m[1]*o.m[0] + m[3]*o.m[1];
        r.m[2] = m[0]*o.m[2] + m[2]*o.m[3];
        r.m[3] = m[1]*o.m[2] + m[3]*o.m[3];
        r.m[4] = m[0]*o.m[4] + m[2]*o.m[5] + m[4];
        r.m[5] = m[1]*o.m[4] + m[3]*o.m[5] + m[5];
        return r;
    }
};

struct Style {
    Color fill_color = colors::clear;
    Color stroke_color = colors::clear;
    float stroke_width = 0;
    float corner_radius = 0;
    float opacity = 1.0f;
};

enum class ShapeType : uint8_t {
    Rect,
    Oval,
    Text,
    Image,
    Group,
    Path,
};

// Scene graph node
class Shape : public RefCounted {
public:
    ShapeType type = ShapeType::Rect;
    Rect bounds;
    Transform2D transform = Transform2D::identity();
    Style style;
    bool visible = true;
    bool needs_redraw = true;
    std::string tag;

    // Children
    std::vector<Ref<Shape>> children;
    Shape* parent = nullptr;

    // Text-specific
    std::string text;
    std::string font_name = "Helvetica";
    float font_size = 14.0f;

    void add_child(Ref<Shape> child) {
        child->parent = this;
        children.push_back(std::move(child));
        invalidate();
    }

    void remove_child(Shape* child) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (it->get() == child) {
                child->parent = nullptr;
                children.erase(it);
                invalidate();
                return;
            }
        }
    }

    void invalidate() {
        needs_redraw = true;
        if (parent) parent->invalidate();
    }

    Shape* find_by_tag(const std::string& t) {
        if (tag == t) return this;
        for (auto& c : children) {
            auto* found = c->find_by_tag(t);
            if (found) return found;
        }
        return nullptr;
    }

    bool hit_test(float px, float py) const {
        if (type == ShapeType::Oval) {
            float cx = bounds.x + bounds.w / 2;
            float cy = bounds.y + bounds.h / 2;
            float rx = bounds.w / 2;
            float ry = bounds.h / 2;
            if (rx <= 0 || ry <= 0) return false;
            float dx = (px - cx) / rx;
            float dy = (py - cy) / ry;
            return dx * dx + dy * dy <= 1.0f;
        }
        return bounds.contains(px, py);
    }
};

// Canvas: owns a pixel buffer and scene graph root
class Canvas : public RefCounted {
public:
    Canvas(int32_t w, int32_t h) : width_(w), height_(h), buffer_(w * h * 4, 0) {}

    int32_t width() const { return width_; }
    int32_t height() const { return height_; }
    uint8_t* buffer() { return buffer_.data(); }
    const uint8_t* buffer() const { return buffer_.data(); }
    size_t buffer_size() const { return buffer_.size(); }

    Ref<Shape> root;

    void resize(int32_t w, int32_t h) {
        width_ = w; height_ = h;
        buffer_.resize(w * h * 4, 0);
    }

    void clear(Color c = colors::clear) {
        uint32_t bgra = c.to_bgra8();
        auto* pixels = reinterpret_cast<uint32_t*>(buffer_.data());
        for (int32_t i = 0; i < width_ * height_; i++) pixels[i] = bgra;
    }

private:
    int32_t width_, height_;
    std::vector<uint8_t> buffer_;
};

} // namespace pho
