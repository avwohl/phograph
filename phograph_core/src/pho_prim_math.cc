#include "pho_prim.h"
#include <cmath>
#include <algorithm>

namespace pho {

static int64_t factorial(int64_t n) {
    if (n < 0) return -1;
    if (n <= 1) return 1;
    int64_t result = 1;
    for (int64_t i = 2; i <= n; i++) {
        result *= i;
        if (result < 0) return -1; // overflow
    }
    return result;
}

static int64_t fibonacci(int64_t n) {
    if (n < 0) return -1;
    if (n <= 1) return n;
    int64_t a = 0, b = 1;
    for (int64_t i = 2; i <= n; i++) {
        int64_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

static int64_t gcd(int64_t a, int64_t b) {
    a = std::abs(a);
    b = std::abs(b);
    while (b != 0) {
        int64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

void register_math_prims() {
    auto& r = PrimitiveRegistry::instance();

    r.register_prim("factorial", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("factorial: expected integer"));
        int64_t n = in[0].as_integer();
        if (n < 0 || n > 20) return PrimResult::fail_with(Value::error("factorial: input must be 0..20"));
        return PrimResult::success(Value::integer(factorial(n)));
    });

    r.register_prim("fibonacci", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("fibonacci: expected integer"));
        int64_t n = in[0].as_integer();
        if (n < 0 || n > 92) return PrimResult::fail_with(Value::error("fibonacci: input must be 0..92"));
        return PrimResult::success(Value::integer(fibonacci(n)));
    });

    r.register_prim("gcd", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("gcd: expected two integers"));
        return PrimResult::success(Value::integer(gcd(in[0].as_integer(), in[1].as_integer())));
    });

    r.register_prim("lcm", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("lcm: expected two integers"));
        int64_t a = in[0].as_integer(), b = in[1].as_integer();
        if (a == 0 && b == 0) return PrimResult::success(Value::integer(0));
        int64_t g = gcd(a, b);
        return PrimResult::success(Value::integer(std::abs(a / g * b)));
    });

    r.register_prim("is-prime", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("is-prime: expected integer"));
        int64_t n = in[0].as_integer();
        if (n < 2) return PrimResult::success(Value::boolean(false));
        if (n < 4) return PrimResult::success(Value::boolean(true));
        if (n % 2 == 0 || n % 3 == 0) return PrimResult::success(Value::boolean(false));
        for (int64_t i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0)
                return PrimResult::success(Value::boolean(false));
        }
        return PrimResult::success(Value::boolean(true));
    });
}

} // namespace pho
