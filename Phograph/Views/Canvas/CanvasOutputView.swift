import SwiftUI

/// Shows the rendered pixel buffer from canvas-render in a floating panel.
struct CanvasOutputView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Canvas Output")
                    .font(.headline)
                Spacer()
                Button("Close") {
                    viewModel.showCanvasOutput = false
                }
                .keyboardShortcut(.escape, modifiers: [])
            }
            .padding()

            PhographCanvasRepresentable(bridge: viewModel.bridge)
                .frame(
                    width: CGFloat(viewModel.bridge.bufferWidth),
                    height: CGFloat(viewModel.bridge.bufferHeight)
                )
                .padding(.bottom)
        }
        .frame(
            minWidth: max(CGFloat(viewModel.bridge.bufferWidth) + 32, 200),
            minHeight: max(CGFloat(viewModel.bridge.bufferHeight) + 80, 200)
        )
    }
}
