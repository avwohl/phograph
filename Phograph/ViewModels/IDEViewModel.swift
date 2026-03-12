import Foundation
import Combine
import UniformTypeIdentifiers
import UIKit

/// Orchestrates the IDE: project loading, engine calls, state coordination.
class IDEViewModel: ObservableObject {
    @Published var project: ProjectModel?
    @Published var currentGraph: GraphModel?
    @Published var isRunning: Bool = false
    @Published var consoleOutput: String = ""
    @Published var statusMessage: String = "Ready"
    @Published var showFuzzyFinder: Bool = false
    @Published var selectedMethodName: String?
    @Published var selectedCaseIndex: Int = 0

    /// Panel visibility
    @Published var showSidebar: Bool = true
    @Published var showInspector: Bool = true
    @Published var showConsole: Bool = true
    @Published var showExampleBrowser: Bool = false
    @Published var showLibraryManager: Bool = false
    @Published var showExportSheet: Bool = false
    @Published var showCanvasOutput: Bool = false
    @Published var canvasOutputImage: UIImage?
    @Published var showFrontPanel: Bool = false
    @Published var frontPanelClassName: String = ""
    @Published var missingLibraries: [LibraryReference] = []
    @Published var showSavePanel: Bool = false
    @Published var showPDFExport: Bool = false
    @Published var showSVGExport: Bool = false

    /// Debugger
    @Published var debugger = DebuggerViewModel()
    @Published var isDebugging: Bool = false
    @Published var pausedNodeId: UInt32?

    /// Zoom request dispatched to canvas
    @Published var zoomRequest: ZoomRequest?

    enum ZoomRequest {
        case zoomIn, zoomOut, fitToWindow
    }

    /// Incremented whenever the graph content changes (nodes/wires added/removed).
    /// Views can observe this to trigger re-render.
    @Published var graphRevision: Int = 0

    let bridge = PhographBridge()
    let libraryManager = LibraryManager()
    var projectJSON: String?
    private var graphCancellable: AnyCancellable?

    init() {
        // Wire up debug event handler
        bridge.debugEventHandler = { [weak self] eventJSON in
            guard let self = self else { return }
            self.handleDebugEvent(eventJSON)
        }
    }

    /// Currently selected method model
    var currentMethod: MethodModel? {
        guard let name = selectedMethodName else { return nil }
        return project?.findMethod(name)
    }

