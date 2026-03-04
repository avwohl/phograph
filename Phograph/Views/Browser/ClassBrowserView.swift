import SwiftUI

/// Project tree outline showing sections, methods, and classes.
struct ClassBrowserView: View {
    @ObservedObject var viewModel: IDEViewModel

    var body: some View {
        List {
            if let project = viewModel.project {
                ForEach(project.sections) { section in
                    Section(header: sectionHeader(section)) {
                        if section.isExpanded {
                            // Methods
                            if !section.methods.isEmpty {
                                DisclosureGroup("Methods") {
                                    ForEach(section.methods) { method in
                                        methodRow(method)
                                    }
                                }
                            }

                            // Classes
                            if !section.classes.isEmpty {
                                DisclosureGroup("Classes") {
                                    ForEach(section.classes) { classDef in
                                        classRow(classDef)
                                    }
                                }
                            }
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
            Text("\(method.numInputs) -> \(method.numOutputs)")
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
            ForEach(classDef.attributes, id: \.self) { attr in
                HStack {
                    Image(systemName: "square.fill")
                        .font(.system(size: 8))
                        .foregroundColor(.green)
                    Text(attr)
                        .font(.caption)
                }
            }
        } label: {
            HStack {
                Image(systemName: "cube")
                    .foregroundColor(.orange)
                Text(classDef.name)
                if let parent = classDef.parentName {
                    Text(": \(parent)")
                        .foregroundColor(.secondary)
                        .font(.caption)
                }
            }
        }
    }
}
