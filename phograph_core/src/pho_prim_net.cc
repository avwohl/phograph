#include "pho_prim.h"
#include <cstring>

// Net primitives: HTTP requires platform support (NSURLSession on macOS).
// These are registered as stubs now. The bridge will connect them to
// actual HTTP when the platform layer is extended.

namespace pho {

void register_net_prims() {
    auto& r = PrimitiveRegistry::instance();

    // http-get: url -> string (response body)
    r.register_prim("http-get", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("http-get: expected string URL"));
        return PrimResult::fail_with(Value::error("http-get: not yet connected to platform HTTP layer"));
    });

    // http-post: url body content-type -> string (response body)
    r.register_prim("http-post", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string() || !in[1].is_string() || !in[2].is_string())
            return PrimResult::fail_with(Value::error("http-post: expected url, body, content-type strings"));
        return PrimResult::fail_with(Value::error("http-post: not yet connected to platform HTTP layer"));
    });

    // json-parse: string -> value (dict or list)
    r.register_prim("json-parse", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("json-parse: expected string"));
        return PrimResult::fail_with(Value::error("json-parse: not yet implemented"));
    });
}

} // namespace pho
