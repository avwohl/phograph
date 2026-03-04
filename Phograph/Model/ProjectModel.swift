import Foundation

/// Swift-side representation of a Phograph project for UI binding.
class ProjectModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var sections: [SectionModel] = []
    @Published var filePath: URL?
    @Published var isDirty: Bool = false

    init(name: String = "Untitled") {
        self.name = name
    }

    /// Load project from JSON string
    func loadFromJSON(_ json: String) -> Bool {
        guard let data = json.data(using: .utf8),
              let root = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let sectionsArray = root["sections"] as? [[String: Any]] else {
            return false
        }

        var newSections: [SectionModel] = []
        for sectionDict in sectionsArray {
            let section = SectionModel(
                name: sectionDict["name"] as? String ?? "Untitled"
            )
            if let methods = sectionDict["methods"] as? [[String: Any]] {
                for methodDict in methods {
                    let method = MethodModel(
                        name: methodDict["name"] as? String ?? "unnamed",
                        numInputs: methodDict["num_inputs"] as? Int ?? 0,
                        numOutputs: methodDict["num_outputs"] as? Int ?? 0
                    )
                    if let cases = methodDict["cases"] as? [[String: Any]] {
                        method.caseCount = cases.count
                        method.casesRaw = cases
                    }
                    section.methods.append(method)
                }
            }
            if let classes = sectionDict["classes"] as? [[String: Any]] {
                for classDict in classes {
                    let classDef = ClassModel(
                        name: classDict["name"] as? String ?? "unnamed",
                        parentName: classDict["parent"] as? String
                    )
                    if let attrs = classDict["attributes"] as? [[String: Any]] {
                        classDef.attributes = attrs.map {
                            $0["name"] as? String ?? ""
                        }
                    }
                    section.classes.append(classDef)
                }
            }
            newSections.append(section)
        }

        self.sections = newSections
        self.isDirty = false
        return true
    }

    /// Find a method model by name
    func findMethod(_ name: String) -> MethodModel? {
        for section in sections {
            if let m = section.methods.first(where: { $0.name == name }) {
                return m
            }
        }
        return nil
    }
}

class SectionModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var methods: [MethodModel] = []
    @Published var classes: [ClassModel] = []
    @Published var isExpanded: Bool = true

    init(name: String) {
        self.name = name
    }
}

class MethodModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var numInputs: Int
    @Published var numOutputs: Int
    @Published var caseCount: Int = 1

    /// Raw JSON dicts for each case (nodes + wires)
    var casesRaw: [[String: Any]] = []

    init(name: String, numInputs: Int = 0, numOutputs: Int = 0) {
        self.name = name
        self.numInputs = numInputs
        self.numOutputs = numOutputs
    }
}

class ClassModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var parentName: String?
    @Published var attributes: [String] = []
    @Published var isExpanded: Bool = false

    init(name: String, parentName: String? = nil) {
        self.name = name
        self.parentName = parentName
    }
}
