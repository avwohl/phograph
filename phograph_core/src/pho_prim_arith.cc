#include "pho_prim.h"
#include <cmath>
#include <random>

namespace pho {

// Helper: promote to real if either operand is real
static bool both_int(const Value& a, const Value& b) {
    return a.is_integer() && b.is_integer();
}

void register_arith_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("+", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1]))
            return PrimResult::success(Value::integer(in[0].as_integer() + in[1].as_integer()));
        return PrimResult::success(Value::real(in[0].as_number() + in[1].as_number()));
    });

    reg.register_prim("-", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1]))
            return PrimResult::success(Value::integer(in[0].as_integer() - in[1].as_integer()));
        return PrimResult::success(Value::real(in[0].as_number() - in[1].as_number()));
    });

    reg.register_prim("*", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1]))
            return PrimResult::success(Value::integer(in[0].as_integer() * in[1].as_integer()));
        return PrimResult::success(Value::real(in[0].as_number() * in[1].as_number()));
    });

    reg.register_prim("/", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        double b = in[1].as_number();
        if (b == 0.0) return PrimResult::fail_with(Value::error("division by zero"));
        if (both_int(in[0], in[1]) && in[0].as_integer() % in[1].as_integer() == 0)
            return PrimResult::success(Value::integer(in[0].as_integer() / in[1].as_integer()));
        return PrimResult::success(Value::real(in[0].as_number() / b));
    });

    reg.register_prim("div-mod", 2, 2, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("div-mod requires integers"));
        int64_t b = in[1].as_integer();
        if (b == 0) return PrimResult::fail_with(Value::error("division by zero"));
        int64_t a = in[0].as_integer();
        return PrimResult::success({Value::integer(a / b), Value::integer(a % b)});
    });

    reg.register_prim("abs", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_integer())
            return PrimResult::success(Value::integer(std::abs(in[0].as_integer())));
        return PrimResult::success(Value::real(std::fabs(in[0].as_number())));
    });

    reg.register_prim("round", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(std::round(in[0].as_number()))));
    });

    reg.register_prim("floor", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(std::floor(in[0].as_number()))));
    });

    reg.register_prim("ceiling", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(std::ceil(in[0].as_number()))));
    });

    reg.register_prim("truncate", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(std::trunc(in[0].as_number()))));
    });

    reg.register_prim("sqrt", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        double v = in[0].as_number();
        if (v < 0) return PrimResult::fail_with(Value::error("sqrt of negative"));
        return PrimResult::success(Value::real(std::sqrt(v)));
    });

    reg.register_prim("power", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::pow(in[0].as_number(), in[1].as_number())));
    });

    reg.register_prim("mod", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1])) {
            int64_t b = in[1].as_integer();
            if (b == 0) return PrimResult::fail_with(Value::error("mod by zero"));
            return PrimResult::success(Value::integer(in[0].as_integer() % b));
        }
        return PrimResult::success(Value::real(std::fmod(in[0].as_number(), in[1].as_number())));
    });

    reg.register_prim("min", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1]))
            return PrimResult::success(Value::integer(std::min(in[0].as_integer(), in[1].as_integer())));
        return PrimResult::success(Value::real(std::min(in[0].as_number(), in[1].as_number())));
    });

    reg.register_prim("max", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (both_int(in[0], in[1]))
            return PrimResult::success(Value::integer(std::max(in[0].as_integer(), in[1].as_integer())));
        return PrimResult::success(Value::real(std::max(in[0].as_number(), in[1].as_number())));
    });

    reg.register_prim("clamp", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_integer() && in[1].is_integer() && in[2].is_integer()) {
            int64_t v = in[0].as_integer();
            int64_t lo = in[1].as_integer();
            int64_t hi = in[2].as_integer();
            return PrimResult::success(Value::integer(std::max(lo, std::min(v, hi))));
        }
        double v = in[0].as_number();
        double lo = in[1].as_number();
        double hi = in[2].as_number();
        return PrimResult::success(Value::real(std::max(lo, std::min(v, hi))));
    });

    reg.register_prim("+1", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (in[0].is_integer())
            return PrimResult::success(Value::integer(in[0].as_integer() + 1));
        return PrimResult::success(Value::real(in[0].as_number() + 1.0));
    });

    // Trig
    reg.register_prim("pi", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        return PrimResult::success(Value::real(M_PI));
    });

    reg.register_prim("sin", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::sin(in[0].as_number())));
    });

    reg.register_prim("cos", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::cos(in[0].as_number())));
    });

    reg.register_prim("tan", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::tan(in[0].as_number())));
    });

    reg.register_prim("asin", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::asin(in[0].as_number())));
    });

    reg.register_prim("acos", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::acos(in[0].as_number())));
    });

    reg.register_prim("atan", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::atan(in[0].as_number())));
    });

    reg.register_prim("atan2", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::atan2(in[0].as_number(), in[1].as_number())));
    });

    // Logarithms
    reg.register_prim("ln", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::log(in[0].as_number())));
    });

    reg.register_prim("log10", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::log10(in[0].as_number())));
    });

    reg.register_prim("log2", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::real(std::log2(in[0].as_number())));
    });

    // Random
    reg.register_prim("rand", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        static thread_local std::mt19937 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        return PrimResult::success(Value::real(dist(rng)));
    });
}

} // namespace pho
