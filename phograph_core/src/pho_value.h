#pragma once
#include "pho_mem.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>

namespace pho {

enum class ValueTag : uint8_t {
    Null = 0,
    Integer,
    Real,
    Boolean,
    String,
    List,
    Dict,
    Data,
    Date,
    Object,
    External,
    Error,
    Enum,
    MethodRef,
};

const char* tag_name(ValueTag tag);

class PhoString;
class PhoList;
class PhoDict;
class PhoData;
class PhoError;
class PhoEnum;
class PhoObject;
class PhoMethodRef;

// 24-byte tagged union value
class Value {
public:
    Value() : tag_(ValueTag::Null), i64_(0) {}

    // Scalar constructors
    static Value null_val() { return Value(); }
    static Value integer(int64_t v) { Value r; r.tag_ = ValueTag::Integer; r.i64_ = v; return r; }
    static Value real(double v) { Value r; r.tag_ = ValueTag::Real; r.f64_ = v; return r; }
    static Value boolean(bool v) { Value r; r.tag_ = ValueTag::Boolean; r.i64_ = v ? 1 : 0; return r; }
    static Value date(double timestamp) { Value r; r.tag_ = ValueTag::Date; r.f64_ = timestamp; return r; }

    // Ref-counted constructors
    static Value string(Ref<PhoString> s);
    static Value string(const char* s);
    static Value string(const std::string& s);
    static Value list(Ref<PhoList> l);
    static Value list(std::vector<Value> elems);
    static Value dict(Ref<PhoDict> d);
    static Value data(Ref<PhoData> d);
    static Value error(Ref<PhoError> e);
    static Value error(const std::string& msg);
    static Value error(const std::string& msg, const std::string& code);
    static Value enum_val(Ref<PhoEnum> e);
    static Value object(Ref<PhoObject> o);
    static Value method_ref(Ref<PhoMethodRef> m);

    // Copy/move
    Value(const Value& o);
    Value(Value&& o) noexcept;
    Value& operator=(const Value& o);
    Value& operator=(Value&& o) noexcept;
    ~Value();

    // Accessors
    ValueTag tag() const { return tag_; }
    bool is_null() const { return tag_ == ValueTag::Null; }
    bool is_integer() const { return tag_ == ValueTag::Integer; }
    bool is_real() const { return tag_ == ValueTag::Real; }
    bool is_boolean() const { return tag_ == ValueTag::Boolean; }
    bool is_string() const { return tag_ == ValueTag::String; }
    bool is_list() const { return tag_ == ValueTag::List; }
    bool is_dict() const { return tag_ == ValueTag::Dict; }
    bool is_data() const { return tag_ == ValueTag::Data; }
    bool is_date() const { return tag_ == ValueTag::Date; }
    bool is_object() const { return tag_ == ValueTag::Object; }
    bool is_external() const { return tag_ == ValueTag::External; }
    bool is_error() const { return tag_ == ValueTag::Error; }
    bool is_enum() const { return tag_ == ValueTag::Enum; }
    bool is_method_ref() const { return tag_ == ValueTag::MethodRef; }
    bool is_numeric() const { return tag_ == ValueTag::Integer || tag_ == ValueTag::Real; }

    int64_t as_integer() const;
    double as_real() const;
    double as_number() const; // works for both integer and real
    bool as_boolean() const;
    PhoString* as_string() const;
    PhoList* as_list() const;
    PhoDict* as_dict() const;
    PhoData* as_data() const;
    PhoError* as_error() const;
    PhoEnum* as_enum() const;
    PhoObject* as_object() const;
    PhoMethodRef* as_method_ref() const;
    double as_date() const;

    // Conversion to string for display
    std::string to_display_string() const;

    // Equality
    bool equals(const Value& o) const;

    // Comparison (-1, 0, 1)
    int compare(const Value& o) const;

    // Truthiness
    bool is_truthy() const;

private:
    void retain_ref();
    void release_ref();

    ValueTag tag_;
    union {
        int64_t i64_;
        double f64_;
        RefCounted* ptr_;
    };
};

// Ref-counted string
class PhoString : public RefCounted {
public:
    explicit PhoString(const char* s) : data_(s) {}
    explicit PhoString(const std::string& s) : data_(s) {}
    explicit PhoString(std::string&& s) : data_(std::move(s)) {}

    const std::string& str() const { return data_; }
    const char* c_str() const { return data_.c_str(); }
    size_t length() const { return data_.size(); }

private:
    std::string data_;
};

// Ref-counted list
class PhoList : public RefCounted {
public:
    PhoList() = default;
    explicit PhoList(std::vector<Value> elems) : elems_(std::move(elems)) {}

    const std::vector<Value>& elems() const { return elems_; }
    std::vector<Value>& elems_mut() { return elems_; }
    size_t size() const { return elems_.size(); }
    bool empty() const { return elems_.empty(); }
    const Value& at(size_t i) const { return elems_.at(i); }
    void push(Value v) { elems_.push_back(std::move(v)); }

private:
    std::vector<Value> elems_;
};

// Ref-counted dict
struct ValueHash {
    size_t operator()(const Value& v) const;
};
struct ValueEqual {
    bool operator()(const Value& a, const Value& b) const { return a.equals(b); }
};

class PhoDict : public RefCounted {
public:
    using Map = std::unordered_map<Value, Value, ValueHash, ValueEqual>;

