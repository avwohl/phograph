import Foundation
import Combine
import CoreGraphics

/// Manages canvas interaction: zoom, pan, selection, wire dragging.
class GraphEditorViewModel: ObservableObject {
    @Published var panOffset: CGPoint = .zero
    @Published var zoomScale: CGFloat = 1.0
    @Published var isDraggingNode: Bool = false
    @Published var isDraggingWire: Bool = false
    @Published var selectionRect: CGRect? = nil

    /// Wire drag endpoints in graph coordinates (rendered inside graphContent)
    @Published var wireStartGraph: CGPoint = .zero
    @Published var wireEndGraph: CGPoint = .zero

    /// Legacy screen-space endpoints (kept for compatibility)
    @Published var wireStartPoint: CGPoint = .zero
    @Published var wireEndPoint: CGPoint = .zero

    /// Which direction we're dragging: from an output (looking for input) or from an input (looking for output)
    enum WireDragDirection { case fromOutput, fromInput }
    var wireDragDirection: WireDragDirection = .fromOutput

    /// The source pin of the wire being dragged (always the output end)
    var wireDragSourceNodeId: UUID?
    var wireDragSourcePinIndex: Int = 0

    /// When reconnecting an existing wire (preserves name), this holds the wire's ID
    var reconnectingWireId: UUID?

    weak var graph: GraphModel?
    private var dragNodeId: UUID?
    private var dragStartOffset: CGPoint = .zero

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

    /// Start dragging a new wire from an output pin
    func beginWireDragFromOutput(nodeId: UUID, pinIndex: Int, graphPos: CGPoint) {
        wireDragDirection = .fromOutput
        wireDragSourceNodeId = nodeId
        wireDragSourcePinIndex = pinIndex
        wireStartGraph = graphPos
        wireEndGraph = graphPos
        isDraggingWire = true
    }

    /// Detach an existing wire from an input pin and start dragging its loose end
    /// Returns true if a wire was detached
    @discardableResult
    func detachAndDragFromInput(nodeId: UUID, pinIndex: Int, graphPos: CGPoint) -> Bool {
        guard let graph = graph,
              let wire = graph.wireToInput(nodeId: nodeId, pinIndex: pinIndex) else {
            return false
        }
        // Remember the source (output) end of the detached wire
        let srcNodeId = wire.sourceNodeId
        let srcPin = wire.sourcePin

        // Disconnect the wire's dest (make it dangling) instead of deleting
        graph.disconnectWireDest(id: wire.id, endpoint: graphPos)
        reconnectingWireId = wire.id

        // Find the output pin's graph position
        if let srcNode = graph.nodes.first(where: { $0.id == srcNodeId }) {
            let px = CGFloat(srcPin + 1) * srcNode.width / CGFloat(srcNode.outputPins.count + 1)
            let srcGraphPos = CGPoint(x: srcNode.x + px, y: srcNode.y + srcNode.height)

            // Start dragging from the source output, looking for a new input to connect to
            wireDragDirection = .fromOutput
            wireDragSourceNodeId = srcNodeId
            wireDragSourcePinIndex = srcPin
            wireStartGraph = srcGraphPos
            wireEndGraph = graphPos
            isDraggingWire = true
            return true
        }
        return false
    }

    func updateWireDragGraph(to graphPoint: CGPoint) {
        wireEndGraph = graphPoint
    }

    /// End wire drag and try to connect. Returns true if a connection was made.
    @discardableResult
    func endWireDragAndConnect() -> Bool {
        isDraggingWire = false
        guard let graph = graph, let sourceNodeId = wireDragSourceNodeId else {
            reconnectingWireId = nil
            return false
        }

        let dropPoint = wireEndGraph
        var connected = false

        // We're dragging from an output, looking for an input pin to connect to
        if let target = graph.findNearestInputPin(at: dropPoint, threshold: 25 / zoomScale),
           target.nodeId != sourceNodeId {
            // Remove any existing wire to this input (one wire per input)
            if let existing = graph.wireToInput(nodeId: target.nodeId, pinIndex: target.pinIndex) {
                graph.removeWire(id: existing.id)
            }

            if let wireId = reconnectingWireId {
                // Reconnect existing wire (preserves name)
                graph.reconnectWire(id: wireId, destNodeId: target.nodeId, destPin: target.pinIndex)
            } else {
                // Create new wire
                let wire = GraphWireModel(
                    sourceNodeId: sourceNodeId,
                    sourcePin: wireDragSourcePinIndex,
                    destNodeId: target.nodeId,
                    destPin: target.pinIndex
                )
                graph.addWire(wire)
            }
            connected = true
        } else if let wireId = reconnectingWireId {
            // Dropped on empty space while reconnecting — leave dangling
            graph.disconnectWireDest(id: wireId, endpoint: dropPoint)
        } else {
            // Dropped on empty space with new wire — create dangling wire
            let wire = GraphWireModel(
                sourceNodeId: sourceNodeId,
                sourcePin: wireDragSourcePinIndex
            )
            wire.danglingEndpoint = dropPoint
            graph.addWire(wire)
        }

        wireDragSourceNodeId = nil
        reconnectingWireId = nil
        return connected
    }

    // Legacy wire drag methods (kept for compatibility)
    func beginWireDrag(nodeId: UUID, pinIndex: Int, screenPoint: CGPoint) {
        wireDragSourceNodeId = nodeId
        wireDragSourcePinIndex = pinIndex
        wireStartPoint = screenPoint
        wireEndPoint = screenPoint
        isDraggingWire = true
    }

    func updateWireDrag(to screenPoint: CGPoint) {
        wireEndPoint = screenPoint
    }

    func endWireDrag(at screenPoint: CGPoint) -> (UUID, Int)? {
        isDraggingWire = false
        guard let sourceId = wireDragSourceNodeId else { return nil }

        let graphPoint = screenToGraph(screenPoint)
        if let targetNode = graph?.nodeAt(point: graphPoint),
           targetNode.id != sourceId {
            let wire = GraphWireModel(
                sourceNodeId: sourceId,
                sourcePin: wireDragSourcePinIndex,
                destNodeId: targetNode.id,
                destPin: 0
            )
            graph?.addWire(wire)
            return (targetNode.id, 0)
        }

        wireDragSourceNodeId = nil
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
