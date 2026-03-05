import SwiftUI
import AppKit

/// Graph editor canvas with zoom/pan/selection and node+wire rendering.
struct GraphCanvasView: View {
    @ObservedObject var viewModel: IDEViewModel
    @StateObject private var editor = GraphEditorViewModel()
    @State private var canvasSize: CGSize = .zero

    /// Tracks the NSView backing this canvas for coordinate conversion
    @State private var canvasNSView: NSView?

    /// Resizable console height
    @State private var consoleHeight: CGFloat = 80

    var body: some View {
        GeometryReader { geo in
            VStack(spacing: 0) {
                ZStack {
                    // White background
                    Color.white
                        .ignoresSafeArea()
                        .onTapGesture {
                            viewModel.currentGraph?.deselectAll()
                        }

                    // Graph content (scrollable via pan + zoom)
                    graphContent
                        .scaleEffect(editor.zoomScale, anchor: .topLeading)
                        .offset(x: editor.panOffset.x, y: editor.panOffset.y)

                    // Selection rect
                    if let rect = editor.selectionRect {
                        Rectangle()
                            .fill(Color.blue.opacity(0.1))
                            .border(Color.blue.opacity(0.5), width: 1)
                            .frame(width: rect.width, height: rect.height)
                            .position(x: rect.midX, y: rect.midY)
                    }

                    // Case navigation bar (top overlay)
                    if let method = viewModel.currentMethod, method.caseCount > 1 {
                        VStack {
                            caseNavigationBar(method: method)
                            Spacer()
                        }
                    }

                    // Missing library banner (bottom overlay)
                    if !viewModel.missingLibraries.isEmpty {
                        VStack {
                            Spacer()
                            missingLibraryBanner
                        }
                    }

                    // Trace overlay during debugging
                    if viewModel.isDebugging && !viewModel.debugger.wireTraceValues.isEmpty {
                        TraceOverlayView(
                            traceValues: viewModel.debugger.wireTraceValues,
                            zoomScale: editor.zoomScale,
                            panOffset: editor.panOffset
                        )
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .clipped()

                // Console overlay at bottom
                if viewModel.showConsole && !viewModel.consoleOutput.isEmpty {
                    consolePanel
                }
            }
            .background(CanvasNSViewFinder(nsView: $canvasNSView))
            .gesture(panGesture)
            .onAppear {
                canvasSize = geo.size
                editor.graph = viewModel.currentGraph
                installRightClickMonitor()
                installKeyMonitor()
                DispatchQueue.main.async {
                    canvasSize = geo.size
                    centerGraph(in: geo.size)
                    syncPanStart()
                }
            }
            .onDisappear {
                removeRightClickMonitor()
                removeKeyMonitor()
            }
            .onChange(of: viewModel.currentGraph?.methodName) { _ in
                editor.graph = viewModel.currentGraph
                DispatchQueue.main.async {
                    centerGraph(in: geo.size)
                    syncPanStart()
                }
            }
            .onChange(of: geo.size) { newSize in
                canvasSize = newSize
                centerGraph(in: newSize)
                syncPanStart()
            }
            .onChange(of: viewModel.zoomRequest) { request in
                guard let request = request else { return }
                let center = CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2)
                switch request {
                case .zoomIn:
                    editor.zoom(by: 1.25, anchor: center)
                case .zoomOut:
                    editor.zoom(by: 0.8, anchor: center)
                case .fitToWindow:
                    editor.zoomToFit(canvasSize: canvasSize)
                }
                syncPanStart()
                viewModel.zoomRequest = nil
            }
        }
    }

    // MARK: - Case Navigation Bar

    private func caseNavigationBar(method: MethodModel) -> some View {
        HStack {
            Button(action: { viewModel.prevCase() }) {
                Image(systemName: "chevron.left")
            }
            .disabled(viewModel.selectedCaseIndex <= 0)

            Text("Case \(viewModel.selectedCaseIndex + 1) of \(method.caseCount)")
                .font(.caption)
                .fontWeight(.medium)

            Button(action: { viewModel.nextCase() }) {
                Image(systemName: "chevron.right")
            }
            .disabled(viewModel.selectedCaseIndex >= method.caseCount - 1)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 8))
        .padding(.top, 8)
    }

    // MARK: - Missing Library Banner

