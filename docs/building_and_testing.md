# Building and Testing

Prerequisites, the test suites, regenerating the Xcode project, and how to contribute. Back to the [README](../README.md).

## Prerequisites

You need a Mac with the following installed:

    Tool          How to get it                      Verify with
    ----          ---------------                    -----------
    Git           Xcode Command Line Tools (below)   git --version
    Xcode 15+     Mac App Store                      xcodebuild -version
    CMake 3.16+   brew install cmake                 cmake --version

Xcode installs the Apple Clang C++17 compiler, the macOS SDKs (CoreAudio, CoreMIDI, CoreGraphics, ImageIO, Security, etc.), and the Swift toolchain. If you only want to run the C++ engine tests and do not need the full IDE, you can use the Xcode Command Line Tools alone (without the full Xcode app):

    xcode-select --install       # installs git, clang, make, etc.
    brew install cmake           # CMake is not included with Xcode

Optional tools:

    Tool          Purpose                            Install
    ----          -------                            -------
    XcodeGen      Regenerate Xcode project           brew install xcodegen
    Homebrew      Install CMake and XcodeGen          /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

## Testing

### C++ Engine Tests

    cd phograph_core
    cmake -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

The test suite contains 13 executables:

    Test                What it covers
    ----                --------------
    test_eval           Core evaluator, JSON parser, basic graph execution
    test_phase2         String, list, dict, type, data, error primitives; multi-case dispatch; recursion
    test_phase3         OOP: classes, instance generators, get/set, inheritance
    test_bridge         C bridge API: load JSON, call methods, pixel buffer, error handling
    test_scene          Scene graph: shapes, transforms, hit testing, canvas, drawing
    test_phase6         Run loop: events, timers, input handling, easing, animation
    test_phase7         IDE rendering: fuzzy search, node layout, wire/grid rendering
    test_phase8         Debugger: breakpoints, traces, snapshots, step control
    test_phase9         Async: futures, channels, effects
    test_phase10        Compiler: Swift code generation, name mangling, topo sort
    test_e2e_compile    End-to-end: compile graph to Swift, run swiftc, verify output
    test_phase11_28     Phases 11-28: execution wires, loops, listMap, try/error, pattern matching, observables
    test_comprehensive  Full-language bridge test: all primitives, types, control flow, OOP, canvas

### Library Validation

    bash tests/test_libraries.sh

Checks that all 10 library manifests are valid JSON, primitives are unique, C++ registration functions exist, and primitive counts match expectations.

### macOS App

    xcodebuild -scheme Phograph -destination 'platform=macOS' build

## Regenerating the Xcode Project

The checked-in `Phograph.xcodeproj` is generated from `project.yml`. If you add or remove source files:

    brew install xcodegen   # one-time
    xcodegen generate

## Contributing

Contributions are welcome.

1. Fork the repository
2. Create a feature branch (`git checkout -b my-feature`)
3. Make your changes -- follow existing code style (SwiftUI views, C++ core)
4. Run the C++ engine tests:

       cd phograph_core && cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure

5. Build the app (if you changed Swift/UI code):

       xcodebuild -scheme Phograph -destination 'platform=macOS' build

6. Open a pull request against `main`
