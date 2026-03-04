import Foundation
import Combine

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

    let bridge = PhographBridge()
    private var projectJSON: String?

    init() {}

    // MARK: - New Project

    func newProject() {
        let json = Self.emptyProjectJSON(name: "Untitled")
        loadProject(json: json, from: nil)
        statusMessage = "New project created"
    }

    /// Minimal valid project JSON with a "main" method containing (3 + 4) * 2
    static func emptyProjectJSON(name: String) -> String {
        return """
        {
          "name": "\(name)",
          "sections": [
            {
              "name": "Main",
              "methods": [
                {
                  "name": "main",
                  "num_inputs": 0,
                  "num_outputs": 1,
                  "cases": [
                    {
                      "nodes": [
                        { "id": 1, "type": "input_bar", "num_inputs": 0, "num_outputs": 0 },
                        { "id": 2, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 3 } },
                        { "id": 3, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 4 } },
                        { "id": 4, "type": "primitive", "name": "+", "num_inputs": 2, "num_outputs": 1 },
                        { "id": 5, "type": "constant", "num_inputs": 0, "num_outputs": 1, "value": { "type": "integer", "value": 2 } },
                        { "id": 6, "type": "primitive", "name": "*", "num_inputs": 2, "num_outputs": 1 },
                        { "id": 7, "type": "output_bar", "num_inputs": 1, "num_outputs": 0 }
                      ],
                      "wires": [
                        { "source_node": 2, "source_pin": 0, "target_node": 4, "target_pin": 0 },
                        { "source_node": 3, "source_pin": 0, "target_node": 4, "target_pin": 1 },
                        { "source_node": 4, "source_pin": 0, "target_node": 6, "target_pin": 0 },
                        { "source_node": 5, "source_pin": 0, "target_node": 6, "target_pin": 1 },
                        { "source_node": 6, "source_pin": 0, "target_node": 7, "target_pin": 0 }
                      ],
                      "input_bar_id": 1,
                      "output_bar_id": 7
                    }
                  ]
                }
              ]
            }
          ]
        }
        """
    }

    // MARK: - Load / Save

    func loadProject(json: String, from url: URL? = nil) {
        projectJSON = json
        let model = ProjectModel(name: url?.deletingPathExtension().lastPathComponent ?? "Untitled")
        _ = model.loadFromJSON(json)
        model.filePath = url
        self.project = model

        do {
            try bridge.loadProjectJSON(json)
            statusMessage = url != nil ? "Opened \(model.name)" : "Project loaded"
        } catch {
            consoleOutput += "Error loading project: \(error.localizedDescription)\n"
            statusMessage = "Load failed"
            return
        }

        // Auto-select first method
        if let firstMethod = model.sections.first?.methods.first {
            selectMethod(name: firstMethod.name, caseIndex: 0)
        }
    }

    func saveProject(to url: URL) {
        guard let json = projectJSON else { return }
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
            currentGraph = GraphModel.fromCaseJSON(
                method.casesRaw[caseIndex],
                methodName: name,
                caseIndex: caseIndex
            )
        } else {
            let graph = GraphModel()
            graph.methodName = name
            graph.caseIndex = caseIndex
            currentGraph = graph
        }
    }

    func runMethod(name: String) {
        isRunning = true
        statusMessage = "Running \(name)..."

        do {
            let result = try bridge.callMethod(name, inputsJSON: nil)
            consoleOutput += "> \(name): \(result)\n"
        } catch {
            consoleOutput += "> \(name): error - \(error.localizedDescription)\n"
        }
        isRunning = false
        statusMessage = "Done"
    }

    func runCurrentMethod() {
        guard let name = selectedMethodName else {
            consoleOutput += "No method selected\n"
            return
        }
        runMethod(name: name)
    }

    /// Get all primitive names for fuzzy finder
    func allPrimitiveNames() -> [String] {
        return [
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
    }
}
