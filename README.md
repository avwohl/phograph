# Phograph - Visual Dataflow Programming

A modern implementation of the [Prograph](https://en.wikipedia.org/wiki/Prograph) visual dataflow programming language for macOS.

Programs are built by connecting nodes with wires on a visual canvas rather than writing text. Data flows left-to-right through wires, and the system evaluates nodes as their inputs become available.

## Features

- **Visual graph editor** - drag nodes, connect wires, zoom/pan canvas
- **Dataflow evaluation** - token-based firing with automatic scheduling
- **13 data types** - integer, float, boolean, string, list, dict, object, enum, data, date, error, future, nothing
- **~400 primitives** - arithmetic, string, list, dict, type, I/O, scene graph, animation
- **Classes and OOP** - inheritance, protocols, enums, data-determined dispatch
- **Multiple cases** - pattern matching with type/value/list/dict destructuring
- **Control flow** - loops, spreads, broadcasts, shift registers, error clusters
- **Async** - futures, channels, managed effects
- **Scene graph** - shape hierarchy with CPU rasterizer, displayed via Metal
- **Compiler** - graph-to-Swift source code generation for standalone binaries
- **Debugger** - breakpoints, step/rollback, trace values on wires

## Getting Started

1. Build and run (see below)
2. Choose **File > New Project** (Cmd+N) to create a starter project
3. The starter project computes `(3 + 4) * 2 = 14`
4. Click **Run** in the toolbar to execute
5. Use **Cmd+K** to open the fuzzy finder and add new nodes

## Architecture

The project follows a portable C++ core with thin platform bridge pattern:

    phograph/                    macOS IDE + app
      Phograph/
        App/                     SwiftUI entry point
        Bridge/                  ObjC++ bridge to C++ engine
        Views/                   SwiftUI IDE views (canvas, browser, inspector)
        ViewModels/              IDE state management
        Model/                   Swift ObservableObject models
        Metal/                   MetalRenderer + shaders
        Runtime/                 Swift runtime for compiled programs

    phograph_core/               Portable C++ engine
      src/
        pho_value.{h,cc}         Tagged union for 13 types
        pho_graph.{h,cc}         Graph model: Node, Wire, Method, Case, Class
        pho_eval.{h,cc}          Dataflow evaluator/scheduler
        pho_prim*.cc             Primitive implementations (~15 files)
        pho_scene.{h,cc}         Scene graph
        pho_draw.{h,cc}          CPU rasterizer
        pho_codegen.{h,cc}       Graph-to-Swift compiler
        pho_debug.{h,cc}         Debugger/trace
        pho_platform.h           Platform abstraction (no implementation)
      tests/                     C++ unit tests

The C++ core has zero platform #includes. All I/O goes through `pho_platform.h`, which has per-platform implementations (Apple: ObjC++ in `Bridge/pho_platform_apple.mm`).

## Building

### Requirements

- Xcode 16+ (Swift 5.9+, C++17)
- macOS 14+
- [xcodegen](https://github.com/yonaskolb/XcodeGen) (for regenerating the Xcode project)

### Build the macOS App

    open Phograph.xcodeproj
    # Select Phograph scheme, press Cmd+R

Or from the command line:

    xcodebuild -scheme Phograph -destination 'platform=macOS' build

### Build the C++ Core (standalone tests)

    cd phograph_core
    cmake -B build
    cmake --build build
    ctest --test-dir build

### Regenerate Xcode Project

If you add/remove source files:

    brew install xcodegen   # one-time
    xcodegen generate

## Documentation

- [Learn Phograph](https://avwohl.github.io/phograph/) -- step-by-step tutorial covering dataflow basics through OOP and advanced patterns
- [IDE Guide](https://avwohl.github.io/phograph/guide.html) -- canvas navigation, keyboard shortcuts, debugger, and export
- [Language Reference](https://avwohl.github.io/phograph/reference.html) -- complete reference for all data types, primitives, and evaluation rules

The full language specification is also available at `docs/prograph_language.md` (~2650 lines).

## Library System

Phograph supports plugin libraries that add new primitives to the environment.

- **Install:** Place a library folder (containing `manifest.json` and implementation files) in `~/Library/Application Support/Phograph/Libraries/`
- **Manage:** Use **Libraries > Manage Libraries...** to view installed libraries, check versions, and toggle availability
- **Use:** Library primitives appear in the right-click context menu and fuzzy finder (Cmd+K) alongside built-in nodes
- **Create:** A library needs a `manifest.json` declaring its name, version, and primitives (with input/output counts). See `docs/prograph_language.md` for the library specification.

## Examples

Open the Example Browser with **Cmd+Shift+E** to explore built-in examples:

- **Basics** -- arithmetic, string ops, control flow
- **Lists** -- map, filter, sort, list comprehensions
- **Classes** -- OOP with inheritance, instance generators, get/set
- **Graphics** -- scene graph shapes, canvas rendering
- **Patterns** -- loops, spreads, broadcasts, error handling

## Contributing

Contributions are welcome.

1. Fork the repository
2. Create a feature branch (`git checkout -b my-feature`)
3. Make your changes -- follow existing code style (SwiftUI views, C++ core)
4. Build and test (`xcodebuild -scheme Phograph -destination 'platform=macOS' build`)
5. Open a pull request against `main`

For C++ engine changes, also run the core tests:

    cd phograph_core && cmake -B build && cmake --build build && ctest --test-dir build

## License

MIT License - see [LICENSE](LICENSE)

## Privacy

See [PRIVACY.md](PRIVACY.md)

## Author

David Wohl

https://github.com/dwohl/phograph
