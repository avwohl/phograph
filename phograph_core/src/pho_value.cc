#include "pho_value.h"
#include <sstream>
#include <iomanip>

namespace pho {

const char* tag_name(ValueTag tag) {
    switch (tag) {
        case ValueTag::Null: return "null";
        case ValueTag::Integer: return "integer";
        case ValueTag::Real: return "real";
        case ValueTag::Boolean: return "boolean";
        case ValueTag::String: return "string";
        case ValueTag::List: return "list";
        case ValueTag::Dict: return "dict";
        case ValueTag::Data: return "data";
        case ValueTag::Date: return "date";
        case ValueTag::Object: return "object";
        case ValueTag::External: return "external";
        case ValueTag::Error: return "error";
        case ValueTag::Enum: return "enum";
    }
    return "unknown";
}

// -- Value ref-counted constructors --

Value Value::string(Ref<PhoString> s) {
    Value r;
    r.tag_ = ValueTag::String;
    r.ptr_ = s.get();
    r.ptr_->retain();
    return r;
}

Value Value::string(const char* s) {
    return Value::string(make_ref<PhoString>(s));
}

Value Value::string(const std::string& s) {
    return Value::string(make_ref<PhoString>(s));
}

Value Value::list(Ref<PhoList> l) {
    Value r;
    r.tag_ = ValueTag::List;
    r.ptr_ = l.get();
    r.ptr_->retain();
    return r;
}

Value Value::list(std::vector<Value> elems) {
    return Value::list(make_ref<PhoList>(std::move(elems)));
}

Value Value::dict(Ref<PhoDict> d) {
    Value r;
    r.tag_ = ValueTag::Dict;
    r.ptr_ = d.get();
    r.ptr_->retain();
    return r;
}

Value Value::data(Ref<PhoData> d) {
    Value r;
    r.tag_ = ValueTag::Data;
    r.ptr_ = d.get();
    r.ptr_->retain();
    return r;
}

Value Value::error(Ref<PhoError> e) {
    Value r;
    r.tag_ = ValueTag::Error;
    r.ptr_ = e.get();
    r.ptr_->retain();
    return r;
}

Value Value::error(const std::string& msg) {
    return Value::error(make_ref<PhoError>(msg));
}

Value Value::error(const std::string& msg, const std::string& code) {
    return Value::error(make_ref<PhoError>(msg, code));
}

Value Value::object(Ref<PhoObject> o) {
    Value r;
    r.tag_ = ValueTag::Object;
    r.ptr_ = o.get();
    r.ptr_->retain();
    return r;
}

Value Value::enum_val(Ref<PhoEnum> e) {
    Value r;
    r.tag_ = ValueTag::Enum;
    r.ptr_ = e.get();
    r.ptr_->retain();
    return r;
}

// -- Copy/move --

void Value::retain_ref() {
    switch (tag_) {
        case ValueTag::String:
        case ValueTag::List:
        case ValueTag::Dict:
        case ValueTag::Data:
        case ValueTag::Error:
        case ValueTag::Enum:
        case ValueTag::Object:
        case ValueTag::External:
            if (ptr_) ptr_->retain();
            break;
        default:
            break;
    }
}

void Value::release_ref() {
    switch (tag_) {
        case ValueTag::String:
        case ValueTag::List:
        case ValueTag::Dict:
        case ValueTag::Data:
        case ValueTag::Error:
        case ValueTag::Enum:
        case ValueTag::Object:
        case ValueTag::External:
            if (ptr_) ptr_->release();
            break;
        default:
            break;
    }
}

Value::Value(const Value& o) : tag_(o.tag_), i64_(o.i64_) {
    retain_ref();
}

Value::Value(Value&& o) noexcept : tag_(o.tag_), i64_(o.i64_) {
    o.tag_ = ValueTag::Null;
    o.i64_ = 0;
}

Value& Value::operator=(const Value& o) {
    if (this != &o) {
        release_ref();
        tag_ = o.tag_;
        i64_ = o.i64_;
        retain_ref();
    }
    return *this;
}

Value& Value::operator=(Value&& o) noexcept {
    if (this != &o) {
        release_ref();
        tag_ = o.tag_;
        i64_ = o.i64_;
        o.tag_ = ValueTag::Null;
        o.i64_ = 0;
    }
    return *this;
}

Value::~Value() {
    release_ref();
}

// -- Accessors --

int64_t Value::as_integer() const {
    assert(tag_ == ValueTag::Integer);
    return i64_;
}

double Value::as_real() const {
    assert(tag_ == ValueTag::Real);
    return f64_;
}

double Value::as_number() const {
    if (tag_ == ValueTag::Integer) return static_cast<double>(i64_);
    if (tag_ == ValueTag::Real) return f64_;
    assert(false && "not a number");
    return 0.0;
}

bool Value::as_boolean() const {
    assert(tag_ == ValueTag::Boolean);
    return i64_ != 0;
}

PhoString* Value::as_string() const {
    assert(tag_ == ValueTag::String);
    return static_cast<PhoString*>(ptr_);
}

PhoList* Value::as_list() const {
    assert(tag_ == ValueTag::List);
    return static_cast<PhoList*>(ptr_);
}

PhoDict* Value::as_dict() const {
    assert(tag_ == ValueTag::Dict);
    return static_cast<PhoDict*>(ptr_);
}

PhoData* Value::as_data() const {
    assert(tag_ == ValueTag::Data);
    return static_cast<PhoData*>(ptr_);
}

PhoError* Value::as_error() const {
    assert(tag_ == ValueTag::Error);
    return static_cast<PhoError*>(ptr_);
}

PhoEnum* Value::as_enum() const {
    assert(tag_ == ValueTag::Enum);
    return static_cast<PhoEnum*>(ptr_);
}

PhoObject* Value::as_object() const {
    assert(tag_ == ValueTag::Object);
    return static_cast<PhoObject*>(ptr_);
}

double Value::as_date() const {
    assert(tag_ == ValueTag::Date);
    return f64_;
}

// -- Display --

std::string Value::to_display_string() const {
    switch (tag_) {
        case ValueTag::Null: return "null";
        case ValueTag::Integer: return std::to_string(i64_);
        case ValueTag::Real: {
            std::ostringstream oss;
            oss << f64_;
            return oss.str();
        }
        case ValueTag::Boolean: return i64_ ? "true" : "false";
        case ValueTag::String: return "\"" + as_string()->str() + "\"";
        case ValueTag::List: {
            std::string s = "(";
            auto* l = as_list();
            for (size_t i = 0; i < l->size(); i++) {
                if (i > 0) s += " ";
                s += l->at(i).to_display_string();
            }
            s += ")";
            return s;
        }
        case ValueTag::Dict: {
            std::string s = "{";
            auto* d = as_dict();
            bool first = true;
            for (auto& [k, v] : d->entries()) {
                if (!first) s += ", ";
                s += k.to_display_string() + ": " + v.to_display_string();
                first = false;
            }
            s += "}";
            return s;
        }
        case ValueTag::Data: return "<data:" + std::to_string(as_data()->length()) + ">";
        case ValueTag::Date: {
            std::ostringstream oss;
            oss << "<date:" << f64_ << ">";
            return oss.str();
        }
        case ValueTag::Error: return "<error:" + as_error()->message() + ">";
        case ValueTag::Enum: return as_enum()->type_name() + "." + as_enum()->variant();
        case ValueTag::Object: return "<object>";
        case ValueTag::External: return "<external>";
    }
    return "<unknown>";
}

// -- Equality --

bool Value::equals(const Value& o) const {
    if (tag_ != o.tag_) {
        // Integer == Real cross-comparison
        if (is_numeric() && o.is_numeric()) {
            return as_number() == o.as_number();
        }
        return false;
    }
    switch (tag_) {
        case ValueTag::Null: return true;
        case ValueTag::Integer: return i64_ == o.i64_;
        case ValueTag::Real: return f64_ == o.f64_;
        case ValueTag::Boolean: return i64_ == o.i64_;
        case ValueTag::String: return as_string()->str() == o.as_string()->str();
        case ValueTag::Date: return f64_ == o.f64_;
        case ValueTag::List: {
            auto* a = as_list();
            auto* b = o.as_list();
            if (a->size() != b->size()) return false;
            for (size_t i = 0; i < a->size(); i++) {
                if (!a->at(i).equals(b->at(i))) return false;
            }
            return true;
        }
        case ValueTag::Dict: {
            auto* a = as_dict();
            auto* b = o.as_dict();
            if (a->size() != b->size()) return false;
            for (auto& [k, v] : a->entries()) {
                if (!b->has(k) || !v.equals(b->get(k))) return false;
            }
            return true;
        }
        default:
            return ptr_ == o.ptr_; // identity comparison for objects etc.
    }
}

// -- Comparison --

int Value::compare(const Value& o) const {
    if (is_numeric() && o.is_numeric()) {
        double a = as_number(), b = o.as_number();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }
    if (tag_ == ValueTag::String && o.tag_ == ValueTag::String) {
        int c = as_string()->str().compare(o.as_string()->str());
        return c < 0 ? -1 : (c > 0 ? 1 : 0);
    }
    if (tag_ == ValueTag::Boolean && o.tag_ == ValueTag::Boolean) {
        return (i64_ < o.i64_) ? -1 : (i64_ > o.i64_ ? 1 : 0);
    }
    if (tag_ == ValueTag::Date && o.tag_ == ValueTag::Date) {
        if (f64_ < o.f64_) return -1;
        if (f64_ > o.f64_) return 1;
        return 0;
    }
    // Incomparable types
    return 0;
}

// -- Truthiness --

bool Value::is_truthy() const {
    switch (tag_) {
        case ValueTag::Null: return false;
        case ValueTag::Boolean: return i64_ != 0;
        case ValueTag::Integer: return i64_ != 0;
        case ValueTag::Real: return f64_ != 0.0;
        case ValueTag::String: return !as_string()->str().empty();
        case ValueTag::List: return !as_list()->empty();
        case ValueTag::Dict: return as_dict()->size() > 0;
        case ValueTag::Error: return false;
        default: return true;
    }
}

// -- Hash --

size_t ValueHash::operator()(const Value& v) const {
    switch (v.tag()) {
        case ValueTag::Integer: return std::hash<int64_t>{}(v.as_integer());
        case ValueTag::Real: return std::hash<double>{}(v.as_real());
        case ValueTag::Boolean: return std::hash<bool>{}(v.as_boolean());
        case ValueTag::String: return std::hash<std::string>{}(v.as_string()->str());
        default: return 0;
    }
}

} // namespace pho
