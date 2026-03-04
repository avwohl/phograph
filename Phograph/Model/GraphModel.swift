import Foundation
import Combine

/// Observable graph model for SwiftUI binding.
/// Represents a single case's graph for display in the canvas.
class GraphModel: ObservableObject {
    @Published var nodes: [GraphNodeModel] = []
    @Published var wires: [GraphWireModel] = []
    @Published var selectedNodeIds: Set<UUID> = []
    @Published var methodName: String = ""
    @Published var caseIndex: Int = 0

    /// Camera state
    @Published var panOffset: CGPoint = .zero
    @Published var zoomScale: CGFloat = 1.0

    func addNode(_ node: GraphNodeModel) {
        nodes.append(node)
    }

    func removeNode(id: UUID) {
        nodes.removeAll { $0.id == id }
        wires.removeAll { $0.sourceNodeId == id || $0.destNodeId == id }
    }

    func addWire(_ wire: GraphWireModel) {
        wires.append(wire)
    }

    func removeWire(id: UUID) {
        wires.removeAll { $0.id == id }
    }

    func selectNode(id: UUID, exclusive: Bool = true) {
        if exclusive {
            selectedNodeIds = [id]
        } else {
            selectedNodeIds.insert(id)
        }
    }

    func deselectAll() {
        selectedNodeIds.removeAll()
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

            rawNodes.append(RawNode(
                nodeId: nodeId, name: name, nodeType: typeStr,
                numInputs: numIn, numOutputs: numOut, constantValue: constVal
            ))
        }

        // Parse wires for topological sorting
        var wiresRaw: [(src: Int, srcPin: Int, dst: Int, dstPin: Int)] = []
        if let wiresArray = caseDict["wires"] as? [[String: Any]] {
            for w in wiresArray {
                let srcNode = w["source_node"] as? Int ?? 0
                let srcPin = w["source_pin"] as? Int ?? 0
                let dstNode = w["target_node"] as? Int ?? 0
                let dstPin = w["target_pin"] as? Int ?? 0
                wiresRaw.append((srcNode, srcPin, dstNode, dstPin))
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

        let rowSpacing: CGFloat = 130
        let colSpacing: CGFloat = 200
        let startX: CGFloat = 40
        let startY: CGFloat = 40

        // Create GraphNodeModels with auto-layout positions
        // depth = row (Y), nodes at same depth spread horizontally (X)
        for (row, nodesInRow) in rows.sorted(by: { $0.key < $1.key }) {
            for (col, rn) in nodesInRow.enumerated() {
                let x = startX + CGFloat(col) * colSpacing
                let y = startY + CGFloat(row) * rowSpacing

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

                for i in 0..<rn.numInputs {
                    node.inputPins.append(PinModel(name: "in\(i)", index: i))
                }
                for i in 0..<rn.numOutputs {
                    node.outputPins.append(PinModel(name: "out\(i)", index: i))
                }
                node.constantValue = rn.constantValue
                node.height = node.computeHeight()
                // Widen for long labels
                let labelWidth = CGFloat(displayName.count) * 9.5 + 40
                node.width = max(node.width, labelWidth)

                nodeIdToUUID[rn.nodeId] = node.id
                graph.nodes.append(node)
            }
        }

        // Create wires
        for w in wiresRaw {
            guard let srcUUID = nodeIdToUUID[w.src],
                  let dstUUID = nodeIdToUUID[w.dst] else { continue }
            graph.wires.append(GraphWireModel(
                sourceNodeId: srcUUID, sourcePin: w.srcPin,
                destNodeId: dstUUID, destPin: w.dstPin
            ))
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

struct GraphWireModel: Identifiable {
    let id = UUID()
    let sourceNodeId: UUID
    let sourcePin: Int
    let destNodeId: UUID
    let destPin: Int
}
