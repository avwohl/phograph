import Foundation
import Combine
import CoreGraphics

/// Manages canvas interaction: zoom, pan, selection, wire dragging.
class GraphEditorViewModel: ObservableObject {
    @Published var panOffset: CGPoint = .zero
    @Published var zoomScale: CGFloat = 1.0
    @Published var isDraggingNode: Bool = false
    @Published var isDraggingWire: Bool = false
    @Published var wireStartPoint: CGPoint = .zero
    @Published var wireEndPoint: CGPoint = .zero
    @Published var selectionRect: CGRect? = nil

    weak var graph: GraphModel?
    private var dragNodeId: UUID?
    private var dragStartOffset: CGPoint = .zero
    private var wireSourceNodeId: UUID?
    private var wireSourcePinIndex: Int = 0

    // Convert screen point to graph coordinates
    func screenToGraph(_ point: CGPoint) -> CGPoint {
        CGPoint(
            x: (point.x - panOffset.x) / zoomScale,
            y: (point.y - panOffset.y) / zoomScale
        )
    }

    // Convert graph point to screen coordinates
    func graphToScreen(_ point: CGPoint) -> CGPoint {
        CGPoint(
            x: point.x * zoomScale + panOffset.x,
            y: point.y * zoomScale + panOffset.y
        )
    }

    // MARK: - Pan & Zoom

    func pan(by delta: CGSize) {
        panOffset.x += delta.width
        panOffset.y += delta.height
    }

    func zoom(by factor: CGFloat, anchor: CGPoint) {
        let oldScale = zoomScale
        zoomScale = max(0.1, min(5.0, zoomScale * factor))
        let scaleChange = zoomScale / oldScale
        panOffset.x = anchor.x - (anchor.x - panOffset.x) * scaleChange
        panOffset.y = anchor.y - (anchor.y - panOffset.y) * scaleChange
    }

    func zoomToFit(canvasSize: CGSize) {
        guard let graph = graph, !graph.nodes.isEmpty else { return }
        var minX = CGFloat.infinity, minY = CGFloat.infinity
        var maxX = -CGFloat.infinity, maxY = -CGFloat.infinity

        for node in graph.nodes {
            minX = min(minX, node.x)
            minY = min(minY, node.y)
            maxX = max(maxX, node.x + node.width)
            maxY = max(maxY, node.y + node.height)
        }

        let graphWidth = maxX - minX + 100
        let graphHeight = maxY - minY + 100
        let scaleX = canvasSize.width / graphWidth
        let scaleY = canvasSize.height / graphHeight
        zoomScale = min(scaleX, scaleY, 2.0)
        panOffset = CGPoint(
            x: (canvasSize.width - graphWidth * zoomScale) / 2 - minX * zoomScale + 50 * zoomScale,
            y: (canvasSize.height - graphHeight * zoomScale) / 2 - minY * zoomScale + 50 * zoomScale
        )
    }

    // MARK: - Node Interaction

    func beginNodeDrag(nodeId: UUID, at point: CGPoint) {
        guard let graph = graph,
              let node = graph.nodes.first(where: { $0.id == nodeId }) else { return }
        dragNodeId = nodeId
        isDraggingNode = true
        dragStartOffset = CGPoint(x: point.x - node.x, y: point.y - node.y)
        graph.selectNode(id: nodeId)
    }

    func updateNodeDrag(to point: CGPoint) {
        guard isDraggingNode, let nodeId = dragNodeId,
              let node = graph?.nodes.first(where: { $0.id == nodeId }) else { return }
        node.x = point.x - dragStartOffset.x
        node.y = point.y - dragStartOffset.y
    }

    func endNodeDrag() {
        isDraggingNode = false
        dragNodeId = nil
    }

    // MARK: - Wire Dragging

    func beginWireDrag(nodeId: UUID, pinIndex: Int, screenPoint: CGPoint) {
        wireSourceNodeId = nodeId
        wireSourcePinIndex = pinIndex
        wireStartPoint = screenPoint
        wireEndPoint = screenPoint
        isDraggingWire = true
    }

    func updateWireDrag(to screenPoint: CGPoint) {
        wireEndPoint = screenPoint
    }

    func endWireDrag(at screenPoint: CGPoint) -> (UUID, Int)? {
        isDraggingWire = false
        guard let sourceId = wireSourceNodeId else { return nil }

        let graphPoint = screenToGraph(screenPoint)
        // Find target node/pin at this position
        if let targetNode = graph?.nodeAt(point: graphPoint),
           targetNode.id != sourceId {
            // Create wire
            let wire = GraphWireModel(
                sourceNodeId: sourceId,
                sourcePin: wireSourcePinIndex,
                destNodeId: targetNode.id,
                destPin: 0 // Simplified: connect to first input
            )
            graph?.addWire(wire)
            return (targetNode.id, 0)
        }

        wireSourceNodeId = nil
        return nil
    }

    // MARK: - Selection

    func beginSelection(at point: CGPoint) {
        selectionRect = CGRect(origin: point, size: .zero)
    }

    func updateSelection(to point: CGPoint) {
        guard let origin = selectionRect?.origin else { return }
        selectionRect = CGRect(
            x: min(origin.x, point.x),
            y: min(origin.y, point.y),
            width: abs(point.x - origin.x),
            height: abs(point.y - origin.y)
        )
    }

    func endSelection() {
        guard let rect = selectionRect, let graph = graph else {
            selectionRect = nil
            return
        }
        graph.selectedNodeIds.removeAll()
        for node in graph.nodes {
            let nodeRect = CGRect(x: node.x, y: node.y, width: node.width, height: node.height)
            if rect.intersects(nodeRect) {
                graph.selectedNodeIds.insert(node.id)
            }
        }
        selectionRect = nil
    }

    func deleteSelected() {
        guard let graph = graph else { return }
        for id in graph.selectedNodeIds {
            graph.removeNode(id: id)
        }
        graph.selectedNodeIds.removeAll()
    }
}
