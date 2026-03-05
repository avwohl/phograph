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

                Button("Browse Examples...") {
                    viewModel.showExampleBrowser = true
                }
                .keyboardShortcut("e", modifiers: [.command, .shift])

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

            // Edit menu additions
            CommandGroup(after: .pasteboard) {
                Divider()

                Button("Delete") {
                    viewModel.deleteSelected()
                }
                .keyboardShortcut(.delete, modifiers: [])
                .disabled(viewModel.currentGraph?.selectedNodeIds.isEmpty ?? true)

                Button("Select All") {
                    viewModel.selectAll()
                }
                .keyboardShortcut("a", modifiers: .command)
                .disabled(viewModel.currentGraph == nil)

                Button("Duplicate") {
                    viewModel.duplicateSelected()
                }
                .keyboardShortcut("d", modifiers: .command)
                .disabled(viewModel.currentGraph?.selectedNodeIds.isEmpty ?? true)
            }

            // View menu
            CommandMenu("View") {
                Button("Zoom In") {
                    viewModel.zoomIn()
                }
                .keyboardShortcut("=", modifiers: .command)

                Button("Zoom Out") {
                    viewModel.zoomOut()
                }
                .keyboardShortcut("-", modifiers: .command)

                Button("Fit to Window") {
                    viewModel.fitToWindow()
                }
                .keyboardShortcut("1", modifiers: .command)

                Divider()

                Button("\(viewModel.showSidebar ? "Hide" : "Show") Sidebar") {
                    viewModel.showSidebar.toggle()
                }
                .keyboardShortcut("0", modifiers: .command)

                Button("\(viewModel.showInspector ? "Hide" : "Show") Inspector") {
                    viewModel.showInspector.toggle()
                }
                .keyboardShortcut("0", modifiers: [.command, .option])

                Button("\(viewModel.showConsole ? "Hide" : "Show") Console") {
                    viewModel.showConsole.toggle()
                }
                .keyboardShortcut("y", modifiers: .command)
            }

            // Run menu
            CommandMenu("Run") {
                Button("Run") {
                    viewModel.runCurrentMethod()
                }
                .keyboardShortcut("r", modifiers: .command)
                .disabled(viewModel.selectedMethodName == nil)

                Button("Debug") {
                    viewModel.debugRun()
                }
                .keyboardShortcut("r", modifiers: [.command, .shift])
                .disabled(viewModel.selectedMethodName == nil)

                Button("Stop") {
                    viewModel.stopExecution()
                }
                .keyboardShortcut(".", modifiers: .command)
                .disabled(!viewModel.isRunning && !viewModel.isDebugging)

                Divider()

                Button("Continue") {
                    viewModel.debugContinue()
                }
                .keyboardShortcut("\\", modifiers: .command)
                .disabled(!viewModel.debugger.isPaused)

                Button("Step Over") {
                    viewModel.debugStepOver()
                }
                .keyboardShortcut("'", modifiers: .command)
                .disabled(!viewModel.debugger.isPaused)

                Button("Step Into") {
                    viewModel.debugStepInto()
                }
                .keyboardShortcut(";", modifiers: .command)
                .disabled(!viewModel.debugger.isPaused)

                Divider()

                Button("Clear Console") {
                    viewModel.clearConsole()
                }
                .keyboardShortcut("k", modifiers: .command)
            }

            // Help menu
            CommandMenu("Help") {
                Button("Lessons") {
                    NSWorkspace.shared.open(URL(string: "https://avwohl.github.io/phograph/")!)
                }

                Button("IDE Guide") {
                    NSWorkspace.shared.open(URL(string: "https://avwohl.github.io/phograph/guide.html")!)
                }

                Button("Language Reference") {
                    NSWorkspace.shared.open(URL(string: "https://avwohl.github.io/phograph/reference.html")!)
                }
            }

            // Libraries menu
            CommandMenu("Libraries") {
                Button("Manage Libraries...") {
                    viewModel.showLibraryManager = true
                }
                .keyboardShortcut("l", modifiers: [.command, .shift])

                Divider()

                Button("Refresh Library Catalog") {
                    viewModel.libraryManager.discoverLibraries()
                }
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
