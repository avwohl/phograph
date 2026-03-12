import SwiftUI
import UniformTypeIdentifiers

@main
struct PhographApp: App {
    @StateObject private var viewModel = IDEViewModel()
    @State private var showAbout = false
    @State private var showFileImporter = false

    var body: some Scene {
        WindowGroup {
            IDEWorkspaceView(viewModel: viewModel)
                .fileImporter(
                    isPresented: $showFileImporter,
                    allowedContentTypes: [.json],
                    allowsMultipleSelection: false
                ) { result in
                    if case .success(let urls) = result, let url = urls.first {
                        guard url.startAccessingSecurityScopedResource() else { return }
                        defer { url.stopAccessingSecurityScopedResource() }
                        do {
                            let json = try String(contentsOf: url)
                            viewModel.loadProject(json: json, from: url)
                        } catch {
                            viewModel.consoleOutput += "Failed to read \(url.lastPathComponent): \(error.localizedDescription)\n"
                            viewModel.statusMessage = "Open failed"
                        }
                    }
                }
                .alert("About Phograph", isPresented: $showAbout) {
                    Button("OK", role: .cancel) {}
                } message: {
                    let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "0.1.0"
                    Text("""
                    Version \(version)

                    A modern implementation of the Prograph
                    visual dataflow programming language.

                    Author: Aaron Wohl
                    Copyright \u{00A9} 2025-2026 Aaron Wohl.
                    All rights reserved.
                    """)
                }
        }
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About Phograph") {
                    showAbout = true
                }
            }
            CommandGroup(replacing: .newItem) {
                Button("New Project") {
                    viewModel.newProject()
                }
                .keyboardShortcut("n", modifiers: .command)

                Button("Open Project...") {
                    showFileImporter = true
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
                    viewModel.showSavePanel = true
                }
                .keyboardShortcut("s", modifiers: [.command, .shift])
                .disabled(viewModel.project == nil)

                Divider()

                Button("Export as PDF...") {
                    viewModel.showPDFExport = true
                }
                .keyboardShortcut("p", modifiers: [.command, .shift])
                .disabled(viewModel.currentGraph == nil)

                Button("Export as SVG...") {
                    viewModel.showSVGExport = true
                }
                .disabled(viewModel.currentGraph == nil)

                Divider()

                Button("Build App...") {
                    viewModel.showExportSheet = true
                }
                .keyboardShortcut("b", modifiers: [.command, .shift])
                .disabled(viewModel.project == nil)
            }

            // Edit menu additions
            CommandGroup(after: .pasteboard) {
                Divider()

                Button("Delete") {
                    viewModel.deleteSelected()
                }
                .keyboardShortcut(.delete, modifiers: [])
                .disabled((viewModel.currentGraph?.selectedNodeIds.isEmpty ?? true) && (viewModel.currentGraph?.selectedWireIds.isEmpty ?? true))

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
                    openURL("https://avwohl.github.io/phograph/")
                }

                Button("IDE Guide") {
                    openURL("https://avwohl.github.io/phograph/guide.html")
                }

                Button("Language Reference") {
                    openURL("https://avwohl.github.io/phograph/reference.html")
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
    }

    private func openURL(_ urlString: String) {
        guard let url = URL(string: urlString) else { return }
        UIApplication.shared.open(url)
    }

    private func saveProject() {
        guard let project = viewModel.project else { return }
        if let url = project.filePath {
            viewModel.saveProject(to: url)
        } else {
            viewModel.showSavePanel = true
        }
    }
}
