#include "pho_prim.h"
#include "pho_prim_wrappers.h"

namespace pho {

void register_scene_prims() {
    auto& r = PrimitiveRegistry::instance();

    // shape-rect: x y w h -> shape
    r.register_prim("shape-rect", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = make_ref<Shape>();
        s->type = ShapeType::Rect;
        s->bounds = Rect(
            static_cast<float>(in[0].as_number()),
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number())
        );
        return PrimResult::success(wrap_shape(std::move(s)));
    });

    // shape-oval: x y w h -> shape
    r.register_prim("shape-oval", 4, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = make_ref<Shape>();
        s->type = ShapeType::Oval;
        s->bounds = Rect(
            static_cast<float>(in[0].as_number()),
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number())
        );
        return PrimResult::success(wrap_shape(std::move(s)));
    });

    // shape-text: x y w h text -> shape
    r.register_prim("shape-text", 5, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = make_ref<Shape>();
        s->type = ShapeType::Text;
        s->bounds = Rect(
            static_cast<float>(in[0].as_number()),
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number())
        );
        if (in[4].is_string()) s->text = in[4].as_string()->str();
        return PrimResult::success(wrap_shape(std::move(s)));
    });

    // shape-group: a b -> shape (creates group with children)
    r.register_prim("shape-group", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto s = make_ref<Shape>();
        s->type = ShapeType::Group;
        // Add each non-null input as a child
        for (auto& v : in) {
            if (auto* child = unwrap_shape_ext(v)) {
                s->add_child(child->shape);
            }
        }
        return PrimResult::success(wrap_shape(std::move(s)));
    });

    // shape-set-fill: shape color -> shape
    r.register_prim("shape-set-fill", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-fill: expected shape"));
        s->style.fill_color = color_from_value(in[1]);
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-stroke: shape color width -> shape
    r.register_prim("shape-set-stroke", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-stroke: expected shape"));
        s->style.stroke_color = color_from_value(in[1]);
        s->style.stroke_width = static_cast<float>(in[2].as_number());
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-corner-radius: shape radius -> shape
    r.register_prim("shape-set-corner-radius", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-corner-radius: expected shape"));
        s->style.corner_radius = static_cast<float>(in[1].as_number());
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-opacity: shape opacity -> shape
    r.register_prim("shape-set-opacity", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-opacity: expected shape"));
        s->style.opacity = static_cast<float>(in[1].as_number());
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-bounds: shape x y w h -> shape
    r.register_prim("shape-set-bounds", 5, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-bounds: expected shape"));
        s->bounds = Rect(
            static_cast<float>(in[1].as_number()),
            static_cast<float>(in[2].as_number()),
            static_cast<float>(in[3].as_number()),
            static_cast<float>(in[4].as_number())
        );
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-visible: shape bool -> shape
    r.register_prim("shape-set-visible", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-visible: expected shape"));
        s->visible = in[1].is_truthy();
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-set-tag: shape tag -> shape
    r.register_prim("shape-set-tag", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-set-tag: expected shape"));
        if (in[1].is_string()) s->tag = in[1].as_string()->str();
        s->invalidate();
        return PrimResult::success(in[0]);
    });

    // shape-add-child: parent child -> parent
    r.register_prim("shape-add-child", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* parent = unwrap_shape(in[0]);
        if (!parent) return PrimResult::fail_with(Value::error("shape-add-child: expected shape parent"));
        auto* child_ext = unwrap_shape_ext(in[1]);
        if (!child_ext) return PrimResult::fail_with(Value::error("shape-add-child: expected shape child"));
        parent->add_child(child_ext->shape);
        return PrimResult::success(in[0]);
    });

    // shape-get-fill: shape -> color
    r.register_prim("shape-get-fill", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-get-fill: expected shape"));
        return PrimResult::success(color_to_value(s->style.fill_color));
    });

    // shape-get-bounds: shape -> list[x, y, w, h]
    r.register_prim("shape-get-bounds", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-get-bounds: expected shape"));
        return PrimResult::success(Value::list({
            Value::real(s->bounds.x), Value::real(s->bounds.y),
            Value::real(s->bounds.w), Value::real(s->bounds.h)
        }));
    });

    // shape-hit-test: shape x y -> boolean
    r.register_prim("shape-hit-test", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-hit-test: expected shape"));
        float px = static_cast<float>(in[1].as_number());
        float py = static_cast<float>(in[2].as_number());
        return PrimResult::success(Value::boolean(s->hit_test(px, py)));
    });

    // shape-find-by-tag: shape tag -> shape (or fail)
    r.register_prim("shape-find-by-tag", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* s = unwrap_shape(in[0]);
        if (!s) return PrimResult::fail_with(Value::error("shape-find-by-tag: expected shape"));
        if (!in[1].is_string()) return PrimResult::fail_with(Value::error("shape-find-by-tag: expected string tag"));
        auto* found = s->find_by_tag(in[1].as_string()->str());
        if (!found) return PrimResult::fail();
        found->retain();
        Ref<Shape> ref = Ref<Shape>::adopt(found);
        return PrimResult::success(wrap_shape(std::move(ref)));
    });
}

} // namespace pho