    PhoDict() = default;

    const Map& entries() const { return entries_; }
    Map& entries_mut() { return entries_; }
    size_t size() const { return entries_.size(); }
    bool has(const Value& key) const { return entries_.count(key) > 0; }

    Value get(const Value& key) const {
        auto it = entries_.find(key);
        if (it != entries_.end()) return it->second;
        return Value::null_val();
    }

    void set(const Value& key, const Value& val) {
        entries_[key] = val;
    }

private:
    Map entries_;
};

// Ref-counted binary data
class PhoData : public RefCounted {
public:
    PhoData() = default;
    explicit PhoData(size_t size) : bytes_(size, 0) {}
    explicit PhoData(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {}

    const std::vector<uint8_t>& bytes() const { return bytes_; }
    std::vector<uint8_t>& bytes_mut() { return bytes_; }
    size_t length() const { return bytes_.size(); }

private:
    std::vector<uint8_t> bytes_;
};

// Ref-counted error
class PhoError : public RefCounted {
public:
    PhoError(const std::string& msg) : message_(msg) {}
    PhoError(const std::string& msg, const std::string& code) : message_(msg), code_(code) {}

    const std::string& message() const { return message_; }
    const std::string& code() const { return code_; }

private:
    std::string message_;
    std::string code_;
};

// Ref-counted enum
class PhoEnum : public RefCounted {
public:
    PhoEnum(const std::string& type_name, const std::string& variant, std::vector<Value> data)
        : type_name_(type_name), variant_(variant), data_(std::move(data)) {}

    const std::string& type_name() const { return type_name_; }
    const std::string& variant() const { return variant_; }
    const std::vector<Value>& data() const { return data_; }

private:
    std::string type_name_;
    std::string variant_;
    std::vector<Value> data_;
};

// Ref-counted object (class instance)
// Phase 26: observer callback
using ObserverCallback = std::function<void(const std::string& attr, const Value& old_val, const Value& new_val)>;
using ObserverId = uint64_t;

class PhoObject : public RefCounted {
public:
    explicit PhoObject(const std::string& class_name)
        : class_name_(class_name) {}

    const std::string& class_name() const { return class_name_; }

    Value get_attr(const std::string& name) const {
        auto it = attrs_.find(name);
        if (it != attrs_.end()) return it->second;
        return Value::null_val();
    }

    void set_attr(const std::string& name, Value val) {
        Value old_val = get_attr(name);
        attrs_[name] = val;
        // Phase 26: fire observers
        fire_observers(name, old_val, val);
    }

    bool has_attr(const std::string& name) const {
        return attrs_.count(name) > 0;
    }

    const std::unordered_map<std::string, Value>& attrs() const { return attrs_; }

    // Phase 26: observer API
    ObserverId add_observer(const std::string& attr, ObserverCallback cb) {
        ObserverId id = next_observer_id_++;
        observers_[attr].push_back({id, std::move(cb)});
        return id;
    }

    ObserverId add_any_observer(ObserverCallback cb) {
        ObserverId id = next_observer_id_++;
        any_observers_.push_back({id, std::move(cb)});
        return id;
    }

    void remove_observer(ObserverId id) {
        for (auto& [attr, list] : observers_) {
            list.erase(std::remove_if(list.begin(), list.end(),
                [id](const ObserverEntry& e) { return e.id == id; }), list.end());
        }
        any_observers_.erase(std::remove_if(any_observers_.begin(), any_observers_.end(),
            [id](const ObserverEntry& e) { return e.id == id; }), any_observers_.end());
    }

private:
    std::string class_name_;
    std::unordered_map<std::string, Value> attrs_;

    // Phase 26: observers
    struct ObserverEntry {
        ObserverId id;
        ObserverCallback callback;
    };
    std::unordered_map<std::string, std::vector<ObserverEntry>> observers_;
    std::vector<ObserverEntry> any_observers_;
    ObserverId next_observer_id_ = 1;

    void fire_observers(const std::string& attr, const Value& old_val, const Value& new_val) {
        auto it = observers_.find(attr);
        if (it != observers_.end()) {
            for (auto& entry : it->second) {
                entry.callback(attr, old_val, new_val);
            }
        }
        for (auto& entry : any_observers_) {
            entry.callback(attr, old_val, new_val);
        }
    }
};

// Ref-counted method reference (Phase 14)
class PhoMethodRef : public RefCounted {
public:
    PhoMethodRef(const std::string& class_name, const std::string& method_name)
        : class_name_(class_name), method_name_(method_name) {}
    PhoMethodRef(const std::string& class_name, const std::string& method_name, Ref<PhoObject> bound)
        : class_name_(class_name), method_name_(method_name), bound_object_(std::move(bound)) {}

    const std::string& class_name() const { return class_name_; }
    const std::string& method_name() const { return method_name_; }
    PhoObject* bound_object() const { return bound_object_.get(); }
    bool has_bound_object() const { return bound_object_.get() != nullptr; }

private:
    std::string class_name_;
    std::string method_name_;
    Ref<PhoObject> bound_object_;
};

} // namespace pho
