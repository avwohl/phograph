import SwiftUI

/// Phase 28: Front Panel / Block Diagram Dual View
/// Classes get an optional Front Panel view with SwiftUI controls
/// mapped to block diagram terminals (class attributes).
struct FrontPanelView: View {
    @ObservedObject var viewModel: IDEViewModel

    /// The class whose front panel we're displaying
    var className: String

    /// Attribute bindings: name -> current value
    @State private var attributeValues: [String: String] = [:]

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: "slider.horizontal.3")
                Text("Front Panel — \(className)")
                    .font(.headline)
                Spacer()
                Button(action: { viewModel.showFrontPanel = false }) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(.secondary)
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(nsColor: .controlBackgroundColor))

            Divider()

            // Control grid
            if let attrs = currentClassAttributes {
                ScrollView {
                    LazyVGrid(columns: [
                        GridItem(.flexible(minimum: 100)),
                        GridItem(.flexible(minimum: 150))
                    ], spacing: 12) {
                        ForEach(attrs, id: \.name) { attr in
                            FrontPanelControlRow(
                                name: attr.name,
                                value: Binding(
                                    get: { attributeValues[attr.name] ?? attr.defaultValue },
                                    set: { newVal in
                                        attributeValues[attr.name] = newVal
                                        sendAttributeUpdate(attr.name, newVal)
                                    }
                                ),
                                attrType: attr.inferredType
                            )
                        }
                    }
                    .padding(16)
                }
            } else {
                VStack {
                    Spacer()
                    Text("No attributes found for \(className)")
                        .foregroundColor(.secondary)
                    Spacer()
                }
            }
        }
        .frame(minWidth: 300, minHeight: 200)
        .onAppear(perform: loadAttributes)
    }

    // MARK: - Data

    struct AttributeInfo {
        let name: String
        let defaultValue: String
        var inferredType: ControlType = .text

        enum ControlType {
            case text, number, toggle, slider
        }
    }

    private var currentClassAttributes: [AttributeInfo]? {
        guard let project = viewModel.project else { return nil }
        guard let cls = project.classes.first(where: { $0.name == className }) else { return nil }
        return cls.attributes.map { attr in
            var info = AttributeInfo(name: attr.name, defaultValue: attr.defaultValue)
            // Infer control type from default value
            if attr.defaultValue == "true" || attr.defaultValue == "false" {
                info.inferredType = .toggle
            } else if Double(attr.defaultValue) != nil {
                info.inferredType = .number
            }
            return info
        }
    }

    private func loadAttributes() {
        if let attrs = currentClassAttributes {
            for attr in attrs {
                if attributeValues[attr.name] == nil {
                    attributeValues[attr.name] = attr.defaultValue
                }
            }
        }
    }

    private func sendAttributeUpdate(_ name: String, _ value: String) {
        // In a live system, this would call through the bridge to update the
        // running object instance's attribute and trigger observer callbacks.
        viewModel.consoleOutput += "Front Panel: \(className).\(name) = \(value)\n"
    }
}

/// A single control row in the front panel grid
struct FrontPanelControlRow: View {
    let name: String
    @Binding var value: String
    let attrType: FrontPanelView.AttributeInfo.ControlType

    var body: some View {
        HStack {
            Text(name)
                .font(.system(.body, design: .monospaced))
                .frame(minWidth: 80, alignment: .trailing)

            switch attrType {
            case .toggle:
                Toggle("", isOn: Binding(
                    get: { value == "true" },
                    set: { value = $0 ? "true" : "false" }
                ))
                .labelsHidden()

            case .number:
                TextField("", text: $value)
                    .textFieldStyle(.roundedBorder)
                    .frame(maxWidth: 120)

            case .slider:
                Slider(value: Binding(
                    get: { Double(value) ?? 0 },
                    set: { value = String($0) }
                ), in: 0...100)
                .frame(maxWidth: 200)

            case .text:
                TextField("", text: $value)
                    .textFieldStyle(.roundedBorder)
            }
        }
    }
}
