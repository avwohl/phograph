#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>
#include <cassert>

namespace pho {

class RefCounted {
public:
    RefCounted() : ref_count_(1) {}
    virtual ~RefCounted() = default;

    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

    void retain() { ++ref_count_; }

    void release() {
        assert(ref_count_ > 0);
        if (--ref_count_ == 0) {
            delete this;
        }
    }

    int32_t ref_count() const { return ref_count_; }

private:
    int32_t ref_count_;
};

// Intrusive smart pointer for RefCounted objects
template <typename T>
class Ref {
public:
    Ref() : ptr_(nullptr) {}

    // Takes ownership (does NOT retain - caller already holds a ref)
    static Ref adopt(T* p) {
        Ref r;
        r.ptr_ = p;
        return r;
    }

    // Retains the pointer
    explicit Ref(T* p) : ptr_(p) {
        if (ptr_) ptr_->retain();
    }

    Ref(const Ref& o) : ptr_(o.ptr_) {
        if (ptr_) ptr_->retain();
    }

    Ref(Ref&& o) noexcept : ptr_(o.ptr_) {
        o.ptr_ = nullptr;
    }

    template <typename U>
    Ref(const Ref<U>& o) : ptr_(o.get()) {
        if (ptr_) ptr_->retain();
    }

    ~Ref() {
        if (ptr_) ptr_->release();
    }

    Ref& operator=(const Ref& o) {
        if (this != &o) {
            if (o.ptr_) o.ptr_->retain();
            if (ptr_) ptr_->release();
            ptr_ = o.ptr_;
        }
        return *this;
    }

    Ref& operator=(Ref&& o) noexcept {
        if (this != &o) {
            if (ptr_) ptr_->release();
            ptr_ = o.ptr_;
            o.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

    bool operator==(const Ref& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const Ref& o) const { return ptr_ != o.ptr_; }

    void reset() {
        if (ptr_) {
            ptr_->release();
            ptr_ = nullptr;
        }
    }

private:
    T* ptr_;
};

template <typename T, typename... Args>
Ref<T> make_ref(Args&&... args) {
    return Ref<T>::adopt(new T(std::forward<Args>(args)...));
}

} // namespace pho
