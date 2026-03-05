import Foundation
import Combine

/// Observable graph model for SwiftUI binding.
/// Represents a single case's graph for display in the canvas.
class GraphModel: ObservableObject {
    @Published var nodes: [GraphNodeModel] = []
    @Published var wires: [GraphWireModel] = []
    @Published var selectedNodeIds: Set<UUID> = []
    @Published var selectedWireIds: Set<UUID> = []
    @Published var methodName: String = ""
    @Published var caseIndex: Int = 0

    /// Camera state
    @Published var panOffset: CGPoint = .zero
    @Published var zoomScale: CGFloat = 1.0

    func addNode(_ node: GraphNodeModel) {
        nodes.append(node)
    }

    func removeNode(id: UUID) {
        let removedWireIds = Set(wires.filter { $0.sourceNodeId == id || $0.destNodeId == id }.map(\.id))
        selectedWireIds.subtract(removedWireIds)
        nodes.removeAll { $0.id == id }
        wires.removeAll { $0.sourceNodeId == id || $0.destNodeId == id }
    }

    func addWire(_ wire: GraphWireModel) {
        wires.append(wire)
    }

    func removeWire(id: UUID) {
        wires.removeAll { $0.id == id }
        selectedWireIds.remove(id)
    }

    func selectNode(id: UUID, exclusive: Bool = true) {
        if exclusive {
            selectedNodeIds = [id]
            selectedWireIds.removeAll()
        } else {
            selectedNodeIds.insert(id)
        }
    }

    func selectWire(id: UUID, exclusive: Bool = true) {
        if exclusive {
            selectedWireIds = [id]
            selectedNodeIds.removeAll()
        } else {
            selectedWireIds.insert(id)
        }
    }

    func deselectAll() {
        selectedNodeIds.removeAll()
        selectedWireIds.removeAll()
    }

    /// Find a wire connected to a specific input pin
    func wireToInput(nodeId: UUID, pinIndex: Int) -> GraphWireModel? {
        wires.first { $0.destNodeId == nodeId && $0.destPin == pinIndex }
    }

    /// Reconnect a dangling wire to a destination pin
    func reconnectWire(id: UUID, destNodeId: UUID, destPin: Int) {
        guard let wire = wires.first(where: { $0.id == id }) else { return }
        wire.destNodeId = destNodeId
        wire.destPin = destPin
        wire.danglingEndpoint = nil
        objectWillChange.send()
    }

    /// Disconnect a wire's destination, making it dangling
    func disconnectWireDest(id: UUID, endpoint: CGPoint) {
        guard let wire = wires.first(where: { $0.id == id }) else { return }
        wire.destNodeId = nil
        wire.destPin = nil
        wire.danglingEndpoint = endpoint
        objectWillChange.send()
    }

    /// Find the nearest input pin to a graph point within a threshold
    func findNearestInputPin(at point: CGPoint, threshold: CGFloat = 20) -> (nodeId: UUID, pinIndex: Int, position: CGPoint)? {
        var best: (nodeId: UUID, pinIndex: Int, position: CGPoint)?
        var bestDist = threshold
        for node in nodes {
            for (i, _) in node.inputPins.enumerated() {
                let px = CGFloat(i + 1) * node.width / CGFloat(node.inputPins.count + 1)
                let pinPos = CGPoint(x: node.x + px, y: node.y)
                let d = hypot(point.x - pinPos.x, point.y - pinPos.y)
                if d < bestDist {
                    bestDist = d
                    best = (node.id, i, pinPos)
                }
            }
        }
        return best
    }

    /// Find the nearest output pin to a graph point within a threshold
    func findNearestOutputPin(at point: CGPoint, threshold: CGFloat = 20) -> (nodeId: UUID, pinIndex: Int, position: CGPoint)? {
        var best: (nodeId: UUID, pinIndex: Int, position: CGPoint)?
        var bestDist = threshold
        for node in nodes {
            for (i, _) in node.outputPins.enumerated() {
                let px = CGFloat(i + 1) * node.width / CGFloat(node.outputPins.count + 1)
                let pinPos = CGPoint(x: node.x + px, y: node.y + node.height)
                let d = hypot(point.x - pinPos.x, point.y - pinPos.y)
                if d < bestDist {
                    bestDist = d
                    best = (node.id, i, pinPos)
                }
            }
        }
        return best
    }

    func nodeAt(point: CGPoint) -> GraphNodeModel? {
        for node in nodes.reversed() {
            let rect = CGRect(x: node.x, y: node.y, width: node.width, height: node.height)
            if rect.contains(point) {
                return node
            }
        }
        return nil
    }

