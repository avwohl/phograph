import SwiftUI

struct LibraryManagerView: View {
    @ObservedObject var viewModel: IDEViewModel
    @State private var selectedLibrary: LibraryInfo?
    @State private var showAddSheet: Bool = false

    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Library Manager")
                    .font(.headline)
                Spacer()
            }
            .padding()

            Divider()

            // Two-column layout
            HStack(spacing: 0) {
                // Left: library list
                List(viewModel.libraryManager.libraries, selection: Binding(
                    get: { selectedLibrary?.id },
                    set: { newId in
                        selectedLibrary = viewModel.libraryManager.libraries.first { $0.id == newId }
                    }
                )) { lib in
                    HStack {
                        Image(systemName: lib.source.iconName)
                            .foregroundColor(.purple)
                            .font(.caption)
                        VStack(alignment: .leading, spacing: 2) {
                            Text(lib.manifest.name)
                                .font(.body)
                            Text("v\(lib.manifest.version) — \(lib.source.displayName)")
                                .font(.caption2)
                                .foregroundColor(.secondary)
                        }
                    }
                    .tag(lib.id)
                    .onTapGesture {
                        selectedLibrary = lib
                    }
                }
                .listStyle(.sidebar)
                .frame(width: 220)

                Divider()

                // Right: detail
                VStack(alignment: .leading, spacing: 0) {
                    if let lib = selectedLibrary {
                        libraryDetail(lib)
                    } else {
                        VStack {
                            Spacer()
                            Text("Select a library")
                                .foregroundColor(.secondary)
                            Spacer()
                        }
                        .frame(maxWidth: .infinity)
                    }
                }
            }
            .frame(maxHeight: .infinity)

            Divider()

            // Bottom bar
            HStack {
                Button("Add Library...") {
                    showAddSheet = true
                }

                if let lib = selectedLibrary, lib.source != .bundled {
                    Button("Remove") {
                        do {
                            try viewModel.libraryManager.removeLibrary(lib)
                            selectedLibrary = nil
                        } catch {
                            viewModel.consoleOutput += "Failed to remove library \(lib.manifest.name): \(error.localizedDescription)\n"
                        }
                    }
                }

                Spacer()

                Button("Done") {
                    viewModel.showLibraryManager = false
                }
                .keyboardShortcut(.defaultAction)
            }
            .padding()
        }
        .frame(width: 700, height: 500)
        .onAppear {
            selectedLibrary = viewModel.libraryManager.libraries.first
        }
        .sheet(isPresented: $showAddSheet) {
            AddLibrarySheet(libraryManager: viewModel.libraryManager, isPresented: $showAddSheet)
        }
    }

    private func libraryDetail(_ lib: LibraryInfo) -> some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 12) {
                // Header
                HStack {
                    Image(systemName: "shippingbox.fill")
                        .foregroundColor(.purple)
                        .font(.title2)
                    VStack(alignment: .leading) {
                        Text(lib.manifest.name)
                            .font(.title3.weight(.semibold))
                        Text("v\(lib.manifest.version)")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    Spacer()
                    Text(lib.source.displayName)
                        .font(.caption)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 3)
                        .background(Color.purple.opacity(0.15), in: Capsule())
                }

                if let desc = lib.manifest.description {
                    Text(desc)
                        .font(.body)
                        .foregroundColor(.secondary)
                }

                if let author = lib.manifest.author {
                    LabeledContent("Author") {
                        Text(author)
                    }
                    .font(.caption)
                }

                if let minVer = lib.manifest.min_engine_version {
                    LabeledContent("Min Engine Version") {
                        Text(minVer)
                    }
                    .font(.caption)
                }

                LabeledContent("Has dylib") {
                    Text(lib.hasDylib ? "Yes" : "No (metadata only)")
                }
                .font(.caption)

                Divider()

                Text("Primitives (\(lib.manifest.primitives.count))")
                    .font(.headline)

                ForEach(lib.manifest.primitives) { prim in
                    HStack(alignment: .top) {
                        Image(systemName: "gearshape")
                            .foregroundColor(.secondary)
                            .frame(width: 16)
                        VStack(alignment: .leading, spacing: 2) {
                            HStack {
                                Text(prim.name)
                                    .font(.body.weight(.medium))
                                Spacer()
                                Text("\(prim.num_inputs) in / \(prim.num_outputs) out")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                            if let desc = prim.description {
                                Text(desc)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                        }
                    }
                    .padding(.vertical, 2)
                }
            }
            .padding()
        }
    }
}

// MARK: - Add Library Sheet

struct AddLibrarySheet: View {
    @ObservedObject var libraryManager: LibraryManager
    @Binding var isPresented: Bool

    @State private var sourceMode: SourceMode = .url
    @State private var urlString: String = ""
    @State private var errorMessage: String?

    enum SourceMode: String, CaseIterable {
        case url = "From URL"
        case local = "From Local Path"
    }

    var body: some View {
        VStack(spacing: 16) {
            Text("Add Library")
                .font(.headline)

            Picker("Source", selection: $sourceMode) {
                ForEach(SourceMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(.segmented)
            .frame(width: 300)

            switch sourceMode {
            case .url:
                urlInputView
            case .local:
                localInputView
            }

            if let error = errorMessage {
                Text(error)
                    .font(.caption)
                    .foregroundColor(.red)
                    .frame(maxWidth: .infinity, alignment: .leading)
            }

            if libraryManager.isDownloading {
                ProgressView("Downloading...")
            }

            HStack {
                Spacer()
                Button("Cancel") {
                    isPresented = false
                }
                .keyboardShortcut(.cancelAction)
            }
        }
        .padding()
        .frame(width: 440)
    }

    private var urlInputView: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Enter an HTTPS URL to a library archive (.tar.gz or .zip):")
                .font(.caption)
                .foregroundColor(.secondary)

            HStack {
                TextField("https://example.com/library.tar.gz", text: $urlString)
                    .textFieldStyle(.roundedBorder)

                Button("Download") {
                    errorMessage = nil
                    guard !urlString.isEmpty else {
                        errorMessage = "Please enter a URL"
                        return
                    }
                    guard urlString.hasPrefix("https://") else {
                        errorMessage = "Only HTTPS URLs are supported"
                        return
                    }
                    Task {
                        do {
                            try await libraryManager.downloadLibrary(from: urlString)
                            await MainActor.run {
                                isPresented = false
                            }
                        } catch {
                            await MainActor.run {
                                errorMessage = error.localizedDescription
                            }
                        }
                    }
                }
                .disabled(libraryManager.isDownloading || urlString.isEmpty)
            }
        }
    }

    private var localInputView: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Select a directory containing a library.json manifest:")
                .font(.caption)
                .foregroundColor(.secondary)

            Button("Choose Directory...") {
                chooseLocalDirectory()
            }
        }
    }

    #if os(macOS)
    private func chooseLocalDirectory() {
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.allowsMultipleSelection = false
        panel.message = "Select a library directory containing library.json"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            do {
                try libraryManager.installFromLocalPath(url)
                isPresented = false
            } catch {
                errorMessage = error.localizedDescription
            }
        }
    }
    #else
    private func chooseLocalDirectory() {}
    #endif
}
