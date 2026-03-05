import Foundation
import Combine

class LibraryManager: ObservableObject {
    @Published var libraries: [LibraryInfo] = []
    @Published var isDownloading: Bool = false
    @Published var downloadError: String?

    /// Maps primitive name -> library name for quick lookup
    private(set) var primOwnership: [String: String] = [:]

    /// User libraries directory: Application Support/Phograph/libraries/
    static var userLibrariesURL: URL {
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        return appSupport.appendingPathComponent("Phograph/libraries", isDirectory: true)
    }

    /// Bundled libraries directory: Bundle.main.resourceURL/Libraries/
    static var bundledLibrariesURL: URL? {
        Bundle.main.resourceURL?.appendingPathComponent("Libraries", isDirectory: true)
    }

    init() {
        ensureUserLibrariesDir()
        seedBundledLibrariesIfNeeded()
    }

    // MARK: - Discovery

    func discoverLibraries() {
        var discovered: [String: LibraryInfo] = [:]

        // 1. Scan bundled libraries
        if let bundledURL = Self.bundledLibrariesURL {
            for info in scanDirectory(bundledURL, source: .bundled) {
                discovered[info.manifest.name] = info
            }
        }

        // 2. Scan user libraries (overrides bundled)
        for info in scanDirectory(Self.userLibrariesURL, source: .local(path: Self.userLibrariesURL.path)) {
            // Check for .source_url metadata to determine if it was downloaded
            let sourceFile = info.directoryURL.appendingPathComponent(".source_url")
            if let urlString = try? String(contentsOf: sourceFile, encoding: .utf8).trimmingCharacters(in: .whitespacesAndNewlines) {
                let remoteInfo = LibraryInfo(manifest: info.manifest, directoryURL: info.directoryURL, source: .remote(url: urlString))
                discovered[info.manifest.name] = remoteInfo
            } else {
                discovered[info.manifest.name] = info
            }
        }

        // Build catalog
        libraries = discovered.values.sorted { $0.manifest.name < $1.manifest.name }

        // Build primOwnership map
        var ownership: [String: String] = [:]
        for lib in libraries {
            for prim in lib.manifest.primitives {
                ownership[prim.name] = lib.manifest.name
            }
        }
        primOwnership = ownership
    }

    private func scanDirectory(_ url: URL, source: LibrarySourceType) -> [LibraryInfo] {
        let fm = FileManager.default
        guard fm.fileExists(atPath: url.path) else { return [] }

        var results: [LibraryInfo] = []
        guard let contents = try? fm.contentsOfDirectory(at: url, includingPropertiesForKeys: [.isDirectoryKey]) else {
            return []
        }

        for item in contents {
            var isDir: ObjCBool = false
            guard fm.fileExists(atPath: item.path, isDirectory: &isDir), isDir.boolValue else { continue }

            let manifestURL = item.appendingPathComponent("library.json")
            guard let data = try? Data(contentsOf: manifestURL),
                  let manifest = try? JSONDecoder().decode(LibraryManifest.self, from: data) else {
                continue
            }

            let actualSource: LibrarySourceType
            switch source {
            case .bundled:
                actualSource = .bundled
            default:
                actualSource = .local(path: item.path)
            }

            results.append(LibraryInfo(manifest: manifest, directoryURL: item, source: actualSource))
        }

        return results
    }

    // MARK: - HTTPS Download

    func downloadLibrary(from urlString: String) async throws {
        guard let url = URL(string: urlString) else {
            throw LibraryError.invalidURL
        }
        guard url.scheme == "https" else {
            throw LibraryError.httpsRequired
        }

        await MainActor.run {
            isDownloading = true
            downloadError = nil
        }

        defer {
            Task { @MainActor in
                isDownloading = false
            }
        }

        let (localURL, response) = try await URLSession.shared.download(from: url)
        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            throw LibraryError.downloadFailed("HTTP \((response as? HTTPURLResponse)?.statusCode ?? 0)")
        }

        let fm = FileManager.default
        let tempDir = fm.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        try fm.createDirectory(at: tempDir, withIntermediateDirectories: true)
        defer { try? fm.removeItem(at: tempDir) }

        let filename = url.lastPathComponent.lowercased()
        if filename.hasSuffix(".tar.gz") || filename.hasSuffix(".tgz") {
            // Extract tar.gz
            let tarPath = tempDir.appendingPathComponent("archive.tar.gz")
            try fm.moveItem(at: localURL, to: tarPath)
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/tar")
            process.arguments = ["-xzf", tarPath.path, "-C", tempDir.path]
            try process.run()
            process.waitUntilExit()
            guard process.terminationStatus == 0 else {
                throw LibraryError.downloadFailed("Failed to extract archive")
            }
        } else if filename.hasSuffix(".zip") {
            // Extract zip via ditto
            let zipPath = tempDir.appendingPathComponent("archive.zip")
            try fm.moveItem(at: localURL, to: zipPath)
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
            process.arguments = ["-xk", zipPath.path, tempDir.path]
            try process.run()
            process.waitUntilExit()
            guard process.terminationStatus == 0 else {
                throw LibraryError.downloadFailed("Failed to extract zip")
            }
        } else {
            // Treat as a single file (unlikely) — just move
            try fm.moveItem(at: localURL, to: tempDir.appendingPathComponent(url.lastPathComponent))
        }

