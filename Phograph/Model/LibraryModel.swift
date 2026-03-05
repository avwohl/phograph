import Foundation

// MARK: - Manifest Types (Codable, from library.json)

struct LibraryPrimitiveDescriptor: Codable, Identifiable {
    var id: String { name }
    let name: String
    let num_inputs: Int
    let num_outputs: Int
    let description: String?
}

struct LibraryManifest: Codable {
    let name: String
    let version: String
    let description: String?
    let author: String?
    let min_engine_version: String?
    let primitives: [LibraryPrimitiveDescriptor]
}

// MARK: - Source Type

enum LibrarySourceType: Equatable {
    case bundled
    case local(path: String)
    case remote(url: String)

    var displayName: String {
        switch self {
        case .bundled: return "Bundled"
        case .local: return "Local"
        case .remote: return "Remote"
        }
    }

    var iconName: String {
        switch self {
        case .bundled: return "shippingbox.fill"
        case .local: return "folder.fill"
        case .remote: return "cloud.fill"
        }
    }
}

// MARK: - Library Info (runtime representation)

class LibraryInfo: ObservableObject, Identifiable {
    var id: String { manifest.name }
    let manifest: LibraryManifest
    let directoryURL: URL
    let source: LibrarySourceType
    @Published var isLoaded: Bool = false
    @Published var loadError: String?

    var hasDylib: Bool {
        let dylibURL = directoryURL.appendingPathComponent("\(manifest.name).dylib")
        return FileManager.default.fileExists(atPath: dylibURL.path)
    }

    init(manifest: LibraryManifest, directoryURL: URL, source: LibrarySourceType) {
        self.manifest = manifest
        self.directoryURL = directoryURL
        self.source = source
    }
}

// MARK: - Library Reference (for project JSON)

struct LibraryReference: Codable {
    let name: String
    let version: String
    let source: String?

    enum CodingKeys: String, CodingKey {
        case name, version, source
    }
}

// MARK: - Errors

enum LibraryError: LocalizedError {
    case invalidURL
    case httpsRequired
    case downloadFailed(String)
    case invalidManifest(String)
    case missingManifest
    case alreadyExists(String)
    case versionConflict(installed: String, required: String)

    var errorDescription: String? {
        switch self {
        case .invalidURL:
            return "Invalid URL"
        case .httpsRequired:
            return "Only HTTPS URLs are supported for security"
        case .downloadFailed(let reason):
            return "Download failed: \(reason)"
        case .invalidManifest(let reason):
            return "Invalid library.json: \(reason)"
        case .missingManifest:
            return "No library.json found in library directory"
        case .alreadyExists(let name):
            return "Library '\(name)' is already installed"
        case .versionConflict(let installed, let required):
            return "Version conflict: installed \(installed), required \(required)"
        }
    }
}
