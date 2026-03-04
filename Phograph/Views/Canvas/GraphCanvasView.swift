import SwiftUI

/// Graph editor canvas with zoom/pan/selection and node+wire rendering.
struct GraphCanvasView: View {
    @ObservedObject var viewModel: IDEViewModel
    @StateObject private var editor = GraphEditorViewModel()

    var body: some View {
        GeometryReader { geo in
            ZStack {
                // White background like classic Prograph
                Color.white
                    .ignoresSafeArea()

                // Graph content (scrollable via pan + zoom)
                graphContent
                    .scaleEffect(editor.zoomScale, anchor: .topLeading)
                    .offset(x: editor.panOffset.x, y: editor.panOffset.y)

                // Wire being dragged
                if editor.isDraggingWire {
                    Path { path in
                        path.move(to: editor.wireStartPoint)
                        let cp1 = CGPoint(x: editor.wireStartPoint.x + 50, y: editor.wireStartPoint.y)
                        let cp2 = CGPoint(x: editor.wireEndPoint.x - 50, y: editor.wireEndPoint.y)
                        path.addCurve(to: editor.wireEndPoint, control1: cp1, control2: cp2)
                    }
                    .stroke(Color.blue.opacity(0.8), lineWidth: 2)
                }

                // Selection rect
                if let rect = editor.selectionRect {
                    Rectangle()
                        .fill(Color.blue.opacity(0.1))
                        .border(Color.blue.opacity(0.5), width: 1)
                        .frame(width: rect.width, height: rect.height)
                        .position(x: rect.midX, y: rect.midY)
                }

                // Console overlay at bottom
                VStack {
                    Spacer()
                    if !viewModel.consoleOutput.isEmpty {
                        ScrollView {
                            Text(viewModel.consoleOutput)
                                .font(.system(.caption, design: .monospaced))
                                .foregroundColor(.white)
                                .padding(8)
                                .frame(maxWidth: .infinity, alignment: .leading)
                        }
                        .frame(height: 80)
                        .background(Color.black.opacity(0.8))
                    }
                }
            }
            .gesture(panGesture)
            .onAppear {
                editor.graph = viewModel.currentGraph
                // Delay centering so GeometryReader has final size
                DispatchQueue.main.async {
                    centerGraph(in: geo.size)
                    syncPanStart()
                }
            }
            .onChange(of: viewModel.currentGraph?.methodName) { _ in
                editor.graph = viewModel.currentGraph
                DispatchQueue.main.async {
                    centerGraph(in: geo.size)
                    syncPanStart()
                }
            }
            .onChange(of: geo.size) { newSize in
                centerGraph(in: newSize)
                syncPanStart()
            }
        }
    }

    private func centerGraph(in size: CGSize) {
        guard let graph = viewModel.currentGraph, !graph.nodes.isEmpty else { return }

        // Find bounding box of all nodes
        var minX = CGFloat.infinity, minY = CGFloat.infinity
        var maxX = -CGFloat.infinity, maxY = -CGFloat.infinity
        for node in graph.nodes {
            minX = min(minX, node.x)
            minY = min(minY, node.y)
            maxX = max(maxX, node.x + node.width)
            maxY = max(maxY, node.y + node.height)
        }

        let graphW = maxX - minX
        let graphH = maxY - minY
        let margin: CGFloat = 40

        // Scale to fit but keep nodes readable (min 0.6, max 1.0)
        let scaleX = (size.width - margin * 2) / max(graphW, 1)
        let scaleY = (size.height - margin * 2) / max(graphH, 1)
        let scale = max(min(min(scaleX, scaleY), 1.0), 0.6)

        // Center the graph in the canvas
        let offsetX = (size.width - graphW * scale) / 2 - minX * scale + margin / 2
        let offsetY = (size.height - graphH * scale) / 2 - minY * scale + margin / 2

        editor.zoomScale = scale
        editor.panOffset = CGPoint(x: offsetX, y: offsetY)
    }

    @ViewBuilder
    private var graphContent: some View {
        if let graph = viewModel.currentGraph {
            ZStack {
                // Wires (draw first, behind nodes)
                ForEach(graph.wires) { wire in
                    wireView(wire: wire, graph: graph)
                }

                // Nodes
                ForEach(graph.nodes) { node in
                    nodeView(node: node, isSelected: graph.selectedNodeIds.contains(node.id))
                }
            }
        }
    }

    // MARK: - Node rendering

