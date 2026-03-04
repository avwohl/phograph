import SwiftUI

/// Legacy content view, now delegates to IDEWorkspaceView.
/// Kept for compatibility - the main app entry now uses IDEWorkspaceView directly.
struct ContentView: View {
    @StateObject private var viewModel = IDEViewModel()

    var body: some View {
        IDEWorkspaceView(viewModel: viewModel)
    }
}