    /// Build a GraphModel from raw JSON case data
    static func fromCaseJSON(_ caseDict: [String: Any], methodName: String, caseIndex: Int) -> GraphModel {
        let graph = GraphModel()
        graph.methodName = methodName
        graph.caseIndex = caseIndex

        // Parse nodes
        var nodeIdToUUID: [Int: UUID] = [:]

        guard let nodesArray = caseDict["nodes"] as? [[String: Any]] else {
            return graph
        }

        let inputBarId = caseDict["input_bar_id"] as? Int ?? 0
        let outputBarId = caseDict["output_bar_id"] as? Int ?? 0

        // Auto-layout: arrange nodes in columns by topological depth
        // First pass: collect nodes and compute depth
        struct RawNode {
            var nodeId: Int
            var name: String
            var nodeType: String
            var numInputs: Int
            var numOutputs: Int
            var constantValue: String?
            var libraryName: String?
            var inputNames: [String]?
            var outputNames: [String]?
        }

        var rawNodes: [RawNode] = []
        for nodeDict in nodesArray {
            let nodeId = nodeDict["id"] as? Int ?? 0
            let typeStr = nodeDict["type"] as? String ?? "primitive"
            let name = nodeDict["name"] as? String ?? typeStr
            let numIn = nodeDict["num_inputs"] as? Int ?? 0
            let numOut = nodeDict["num_outputs"] as? Int ?? 0

            var constVal: String? = nil
            if typeStr == "constant", let valDict = nodeDict["value"] as? [String: Any] {
                if let v = valDict["value"] {
                    constVal = "\(v)"
                }
            }

            let libName = nodeDict["library"] as? String
            let inNames = nodeDict["input_names"] as? [String]
            let outNames = nodeDict["output_names"] as? [String]

            rawNodes.append(RawNode(
                nodeId: nodeId, name: name, nodeType: typeStr,
                numInputs: numIn, numOutputs: numOut, constantValue: constVal,
                libraryName: libName, inputNames: inNames, outputNames: outNames
            ))
        }

        // Parse wires for topological sorting
        var wiresRaw: [(src: Int, srcPin: Int, dst: Int, dstPin: Int, name: String)] = []
        if let wiresArray = caseDict["wires"] as? [[String: Any]] {
            for w in wiresArray {
                let srcNode = w["source_node"] as? Int ?? 0
                let srcPin = w["source_pin"] as? Int ?? 0
                let dstNode = w["target_node"] as? Int ?? 0
                let dstPin = w["target_pin"] as? Int ?? 0
                let wireName = w["name"] as? String ?? ""
                wiresRaw.append((srcNode, srcPin, dstNode, dstPin, wireName))
            }
        }

        // Compute topological depth for layout (top-to-bottom)
        // Input bar is always row 0; everything else starts at row 1+
        var depth: [Int: Int] = [:]
        for rn in rawNodes {
            depth[rn.nodeId] = (rn.nodeId == inputBarId) ? 0 : 1
        }

        for _ in 0..<rawNodes.count {
            for w in wiresRaw {
                let srcDepth = depth[w.src] ?? 0
                let dstDepth = depth[w.dst] ?? 0
                if srcDepth + 1 > dstDepth {
                    depth[w.dst] = srcDepth + 1
                }
            }
        }

        // Pull leaf nodes (no incoming wires, not input bar) to 1 row above their consumer
        let nodesWithIncoming = Set(wiresRaw.map { $0.dst })
        for rn in rawNodes {
            if rn.nodeId != inputBarId && !nodesWithIncoming.contains(rn.nodeId) {
                let consumerDepths = wiresRaw
                    .filter { $0.src == rn.nodeId }
                    .compactMap { depth[$0.dst] }
                if let minConsumer = consumerDepths.min() {
                    depth[rn.nodeId] = max(1, minConsumer - 1)
                }
            }
        }

        // Output bar is always the last row
        let maxDepth = depth.values.max() ?? 0
        depth[outputBarId] = max(maxDepth, (depth[outputBarId] ?? 0))

        // Group by depth for row layout (top-to-bottom like real Prograph)
        var rows: [Int: [RawNode]] = [:]
        for rn in rawNodes {
            let d = depth[rn.nodeId] ?? 0
            rows[d, default: []].append(rn)
        }

        let rowSpacing: CGFloat = 120
        let colSpacing: CGFloat = 170
        let startX: CGFloat = 40
        let startY: CGFloat = 40
        let maxColumns = 4
        let wrapRowOffset: CGFloat = 60

        // Compute cumulative Y for each depth, accounting for row wrapping
        let sortedDepths = rows.keys.sorted()
        var yForDepth: [Int: CGFloat] = [:]
        var currentY: CGFloat = startY
        for d in sortedDepths {
            yForDepth[d] = currentY
            let count = rows[d]?.count ?? 1
            let subRows = max(1, Int(ceil(Double(count) / Double(maxColumns))))
            currentY += rowSpacing + CGFloat(subRows - 1) * wrapRowOffset
        }

        // Create GraphNodeModels with auto-layout positions
        // depth = row (Y), nodes at same depth spread horizontally (X)
        // Wraps to sub-rows when > maxColumns at same depth
        for (row, nodesInRow) in rows.sorted(by: { $0.key < $1.key }) {
            for (index, rn) in nodesInRow.enumerated() {
                let col = index % maxColumns
                let subRow = index / maxColumns
                let x = startX + CGFloat(col) * colSpacing
                let y = (yForDepth[row] ?? startY) + CGFloat(subRow) * wrapRowOffset

                let displayName: String
                let displayType: String
                switch rn.nodeType {
                case "input_bar":
                    displayName = "Inputs"
                    displayType = "input_bar"
                case "output_bar":
                    displayName = "Outputs"
                    displayType = "output_bar"
                case "constant":
                    displayName = rn.constantValue ?? "?"
                    displayType = "constant"
                case "primitive":
                    displayName = rn.name
                    displayType = "primitive"
                case "method_call":
                    displayName = rn.name
                    displayType = "method_call"
                case "instance_generator":
                    displayName = "new \(rn.name)"
                    displayType = "instance"
                case "get":
                    displayName = "get \(rn.name)"
                    displayType = "get"
                case "set":
                    displayName = "set \(rn.name)"
                    displayType = "set"
                default:
                    displayName = rn.name
                    displayType = rn.nodeType
                }

                let node = GraphNodeModel(x: x, y: y, label: displayName, nodeType: displayType)
                node.engineNodeId = UInt32(rn.nodeId)

                for i in 0..<rn.numInputs {
                    let pinName = rn.inputNames?[safe: i] ?? "in\(i)"
                    node.inputPins.append(PinModel(name: pinName, index: i))
                }
                for i in 0..<rn.numOutputs {
                    let pinName = rn.outputNames?[safe: i] ?? "out\(i)"
                    node.outputPins.append(PinModel(name: pinName, index: i))
                }
                node.constantValue = rn.constantValue
                node.libraryName = rn.libraryName
                if let annStr = nodesArray.first(where: { ($0["id"] as? Int) == rn.nodeId })?["annotation"] as? String,
                   let ann = GraphNodeModel.NodeAnnotation(rawValue: annStr) {
                    node.annotation = ann
                }
                node.height = node.computeHeight()
                // Widen for long labels, but cap to avoid overly wide nodes
                let labelWidth = CGFloat(displayName.count) * 9.5 + 40
                node.width = min(max(node.width, labelWidth), 200)

                nodeIdToUUID[rn.nodeId] = node.id
                graph.nodes.append(node)
            }
        }

        // Create wires
        for w in wiresRaw {
            guard let srcUUID = nodeIdToUUID[w.src],
                  let dstUUID = nodeIdToUUID[w.dst] else { continue }
            let wire = GraphWireModel(
                sourceNodeId: srcUUID, sourcePin: w.srcPin,
                destNodeId: dstUUID, destPin: w.dstPin
            )
            wire.name = w.name
            graph.wires.append(wire)
        }

        return graph
    }
}

