#include "pho_prim.h"
#include "pho_value.h"
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace pho {

// Future: a container for a value that may arrive later.
// Stored as PhoObject with class_name "Future".
class PhoFuture : public RefCounted {
public:
    PhoFuture() = default;

    bool is_resolved() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return resolved_;
    }

    Value get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    void resolve(Value val) {
        std::vector<std::function<void(Value)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (resolved_) return;
            resolved_ = true;
            value_ = std::move(val);
            callbacks = std::move(then_callbacks_);
        }
        for (auto& cb : callbacks) {
            cb(value_);
        }
    }

    void then(std::function<void(Value)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (resolved_) {
            callback(value_);
        } else {
            then_callbacks_.push_back(std::move(callback));
        }
    }

private:
    mutable std::mutex mutex_;
    bool resolved_ = false;
    Value value_;
    std::vector<std::function<void(Value)>> then_callbacks_;
};

static Value wrap_future(Ref<PhoFuture> f) {
    auto obj = make_ref<PhoObject>("Future");
    obj->set_attr("_ptr", Value::integer(reinterpret_cast<int64_t>(f.get())));
    f->retain();
    return Value::object(std::move(obj));
}

static PhoFuture* unwrap_future(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Future") return nullptr;
    auto ptr = obj->get_attr("_ptr");
    if (!ptr.is_integer()) return nullptr;
    return reinterpret_cast<PhoFuture*>(ptr.as_integer());
}

void register_future_prims() {
    auto& r = PrimitiveRegistry::instance();

    // future-value: value -> future (already resolved)
    r.register_prim("future-value", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto f = make_ref<PhoFuture>();
        f->resolve(in[0]);
        return PrimResult::success(wrap_future(std::move(f)));
    });

    // future-create: -> future (unresolved)
    r.register_prim("future-create", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        auto f = make_ref<PhoFuture>();
        return PrimResult::success(wrap_future(std::move(f)));
    });

    // future-resolve: future value -> future
    r.register_prim("future-resolve", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* f = unwrap_future(in[0]);
        if (!f) return PrimResult::fail_with(Value::error("future-resolve: expected future"));
        f->resolve(in[1]);
        return PrimResult::success(in[0]);
    });

    // future-resolved?: future -> boolean
    r.register_prim("future-resolved?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* f = unwrap_future(in[0]);
        if (!f) return PrimResult::fail_with(Value::error("future-resolved?: expected future"));
        return PrimResult::success(Value::boolean(f->is_resolved()));
    });

    // future-get: future -> value (blocks if not resolved, returns null if not ready)
    r.register_prim("future-get", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* f = unwrap_future(in[0]);
        if (!f) return PrimResult::fail_with(Value::error("future-get: expected future"));
        if (!f->is_resolved()) return PrimResult::fail_with(Value::error("future-get: not yet resolved"));
        return PrimResult::success(f->get());
    });

    // future-then: future callback-name -> future (chains a new future)
    // The callback is a string naming a method to call with the result.
    // Returns a new future that resolves with the method's output.
    r.register_prim("future-then", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* f = unwrap_future(in[0]);
        if (!f) return PrimResult::fail_with(Value::error("future-then: expected future"));
        if (!in[1].is_string()) return PrimResult::fail_with(Value::error("future-then: expected method name"));

        auto result_future = make_ref<PhoFuture>();
        auto result_val = wrap_future(result_future);

        // When f resolves, store the value in result_future
        // (In a full implementation, we'd call the named method)
        PhoFuture* rf = result_future.get();
        rf->retain(); // prevent premature destruction
        f->then([rf](Value val) {
            rf->resolve(std::move(val));
            rf->release();
        });

        return PrimResult::success(result_val);
    });

    // future-all: list-of-futures -> future (resolves when all resolve)
    r.register_prim("future-all", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list()) return PrimResult::fail_with(Value::error("future-all: expected list"));
        auto* list = in[0].as_list();

        auto combined = make_ref<PhoFuture>();
        auto result_val = wrap_future(combined);

        size_t total = list->size();
        if (total == 0) {
            combined->resolve(Value::list(std::vector<Value>{}));
            return PrimResult::success(result_val);
        }

        struct SharedState : public RefCounted {
            std::mutex mutex;
            std::vector<Value> results;
            size_t remaining;
            PhoFuture* combined;
        };
        auto state = make_ref<SharedState>();
        state->results.resize(total);
        state->remaining = total;
        state->combined = combined.get();
        combined->retain();

        for (size_t i = 0; i < total; i++) {
            auto* f = unwrap_future(list->at(i));
            if (!f) continue;
            state->retain();
            f->then([s = state.get(), i](Value val) {
                bool all_done = false;
                {
                    std::lock_guard<std::mutex> lock(s->mutex);
                    s->results[i] = std::move(val);
                    s->remaining--;
                    all_done = (s->remaining == 0);
                }
                if (all_done) {
                    s->combined->resolve(Value::list(s->results));
                    s->combined->release();
                }
                s->release();
            });
        }

        return PrimResult::success(result_val);
    });

    // future-any: list-of-futures -> future (resolves when first resolves)
    r.register_prim("future-any", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list()) return PrimResult::fail_with(Value::error("future-any: expected list"));
        auto* list = in[0].as_list();

        auto result = make_ref<PhoFuture>();
        auto result_val = wrap_future(result);

        PhoFuture* rp = result.get();
        rp->retain();
        bool resolved_ref = false; // tracked via atomic in full impl

        for (size_t i = 0; i < list->size(); i++) {
            auto* f = unwrap_future(list->at(i));
            if (!f) continue;
            f->then([rp, &resolved_ref](Value val) {
                if (!rp->is_resolved()) {
                    rp->resolve(std::move(val));
                    rp->release();
                }
            });
        }

        return PrimResult::success(result_val);
    });
}

} // namespace pho
