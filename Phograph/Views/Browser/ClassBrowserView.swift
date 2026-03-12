import SwiftUI

/// Project tree outline: classes (with attributes + methods), top-level methods, libraries.
struct ClassBrowserView: View {
    @ObservedObject var viewModel: IDEViewModel
    @State private var renamingClassId: UUID?
    @State private var renameText: String = ""

    var body: some View {
        // Read graphRevision to trigger re-render when classes/methods are added
        let _ = viewModel.graphRevision
        List {
            if let project = viewModel.project {
                ForEach(project.sections) { section in
                    Section(header: sectionHeader(section)) {
                        if section.isExpanded {
                            // Classes heading with [+]
                            classesHeading(section)

                            // Each class
                            ForEach(section.classes) { classDef in
                                classRow(classDef)
                            }

                            // Top-level methods shown directly
                            ForEach(section.methods) { method in
                                methodRow(method)
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

    // MARK: - Section Header

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

    // MARK: - Classes Heading

    private func classesHeading(_ section: SectionModel) -> some View {
        HStack {
            Image(systemName: "hexagon.fill")
                .foregroundColor(.orange)
                .font(.caption)
            Text("Classes")
                .font(.caption)
                .foregroundColor(.secondary)
                .textCase(.uppercase)
            Spacer()
            Button(action: {
                let newClass = viewModel.addClass(in: section)
                startRename(newClass)
            }) {
                Image(systemName: "plus")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .buttonStyle(.plain)
            .help("Add new class")
        }
    }

    // MARK: - Class Row

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
                if renamingClassId == classDef.id {
                    TextField("Class name", text: $renameText, onCommit: {
                        commitRename(classDef)
                    })
                    .textFieldStyle(.plain)
                    .onExitCommand { renamingClassId = nil }
                } else {
                    Text(classDef.name)
                    if let parent = classDef.parentName {
                        Text(": \(parent)")
                            .foregroundColor(.secondary)
                            .font(.caption)
                    }
                }
                Spacer()
                Menu {
                    Button("Add Method") { viewModel.addMethod(to: classDef) }
                    Button("Add Attribute") { viewModel.addAttribute(to: classDef) }
                    Divider()
                    Button("Rename") { startRename(classDef) }
                } label: {
                    Image(systemName: "plus")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                .menuStyle(.borderlessButton)
                .fixedSize()
                .help("Add to \(classDef.name)")
            }
            .contextMenu {
                Button("Rename") { startRename(classDef) }
            }
        }
    }

    // MARK: - Class Rename

    private func startRename(_ classDef: ClassModel) {
        renameText = classDef.name
        renamingClassId = classDef.id
    }

    private func commitRename(_ classDef: ClassModel) {
        let trimmed = renameText.trimmingCharacters(in: .whitespaces)
        if !trimmed.isEmpty {
            viewModel.renameClass(classDef, to: trimmed)
        }
        renamingClassId = nil
    }

    // MARK: - Method Row

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

    // MARK: - Libraries

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

}
