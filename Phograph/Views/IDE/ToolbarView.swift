import SwiftUI

/// IDE toolbar with Run/Stop buttons and status.
struct ToolbarView: ToolbarContent {
    @ObservedObject var viewModel: IDEViewModel

    var body: some ToolbarContent {
        ToolbarItem(placement: .automatic) {
            HStack(spacing: 12) {
                Button(action: { viewModel.runCurrentMethod() }) {
                    Label("Run", systemImage: "play.fill")
                }
                .keyboardShortcut("r", modifiers: .command)
                .disabled(viewModel.isRunning || viewModel.selectedMethodName == nil)

                Button(action: { viewModel.showFuzzyFinder.toggle() }) {
                    Label("Add Node", systemImage: "plus.circle")
                }
                .keyboardShortcut("n", modifiers: .command)

                Spacer()

                Text(viewModel.statusMessage)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
}
