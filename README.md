# Phograph - Visual Dataflow Programming

A modern implementation of the [Prograph](https://en.wikipedia.org/wiki/Prograph) visual dataflow programming language for macOS.

Programs are built by connecting nodes with wires on a visual canvas rather than writing text. Data flows left-to-right through wires, and the system evaluates nodes as their inputs become available.

## Features

- **Visual graph editor** - drag nodes, connect wires, zoom/pan canvas
- **Dataflow evaluation** - token-based firing with automatic scheduling
- **14 data types** - integer, float, boolean, string, list, dict, object, enum, data, date, error, future, method-ref, nothing
- **~400 built-in primitives** - arithmetic, string, list, dict, type, date, JSON, I/O, scene graph, animation, method refs, observables
- **Classes and OOP** - inheritance, protocols, enums, data-determined dispatch
- **Multiple cases** - pattern matching with type/value/list/dict destructuring, case guards with type/value/wildcard
- **Control flow** - execution wires, evaluation nodes, loops, spreads, broadcasts, shift registers, error clusters
- **Async** - futures, channels, managed effects
- **Observable attributes** - reactive attribute system with actor classes
- **Front panel** - runtime UI for interactive programs
- **Scene graph** - shape hierarchy with CPU rasterizer, displayed via Metal
- **Compiler** - graph-to-Swift source code generation for standalone binaries
- **Debugger** - breakpoints, step/rollback, trace values on wires
- **10 plugin libraries** - math, crypto, image, sound, MIDI, networking, file I/O, and more (135 additional primitives)

## Prerequisites

A Mac with Xcode 15+ (macOS 13+ target) and CMake 3.16+ (`brew install cmake`).
The C++ engine alone builds with the Xcode Command Line Tools. The full
prerequisite list is in [docs/building_and_testing.md](docs/building_and_testing.md).

## Quick Start

Clone the repository:

    git clone https://github.com/avwohl/phograph.git
    cd phograph

### Option A: Run the macOS App (Xcode)

    open Phograph.xcodeproj

Select the **Phograph** scheme and press **Cmd+R** to build and run. The app targets macOS 13+.

Or build from the command line:

    xcodebuild -scheme Phograph -destination 'platform=macOS' build

Once running:

1. **File > New Project** (Cmd+N) creates a starter project that computes `(3 + 4) * 2 = 14`
2. Click **Run** in the toolbar to execute
3. **Cmd+K** opens the fuzzy finder to add new nodes

### Option B: Build and Test the C++ Engine Only

    cd phograph_core
    cmake -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

This builds the portable C++ engine as a static library and runs the full test suite (13 test executables, ~200 assertions). No Xcode app or GUI required.

Expected output:

    100% tests passed, 0 tests failed out of 13

## Documentation

- [Learn Phograph](https://avwohl.github.io/phograph/) -- step-by-step tutorial covering dataflow basics through OOP and advanced patterns
- [IDE Guide](https://avwohl.github.io/phograph/guide.html) -- canvas navigation, keyboard shortcuts, debugger, and export
- [Language Reference](https://avwohl.github.io/phograph/reference.html) -- complete reference for all data types, primitives, and evaluation rules

The full language specification is also available at `docs/prograph_language.md` (~2650 lines).

More documentation in `docs/`:

- [docs/architecture.md](docs/architecture.md) -- source tree layout: macOS IDE, portable C++ engine, plugin libraries
- [docs/building_and_testing.md](docs/building_and_testing.md) -- prerequisites, the 13 test executables, library validation, regenerating the Xcode project, contributing

## Library System

Phograph supports plugin libraries that add new primitives to the environment.

- **Install:** Place a library folder (containing `library.json` and implementation files) in `~/Library/Application Support/Phograph/Libraries/`
- **Manage:** Use **Libraries > Manage Libraries...** to view installed libraries, check versions, and toggle availability
- **Use:** Library primitives appear in the right-click context menu and fuzzy finder (Cmd+K) alongside built-in nodes
- **Create:** A library needs a `library.json` declaring its name, version, and primitives (with input/output counts). See `docs/prograph_language.md` for the library specification.

## Examples

Open the Example Browser with **Cmd+Shift+E** to explore built-in examples:

- **Basics** -- arithmetic, string ops, control flow
- **Lists** -- map, filter, sort, list comprehensions
- **Classes** -- OOP with inheritance, instance generators, get/set
- **Graphics** -- scene graph shapes, canvas rendering
- **Patterns** -- loops, spreads, broadcasts, error handling

## Contributing

Contributions are welcome.
See [docs/building_and_testing.md](docs/building_and_testing.md#contributing) for the steps.

## License

GPLv3 License - see [LICENSE](LICENSE)

## Privacy

See [PRIVACY.md](PRIVACY.md)

## Author

Aaron Wohl

https://github.com/avwohl/phograph
