import SwiftUI

struct ExampleBrowserView: View {
    @ObservedObject var viewModel: IDEViewModel
    @State private var selectedCategory: ExampleCategory?
    @State private var selectedExample: ExampleEntry?

    private let categories = ExampleCatalog.categories

    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Example Projects")
                    .font(.headline)
                Spacer()
            }
            .padding()

            Divider()

            // Two-column browser
            HStack(spacing: 0) {
                // Left: category list
                List(categories, selection: $selectedCategory) { category in
                    Label(category.name, systemImage: category.icon)
                        .tag(category)
                }
                .listStyle(.sidebar)
                .frame(width: 200)

                Divider()

                // Right: examples for selected category
                VStack(spacing: 0) {
                    if let category = selectedCategory {
                        List(category.examples, selection: $selectedExample) { example in
                            VStack(alignment: .leading, spacing: 4) {
                                Text(example.name)
                                    .font(.body.weight(.medium))
                                Text(example.description)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                    .lineLimit(2)
                            }
                            .padding(.vertical, 4)
                            .tag(example)
                            .onTapGesture(count: 2) {
                                selectedExample = example
                                openSelected()
                            }
                            .onTapGesture(count: 1) {
                                selectedExample = example
                            }
                        }
                        .listStyle(.inset)
                    } else {
                        VStack {
                            Spacer()
                            Text("Select a category")
                                .foregroundColor(.secondary)
                            Spacer()
                        }
                        .frame(maxWidth: .infinity)
                    }
                }
            }
            .frame(maxHeight: .infinity)

            Divider()

            // Bottom bar: description + buttons
            HStack {
                if let example = selectedExample {
                    Text(example.description)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .lineLimit(2)
                        .frame(maxWidth: .infinity, alignment: .leading)
                } else {
                    Text("Choose an example to get started")
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }

                Button("Cancel") {
                    viewModel.showExampleBrowser = false
                }
                .keyboardShortcut(.cancelAction)

                Button("Open") {
                    openSelected()
                }
                .keyboardShortcut(.defaultAction)
                .disabled(selectedExample == nil)
            }
            .padding()
        }
        .frame(width: 640, height: 440)
        .onAppear {
            selectedCategory = categories.first
        }
    }

    private func openSelected() {
        guard let example = selectedExample else { return }
        viewModel.showExampleBrowser = false
        viewModel.loadExample(example)
    }
}

// MARK: - Hashable/Equatable conformance for selection binding

extension ExampleCategory: Hashable, Equatable {
    static func == (lhs: ExampleCategory, rhs: ExampleCategory) -> Bool {
        lhs.id == rhs.id
    }
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}

extension ExampleEntry: Hashable, Equatable {
    static func == (lhs: ExampleEntry, rhs: ExampleEntry) -> Bool {
        lhs.id == rhs.id
    }
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}
