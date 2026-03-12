import SwiftUI

/// IDE toolbar with Run/Stop buttons, view toggles, and status.
struct ToolbarView: ToolbarContent {
    @ObservedObject var viewModel: IDEViewModel

    var body: some ToolbarContent {
        ToolbarItemGroup(placement: .primaryAction) {
            Button(action: { viewModel.runCurrentMethod() }) {
                Label("Run", systemImage: "play.fill")
            }
            .keyboardShortcut("r", modifiers: .command)
            .disabled(viewModel.isRunning || viewModel.selectedMethodName == nil)

            Button(action: { viewModel.debugRun() }) {
                Label("Debug", systemImage: "ant.fill")
            }
            .keyboardShortcut("r", modifiers: [.command, .shift])
            .disabled(viewModel.isRunning || viewModel.selectedMethodName == nil)

            if viewModel.isRunning || viewModel.isDebugging {
                Button(action: { viewModel.stopExecution() }) {
                    Label("Stop", systemImage: "stop.fill")
                }
                .keyboardShortcut(".", modifiers: .command)
            }

            Button(action: { viewModel.showFuzzyFinder.toggle() }) {
                Label("Add Node", systemImage: "plus.circle")
            }
            .keyboardShortcut("n", modifiers: .command)
        }

        ToolbarItemGroup(placement: .secondaryAction) {
            Button(action: { viewModel.showSidebar.toggle() }) {
                Label(viewModel.showSidebar ? "Hide Sidebar" : "Show Sidebar", systemImage: "sidebar.left")
            }

            Button(action: { viewModel.showInspector.toggle() }) {
                Label(viewModel.showInspector ? "Hide Inspector" : "Show Inspector", systemImage: "sidebar.right")
            }

            Button(action: { viewModel.showConsole.toggle() }) {
                Label(viewModel.showConsole ? "Hide Console" : "Show Console", systemImage: "terminal")
            }

            Divider()

            Button(action: { viewModel.showExampleBrowser = true }) {
                Label("Examples", systemImage: "book")
            }

            Button(action: { viewModel.showLibraryManager = true }) {
                Label("Libraries", systemImage: "shippingbox")
            }
        }

        ToolbarItem(placement: .status) {
            Text(viewModel.statusMessage)
                .font(.caption)
                .foregroundColor(.secondary)
        }
    }
}
