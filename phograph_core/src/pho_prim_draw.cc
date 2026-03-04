#include "pho_prim.h"
#include "pho_prim_wrappers.h"
#include "pho_draw.h"

namespace pho {

void register_draw_prims() {
    auto& r = PrimitiveRegistry::instance();

    // draw-fill-rect: canvas x y w h color -> canvas
    r.register_prim("draw-fill-rect", 6, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-fill-rect: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.fill_rect(
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number()),
            static_cast<float>(in[4].as_number()),
            color_from_value(in[5])
        );
        return PrimResult::success(in[0]);
    });

    // draw-stroke-rect: canvas x y w h color line-width -> canvas
    r.register_prim("draw-stroke-rect", 7, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-stroke-rect: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.stroke_rect(
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number()),
            static_cast<float>(in[4].as_number()),
            color_from_value(in[5]),
            static_cast<float>(in[6].as_number())
        );
        return PrimResult::success(in[0]);
    });

    // draw-fill-oval: canvas cx cy rx ry color -> canvas
    r.register_prim("draw-fill-oval", 6, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-fill-oval: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.fill_oval(
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number()),
            static_cast<float>(in[4].as_number()),
            color_from_value(in[5])
        );
        return PrimResult::success(in[0]);
    });

    // draw-fill-rounded-rect: canvas x y w h radius color -> canvas
    r.register_prim("draw-fill-rounded-rect", 7, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-fill-rounded-rect: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.fill_rounded_rect(
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number()),
            static_cast<float>(in[4].as_number()),
            static_cast<float>(in[5].as_number()),
            color_from_value(in[6])
        );
        return PrimResult::success(in[0]);
    });

    // draw-set-pixel: canvas x y color -> canvas
    r.register_prim("draw-set-pixel", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-set-pixel: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.set_pixel(
            static_cast<int32_t>(in[1].as_number()),
            static_cast<int32_t>(in[2].as_number()),
            color_from_value(in[3])
        );
        return PrimResult::success(in[0]);
    });

    // draw-clear: canvas color -> canvas
    r.register_prim("draw-clear", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* c = unwrap_canvas(in[0]);
        if (!c) return PrimResult::fail_with(Value::error("draw-clear: expected canvas"));
        DrawContext ctx(c->buffer(), c->width(), c->height());
        ctx.clear(color_from_value(in[1]));
        return PrimResult::success(in[0]);
    });
}

} // namespace pho
