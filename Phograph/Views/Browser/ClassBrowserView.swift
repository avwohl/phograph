import SwiftUI

/// Project tree outline showing sections with classes (containing methods) and universal methods.
struct ClassBrowserView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        List {
            if let project = viewModel.project {
                ForEach(project.sections) { section in
                    Section(header: sectionHeader(section)) {
                        if section.isExpanded {
                            // Classes first (Prograph-style)
                            ForEach(section.classes) { classDef in
                                classRow(classDef)
                            }

                            // Universal methods (not owned by any class)
                            if !section.methods.isEmpty {
                                DisclosureGroup("Universal Methods") {
                                    ForEach(section.methods) { method in
                                        methodRow(method)
                                    }
                                }
                            }
                        }
                    }
                }
                // Libraries section
                if !viewModel.libraryManager.libraries.isEmpty {
                    Section(header: librariesHeader) {
                        ForEach(viewModel.libraryManager.libraries) { lib in
                            libraryRow(lib)
                        }
                    }
                }
            } else {
                Text("No project loaded")
                    .foregroundColor(.secondary)
                    .italic()
            }
        }
        .listStyle(.sidebar)
        .navigationTitle("Browser")
    }

    private var librariesHeader: some View {
        HStack {
            Image(systemName: "shippingbox.fill")
                .foregroundColor(.purple)
            Text("Libraries")
                .font(.headline)
            Spacer()
            Button(action: { viewModel.showLibraryManager = true }) {
                Image(systemName: "gear")
                    .font(.caption)
            }
            .buttonStyle(.plain)
        }
    }

    private func libraryRow(_ lib: LibraryInfo) -> some View {
        DisclosureGroup {
            ForEach(lib.manifest.primitives) { prim in
                HStack {
                    Image(systemName: "gearshape")
                        .foregroundColor(.secondary)
                        .font(.caption)
                    Text(prim.name)
                        .font(.caption)
                    Spacer()
                    Text("\(prim.num_inputs)\u{2192}\(prim.num_outputs)")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
                .contentShape(Rectangle())
                .onTapGesture {
                    insertLibraryPrimitive(prim, from: lib)
                }
            }
        } label: {
            HStack {
                Image(systemName: lib.source.iconName)
                    .foregroundColor(.purple)
                    .font(.caption)
                Text(lib.manifest.name)
                    .font(.body)
                Spacer()
                Text("v\(lib.manifest.version)")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
    }

    private func insertLibraryPrimitive(_ prim: LibraryPrimitiveDescriptor, from lib: LibraryInfo) {
        guard let graph = viewModel.currentGraph else { return }
        let node = GraphNodeModel(x: 200, y: 200, label: prim.name, nodeType: "primitive")
        for i in 0..<prim.num_inputs {
            node.inputPins.append(PinModel(name: "in\(i)", index: i))
        }
        for i in 0..<prim.num_outputs {
            node.outputPins.append(PinModel(name: "out\(i)", index: i))
        }
        node.libraryName = lib.manifest.name
        node.height = node.computeHeight()
        let labelWidth = CGFloat(prim.name.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
    }

    private func sectionHeader(_ section: SectionModel) -> some View {
        HStack {
            Image(systemName: section.isExpanded ? "folder.fill" : "folder")
                .foregroundColor(.yellow)
            Text(section.name)
                .font(.headline)
        }
        .onTapGesture {
            section.isExpanded.toggle()
        }
    }

    private func methodRow(_ method: MethodModel) -> some View {
        HStack {
            Image(systemName: "function")
                .foregroundColor(.blue)
            Text(method.name)
                .font(.body)
            Spacer()
            if method.caseCount > 1 {
                Text("\(method.caseCount) cases")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
            Text("\(method.numInputs)\u{2192}\(method.numOutputs)")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .contentShape(Rectangle())
        .onTapGesture {
            viewModel.selectMethod(name: method.name, caseIndex: 0)
        }
        .listRowBackground(
            viewModel.selectedMethodName == method.name
                ? Color.blue.opacity(0.2)
                : Color.clear
        )
    }

    private func classRow(_ classDef: ClassModel) -> some View {
        DisclosureGroup(isExpanded: Binding(
            get: { classDef.isExpanded },
            set: { classDef.isExpanded = $0 }
        )) {
            // Attributes
            ForEach(classDef.attributes) { attr in
                HStack {
                    Circle()
                        .fill(Color.green)
                        .frame(width: 8, height: 8)
                    Text(attr.name)
                        .font(.caption)
                    Spacer()
                    Button(action: { insertGetNode(className: classDef.name, attrName: attr.name) }) {
                        Image(systemName: "plus.circle")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                    .help("Insert get \(attr) node")
                }
            }

            // Methods owned by this class
            ForEach(classDef.methods) { method in
                methodRow(method)
            }
        } label: {
            HStack {
                Image(systemName: "hexagon.fill")
                    .foregroundColor(.orange)
                Text(classDef.name)
                if let parent = classDef.parentName {
                    Text(": \(parent)")
                        .foregroundColor(.secondary)
                        .font(.caption)
                }
                Spacer()
                Button(action: { insertInstanceGenerator(className: classDef.name) }) {
                    Image(systemName: "plus.circle")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                .buttonStyle(.plain)
                .help("Insert new \(classDef.name) node")
            }
        }
    }

    private func insertInstanceGenerator(className: String) {
        guard let graph = viewModel.currentGraph else { return }
        let label = "new \(className)"
        let node = GraphNodeModel(x: 200, y: 200, label: label, nodeType: "instance_generator")
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.height = node.computeHeight()
        node.width = max(node.width, CGFloat(label.count) * 9.5 + 40)
        graph.addNode(node)
    }

    private func insertGetNode(className: String, attrName: String) {
        guard let graph = viewModel.currentGraph else { return }
        let label = "get \(attrName)"
        let node = GraphNodeModel(x: 200, y: 200, label: label, nodeType: "get")
        node.inputPins.append(PinModel(name: "in0", index: 0))
        node.outputPins.append(PinModel(name: "out0", index: 0))
        node.outputPins.append(PinModel(name: "out1", index: 1))
        node.height = node.computeHeight()
        node.width = max(node.width, CGFloat(label.count) * 9.5 + 40)
        graph.addNode(node)
    }
}
