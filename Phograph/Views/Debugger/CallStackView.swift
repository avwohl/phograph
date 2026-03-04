import SwiftUI

/// Visual call stack display during debugging.
struct CallStackView: View {
    @ObservedObject var viewModel: DebuggerViewModel

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Call Stack")
                .font(.caption)
                .fontWeight(.bold)
                .foregroundColor(.secondary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)

            Divider()

            if viewModel.callStack.isEmpty {
                Text("Not debugging")
                    .font(.caption)
                    .foregroundColor(.secondary)
                    .padding(8)
            } else {
                List(viewModel.callStack.indices.reversed(), id: \.self) { index in
                    let frame = viewModel.callStack[index]
                    HStack {
                        Image(systemName: index == viewModel.callStack.count - 1
                              ? "arrow.right.circle.fill"
                              : "circle")
                            .font(.system(size: 10))
                            .foregroundColor(index == viewModel.callStack.count - 1
                                             ? .yellow : .secondary)
                        Text(frame.methodName)
                            .font(.system(.caption, design: .monospaced))
                        if frame.caseIndex > 0 {
                            Text("case \(frame.caseIndex)")
                                .font(.system(size: 9))
                                .foregroundColor(.secondary)
                        }
                    }
                }
                .listStyle(.plain)
            }
        }
    }
}
