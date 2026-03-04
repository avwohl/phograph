// PhographRuntime.swift
// Standalone runtime library for compiled Phograph programs.
// Include this file alongside generated Swift source to build a native binary.

import Foundation

// MARK: - Core Value Type

enum PhoValue {
    case null
    case integer(Int64)
    case real(Double)
    case boolean(Bool)
    case string(String)
    case list([PhoValue])
    case dict([String: PhoValue])
    case object(PhoObject)
    case error(String)

    var isTruthy: Bool {
        switch self {
        case .null: return false
        case .boolean(let b): return b
        case .integer(let i): return i != 0
        case .string(let s): return !s.isEmpty
        case .list(let l): return !l.isEmpty
        case .error: return false
        default: return true
        }
    }

    var asInteger: Int64 {
        switch self {
        case .integer(let i): return i
        case .real(let r): return Int64(r)
        case .boolean(let b): return b ? 1 : 0
        default: return 0
        }
    }

    var asReal: Double {
        switch self {
        case .real(let r): return r
        case .integer(let i): return Double(i)
        default: return 0
        }
    }

    var asString: String {
        switch self {
        case .string(let s): return s
        case .integer(let i): return "\(i)"
        case .real(let r): return "\(r)"
        case .boolean(let b): return b ? "true" : "false"
        case .null: return "null"
        case .list(let arr):
            let inner = arr.map { $0.asString }.joined(separator: ", ")
            return "[\(inner)]"
        case .dict(let d):
            let inner = d.map { "\($0.key): \($0.value.asString)" }.joined(separator: ", ")
            return "{\(inner)}"
        case .object(let o): return "<\(o.className)>"
        case .error(let msg): return "error: \(msg)"
        }
    }

    var asBool: Bool {
        switch self {
        case .boolean(let b): return b
        default: return isTruthy
        }
    }
}

// MARK: - Object

class PhoObject {
    var className: String
    var attrs: [String: PhoValue] = [:]
    init(_ name: String) { className = name }
}

// MARK: - Arithmetic

func phoAdd(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x &+ y)
    case (.real(let x), .real(let y)): return .real(x + y)
    case (.integer(let x), .real(let y)): return .real(Double(x) + y)
    case (.real(let x), .integer(let y)): return .real(x + Double(y))
    case (.string(let x), .string(let y)): return .string(x + y)
    default: return .error("add: type mismatch")
    }
}

func phoSub(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x &- y)
    case (.real(let x), .real(let y)): return .real(x - y)
    case (.integer(let x), .real(let y)): return .real(Double(x) - y)
    case (.real(let x), .integer(let y)): return .real(x - Double(y))
    default: return .error("sub: type mismatch")
    }
}

func phoMul(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(x &* y)
    case (.real(let x), .real(let y)): return .real(x * y)
    case (.integer(let x), .real(let y)): return .real(Double(x) * y)
    case (.real(let x), .integer(let y)): return .real(x * Double(y))
    default: return .error("mul: type mismatch")
    }
}

func phoDiv(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)):
        if y == 0 { return .error("division by zero") }
        return .integer(x / y)
    case (.real(let x), .real(let y)):
        if y == 0 { return .error("division by zero") }
        return .real(x / y)
    case (.integer(let x), .real(let y)): return .real(Double(x) / y)
    case (.real(let x), .integer(let y)): return .real(x / Double(y))
    default: return .error("div: type mismatch")
    }
}

func phoMod(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)):
        if y == 0 { return .error("mod by zero") }
        return .integer(x % y)
    default: return .error("mod: type mismatch")
    }
}

func phoAbs(_ a: PhoValue) -> PhoValue {
    switch a {
    case .integer(let x): return .integer(abs(x))
    case .real(let x): return .real(abs(x))
    default: return .error("abs: type mismatch")
    }
}

func phoNegate(_ a: PhoValue) -> PhoValue {
    switch a {
    case .integer(let x): return .integer(-x)
    case .real(let x): return .real(-x)
    default: return .error("negate: type mismatch")
    }
}

func phoSqrt(_ a: PhoValue) -> PhoValue { .real(sqrt(a.asReal)) }
func phoRound(_ a: PhoValue) -> PhoValue { .integer(Int64(a.asReal.rounded())) }
func phoFloor(_ a: PhoValue) -> PhoValue { .integer(Int64(floor(a.asReal))) }
func phoCeil(_ a: PhoValue) -> PhoValue { .integer(Int64(ceil(a.asReal))) }

