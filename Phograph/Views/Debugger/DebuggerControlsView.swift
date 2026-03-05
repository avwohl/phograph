import SwiftUI

/// Debug toolbar with Step/Continue/Rollback controls.
struct DebuggerControlsView: View {
    @ObservedObject var viewModel: DebuggerViewModel
    var ideViewModel: IDEViewModel?

    var body: some View {
        HStack(spacing: 8) {
            Button(action: { ideViewModel?.debugContinue() ?? viewModel.continueExecution() }) {
                Label("Continue", systemImage: "play.fill")
            }
            .disabled(!viewModel.isPaused)

            Button(action: { ideViewModel?.debugStepOver() ?? viewModel.stepOver() }) {
                Label("Step Over", systemImage: "arrow.right")
            }
            .disabled(!viewModel.isPaused)

            Button(action: { ideViewModel?.debugStepInto() ?? viewModel.stepInto() }) {
                Label("Step Into", systemImage: "arrow.down.right")
            }
            .disabled(!viewModel.isPaused)

            Button(action: { viewModel.rollback() }) {
                Label("Rollback", systemImage: "arrow.uturn.backward")
            }
            .disabled(!viewModel.canRollback)

            Divider().frame(height: 20)

            Button(action: { ideViewModel?.debugStop() ?? viewModel.stop() }) {
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
