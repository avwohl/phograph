// AppExporter.swift
// Orchestrates exporting a Phograph program as a standalone macOS/iOS app.

import Foundation
#if os(macOS)
import AppKit
#endif

class AppExporter: ObservableObject {

    struct Config {
        var appName: String
        var bundleID: String
        var teamID: String
        var platforms: Set<Platform>
        var entryMethod: String
        var outputDirectory: URL
    }

    enum Platform: String, CaseIterable, Identifiable {
        case macOS, iOS
        var id: String { rawValue }
    }

    enum ExportPhase: String {
        case idle = "Idle"
        case compiling = "Compiling program..."
        case generating = "Generating project..."
        case building = "Building app..."
        case done = "Done"
        case failed = "Failed"
    }

    @Published var phase: ExportPhase = .idle
    @Published var buildLog: String = ""
    @Published var error: String?

    private let bridge: PhographBridge

    init(bridge: PhographBridge) {
        self.bridge = bridge
    }

    /// Sanitize app name for use as Swift identifier
    private func sanitizedName(_ name: String) -> String {
        var result = name.replacingOccurrences(of: " ", with: "")
            .replacingOccurrences(of: "-", with: "")
        result = result.filter { $0.isLetter || $0.isNumber }
        if result.isEmpty { result = "PhographApp" }
        if let first = result.first, first.isNumber {
            result = "App" + result
        }
        return result
    }

    /// Sanitize entry method name for Swift function call
    private func swiftFuncName(_ phoName: String) -> String {
        var result = ""
        var nextUpper = false
        for (i, c) in phoName.enumerated() {
            if c == "-" || c == "?" || c == "!" || c == "/" || c == " " {
                nextUpper = true
            } else if c.isLetter || c.isNumber || c == "_" {
                if nextUpper && !result.isEmpty {
                    result.append(Character(String(c).uppercased()))
                    nextUpper = false
                } else {
                    result.append(c)
                    nextUpper = false
                }
            }
        }
        if result.isEmpty { result = "main" }
        return result
    }

    // MARK: - Export

    @MainActor
    func export(config: Config) async throws -> URL {
        phase = .compiling
        buildLog = ""
        error = nil

        let safeAppName = sanitizedName(config.appName)

        // Step 1: Compile to Swift source
        log("Compiling project to Swift...")
        let generatedSource: String
        do {
            let source = try bridge.compile(toSwift: config.entryMethod, emitMain: false)
            generatedSource = source
            log("Compilation successful (\(source.count) chars)")
        } catch {
            self.error = "Compile error: \(error.localizedDescription)"
            phase = .failed
            throw error
        }

        // Step 2: Generate project directory
        phase = .generating
        let entryFunc = swiftFuncName("pho_entry_" + config.entryMethod)

        for platform in config.platforms {
            let platformDir = config.outputDirectory.appendingPathComponent("\(safeAppName)-\(platform.rawValue)")
            try generateProject(
                at: platformDir,
                appName: safeAppName,
                config: config,
                platform: platform,
                generatedSource: generatedSource,
                entryFunc: entryFunc
            )

            // Step 3: Run xcodegen
            let hasXcodegen = await checkTool("xcodegen")
            if hasXcodegen {
                log("Running xcodegen...")
                let xcodegenResult = await runProcess("xcodegen", args: ["generate", "--spec", "project.yml"], at: platformDir)
                if !xcodegenResult.success {
                    log("xcodegen failed: \(xcodegenResult.output)")
                    // Don't fail — user can run xcodegen manually
                } else {
                    log("Generated \(safeAppName).xcodeproj")
                }

                // Step 4: Build with xcodebuild
                phase = .building
                let destination: String
                let scheme = safeAppName
                switch platform {
                case .macOS:
                    destination = "platform=macOS"
                case .iOS:
                    destination = "generic/platform=iOS"
                }

                log("Building \(platform.rawValue) app...")
                let archivePath = platformDir.appendingPathComponent("\(safeAppName).xcarchive").path
                let buildResult = await runProcess("xcodebuild", args: [
                    "-project", "\(safeAppName).xcodeproj",
                    "-scheme", scheme,
                    "-destination", destination,
                    "-archivePath", archivePath,
                    "archive"
                ], at: platformDir)

                if buildResult.success {
                    log("Build succeeded: \(archivePath)")
                } else {
                    log("Build failed (exit \(buildResult.exitCode)). Partial output:\n\(buildResult.output.suffix(2000))")
                }
            } else {
                log("xcodegen not found. Install with: brew install xcodegen")
                log("Project files written to: \(platformDir.path)")
                log("You can open the directory and run xcodegen manually.")
            }
        }

        phase = .done
        log("Export complete.")
        return config.outputDirectory
    }

    // MARK: - Project Generation

