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

// MARK: - String Extras

func phoUppercase(_ a: PhoValue) -> PhoValue { .string(a.asString.uppercased()) }
func phoLowercase(_ a: PhoValue) -> PhoValue { .string(a.asString.lowercased()) }

func phoStringContains(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    .boolean(a.asString.contains(b.asString))
}

func phoStringSplit(_ a: PhoValue, _ sep: PhoValue) -> PhoValue {
    let parts = a.asString.components(separatedBy: sep.asString)
    return .list(parts.map { .string($0) })
}

func phoStringReplace(_ a: PhoValue, _ old: PhoValue, _ new: PhoValue) -> PhoValue {
    .string(a.asString.replacingOccurrences(of: old.asString, with: new.asString))
}

func phoStringTrim(_ a: PhoValue) -> PhoValue {
    .string(a.asString.trimmingCharacters(in: .whitespacesAndNewlines))
}

func phoSubstring(_ a: PhoValue, _ start: PhoValue, _ len: PhoValue) -> PhoValue {
    let s = a.asString
    let st = Int(start.asInteger)
    let ln = Int(len.asInteger)
    guard st >= 0 && st < s.count else { return .string("") }
    let startIdx = s.index(s.startIndex, offsetBy: st)
    let endIdx = s.index(startIdx, offsetBy: Swift.min(ln, s.count - st))
    return .string(String(s[startIdx..<endIdx]))
}

func phoStringSearch(_ a: PhoValue, _ needle: PhoValue) -> PhoValue {
    let s = a.asString, n = needle.asString
    guard let range = s.range(of: n) else { return .integer(-1) }
    return .integer(Int64(s.distance(from: s.startIndex, to: range.lowerBound)))
}

func phoCharAt(_ a: PhoValue, _ idx: PhoValue) -> PhoValue {
    let s = a.asString
    let i = Int(idx.asInteger)
    guard i >= 0 && i < s.count else { return .string("") }
    return .string(String(s[s.index(s.startIndex, offsetBy: i)]))
}

// MARK: - List Extras

func phoListFirst(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list, !arr.isEmpty else { return .null }
    return arr[0]
}

func phoListRest(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list, arr.count > 1 else { return .list([]) }
    return .list(Array(arr.dropFirst()))
}

func phoListReverse(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .list([]) }
    return .list(arr.reversed())
}

func phoListSort(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .list([]) }
    return .list(arr.sorted { $0.asReal < $1.asReal })
}

func phoListContains(_ list: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .boolean(false) }
    for item in arr {
        if case .boolean(true) = phoEqual(item, val) { return .boolean(true) }
    }
    return .boolean(false)
}

func phoListEmpty(_ list: PhoValue) -> PhoValue {
    guard case .list(let arr) = list else { return .boolean(true) }
    return .boolean(arr.isEmpty)
}

func phoListMap(_ list: PhoValue, _ fn: PhoValue) -> PhoValue {
    // list-map with a function value isn't straightforward in compiled mode;
    // return list unchanged as fallback (function dispatch handled differently)
    return list
}

// MARK: - Dict

func phoDictCreate() -> PhoValue { .dict([:]) }

func phoDictGet(_ d: PhoValue, _ key: PhoValue) -> PhoValue {
    guard case .dict(let dict) = d else { return .error("dict-get: not a dict") }
    return dict[key.asString] ?? .null
}

func phoDictSet(_ d: PhoValue, _ key: PhoValue, _ val: PhoValue) -> PhoValue {
    guard case .dict(var dict) = d else { return .error("dict-set: not a dict") }
    dict[key.asString] = val
    return .dict(dict)
}

func phoDictHas(_ d: PhoValue, _ key: PhoValue) -> PhoValue {
    guard case .dict(let dict) = d else { return .boolean(false) }
    return .boolean(dict[key.asString] != nil)
}

func phoDictRemove(_ d: PhoValue, _ key: PhoValue) -> PhoValue {
    guard case .dict(var dict) = d else { return .error("dict-remove: not a dict") }
    dict.removeValue(forKey: key.asString)
    return .dict(dict)
}

func phoDictKeys(_ d: PhoValue) -> PhoValue {
    guard case .dict(let dict) = d else { return .list([]) }
    return .list(dict.keys.sorted().map { .string($0) })
}

func phoDictValues(_ d: PhoValue) -> PhoValue {
    guard case .dict(let dict) = d else { return .list([]) }
    return .list(dict.keys.sorted().map { dict[$0]! })
}

func phoDictMerge(_ a: PhoValue, _ b: PhoValue) -> PhoValue {
    guard case .dict(var da) = a else { return b }
    guard case .dict(let db) = b else { return a }
    for (k, v) in db { da[k] = v }
    return .dict(da)
}

func phoDictSize(_ d: PhoValue) -> PhoValue {
    guard case .dict(let dict) = d else { return .integer(0) }
    return .integer(Int64(dict.count))
}

// MARK: - Math Extras

func phoPower(_ base: PhoValue, _ exp: PhoValue) -> PhoValue {
    .real(pow(base.asReal, exp.asReal))
}

func phoSin(_ a: PhoValue) -> PhoValue { .real(sin(a.asReal)) }
func phoCos(_ a: PhoValue) -> PhoValue { .real(cos(a.asReal)) }
func phoTan(_ a: PhoValue) -> PhoValue { .real(tan(a.asReal)) }
func phoAtan2(_ y: PhoValue, _ x: PhoValue) -> PhoValue { .real(atan2(y.asReal, x.asReal)) }
func phoPi() -> PhoValue { .real(Double.pi) }
func phoRandom() -> PhoValue { .real(Double.random(in: 0..<1)) }

// MARK: - Error

func phoErrorCreate(_ msg: PhoValue) -> PhoValue { .error(msg.asString) }
func phoErrorMessage(_ e: PhoValue) -> PhoValue {
    if case .error(let msg) = e { return .string(msg) }
    return .string("")
}
func phoIsError(_ a: PhoValue) -> PhoValue {
    if case .error = a { return .boolean(true) }
    return .boolean(false)
}

// MARK: - I/O

/// Shared console output buffer for standalone apps
var phoConsoleOutput: String = ""

func phoPrint(_ a: PhoValue) -> PhoValue {
    let text = a.asString
    phoConsoleOutput += text + "\n"
    print(text)
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
