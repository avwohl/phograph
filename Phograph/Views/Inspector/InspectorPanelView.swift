import SwiftUI

/// Property inspector for selected nodes.
struct InspectorPanelView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                if let graph = viewModel.currentGraph,
                   let wireId = graph.selectedWireIds.first,
                   let wire = graph.wires.first(where: { $0.id == wireId }) {
                    wireInspector(wire, graph: graph)
                } else if let graph = viewModel.currentGraph,
                   let nodeId = graph.selectedNodeIds.first,
                   let node = graph.nodes.first(where: { $0.id == nodeId }) {
                    nodeInspector(node)
                } else {
                    noSelectionView
                }
            }
            .padding()
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(.systemBackground))
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

            // Annotation
            if node.annotation != .none {
                LabeledContent("Annotation") {
                    Text(node.annotation.rawValue)
                        .foregroundColor(.secondary)
                }
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

            // Library info
            if let libName = node.libraryName {
                Divider()
                libraryInfoSection(libraryName: libName, primitiveName: node.label)
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

    private func wireInspector(_ wire: GraphWireModel, graph: GraphModel) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header
            HStack {
                Image(systemName: "line.diagonal")
                    .foregroundColor(.blue)
                Text("Wire")
                    .font(.headline)
            }

            Divider()

            // Editable name
            LabeledContent("Name") {
                TextField("Wire name", text: Binding(
                    get: { wire.name },
                    set: { wire.name = $0; graph.objectWillChange.send() }
                ))
                .textFieldStyle(.roundedBorder)
                .frame(maxWidth: 140)
            }

            // Source info
            if let srcNode = graph.nodes.first(where: { $0.id == wire.sourceNodeId }) {
                LabeledContent("Source") {
                    Text("\(srcNode.label) [pin \(wire.sourcePin)]")
                        .foregroundColor(.secondary)
                }
            }

            // Dest info
            if let destId = wire.destNodeId, let destPin = wire.destPin,
               let destNode = graph.nodes.first(where: { $0.id == destId }) {
                LabeledContent("Dest") {
                    Text("\(destNode.label) [pin \(destPin)]")
                        .foregroundColor(.secondary)
                }
            } else {
                LabeledContent("Dest") {
                    Text("Dangling")
                        .foregroundColor(.orange)
                }
            }
        }
    }

    private func libraryInfoSection(libraryName: String, primitiveName: String) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text("Library")
                .font(.subheadline)
                .fontWeight(.medium)

            if let result = viewModel.libraryPrimitive(named: primitiveName),
               result.library.manifest.name == libraryName {
                let lib = result.library
                let prim = result.primitive

                LabeledContent("Library") {
                    Text(lib.manifest.name)
                        .foregroundColor(.purple)
                }
                LabeledContent("Version") {
                    Text(lib.manifest.version)
                        .foregroundColor(.secondary)
                }
                if let author = lib.manifest.author {
                    LabeledContent("Author") {
                        Text(author)
                            .foregroundColor(.secondary)
                    }
                }
                if let desc = prim.description {
                    Text(desc)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .padding(.top, 2)
                }
            } else {
                // Library not installed
                HStack {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.yellow)
                    Text("Library '\(libraryName)' not installed")
                        .font(.caption)
                        .foregroundColor(.yellow)
                }
            }
        }
    }
}
