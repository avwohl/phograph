#include "pho_prim.h"
#include "pho_thread.h"
#include <cmath>

namespace pho {

// Easing functions
namespace easing {
    static float linear(float t) { return t; }
    static float ease_in_quad(float t) { return t * t; }
    static float ease_out_quad(float t) { return t * (2 - t); }
    static float ease_in_out_quad(float t) {
        return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
    }
    static float ease_in_cubic(float t) { return t * t * t; }
    static float ease_out_cubic(float t) { float t1 = t - 1; return t1 * t1 * t1 + 1; }
    static float ease_in_out_cubic(float t) {
        return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
    }
    static float ease_in_sine(float t) {
        return 1 - std::cos(t * static_cast<float>(M_PI) / 2);
    }
    static float ease_out_sine(float t) {
        return std::sin(t * static_cast<float>(M_PI) / 2);
    }
    static float ease_in_out_sine(float t) {
        return 0.5f * (1 - std::cos(static_cast<float>(M_PI) * t));
    }
    static float spring(float t) {
        return 1 - std::cos(t * 4.5f * static_cast<float>(M_PI)) * std::exp(-6 * t);
    }
    static float bounce_out(float t) {
        if (t < 1 / 2.75f) return 7.5625f * t * t;
        if (t < 2 / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
        if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }

    using EasingFn = float(*)(float);
    static EasingFn from_name(const std::string& name) {
        if (name == "linear") return linear;
        if (name == "ease-in" || name == "ease-in-quad") return ease_in_quad;
        if (name == "ease-out" || name == "ease-out-quad") return ease_out_quad;
        if (name == "ease-in-out" || name == "ease-in-out-quad") return ease_in_out_quad;
        if (name == "ease-in-cubic") return ease_in_cubic;
        if (name == "ease-out-cubic") return ease_out_cubic;
        if (name == "ease-in-out-cubic") return ease_in_out_cubic;
        if (name == "ease-in-sine") return ease_in_sine;
        if (name == "ease-out-sine") return ease_out_sine;
        if (name == "ease-in-out-sine") return ease_in_out_sine;
        if (name == "spring") return spring;
        if (name == "bounce") return bounce_out;
        return linear;
    }
}

void register_anim_prims() {
    auto& r = PrimitiveRegistry::instance();

    // ease: t easing-name -> eased-t
    // Applies an easing function to a t value (0..1)
    r.register_prim("ease", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("ease: t must be numeric"));
        if (!in[1].is_string()) return PrimResult::fail_with(Value::error("ease: name must be string"));
        float t = static_cast<float>(in[0].as_number());
        t = std::max(0.0f, std::min(1.0f, t));
        auto fn = easing::from_name(in[1].as_string()->str());
        return PrimResult::success(Value::real(fn(t)));
    });

    // lerp: a b t -> interpolated
    // Linear interpolation between two numbers
    r.register_prim("lerp", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric() || !in[2].is_numeric())
            return PrimResult::fail_with(Value::error("lerp: expected numbers"));
        double a = in[0].as_number();
        double b = in[1].as_number();
        double t = in[2].as_number();
        return PrimResult::success(Value::real(a + (b - a) * t));
    });

    // color-lerp: color1 color2 t -> color
    // Interpolate between two colors
    r.register_prim("color-lerp", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list() || !in[1].is_list() || !in[2].is_numeric())
            return PrimResult::fail_with(Value::error("color-lerp: expected colors and t"));
        auto* c1 = in[0].as_list();
        auto* c2 = in[1].as_list();
        if (c1->size() < 3 || c2->size() < 3)
            return PrimResult::fail_with(Value::error("color-lerp: colors need 3+ components"));
        double t = in[2].as_number();
        t = std::max(0.0, std::min(1.0, t));

        std::vector<Value> result;
        size_t n = std::min(c1->size(), c2->size());
        for (size_t i = 0; i < n; i++) {
            double a = c1->at(i).as_number();
            double b = c2->at(i).as_number();
            result.push_back(Value::real(a + (b - a) * t));
        }
        return PrimResult::success(Value::list(std::move(result)));
    });

    // clamp01: x -> clamped
    r.register_prim("clamp01", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric()) return PrimResult::fail_with(Value::error("clamp01: expected number"));
        double v = in[0].as_number();
        return PrimResult::success(Value::real(std::max(0.0, std::min(1.0, v))));
    });

    // animate-progress: start-time duration current-time -> t (0..1)
    r.register_prim("animate-progress", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_numeric() || !in[1].is_numeric() || !in[2].is_numeric())
            return PrimResult::fail_with(Value::error("animate-progress: expected numbers"));
        double start = in[0].as_number();
        double duration = in[1].as_number();
        double now = in[2].as_number();
        if (duration <= 0) return PrimResult::success(Value::real(1.0));
        double t = (now - start) / duration;
        t = std::max(0.0, std::min(1.0, t));
        return PrimResult::success(Value::real(t));
    });
}

} // namespace pho
