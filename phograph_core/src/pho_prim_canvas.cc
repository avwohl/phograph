#include "pho_prim.h"
#include "pho_prim_wrappers.h"
#include "pho_draw.h"

namespace pho {

void register_canvas_prims() {
    auto& r = PrimitiveRegistry::instance();

    // create-canvas: width height -> canvas
    r.register_prim("create-canvas", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric())
            return PrimResult::fail_with(Value::error("create-canvas: width and height must be numbers"));
        int32_t w = static_cast<int32_t>(in[0].as_number());
        int32_t h = static_cast<int32_t>(in[1].as_number());
        if (w <= 0 || h <= 0) return PrimResult::fail_with(Value::error("create-canvas: invalid dimensions"));
        auto canvas = make_ref<Canvas>(w, h);
        return PrimResult::success(wrap_canvas(std::move(canvas)));
    });

    // canvas-width: canvas -> integer
    r.register_prim("canvas-width", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("canvas-width: expected canvas"));
        return PrimResult::success(Value::integer(c->width()));
    });

    // canvas-height: canvas -> integer
    r.register_prim("canvas-height", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("canvas-height: expected canvas"));
        return PrimResult::success(Value::integer(c->height()));
    });

    // canvas-clear: canvas color -> canvas
    r.register_prim("canvas-clear", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("canvas-clear: expected canvas"));
        c->clear(color_from_value(in[1]));
        return PrimResult::success(in[0]);
    });

    // canvas-render: canvas scene -> canvas (sets root from scene, renders to pixel buffer, blits to display)
    r.register_prim("canvas-render", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ext = unwrap_canvas_ext(in[0]);
        if (!ext) return PrimResult::fail_with(Value::error("canvas-render: expected canvas"));
        // Set root from scene input if provided
        if (auto* shape_ext = unwrap_shape_ext(in[1])) {
            ext->canvas->root = shape_ext->shape;
        }
        DrawContext::render_canvas(*ext->canvas);
        // Copy rendered pixels to IDE display buffer
        pho_display_blit(ext->canvas->buffer(), ext->canvas->width(), ext->canvas->height());
        return PrimResult::success(in[0]);
    });

    // canvas-pixel-at: canvas x y -> list[r, g, b, a]
    r.register_prim("canvas-pixel-at", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("canvas-pixel-at: expected canvas"));
        int32_t x = static_cast<int32_t>(in[1].as_number());
        int32_t y = static_cast<int32_t>(in[2].as_number());
        if (x < 0 || x >= c->width() || y < 0 || y >= c->height())
            return PrimResult::fail_with(Value::error("canvas-pixel-at: coordinates out of bounds"));
        const uint8_t* buf = c->buffer();
        int idx = (y * c->width() + x) * 4;
        // BGRA -> RGBA
        std::vector<Value> rgba = {
            Value::real(buf[idx + 2] / 255.0),
            Value::real(buf[idx + 1] / 255.0),
            Value::real(buf[idx + 0] / 255.0),
            Value::real(buf[idx + 3] / 255.0)
        };
        return PrimResult::success(Value::list(std::move(rgba)));
    });

    // canvas-set-root: canvas shape -> canvas
    r.register_prim("canvas-set-root", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ext = unwrap_canvas_ext(in[0]);
        if (!ext) return PrimResult::fail_with(Value::error("canvas-set-root: expected canvas"));
        auto* shape_ext = unwrap_shape_ext(in[1]);
        if (!shape_ext) return PrimResult::fail_with(Value::error("canvas-set-root: expected shape"));
        ext->canvas->root = shape_ext->shape;
        return PrimResult::success(in[0]);
    });
}

} // namespace pho
