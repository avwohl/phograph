import SwiftUI

@main
struct PhographApp: App {
    @StateObject private var viewModel = IDEViewModel()

    var body: some Scene {
        WindowGroup {
            IDEWorkspaceView(viewModel: viewModel)
        }
        #if os(macOS)
        .defaultSize(width: 1200, height: 800)
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About Phograph") {
                    showAbout()
                }
            }
            CommandGroup(replacing: .newItem) {
                Button("New Project") {
                    viewModel.newProject()
                }
                .keyboardShortcut("n", modifiers: .command)

                Button("Open Project...") {
                    openProject()
                }
                .keyboardShortcut("o", modifiers: .command)

                Divider()

                Button("Save Project") {
                    saveProject()
                }
                .keyboardShortcut("s", modifiers: .command)
                .disabled(viewModel.project == nil)

                Button("Save Project As...") {
                    saveProjectAs()
                }
                .keyboardShortcut("s", modifiers: [.command, .shift])
                .disabled(viewModel.project == nil)
            }
        }
        #endif
    }

    #if os(macOS)
    private func showAbout() {
        let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "0.1.0"
        let alert = NSAlert()
        alert.messageText = "Phograph"
        alert.informativeText = """
            Version \(version)

            A modern implementation of the Prograph
            visual dataflow programming language.

            Author: David Wohl
            Copyright \u{00A9} 2025-2026 David Wohl.
            All rights reserved.

            https://github.com/dwohl/phograph
            """
        alert.alertStyle = .informational
        alert.icon = NSApp.applicationIconImage
        alert.runModal()
    }

    private func openProject() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.json]
        panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            if let json = try? String(contentsOf: url) {
                viewModel.loadProject(json: json, from: url)
            }
        }
    }

    private func saveProject() {
        guard let project = viewModel.project else { return }
        if let url = project.filePath {
            viewModel.saveProject(to: url)
        } else {
            saveProjectAs()
        }
    }

    private func saveProjectAs() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.json]
        panel.nameFieldStringValue = (viewModel.project?.name ?? "Untitled") + ".phograph.json"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            viewModel.saveProject(to: url)
        }
    }
    #endif
}
