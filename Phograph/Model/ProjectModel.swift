import Foundation

/// Swift-side representation of a Phograph project for UI binding.
class ProjectModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var sections: [SectionModel] = []
    @Published var filePath: URL?
    @Published var isDirty: Bool = false
    @Published var libraryReferences: [LibraryReference] = []

    init(name: String = "Untitled") {
        self.name = name
    }

    enum LoadError: LocalizedError {
        case invalidJSON(String)
        case noSections

        var errorDescription: String? {
            switch self {
            case .invalidJSON(let detail): return "Invalid JSON: \(detail)"
            case .noSections: return "Project has no \"sections\" array"
            }
        }
    }

    /// Load project from JSON string
    func loadFromJSON(_ json: String) throws {
        guard let data = json.data(using: .utf8) else {
            throw LoadError.invalidJSON("string is not valid UTF-8")
        }
        let parsed: Any
        do {
            parsed = try JSONSerialization.jsonObject(with: data)
        } catch {
            throw LoadError.invalidJSON(error.localizedDescription)
        }
        guard let root = parsed as? [String: Any] else {
            throw LoadError.invalidJSON("root is not a JSON object")
        }

        guard let sectionsArray = root["sections"] as? [[String: Any]] else {
            throw LoadError.noSections
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
                        classDef.attributes = attrs.map { attrDict in
                            let name = attrDict["name"] as? String ?? ""
                            var defaultStr = ""
                            if let defDict = attrDict["default"] as? [String: Any],
                               let val = defDict["value"] {
                                defaultStr = "\(val)"
                            }
                            return ClassAttributeModel(name: name, defaultValue: defaultStr)
                        }
                    }
                    if let classMethods = classDict["methods"] as? [[String: Any]] {
                        for methodDict in classMethods {
                            let method = MethodModel(
                                name: methodDict["name"] as? String ?? "unnamed",
                                numInputs: methodDict["num_inputs"] as? Int ?? 0,
                                numOutputs: methodDict["num_outputs"] as? Int ?? 0
                            )
                            if let cases = methodDict["cases"] as? [[String: Any]] {
                                method.caseCount = cases.count
                                method.casesRaw = cases
                            }
                            classDef.methods.append(method)
                        }
                    }
                    section.classes.append(classDef)
                }
            }
            newSections.append(section)
        }

        self.sections = newSections

        // Parse library references
        if let libsArray = root["libraries"] as? [[String: Any]] {
            self.libraryReferences = libsArray.compactMap { dict in
                guard let name = dict["name"] as? String,
                      let version = dict["version"] as? String else { return nil }
                return LibraryReference(name: name, version: version, source: dict["source"] as? String)
            }
        } else {
            self.libraryReferences = []
        }

        self.isDirty = false
    }

    /// Find a method model by name (searches section methods and class methods)
    func findMethod(_ name: String) -> MethodModel? {
        for section in sections {
            if let m = section.methods.first(where: { $0.name == name }) {
                return m
            }
            for classDef in section.classes {
                if let m = classDef.methods.first(where: { $0.name == name }) {
                    return m
                }
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

/// Attribute info for front panel display
struct ClassAttributeModel: Identifiable {
    let id = UUID()
    let name: String
    let defaultValue: String
}

class ClassModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var parentName: String?
    @Published var attributes: [ClassAttributeModel] = []
    @Published var methods: [MethodModel] = []
    @Published var isExpanded: Bool = true

    init(name: String, parentName: String? = nil) {
        self.name = name
        self.parentName = parentName
    }
}

extension ProjectModel {
    /// All classes across all sections
    var classes: [ClassModel] {
        sections.flatMap { $0.classes }
    }
}