func phoClamp(_ v: PhoValue, _ lo: PhoValue, _ hi: PhoValue) -> PhoValue {
    let val = v.asReal, low = lo.asReal, high = hi.asReal
    return .real(Swift.min(Swift.max(val, low), high))
}

func phoMin(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(Swift.min(x, y))
    default: return .real(Swift.min(a.asReal, b.asReal))
    }
}

func phoMax(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .integer(Swift.max(x, y))
    default: return .real(Swift.max(a.asReal, b.asReal))
    }
}

// MARK: - Comparison

func phoEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    switch (a, b) {
    case (.integer(let x), .integer(let y)): return .boolean(x == y)
    case (.real(let x), .real(let y)): return .boolean(x == y)
    case (.integer(let x), .real(let y)): return .boolean(Double(x) == y)
    case (.real(let x), .integer(let y)): return .boolean(x == Double(y))
    case (.boolean(let x), .boolean(let y)): return .boolean(x == y)
    case (.string(let x), .string(let y)): return .boolean(x == y)
    case (.null, .null): return .boolean(true)
    default: return .boolean(false)
    }
}

func phoNotEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    if case .boolean(let eq) = phoEqual(a, b) { return .boolean(!eq) }
    return .boolean(true)
}

func phoLessThan(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal < b.asReal) }
func phoGreaterThan(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal > b.asReal) }
func phoLessEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal <= b.asReal) }
func phoGreaterEqual(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asReal >= b.asReal) }

// MARK: - Logic

func phoAnd(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asBool && b.asBool) }
func phoOr(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .boolean(a.asBool || b.asBool) }
func phoNot(_ a: PhoValue) -> PhoValue { .boolean(!a.asBool) }

// MARK: - String

func phoStringConcat(_ a: PhoValue, _ b: PhoValue) -> PhoValue { .string(a.asString + b.asString) }
func phoStringLength(_ a: PhoValue) -> PhoValue { .integer(Int64(a.asString.count)) }
func phoToString(_ a: PhoValue) -> PhoValue { .string(a.asString) }

// MARK: - List

func phoListCreate(_ elems: [PhoValue]) -> PhoValue { .list(elems) }

func phoListGet(_ list: PhoValue, _ idx: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .error("list-get: not a list") }
    let i = Int(idx.asInteger)
    guard i >= 0 && i < arr.count else { return .error("list-get: index out of bounds") }
    return arr[i]
}

func phoListSet(_ list: PhoValue, _ idx: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .list(var arr) = list else { return .error("list-set: not a list") }
    let i = Int(idx.asInteger)
    guard i >= 0 && i < arr.count else { return .error("list-set: index out of bounds") }
    arr[i] = val
    return .list(arr)
}

func phoListLength(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .integer(0) }
    return .integer(Int64(arr.count))
}

func phoListAppend(_ list: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .list(var arr) = list else { return .list([val]) }
    arr.append(val)
    return .list(arr)
}

// MARK: - Type Checks

func phoIsInteger(_ a: PhoValue) -> PhoValue { if case .integer = a { return .boolean(true) }; return .boolean(false) }
func phoIsReal(_ a: PhoValue) -> PhoValue { if case .real = a { return .boolean(true) }; return .boolean(false) }
func phoIsString(_ a: PhoValue) -> PhoValue { if case .string = a { return .boolean(true) }; return .boolean(false) }
func phoIsBoolean(_ a: PhoValue) -> PhoValue { if case .boolean = a { return .boolean(true) }; return .boolean(false) }
func phoIsList(_ a: PhoValue) -> PhoValue { if case .list = a { return .boolean(true) }; return .boolean(false) }
func phoIsNull(_ a: PhoValue) -> PhoValue { if case .null = a { return .boolean(true) }; return .boolean(false) }

func phoTypeOf(_ a: PhoValue) -> PhoValue {
    switch a {
    case .null: return .string("null")
    case .integer: return .string("integer")
    case .real: return .string("real")
    case .boolean: return .string("boolean")
    case .string: return .string("string")
    case .list: return .string("list")
    case .dict: return .string("dict")
    case .object: return .string("object")
    case .error: return .string("error")
    }
}

// MARK: - I/O

func phoPrint(_ a: PhoValue) -> PhoValue {
    print(a.asString)
    return .null
}

// MARK: - Fallbacks

func phoCallPrim(_ name: String, _ args: [PhoValue]) -> PhoValue {
    print("WARNING: uncompiled primitive '\(name)' called at runtime")
    return .null
}

func phoDispatch(_ obj: PhoValue, _ method: String, _ args: [PhoValue]) -> PhoValue {
    print("WARNING: dynamic dispatch '\(method)' not compiled")
    return .null
}