    private func nodeView(node: GraphNodeModel, isSelected: Bool) -> some View {
        let colors = nodeColors(for: node.nodeType)

        return VStack(spacing: 0) {
            // Header
            HStack {
                Text(node.label)
                    .font(.system(size: 14, weight: .semibold))
                    .foregroundColor(colors.headerText)
                    .lineLimit(1)
                Spacer()
            }
            .padding(.horizontal, 10)
            .frame(height: 30)
            .background(colors.header)

            // Pin area
            if node.inputPins.count > 0 || node.outputPins.count > 0 {
                HStack(alignment: .top) {
                    // Input pins (left side)
                    VStack(alignment: .leading, spacing: 4) {
                        ForEach(node.inputPins) { pin in
                            HStack(spacing: 5) {
                                Circle()
                                    .fill(Color(red: 0.2, green: 0.5, blue: 0.9))
                                    .frame(width: 10, height: 10)
                                Text(pin.name)
                                    .font(.system(size: 11))
                                    .foregroundColor(.secondary)
                            }
                            .frame(height: 22)
                        }
                    }
                    Spacer()
                    // Output pins (right side)
                    VStack(alignment: .trailing, spacing: 4) {
                        ForEach(node.outputPins) { pin in
                            HStack(spacing: 5) {
                                Text(pin.name)
                                    .font(.system(size: 11))
                                    .foregroundColor(.secondary)
                                Circle()
                                    .fill(Color(red: 0.9, green: 0.3, blue: 0.2))
                                    .frame(width: 10, height: 10)
                            }
                            .frame(height: 22)
                        }
                    }
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
            }
        }
        .frame(width: node.width, height: node.height)
        .background(colors.body)
        .clipShape(RoundedRectangle(cornerRadius: 4))
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(isSelected ? Color.blue : colors.border, lineWidth: isSelected ? 2.5 : 1)
        )
        .shadow(color: .black.opacity(0.15), radius: 2, x: 0, y: 1)
        .position(x: node.x + node.width / 2, y: node.y + node.height / 2)
        .gesture(
            DragGesture()
                .onChanged { value in
                    let graphPt = editor.screenToGraph(value.location)
                    if !editor.isDraggingNode {
                        editor.beginNodeDrag(nodeId: node.id, at: graphPt)
                    }
                    editor.updateNodeDrag(to: graphPt)
                }
                .onEnded { _ in
                    editor.endNodeDrag()
                }
        )
        .onTapGesture {
            viewModel.currentGraph?.selectNode(id: node.id)
        }
    }

    private struct NodeColors {
        var header: Color
        var headerText: Color
        var body: Color
        var border: Color
    }

    private func nodeColors(for nodeType: String) -> NodeColors {
        switch nodeType {
        case "input_bar":
            return NodeColors(
                header: Color(red: 0.2, green: 0.55, blue: 0.3),
                headerText: .white,
                body: Color(red: 0.85, green: 0.95, blue: 0.87),
                border: Color(red: 0.2, green: 0.55, blue: 0.3).opacity(0.6)
            )
        case "output_bar":
            return NodeColors(
                header: Color(red: 0.7, green: 0.2, blue: 0.2),
                headerText: .white,
                body: Color(red: 0.97, green: 0.88, blue: 0.88),
                border: Color(red: 0.7, green: 0.2, blue: 0.2).opacity(0.6)
            )
        case "constant":
            return NodeColors(
                header: Color(red: 0.5, green: 0.45, blue: 0.2),
                headerText: .white,
                body: Color(red: 0.98, green: 0.96, blue: 0.88),
                border: Color(red: 0.5, green: 0.45, blue: 0.2).opacity(0.6)
            )
        case "primitive":
            return NodeColors(
                header: Color(red: 0.25, green: 0.35, blue: 0.6),
                headerText: .white,
                body: Color(red: 0.92, green: 0.94, blue: 0.98),
                border: Color(red: 0.25, green: 0.35, blue: 0.6).opacity(0.5)
            )
        case "method_call":
            return NodeColors(
                header: Color(red: 0.4, green: 0.25, blue: 0.55),
                headerText: .white,
                body: Color(red: 0.95, green: 0.92, blue: 0.98),
                border: Color(red: 0.4, green: 0.25, blue: 0.55).opacity(0.5)
            )
        default:
            return NodeColors(
                header: Color(red: 0.4, green: 0.4, blue: 0.4),
                headerText: .white,
                body: Color(red: 0.95, green: 0.95, blue: 0.95),
                border: Color(red: 0.4, green: 0.4, blue: 0.4).opacity(0.5)
            )
        }
    }

    // MARK: - Wire rendering

    private func wireView(wire: GraphWireModel, graph: GraphModel) -> some View {
        let sourceNode = graph.nodes.first { $0.id == wire.sourceNodeId }
        let destNode = graph.nodes.first { $0.id == wire.destNodeId }

        return Group {
            if let src = sourceNode, let dst = destNode {
                // Pin Y: header(30) + padding(4) + pinIndex*26 + center(11)
                let startX = src.x + src.width
                let startY = src.y + 30 + 4 + CGFloat(wire.sourcePin) * 26 + 11
                let endX = dst.x
                let endY = dst.y + 30 + 4 + CGFloat(wire.destPin) * 26 + 11

                Path { path in
                    path.move(to: CGPoint(x: startX, y: startY))
                    let dx = max(abs(endX - startX) * 0.4, 30)
                    path.addCurve(
                        to: CGPoint(x: endX, y: endY),
                        control1: CGPoint(x: startX + dx, y: startY),
                        control2: CGPoint(x: endX - dx, y: endY)
                    )
                }
                .stroke(Color.black.opacity(0.55), lineWidth: 1.5)
            }
        }
    }

    // MARK: - Gestures

    @State private var panStart: CGPoint = .zero

    private var panGesture: some Gesture {
        DragGesture(minimumDistance: 5)
            .onChanged { value in
                if value.translation == .zero { return }
                let newOffset = CGPoint(
                    x: panStart.x + value.translation.width,
                    y: panStart.y + value.translation.height
                )
                editor.panOffset = newOffset
            }
            .onEnded { _ in
                panStart = editor.panOffset
            }
    }

    private func syncPanStart() {
        panStart = editor.panOffset
    }
}