class GraphNodeModel: ObservableObject, Identifiable {
    let id = UUID()
    @Published var x: CGFloat
    @Published var y: CGFloat
    @Published var width: CGFloat = 160
    @Published var height: CGFloat = 80
    @Published var label: String
    @Published var nodeType: String // "primitive", "method_call", "constant", etc.
    @Published var inputPins: [PinModel] = []
    @Published var outputPins: [PinModel] = []

    /// Value for constant nodes
    @Published var constantValue: String?

    /// Trace value (shown during debugging)
    @Published var traceValues: [String] = []

    /// Engine node ID (from C++ engine, used for debugger breakpoints)
    var engineNodeId: UInt32 = 0

    /// Library that owns this primitive (nil for built-in)
    @Published var libraryName: String?

    /// Prograph annotation (loop, match, control flow)
    @Published var annotation: NodeAnnotation = .none

    enum NodeAnnotation: String {
        case none
        case loop, listMap, partition, inject
        case nextCaseOnFailure, nextCaseOnSuccess
        case continueOnFailure, terminateOnSuccess, terminateOnFailure
        case finishOnSuccess, finishOnFailure, failOnSuccess, failOnFailure
    }

    init(x: CGFloat, y: CGFloat, label: String, nodeType: String = "primitive") {
        self.x = x
        self.y = y
        self.label = label
        self.nodeType = nodeType
    }

    func computeHeight() -> CGFloat {
        // Top-to-bottom Prograph layout: pin circles on top/bottom edges
        // pinRadius(6) top + header(30+padding) + pinRadius(6) bottom
        let pinSpace: CGFloat = 6
        let headerHeight: CGFloat = 34
        var h = headerHeight
        if inputPins.count > 0 { h += pinSpace }
        if outputPins.count > 0 { h += pinSpace }
        return h
    }
}

class PinModel: Identifiable {
    let id = UUID()
    let name: String
    let index: Int

    init(name: String, index: Int) {
        self.name = name
        self.index = index
    }
}

class GraphWireModel: ObservableObject, Identifiable {
    let id = UUID()
    let sourceNodeId: UUID
    let sourcePin: Int
    @Published var destNodeId: UUID?
    @Published var destPin: Int?
    @Published var name: String = ""
    @Published var danglingEndpoint: CGPoint?

    init(sourceNodeId: UUID, sourcePin: Int, destNodeId: UUID? = nil, destPin: Int? = nil) {
        self.sourceNodeId = sourceNodeId
        self.sourcePin = sourcePin
        self.destNodeId = destNodeId
        self.destPin = destPin
    }
}

private extension Array {
    subscript(safe index: Int) -> Element? {
        indices.contains(index) ? self[index] : nil
    }
}
