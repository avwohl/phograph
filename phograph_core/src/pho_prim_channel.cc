#include "pho_prim.h"
#include "pho_value.h"
#include <mutex>
#include <queue>

namespace pho {

// Channel: thread-safe FIFO queue for passing values between computations.
class PhoChannel : public RefCounted {
public:
    explicit PhoChannel(size_t capacity = 0) : capacity_(capacity) {}

    bool send(Value val) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) return false;
        if (capacity_ > 0 && queue_.size() >= capacity_) return false;
        queue_.push(std::move(val));
        return true;
    }

    bool receive(Value& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool is_empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }

    bool is_closed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

private:
    mutable std::mutex mutex_;
    std::queue<Value> queue_;
    size_t capacity_;
    bool closed_ = false;
};

static Value wrap_channel(Ref<PhoChannel> ch) {
    auto obj = make_ref<PhoObject>("Channel");
    obj->set_attr("_ptr", Value::integer(reinterpret_cast<int64_t>(ch.get())));
    ch->retain();
    return Value::object(std::move(obj));
}

static PhoChannel* unwrap_channel(const Value& v) {
    if (!v.is_object()) return nullptr;
    auto* obj = v.as_object();
    if (obj->class_name() != "Channel") return nullptr;
    auto ptr = obj->get_attr("_ptr");
    if (!ptr.is_integer()) return nullptr;
    return reinterpret_cast<PhoChannel*>(ptr.as_integer());
}

void register_channel_prims() {
    auto& r = PrimitiveRegistry::instance();

    // channel-create: -> channel (unbounded)
    r.register_prim("channel-create", 0, 1, [](const std::vector<Value>&) -> PrimResult {
        auto ch = make_ref<PhoChannel>();
        return PrimResult::success(wrap_channel(std::move(ch)));
    });

    // channel-create-bounded: capacity -> channel
    r.register_prim("channel-create-bounded", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("channel-create-bounded: expected integer"));
        auto ch = make_ref<PhoChannel>(static_cast<size_t>(in[0].as_integer()));
        return PrimResult::success(wrap_channel(std::move(ch)));
    });

    // channel-send: channel value -> boolean
    r.register_prim("channel-send", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-send: expected channel"));
        bool ok = ch->send(in[1]);
        return PrimResult::success(Value::boolean(ok));
    });

    // channel-receive: channel -> value (or fail if empty)
    r.register_prim("channel-receive", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-receive: expected channel"));
        Value val;
        if (!ch->receive(val)) return PrimResult::fail();
        return PrimResult::success(std::move(val));
    });

    // channel-empty?: channel -> boolean
    r.register_prim("channel-empty?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-empty?: expected channel"));
        return PrimResult::success(Value::boolean(ch->is_empty()));
    });

    // channel-size: channel -> integer
    r.register_prim("channel-size", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-size: expected channel"));
        return PrimResult::success(Value::integer(static_cast<int64_t>(ch->size())));
    });

    // channel-close: channel -> boolean
    r.register_prim("channel-close", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-close: expected channel"));
        ch->close();
        return PrimResult::success(Value::boolean(true));
    });

    // channel-closed?: channel -> boolean
    r.register_prim("channel-closed?", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        auto* ch = unwrap_channel(in[0]);
        if (!ch) return PrimResult::fail_with(Value::error("channel-closed?: expected channel"));
        return PrimResult::success(Value::boolean(ch->is_closed()));
    });
}

} // namespace pho
