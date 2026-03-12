// ExportAppSheet.swift
// SwiftUI sheet for configuring and triggering standalone app export.

import SwiftUI

struct ExportAppSheet: View {
    @ObservedObject var viewModel: IDEViewModel
    @StateObject private var exporter: AppExporter
    @Environment(\.dismiss) private var dismiss

    @State private var appName: String
    @State private var bundleID: String = "com.example.myapp"
    @State private var teamID: String = ""
    @State private var buildMacOS: Bool = true
    @State private var buildIOS: Bool = false
    @State private var entryMethod: String
    @State private var outputDirectory: URL?
    @State private var isExporting: Bool = false
    @State private var showBuildLog: Bool = false
    @State private var showDirectoryPicker: Bool = false

    init(viewModel: IDEViewModel) {
        self.viewModel = viewModel
        _exporter = StateObject(wrappedValue: AppExporter(bridge: viewModel.bridge))
        _appName = State(initialValue: viewModel.project?.name ?? "MyApp")
        _entryMethod = State(initialValue: viewModel.selectedMethodName ?? "main")
    }

    private var methodNames: [String] {
        var names: [String] = []
        if let sections = viewModel.project?.sections {
            for section in sections {
                for method in section.methods {
                    names.append(method.name)
                }
            }
        }
        return names
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Build Standalone App")
                .font(.title2)
                .fontWeight(.semibold)

            Form {
                Section("App Configuration") {
                    TextField("App Name", text: $appName)
                    TextField("Bundle Identifier", text: $bundleID)
                    TextField("Team ID", text: $teamID)
                }

                Section("Platform") {
                    Toggle("macOS", isOn: $buildMacOS)
                    Toggle("iOS", isOn: $buildIOS)
                }

                Section("Entry Point") {
                    Picker("Entry Method", selection: $entryMethod) {
                        ForEach(methodNames, id: \.self) { name in
                            Text(name).tag(name)
                        }
                    }
                }

                Section("Output") {
                    HStack {
                        Text(outputDirectory?.path ?? "No directory selected")
                            .foregroundColor(outputDirectory == nil ? .secondary : .primary)
                            .lineLimit(1)
                            .truncationMode(.middle)
                        Spacer()
                        Button("Choose...") {
                            showDirectoryPicker = true
                        }
                    }
                }
            }

            // Build log
            if showBuildLog {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text("Build Log")
                            .font(.headline)
                        Spacer()
                        Text(exporter.phase.rawValue)
                            .foregroundColor(exporter.phase == .failed ? .red : .secondary)
                    }
                    ScrollView {
                        Text(exporter.buildLog)
                            .font(.system(.caption, design: .monospaced))
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .textSelection(.enabled)
                    }
                    .frame(height: 150)
                    .background(Color(.secondarySystemBackground))
                    .cornerRadius(4)
                }
            }

            // Error
            if let error = exporter.error {
                Text(error)
                    .foregroundColor(.red)
                    .font(.caption)
            }

            // Actions
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)

                Spacer()

                if showBuildLog && exporter.phase == .done {
                    Button("Reveal in Files") {
                        if let dir = outputDirectory {
                            UIApplication.shared.open(dir)
                        }
                    }
                }

                Button("Build") {
                    startExport()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(!canBuild || isExporting)
            }
        }
        .padding(20)
        .frame(width: 500, height: showBuildLog ? 650 : 450)
        .fileImporter(
            isPresented: $showDirectoryPicker,
            allowedContentTypes: [.folder],
            allowsMultipleSelection: false
        ) { result in
            if case .success(let urls) = result, let url = urls.first {
                outputDirectory = url
            }
        }
    }

    private var canBuild: Bool {
        !appName.isEmpty && !bundleID.isEmpty && outputDirectory != nil && (buildMacOS || buildIOS)
    }

    private func startExport() {
        guard canBuild, let outputDir = outputDirectory else { return }

        var platforms: Set<AppExporter.Platform> = []
        if buildMacOS { platforms.insert(.macOS) }
        if buildIOS { platforms.insert(.iOS) }

        let config = AppExporter.Config(
            appName: appName,
            bundleID: bundleID,
            teamID: teamID,
            platforms: platforms,
            entryMethod: entryMethod,
            outputDirectory: outputDir
        )

        isExporting = true
        showBuildLog = true

        Task {
            do {
                let _ = try await exporter.export(config: config)
                await MainActor.run {
                    isExporting = false
                    viewModel.statusMessage = "App exported"
                }
            } catch {
                await MainActor.run {
                    isExporting = false
                    viewModel.statusMessage = "Export failed"
                }
            }
        }
    }
}
