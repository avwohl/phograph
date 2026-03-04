import SwiftUI

/// Debug toolbar with Step/Continue/Rollback controls.
struct DebuggerControlsView: View {
    @ObservedObject var viewModel: DebuggerViewModel

    var body: some View {
        HStack(spacing: 8) {
            Button(action: { viewModel.continueExecution() }) {
                Label("Continue", systemImage: "play.fill")
            }
            .disabled(!viewModel.isPaused)

            Button(action: { viewModel.stepOver() }) {
                Label("Step", systemImage: "arrow.right")
            }
            .disabled(!viewModel.isPaused)

            Button(action: { viewModel.rollback() }) {
                Label("Rollback", systemImage: "arrow.uturn.backward")
            }
            .disabled(!viewModel.canRollback)

            Divider().frame(height: 20)

            Button(action: { viewModel.stop() }) {
                Label("Stop", systemImage: "stop.fill")
            }
            .disabled(!viewModel.isDebugging)

            Spacer()

            if viewModel.isDebugging {
                Text("Step \(viewModel.currentStep)")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.horizontal)
        .padding(.vertical, 4)
    }
}
