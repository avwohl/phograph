import SwiftUI

/// Property inspector for selected nodes.
struct InspectorPanelView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                if let graph = viewModel.currentGraph,
                   let nodeId = graph.selectedNodeIds.first,
                   let node = graph.nodes.first(where: { $0.id == nodeId }) {
                    nodeInspector(node)
                } else {
                    noSelectionView
                }
            }
            .padding()
        }
        .navigationTitle("Inspector")
    }

    private var noSelectionView: some View {
        VStack(spacing: 8) {
            Image(systemName: "square.dashed")
                .font(.largeTitle)
                .foregroundColor(.secondary)
            Text("No selection")
                .foregroundColor(.secondary)
            Text("Select a node to inspect its properties")
                .font(.caption)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding(.top, 40)
    }

    private func nodeInspector(_ node: GraphNodeModel) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header
            HStack {
                Image(systemName: "rectangle.fill")
                    .foregroundColor(.blue)
                Text(node.label)
                    .font(.headline)
            }

            Divider()

            // Type
            LabeledContent("Type") {
                Text(node.nodeType)
                    .foregroundColor(.secondary)
            }

            // Position
            LabeledContent("Position") {
                Text("(\(Int(node.x)), \(Int(node.y)))")
                    .foregroundColor(.secondary)
            }

            // Size
            LabeledContent("Size") {
                Text("\(Int(node.width)) x \(Int(node.height))")
                    .foregroundColor(.secondary)
            }

            // Inputs
            if !node.inputPins.isEmpty {
                Divider()
                Text("Inputs")
                    .font(.subheadline)
                    .fontWeight(.medium)
                ForEach(node.inputPins) { pin in
                    HStack {
                        Circle()
                            .fill(Color.green.opacity(0.8))
                            .frame(width: 8, height: 8)
                        Text(pin.name)
                            .font(.caption)
                    }
                }
            }

            // Outputs
            if !node.outputPins.isEmpty {
                Divider()
                Text("Outputs")
                    .font(.subheadline)
                    .fontWeight(.medium)
                ForEach(node.outputPins) { pin in
                    HStack {
                        Circle()
                            .fill(Color.red.opacity(0.8))
                            .frame(width: 8, height: 8)
                        Text(pin.name)
                            .font(.caption)
                    }
                }
            }

            // Constant value
            if let value = node.constantValue {
                Divider()
                LabeledContent("Value") {
                    Text(value)
                        .font(.system(.body, design: .monospaced))
                }
            }

            // Trace values (debugging)
            if !node.traceValues.isEmpty {
                Divider()
                Text("Trace")
                    .font(.subheadline)
                    .fontWeight(.medium)
                ForEach(node.traceValues, id: \.self) { tv in
                    Text(tv)
                        .font(.system(.caption, design: .monospaced))
                        .foregroundColor(.yellow)
                }
            }
        }
    }
}
