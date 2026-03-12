import SwiftUI
#if os(macOS)
import AppKit
#endif

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
                .keyboardShortcut(.escape, modifiers: [])
            }
            .padding()

            if let image = makeImage() {
                Image(nsImage: image)
                    .interpolation(.none)
                    .padding(.bottom)
            } else {
                Text("No pixel data")
                    .foregroundColor(.secondary)
                    .padding()
            }
        }
        .frame(
            minWidth: max(CGFloat(viewModel.bridge.bufferWidth) + 32, 200),
            minHeight: max(CGFloat(viewModel.bridge.bufferHeight) + 80, 200)
        )
    }

    #if os(macOS)
    private func makeImage() -> NSImage? {
        var w: Int32 = 0
        var h: Int32 = 0
        guard let ptr = viewModel.bridge.pixelBufferWidth(&w, height: &h),
              w > 0, h > 0 else { return nil }

        let width = Int(w)
        let height = Int(h)
        let bytesPerRow = width * 4

        // BGRA8 → NSBitmapImageRep
        guard let rep = NSBitmapImageRep(
            bitmapDataPlanes: nil,
            pixelsWide: width,
            pixelsHigh: height,
            bitsPerSample: 8,
            samplesPerPixel: 4,
            hasAlpha: true,
            isPlanar: false,
            colorSpaceName: .deviceRGB,
            bytesPerRow: bytesPerRow,
            bitsPerPixel: 32
        ), let bitmapData = rep.bitmapData else { return nil }

        // Copy BGRA → RGBA
        let src = UnsafeRawPointer(ptr).bindMemory(to: UInt8.self, capacity: bytesPerRow * height)
        for i in 0..<(width * height) {
            let si = i * 4
            bitmapData[si + 0] = src[si + 2]  // R ← B
            bitmapData[si + 1] = src[si + 1]  // G
            bitmapData[si + 2] = src[si + 0]  // B ← R
            bitmapData[si + 3] = src[si + 3]  // A
        }

        let image = NSImage(size: NSSize(width: width, height: height))
        image.addRepresentation(rep)
        return image
    }
    #endif
}
