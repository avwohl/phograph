import Foundation

struct ExampleEntry: Identifiable {
    let id = UUID()
    let name: String
    let description: String
    let projectJSON: String
}

struct ExampleCategory: Identifiable {
    let id = UUID()
    let name: String
    let icon: String
    let examples: [ExampleEntry]
}

struct ExampleCatalog {
    static let categories: [ExampleCategory] = loadCategories()

    private static func loadCategories() -> [ExampleCategory] {
        guard let catalogURL = Bundle.main.url(forResource: "catalog", withExtension: "json", subdirectory: "examples"),
              let data = try? Data(contentsOf: catalogURL),
              let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let cats = json["categories"] as? [[String: Any]] else {
            print("ExampleCatalog: failed to load examples/catalog.json from bundle")
            return []
        }

        return cats.compactMap { cat in
            guard let name = cat["name"] as? String,
                  let icon = cat["icon"] as? String,
                  let entries = cat["examples"] as? [[String: Any]] else { return nil }

            let examples = entries.compactMap { entry -> ExampleEntry? in
                guard let eName = entry["name"] as? String,
                      let eDesc = entry["description"] as? String,
                      let eFile = entry["file"] as? String else { return nil }

                guard let fileURL = Bundle.main.url(forResource: eFile, withExtension: nil, subdirectory: "examples"),
                      let jsonStr = try? String(contentsOf: fileURL, encoding: .utf8) else {
                    print("ExampleCatalog: failed to load examples/\(eFile)")
                    return nil
                }

                return ExampleEntry(name: eName, description: eDesc, projectJSON: jsonStr)
            }

            return ExampleCategory(name: name, icon: icon, examples: examples)
        }
    }
}
