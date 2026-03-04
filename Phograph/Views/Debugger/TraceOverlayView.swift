import SwiftUI

/// Overlays trace values on wires in the graph canvas.
struct TraceOverlayView: View {
    let traceValues: [WireTraceValue]
    let zoomScale: CGFloat
    let panOffset: CGPoint

    var body: some View {
        ForEach(traceValues) { trace in
            Text(trace.displayValue)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(.yellow)
                .padding(.horizontal, 4)
                .padding(.vertical, 2)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(Color.black.opacity(0.8))
                )
                .position(
                    x: trace.midX * zoomScale + panOffset.x,
                    y: trace.midY * zoomScale + panOffset.y - 12
                )
        }
    }
}

struct WireTraceValue: Identifiable {
    let id = UUID()
    let midX: CGFloat
    let midY: CGFloat
    let displayValue: String
}