    private var missingLibraryBanner: some View {
        HStack {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(.yellow)
            Text("Missing libraries: \(viewModel.missingLibraries.map(\.name).joined(separator: ", "))")
                .font(.caption)
            Spacer()
            Button("Manage...") {
                viewModel.showLibraryManager = true
            }
            .font(.caption)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(.regularMaterial, in: RoundedRectangle(cornerRadius: 8))
        .padding(8)
    }

    // MARK: - Console Panel

    private var consolePanel: some View {
        VStack(spacing: 0) {
            // Drag handle
            Rectangle()
                .fill(Color.gray.opacity(0.3))
                .frame(height: 4)
                .frame(maxWidth: .infinity)
                .contentShape(Rectangle())
                .gesture(
                    DragGesture()
                        .onChanged { value in
                            consoleHeight = max(40, min(300, consoleHeight - value.translation.height))
                        }
                )
                .onHover { hovering in
                    if hovering {
                        NSCursor.resizeUpDown.push()
                    } else {
                        NSCursor.pop()
                    }
                }

            // Console header
            HStack {
                Text("Console")
                    .font(.caption)
                    .fontWeight(.medium)
                Spacer()
                Button(action: { viewModel.clearConsole() }) {
                    Image(systemName: "trash")
                        .font(.caption)
                }
                .buttonStyle(.plain)
                Button(action: { viewModel.showConsole = false }) {
                    Image(systemName: "xmark")
                        .font(.caption)
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(Color.black.opacity(0.9))
            .foregroundColor(.white)

            // Console content
            ScrollViewReader { proxy in
                ScrollView {
                    Text(viewModel.consoleOutput)
                        .font(.system(.caption, design: .monospaced))
                        .foregroundColor(.white)
                        .padding(8)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .id("consoleBottom")
                }
                .onChange(of: viewModel.consoleOutput) { _ in
                    proxy.scrollTo("consoleBottom", anchor: .bottom)
                }
            }
            .frame(height: consoleHeight)
            .background(Color.black.opacity(0.8))
        }
    }

    private func centerGraph(in size: CGSize) {
        guard let graph = viewModel.currentGraph, !graph.nodes.isEmpty else { return }

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

        let scaleX = (size.width - margin * 2) / max(graphW, 1)
        let scaleY = (size.height - margin * 2) / max(graphH, 1)
        let scale = max(min(min(scaleX, scaleY), 1.0), 0.6)

        let offsetX = (size.width - graphW * scale) / 2 - minX * scale + margin / 2
        let offsetY = (size.height - graphH * scale) / 2 - minY * scale + margin / 2

        editor.zoomScale = scale
        editor.panOffset = CGPoint(x: offsetX, y: offsetY)
    }

    @ViewBuilder
    private var graphContent: some View {
        let _ = viewModel.graphRevision
        if let graph = viewModel.currentGraph {
            ZStack {
                // Wires
                ForEach(graph.wires) { wire in
                    wireView(wire: wire, graph: graph)
                }
                // Nodes
                ForEach(graph.nodes) { node in
                    nodeView(node: node, isSelected: graph.selectedNodeIds.contains(node.id))
                }
                // Wire being dragged (rendered in graph space)
                if editor.isDraggingWire {
                    wireDragPreview
                }
            }
        }
    }

    /// Preview wire while dragging — rendered in graph coordinates
    private var wireDragPreview: some View {
        let startPt = editor.wireStartGraph
        let endPt = editor.wireEndGraph
        return Path { path in
            path.move(to: startPt)
            let dy = max(abs(endPt.y - startPt.y) * 0.4, 30)
            path.addCurve(
                to: endPt,
                control1: CGPoint(x: startPt.x, y: startPt.y + dy),
                control2: CGPoint(x: endPt.x, y: endPt.y - dy)
            )
        }
        .stroke(Color.blue.opacity(0.8), style: StrokeStyle(lineWidth: 2, dash: [6, 4]))
    }

    // MARK: - Node rendering

    private static let pinRadius: CGFloat = 6
    /// Hit target radius for pins (larger than visual for easier clicking)
    private static let pinHitRadius: CGFloat = 14

    private func nodeView(node: GraphNodeModel, isSelected: Bool) -> some View {
        let colors = nodeColors(for: node.nodeType)
        let hasBreakpoint = viewModel.debugger.breakpointNodeIds.contains(String(node.engineNodeId))
        let isPausedHere = viewModel.pausedNodeId == node.engineNodeId && node.engineNodeId != 0

        return ZStack {
            VStack(spacing: 0) {
                if node.inputPins.count > 0 {
                    Spacer().frame(height: Self.pinRadius)
                }
                Text(node.label)
                    .font(.system(size: 14, weight: .semibold))
                    .foregroundColor(colors.headerText)
                    .lineLimit(1)
                    .frame(maxWidth: .infinity)
                    .padding(.horizontal, 10)
                    .padding(.vertical, 8)
                    .background(colors.header)
                if node.outputPins.count > 0 {
                    Spacer().frame(height: Self.pinRadius)
                }
            }
            .frame(width: node.width, height: node.height)
            .background(isPausedHere ? Color.yellow.opacity(0.3) : colors.body)
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(isPausedHere ? Color.yellow : (isSelected ? Color.blue : colors.border),
                            lineWidth: isPausedHere ? 3 : (isSelected ? 2.5 : 1))
            )

            // Breakpoint indicator (red dot, top-left)
            if hasBreakpoint {
                Circle()
                    .fill(Color.red)
                    .frame(width: 10, height: 10)
                    .position(x: 8, y: 8)
            }

            // Input pins along top edge
            ForEach(Array(node.inputPins.enumerated()), id: \.element.id) { i, pin in
                let px = pinX(index: i, count: node.inputPins.count, nodeWidth: node.width)
                pinCircle(
                    fill: Color(red: 0.2, green: 0.5, blue: 0.9),
                    hasWire: viewModel.currentGraph?.wireToInput(nodeId: node.id, pinIndex: i) != nil
                )
                .position(x: px, y: 0)
            }

            // Output pins along bottom edge
            ForEach(Array(node.outputPins.enumerated()), id: \.element.id) { i, pin in
                let px = pinX(index: i, count: node.outputPins.count, nodeWidth: node.width)
                pinCircle(
                    fill: Color(red: 0.9, green: 0.3, blue: 0.2),
                    hasWire: true // output pins always look active
                )
                .position(x: px, y: node.height)
            }

            // Annotation badge (top-right corner)
            if node.annotation != .none {
                annotationBadge(node.annotation)
                    .position(x: node.width - 10, y: 10)
            }
        }
        .frame(width: node.width, height: node.height)
        .shadow(color: .black.opacity(0.15), radius: 2, x: 0, y: 1)
        .position(x: node.x + node.width / 2, y: node.y + node.height / 2)
        .gesture(
            DragGesture(minimumDistance: 3)
                .onChanged { value in
                    if !editor.isDraggingNode && !editor.isDraggingWire {
                        // First drag movement — decide: pin drag or node drag?
                        let start = value.startLocation // in node-local coords (0,0 = top-left)

                        // Check output pins (bottom edge)
                        for (i, _) in node.outputPins.enumerated() {
                            let pinLocalPos = CGPoint(
                                x: pinX(index: i, count: node.outputPins.count, nodeWidth: node.width),
                                y: node.height
                            )
                            if hypot(start.x - pinLocalPos.x, start.y - pinLocalPos.y) < Self.pinHitRadius {
                                let graphPos = CGPoint(x: node.x + pinLocalPos.x, y: node.y + pinLocalPos.y)
                                editor.beginWireDragFromOutput(nodeId: node.id, pinIndex: i, graphPos: graphPos)
                                return
                            }
                        }

                        // Check input pins (top edge) — detach existing wire if any
                        for (i, _) in node.inputPins.enumerated() {
                            let pinLocalPos = CGPoint(
                                x: pinX(index: i, count: node.inputPins.count, nodeWidth: node.width),
                                y: 0
                            )
                            if hypot(start.x - pinLocalPos.x, start.y - pinLocalPos.y) < Self.pinHitRadius {
                                let graphPos = CGPoint(x: node.x + pinLocalPos.x, y: node.y + pinLocalPos.y)
                                if editor.detachAndDragFromInput(nodeId: node.id, pinIndex: i, graphPos: graphPos) {
                                    return
                                }
                                // No wire to detach — could start a new wire from input (drag upward)
                                // For now, fall through to node drag
                            }
                        }

                        // Not near a pin → node drag
                        let graphPt = editor.screenToGraph(value.location)
                        editor.beginNodeDrag(nodeId: node.id, at: graphPt)
                    }

                    if editor.isDraggingWire {
                        // Convert drag location from node-local to graph coords
                        let graphPt = CGPoint(x: node.x + value.location.x, y: node.y + value.location.y)
                        editor.updateWireDragGraph(to: graphPt)
                    } else if editor.isDraggingNode {
                        let graphPt = editor.screenToGraph(value.location)
                        editor.updateNodeDrag(to: graphPt)
                    }
                }
                .onEnded { _ in
                    if editor.isDraggingWire {
                        editor.endWireDragAndConnect()
                    } else {
                        editor.endNodeDrag()
                    }
                }
        )
        .onTapGesture {
            viewModel.currentGraph?.selectNode(id: node.id)
        }
    }

    /// Pin circle with larger invisible hit area
    private func pinCircle(fill: Color, hasWire: Bool, isOptional: Bool = false, isHot: Bool = true) -> some View {
        ZStack {
            if isOptional {
                // Phase 12: dashed circle for optional pins
                Circle()
                    .stroke(fill, style: StrokeStyle(lineWidth: 1.5, dash: [3, 2]))
                    .frame(width: Self.pinRadius * 2, height: Self.pinRadius * 2)
                if hasWire {
                    Circle()
                        .fill(fill)
                        .frame(width: Self.pinRadius * 2 - 2, height: Self.pinRadius * 2 - 2)
                }
            } else if !isHot {
                // Phase 12: hollow fill for cold pins
                Circle()
                    .stroke(fill, lineWidth: 1.5)
                    .frame(width: Self.pinRadius * 2, height: Self.pinRadius * 2)
            } else {
                // Normal pin
                Circle()
                    .fill(fill)
                    .frame(width: Self.pinRadius * 2, height: Self.pinRadius * 2)
            }
            // Connected indicator ring
            if hasWire {
                Circle()
                    .stroke(fill.opacity(0.4), lineWidth: 1.5)
                    .frame(width: Self.pinRadius * 2 + 4, height: Self.pinRadius * 2 + 4)
            }
        }
    }

    @ViewBuilder
    private func annotationBadge(_ annotation: GraphNodeModel.NodeAnnotation) -> some View {
        let (icon, color) = annotationStyle(annotation)
        Image(systemName: icon)
            .font(.system(size: 10, weight: .bold))
            .foregroundColor(.white)
            .frame(width: 18, height: 18)
            .background(color, in: Circle())
    }

    private func annotationStyle(_ annotation: GraphNodeModel.NodeAnnotation) -> (String, Color) {
        switch annotation {
        case .none:
            return ("questionmark", .gray)
        case .loop:
            return ("arrow.triangle.2.circlepath", .blue)
        case .listMap:
            return ("ellipsis", .purple)
        case .partition:
            return ("arrow.triangle.branch", .orange)
        case .inject:
            return ("arrow.right.circle", .teal)
        case .nextCaseOnFailure, .nextCaseOnSuccess:
            return ("arrow.right", .yellow)
        case .continueOnFailure:
            return ("arrow.forward", .green)
        case .terminateOnSuccess, .terminateOnFailure:
            return ("stop.fill", .red)
        case .finishOnSuccess, .finishOnFailure:
            return ("checkmark.circle", .green)
        case .failOnSuccess, .failOnFailure:
            return ("xmark.circle", .red)
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
        case "instance_generator":
            return NodeColors(
                header: Color(red: 0.8, green: 0.5, blue: 0.0),
                headerText: .white,
                body: Color(red: 0.98, green: 0.94, blue: 0.85),
                border: Color(red: 0.8, green: 0.5, blue: 0.0).opacity(0.5)
            )
        case "get":
            return NodeColors(
                header: Color(red: 0.2, green: 0.5, blue: 0.45),
                headerText: .white,
                body: Color(red: 0.88, green: 0.96, blue: 0.94),
                border: Color(red: 0.2, green: 0.5, blue: 0.45).opacity(0.5)
            )
        case "set":
            return NodeColors(
                header: Color(red: 0.55, green: 0.3, blue: 0.2),
                headerText: .white,
                body: Color(red: 0.97, green: 0.92, blue: 0.88),
                border: Color(red: 0.55, green: 0.3, blue: 0.2).opacity(0.5)
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

    // MARK: - Pin positioning

    private func pinX(index: Int, count: Int, nodeWidth: CGFloat) -> CGFloat {
        CGFloat(index + 1) * nodeWidth / CGFloat(count + 1)
    }

    // MARK: - Wire rendering

    /// Compute bezier geometry for a wire. Returns (start, end, control1, control2) or nil if can't resolve.
    private func wireGeometry(wire: GraphWireModel, graph: GraphModel) -> (start: CGPoint, end: CGPoint, c1: CGPoint, c2: CGPoint)? {
        guard let src = graph.nodes.first(where: { $0.id == wire.sourceNodeId }) else { return nil }
        let startX = src.x + pinX(index: wire.sourcePin, count: src.outputPins.count, nodeWidth: src.width)
        let startY = src.y + src.height
        let start = CGPoint(x: startX, y: startY)

        let end: CGPoint
        if let destId = wire.destNodeId, let destPin = wire.destPin,
           let dst = graph.nodes.first(where: { $0.id == destId }) {
            let endX = dst.x + pinX(index: destPin, count: dst.inputPins.count, nodeWidth: dst.width)
            end = CGPoint(x: endX, y: dst.y)
        } else if let dangling = wire.danglingEndpoint {
            end = dangling
        } else {
            return nil
        }

        let dy = max(abs(end.y - start.y) * 0.4, 30)
        let c1 = CGPoint(x: start.x, y: start.y + dy)
        let c2 = CGPoint(x: end.x, y: end.y - dy)
        return (start, end, c1, c2)
    }

    /// Cubic bezier midpoint at t=0.5
    private func bezierMidpoint(p0: CGPoint, c1: CGPoint, c2: CGPoint, p3: CGPoint) -> CGPoint {
        // B(0.5) = P0/8 + 3*C1/8 + 3*C2/8 + P3/8
        CGPoint(
            x: p0.x / 8 + 3 * c1.x / 8 + 3 * c2.x / 8 + p3.x / 8,
            y: p0.y / 8 + 3 * c1.y / 8 + 3 * c2.y / 8 + p3.y / 8
        )
    }

    private func wireView(wire: GraphWireModel, graph: GraphModel) -> some View {
        let geo = wireGeometry(wire: wire, graph: graph)
        let isSelected = graph.selectedWireIds.contains(wire.id)
        let isDangling = wire.destNodeId == nil

        return Group {
            if let g = geo {
                let path = Path { p in
                    p.move(to: g.start)
                    p.addCurve(to: g.end, control1: g.c1, control2: g.c2)
                }

                // Invisible wide stroke for hit testing
                path.stroke(Color.clear, lineWidth: 10)
                    .contentShape(path.strokedPath(StrokeStyle(lineWidth: 10)))
                    .onTapGesture {
                        graph.selectWire(id: wire.id)
                    }

                // Visual stroke
                if isSelected {
                    path.stroke(Color.blue, lineWidth: 2)
                } else if isDangling {
                    path.stroke(Color.orange, style: StrokeStyle(lineWidth: 1.5, dash: [6, 4]))
                } else if wire.isExecution {
                    // Phase 11: execution wires rendered as dashed dark gray with arrow
                    path.stroke(Color.gray.opacity(0.7), style: StrokeStyle(lineWidth: 2, dash: [8, 4]))
                } else {
                    path.stroke(Color.black.opacity(0.55), lineWidth: 1.5)
                }

                // Wire name label at midpoint
                if !wire.name.isEmpty {
                    let mid = bezierMidpoint(p0: g.start, c1: g.c1, c2: g.c2, p3: g.end)
                    Text(wire.name)
                        .font(.system(size: 10))
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Color.white)
                                .shadow(color: .black.opacity(0.15), radius: 1, x: 0, y: 0.5)
                        )
                        .position(mid)
                }

                // Dangling endpoint circle (draggable)
                if isDangling, let dangling = wire.danglingEndpoint {
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 10, height: 10)
                        .position(dangling)
                        .gesture(
                            DragGesture(minimumDistance: 1)
                                .onChanged { value in
                                    // Update dangling endpoint position in graph coords
                                    // value.location is relative to the circle's position frame
                                    let newPos = CGPoint(
                                        x: dangling.x + value.translation.width,
                                        y: dangling.y + value.translation.height
                                    )
                                    wire.danglingEndpoint = newPos
                                    graph.objectWillChange.send()
                                }
                                .onEnded { value in
                                    let finalPos = CGPoint(
                                        x: dangling.x + value.translation.width,
                                        y: dangling.y + value.translation.height
                                    )
                                    // Try to snap to a nearby input pin
                                    if let target = graph.findNearestInputPin(at: finalPos, threshold: 25),
                                       target.nodeId != wire.sourceNodeId {
                                        // Remove any existing wire to this input
                                        if let existing = graph.wireToInput(nodeId: target.nodeId, pinIndex: target.pinIndex),
                                           existing.id != wire.id {
                                            graph.removeWire(id: existing.id)
                                        }
                                        graph.reconnectWire(id: wire.id, destNodeId: target.nodeId, destPin: target.pinIndex)
                                    } else {
                                        wire.danglingEndpoint = finalPos
                                        graph.objectWillChange.send()
                                    }
                                }
                        )
                }
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

    // MARK: - Key event monitor

    @State private var keyMonitor: Any?

    private func installKeyMonitor() {
        keyMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { event in
            // Delete / Backspace
            if event.keyCode == 51 || event.keyCode == 117 {
                if event.modifierFlags.intersection(.deviceIndependentFlagsMask).isEmpty {
                    viewModel.deleteSelected()
                    return nil
                }
            }
            // Cmd+A — Select All
            if event.charactersIgnoringModifiers == "a" && event.modifierFlags.contains(.command) && !event.modifierFlags.contains(.shift) {
                viewModel.selectAll()
                return nil
            }
            // Cmd+D — Duplicate
            if event.charactersIgnoringModifiers == "d" && event.modifierFlags.contains(.command) {
                viewModel.duplicateSelected()
                return nil
            }
            return event
        }
    }

    private func removeKeyMonitor() {
        if let monitor = keyMonitor {
            NSEvent.removeMonitor(monitor)
            keyMonitor = nil
        }
    }

    // MARK: - Right-click event monitor

    @State private var rightClickMonitor: Any?

    private func installRightClickMonitor() {
        rightClickMonitor = NSEvent.addLocalMonitorForEvents(matching: .rightMouseDown) { [self] event in
            guard let nsView = canvasNSView else { return event }
            let locationInView = nsView.convert(event.locationInWindow, from: nil)
            guard nsView.bounds.contains(locationInView) else { return event }
            showContextMenu(at: locationInView, in: nsView)
            return nil
        }
    }

    private func removeRightClickMonitor() {
        if let monitor = rightClickMonitor {
            NSEvent.removeMonitor(monitor)
            rightClickMonitor = nil
        }
    }

    // MARK: - Right-click context menu (native NSMenu)

    private func showContextMenu(at location: NSPoint, in view: NSView) {
        let menu = NSMenu(title: "Insert Node")

        func addPrim(_ name: String, inputs: Int, outputs: Int, to parent: NSMenu, libraryName: String? = nil) {
            let item = NSMenuItem(title: name, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
            item.representedObject = { [weak viewModel] in
                self.insertPrimitive(name: name, inputs: inputs, outputs: outputs, libraryName: libraryName)
            } as () -> Void
            item.target = CanvasMenuTarget.shared
            parent.addItem(item)
        }

        let arith = NSMenuItem(title: "Arithmetic", action: nil, keyEquivalent: "")
        let arithSub = NSMenu()
        for name in ["+", "-", "*", "/", "mod", "abs", "round"] { addPrim(name, inputs: 2, outputs: 1, to: arithSub) }
        arith.submenu = arithSub
        menu.addItem(arith)

        let cmp = NSMenuItem(title: "Compare", action: nil, keyEquivalent: "")
        let cmpSub = NSMenu()
        for name in ["=", "<", ">", "<=", ">=", "!="] { addPrim(name, inputs: 2, outputs: 1, to: cmpSub) }
        cmp.submenu = cmpSub
        menu.addItem(cmp)

        let logic = NSMenuItem(title: "Logic", action: nil, keyEquivalent: "")
        let logicSub = NSMenu()
        addPrim("and", inputs: 2, outputs: 1, to: logicSub)
        addPrim("or", inputs: 2, outputs: 1, to: logicSub)
        addPrim("not", inputs: 1, outputs: 1, to: logicSub)
        addPrim("if", inputs: 3, outputs: 1, to: logicSub)
        logic.submenu = logicSub
        menu.addItem(logic)

        let str = NSMenuItem(title: "String", action: nil, keyEquivalent: "")
        let strSub = NSMenu()
        for name in ["concat", "length", "to-string", "split", "trim", "replace"] { addPrim(name, inputs: 2, outputs: 1, to: strSub) }
        str.submenu = strSub
        menu.addItem(str)

        let list = NSMenuItem(title: "List", action: nil, keyEquivalent: "")
        let listSub = NSMenu()
        for name in ["get-nth", "append", "sort", "empty?", "length", "map", "filter"] { addPrim(name, inputs: 2, outputs: 1, to: listSub) }
        list.submenu = listSub
        menu.addItem(list)

        let dict = NSMenuItem(title: "Dict", action: nil, keyEquivalent: "")
        let dictSub = NSMenu()
        addPrim("dict-create", inputs: 0, outputs: 1, to: dictSub)
        addPrim("dict-get", inputs: 2, outputs: 1, to: dictSub)
        addPrim("dict-set", inputs: 3, outputs: 1, to: dictSub)
        dict.submenu = dictSub
        menu.addItem(dict)

        menu.addItem(NSMenuItem.separator())

        let constItem = NSMenuItem(title: "Constant", action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
        constItem.representedObject = { [weak viewModel] in
            self.insertConstant()
        } as () -> Void
        constItem.target = CanvasMenuTarget.shared
        menu.addItem(constItem)

        let io = NSMenuItem(title: "I/O", action: nil, keyEquivalent: "")
        let ioSub = NSMenu()
        addPrim("log", inputs: 1, outputs: 1, to: ioSub)
        addPrim("inspect", inputs: 1, outputs: 1, to: ioSub)
        io.submenu = ioSub
        menu.addItem(io)

        // Library submenus
        let libs = viewModel.libraryManager.libraries
        if !libs.isEmpty {
            menu.addItem(NSMenuItem.separator())
            for lib in libs {
                let libItem = NSMenuItem(title: lib.manifest.name, action: nil, keyEquivalent: "")
                let libSub = NSMenu()
                for prim in lib.manifest.primitives {
                    addPrim(prim.name, inputs: prim.num_inputs, outputs: prim.num_outputs, to: libSub, libraryName: lib.manifest.name)
                }
                libItem.submenu = libSub
                menu.addItem(libItem)
            }
        }

        // Classes submenu
        let allClasses = viewModel.project?.sections.flatMap(\.classes) ?? []
        if !allClasses.isEmpty {
            menu.addItem(NSMenuItem.separator())
            let classesItem = NSMenuItem(title: "Classes", action: nil, keyEquivalent: "")
            let classesSub = NSMenu()

            for classDef in allClasses {
                let classItem = NSMenuItem(title: classDef.name, action: nil, keyEquivalent: "")
                let classSub = NSMenu()

                // "new ClassName"
                let newItem = NSMenuItem(title: "new \(classDef.name)", action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
                newItem.representedObject = { [weak viewModel] in
                    self.insertInstanceGenerator(className: classDef.name)
                } as () -> Void
                newItem.target = CanvasMenuTarget.shared
                classSub.addItem(newItem)

                // Get submenu
                if !classDef.attributes.isEmpty {
                    let getItem = NSMenuItem(title: "Get", action: nil, keyEquivalent: "")
                    let getSub = NSMenu()
                    for attr in classDef.attributes {
                        let item = NSMenuItem(title: attr.name, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
                        item.representedObject = { [weak viewModel] in
                            self.insertGetNode(className: classDef.name, attrName: attr.name)
                        } as () -> Void
                        item.target = CanvasMenuTarget.shared
                        getSub.addItem(item)
                    }
                    getItem.submenu = getSub
                    classSub.addItem(getItem)

                    // Set submenu
                    let setItem = NSMenuItem(title: "Set", action: nil, keyEquivalent: "")
                    let setSub = NSMenu()
                    for attr in classDef.attributes {
                        let item = NSMenuItem(title: attr.name, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
                        item.representedObject = { [weak viewModel] in
                            self.insertSetNode(className: classDef.name, attrName: attr.name)
                        } as () -> Void
                        item.target = CanvasMenuTarget.shared
                        setSub.addItem(item)
                    }
                    setItem.submenu = setSub
                    classSub.addItem(setItem)
                }

                // Class methods
                for method in classDef.methods {
                    let mItem = NSMenuItem(title: method.name, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
                    mItem.representedObject = { [weak viewModel] in
                        self.insertMethodCall(name: method.name, inputs: method.numInputs, outputs: method.numOutputs)
                    } as () -> Void
                    mItem.target = CanvasMenuTarget.shared
                    classSub.addItem(mItem)
                }

                classItem.submenu = classSub
                classesSub.addItem(classItem)
            }

            classesItem.submenu = classesSub
            menu.addItem(classesItem)
        }

        // Methods submenu (universal methods across all sections)
        let allMethods = viewModel.project?.sections.flatMap(\.methods) ?? []
        if !allMethods.isEmpty {
            let methodsItem = NSMenuItem(title: "Methods", action: nil, keyEquivalent: "")
            let methodsSub = NSMenu()
            for method in allMethods {
                let mItem = NSMenuItem(title: method.name, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
                mItem.representedObject = { [weak viewModel] in
                    self.insertMethodCall(name: method.name, inputs: method.numInputs, outputs: method.numOutputs)
                } as () -> Void
                mItem.target = CanvasMenuTarget.shared
                methodsSub.addItem(mItem)
            }
            methodsItem.submenu = methodsSub
            menu.addItem(methodsItem)
        }

        // Breakpoint toggle (if clicking on a node)
        let graphPt = editor.screenToGraph(CGPoint(x: location.x, y: view.bounds.height - location.y))
        if let node = viewModel.currentGraph?.nodeAt(point: graphPt), node.engineNodeId != 0 {
            menu.addItem(NSMenuItem.separator())
            let bpTitle = viewModel.debugger.breakpointNodeIds.contains(String(node.engineNodeId))
                ? "Remove Breakpoint" : "Toggle Breakpoint"
            let bpItem = NSMenuItem(title: bpTitle, action: #selector(CanvasMenuTarget.menuAction(_:)), keyEquivalent: "")
            let engineId = node.engineNodeId
            bpItem.representedObject = { [weak viewModel] in
                viewModel?.toggleBreakpoint(nodeId: engineId)
            } as () -> Void
            bpItem.target = CanvasMenuTarget.shared
            menu.addItem(bpItem)
        }

        menu.popUp(positioning: nil, at: location, in: view)
    }

    private var insertionPoint: CGPoint {
        editor.screenToGraph(CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2))
    }

    private func insertPrimitive(name: String, inputs: Int, outputs: Int, libraryName: String? = nil) {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: name, nodeType: "primitive")
        for i in 0..<inputs {
            node.inputPins.append(PinModel(name: "in\(i)", index: i))
        }
        for i in 0..<outputs {
            node.outputPins.append(PinModel(name: "out\(i)", index: i))
        }
        node.libraryName = libraryName
        node.height = node.computeHeight()
        let labelWidth = CGFloat(name.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }

    private func insertConstant() {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: "0", nodeType: "constant")
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.constantValue = "0"
        node.height = node.computeHeight()
        graph.addNode(node)
    }

    private func insertInstanceGenerator(className: String) {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let label = "new \(className)"
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: label, nodeType: "instance_generator")
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.height = node.computeHeight()
        let labelWidth = CGFloat(label.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }

    private func insertGetNode(className: String, attrName: String) {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let label = "get \(attrName)"
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: label, nodeType: "get")
        node.inputPins.append(PinModel(name: "in0", index: 0))
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.outputPins.append(PinModel(name: "out1", index: 1))
        node.height = node.computeHeight()
        let labelWidth = CGFloat(label.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }

    private func insertSetNode(className: String, attrName: String) {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let label = "set \(attrName)"
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: label, nodeType: "set")
        node.inputPins.append(PinModel(name: "in0", index: 0))
        node.inputPins.append(PinModel(name: "in1", index: 1))
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.height = node.computeHeight()
        let labelWidth = CGFloat(label.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }

    private func insertMethodCall(name: String, inputs: Int, outputs: Int) {
        guard let graph = viewModel.currentGraph else { return }
        let pt = insertionPoint
        let node = GraphNodeModel(x: pt.x - 80, y: pt.y - 20, label: name, nodeType: "method_call")
        for i in 0..<inputs {
            node.inputPins.append(PinModel(name: "in\(i)", index: i))
        }
        for i in 0..<outputs {
            node.outputPins.append(PinModel(name: "out\(i)", index: i))
        }
        node.height = node.computeHeight()
        let labelWidth = CGFloat(name.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }
}

// MARK: - Native NSMenu support for right-click

class CanvasMenuTarget: NSObject {
    static let shared = CanvasMenuTarget()

    @objc func menuAction(_ sender: NSMenuItem) {
        if let action = sender.representedObject as? () -> Void {
            action()
        }
    }
}

struct CanvasNSViewFinder: NSViewRepresentable {
    @Binding var nsView: NSView?

    func makeNSView(context: Context) -> NSView {
        let view = NSView()
        DispatchQueue.main.async { self.nsView = view }
        return view
    }

    func updateNSView(_ nsView: NSView, context: Context) {
        DispatchQueue.main.async { self.nsView = nsView }
    }
}