        // Find the library directory (contains library.json)
        let libDir = try findLibraryDir(in: tempDir)
        let data = try Data(contentsOf: libDir.appendingPathComponent("library.json"))
        let manifest = try JSONDecoder().decode(LibraryManifest.self, from: data)

        // Check for existing
        let destDir = Self.userLibrariesURL.appendingPathComponent(manifest.name)
        if fm.fileExists(atPath: destDir.path) {
            throw LibraryError.alreadyExists(manifest.name)
        }

        // Copy to user libraries
        try fm.copyItem(at: libDir, to: destDir)

        // Write source URL metadata
        let sourceFile = destDir.appendingPathComponent(".source_url")
        try urlString.write(to: sourceFile, atomically: true, encoding: .utf8)

        await MainActor.run {
            discoverLibraries()
        }
    }

    // MARK: - Local Install

    func installFromLocalPath(_ url: URL) throws {
        let fm = FileManager.default
        let manifestURL = url.appendingPathComponent("library.json")
        guard fm.fileExists(atPath: manifestURL.path) else {
            throw LibraryError.missingManifest
        }

        let data = try Data(contentsOf: manifestURL)
        let manifest = try JSONDecoder().decode(LibraryManifest.self, from: data)

        let destDir = Self.userLibrariesURL.appendingPathComponent(manifest.name)
        if fm.fileExists(atPath: destDir.path) {
            throw LibraryError.alreadyExists(manifest.name)
        }

        try fm.copyItem(at: url, to: destDir)
        discoverLibraries()
    }

    // MARK: - Remove

    func removeLibrary(_ library: LibraryInfo) throws {
        guard library.source != .bundled else { return }
        try FileManager.default.removeItem(at: library.directoryURL)
        discoverLibraries()
    }

    // MARK: - Version Checking

    /// Semver compatibility: same major, installed minor >= required minor
    static func versionSatisfies(installed: String, required: String) -> Bool {
        let iParts = installed.split(separator: ".").compactMap { Int($0) }
        let rParts = required.split(separator: ".").compactMap { Int($0) }
        guard iParts.count >= 2, rParts.count >= 2 else { return installed == required }
        // Same major
        guard iParts[0] == rParts[0] else { return false }
        // Installed minor >= required minor
        return iParts[1] >= rParts[1]
    }

    /// Resolve project library references against installed libraries
    func resolveReferences(_ refs: [LibraryReference]) -> [LibraryReference] {
        var missing: [LibraryReference] = []
        for ref in refs {
            if let lib = libraries.first(where: { $0.manifest.name == ref.name }) {
                if !Self.versionSatisfies(installed: lib.manifest.version, required: ref.version) {
                    missing.append(ref)
                }
            } else {
                missing.append(ref)
            }
        }
        return missing
    }

    // MARK: - Helpers

    private func ensureUserLibrariesDir() {
        let fm = FileManager.default
        if !fm.fileExists(atPath: Self.userLibrariesURL.path) {
            try? fm.createDirectory(at: Self.userLibrariesURL, withIntermediateDirectories: true)
        }
    }

    /// On first launch, copy bundled libraries to user dir
    private func seedBundledLibrariesIfNeeded() {
        let fm = FileManager.default
        let userDir = Self.userLibrariesURL

        // Only seed if the libraries directory is empty
        let contents = (try? fm.contentsOfDirectory(atPath: userDir.path)) ?? []
        let libDirs = contents.filter { !$0.hasPrefix(".") }
        guard libDirs.isEmpty else { return }

        // Look for bundled libraries in the app bundle
        if let resourceURL = Bundle.main.resourceURL {
            let bundledDir = resourceURL.appendingPathComponent("libraries")
            if fm.fileExists(atPath: bundledDir.path),
               let entries = try? fm.contentsOfDirectory(at: bundledDir, includingPropertiesForKeys: nil) {
                for entry in entries {
                    let dest = userDir.appendingPathComponent(entry.lastPathComponent)
                    try? fm.copyItem(at: entry, to: dest)
                }
            }
        }
    }

    /// Find the directory containing library.json within an extracted archive
    private func findLibraryDir(in dir: URL) throws -> URL {
        let fm = FileManager.default
        // Check top level
        if fm.fileExists(atPath: dir.appendingPathComponent("library.json").path) {
            return dir
        }
        // Check one level deep
        if let items = try? fm.contentsOfDirectory(at: dir, includingPropertiesForKeys: [.isDirectoryKey]) {
            for item in items {
                var isDir: ObjCBool = false
                if fm.fileExists(atPath: item.path, isDirectory: &isDir), isDir.boolValue {
                    if fm.fileExists(atPath: item.appendingPathComponent("library.json").path) {
                        return item
                    }
                }
            }
        }
        throw LibraryError.missingManifest
    }
}