    /// Subscribe to graph changes so the view re-renders when nodes/wires change
    private func observeGraph(_ graph: GraphModel?) {
        graphCancellable?.cancel()
        guard let graph = graph else { graphCancellable = nil; return }
        graphCancellable = graph.objectWillChange.sink { [weak self] _ in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.graphRevision += 1
                print("[OBSERVE] graphRevision now: \(self.graphRevision)")
            }
        }
    }

    // MARK: - New Project

    func newProject() {
        let json = Self.emptyProjectJSON(name: "Untitled")
        loadProject(json: json, from: nil)
        statusMessage = "New project created"
    }

    /// Minimal valid project JSON with an empty "main" method
    static func emptyProjectJSON(name: String) -> String {
        return """
        {
          "name": "\(name)",
          "sections": [
            {
              "name": "Main",
              "methods": []
            }
          ]
        }
        """
    }

    // MARK: - Examples

    func loadExample(_ example: ExampleEntry) {
        loadProject(json: example.projectJSON, from: nil)
        statusMessage = "Loaded example: \(example.name)"
    }

    // MARK: - Load / Save

    func loadProject(json: String, from url: URL? = nil) {
        projectJSON = json
        let model = ProjectModel(name: url?.deletingPathExtension().lastPathComponent ?? "Untitled")
        do {
            try model.loadFromJSON(json)
        } catch {
            consoleOutput += "Error parsing project: \(error.localizedDescription)\n"
            statusMessage = "Parse failed"
            showConsole = true
        }
        model.filePath = url
        self.project = model

        do {
            try bridge.loadProjectJSON(json)
            statusMessage = url != nil ? "Opened \(model.name)" : "Project loaded"
        } catch {
            consoleOutput += "Error loading project: \(error.localizedDescription)\n"
            statusMessage = "Load failed"
            showConsole = true
            return
        }

        // Resolve library references
        missingLibraries = libraryManager.resolveReferences(model.libraryReferences)

        // Auto-select first method
        if let firstMethod = model.sections.first?.methods.first {
            selectMethod(name: firstMethod.name, caseIndex: 0)
        }
    }

    func saveProject(to url: URL) {
        guard let json = projectJSON else {
            consoleOutput += "Nothing to save — no project data loaded\n"
            statusMessage = "Save failed"
            return
        }
        do {
            try json.write(to: url, atomically: true, encoding: .utf8)
            project?.filePath = url
            project?.name = url.deletingPathExtension().lastPathComponent
            project?.isDirty = false
            statusMessage = "Saved"
        } catch {
            consoleOutput += "Save error: \(error.localizedDescription)\n"
            statusMessage = "Save failed"
        }
    }

    // MARK: - Method Selection / Execution

    func selectMethod(name: String, caseIndex: Int) {
        selectedMethodName = name
        selectedCaseIndex = caseIndex

        // Build graph from project data
        if let method = project?.findMethod(name),
           caseIndex < method.casesRaw.count {
            let graph = GraphModel.fromCaseJSON(
                method.casesRaw[caseIndex],
                methodName: name,
                caseIndex: caseIndex
            )
            currentGraph = graph
            observeGraph(graph)
        } else {
            let graph = GraphModel()
            graph.methodName = name
            graph.caseIndex = caseIndex
            currentGraph = graph
            observeGraph(graph)
        }
    }

    func runMethod(name: String) {
        isRunning = true
        statusMessage = "Running \(name)..."

        do {
            let resultStr = try bridge.callMethod(name, inputsJSON: nil)
            // Parse the JSON result
            guard let data = resultStr.data(using: .utf8) else {
                consoleOutput += "> \(name): \(resultStr)\n"
                statusMessage = "Done"
                isRunning = false
                return
            }
            do {
                guard let json = try JSONSerialization.jsonObject(with: data) as? [String: Any] else {
                    consoleOutput += "> \(name): \(resultStr)\n"
                    statusMessage = "Done"
                    isRunning = false
                    return
                }

                let status = json["status"] as? String ?? "unknown"

                // Append console output from engine
                if let console = json["console"] as? String, !console.isEmpty {
                    consoleOutput += console
                }

                // Show error message if present
                if let errorMsg = json["error"] as? String, !errorMsg.isEmpty {
                    consoleOutput += "Error in \(name): \(errorMsg)\n"
                    showConsole = true
                }

                // Show outputs
                if let outputs = json["outputs"] as? [Any], !outputs.isEmpty {
                    let outputStrs = outputs.map { val -> String in
                        if let s = val as? String { return s }
                        if let n = val as? NSNumber { return n.stringValue }
                        if val is NSNull { return "null" }
                        return "\(val)"
                    }
                    consoleOutput += "> \(name): \(outputStrs.joined(separator: ", "))\n"
                }

                statusMessage = status == "success" ? "Done" : "Done (\(status))"

                // Check if canvas output was produced — capture image now before buffer is invalidated
                if bridge.bufferWidth > 0 && bridge.bufferHeight > 0 {
                    canvasOutputImage = captureCanvasImage()
                    showCanvasOutput = true
                }
            } catch {
                consoleOutput += "> \(name): result parse error - \(error.localizedDescription)\n"
                consoleOutput += "> \(name): raw output: \(resultStr)\n"
                statusMessage = "Done (parse error)"
                showConsole = true
            }
        } catch {
            consoleOutput += "> \(name): error - \(error.localizedDescription)\n"
            statusMessage = "Error"
            showConsole = true
        }
        isRunning = false
    }

    func runCurrentMethod() {
        guard let name = selectedMethodName else {
            consoleOutput += "No method selected\n"
            return
        }
        runMethod(name: name)
    }

    /// Capture the current pixel buffer as a UIImage (BGRA → RGBA conversion).
    private func captureCanvasImage() -> UIImage? {
        var w: Int32 = 0
        var h: Int32 = 0
        guard let ptr = bridge.pixelBufferWidth(&w, height: &h),
              w > 0, h > 0 else { return nil }

        let width = Int(w)
        let height = Int(h)
        let bytesPerRow = width * 4
        let totalBytes = bytesPerRow * height

        guard let ctx = CGContext(
            data: nil,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: bytesPerRow,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
        ), let dest = ctx.data else { return nil }

        let destPtr = dest.bindMemory(to: UInt8.self, capacity: totalBytes)
        let src = UnsafeRawPointer(ptr).bindMemory(to: UInt8.self, capacity: totalBytes)
        for i in 0..<(width * height) {
            let si = i * 4
            destPtr[si + 0] = src[si + 2]  // R ← B
            destPtr[si + 1] = src[si + 1]  // G
            destPtr[si + 2] = src[si + 0]  // B ← R
            destPtr[si + 3] = src[si + 3]  // A
        }

        guard let cgImage = ctx.makeImage() else { return nil }
        return UIImage(cgImage: cgImage)
    }

    // MARK: - Edit Actions

    func deleteSelected() {
        guard let graph = currentGraph else { return }
        for id in graph.selectedNodeIds {
            graph.removeNode(id: id)
        }
        for id in graph.selectedWireIds {
            graph.removeWire(id: id)
        }
        graph.selectedNodeIds.removeAll()
        graph.selectedWireIds.removeAll()
    }

    func selectAll() {
        guard let graph = currentGraph else { return }
        graph.selectedNodeIds = Set(graph.nodes.map { $0.id })
    }

    func duplicateSelected() {
        guard let graph = currentGraph else { return }
        var newIds: Set<UUID> = []
        for id in graph.selectedNodeIds {
            guard let node = graph.nodes.first(where: { $0.id == id }) else { continue }
            let copy = GraphNodeModel(x: node.x + 30, y: node.y + 30, label: node.label, nodeType: node.nodeType)
            copy.inputPins = node.inputPins.map { PinModel(name: $0.name, index: $0.index) }
            copy.outputPins = node.outputPins.map { PinModel(name: $0.name, index: $0.index) }
            copy.constantValue = node.constantValue
            copy.width = node.width
            copy.height = node.height
            copy.annotation = node.annotation
            copy.libraryName = node.libraryName
            graph.addNode(copy)
            newIds.insert(copy.id)
        }
        graph.selectedNodeIds = newIds
    }

    // MARK: - Execution

    func stopExecution() {
        if isDebugging {
            debugStop()
        }
        isRunning = false
        statusMessage = "Stopped"
    }

    func clearConsole() {
        consoleOutput = ""
    }

    // MARK: - Debug

    func debugRun() {
        guard let name = selectedMethodName else {
            consoleOutput += "No method selected\n"
            return
        }
        isDebugging = true
        isRunning = true
        debugger.startDebugging()
        statusMessage = "Debugging \(name)..."
        consoleOutput += "> Debug: \(name)\n"
        bridge.debugRun(name)
    }

    func debugContinue() {
        debugger.continueExecution()
        pausedNodeId = nil
        bridge.debugContinue()
    }

    func debugStepOver() {
        debugger.stepOver()
        pausedNodeId = nil
        bridge.debugStepOver()
    }

    func debugStepInto() {
        debugger.isPaused = false
        pausedNodeId = nil
        bridge.debugStepInto()
    }

    func debugStop() {
        bridge.debugStop()
        debugger.stop()
        isDebugging = false
        isRunning = false
        pausedNodeId = nil
        statusMessage = "Stopped"
    }

    func toggleBreakpoint(nodeId: UInt32) {
        let idStr = String(nodeId)
        if debugger.breakpointNodeIds.contains(idStr) {
            debugger.removeBreakpoint(nodeId: idStr)
            bridge.debugRemoveBreakpoint(nodeId)
        } else {
            let method = selectedMethodName ?? ""
            debugger.addBreakpoint(nodeId: idStr)
            bridge.debugAddBreakpoint(nodeId, method: method, caseIndex: Int32(selectedCaseIndex))
        }
    }

    private func handleDebugEvent(_ jsonStr: String) {
        guard let data = jsonStr.data(using: .utf8) else {
            consoleOutput += "Debug: received non-UTF8 event data\n"
            return
        }
        let json: [String: Any]
        do {
            guard let parsed = try JSONSerialization.jsonObject(with: data) as? [String: Any] else {
                consoleOutput += "Debug: event was not a JSON object\n"
                return
            }
            json = parsed
        } catch {
            consoleOutput += "Debug: failed to parse event - \(error.localizedDescription)\n"
            return
        }

        let event = json["event"] as? String ?? ""

        switch event {
        case "paused":
            let nodeId = json["node_id"] as? UInt32 ?? 0
            let step = json["step"] as? Int ?? 0
            pausedNodeId = nodeId
            debugger.onBreakpointHit(nodeId: Int(nodeId), step: step)

            // Parse trace values for wire overlay
            if let traces = json["traces"] as? [[String: Any]] {
                var wireTraces: [WireTraceValue] = []
                for t in traces {
                    // We'd need node positions to compute midpoints -- use placeholder
                    let val = t["val"] as? String ?? "?"
                    let src = t["src"] as? Int ?? 0
                    let dst = t["dst"] as? Int ?? 0
                    wireTraces.append(WireTraceValue(
                        midX: CGFloat(src * 50 + dst * 50) / 2,
                        midY: 100,
                        displayValue: val
                    ))
                }
                debugger.updateTraceValues(wireTraces)
            }

            statusMessage = "Paused at node \(nodeId), step \(step)"

        case "completed":
            let status = json["status"] as? String ?? "unknown"
            isDebugging = false
            isRunning = false
            debugger.stop()
            pausedNodeId = nil

            // Grab any console output
            if let console = bridge.consoleOutput, !console.isEmpty {
                consoleOutput += console
            }

            // Show error message if present
            if let errorMsg = json["error"] as? String, !errorMsg.isEmpty {
                consoleOutput += "Error: \(errorMsg)\n"
                showConsole = true
            }

            statusMessage = "Debug completed (\(status))"

        default:
            consoleOutput += "Debug: unknown event '\(event)'\n"
        }
    }

    // MARK: - Class/Method Creation

    /// Find the class that owns the currently selected method, if any
    var selectedMethodOwnerClass: String? {
        guard let name = selectedMethodName else { return nil }
        for section in project?.sections ?? [] {
            for classDef in section.classes {
                if classDef.methods.contains(where: { $0.name == name }) {
                    return classDef.name
                }
            }
        }
        return nil
    }

    @discardableResult
    func addClass(in section: SectionModel) -> ClassModel {
        var baseName = "NewClass"
        var counter = 1
        let existingNames = Set(section.classes.map { $0.name })
        while existingNames.contains(baseName) {
            counter += 1
            baseName = "NewClass\(counter)"
        }
        let newClass = ClassModel(name: baseName)
        section.classes.append(newClass)
        project?.isDirty = true
        graphRevision += 1
        return newClass
    }

    func renameClass(_ classDef: ClassModel, to newName: String) {
        classDef.name = newName
        project?.isDirty = true
        graphRevision += 1
    }

    func addMethod(to classDef: ClassModel) {
        var baseName = "newMethod"
        var counter = 1
        let existingNames = Set(classDef.methods.map { $0.name })
        while existingNames.contains(baseName) {
            counter += 1
            baseName = "newMethod\(counter)"
        }
        let method = MethodModel(name: baseName, numInputs: 1, numOutputs: 1)
        method.caseCount = 1
        method.casesRaw = [[:]]
        classDef.methods.append(method)
        project?.isDirty = true
        graphRevision += 1
        selectMethod(name: baseName, caseIndex: 0)
    }

    func addAttribute(to classDef: ClassModel) {
        var baseName = "newAttr"
        var counter = 1
        let existingNames = Set(classDef.attributes.map { $0.name })
        while existingNames.contains(baseName) {
            counter += 1
            baseName = "newAttr\(counter)"
        }
        let attr = ClassAttributeModel(name: baseName, defaultValue: "")
        classDef.attributes.append(attr)
        classDef.isExpanded = true
        project?.isDirty = true
        graphRevision += 1
    }

    func addTopLevelMethod(in section: SectionModel) {
        var baseName = "newMethod"
        var counter = 1
        let existingNames = Set(section.methods.map { $0.name })
        while existingNames.contains(baseName) {
            counter += 1
            baseName = "newMethod\(counter)"
        }
        let method = MethodModel(name: baseName, numInputs: 0, numOutputs: 0)
        method.caseCount = 1
        method.casesRaw = [[:]]
        section.methods.append(method)
        project?.isDirty = true
        graphRevision += 1
        selectMethod(name: baseName, caseIndex: 0)
    }

    // MARK: - Zoom

    func zoomIn() { zoomRequest = .zoomIn }
    func zoomOut() { zoomRequest = .zoomOut }
    func fitToWindow() { zoomRequest = .fitToWindow }

    // MARK: - Export

    func exportPDF(to url: URL) {
        guard let graph = currentGraph else { return }
        do {
            try GraphExporter.exportPDF(graph: graph, to: url)
            statusMessage = "Exported PDF"
        } catch {
            consoleOutput += "PDF export error: \(error.localizedDescription)\n"
            statusMessage = "Export failed"
        }
    }

    func exportSVG(to url: URL) {
        guard let graph = currentGraph else { return }
        let svgString = GraphExporter.exportSVG(graph: graph)
        do {
            try svgString.write(to: url, atomically: true, encoding: .utf8)
            statusMessage = "Exported SVG"
        } catch {
            consoleOutput += "SVG export error: \(error.localizedDescription)\n"
            statusMessage = "Export failed"
        }
    }

    // MARK: - Case Navigation

    func nextCase() {
        guard let method = currentMethod else { return }
        let next = selectedCaseIndex + 1
        if next < method.caseCount {
            selectMethod(name: method.name, caseIndex: next)
        }
    }

    func prevCase() {
        guard let method = currentMethod else { return }
        let prev = selectedCaseIndex - 1
        if prev >= 0 {
            selectMethod(name: method.name, caseIndex: prev)
        }
    }

    /// Get all primitive names for fuzzy finder (built-in + library)
    func allPrimitiveNames() -> [String] {
        var names: [String] = [
            "+", "-", "*", "/", "=", "<", ">", "<=", ">=", "!=",
            "and", "or", "not",
            "concat", "length", "to-string",
            "get-nth", "append", "sort", "empty?",
            "dict-create", "dict-get", "dict-set",
            "if", "log", "inspect",
            "shape-rect", "shape-oval", "shape-group",
            "shape-set-fill", "shape-add-child",
            "create-canvas", "canvas-render",
        ]
        // Append library primitives
        for lib in libraryManager.libraries {
            for prim in lib.manifest.primitives {
                if !names.contains(prim.name) {
                    names.append(prim.name)
                }
            }
        }
        // Append class-derived names
        if let sections = project?.sections {
            for section in sections {
                for classDef in section.classes {
                    names.append("new \(classDef.name)")
                    for attr in classDef.attributes {
                        let getName = "get \(attr.name)"
                        let setName = "set \(attr.name)"
                        if !names.contains(getName) { names.append(getName) }
                        if !names.contains(setName) { names.append(setName) }
                    }
                    for method in classDef.methods {
                        names.append("\(classDef.name)/\(method.name)")
                    }
                }
                // Universal methods
                for method in section.methods {
                    let mName = method.name
                    if !names.contains(mName) { names.append(mName) }
                }
            }
        }
        return names
    }

    /// Look up a library primitive descriptor by name
    func libraryPrimitive(named name: String) -> (library: LibraryInfo, primitive: LibraryPrimitiveDescriptor)? {
        for lib in libraryManager.libraries {
            if let prim = lib.manifest.primitives.first(where: { $0.name == name }) {
                return (lib, prim)
            }
        }
        return nil
    }
}
