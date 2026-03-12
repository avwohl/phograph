import SwiftUI

/// Searchable popup for node creation via fuzzy matching.
struct FuzzyFinderPopup: View {
    @Binding var isPresented: Bool
    let items: [String]
    let onSelect: (String) -> Void

    @State private var query: String = ""
    @State private var selectedIndex: Int = 0
    @FocusState private var isFocused: Bool

    private var filteredItems: [String] {
        if query.isEmpty { return items }
        return items.filter { item in
            fuzzyMatch(query: query, text: item)
        }.sorted { a, b in
            fuzzyScore(query: query, text: a) > fuzzyScore(query: query, text: b)
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            // Search field
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.secondary)
                TextField("Search nodes...", text: $query)
                    .textFieldStyle(.plain)
                    .focused($isFocused)
            }
            .padding(10)
            .background(Color(white: 0.2))

            Divider()

            // Results list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    ForEach(Array(filteredItems.prefix(20).enumerated()), id: \.offset) { index, item in
                        HStack {
                            Text(item)
                                .font(.system(.body, design: .monospaced))
                                .foregroundColor(.white)
                            Spacer()
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(index == selectedIndex ? Color.blue.opacity(0.3) : Color.clear)
                        .contentShape(Rectangle())
                        .onTapGesture {
                            onSelect(item)
                            isPresented = false
                        }
                    }
                }
            }
            .frame(maxHeight: 300)
        }
        .frame(width: 350)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color(red: 0.15, green: 0.15, blue: 0.18))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(Color(white: 0.3), lineWidth: 1)
        )
        .shadow(radius: 20)
        .onAppear {
            isFocused = true
            selectedIndex = 0
        }
        .onChange(of: query) { _ in
            selectedIndex = 0
        }
    }

    // Simple fuzzy matching
    private func fuzzyMatch(query: String, text: String) -> Bool {
        let q = query.lowercased()
        let t = text.lowercased()
        var qi = q.startIndex
        for tc in t {
            if qi < q.endIndex && tc == q[qi] {
                qi = q.index(after: qi)
            }
        }
        return qi == q.endIndex
    }

    private func fuzzyScore(query: String, text: String) -> Int {
        let q = query.lowercased()
        let t = text.lowercased()
        var score = 0
        var qi = q.startIndex
        var prevMatch = false
        for (i, tc) in t.enumerated() {
            if qi < q.endIndex && tc == q[qi] {
                qi = q.index(after: qi)
                score += 10
                if prevMatch { score += 5 }
                if i == 0 { score += 8 }
                prevMatch = true
            } else {
                prevMatch = false
            }
        }
        score += max(0, 50 - text.count)
        return qi == q.endIndex ? score : 0
    }
}
