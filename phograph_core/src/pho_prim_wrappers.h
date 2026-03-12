#pragma once
#include "pho_scene.h"

namespace pho {

// Wrappers for storing scene graph types in Value (via PhoObject with _ptr attribute).
// Shared between pho_prim_canvas.cc, pho_prim_scene.cc, pho_prim_draw.cc.

class CanvasExternal : public RefCounted {
public:
    Ref<Canvas> canvas;
    explicit CanvasExternal(Ref<Canvas> c) : canvas(std::move(c)) {}
};

class ShapeExternal : public RefCounted {
public:
    Ref<Shape> shape;
    explicit ShapeExternal(Ref<Shape> s) : shape(std::move(s)) {}
};

inline Value wrap_canvas(Ref<Canvas> c) {
    auto ext = make_ref<CanvasExternal>(std::move(c));
    auto obj = make_ref<PhoObject>("Canvas");
    obj->set_attr("_ptr", Value::integer(reinterpret_cast<int64_t>(ext.get())));
    ext->retain();
    return Value::object(std::move(obj));
}

inline Canvas* unwrap_canvas(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Canvas") return nullptr;
    auto ptr_val = obj->get_attr("_ptr");
    if (!ptr_val.is_integer()) return nullptr;
    return reinterpret_cast<CanvasExternal*>(ptr_val.as_integer())->canvas.get();
}

inline CanvasExternal* unwrap_canvas_ext(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Canvas") return nullptr;
    auto ptr_val = obj->get_attr("_ptr");
    if (!ptr_val.is_integer()) return nullptr;
    return reinterpret_cast<CanvasExternal*>(ptr_val.as_integer());
}

inline Value wrap_shape(Ref<Shape> s) {
    auto ext = make_ref<ShapeExternal>(std::move(s));
    auto obj = make_ref<PhoObject>("Shape");
    obj->set_attr("_ptr", Value::integer(reinterpret_cast<int64_t>(ext.get())));
    ext->retain();
    return Value::object(std::move(obj));
}

inline Shape* unwrap_shape(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Shape") return nullptr;
    auto ptr_val = obj->get_attr("_ptr");
    if (!ptr_val.is_integer()) return nullptr;
    return reinterpret_cast<ShapeExternal*>(ptr_val.as_integer())->shape.get();
}

inline ShapeExternal* unwrap_shape_ext(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Shape") return nullptr;
    auto ptr_val = obj->get_attr("_ptr");
    if (!ptr_val.is_integer()) return nullptr;
    return reinterpret_cast<ShapeExternal*>(ptr_val.as_integer());
}

inline Color color_from_value(const Value& v) {
    if (v.is_list()) {
        auto* l = v.as_list();
        if (l->size() >= 3) {
            float r = static_cast<float>(l->at(0).as_number());
            float g = static_cast<float>(l->at(1).as_number());
            float b = static_cast<float>(l->at(2).as_number());
            float a = l->size() >= 4 ? static_cast<float>(l->at(3).as_number()) : 1.0f;
            return Color(r, g, b, a);
        }
    }
    if (v.is_string()) {
        auto name = v.as_string()->str();
        if (name == "black")   return colors::black;
        if (name == "white")   return colors::white;
        if (name == "red")     return colors::red;
        if (name == "green")   return colors::green;
        if (name == "blue")    return colors::blue;
        if (name == "yellow")  return colors::yellow;
        if (name == "cyan")    return colors::cyan;
        if (name == "magenta") return colors::magenta;
        if (name == "orange")  return colors::orange;
        if (name == "gray" || name == "grey") return colors::gray;
        if (name == "clear")   return colors::clear;
    }
    return colors::clear;
}

inline Value color_to_value(const Color& c) {
    return Value::list({Value::real(c.r), Value::real(c.g), Value::real(c.b), Value::real(c.a)});
}

} // namespace pho
