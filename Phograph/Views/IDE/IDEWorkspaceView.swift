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
    }

    private var projectView: some View {
        HSplitView {
            ClassBrowserView(viewModel: viewModel)
                .frame(minWidth: 180, idealWidth: 220, maxWidth: 280)

            GraphCanvasView(viewModel: viewModel)
                .frame(maxWidth: .infinity, maxHeight: .infinity)

            InspectorPanelView(viewModel: viewModel)
                .frame(minWidth: 180, idealWidth: 220, maxWidth: 280)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
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
            }
            .padding(.top, 8)

            Spacer()

            Text("https://github.com/dwohl/phograph")
                .font(.caption)
                .foregroundColor(.secondary)
                .padding(.bottom, 16)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(nsColor: .windowBackgroundColor))
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
            if let json = try? String(contentsOf: url) {
                viewModel.loadProject(json: json, from: url)
            }
        }
    }
    #else
    private func openProject() {}
    #endif

    private func addNodeFromFuzzyFinder(name: String) {
        guard let graph = viewModel.currentGraph else { return }
        let node = GraphNodeModel(x: 200, y: 200, label: name, nodeType: "primitive")
        node.inputPins = [PinModel(name: "in", index: 0)]
        node.outputPins = [PinModel(name: "out", index: 0)]
        node.height = node.computeHeight()
        graph.addNode(node)
        viewModel.showFuzzyFinder = false
    }
}
