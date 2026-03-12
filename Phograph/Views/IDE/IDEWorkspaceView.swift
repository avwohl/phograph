import SwiftUI

/// Root IDE view with three-panel layout: browser | canvas | inspector.
/// Shows a welcome screen when no project is loaded.
struct IDEWorkspaceView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        Group {
            if viewModel.project != nil {
                projectView
            } else {
                welcomeView
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .onAppear {
            viewModel.libraryManager.discoverLibraries()
        }
        .sheet(isPresented: $viewModel.showLibraryManager) {
            LibraryManagerView(viewModel: viewModel)
        }
        .sheet(isPresented: $viewModel.showExportSheet) {
            ExportAppSheet(viewModel: viewModel)
        }
        .sheet(isPresented: $viewModel.showCanvasOutput) {
            CanvasOutputView(viewModel: viewModel)
        }
    }

    private var projectView: some View {
        VStack(spacing: 0) {
            if viewModel.isDebugging {
                DebuggerControlsView(viewModel: viewModel.debugger, ideViewModel: viewModel)
                    .background(Color(nsColor: .controlBackgroundColor))
                Divider()
            }

            HStack(spacing: 0) {
                if viewModel.showSidebar {
                    ClassBrowserView(viewModel: viewModel)
                        .frame(width: 220)
                    Divider()
                }

                GraphCanvasView(viewModel: viewModel)
                    .frame(maxWidth: .infinity, maxHeight: .infinity)

                if viewModel.showInspector {
                    Divider()
                    InspectorPanelView(viewModel: viewModel)
                        .frame(width: 220)
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
        .toolbar {
            ToolbarView(viewModel: viewModel)
        }
        .overlay {
            if viewModel.showFuzzyFinder {
                FuzzyFinderPopup(
                    isPresented: $viewModel.showFuzzyFinder,
                    items: viewModel.allPrimitiveNames(),
                    onSelect: { name in
                        addNodeFromFuzzyFinder(name: name)
                    }
                )
            }
        }
    }

    private var welcomeView: some View {
        VStack(spacing: 24) {
            Spacer()

            Text("Phograph")
                .font(.system(size: 36, weight: .light))

            Text("Visual Dataflow Programming")
                .font(.title3)
                .foregroundColor(.secondary)

            VStack(spacing: 12) {
                welcomeButton("New Project", shortcut: "N") {
                    viewModel.newProject()
                }
                welcomeButton("Open Project...", shortcut: "O") {
                    openProject()
                }
                welcomeButton("Browse Examples...", shortcut: "\u{21E7}E") {
                    viewModel.showExampleBrowser = true
                }
            }
            .padding(.top, 8)

            Spacer()

            Link("https://github.com/avwohl/phograph",
                 destination: URL(string: "https://github.com/avwohl/phograph")!)
                .font(.caption)
                .padding(.bottom, 16)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
        .sheet(isPresented: $viewModel.showExampleBrowser) {
            ExampleBrowserView(viewModel: viewModel)
        }
    }

    private func welcomeButton(_ title: String, shortcut: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack {
                Text(title)
                    .frame(width: 160, alignment: .leading)
                Text("\u{2318}\(shortcut)")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
        }
        .buttonStyle(.bordered)
    }

    #if os(macOS)
    private func openProject() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.json]
        panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            do {
                let json = try String(contentsOf: url)
                viewModel.loadProject(json: json, from: url)
            } catch {
                viewModel.consoleOutput += "Failed to read \(url.lastPathComponent): \(error.localizedDescription)\n"
                viewModel.statusMessage = "Open failed"
            }
        }
    }
    #else
    private func openProject() {}
    #endif

    private func addNodeFromFuzzyFinder(name: String) {
        guard let graph = viewModel.currentGraph else { return }

        // Detect class-derived node types by prefix/pattern
        if name.hasPrefix("new ") {
            _ = String(name.dropFirst(4))
            let node = GraphNodeModel(x: 200, y: 200, label: name, nodeType: "instance_generator")
            node.outputPins.append(PinModel(name: "out0", index: 0))
            node.height = node.computeHeight()
            node.width = max(node.width, CGFloat(name.count) * 9.5 + 40)
            graph.addNode(node)
            viewModel.showFuzzyFinder = false
            return
        }

        if name.hasPrefix("get ") {
            let node = GraphNodeModel(x: 200, y: 200, label: name, nodeType: "get")
            node.inputPins.append(PinModel(name: "in0", index: 0))
            node.outputPins.append(PinModel(name: "out0", index: 0))
            node.outputPins.append(PinModel(name: "out1", index: 1))
            node.height = node.computeHeight()
            node.width = max(node.width, CGFloat(name.count) * 9.5 + 40)
            graph.addNode(node)
            viewModel.showFuzzyFinder = false
            return
        }

        if name.hasPrefix("set ") {
            let node = GraphNodeModel(x: 200, y: 200, label: name, nodeType: "set")
            node.inputPins.append(PinModel(name: "in0", index: 0))
            node.inputPins.append(PinModel(name: "in1", index: 1))
            node.outputPins.append(PinModel(name: "out0", index: 0))
            node.height = node.computeHeight()
            node.width = max(node.width, CGFloat(name.count) * 9.5 + 40)
            graph.addNode(node)
            viewModel.showFuzzyFinder = false
            return
        }

        if name.contains("/") {
            // ClassName/methodName format — look up method pin counts
            let parts = name.split(separator: "/", maxSplits: 1)
            let methodName = parts.count > 1 ? String(parts[1]) : name
            var numIn = 1, numOut = 1
            if let method = viewModel.project?.findMethod(methodName) {
                numIn = method.numInputs
                numOut = method.numOutputs
            }
            let node = GraphNodeModel(x: 200, y: 200, label: methodName, nodeType: "method_call")
            for i in 0..<numIn { node.inputPins.append(PinModel(name: "in\(i)", index: i)) }
            for i in 0..<numOut { node.outputPins.append(PinModel(name: "out\(i)", index: i)) }
            node.height = node.computeHeight()
            node.width = max(node.width, CGFloat(methodName.count) * 9.5 + 40)
            graph.addNode(node)
            viewModel.showFuzzyFinder = false
            return
        }

        // Default: primitive or library node
        let node = GraphNodeModel(x: 200, y: 200, label: name, nodeType: "primitive")

        // Look up correct pin counts from library primitives
        if let result = viewModel.libraryPrimitive(named: name) {
            let prim = result.primitive
            for i in 0..<prim.num_inputs {
                node.inputPins.append(PinModel(name: "in\(i)", index: i))
            }
            for i in 0..<prim.num_outputs {
                node.outputPins.append(PinModel(name: "out\(i)", index: i))
            }
            node.libraryName = result.library.manifest.name
        } else {
            // Check if it's a known universal method
            if let method = viewModel.project?.findMethod(name) {
                node.nodeType = "method_call"
                for i in 0..<method.numInputs { node.inputPins.append(PinModel(name: "in\(i)", index: i)) }
                for i in 0..<method.numOutputs { node.outputPins.append(PinModel(name: "out\(i)", index: i)) }
            } else {
                node.inputPins = [PinModel(name: "in", index: 0)]
                node.outputPins = [PinModel(name: "out", index: 0)]
            }
        }

        node.height = node.computeHeight()
        let labelWidth = CGFloat(name.count) * 9.5 + 40
        node.width = max(node.width, labelWidth)
        graph.addNode(node)
        viewModel.showFuzzyFinder = false
    }
}