    private func generateProject(
        at dir: URL,
        appName: String,
        config: Config,
        platform: Platform,
        generatedSource: String,
        entryFunc: String
    ) throws {
        let sourcesDir = dir.appendingPathComponent("Sources")
        let assetsDir = dir.appendingPathComponent("Sources/Assets.xcassets")
        let appIconDir = assetsDir.appendingPathComponent("AppIcon.appiconset")

        let fm = FileManager.default
        try fm.createDirectory(at: appIconDir, withIntermediateDirectories: true)

        // Read runtime files from IDE bundle
        let runtimeSource: String
        let sceneRuntimeSource: String

        if let runtimeURL = Bundle.main.url(forResource: "PhographRuntime", withExtension: "swift", subdirectory: nil) {
            runtimeSource = try String(contentsOf: runtimeURL)
        } else {
            // Fallback: read from known location
            let runtimePath = Bundle.main.bundlePath + "/Contents/Resources/PhographRuntime.swift"
            if fm.fileExists(atPath: runtimePath) {
                runtimeSource = try String(contentsOfFile: runtimePath)
            } else {
                // Last resort: use the file in the source tree
                runtimeSource = "// Runtime not found in bundle — include PhographRuntime.swift manually"
            }
        }

        if let sceneURL = Bundle.main.url(forResource: "PhographSceneRuntime", withExtension: "swift", subdirectory: nil) {
            sceneRuntimeSource = try String(contentsOf: sceneURL)
        } else {
            let scenePath = Bundle.main.bundlePath + "/Contents/Resources/PhographSceneRuntime.swift"
            if fm.fileExists(atPath: scenePath) {
                sceneRuntimeSource = try String(contentsOfFile: scenePath)
            } else {
                sceneRuntimeSource = "// Scene runtime not found in bundle — include PhographSceneRuntime.swift manually"
            }
        }

        // Write source files
        try runtimeSource.write(to: sourcesDir.appendingPathComponent("PhographRuntime.swift"), atomically: true, encoding: .utf8)
        try sceneRuntimeSource.write(to: sourcesDir.appendingPathComponent("PhographSceneRuntime.swift"), atomically: true, encoding: .utf8)
        try generatedSource.write(to: sourcesDir.appendingPathComponent("Generated.swift"), atomically: true, encoding: .utf8)
        try AppTemplates.appSwift(appName: appName).write(to: sourcesDir.appendingPathComponent("App.swift"), atomically: true, encoding: .utf8)
        try AppTemplates.contentView(entryFunction: entryFunc).write(to: sourcesDir.appendingPathComponent("ContentView.swift"), atomically: true, encoding: .utf8)

        // Platform-specific CanvasView
        let canvasViewSource: String
        switch platform {
        case .macOS:
            canvasViewSource = AppTemplates.canvasViewMacOS(entryFunction: entryFunc)
        case .iOS:
            canvasViewSource = AppTemplates.canvasViewIOS(entryFunction: entryFunc)
        }
        try canvasViewSource.write(to: sourcesDir.appendingPathComponent("CanvasView.swift"), atomically: true, encoding: .utf8)

        // Metal
        try AppTemplates.metalRenderer.write(to: sourcesDir.appendingPathComponent("MetalRenderer.swift"), atomically: true, encoding: .utf8)
        try AppTemplates.shadersMetal.write(to: sourcesDir.appendingPathComponent("Shaders.metal"), atomically: true, encoding: .utf8)

        // Config files
        let deploymentTarget = platform == .macOS ? "13.0" : "16.0"
        try AppTemplates.projectYml(
            appName: appName,
            bundleID: config.bundleID,
            teamID: config.teamID,
            platform: platform.rawValue,
            deploymentTarget: deploymentTarget
        ).write(to: dir.appendingPathComponent("project.yml"), atomically: true, encoding: .utf8)

        try AppTemplates.infoPlist(bundleID: config.bundleID, appName: config.appName)
            .write(to: dir.appendingPathComponent("Info.plist"), atomically: true, encoding: .utf8)

        try AppTemplates.entitlements(appName: appName)
            .write(to: dir.appendingPathComponent("\(appName).entitlements"), atomically: true, encoding: .utf8)

        // Assets
        try AppTemplates.assetsContentsJSON.write(to: assetsDir.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8)
        try AppTemplates.appIconContentsJSON.write(to: appIconDir.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8)

        log("Generated project at \(dir.path)")
    }

    // MARK: - Helpers

    private func log(_ msg: String) {
        let line = msg + "\n"
        DispatchQueue.main.async {
            self.buildLog += line
        }
    }

    private func checkTool(_ name: String) async -> Bool {
        let result = await runProcess("/usr/bin/which", args: [name], at: URL(fileURLWithPath: "/tmp"))
        return result.success
    }

    private struct ProcessResult {
        var output: String
        var exitCode: Int32
        var success: Bool { exitCode == 0 }
    }

    private func runProcess(_ path: String, args: [String], at workingDir: URL) async -> ProcessResult {
        await withCheckedContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async {
                let process = Process()
                let pipe = Pipe()

                // Resolve tool path
                if path.hasPrefix("/") {
                    process.executableURL = URL(fileURLWithPath: path)
                } else {
                    // Use /usr/bin/env to find the tool in PATH
                    process.executableURL = URL(fileURLWithPath: "/usr/bin/env")
                    process.arguments = [path] + args
                }
                if process.arguments == nil {
                    process.arguments = args
                }

                process.currentDirectoryURL = workingDir
                process.standardOutput = pipe
                process.standardError = pipe

                // Inherit PATH from user environment
                var env = ProcessInfo.processInfo.environment
                if let homebrew = ["/opt/homebrew/bin", "/usr/local/bin"].first(where: { FileManager.default.fileExists(atPath: $0) }) {
                    env["PATH"] = "\(homebrew):\(env["PATH"] ?? "/usr/bin:/bin")"
                }
                process.environment = env

                do {
                    try process.run()
                    process.waitUntilExit()
                    let data = pipe.fileHandleForReading.readDataToEndOfFile()
                    let output = String(data: data, encoding: .utf8) ?? ""
                    continuation.resume(returning: ProcessResult(output: output, exitCode: process.terminationStatus))
                } catch {
                    continuation.resume(returning: ProcessResult(output: error.localizedDescription, exitCode: -1))
                }
            }
        }
    }
}

enum ExportError: LocalizedError {
    case compileFailed(String)
    case projectGenFailed(String)
    case buildFailed(String)

    var errorDescription: String? {
        switch self {
        case .compileFailed(let msg): return "Compile failed: \(msg)"
        case .projectGenFailed(let msg): return "Project generation failed: \(msg)"
        case .buildFailed(let msg): return "Build failed: \(msg)"
        }
    }
}
