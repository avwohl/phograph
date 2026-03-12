import SwiftUI
import UIKit

/// Shows the rendered pixel buffer from canvas-render as a static image.
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
            }
            .padding()

            Divider()

            if let image = viewModel.canvasOutputImage {
                Image(uiImage: image)
                    .interpolation(.none)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .padding()
            } else {
                Text("No pixel data")
                    .foregroundColor(.secondary)
                    .padding()
            }

            Spacer()
        }
        .frame(minWidth: 300, minHeight: 300)
    }
}
