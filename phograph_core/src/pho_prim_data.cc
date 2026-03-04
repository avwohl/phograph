#include "pho_prim.h"

namespace pho {

void register_data_prims() {
    auto& reg = PrimitiveRegistry::instance();

    reg.register_prim("data-create", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        int64_t size = in[0].as_integer();
        if (size < 0) return PrimResult::fail_with(Value::error("data-create: negative size"));
        return PrimResult::success(Value::data(make_ref<PhoData>(static_cast<size_t>(size))));
    });

    reg.register_prim("data-from-list", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* list = in[0].as_list();
        std::vector<uint8_t> bytes;
        bytes.reserve(list->size());
        for (auto& v : list->elems()) {
            int64_t b = v.as_integer();
            if (b < 0 || b > 255) return PrimResult::fail_with(Value::error("byte out of range"));
            bytes.push_back(static_cast<uint8_t>(b));
        }
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    reg.register_prim("data-to-list", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_data();
        std::vector<Value> elems;
        for (auto b : d->bytes()) elems.push_back(Value::integer(b));
        return PrimResult::success(Value::list(std::move(elems)));
    });

    reg.register_prim("data-length", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        return PrimResult::success(Value::integer(static_cast<int64_t>(in[0].as_data()->length())));
    });

    reg.register_prim("data-slice", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_data();
        int64_t offset = in[1].as_integer();
        int64_t length = in[2].as_integer();
        if (offset < 0 || length < 0 || static_cast<size_t>(offset + length) > d->length())
            return PrimResult::fail_with(Value::error("data-slice: out of bounds"));
        std::vector<uint8_t> bytes(d->bytes().begin() + offset, d->bytes().begin() + offset + length);
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    reg.register_prim("data-concat", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* a = in[0].as_data();
        auto* b = in[1].as_data();
        std::vector<uint8_t> bytes = a->bytes();
        bytes.insert(bytes.end(), b->bytes().begin(), b->bytes().end());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    reg.register_prim("data-get-byte", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_data();
        int64_t idx = in[1].as_integer();
        if (idx < 0 || static_cast<size_t>(idx) >= d->length())
            return PrimResult::fail_with(Value::error("data-get-byte: out of bounds"));
        return PrimResult::success(Value::integer(d->bytes()[idx]));
    });

    reg.register_prim("data-set-byte", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_data();
        int64_t idx = in[1].as_integer();
        int64_t val = in[2].as_integer();
        if (idx < 0 || static_cast<size_t>(idx) >= d->length())
            return PrimResult::fail_with(Value::error("data-set-byte: out of bounds"));
        std::vector<uint8_t> bytes = d->bytes();
        bytes[idx] = static_cast<uint8_t>(val);
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });

    reg.register_prim("data-to-string", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* d = in[0].as_data();
        // For now, treat as UTF-8
        std::string s(d->bytes().begin(), d->bytes().end());
        return PrimResult::success(Value::string(s));
    });

    reg.register_prim("data-from-string", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto& s = in[0].as_string()->str();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(bytes))));
    });
}

} // namespace pho
