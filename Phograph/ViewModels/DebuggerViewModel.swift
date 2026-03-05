import Foundation
import Combine

struct CallStackFrame: Identifiable {
    let id = UUID()
    let methodName: String
    let caseIndex: Int
    let stepNumber: Int
}

/// Manages debugger state and execution control.
class DebuggerViewModel: ObservableObject {
    @Published var isDebugging: Bool = false
    @Published var isPaused: Bool = false
    @Published var currentStep: Int = 0
    @Published var callStack: [CallStackFrame] = []
    @Published var wireTraceValues: [WireTraceValue] = []
    @Published var canRollback: Bool = false
    @Published var breakpointNodeIds: Set<String> = []

    func startDebugging() {
        isDebugging = true
        isPaused = false
        currentStep = 0
        callStack = []
        wireTraceValues = []
    }

    func continueExecution() {
        isPaused = false
    }

    func stepOver() {
        // Will resume for one step then pause again
        isPaused = false
    }

    func stepInto() {
        // Step into method calls
        isPaused = false
    }

    func rollback() {
        guard canRollback else { return }
        // Roll back to previous snapshot
        isPaused = true
    }

    func stop() {
        isDebugging = false
        isPaused = false
        currentStep = 0
        callStack = []
        wireTraceValues = []
    }

    func addBreakpoint(nodeId: String) {
        breakpointNodeIds.insert(nodeId)
    }

    func removeBreakpoint(nodeId: String) {
        breakpointNodeIds.remove(nodeId)
    }

    func toggleBreakpoint(nodeId: String) {
        if breakpointNodeIds.contains(nodeId) {
            breakpointNodeIds.remove(nodeId)
        } else {
            breakpointNodeIds.insert(nodeId)
        }
    }

    // Called by the engine bridge when a breakpoint is hit
    func onBreakpointHit(nodeId: Int, step: Int) {
        isPaused = true
        currentStep = step
    }

    // Called by the engine bridge with trace data
    func updateTraceValues(_ traces: [WireTraceValue]) {
        wireTraceValues = traces
    }

    func pushCallFrame(methodName: String, caseIndex: Int) {
        callStack.append(CallStackFrame(
            methodName: methodName,
            caseIndex: caseIndex,
            stepNumber: currentStep
        ))
    }

    func popCallFrame() {
        if !callStack.isEmpty {
            callStack.removeLast()
        }
    }
}
