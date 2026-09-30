# Architecture

How the Phograph source tree is laid out: the macOS IDE, the portable C++ engine, and the plugin libraries. Back to the [README](../README.md).

The project follows a portable C++ core with thin platform bridge pattern:

    phograph/                    macOS IDE + app
      Phograph/
        App/                     SwiftUI entry point
        Bridge/                  ObjC++ bridge to C++ engine
        Views/                   SwiftUI IDE views (canvas, browser, inspector, front panel)
          IDE/FrontPanelView.swift  Runtime front panel UI
        ViewModels/              IDE state management
        Model/                   Swift ObservableObject models
        Metal/                   MetalRenderer + shaders
        Runtime/                 Swift runtime for compiled programs

    phograph_core/               Portable C++ engine (no platform #includes)
      src/
        pho_value.{h,cc}         Tagged union for 13 types
        pho_graph.{h,cc}         Graph model: Node, Wire, Method, Case, Class
        pho_eval.{h,cc}          Dataflow evaluator/scheduler
        pho_serial.{h,cc}        JSON serialization/deserialization
        pho_bridge.{h,cc}        C API for Swift/ObjC interop
        pho_prim*.cc             Primitive implementations (~25 files)
        pho_prim_date.cc         Date/time primitives
        pho_prim_methodref.cc    Method reference primitives
        pho_scene.{h,cc}         Scene graph
        pho_draw.{h,cc}          CPU rasterizer
        pho_codegen.{h,cc}       Graph-to-Swift compiler
        pho_debug.{h,cc}         Debugger/trace
        pho_thread.{h,cc}        Run loop, timers, event queue
        pho_platform.h           Platform abstraction (no implementation)
        plugins/                 Native audio/MIDI plugin implementations
      tests/                     C++ test suite (13 executables)

    libraries/                   Plugin libraries (10 libraries, 135 primitives)
      math/                      factorial, fibonacci, gcd, lcm, is-prime
      crypto/                    SHA-256, HMAC, AES, random bytes, etc.
      image/                     Load/save PNG/JPEG, resize, pixel access
      sound/                     Audio playback, sample generation
      midi/                      MIDI I/O, note on/off, control change
      fileio/                    Read/write/list files and directories
      socket/                    TCP/UDP client/server
      net/                       HTTP get/post
      bitmap/                    Pixel buffer manipulation
      locale/                    Date/time formatting, locale info

    docs/                        Language spec, design docs
    site/                        GitHub Pages tutorial site

The C++ core has zero platform `#include`s. All I/O goes through `pho_platform.h`, which has per-platform implementations (Apple: ObjC++ in `Bridge/pho_platform_apple.mm`).
