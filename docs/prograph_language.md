# Phograph Language Specification

A comprehensive reference for implementing a modern Prograph-based visual dataflow programming language for macOS and iOS.

---

## Table of Contents

1. [History and Origins](#1-history-and-origins)
2. [Core Language Model](#2-core-language-model)
3. [Program Structure](#3-program-structure)
4. [Data Types](#4-data-types)
5. [Operations and Icons](#5-operations-and-icons)
6. [Primitives Reference](#6-primitives-reference)
7. [Control Flow](#7-control-flow)
8. [Loops and List Annotations](#8-loops-and-list-annotations)
9. [Classes and Object-Orientation](#9-classes-and-object-orientation)
10. [Persistents and Global State](#10-persistents-and-global-state)
11. [Canvas, Drawing, and Scene Graph](#11-canvas-drawing-and-scene-graph)
12. [Input: Pointer, Gesture, and Keyboard Events](#12-input-pointer-gesture-and-keyboard-events)
13. [Execution Model and Debugging](#13-execution-model-and-debugging)
14. [External Code Integration](#14-external-code-integration)
15. [Known Limitations and Status](#15-known-limitations-of-the-original-and-how-phograph-addresses-them)
16. [References](#16-references)

---

## 1. History and Origins

### Academic Beginnings (1982-1985)

Research on Prograph began at **Acadia University** (Nova Scotia, Canada) in 1982 as a general investigation into dataflow languages, stimulated by a seminar on functional languages conducted by Michael Levin. Diagrams were used to clarify discussions, leading to the central insight: *"since the diagrams are clearer than the code, why not make the diagrams themselves executable!"*

Key researchers:

- **Tomasz Pietrzykowski** -- primary visionary; formed The Gunakara Sun Systems (TGS) consulting firm at Acadia University
- **Stan Matwin** -- co-authored the 1985 preliminary report
- **Philip T. Cox** -- joined from the Technical University of Nova Scotia; later became CTO and President/CEO

The 1985 paper by Matwin and Pietrzykowski formally described PROGRAPH as "a functional, data flow oriented language, expressed graphically in the form of pictographs" that includes "a database subsystem which is also functional in nature."

In 1985, work began on a commercializable prototype for the Macintosh, chosen because it was "the only widely available, low-priced computer with high-level graphics support."

### Commercialization: TGS Systems (1986-1990)

In early 1986, The Gunakara Sun Systems took over the prototype for commercialization.

In **1987**, **Mark Szpakowski** suggested the merger of object-orientation with visual dataflow, creating an "objectflow" system -- a pivotal conceptual development.

**Version timeline:**

| Version | Date | Notes |
|---------|------|-------|
| 1.0 | October 1988 | First release |
| 1.1 | March 1989 | Numerous additions |
| 1.2 | September 1989 | Near-complete Mac Toolbox coverage; 1989 MacUser Editor's Choice Award for Best Development Tool |
| 2.0 | July 1990 | Added a compiler (previously interpreter-only); TGS renamed to Prograph International |
| 2.5 | October 1991 | Included a sparse class library (System Classes, Essentials, Goodies) |

### Prograph CPX (1992-1995)

**Prograph CPX** (Cross-Platform eXtensions) was released in 1993. CPX consisted of three parts:

1. **Editor/Interpreter** -- an incremental compiler approximately 10x faster than the Prograph 2.5 interpreter
2. **Compiler** -- produces standalone executables
3. **Application Builder Classes (ABCs)** -- a complete application framework

Despite increasing sales, Prograph International was unable to sustain operating costs and went into receivership in early 1995.

### Pictorius and Later (1995-2002)

Management and employees formed **Pictorius**, which acquired PI's assets. A Windows version of CPX was begun but never formally released. In April 2002, the web development part was acquired by Paragon Technology Group. The Prograph source code rights were retained by McLean Watson Capital.

### Marten by Andescotia (2010s-present)

**Andescotia Software** released **Marten IDE**, a macOS-native revival of Prograph. Marten is a multi-process, multi-threaded Carbon application (~500 classes) supporting Carbon and Cocoa frameworks, database APIs (MySQL, Postgres), and C code export. Available as a free download.

---

## 2. Core Language Model

### The Dataflow Paradigm

Phograph is a **visual, object-oriented, dataflow, multiparadigm programming language** that uses iconic symbols to represent actions taken on data. It descends from Prograph but incorporates ideas from 40 years of dataflow language evolution -- Unreal Blueprints, Houdini, LabVIEW, vvvv, Enso, Max/MSP, Hazel, the Elm Architecture, and others.

The fundamental execution model:

- **Data flows from top to bottom** through a directed acyclic graph of operation icons
- Operations execute **when all their input data is available** (data-driven firing rule)
- Execution order depends solely on data dependencies, not spatial position
- This model is **inherently parallelizable**: independent operations with no data dependencies between them can execute concurrently as Swift tasks (see §13)

Phograph is **not a pure dataflow language** -- data flowing through diagrams can include class instances subject to side-effects. It is formally "a class-based, single inheritance object-oriented language with dynamic typing, protocols, and automatic reference counting."

### Dual Wire Types: Data and Execution

Phograph has two kinds of wires, inspired by Unreal Blueprints and Cables.gl:

- **Data wires** (solid lines): carry values between operations. This is the classic Prograph wire. Data flows top-to-bottom. An operation fires when all data inputs are available.
- **Execution wires** (dashed gray lines with arrow): enforce sequencing between operations that have side effects. Marked `is_execution=true` in the graph model. An execution wire carries no data -- it only signals completion. An operation with an execution-in pin fires only after the upstream operation on the execution wire has completed **and** all data inputs are available.

**Why both?** Pure dataflow can express any computation, but when operations have side effects (file I/O, network calls, UI mutations), the programmer must control the order. Original Prograph used **synchro links** for this, but they were awkward and disconnected from the data model. Execution wires make sequencing first-class and visible.

**Rules:**
- Execution wires are **optional**. Pure-dataflow graphs (no side effects) need no execution wires.
- An operation can have **both** data inputs and an execution-in pin. It fires when all data inputs are available **and** the execution wire is triggered.
- Execution wires form a linear chain (or tree) through a method. Multiple execution wires can fan out from one operation (parallel execution of the targets).
- If an operation has no execution-in pin, it fires purely on data availability (classic dataflow).

### Hybrid Evaluation: Push, Pull, and Tick

Phograph uses a **hybrid evaluation model** rather than pure push or pure pull:

- **Pull-based (demand-driven)** by default: computation proceeds only when a downstream operation demands a value. Unused branches are never evaluated. This is critical for performance (inspired by Nuke's ROI propagation and Kotlin Flow's cold-by-default).
- **Push-based (event-driven)** for reactive UI: when an observable attribute changes or a user event fires, the change pushes downstream through bound nodes. This maps to Combine publishers.
- **Tick-based (synchronous)** for animation: during each display frame, all animation-connected nodes evaluate synchronously. The current frame is a "tick" in the Lustre/SCADE sense. Maps to `CADisplayLink`.
- **Async** for I/O: async operations return Futures. The dataflow firing rule handles waiting naturally (see §7.2). Maps to Swift `Task`.

### Dual Visual/Textual Representation

Inspired by Enso's isomorphic representation, every Phograph graph has an equivalent **Swift source code** representation. The programmer can switch between views:

- **Visual view**: the dataflow graph (primary editing mode)
- **Text view**: generated Swift code (read-only by default; editable for dense computation)

This solves the "screen real estate" problem: a 50-node arithmetic subgraph that occupies a large screen area may be a single line of Swift. The programmer can collapse a subgraph into an **inline expression node** (see §5) or switch to text view for complex math.

The dual representation also enables:
- **Text-based diffs and code review** (git-friendly)
- **Searching** code with standard text tools
- **Copy-paste** between Phograph and Swift projects

The Swift representation is the **compilation target** -- the visual graph compiles to Swift source, which compiles to native ARM64 via the Swift compiler.

### Visual Syntax

The visual syntax is the **primary** representation of Phograph code. The editor enforces structural validity by construction -- invalid connections are rejected. The approximately twenty operation icon types each have:

- **Input nodes** (terminals): shaped pins at the **top** of the icon (shape indicates type; see Pin Shapes below)
- **Output nodes** (roots): shaped pins at the **bottom** of the icon
- **Data links** (wires): solid lines connecting the root (output) of one icon to the terminal (input) of another
- **Execution links**: dashed lines with arrows connecting execution-out to execution-in pins

Data flows top-to-bottom through wires. In debug/step mode, data can be observed on every wire with **live value visualization** -- hover over any wire to see its current value (or its value from the last execution trace).

### Pin Shapes and Type Indication

Inspired by Scratch/Snap!'s shape-based type system, Phograph pins have shapes that communicate type compatibility at a glance:

| Pin Shape | Type Category | Examples |
|-----------|--------------|----------|
| **Circle** | Scalar | Integer, Real, String |
| **Hexagon** | Boolean | Boolean values (distinct shape for at-a-glance recognition) |
| **Square** | Collection | List, Dict |
| **Diamond** | Optional | Any type that may be null |
| **Pentagon** | Object | Class instances, External references |
| **Triangle** | Execution | Execution-in / execution-out flow |
| **Star** | Error | Error output from try annotations |

**Colors** further distinguish types within a shape category: Integer (blue circle), Real (green circle), String (pink circle), Data (dark cyan circle), Date (amber circle), Boolean (red hexagon), List (orange square), Dict (purple square), Object (teal pentagon), External (gray pentagon), Enum (indigo pentagon), MethodRef (olive pentagon), Null (white diamond), Error (bright red star).

### Visible Coercion Dots

Inspired by LabVIEW, wherever Phograph performs an implicit type conversion (e.g., Integer to Real, or value to String), a small **coercion dot** appears on the wire at the point of conversion. This makes type coercions visible rather than invisible magic. The dot's color indicates the conversion direction.

### Typed Holes: Always-Runnable Graphs

Inspired by Hazel, an incomplete Phograph graph is **always valid and always runnable**:

- An unconnected input pin is a **typed hole** with an inferred type and a placeholder/default value (zero for numbers, empty string for strings, empty list for lists, null for objects).
- The type of a hole is inferred from context (what the pin expects).
- When the graph runs with holes, the holes produce their placeholder values, and the graph executes to the extent possible, producing partial results.
- The editor visually marks holes (pulsing outline) so they're obvious but not errors.
- **Fill-and-resume**: when the programmer connects a wire to a hole in a running graph, only the affected subgraph re-evaluates. The rest of the graph keeps its results. This is incremental computation at the language level.

**Design principle:** There is no "broken" state. Every intermediate editing state produces some output. This is the single most important UX principle for a visual programming language.

### No Local Variables

There are no named local variables. Data exists only as values flowing through wires between operations. This eliminates an entire category of bugs (uninitialized variables, naming conflicts) and makes data dependencies immediately visible.

---

## 3. Program Structure

### Hierarchy: Project > Section > Classes / Methods / Persistents

A **Project** is the top-level container, displayed in a Project Window. It contains one or more **Sections**, which are independent units of compilation saved as separate files, reusable across multiple projects.

Each Section has three compartments (visually represented as a three-part icon):

1. **Classes** (left) -- object-oriented code and data
2. **Universal Methods** (middle) -- standalone functions not tied to any class
3. **Persistents** (right) -- global persistent variables

**Rules:**
- A class must be entirely contained within a single section
- A method must be entirely contained within a single section
- Class names, universal method names, and persistent names must be unique within a project
- Inheritance chains can span any number of sections
- The same section may be used in any number of projects

### Methods and Cases

A **method** consists of one or more **cases**. Each case is a separate dataflow diagram (directed graph) displayed in its own window. The window title shows `case_number:total_cases` (e.g., "2:3" = case 2 of 3 cases).

Each case contains:

- An **input bar** at the top with root nodes (representing method parameters)
- An **output bar** at the bottom with terminal nodes (representing return values)
- **Operation icons** in the body
- **Data links** (wires) connecting outputs to inputs

When a method is called:
1. Data values from the calling operation's input wires are copied to the first case's input bar roots
2. Operations in the case execute as their inputs become available
3. If a control annotation causes a "next case" jump, execution moves to the next case
4. Data produced at the output bar's terminal nodes flows back to the caller

### Method Arity and Optional Parameters

Every method has a defined **arity** -- a specific number of inputs and outputs. The visual editor displays and enforces this.

**Phograph extension -- optional inputs:** An input node can be marked as **optional** (visually: a dashed circle instead of solid). Optional inputs have a **default value** specified in the node's properties (`PinDef.is_optional = true`, `PinDef.default_value`). If no wire is connected to an optional input, the default value is used. A node fires when all **required** (non-optional) inputs are filled; optional inputs use their defaults if unconnected. This reduces method proliferation -- instead of writing three methods for different argument counts, write one with optional parameters.

```
Method "draw-circle" (3 required, 2 optional):
  Input: ctx          (required)
  Input: center       (required)
  Input: radius       (required)
  Input: fill-color   (optional, default: black)
  Input: stroke-width (optional, default: 0.0)
```

**Rules:**
- Optional inputs must come after all required inputs
- When calling, wires to optional inputs can be omitted
- The editor shows optional inputs with dashed circles and default values annotated
- Output nodes cannot be optional

**Hot and cold pins** (see also §5): Each input pin has an `is_hot` flag (default true). Hot pins trigger evaluation when new data arrives; cold pins store data passively. The hot/cold distinction determines firing behavior in reactive and streaming graphs.

**Phograph extension -- variadic inputs:** An input node can be marked as **variadic** (visually: ellipsis inside the circle). A variadic input accepts any number of wires; the values are collected into a list. Only one variadic input is allowed per method, and it must be the last input.

```
Method "concat" (variadic):
  Input: strings... (variadic)
  Output: result
```

### Local Methods

A **local method** groups operations within a parent method for readability. Visually distinct icon (vertical lines on left and right sides). Zero function-call overhead. Not callable from outside the containing method. Can be promoted to a universal method.

Created by selecting operations and choosing "Opers To Local." Can be reversed with "Local To Method."

### Creating Methods at Runtime (Prototyping)

During interpretation, if a called method doesn't exist, an alert offers to create it on-the-fly. This enables **top-down prototyping**: write high-level skeleton code, run it, and fill in details incrementally as the interpreter prompts for missing implementations.

---

## 4. Data Types

Phograph has **fourteen data types**:

| Type | Description |
|------|-------------|
| **Integer** | Whole numbers (64-bit signed) |
| **Real** | Floating-point numbers (64-bit IEEE 754 double) |
| **String** | Text data (UTF-8, treated as a unit, not a character array) |
| **Boolean** | `true` / `false` |
| **List** | Ordered collection; elements can be of any type; nesting supported |
| **Dict** | Key-value map; keys can be any hashable type (integer, real, string, boolean); values any type |
| **Data** | Raw binary byte buffer (for file I/O, network payloads, images, cryptography) |
| **Date** | Point in time with nanosecond precision (see Date/Time Primitives below) |
| **Object** | Instance of a user-defined class |
| **External** | Opaque reference to a platform/FFI resource (Swift object, C pointer, OS handle) |
| **Error** | Error value with message, code, and optional details (see §7.1) |
| **Enum** | Value of an enum type with optional associated data (see §9, Enum Types) |
| **MethodRef** | First-class reference to a method (see §5, Method References) |
| **Null** | The single "nothing" value. Represents absence of data. |

**Design change from original Prograph:** The original had three confusing "nothing" values -- NULL, NONE, and Undefined -- with unclear, overlapping semantics. Phograph collapses these into a single `null`. An uninitialized attribute is `null`. An absent return value is `null`. There is one nothing, not three.

### Dynamic Typing

Phograph is **dynamically typed** (late-binding, duck-typed, comparable to Python). Types are checked at runtime, not compile time. Persistents and class attributes can hold any data type and can change types during execution. Declaring a type in a class definition sets only the default initial value.

### Optional Type Annotations

While Phograph is dynamically typed, **optional type annotations** can be added to method input/output nodes and class attributes. These serve as:
- Documentation (visible in the editor as labels on nodes)
- Runtime validation (if enabled, a type mismatch produces a clear error rather than a downstream failure)
- Editor assistance (autocomplete, refactoring)

Annotations are never required. When present, they are checked at runtime on entry to the annotated method. Annotation syntax in the node label: `name: Type` (e.g., `count: Integer`, `items: List`, `config: Dict`).

### Constants

Depicted as a horizontal line with a root node and the value displayed above. Immutable. Types include integers, reals, strings, lists, dicts, booleans, and null.

### Strings

- UTF-8 encoded, full Unicode support
- Entire blocks of text treated as a single entity, not a character array
- Number base representation: decimal (plain digits), hex (`"16#<digits>"`), octal (`"8#<digits>"`), binary (`"2#<digits>"`)
- String interpolation: `"Hello, {name}"` where `{name}` is replaced by the value of the wire named `name` entering the constant

### Lists

- Flexible arrays; elements need not be the same type; size not pre-declared
- Literal syntax in dialogs: `(1 2 3 4)`
- Nested lists (2D or higher): `((1 2 3)(4 5 6))`
- Safe access: out-of-bounds `get-nth` produces a **failure** (not an error), which can be handled by control annotations
- 1-indexed (not 0-indexed)

### Dicts (Dictionaries)

A **Dict** is an unordered key-value map. Keys must be hashable (integer, real, string, boolean). Values can be any type.

- Literal syntax in dialogs: `{name: "Alice", age: 30, scores: (95 87 92)}`
- Nested dicts: `{user: {name: "Alice", email: "a@b.com"}}`
- Missing key access produces a **failure** (not an error), handled by control annotations

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `dict-create` | -- | dict | Create an empty dict |
| `dict-get` | dict, key | value | Get value for key (fails if key absent) |
| `dict-get-default` | dict, key, default | value | Get value for key, or default if absent |
| `dict-set` | dict, key, value | dict | Return new dict with key set (immutable) |
| `dict-set!` | dict, key, value | -- | Set key in place (mutating) |
| `dict-remove` | dict, key | dict | Return new dict with key removed |
| `dict-has?` | dict, key | boolean | Check if key exists |
| `dict-keys` | dict | list | List of all keys |
| `dict-values` | dict | list | List of all values |
| `dict-pairs` | dict | list | List of (key, value) pairs |
| `dict-size` | dict | integer | Number of entries |
| `dict-merge` | dict1, dict2 | dict | Merge two dicts (dict2 wins on conflicts) |

Dicts work with **list annotations**: applying an ellipsis-annotated operation to a dict iterates over its (key, value) pairs, each delivered as a two-element list.

### Evaluations (Inline Expressions)

Small formula-like constructs embedded in an operation icon. Hold a single expression with inputs named by the wires connected to them. No function-call overhead. Example: `b*b - 4*a*c` with inputs `a`, `b`, `c`.

**Design change from original Prograph:** The original limited evaluations to 26 inputs named `a` through `z` and supported only arithmetic. Phograph **inline expression nodes** (see §5) extend evaluations with a richer expression language: arithmetic, comparison, boolean logic, string concatenation, ternary (`condition ? then : else`), and method calls. Inputs are bound to variables `a`, `b`, `c`... (positional) as well as `input0`, `input1`... (indexed), so expressions like `a * b + c` or `input0 > input1 ? input0 : input1` work naturally. Wire labels name the inputs, with no fixed limit. Evaluations and inline expressions are the same feature -- "evaluation" is the original Prograph term; "inline expression" is the modern name.

### Memory Management

**Automatic Reference Counting (ARC)** with **cycle detection**. Each object tracks its reference count; when the count reaches zero, the object is deallocated immediately (deterministic destruction). A background cycle detector periodically scans for reference cycles among objects and breaks them.

This replaces the original's pure reference counting (which leaked cycles) and aligns with Swift's ARC model for seamless interop. Deterministic destruction enables **automatic cleanup**: objects that hold resources (file handles, network connections, GPU buffers) can implement a `/finalize` method that is called when the object is deallocated.

**Design change from original Prograph:** The original had no destructors. Phograph adds `/finalize` -- an optional method called automatically when an object's reference count reaches zero. Use it for resource cleanup (close files, release GPU buffers, disconnect network sockets). Not a replacement for explicit cleanup in all cases, but a safety net.

### Data (Binary Buffers)

A **Data** value is a contiguous buffer of raw bytes. Used for binary file I/O, network payloads, image data, and cryptographic operations.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `data-create` | size | data | Create a zero-filled buffer of N bytes |
| `data-from-list` | list of integers | data | Create from a list of byte values (0-255) |
| `data-to-list` | data | list of integers | Convert to list of byte values |
| `data-length` | data | integer | Number of bytes |
| `data-slice` | data, offset, length | data | Extract a sub-range |
| `data-concat` | data1, data2 | data | Concatenate two buffers |
| `data-to-string` | data, encoding | string | Decode bytes to string (`"utf8"`, `"ascii"`, `"base64"`) |
| `data-from-string` | string, encoding | data | Encode string to bytes |
| `data-get-byte` | data, index | integer | Get byte at index (0-indexed, since bytes are not a Phograph collection) |
| `data-set-byte` | data, index, value | data | Return new buffer with byte set |

Data values are **immutable by default** (operations return new buffers). For performance-critical scenarios, mutable variants (`data-set-byte!`) modify in place.

Maps to Swift `Data` for interop.

### Date and Time

A **Date** represents an absolute point in time with nanosecond precision. Internally stored as seconds since the Unix epoch (January 1, 1970 UTC). Maps to Swift `Date` for interop.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `date-now` | -- | date | Current date and time |
| `date-create` | year, month, day, hour, minute, second | date | Create a date from components (in local timezone) |
| `date-create-utc` | year, month, day, hour, minute, second | date | Create a date from components (in UTC) |
| `date-components` | date | year, month, day, hour, minute, second | Decompose a date (in local timezone) |
| `date-components-utc` | date | year, month, day, hour, minute, second | Decompose a date (in UTC) |
| `date-to-timestamp` | date | real | Seconds since Unix epoch |
| `date-from-timestamp` | real | date | Create from Unix timestamp |
| `date-add` | date, seconds | date | Add a duration (in seconds) to a date |
| `date-diff` | date1, date2 | real | Difference in seconds between two dates |
| `date-format` | date, format-string | string | Format a date (e.g., `"yyyy-MM-dd HH:mm:ss"`) |
| `date-parse` | string, format-string | date | Parse a date from a string (fails if format doesn't match) |
| `date-weekday` | date | integer | Day of week (1=Sunday, 7=Saturday) |
| `date-compare` | date1, date2 | integer | -1, 0, or 1 (for use with `sort-by`) |

**Duration primitives** for convenience:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `seconds` | n | real | n seconds (identity, for readability) |
| `minutes` | n | real | n * 60 seconds |
| `hours` | n | real | n * 3600 seconds |
| `days` | n | real | n * 86400 seconds |

Dates are compared with standard comparison primitives (`=`, `<`, `>`, etc.).

---

## 5. Operations and Icons

### Icon Taxonomy

| Icon Type | Visual Appearance | Description |
|-----------|-------------------|-------------|
| **Plain operation** | Rectangle | Unnamed placeholder; becomes typed once named |
| **Universal method** | Rectangle with name | Call to a standalone method |
| **Primitive** | Rectangle with single line at bottom edge | Built-in operation supplied by the runtime |
| **External method** | Rectangle with lines at top and bottom edges | Call to Swift/C function (see §14) |
| **Local method** | Rectangle with vertical lines on left and right | Grouped operations within a parent method |
| **Instance generator** | Octagon | Creates a new instance of a class |
| **Persistent** | Oval | Accesses a global persistent variable |
| **Get operator** | Special icon | Reads an attribute value from an object |
| **Set operator** | Special icon | Writes an attribute value to an object |
| **Constant** | Horizontal line with root and value label | Supplies a fixed value |
| **Evaluation** | Similar to method but with equation inside | Inline formula |
| **Inject** | Rectangle with inject input node | Dynamic dispatch: method name supplied at runtime |
| **Match** | Operation with attached control annotation | Conditional test |

### Node Types

- **Root nodes** (outputs): shaped pins at the bottom of an icon; data exits here (shape indicates type, see §2 Pin Shapes)
- **Terminal nodes** (inputs): shaped pins at the top of an icon; data enters here

### Data Links (Wires)

Lines connecting a root of one icon to a terminal of another. Data flows along these links from producer to consumer. Multiple wires can originate from a single root (fan-out / data duplication). A terminal can receive from at most one root.

### Synchro Links (Deprecated)

The original Prograph's **synchro links** are superseded by **execution wires** (see §2). Synchro links are accepted by the parser for backward compatibility but the editor converts them to execution wires automatically.

### Execution Pins

Operations that have side effects can have **execution-in** (triangle at top-left) and **execution-out** (triangle at bottom-left) pins. These accept execution wires that enforce sequencing.

- Execution-in: the operation does not fire until the upstream execution wire triggers, even if all data inputs are available.
- Execution-out: fires after the operation completes, triggering downstream operations on execution wires.
- An operation can have multiple execution-out pins (fan-out = parallel execution of targets).

**Pure operations** (no side effects) have no execution pins. The editor can infer purity: an operation with no Set, no I/O primitive, and no external call is pure. Pure operations fire on data availability alone.

### Hot and Cold Inlets

Inspired by Max/MSP, each data input pin on an operation is either **hot** or **cold**:

- **Hot inlet** (filled pin): when new data arrives on this pin and all other inputs are available, the operation fires. This is the default for all pins in classic Prograph.
- **Cold inlet** (hollow pin): new data arriving on this pin does NOT trigger firing. The value is stored and used when the operation next fires (triggered by a hot inlet or execution wire).

**Why?** In reactive/streaming graphs, an operation like `multiply(price, quantity)` should not re-fire every time `quantity` updates AND every time `price` updates -- you typically want it to fire when `price` updates, using the latest `quantity`. Making `quantity` cold solves this.

- By default, all inlets are hot (classic Prograph behavior).
- The programmer toggles an inlet between hot/cold via a context menu.
- Cold inlets are visually distinct (hollow circle vs filled circle within their shape).
- In non-reactive (one-shot) execution, the hot/cold distinction has no effect -- the operation still fires when all inputs are available.

### Inline Expression Nodes

Inspired by Houdini's VEX expressions, an **inline expression node** embeds a small textual expression directly inside an operation icon, avoiding the need to create a subgraph for simple math or string operations.

An inline expression node:
- Displays the expression text inside the icon (e.g., `a * b + c`)
- Has input pins named after the variables in the expression
- Has one output pin for the result
- Supports arithmetic, comparison, boolean logic, string interpolation, and ternary (`condition ? then : else`)
- Is semantically equivalent to a subgraph of primitive operations, but takes much less screen space

This extends the existing **Evaluation** icon (§4) with a richer expression language. Simple computations that would require 3-5 nodes can be a single inline expression.

### Reroute Nodes

Inspired by Unreal Blueprints, a **reroute node** (also called a knot) is a zero-operation pass-through that lets the programmer route wires around visual obstacles. A reroute node has one input and one output of the same type. It adds no computation -- it's purely for wire management.

### Fuzzy Finder for Node Creation

Inspired by Unity Visual Scripting and Enso, the **primary way to add nodes** to a graph is a fuzzy-search popup:

- Triggered by double-clicking empty canvas, pressing Tab, or typing
- Shows a searchable list of all available operations, primitives, class methods, and Swift API nodes
- **Context-sensitive**: if triggered by dragging from an existing pin, the list is filtered to show only compatible operations (matching the pin's type)
- Supports fuzzy matching: typing "htgt" matches "http-get"
- Shows type signatures and brief descriptions in the popup
- Recent/frequent operations appear at the top

### Evaluation Nodes (NodeType::Evaluation)

An **evaluation node** (`NodeType::Evaluation`) holds an expression string that is evaluated inline. Inputs are bound to named variables: the first input is `a` (alias `input0`), the second is `b` (alias `input1`), and so on. The expression supports:

- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Boolean**: `&&`, `||`, `!`
- **String concatenation**: `+` on strings
- **Ternary**: `condition ? then_value : else_value`

Examples: `a * b + c`, `a > 0 ? a : -a`, `a + " world"`.

Evaluation nodes are semantically equivalent to a subgraph of primitive operations but take far less screen space. They compile to a single Swift expression with zero function-call overhead.

### Local Method Nodes (NodeType::LocalMethod)

A **local method node** (`NodeType::LocalMethod`) embeds a complete method body (`Node.local_method`) inside a parent method. The embedded method is evaluated inline when the node fires. Local methods are not callable from outside their containing method -- they exist purely for organizational clarity.

Local methods are created by selecting operations and choosing "Opers To Local." They can be promoted to universal methods later.

### Inject Construct (NodeType::Inject)

An **inject** determines at runtime which method to call. The first input is the method name as a string or a method reference; remaining inputs are passed as arguments to the resolved method. This enables:

- Runtime dispatch based on user selection
- Function-pointer-like behavior
- Callback patterns
- Plugin/extension architectures where behavior is selected dynamically

### Method References (Phograph Extension)

The original inject construct takes a method name as a **string** -- stringly-typed dispatch with no validation. Phograph adds **method references**: first-class values that refer to a specific method and can carry captured state.

A **method reference** is created with the `method-ref` icon (rectangle with an arrow). It produces a value on its output wire that refers to the named method. This value can flow along wires, be stored in lists or attributes, and be invoked later.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `method-ref` | method-name | reference | Create a reference to a universal method by name |
| `method-ref-class` | class-name, method-name | reference | Create a reference to a class method |
| `method-ref-bound` | object, method-name | reference | Create a reference bound to a specific object (captures `self`) |
| `call` | reference, args-list | result | Invoke a method reference with arguments |
| `call-async` | reference, args-list | future | Invoke a method reference asynchronously, returns a Future |
| `is-method-ref?` | value | boolean | Test if a value is a method reference |
| `method-ref-name` | reference | string | Get the method name from a reference |

Method references replace stringly-typed inject for most uses. The editor validates that the referenced method exists at edit time (though the check is advisory, not enforced until runtime).

**Bound references** capture the target object, enabling closures-like behavior:

```
[method-ref-bound] my-button, "on-tap" → ref
[timer-after] 2.0, ref → timer
```

This replaces the inject pattern of passing a method name string and hoping it resolves correctly at runtime.

**Design note:** The original inject construct is retained for backward compatibility and for cases where the method name genuinely must be computed at runtime (e.g., from user input). Method references are the preferred mechanism.

---

## 6. Primitives Reference

Phograph includes approximately **400 built-in primitives**. Key categories:

### Console / Debug I/O Primitives

The original Prograph `show`/`ask`/`answer` primitives were blocking modal dialogs. Phograph replaces them with non-blocking canvas-based equivalents (see §12.8: `alert`, `prompt`, `confirm`) for user-facing interaction. The following are retained for **debugging and console output only**:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `log` | value(s) | -- | Print value(s) to the debug console; polymorphic, accepts any type. Does not block. |
| `inspect` | value | value | Print value to debug console and pass it through unchanged. Useful for inserting into a dataflow chain without breaking it. |
| `breakpoint` | -- | -- | Pause execution here if debugger is attached. No-op in compiled builds. |

### Arithmetic Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `+` | a, b | sum | Addition |
| `-` | a, b | difference | Subtraction |
| `*` | a, b | product | Multiplication |
| `/` | a, b | quotient | Division |
| `div-mod` (`÷÷`) | a, b | quotient, remainder | Integer division with remainder (two outputs) |
| `abs` | n | \|n\| | Absolute value |
| `round` | n | rounded | Round to nearest integer |
| `sqrt` | n | root | Square root |
| `power` | base, exp | result | Exponentiation |
| `pi` | -- | 3.14159... | Pi constant |
| `sin` | radians | sine | Sine |
| `cos` | radians | cosine | Cosine |
| `tan` | radians | tangent | Tangent |
| `asin` | value | radians | Arc sine |
| `acos` | value | radians | Arc cosine |
| `atan` | value | radians | Arc tangent |
| `atan2` | y, x | radians | Arc tangent of y/x (preserves quadrant) |
| `rand` | -- | random real | Random number [0, 1) |
| `+1` | n | n+1 | Increment by one |
| `mod` | a, b | remainder | Modulo (remainder of a / b) |
| `min` | a, b | minimum | Minimum of two values |
| `max` | a, b | maximum | Maximum of two values |
| `clamp` | value, low, high | clamped | Clamp value to range [low, high] |
| `floor` | n | floor | Floor |
| `ceiling` | n | ceiling | Ceiling |
| `truncate` | n | truncated | Truncate to integer |
| `ln` | n | natural-log | Natural logarithm |
| `log10` | n | log-base-10 | Base-10 logarithm |
| `log2` | n | log-base-2 | Base-2 logarithm |

### Comparison / Relational Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `=` | a, b | boolean | Equality (polymorphic) |
| `<` | a, b | boolean | Less than |
| `>` | a, b | boolean | Greater than |
| `<=` (`≤`) | a, b | boolean | Less than or equal |
| `>=` (`≥`) | a, b | boolean | Greater than or equal |
| `!=` (`≠`) | a, b | boolean | Not equal (polymorphic) |

All comparison primitives automatically carry a **control annotation** for use in matches.

### Logical / Boolean Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `and` | a, b | boolean | Logical AND |
| `or` | a, b | boolean | Logical OR |
| `not` | a | boolean | Logical NOT |

### Bitwise Primitives

| Primitive | Description |
|-----------|-------------|
| `bit-and` | Bitwise AND |
| `bit-or` | Bitwise OR |
| `bit-not` | Bitwise NOT |
| `bit-xor` | Bitwise XOR |
| `bit-shift-left` | Left bit shift |
| `bit-shift-right` | Right bit shift |
| `bit-test` | Test specific bit |

### String Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `concat` | str1, str2, ... | concatenated | Concatenate strings |
| `prefix` | string, count | prefix | First N characters |
| `suffix` | string, count | suffix | Last N characters |
| `middle` | string, start, count | substring | Extract substring |
| `to-string` | value | string | Convert any type to string representation (polymorphic) |
| `from-string` | string | value | Parse string to number/other type |
| `format` | value, format-spec | formatted string | Printf-like formatting |
| `to-codepoints` | string | list of ints | String to list of Unicode code points |
| `from-codepoints` | list of ints | string | List of Unicode code points to string |
| `length` | string | integer | String length |
| `string-search` | haystack, needle | position | Find substring (fails if not found) |
| `string-contains?` | haystack, needle | boolean | Test if string contains substring |
| `string-replace` | string, target, replacement | string | Replace all occurrences of target |
| `string-replace-first` | string, target, replacement | string | Replace first occurrence |
| `string-split` | string, separator | list of strings | Split string by separator |
| `string-trim` | string | string | Remove leading/trailing whitespace |
| `string-starts-with?` | string, prefix | boolean | Test if string starts with prefix |
| `string-ends-with?` | string, suffix | boolean | Test if string ends with suffix |
| `uppercase` | string | string | Convert to uppercase |
| `lowercase` | string | string | Convert to lowercase |
| `string-repeat` | string, count | string | Repeat string N times |
| `char-at` | string, index | string | Character at position (1-indexed, returns single-char string) |

### List Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `get-nth` | list, index | element | Get element at position (1-indexed) |
| `get-nth` (3 inputs) | list, row, col | element | Access nested list element (2D) |
| `set-nth` | list, index, value | modified list | Set element (returns new list) |
| `set-nth!` | list, index, value | -- | Set element in place (mutating) |
| `detach-l` | list | first, rest | Remove and return leftmost element |
| `detach-r` | list | rest, last | Remove and return rightmost element |
| `sort` | list | sorted list | Sort in ascending order |
| `sort-by` | list, method-ref | sorted list | Sort using a comparison method (see §5, Method References) |
| `make-list` | count, initial-value | list | Create list of N elements initialized to value |
| `in` | list, element | index | Find element index (1-indexed). **Fails** if not found (use with try or control annotation). |
| `contains?` | list, element | boolean | Test if list contains element (non-failing alternative to `in`) |
| `split-nth` | list, position | left, right | Partition list at position |
| `copy` | value | deep copy | Deep copy of any value including objects |
| `length` | list | integer | Number of elements |
| `first` | list | element | First element |
| `rest` | list | list | All elements except the first |
| `append` | list1, list2 | combined | Concatenate two lists |
| `reverse` | list | reversed | Reverse list order |
| `empty?` | list | boolean | Test if list is empty |
| `filter` | list, method-ref | list | Keep elements where method-ref returns true |
| `reduce` | list, initial-value, method-ref | value | Fold list: method-ref(accumulator, element) → accumulator |
| `map` | list, method-ref | list | Apply method-ref to each element, collect results |
| `flat-map` | list, method-ref | list | Map, then flatten one level |
| `any?` | list, method-ref | boolean | True if method-ref returns true for any element |
| `all?` | list, method-ref | boolean | True if method-ref returns true for all elements |
| `find` | list, method-ref | element | First element where method-ref returns true (fails if not found) |
| `zip` | list1, list2 | list of pairs | Combine two lists element-wise into (a, b) pairs |
| `enumerate` | list | list of pairs | Each element paired with its 1-based index: ((1, a) (2, b) ...) |
| `unique` | list | list | Remove duplicate elements, preserving order |
| `group-by` | list, method-ref | dict | Group elements by method-ref result; returns dict of key → list |

**Note on `map` vs spreads:** Spreads (§8) auto-broadcast scalar operations over lists, replacing `map` for primitives. Use explicit `map` when the transformation is a user-defined method that takes extra arguments or has complex logic. Spreads handle the common case; `map`/`filter`/`reduce` handle the general case.

### Type-Checking and Introspection Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `integer?` | value | boolean | Test if value is an integer |
| `real?` | value | boolean | Test if value is a real |
| `string?` | value | boolean | Test if value is a string |
| `list?` | value | boolean | Test if value is a list |
| `dict?` | value | boolean | Test if value is a dict |
| `boolean?` | value | boolean | Test if value is a boolean |
| `null?` | value | boolean | Test if value is null |
| `object?` | value | boolean | Test if value is a class instance |
| `error?` | value | boolean | Test if value is an error |
| `data?` | value | boolean | Test if value is binary data |
| `date?` | value | boolean | Test if value is a date |
| `external?` | value | boolean | Test if value is an external reference |
| `type-of` | value | string | Returns type name: `"integer"`, `"real"`, `"string"`, `"list"`, `"dict"`, `"boolean"`, `"data"`, `"date"`, `"enum"`, `"null"`, `"error"`, `"external"`, or the class name for objects |
| `class-of` | object | string | Returns the class name of an object instance |
| `instance-of?` | object, class-name | boolean | Test if object is an instance of the named class (including superclasses) |
| `responds-to?` | object, method-name | boolean | Test if object has a method with the given name |
| `conforms-to?` | object, protocol-name | boolean | Test if object conforms to a protocol (see §9) |
| `enum-create` | type-name, variant-name, data | enum | Create an enum value programmatically |
| `enum-data` | enum-value | value | Extract associated data from an enum value |

### File Primitives

See §12.12 for the full file I/O primitive set (`file-read-text`, `file-write-text`, `file-read-object`, `file-write-object`, `file-pick`, etc.). The original Prograph's `save`/`load` primitives are replaced by these.

### Dict Primitives

See §4 (Dicts) for the full dict primitive set (`dict-get`, `dict-set`, `dict-keys`, etc.).

### Networking Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `http-get` | url | response | Async HTTP GET. Returns a Future (see §7.2). |
| `http-post` | url, body, content-type | response | Async HTTP POST. Returns a Future. |
| `http-request` | method, url, headers-dict, body | response | General async HTTP request. Returns a Future. |
| `response-status` | response | integer | HTTP status code |
| `response-body` | response | string | Response body as string |
| `response-body-data` | response | data | Response body as raw bytes |
| `response-headers` | response | dict | Response headers as dict |
| `json-parse` | string | value | Parse JSON string into Phograph values (dicts, lists, strings, numbers, booleans, null) |
| `json-encode` | value | string | Encode Phograph value as JSON string |
| `url-encode` | string | string | URL-encode a string |
| `url-decode` | string | string | URL-decode a string |

### Timer Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `timer-after` | seconds, method-ref | timer | Call method reference after delay. Returns a timer handle. |
| `timer-every` | seconds, method-ref | timer | Call method reference repeatedly at interval. |
| `timer-cancel` | timer | -- | Cancel a pending timer. |

---

## 7. Control Flow

Control flow is "the most unusual aspect of the Prograph language." It combines **annotations on operations** with the **case structure** to implement all standard control patterns.

### Success and Failure

Every operation execution has one of three outcomes:

1. **Success** -- the operation completed normally and produced output
2. **Failure** -- a controlled outcome that triggers a control annotation (not an error)
3. **Error** -- an unrecoverable problem that halts execution

Failure is a first-class concept in Prograph's control flow, distinct from errors. It is the mechanism by which conditional branching occurs.

### The Ten Control Annotations

Each operation can carry a control annotation that specifies what happens on success or failure. There are **ten control conditions**, grouped into five pairs:

| Annotation | On Success | On Failure |
|------------|-----------|------------|
| **Continue** | Continue to next operation (DEFAULT, no icon) | Continue despite failure (ignore it) |
| **Next Case** | Jump to next case of this method | Jump to next case of this method |
| **Fail** | Propagate failure to caller | Propagate failure to caller |
| **Terminate** | Exit current loop immediately | Exit current loop immediately |
| **Finish** | Complete current iteration, then exit loop | Complete current iteration, then exit loop |

**Default behavior:** An operation with no annotation has "continue on success." If it fails without a failure annotation, execution halts with an error.

### Visual Representation of Controls

| Control | Icon Symbol | Description |
|---------|-------------|-------------|
| Next Case on Failure | **X** | Default match annotation |
| Next Case on Success | **checkmark** | Inverted match |
| Continue | Two bars (top and bottom) | Execution continues regardless |
| Terminate | One bar at top only | Immediately exit loop |
| Finish | One bar at bottom only | Finish current iteration, then exit |
| Fail | Propagates to caller's control | Sets failure flag for caller |

### If-Then-Else Pattern

Conditional branching uses **multiple cases** with guard tests:

```
Method "example" has 3 cases:

Case 1:3  -- if condition A
  [test A] --X-- (next case on failure)
  [code for condition A]

Case 2:3  -- else if condition B
  [test B] --X-- (next case on failure)
  [code for condition B]

Case 3:3  -- else (default)
  [default code]
```

When a test fails and has "next case on failure" (X), execution abandons the current case entirely and begins the next case from its input bar.

### Match Operations

A **match** combines a comparison primitive with a control annotation. Comparison primitives (`=`, `<`, `>`, `<=`, `>=`) automatically carry a control. The control determines what happens when the comparison result indicates the condition being tested.

### The Fail Mechanism

The **Fail** control is used inside local methods or subordinate methods. When activated, it sets a failure flag that propagates upward to the calling method. The calling method's operation icon can then have its own control annotation to respond to the failure. This provides a limited form of exception handling.

### 7.1 Error Handling (Phograph Extension)

The original Prograph Fail mechanism propagates a boolean flag with no information about *what* went wrong. Phograph extends this with **Error values** that carry a message and optional details.

#### Error Data Type

An **Error** is a value (not an exception) with:

| Field | Type | Description |
|-------|------|-------------|
| `message` | String | Human-readable error description |
| `code` | String | Machine-readable error code (e.g., `"file-not-found"`, `"network-timeout"`) |
| `details` | Dict or Null | Optional additional context |

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `error-create` | message | error | Create an error value |
| `error-create-code` | message, code | error | Create an error with a code |
| `error-message` | error | string | Get error message |
| `error-code` | error | string | Get error code |
| `error-details` | error | dict or null | Get error details |
| `error?` | value | boolean | Test if a value is an error |

#### Try Annotation

The **try annotation** (`has_try=true` on a node) catches failures from an operation and converts them to error values flowing out of a dedicated error output pin (`error_out_pin` specifies which output pin receives the error).

When an operation has a try annotation:
- On **success**: data flows through the normal output nodes. The error output produces `null`.
- On **failure**: normal output nodes produce `null`. The error output (identified by `error_out_pin`) produces an `Error` value describing what went wrong.

This replaces the pattern of using `continue-on-failure` + downstream testing with a direct, visible error flow path. The error wire is visually distinct (dashed red) so error paths are immediately obvious in the dataflow graph.

```
[read-file] ──try──┬── file-contents (string or null)
                    └── error (Error or null)
```

Downstream operations can test the error output with a match:

```
Case 1:2
  [read-file] ──try── contents, err
  [error?] err ──X── (next case on success: there IS an error)
  [...process contents normally...]

Case 2:2
  [...handle the error...]
```

#### Error Cluster Wiring (Phograph Extension)

Inspired by LabVIEW's error cluster pattern, fallible operations can optionally accept an **error-in** pin and produce an **error-out** pin. This enables chaining error handling through a sequence of operations:

- If an operation receives an error on its error-in pin, it **skips execution** and passes the error through to error-out unchanged.
- If an operation receives null on error-in (no error), it executes normally. If it fails, it produces an error on error-out. If it succeeds, it passes null on error-out.

This creates a clean error propagation chain without explicit error checking at each step:

```
[open-file] ──err──▶ [read-contents] ──err──▶ [parse-json] ──err──▶ [validate]
                                                                         │
                                                                    error (if any)
```

If `open-file` fails, `read-contents`, `parse-json`, and `validate` are all skipped, and the original error flows through to the end. The programmer only needs to check for errors once, at the end of the chain.

Error wires are **visually distinct** (dashed red lines with star-shaped error pins). The error path can be **collapsed/hidden** for clean graphs -- the error wiring still exists but is not rendered unless the programmer enables error path visibility.

This maps directly to Swift's `throws` / `try` chains and `Result<T, Error>`.

#### Let-It-Crash Semantics (Phograph Extension)

Inspired by Erlang's supervision model, unexpected errors in Phograph are **isolated per-node**:

- When a node crashes (runtime error, not a controlled Fail), the crash is contained to that node.
- The node shows a **red error state** with the error message.
- Downstream nodes receive an error value on their inputs (not a process crash).
- The rest of the graph continues running.
- The errored node can be **restarted** (re-evaluated with its last good inputs) from the debugger.

The graph never stops running due to a single node failure. This is critical for live/interactive use.

#### Fail With Error

The original Fail control propagates a bare boolean. Phograph extends it: when a method fails, it can attach an Error value to the failure. The try annotation on the calling operation captures this Error.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `fail-with` | error | -- | Fail the current method with an error value attached. The error propagates to the caller's try annotation. |

If no try annotation exists on the caller, the failure propagates upward as in original Prograph. If it reaches the top level with no handler, the runtime halts with an error report showing the full call stack.

### 7.2 Concurrency and Async (Phograph Extension)

The original Prograph mentioned "thread primitives" without specification. Phograph defines a concurrency model based on **Futures** and **async operations** that fits naturally into the dataflow paradigm.

#### The Dataflow Advantage

Prograph's dataflow model is already inherently concurrent: operations with no data dependencies between them *can* execute in parallel. Phograph makes this explicit.

#### Futures

A **Future** represents a value that will be available later. When an async operation (e.g., `http-get`, `file-read-text-async`) is called, it returns a Future immediately. Data continues to flow through other independent parts of the graph. When the Future's value is needed by a downstream operation, that operation **waits** until the Future resolves.

This means async behavior is implicit in the wiring: connect a Future to a downstream operation, and the downstream operation automatically waits. No special `await` keyword or annotation needed -- it falls out of the data-driven firing rule ("execute when all inputs are available").

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `future-value` | future | value | Block until the future resolves and return its value. (Usually implicit -- just connect the wire.) |
| `future-resolved?` | future | boolean | Non-blocking check: has the future resolved yet? |
| `future-then` | future, method-ref | future | When future resolves, call method ref with result. Returns a new future for the method's output. |
| `future-error` | future | error or null | If the future resolved with failure, return the error. |
| `future-all` | list of futures | future of list | A future that resolves when all input futures resolve. |
| `future-any` | list of futures | future | A future that resolves when the first input future resolves. |
| `future-create` | -- | future, resolver | Create a future and a resolver. Call `resolver-resolve` or `resolver-fail` on the resolver to complete the future. |
| `resolver-resolve` | resolver, value | -- | Resolve the associated future with a value. |
| `resolver-fail` | resolver, error | -- | Fail the associated future with an error. |

#### Async Variants of Blocking Primitives

For operations that might take significant time, async variants return Futures:

| Primitive | Returns | Description |
|-----------|---------|-------------|
| `http-get` | Future of response | Non-blocking HTTP GET |
| `http-post` | Future of response | Non-blocking HTTP POST |
| `file-read-text-async` | Future of string | Non-blocking file read |
| `file-write-text-async` | Future of null | Non-blocking file write |

#### Dispatch

For CPU-bound work that should run off the main thread:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `dispatch` | method-ref, args-list | future | Execute method reference on a background thread. Returns a future for the result. |
| `dispatch-main` | method-ref, args-list | future | Execute method reference on the main thread (required for UI updates). |

#### Channels (Inter-method Communication)

For producer-consumer patterns:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `channel-create` | -- | channel | Create a new channel |
| `channel-send` | channel, value | -- | Send a value into the channel (blocks if buffer full) |
| `channel-receive` | channel | value | Receive a value from the channel (blocks if empty) |
| `channel-try-receive` | channel | value or null | Non-blocking receive; returns null if empty |
| `channel-close` | channel | -- | Close the channel |

### 7.3 Managed Effects (Phograph Extension)

Inspired by the Elm Architecture, side effects in Phograph can be expressed as **commands** (data describing an effect) rather than performed directly. This keeps the dataflow graph pure and testable.

A **Command** is a value that describes a side effect without performing it:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `cmd-http-get` | url | command | Describe an HTTP GET (does not execute it) |
| `cmd-http-post` | url, body, content-type | command | Describe an HTTP POST |
| `cmd-write-file` | path, contents | command | Describe a file write |
| `cmd-delay` | seconds, message | command | Describe a delayed message |
| `cmd-batch` | list of commands | command | Combine multiple commands |
| `cmd-none` | -- | command | No-op command |
| `perform` | command | future | Execute a command, returning a Future for its result |

**Why managed effects?** When effects are data, the graph is a pure function from inputs to (outputs + commands). This enables:
- **Testing**: pass test inputs, check that the correct commands are produced, without actually performing I/O
- **Replay**: record commands from a session and replay them for debugging
- **Batching**: combine multiple effects and execute them efficiently

Managed effects are **optional**. The programmer can call `http-get` directly (imperative style) or produce a `cmd-http-get` command (managed style). The managed style is recommended for application-level logic; imperative style is fine for scripts and prototypes.

### 7.4 Summary of Control Flow Extensions

| Original Prograph | Phograph |
|-------------------|----------|
| Fail propagates bare boolean | Fail can carry an Error value with message and code |
| No error information at call site | Try annotation captures Error on dedicated output wire |
| Error checking at every step | Error cluster wiring chains error propagation automatically |
| One crash kills the program | Let-it-crash: node crashes are isolated, rest of graph continues |
| Unhandled failure = silent halt | Unhandled failure = error report with full call stack |
| "Thread primitives" unspecified | Futures, dispatch, channels |
| No async operations | Async primitives return Futures; dataflow firing rule handles waiting naturally |
| Side effects performed directly | Managed effects: describe effects as commands, execute via runtime |

---

## 8. Loops and List Annotations

### Loop Construct (Counted Loop)

Equivalent to a `for` loop. A MethodCall node with `is_loop=true` calls its target method iteratively. The icon displays stacked arrows showing the feedback path.

Behavior:
1. The output of each iteration feeds back as the input for the next iteration
2. A match test inside the loop method checks the termination condition; alternatively, the method can **fail** to terminate the loop
3. A **Terminate** or **Finish** control on the match ends the loop
4. When the called method fails, the loop terminates and returns the **last successful input values** (not the failed outputs)
5. The final value exits the loop to downstream operations

### Repeat Construct (Indefinite Loop)

Equivalent to `while` or `do-while`. Created by highlighting a method icon and selecting Repeat.

Behavior:
1. The method executes repeatedly with no counter
2. No feedback value required (though data can be threaded through)
3. Continues until a **Terminate** or **Finish** control fires inside the repeated method

### List Annotation (Ellipsis / ListMap)

Any input or output node can be marked with an **ellipsis annotation**, or equivalently a node can set `list_map=true`:

- **ListMap node**: a node with `list_map=true` iterates over the elements of its list input, executing once per element. Results are gathered into a list.
- **Scalar inputs broadcast**: if a ListMap node has both a list input and scalar inputs, the scalar values are broadcast (reused) for each iteration.
- **Ellipsis on input**: equivalent visual representation -- the operation expects a list; it automatically executes once for each element (map/for-each).
- **Ellipsis on output** (with ellipsis input): Results are gathered into a list (map with collect).

This is one of Prograph's most powerful features. A single annotated `+` operation with a list input will add a value to every element, producing a new list -- no explicit loop needed.

### Partition Annotation

A node with `partition=true` splits a list based on a boolean output. The operation processes every element of an input list and produces **two output lists**: one where the test returned true (pass) and one where it returned false (fail). Equivalent to a `partition` or `filter` in functional languages.

### Inject/Accumulate Annotation (Fold/Reduce)

An annotation that "winds an output back to the top" to become an input on the next iteration. Implements fold/inject/reduce behavior: an accumulator value is threaded through each iteration.

### Loop Modifier Annotation

Attached to an operation to make it iterate. The loop continues until a control annotation (terminate/finish) fires. When loop, partition, or list annotations are added, the icon takes on a **"stack of icons" appearance** to indicate multiple executions.

### Shift Registers (Phograph Extension)

Inspired by LabVIEW, a **shift register** is named state carried across loop iterations. Each shift register has an `input_pin`, an `output_pin`, and an `initial` value. Shift registers are stored on the Case that contains the loop.

- A shift register appears as a pair of connected terminals on opposite sides of a loop boundary: one on the **right edge** (end of iteration) and one on the **left edge** (start of next iteration).
- The programmer sets an **initial value** on the left terminal (the `initial` field).
- At the end of each iteration, the value on the right terminal (`output_pin`) feeds back to the left terminal (`input_pin`) for the next iteration.
- When the loop completes, the final value on the right terminal exits the loop.

```
     ┌──────────────────────────────┐
 0 ──┤◀ accumulator    accumulator ▶├── final sum
     │                              │
     │   [+] input, accumulator     │
     │        → accumulator         │
     └──────────────────────────────┘
```

This is the visual equivalent of `fold`/`reduce`: the wire going from end to start of the loop IS the accumulator. Multiple shift registers on a single loop enable carrying multiple accumulators.

### Spreads: Automatic Broadcasting (Phograph Extension)

Inspired by vvvv's **spreads** and Grasshopper's data trees, Phograph wires implicitly support **automatic broadcasting** over collections. This generalizes the original ellipsis annotation:

- Any scalar operation can receive a **List** on an input that expects a scalar. The operation automatically executes once per element, producing a List of results.
- If multiple inputs receive Lists, the operation maps over them in parallel (zip semantics for equal-length lists; shorter lists cycle for unequal lengths, vvvv-style).
- Broadcasting is **recursive**: a List of Lists produces a List of Lists.

This means **most explicit loops are unnecessary**. Instead of:

```
[list of prices] ──ellipsis──▶ [* 1.1] ──ellipsis──▶ [list of adjusted prices]
```

The programmer simply wires a List into a scalar operation, and the result is a List:

```
[list of prices] ──▶ [* 1.1] ──▶ [list of adjusted prices]
```

The wire's visual appearance changes to indicate spread data flow: a **double line** for Lists, a **triple line** for Lists of Lists.

**Relationship to ellipsis annotation:** The original ellipsis annotation is retained as an **explicit** opt-in. Spreads are **implicit** -- the system auto-broadcasts when a List arrives at a scalar pin. The programmer can disable auto-broadcasting on a per-pin basis if they want the List treated as a single value (e.g., for `length`, `sort`, `first`).

Operations that are inherently list-level (like `length`, `sort`, `reverse`, `append`) do **not** auto-broadcast -- they operate on the list as a whole. The type system distinguishes between "this pin expects a scalar (auto-broadcasts)" and "this pin expects a List (no broadcasting)."

### Nested Loops

Loops and repeats can be nested: a repeat containing a loop, a loop containing another loop, etc. Each nested construct is visually distinct in its own method/case window.

---

## 9. Classes and Object-Orientation

### Class Definition

Classes are created in the Classes window of a section. Each class icon is a **hexagon** split in two:

- **Left half**: Attributes (data) -- click to open the Class Attribute Window
- **Right half**: Methods (behavior) -- click to open the Methods Window

### Attributes

The Class Attribute Window has two regions separated by a horizontal line:

| Region | Type | Symbol | Description |
|--------|------|--------|-------------|
| Above the line | **Class attributes** | Hexagon | Shared by all instances (like `static` in C++) |
| Below the line | **Instance attributes** | Triangle | Unique per instance |

Inherited attributes show an arrow in their symbol. Default values are set in the Attributes window and automatically applied when instances are created.

### Access Control (Phograph Extension)

In original Prograph CPX, all attributes were effectively public. Phograph adds visibility modifiers:

| Visibility | Symbol | Description |
|------------|--------|-------------|
| **public** | Open circle | Accessible from anywhere. Default for methods. |
| **private** | Filled circle | Accessible only from within the same class. Default for attributes. |
| **protected** | Half-filled circle | Accessible from the same class and its subclasses. |
| **read-only** | Circle with line | Publicly readable (get), privately writable (set). |

Visibility is set per-attribute and per-method via the `Access` enum (`Public`, `Protected`, `Private`). Violating access control produces a **failure** (not a compile error, since Phograph is dynamically typed), which can be caught with a try annotation or control annotation.

**Design rationale:** Attributes default to **private** (the safe default). Methods default to **public** (the useful default). This matches the principle of least surprise: you must explicitly expose data, but all behavior is visible by default.

### Instance Generation

An object is created with an **instance generator** icon (octagon). It has:
- One root node (outputs the new object)
- One optional terminal node (receives a list of attribute initialization values)

### Initialization Methods (Constructors)

Named **`init`**. Automatically called when an object is instantiated. Equivalent to constructors in C++/Java.

**Design change from original Prograph:** The original used `<<>>` as the constructor name, which was cryptic and hard to remember. Phograph uses `init`, consistent with Swift and Objective-C conventions.

The `/finalize` method (see §4, Memory Management) serves as the destructor, called automatically when the object's reference count reaches zero.

### Get and Set Operators

| Operator | Created By | Behavior |
|----------|-----------|----------|
| **Get** | `Opers Menu > Get` | Retrieves attribute value; left output passes self, right passes value |
| **Set** | `Opers Menu > Set` | Assigns a new value to an attribute |

**Custom vs. built-in access:**

- `/attributeName` -- calls a **custom Get/Set method** you wrote (if one exists)
- `attributeName` (no slash) -- calls the **built-in Get/Set operation** directly

This allows writing custom Set methods that perform validation before setting the attribute value.

### Three Method Call Syntaxes

| Syntax | Name | Description |
|--------|------|-------------|
| `/MethodName` | **Data-determined** | First input must be an instance; method dispatched based on instance's class. Enables polymorphism. |
| `//MethodName` | **Context-determined** | Called from within another method of the same class. No instance input needed. |
| `ClassName/MethodName` | **Explicit** | Explicitly names the target class. No instance input needed. Used for class-level operations. |

### Single Inheritance

Prograph implements **class-based single inheritance only** (no multiple inheritance). Subclasses are created by:
1. Highlighting the parent class icon
2. Option-clicking to create a new class below it
3. Naming the subclass

A visual link between class icons shows the inheritance relationship. All parent methods and attributes are automatically inherited.

### Method Overriding

Create a method in the subclass with the same name as a parent method. The subclass version is called for instances of that subclass.

**Super calls:** To call the parent's version from within an override, highlight the method call icon and select `Opers Menu > Super`.

### Abstract Superclasses

Classes intended never to be instantiated directly. Made abstract by omitting the `init` method. Provide shared code to subclasses.

### Polymorphism

When the same method name exists in multiple classes in an inheritance chain:
- Prograph determines at runtime which class the object belongs to
- The appropriate version executes
- The caller doesn't need to know the specific subclass

Especially powerful with **list annotations**: send the same method to every object in a mixed-type list, and each object executes its own version.

### Front Panel / Block Diagram Duality (Phograph Extension)

Inspired by LabVIEW and the natural marriage with SwiftUI, a Phograph class can have **two views**:

- **Block Diagram**: the dataflow graph (methods, operations, wires). This is the implementation.
- **Front Panel**: a SwiftUI view that serves as the class's visual interface. UI controls on the front panel (sliders, text fields, buttons, toggles) automatically appear as **input terminals** on the block diagram. Display elements (labels, charts, images) automatically appear as **output terminals**.

This means building a UI is declarative: place controls on the front panel, and they become wirable data sources/sinks in the dataflow graph.

**How it works:**
1. The programmer designs the front panel by placing SwiftUI-like controls from a palette.
2. Each control is **bound to a class attribute**. The control type is inferred from the attribute's default value:
   - Boolean default -> Toggle (switch)
   - String default -> TextField
   - Numeric default (Integer or Real) -> number field / Slider
   - Other types -> Label (read-only display)
3. The programmer wires these attribute terminals to operations in the block diagram.
4. At runtime, user interaction with the front panel pushes values into the graph via attribute changes; graph outputs update the display elements through observable attribute bindings.

**SwiftUI mapping:** The front panel compiles to a SwiftUI `View` struct. Each control maps to a SwiftUI view with appropriate property wrappers: Slider -> `@State var sliderValue: Double`, TextField -> `@Binding var text: String`, Toggle -> `@State var isOn: Bool`, etc. (See §14 for details.)

Not every class needs a front panel. Classes without one are pure logic / data classes. Classes with a front panel are **view classes** -- they have UI.

**Relationship to Canvas system (§11):** The Front Panel and Canvas serve different purposes:

- **Front Panel** (this section) is for **application-level UI** -- forms, dashboards, settings screens, data-driven interfaces. It compiles to SwiftUI views and gets native platform appearance (dark mode, Dynamic Type, accessibility) for free. Use the front panel when you want standard app UI.
- **Canvas** (§11) is for **custom rendering** -- games, visualizations, charts, drawing apps, image editors, or any UI that needs pixel-level control. It renders into a GPU-backed pixel buffer with full control over every pixel.

A single class can have both: a front panel for controls and a canvas for a custom rendering area (the canvas would be embedded as a view in the front panel). The front panel's controls generate terminals on the block diagram; the canvas's shapes are manipulated through the block diagram's operations.

### Actor-Based Concurrency for Objects (Phograph Extension)

Inspired by Swift Concurrency and Erlang, a class can be marked as an **actor** by setting `ClassDef.is_actor = true`. When codegen runs, actor classes emit the Swift `actor` keyword instead of `class`:

- Instance attributes are **actor-isolated**: only the instance's own methods can access them directly.
- Method calls from outside the instance are automatically `await`ed (asynchronous cross-actor access). Semantically, method calls are serialized per instance.
- This eliminates shared mutable state bugs: two operations can never simultaneously mutate the same object.
- In the visual graph, wires that cross an actor boundary are rendered as **dashed lines** to indicate async access. The compiler verifies that values crossing boundaries are `Sendable`.

**Opt-out:** Classes without `is_actor` (the default) compile to a plain Swift `class`. Set `is_actor = true` only for classes that need concurrency safety.

### Composition

One class contains an instance of another class as an attribute ("has-a" relationship). The containing object's methods delegate to the contained object's methods.

### Class Aliases

A class alias marks that you want to reference or subclass a class from another section without moving it. Created via `Convert To Alias...`. Enables subclasses to live in their own sections.

### Protocols (Phograph Extension)

The original Prograph had single inheritance only. If a `Shape` and a `DatabaseRecord` both need to be `Serializable`, the only option was to put `Serializable` high in the inheritance chain -- but `Shape` and `DatabaseRecord` have no common ancestor. This forces artificial hierarchies.

Phograph adds **protocols** -- named sets of method signatures that any class can declare conformance to, regardless of its inheritance chain. Protocols are similar to interfaces (Java), protocols (Swift), or traits (Rust).

#### Defining a Protocol

A protocol (`ProtocolDef`) is created in the Classes window of a section, like a class, but with a distinct icon (diamond shape). A protocol defines:

- **`name`**: the protocol name
- **`required_methods`**: list of method names that conforming classes must implement
- **No attributes**: protocols cannot define instance or class attributes
- **No implementation**: protocol methods have no code -- they are signatures only

Protocols are stored on the `Section` alongside classes.

#### Declaring Conformance

A class declares conformance by listing protocol names in its `conforms_to` array. The editor verifies that the class implements all required methods. In the codegen, conformance compiles to Swift protocol conformance in the class declaration.

A class can conform to **multiple protocols** while inheriting from only one superclass:

```
    Shape (superclass)
      │
      ▼ inherits
  MyWidget ···▷ Serializable (protocol)
           ···▷ Animatable (protocol)
```

#### Using Protocols

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `conforms-to?` | object, protocol-name | boolean | Test if object conforms to a protocol |

Protocol names can be used as type annotations on method inputs: `widget: Serializable`. At runtime, the system verifies the object conforms to the protocol.

Data-determined dispatch (`/MethodName`) works across protocol boundaries: if multiple unrelated classes conform to the same protocol and implement the same method, sending that method to any conforming object dispatches correctly.

**Example built-in protocols:**

| Protocol | Required Methods | Description |
|----------|-----------------|-------------|
| `Equatable` | `/equals` | Can be compared for equality. The `=` primitive calls this method on objects. |
| `Comparable` | `/compare` | Can be ordered (returns -1, 0, 1). The `<`, `>`, `sort` primitives call this. |
| `Serializable` | `/serialize`, `/deserialize` | Can be saved to / loaded from data. `file-write-object` calls `/serialize`. |
| `Printable` | `/to-string` | Has a human-readable string representation. The `to-string` primitive and `log` call this method on objects that conform. |

### Enum Types (Phograph Extension)

Phograph adds **enum types** -- algebraic data types (tagged unions) where each variant can carry associated data. This fills a critical gap: without enums, pattern matching is limited to duck-typing on primitive types.

#### Defining an Enum

An enum is created in the Classes window, like a class, but with a distinct icon (rounded rectangle with vertical dividers). Each variant is listed with its name and optional associated data types.

```
Enum "Result" has 2 variants:
  success(value)       -- carries one value of any type
  failure(error)       -- carries an Error

Enum "Direction" has 4 variants:
  north                -- no associated data
  south
  east
  west

Enum "LoadState" has 4 variants:
  idle                 -- no data
  loading(progress: Real)   -- carries progress 0.0-1.0
  loaded(data: Data)        -- carries the loaded data
  failed(error: Error)      -- carries error info
```

#### Creating Enum Values

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `EnumName.variant` | associated values | enum value | Create an enum value. E.g., `Result.success` with input `42` produces a `Result` value. |

In the visual graph, creating an enum value uses an icon labeled with the enum and variant name (e.g., `Result.success`). The input pins match the variant's associated data types.

#### Pattern Matching on Enums

Enum variants are the primary use case for case-based pattern matching:

```
Method "handle-result" has 2 cases:

Case 1:2  Input: result: Result.success(value)
  [...use value...]

Case 2:2  Input: result: Result.failure(error)
  [...handle error...]
```

Each case specifies the variant to match. Associated data is destructured into named bindings. This is exhaustive -- the editor warns if not all variants are covered.

#### Enum Introspection

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `enum-variant` | enum-value | string | Get the variant name (e.g., `"success"`) |
| `enum-type` | enum-value | string | Get the enum type name (e.g., `"Result"`) |
| `enum?` | value | boolean | Test if a value is an enum |

**Swift mapping:** Phograph enums compile to Swift `enum` types with associated values. `Result.success(42)` becomes `Result.success(42)` in Swift. This enables seamless interop with Swift APIs that return enums (like `Swift.Result`).

### Pattern Matching on Cases (Phograph Extension)

The original case system is essentially a chain of if/else guards. Phograph extends it with **pattern matching** -- cases can match on the type, structure, and value of their inputs without explicit test operations.

#### Type Patterns

A case's input bar can specify a **type constraint** on each input. The case only executes if all inputs match their type constraints. If they don't, execution falls through to the next case automatically.

```
Method "process" has 3 cases:

Case 1:3  -- input is an Integer
  Input: value: Integer
  [...handle integer...]

Case 2:3  -- input is a String
  Input: value: String
  [...handle string...]

Case 3:3  -- input is anything else
  Input: value
  [...default handling...]
```

This replaces the pattern of writing `integer?` / `string?` tests with next-case-on-failure annotations.

#### Value Patterns

A case's input bar can specify a **literal value**. The case only executes if the input matches:

```
Method "describe-color" has 4 cases:

Case 1:4  Input: color = "red"     → "warm"
Case 2:4  Input: color = "blue"    → "cool"
Case 3:4  Input: color = "green"   → "natural"
Case 4:4  Input: color             → "unknown"
```

#### List Destructuring

A case can destructure a list input into head and tail:

```
Method "sum-list" has 2 cases:

Case 1:2  Input: ()           → 0          (empty list: base case)
Case 2:2  Input: (head | tail) → head + [sum-list] tail   (cons pattern)
```

The `(head | tail)` pattern binds the first element to `head` and the remaining list to `tail`. This makes recursive list processing natural.

#### Dict Destructuring

A case can destructure a dict by specifying required keys:

```
Case 1:1  Input: {name, age, ...rest}
  -- name, age are bound to their values
  -- rest is a dict of remaining keys
```

#### Pattern Matching Guards

Each case can have **guards** (`CaseGuard`) attached to its input pins. Guards are checked before evaluating the case body. Three guard kinds are supported:

- **TypeMatch**: the input value must be of a specific type (e.g., `Integer`, `String`, or a class name). `guard.kind = TypeMatch`, `guard.match_type = "Integer"`.
- **ValueMatch**: the input value must equal a specific literal. `guard.kind = ValueMatch`, `guard.match_val = <value>`.
- **Wildcard**: matches any value (default). Used for the "else" case.

Guards enable type-based and value-based multi-case dispatch without explicit test operations in the case body. Multiple guards on a single case form a conjunction -- all must match for the case to execute.

#### Pattern Matching Order

Cases are tried in order (1, 2, 3, ...). The first case whose patterns all match executes. If no case matches, the method **fails** (which can be caught by a try annotation or control annotation on the caller).

**Design rationale:** Pattern matching makes the case system dramatically more useful. The original's "most confusing construct" becomes intuitive when cases declare what they expect rather than imperatively testing for it.

### Observation and Reactive Bindings (Phograph Extension)

For the canvas-based UI system (§11), Phograph needs a way to automatically update the display when data changes. The original ABCs had no observation mechanism -- all updates were manual.

#### Observable Attributes

Any class attribute can be marked as **observable** (distinct visibility symbol: concentric circles). When an observable attribute's value changes via `set_attr`, the system automatically fires all registered observer callbacks with the attribute name, old value, and new value. Objects maintain per-attribute and any-attribute observer lists internally.

#### Observer Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `observe` | object, attribute-name | observer-id | Register an observer on a specific attribute. Returns an integer observer handle. |
| `unobserve` | object, observer-id | null | Remove an observation by its handle |
| `observe-any` | object | observer-id | Observe all attribute changes on an object |

#### Reactive Bindings for UI

A **binding** connects a data source to a target object, so attribute changes propagate automatically:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `bind` | source-object, source-attr, target-object, target-attr | observer-id | One-way binding: when source attribute changes, target attribute updates automatically. Returns an observer handle. |
| `unbind` | object, observer-id | null | Remove a binding by its handle |

Example: bind a data model's `name` attribute to a `Label`'s `text`:

```
[bind] my-model, "name", my-label, "text" → binding
-- Now whenever my-model's name changes, my-label automatically updates
```

This is the primary mechanism for keeping UI in sync with data, replacing the ABC pattern of manually calling Set Value on window items after every data change.

---

## 10. Persistents and Global State

### Persistent Variables

Persistents are Phograph's global variables with automatic persistence, represented by `NodeType::Persistent` in the graph:

- **Icon shape:** Oval
- **Created via:** `Opers Menu > Persistent` on a blank operation, then named
- **Write mode:** when an input is connected to the node, the value is stored in the persistent store
- **Read mode:** when no input is connected, the node reads the stored value (or returns the default if not yet written)
- **Value dialog** shows current type and value; type selected from popup (null, integer, real, string, list, dict, boolean, etc.)

### Persistence Behavior (Phograph Redesign)

The original Prograph had a broken persistence model: the interpreter embedded persistent values directly into the program file (mixing code and data), and compiled programs had no automatic persistence at all. Phograph fixes this.

**Phograph persistence** uses a separate **persistent store** -- a file in the application's data directory, distinct from the program source. Persistence works identically in the interpreter and in compiled applications.

| Aspect | Original Prograph | Phograph |
|--------|-------------------|----------|
| Storage location | Embedded in program file | Separate store file in app data directory |
| Interpreter | Auto-save on exit, auto-load on start | Same |
| Compiled | No auto-persistence; manual save/load | Auto-persistence, same as interpreter |
| Data/code separation | Violated (data in source file) | Clean separation |

#### Persistent Store Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `persistent-save-all` | -- | -- | Explicitly flush all persistent values to disk |
| `persistent-load-all` | -- | -- | Explicitly reload all persistent values from disk |
| `persistent-reset` | name | -- | Reset a named persistent to its default value |
| `persistent-reset-all` | -- | -- | Reset all persistents to defaults |
| `persistent-path` | -- | string | Get the file path of the persistent store |

Auto-save triggers:
- When the application exits normally
- Periodically (configurable interval, default 60 seconds)
- On `persistent-save-all`

Auto-load triggers:
- When the application starts
- On `persistent-load-all`

### Class Attributes

Class attributes (shared across all instances) are **not** automatically persistent. They behave as global variables that reset to their default values on each application launch. To persist a class attribute, use a persistent variable instead.

**Design rationale:** Implicit persistence on class attributes (original Prograph) created confusion about which values survived restarts. In Phograph, persistence is always explicit: if you want a value to survive restarts, use a persistent.

### Persistents and Concurrency

Persistents are global shared state. In the actor-based concurrency model (§9), persistent access is mediated by a dedicated **PersistentStore actor**:

- Reading a persistent is an `await` (async read from the store actor)
- Writing a persistent is an `await` (async write to the store actor)
- This serializes all persistent access, preventing data races
- In the visual graph, persistent ovals have dashed wires (like cross-actor access) to indicate the async nature

For performance, frequently-read persistents can be **cached** per-actor with a configurable staleness tolerance.

### Object Serialization

Any Phograph object can be serialized to disk and deserialized back. This replaces the original's vague "object persistence" with explicit file I/O:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `file-write-object` | path, object | -- | Serialize an object graph to a file |
| `file-read-object` | path | object | Deserialize an object graph from a file |

The serializer handles object graphs including shared references and cycles. It does **not** serialize external resources (file handles, network connections, GPU buffers) -- these serialize as null with a warning in the debug console.

---

## 11. Canvas, Drawing, and Scene Graph

This section replaces the original Prograph ABCs (147 classes wrapping Mac Toolbox widgets) with a simpler, more direct system inspired by GPU-accelerated canvas rendering with an in-language scene graph. No OS-native widgets are used. Everything is drawn by the language runtime into a pixel buffer, composited by the GPU, and displayed on screen.

### 11.1 Design Principles

1. **Single canvas, not OS widgets.** The application renders into a GPU-backed pixel buffer. There are no platform-native buttons, checkboxes, or scroll bars. Every visual element is drawn by Phograph code.
2. **Scene graph, not ownership hierarchy.** UI is a tree of `Shape` objects with parent/child relationships, transforms, styles, and event handlers. Not a 60-class "Owner" chain.
3. **Drawing is dataflow.** Drawing operations are Phograph methods that receive a `DrawContext` and issue commands. Data flows in; pixels flow out.
4. **Events are methods, not injections.** Event handlers are ordinary class methods on Shape subclasses (`/on-pointer-down`, `/on-tap`). No separate Behavior Editor, no inject indirection.
5. **Touch and mouse are unified.** A single `Pointer` event model covers both, with platform-specific gesture recognizers layered on top.
6. **Small class count.** ~25 core classes replace ~147 ABCs. Users build complex UIs by composing simple shapes, not by navigating a deep inheritance forest.

### 11.2 The Canvas

A **Canvas** is a GPU-accelerated rendering surface. It owns:

- A **pixel buffer** (pre-allocated, BGRA8, sized to maximum expected resolution)
- A **scene graph root** (the top-level `Shape`)
- An **event queue** (thread-safe)
- A **display link** tied to screen refresh

```
Canvas
  ├── pixel buffer (BGRA8, Metal-backed)
  ├── root Shape (the scene graph)
  ├── event queue
  └── display link (renders at screen refresh rate)
```

**On macOS:** Each Canvas lives inside an OS window. Multiple canvases are supported (multiple windows). Created via `create-canvas` which returns a Canvas and opens a window.

**On iOS:** A single full-screen Canvas is the norm. The Canvas fills the screen below the status bar (or the entire screen in full-screen mode). Additional canvases can be created for multi-window iPad support.

**Rendering loop:** On each display-link tick:
1. If the scene graph is dirty (any shape marked for redraw), traverse the tree and call each dirty shape's `/draw` method with a `DrawContext`
2. Upload the pixel buffer to a GPU texture
3. Render a fullscreen quad to the screen

Partial redraw is supported: only dirty regions of the pixel buffer are re-rendered and re-uploaded.

#### Canvas Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `create-canvas` | width, height, title | canvas | Create a new canvas (and OS window on macOS) |
| `canvas-size` | canvas | width, height | Get canvas dimensions |
| `canvas-set-title` | canvas, title | -- | Set window title (macOS only) |
| `canvas-set-size` | canvas, width, height | -- | Resize canvas |
| `canvas-close` | canvas | -- | Close canvas and its window |
| `canvas-root` | canvas | shape | Get the root shape of the scene graph |
| `canvas-set-root` | canvas, shape | -- | Replace the root shape |
| `canvas-invalidate` | canvas | -- | Mark entire canvas for redraw |
| `canvas-scale-factor` | canvas | real | Display scale factor (1.0, 2.0 for Retina, 3.0) |

### 11.3 The Scene Graph

The scene graph is a tree of **Shape** objects. Every visible element -- a button, a line of text, a chart, a game sprite -- is a Shape or a subclass of Shape.

#### The Shape Base Class

`Shape` is the root class of the scene graph hierarchy. All visual elements inherit from it.

**Instance attributes:**

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `bounds` | Rect | (0, 0, 0, 0) | Position and size in parent coordinates |
| `transform` | Transform | identity | Affine transform (translate, rotate, scale, skew) |
| `children` | List | () | Ordered list of child shapes (drawn back-to-front) |
| `parent` | Shape or null | null | Parent shape (set automatically on add) |
| `style` | Style | default | Visual style (fill, stroke, shadow, opacity, corner-radius) |
| `visible` | Boolean | true | Whether this shape and its children are drawn |
| `interactive` | Boolean | true | Whether this shape receives pointer/key events |
| `clips-children` | Boolean | false | Whether children are clipped to this shape's bounds |
| `needs-redraw` | Boolean | true | Dirty flag; set automatically when attributes change |
| `tag` | String | "" | Optional identifier for finding shapes in the tree |
| `cursor` | String | "default" | Cursor to show on hover (macOS: arrow, pointer, crosshair, text, etc.) |

**Key methods:**

| Method | Description |
|--------|-------------|
| `/draw` | Called with a `DrawContext` when the shape needs rendering. Default draws background fill and stroke per `style`. Override for custom drawing. |
| `/layout` | Called before drawing when bounds have changed. Override to position children. |
| `/hit-test` | Given a point in local coordinates, returns true/false. Default checks `bounds`. Override for non-rectangular hit areas (e.g., circles, paths). |
| `/add-child` | Append a child shape. Sets child's `parent`. Marks dirty. |
| `/remove-child` | Remove a child shape. Clears child's `parent`. Marks dirty. |
| `/find-by-tag` | Recursively search children for a shape with the given tag string. |
| `/local-to-global` | Convert a point from this shape's coordinate space to canvas coordinates. |
| `/global-to-local` | Convert a point from canvas coordinates to this shape's local space. |
| `/invalidate` | Mark this shape (and ancestors) as needing redraw. |

#### Coordinate System

- Origin is **top-left** (consistent with iOS/macOS screen coordinates)
- Each shape's `bounds` are in its **parent's coordinate space**
- The `transform` is applied on top of `bounds` positioning
- Children draw in their parent's coordinate space, offset by the parent's bounds origin
- `clips-children` controls whether children can draw outside the parent's bounds

#### Shape Subclasses

| Class | Inherits | Description |
|-------|----------|-------------|
| `Shape` | -- | Base class. Draws a rectangle with style. |
| `Oval` | Shape | Ellipse/circle. `/hit-test` uses ellipse math. |
| `Path` | Shape | Arbitrary bezier path (move-to, line-to, curve-to, close). |
| `TextShape` | Shape | Renders text. Attributes: `text`, `font`, `font-size`, `text-color`, `alignment`, `wraps`. |
| `ImageShape` | Shape | Displays a bitmap image. Attributes: `image`, `scale-mode` (fill, fit, stretch, center). |
| `Group` | Shape | Pure container. No drawing of its own. Useful for grouping and applying a shared transform. |
| `Stack` | Shape | Layout container: arranges children in a line. Attributes: `direction` (horizontal/vertical), `spacing`, `alignment` (start/center/end/stretch). |
| `Grid` | Shape | Layout container: arranges children in a grid. Attributes: `columns`, `row-spacing`, `column-spacing`. |
| `ScrollShape` | Shape | Scrollable container. Attributes: `content-size`, `scroll-offset`, `shows-scrollbars`. Handles scroll/pan events internally. |
| `ClipShape` | Shape | Clips children to an arbitrary `Path`. |

### 11.4 Style

Every Shape has a `Style` object controlling its visual appearance. Style attributes cascade: if a child's style attribute is `null`, it inherits from its parent.

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `fill-color` | Color or null | null | Background fill color |
| `fill-gradient` | Gradient or null | null | Linear or radial gradient fill (overrides fill-color) |
| `stroke-color` | Color or null | null | Border/outline color |
| `stroke-width` | Real | 0.0 | Border/outline thickness |
| `corner-radius` | Real | 0.0 | Rounded corner radius |
| `opacity` | Real | 1.0 | Opacity (0.0 transparent, 1.0 opaque). Applied to shape and all children. |
| `shadow-color` | Color or null | null | Drop shadow color |
| `shadow-offset` | Point | (0, 0) | Drop shadow offset |
| `shadow-blur` | Real | 0.0 | Drop shadow blur radius |
| `padding` | Insets | (0,0,0,0) | Internal padding (top, right, bottom, left) |

#### Color

A `Color` is a data type with four components: red, green, blue, alpha (each 0.0 to 1.0).

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `rgb` | r, g, b | color | Create an opaque color |
| `rgba` | r, g, b, a | color | Create a color with alpha |
| `hsb` | hue, saturation, brightness | color | Create from HSB |
| `color-components` | color | r, g, b, a | Decompose a color |
| `color-blend` | color1, color2, fraction | color | Interpolate between two colors |

Named color constants: `black`, `white`, `red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, `orange`, `purple`, `brown`, `gray`, `light-gray`, `dark-gray`, `clear` (fully transparent).

On macOS/iOS, system semantic colors are also available: `system-background`, `system-label`, `system-accent`, `system-separator`, `system-grouped-background`. These adapt to light/dark mode automatically.

#### Gradient

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `linear-gradient` | color-stops, start-point, end-point | gradient | Linear gradient. Color-stops is a list of (position, color) pairs. |
| `radial-gradient` | color-stops, center, radius | gradient | Radial gradient. |

#### Transform

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `translate` | dx, dy | transform | Translation |
| `rotate` | radians | transform | Rotation around origin |
| `scale` | sx, sy | transform | Scale |
| `transform-concat` | t1, t2 | transform | Concatenate two transforms |
| `transform-invert` | t | transform | Invert a transform |
| `transform-point` | t, point | point | Apply transform to a point |

### 11.5 The DrawContext

When a Shape's `/draw` method is called, it receives a **DrawContext** as input. The DrawContext provides immediate-mode drawing commands that render into the canvas pixel buffer.

The DrawContext maintains a **graphics state stack** (fill color, stroke color, line width, font, clipping, transform). Push/pop to save and restore state.

#### Drawing Primitives

**Rectangles:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-fill-rect` | ctx, rect | ctx | Fill a rectangle with current fill color |
| `draw-stroke-rect` | ctx, rect | ctx | Stroke a rectangle outline |
| `draw-round-rect` | ctx, rect, radius | ctx | Fill a rounded rectangle |
| `draw-stroke-round-rect` | ctx, rect, radius | ctx | Stroke a rounded rectangle outline |

**Ovals:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-fill-oval` | ctx, rect | ctx | Fill an ellipse inscribed in rect |
| `draw-stroke-oval` | ctx, rect | ctx | Stroke an ellipse outline |

**Paths:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-fill-path` | ctx, path | ctx | Fill a path |
| `draw-stroke-path` | ctx, path | ctx | Stroke a path |
| `path-move-to` | path, x, y | path | Move current point |
| `path-line-to` | path, x, y | path | Line from current point to (x,y) |
| `path-curve-to` | path, cp1x, cp1y, cp2x, cp2y, x, y | path | Cubic bezier curve |
| `path-quad-to` | path, cpx, cpy, x, y | path | Quadratic bezier curve |
| `path-arc` | path, cx, cy, radius, start-angle, end-angle | path | Circular arc |
| `path-close` | path | path | Close the current subpath |
| `path-create` | -- | path | Create a new empty path |

**Text:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-text` | ctx, string, point | ctx | Draw text at point with current font and fill color |
| `draw-text-in-rect` | ctx, string, rect, alignment | ctx | Draw text wrapped within a rectangle |
| `measure-text` | ctx, string | width, height | Measure text size with current font |

**Images:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-image` | ctx, image, rect | ctx | Draw image scaled to rect |
| `draw-image-at` | ctx, image, point | ctx | Draw image at natural size at point |
| `load-image` | path-string | image | Load an image from file |
| `image-size` | image | width, height | Get image dimensions |
| `create-image` | width, height | image | Create a blank image (for offscreen drawing) |

**Lines:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-line` | ctx, x1, y1, x2, y2 | ctx | Draw a line |

**State:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `draw-set-fill` | ctx, color | ctx | Set fill color |
| `draw-set-stroke` | ctx, color | ctx | Set stroke color |
| `draw-set-line-width` | ctx, width | ctx | Set line width |
| `draw-set-font` | ctx, font-name, size | ctx | Set font |
| `draw-set-font-bold` | ctx, bold? | ctx | Toggle bold |
| `draw-set-font-italic` | ctx, italic? | ctx | Toggle italic |
| `draw-push-state` | ctx | ctx | Push graphics state onto stack |
| `draw-pop-state` | ctx | ctx | Pop and restore graphics state |
| `draw-push-transform` | ctx, transform | ctx | Apply transform and push state |
| `draw-clip-rect` | ctx, rect | ctx | Intersect clipping region with rect |
| `draw-clip-path` | ctx, path | ctx | Intersect clipping region with path |

**Note:** All draw primitives take the `DrawContext` as first input and pass it through as output. This enables chaining in the dataflow graph: the context flows through a pipeline of drawing operations, with execution wires ensuring correct ordering when the same context passes through independent branches.

#### Retained vs. Immediate Drawing

Most UIs should be built with the **retained** scene graph (compose Shape objects, let the runtime draw them). Override `/draw` for **custom rendering** -- charts, games, visualizations, or any shape that needs procedural drawing.

The default `/draw` method on Shape renders the shape's `style` (fill, stroke, corner radius, shadow). Subclass implementations call the default first (via super), then add custom drawing on top.

### 11.6 Layout System

Layout in the original ABCs required manual pixel positioning. Phograph uses automatic layout via container shapes.

#### Stack Layout

`Stack` arranges its children sequentially along one axis:

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `direction` | Symbol | `vertical` | `horizontal` or `vertical` |
| `spacing` | Real | 0.0 | Gap between children |
| `alignment` | Symbol | `start` | Cross-axis alignment: `start`, `center`, `end`, `stretch` |
| `distribution` | Symbol | `packed` | Main-axis distribution: `packed`, `space-between`, `space-around`, `space-evenly` |

#### Grid Layout

`Grid` arranges children in a grid:

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `columns` | Integer | 2 | Number of columns |
| `row-spacing` | Real | 0.0 | Gap between rows |
| `column-spacing` | Real | 0.0 | Gap between columns |
| `cell-alignment` | Symbol | `center` | Alignment of items within each cell |

#### Size Constraints

Any Shape can specify size constraints that the layout system respects:

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `min-width` | Real or null | null | Minimum width |
| `max-width` | Real or null | null | Maximum width |
| `min-height` | Real or null | null | Minimum height |
| `max-height` | Real or null | null | Maximum height |
| `flex` | Real | 0.0 | Flex grow factor within a Stack (0 = fixed size, >0 = proportional share of remaining space) |

Layout is computed in the `/layout` method, which runs top-down before drawing. A shape's `/layout` is called only when its bounds or children have changed (dirty flag).

---

## 12. Input: Pointer, Gesture, and Keyboard Events

The original ABCs used a rigid hierarchy of Behavior/Inject indirection for event handling. Phograph replaces this with a direct, unified event system that treats mouse and touch as variants of the same **Pointer** abstraction, adds first-class **Gesture** recognition, and dispatches events as ordinary method calls on shapes.

### 12.1 Design Principles

1. **Mouse and touch are both pointers.** A mouse click and a finger tap both produce `pointer-down` → `pointer-up`. The event carries metadata distinguishing them (button identity, pressure, is-touch flag), but handlers that don't care can treat them identically.
2. **Gestures layer on top of raw pointers.** Tap, long-press, pan, pinch, rotate, and scroll are recognized from raw pointer streams and dispatched as separate, higher-level events.
3. **Events are class methods.** No Behavior Editor, no inject indirection. You handle a tap on a button by writing a `/on-tap` method on your Button subclass (or on that specific instance's class). Data flows in via the method's inputs; you act on it with normal Phograph operations.
4. **Hit testing walks the scene graph.** The deepest visible, interactive shape under the pointer receives the event first. If it doesn't handle it (no matching method), the event propagates up to the parent, and so on to the root.
5. **Pointer capture.** A shape can claim a pointer (by pointer-id) on `pointer-down`. All subsequent move/up events for that pointer go directly to the capturing shape, bypassing hit-testing. Released on `pointer-up` or explicitly.

### 12.2 The Pointer Event

A `PointerEvent` is a data type flowing through event-handler methods.

| Field | Type | Description |
|-------|------|-------------|
| `type` | Symbol | `down`, `move`, `up`, `cancel` |
| `x` | Real | X position in the **target shape's** local coordinate space |
| `y` | Real | Y position in the target shape's local coordinate space |
| `canvas-x` | Real | X position in canvas (global) coordinates |
| `canvas-y` | Real | Y position in canvas (global) coordinates |
| `button` | Symbol | `primary`, `secondary`, `middle` (mouse) or `touch` (finger) |
| `pointer-id` | Integer | Unique ID for this pointer (enables multi-touch tracking) |
| `pressure` | Real | 0.0 to 1.0 (Apple Pencil / Force Touch; 1.0 for mouse clicks) |
| `is-touch` | Boolean | true for finger/pencil, false for mouse |
| `modifiers` | List | Active modifier keys: sublists of `shift`, `control`, `option`, `command` |
| `timestamp` | Real | Event timestamp in seconds |
| `click-count` | Integer | 1 for single click, 2 for double, 3 for triple (mouse only) |

#### Pointer Event Handler Methods

Shapes handle pointer events by implementing these class methods:

| Method | Inputs | When Called |
|--------|--------|------------|
| `/on-pointer-down` | self, event | Pointer pressed on this shape |
| `/on-pointer-move` | self, event | Pointer moved while over this shape (or while captured) |
| `/on-pointer-up` | self, event | Pointer released on this shape (or while captured) |
| `/on-pointer-cancel` | self, event | Pointer sequence cancelled by system |
| `/on-pointer-enter` | self, event | Pointer entered this shape's bounds (hover) |
| `/on-pointer-exit` | self, event | Pointer exited this shape's bounds (hover) |

**Pointer capture:**

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `capture-pointer` | shape, pointer-id | -- | Capture all events for this pointer-id to this shape |
| `release-pointer` | shape, pointer-id | -- | Release capture; resume hit-testing |

Example -- a draggable shape:
```
MyDraggable/on-pointer-down (case 1:1)
  Input: self, event
  [capture-pointer] ← self, event.pointer-id
  [//set drag-origin] ← (event.x, event.y)

MyDraggable/on-pointer-move (case 1:1)
  Input: self, event
  [//get drag-origin] → (ox, oy)
  [//get bounds] → rect
  [-] event.x - ox → dx
  [-] event.y - oy → dy
  [offset-rect] rect, dx, dy → new-rect
  [//set bounds] ← new-rect
  [//set drag-origin] ← (event.x, event.y)

MyDraggable/on-pointer-up (case 1:1)
  Input: self, event
  [release-pointer] ← self, event.pointer-id
```

### 12.3 Gesture Events

Gesture recognizers convert raw pointer streams into semantic events. They run concurrently with raw pointer dispatch and can **consume** pointer events (preventing the raw handlers from firing) or coexist.

#### Built-in Gesture Recognizers

| Gesture | Trigger | Available On |
|---------|---------|--------------|
| **Tap** | Quick press-and-release | Both |
| **Double-Tap** | Two quick taps | Both |
| **Long-Press** | Press held for 0.5s | Both (on Mac: also right-click) |
| **Pan** | Press and drag | Both |
| **Scroll** | Scroll wheel / two-finger trackpad scroll / two-finger touch pan | Both |
| **Pinch** | Two-finger pinch (touch) or Cmd+scroll (Mac) | Both |
| **Rotate** | Two-finger rotation | iOS primarily; Option+drag on Mac |

#### GestureEvent Data Type

| Field | Type | Description |
|-------|------|-------------|
| `type` | Symbol | `tap`, `double-tap`, `long-press`, `pan`, `scroll`, `pinch`, `rotate` |
| `state` | Symbol | `began`, `changed`, `ended`, `cancelled` |
| `x` | Real | Position in target shape's local coordinates |
| `y` | Real | Position in target shape's local coordinates |
| `modifiers` | List | Active modifier keys |
| `velocity-x` | Real | Pan/scroll velocity X (points/sec) |
| `velocity-y` | Real | Pan/scroll velocity Y (points/sec) |
| `delta-x` | Real | Pan/scroll delta since last event |
| `delta-y` | Real | Pan/scroll delta since last event |
| `scale` | Real | Pinch scale factor (1.0 = no change) |
| `rotation` | Real | Rotation in radians |
| `pressure` | Real | Force Touch / Pencil pressure |

#### Gesture Handler Methods

| Method | Inputs | When Called |
|--------|--------|------------|
| `/on-tap` | self, event | Quick press-and-release |
| `/on-double-tap` | self, event | Two quick taps |
| `/on-long-press` | self, event | Press held 0.5s |
| `/on-pan` | self, event | Press-and-drag (state: began/changed/ended) |
| `/on-scroll` | self, event | Scroll wheel or two-finger scroll |
| `/on-pinch` | self, event | Pinch to zoom |
| `/on-rotate` | self, event | Two-finger rotation |

#### Attaching Gesture Recognizers

Gestures are enabled per-shape. By default, all shapes respond to any gesture for which they implement the handler method. To explicitly control gesture recognition:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `enable-gesture` | shape, gesture-type | -- | Enable a specific gesture on this shape |
| `disable-gesture` | shape, gesture-type | -- | Disable a specific gesture |
| `set-gesture-option` | shape, gesture-type, option, value | -- | Configure a gesture (e.g., long-press duration) |

#### Touch-to-Mouse Mapping (iOS)

On iOS, where there is no mouse, touch gestures map to the traditional Prograph button model:

| Touch Gesture | Maps To |
|---------------|---------|
| Single-finger tap/drag | Primary button (left-click) |
| Long-press (0.5s hold) | Secondary button (right-click); haptic feedback |
| Two-finger tap | Secondary button (right-click); light haptic |
| Two-finger pan | Scroll event |
| Pinch | Scroll event with `command` modifier (zoom) |

This mapping is built into the runtime and fires automatically. Shapes that only handle `/on-pointer-down` and `/on-pointer-up` work identically on Mac and iOS without any platform-specific code.

### 12.4 Keyboard Events

| Field | Type | Description |
|-------|------|-------------|
| `type` | Symbol | `key-down`, `key-up` |
| `key-code` | Integer | Platform key code |
| `character` | String | The typed character (empty for modifier-only keys) |
| `modifiers` | List | Active modifiers |
| `is-repeat` | Boolean | true if this is an auto-repeat event |

#### Keyboard Handler Methods

| Method | Inputs | When Called |
|--------|--------|------------|
| `/on-key-down` | self, event | Key pressed while this shape has keyboard focus |
| `/on-key-up` | self, event | Key released |
| `/on-text-input` | self, string | Committed text input (handles IME, dead keys, etc.) |

#### Keyboard Focus

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `request-focus` | shape | -- | Give keyboard focus to this shape |
| `resign-focus` | shape | -- | Release keyboard focus |
| `has-focus?` | shape | boolean | Check if this shape has keyboard focus |

Focus methods on shapes:

| Method | Inputs | When Called |
|--------|--------|------------|
| `/on-focus` | self | This shape received keyboard focus |
| `/on-blur` | self | This shape lost keyboard focus |

### 12.5 Event Propagation

Events propagate through the scene graph using a **bubble** model:

1. **Hit test:** Find the deepest visible, interactive shape whose bounds (or `/hit-test` override) contain the pointer position.
2. **Dispatch:** Call the appropriate handler method (e.g., `/on-tap`) on the hit shape.
3. **Propagate:** If the shape does not implement the handler (no matching method exists), the event propagates to the parent shape. This continues up to the root.
4. **Consume:** If a handler method exists and executes successfully, the event is consumed. The handler can explicitly pass the event to the parent by calling `/propagate-event` with self and the event.

This is simpler than the ABCs' Commander chain. No Behavior objects, no inject indirection. You write a method on a shape; the event reaches it.

### 12.6 Modifier Strip (iOS)

On iOS, where there is no physical keyboard for modifier keys, a floating **ModifierStrip** can be displayed at the screen edge. It provides toggle buttons for Shift, Control, Option, and Command. When toggled on, subsequent pointer events include the corresponding modifier in their `modifiers` list.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `show-modifier-strip` | canvas | -- | Show the modifier key strip |
| `hide-modifier-strip` | canvas | -- | Hide the modifier key strip |
| `modifier-strip-visible?` | canvas | boolean | Check visibility |

### 12.7 Menus

Menus are not a separate 8-class hierarchy. They are `Shape` subclasses with built-in event handling.

#### MenuShape

A `MenuShape` is a vertical list of `MenuItemShape` children. It appears as a popup when triggered, disappears when an item is selected or the user clicks outside.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `menu-create` | items-list | menu | Create a menu from a list of (label, tag) pairs |
| `menu-show` | menu, canvas, x, y | -- | Show menu as popup at position |
| `menu-add-item` | menu, label, tag | -- | Add an item |
| `menu-add-separator` | menu | -- | Add a separator line |
| `menu-add-submenu` | menu, label, submenu | -- | Add a submenu |

`MenuItemShape` delivers events via `/on-menu-select` on the target shape:

| Method | Inputs | When Called |
|--------|--------|------------|
| `/on-menu-select` | self, tag | A menu item with this tag was selected in a menu owned by this shape |

#### Menu Bar (macOS)

On macOS, the system menu bar is exposed via platform primitives:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `menubar-set` | menu-structure | -- | Set the application menu bar from a nested list structure |
| `menubar-set-item-enabled` | menu-title, item-title, enabled? | -- | Enable/disable a menu item |
| `menubar-set-item-checked` | menu-title, item-title, checked? | -- | Check/uncheck a menu item |

The menu-structure is a list of lists: `(("File" ("New" "new") ("Open" "open") ("---") ("Quit" "quit")) ("Edit" ("Cut" "cut") ...))`. Tags are delivered via a global `/on-menu-command` method.

On iOS, the menu bar is not used. Contextual actions use `MenuShape` popups triggered by long-press or secondary tap.

### 12.8 Built-in Control Shapes

A small set of ready-made interactive shapes. Each is a Shape subclass with appropriate event handlers and drawing. Users can subclass these or build entirely custom controls from scratch.

| Class | Inherits | Description |
|-------|----------|-------------|
| `Button` | Shape | Push button with label. Attributes: `label` (string), `enabled` (boolean). Fires `/on-tap` on the button itself; the button's parent receives `/on-button-tap` with the button as input. |
| `Toggle` | Shape | Checkbox / switch. Attributes: `checked` (boolean), `label` (string). Fires `/on-toggle-changed` on parent with self and new value. |
| `Slider` | Shape | Value slider. Attributes: `value` (real, 0-1), `min-value`, `max-value`. Fires `/on-slider-changed` on parent. |
| `TextField` | Shape | Text input (single-line). Attributes: `text` (string), `placeholder` (string), `editable` (boolean). Requests keyboard focus on tap. Fires `/on-text-changed` and `/on-text-committed` on parent. |
| `TextArea` | Shape | Multi-line text input with scrolling. Same events as TextField. |
| `Label` | TextShape | Non-interactive text display. Convenience subclass of TextShape with `interactive` defaulting to false. |
| `ListView` | ScrollShape | Efficient scrolling list with view recycling. Attributes: `item-count` (integer), `item-height` (real). Calls `/on-list-item` to request drawing of each visible item. Fires `/on-list-select` when an item is tapped. |
| `ProgressBar` | Shape | Progress indicator. Attributes: `progress` (real, 0-1), `indeterminate` (boolean). |
| `ModalOverlay` | Shape | Semi-transparent overlay that captures all events. For dialogs and alerts. |

#### Dialogs

Simple modal dialogs are provided as primitives (these block execution, similar to old Prograph `ask`/`answer`):

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `alert` | title, message, button-labels | selected-label | Show a modal alert with 1-3 buttons; returns which was tapped |
| `prompt` | title, message, default-text | text or null | Show a text-input dialog; returns entered text or null if cancelled |
| `confirm` | title, message | boolean | Show OK/Cancel dialog; returns true/false |

### 12.9 Clipboard

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `clipboard-get-text` | -- | string or null | Get text from system clipboard |
| `clipboard-set-text` | string | -- | Set text on system clipboard |
| `clipboard-get-image` | -- | image or null | Get image from system clipboard |
| `clipboard-set-image` | image | -- | Set image on system clipboard |
| `clipboard-has-text?` | -- | boolean | Check if clipboard contains text |
| `clipboard-has-image?` | -- | boolean | Check if clipboard contains image |

### 12.10 Cursor (macOS)

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `set-cursor` | cursor-name | -- | Set cursor: `arrow`, `pointer`, `crosshair`, `text`, `grab`, `grabbing`, `resize-ns`, `resize-ew`, `resize-nwse`, `resize-nesw`, `not-allowed`, `wait` |
| `push-cursor` | cursor-name | -- | Push cursor onto stack |
| `pop-cursor` | -- | -- | Restore previous cursor |
| `hide-cursor` | -- | -- | Hide cursor |
| `show-cursor` | -- | -- | Show cursor |

Per-shape cursors are also supported: set the `cursor` attribute on any Shape, and the runtime automatically changes the cursor on hover.

### 12.11 Animation

Shapes can be animated by interpolating attribute values over time.

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `animate` | shape, attribute, from, to, duration, easing | animation | Animate an attribute from one value to another |
| `animate-delay` | animation, delay | animation | Add a start delay |
| `animate-on-complete` | animation, method-name | animation | Call a method when animation completes |
| `animate-cancel` | animation | -- | Cancel a running animation |
| `animate-group` | animations-list | animation | Run multiple animations in parallel |
| `animate-sequence` | animations-list | animation | Run animations in sequence |

Easing functions: `linear`, `ease-in`, `ease-out`, `ease-in-out`, `spring`.

Example: fade in a shape over 0.3 seconds with ease-out:
```
[animate] my-shape, "opacity", 0.0, 1.0, 0.3, "ease-out" → anim
```

### 12.12 Files and Documents

File I/O is handled by simple primitives, not a 9-class hierarchy:

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `file-read-text` | path | string | Read entire file as text |
| `file-write-text` | path, string | -- | Write string to file |
| `file-read-data` | path | data | Read file as raw byte data |
| `file-write-data` | path, data | -- | Write raw data to file |
| `file-read-object` | path | object | Deserialize an object from file |
| `file-write-object` | path, object | -- | Serialize an object to file |
| `file-exists?` | path | boolean | Check if file exists |
| `file-delete` | path | -- | Delete a file |
| `file-list` | directory-path | list of strings | List files in a directory |
| `file-pick` | title, file-types | path or null | System file picker dialog |
| `file-pick-save` | title, default-name | path or null | System save dialog |

### 12.13 Printing

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `print-shape` | shape | -- | Print a shape tree using the system print dialog |
| `print-to-pdf` | shape, path | -- | Render a shape tree to PDF file |

### 12.14 Platform Integration

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `platform` | -- | string | Returns `"macos"` or `"ios"` |
| `screen-size` | -- | width, height | Screen dimensions in points |
| `screen-scale` | -- | real | Display scale factor |
| `is-dark-mode?` | -- | boolean | System dark mode state |
| `open-url` | url-string | -- | Open URL in system browser |
| `haptic-feedback` | intensity | -- | Trigger haptic (iOS only; no-op on Mac) |
| `status-bar-style` | style | -- | Set iOS status bar: `default`, `light`, `dark`, `hidden` |
| `safe-area-insets` | canvas | top, right, bottom, left | Safe area insets (notch, home indicator, etc.) |

### 12.15 Rendering Pipeline Summary

The complete rendering pipeline, from scene graph to pixels on screen:

```
Scene Graph (Shape tree, Phograph objects)
    │
    ▼  layout pass (top-down, dirty shapes only)
Shape/layout methods compute child positions
    │
    ▼  draw pass (back-to-front tree traversal, dirty regions only)
Shape/draw methods issue DrawContext commands
    │
    ▼  rasterization
DrawContext → pixel buffer (BGRA8, pre-allocated, in-language memory)
    │
    ▼  GPU upload (dirty regions only)
pixel buffer → Metal texture (texture.replace)
    │
    ▼  GPU composite
Metal renders fullscreen quad with texture
    │
    ▼
Screen
```

**Thread model:**
- Canvas rendering (Metal) runs on the main thread (`@MainActor`, Apple requirement)
- Dataflow operations run as Swift tasks on background executors; UI-touching operations are automatically `@MainActor`
- Operations on the same object (actor) are serialized; operations on independent objects run in parallel (see §9, Actor-Based Concurrency)
- The event queue bridges user input to the dataflow graph via `@MainActor` event handlers
- The pixel buffer uses its own actor isolation for resize safety

### 12.16 Comparison: Original ABCs vs. Phograph Canvas System

| Aspect | Original ABCs | Phograph Canvas |
|--------|---------------|-----------------|
| Class count | ~147 classes | ~25 classes |
| Widget model | OS-native wrappers (Mac Toolbox) | In-language scene graph; everything drawn by Phograph |
| Event handling | Behavior objects + inject indirection | Direct method calls on shape subclasses |
| Drawing | Mac Toolbox QuickDraw | DrawContext primitives into GPU-backed pixel buffer |
| Layout | Manual pixel positioning | Automatic: Stack, Grid, constraints, flex |
| Platform | Mac Toolbox only | macOS + iOS from one codebase |
| Touch support | None | First-class: unified pointer model, gestures, haptics |
| Animation | None built-in | Built-in attribute animation with easing |
| Conditionals for UI state | Behavior Editor GUI | Standard Phograph case/control flow |
| Menus | 8 separate classes | One MenuShape class + primitives |
| Documents/Files | 9 classes (Document, Document Data, File, etc.) | File I/O primitives |
| Printing | 2 classes (Printer, Print Layout) | `print-shape` primitive |
| Windowing | Desktop > Window > View > WindowItem hierarchy | Canvas with Shape tree; thin OS window wrapper on Mac |
| Clipboard | Clipboard class | Clipboard primitives |
| WYSIWYG editors | 15 separate ABE editor processes | Not needed; shapes are composed programmatically or via a visual scene builder (future) |

---

## 13. Execution Model and Debugging

### Interpreter / Incremental Compiler

The Phograph development environment uses an **incremental compiler** that compiles individual methods on demand. Programs can be edited while running. The environment is tightly integrated: editor, compiler, and debugger are unified.

Key capabilities:
- Methods can be created while executing (via the "method does not exist" alert)
- Individual methods can be executed in isolation (`Execute Method`)
- Execution can be resumed after creating missing methods (`Resume` from Exec menu)
- Abort infinite loops: Command+Period
- **Hot reloading** (inspired by Erlang): edit a node's implementation while the graph is running. The change takes effect on the next evaluation cycle. No restart needed.

### Stand-Alone Compiler

The compiler produces standalone applications via a two-stage process:

1. **Visual graph → Swift source code**: the dataflow graph is translated to Swift source, preserving the dual representation (§2). Each method becomes a Swift function. Each class becomes a Swift actor (or class, if `@nonactor`).
2. **Swift → native binary**: the Swift compiler produces optimized ARM64 code.

Compilation details:
- Tree-shakes unused classes and methods to reduce binary size
- Supports automatic persistence (same behavior as interpreter -- see §10)
- Produces universal binaries for macOS (Apple Silicon + Intel) or iOS .ipa bundles
- The generated Swift code is human-readable and can be inspected or modified

### Automatic Parallelism from Graph Structure

Inspired by LabVIEW and Swift Structured Concurrency, independent branches of the dataflow graph compile to **concurrent Swift tasks** automatically. The programmer does not explicitly thread -- the compiler infers parallelism from the graph structure.

**How it works:**
- The compiler analyzes data dependencies in each method's graph.
- Operations with no data dependency between them are placed in a **Swift TaskGroup** and execute concurrently.
- The TaskGroup awaits all results before downstream operations fire.
- A compound node (subgraph / method call) is a child task. Cancellation propagates down the task tree.

**Example:**
```
Input ──┬──▶ [http-get url-a] ──┬──▶ [combine-results] ──▶ Output
        └──▶ [http-get url-b] ──┘
```
The two `http-get` operations have no dependency between them. The compiler emits:
```swift
async let resultA = httpGet(urlA)
async let resultB = httpGet(urlB)
let combined = combineResults(await resultA, await resultB)
```

**UI safety:** Operations that touch UI are automatically `@MainActor`. The compiler enforces that non-`@MainActor` code does not directly access `@MainActor` state (visualized as dashed wires crossing an actor boundary).

### Incremental Evaluation

Inspired by Hazel's fill-and-resume and Compose's positional memoization:

- When a node is added or modified in a running graph, only the **affected subgraph** re-evaluates. Unaffected nodes retain their previous results.
- The runtime tracks which state each node depends on. When state changes, only nodes downstream of the change re-evaluate.
- This makes live editing fast even in large graphs.

### Debugging

Phograph's debugging is visual and deeply integrated:

| Feature | Description |
|---------|-------------|
| **Visual execution** | Each operation highlights as it executes; stippled background on active case window |
| **Data inspection** | Tooltip-like popups show data values on any wire when stopped |
| **Value modification** | Change data values mid-execution and resume |
| **Live code editing** | Modify code while stopped in debugger; no recompile needed |
| **Breakpoints** | On data wires (break when value passes) and execution wires (break before operation fires) |
| **Single-step** | Step operation by operation, following execution wires |
| **Roll-back** | Step backward through execution history |
| **Roll-forward** | Step forward after rolling back |
| **Execution stack** | Visual display of the call stack |
| **Error isolation** | Crashed nodes show red with error message; rest of graph continues (see §7.1) |

### Trace-Driven Development (Phograph Extension)

Inspired by Darklang, every graph execution **records a trace** -- the value on every wire at every evaluation:

- When you open a graph, you see the **actual values from the last run** on every wire. No need to set breakpoints or add `log` nodes.
- Values are rendered **appropriately for their type**: numbers as numbers, strings as strings, lists as scrollable tables, images as thumbnails, colors as swatches.
- Clicking a wire shows the **value history**: all values that have flowed through it across recent evaluations.
- You develop against **real data**, not hypothetical inputs.

Traces are stored in the project's debug data (not committed to version control). They can be exported for bug reports.

**Trace-based testing:** A trace can be saved as a **test fixture**. The system re-runs the graph with the recorded inputs and verifies that the outputs match. This generates regression tests from normal development.

### Per-Node Preview Toggle

Inspired by Grasshopper, each node can have its output **previewed** inline in the graph. The preview shows a small visualization of the output value:

- Numbers: the value
- Strings: first ~50 characters
- Lists: count and first few elements
- Images: thumbnail
- Objects: class name and key attribute values
- Shapes: rendered preview

Previews can be toggled on/off per-node (right-click → Toggle Preview). Previews update in real time during live evaluation.

### Concurrency

Phograph maps to **Swift Structured Concurrency** (see §7.2):

| Phograph Concept | Swift Mapping |
|-----------------|--------------|
| Independent graph branches | `async let` / `TaskGroup` |
| Class instance | `actor` (default) or `class` (`@nonactor`) |
| Method call to another object | `await actor.method()` |
| Future | `Task<T, Error>` |
| Channel | `AsyncStream` / `AsyncChannel` |
| Dispatch | `Task { }` |
| Dispatch-main | `MainActor.run { }` |
| Observable attribute | `@Published` / Combine publisher |

---

## 14. External Code Integration

### Overview

Phograph runs on Apple platforms (macOS, iOS, iPadOS). External code integration targets **Swift** as the primary interop language, with C interop for lower-level needs. The original Prograph's Mac Toolbox / Pascal integration is replaced entirely.

### Swift Interop

Phograph can call Swift functions and methods, and Swift code can call Phograph methods. This is the primary mechanism for accessing platform APIs (UIKit, AppKit, SwiftUI views, CoreData, AVFoundation, etc.).

#### Calling Swift from Phograph

An **external method** icon (rectangle with lines at top and bottom edges) represents a call to a Swift function. External methods are declared in a **bridge section** that maps Phograph names to Swift function signatures.

```
Bridge declaration:
  Phograph name: "camera-capture"
  Swift function: CameraCapture.takePhoto() -> UIImage?
  Inputs: none
  Outputs: image or null
```

Type marshaling between Phograph and Swift:

| Phograph Type | Swift Type |
|---------------|------------|
| Integer | Int |
| Real | Double |
| String | String |
| Boolean | Bool |
| List | [Any] |
| Dict | [AnyHashable: Any] |
| Null | nil (Optional) |
| Object | Wrapped in a PhographObject container |
| Error | PhographError (struct with message, code, details) |
| Enum | Swift enum with associated values |
| External | The raw Swift/ObjC object reference |

The `External` type holds an opaque reference to a Swift or Objective-C object. It can be passed to other external methods but not inspected from Phograph code. To extract data from an External, call an external method that returns Phograph-native types.

#### Calling Phograph from Swift

Swift code can invoke Phograph methods via the runtime API:

```swift
let runtime = PhographRuntime.shared
let result = runtime.call("MyClass/calculate", args: [42, 3.14])
```

This enables:
- Swift UI code calling back into Phograph logic
- Platform event handlers delegating to Phograph methods
- Integration with Swift frameworks that require callbacks/delegates

#### External Method Declaration Primitives

| Primitive | Inputs | Outputs | Description |
|-----------|--------|---------|-------------|
| `external-call` | module, function-name, args-list | result | Dynamic external call (slower, no bridge section needed) |
| `external-async-call` | module, function-name, args-list | future | Async external call, returns a Future |

### C Interop

For C libraries and lower-level system calls, Phograph supports calling C functions through Swift's C interop:

- C functions are wrapped in a thin Swift bridge
- Pointer types are represented as `External` values in Phograph
- Memory management for C allocations must be handled explicitly (allocate/free)

### Automatic Swift API Surface (Phograph Extension)

Inspired by Unity Visual Scripting's automatic node generation, Phograph can **automatically import any Swift type, method, and property as a node** -- no manual bridge declarations needed.

**How it works:**
1. The Phograph compiler reads Swift module interfaces (`.swiftinterface` files) for imported frameworks.
2. Every public type, method, property, and enum case is automatically available as a node in the fuzzy finder (§5).
3. Type marshaling (see table above) applies automatically. Swift generics are handled via wildcard pins.
4. The programmer types `UIImage` in the fuzzy finder and sees all `UIImage` methods as available nodes.

**Node naming convention:** Swift methods appear as `TypeName.methodName`. For instance, `String.count`, `Array.append`, `URLSession.data(from:)`.

**Advantages:**
- No manual bridge maintenance. When Apple releases a new API, it's immediately available.
- Autocomplete shows Swift documentation inline.
- Type safety is preserved: the generated node's pins have the correct Phograph types.

**Limitations:**
- Complex Swift generics (associated types, existentials) may require manual bridge hints.
- Objective-C APIs accessed through Swift bridging headers are supported but may have less precise type information.
- The automatic surface is read-only -- the programmer cannot modify the generated nodes, only use them.

For cases where automatic import doesn't suffice, the manual bridge declaration (see above) remains available.

### Platform API Access

Common platform APIs are additionally exposed as **curated Phograph primitive sections** with Phograph-idiomatic naming and documentation (importable sections, not built into the core language):

| Section | Contents |
|---------|----------|
| `Platform.Camera` | Photo/video capture, camera permissions |
| `Platform.Location` | GPS, heading, geofencing |
| `Platform.Notifications` | Local and push notifications |
| `Platform.Keychain` | Secure credential storage |
| `Platform.UserDefaults` | Simple key-value preferences |
| `Platform.Biometrics` | Face ID / Touch ID |
| `Platform.Share` | System share sheet |
| `Platform.Speech` | Text-to-speech, speech recognition |
| `Platform.Haptics` | Haptic feedback engine |
| `Platform.StoreKit` | In-app purchases |

These sections wrap Swift/platform APIs into Phograph primitives with Phograph-native types. They are optional imports -- a Phograph program only links what it uses. These exist alongside the automatic API surface for convenience and better documentation.

---

## 15. Known Limitations of the Original and How Phograph Addresses Them

Documented weaknesses of the original Prograph, with their status in Phograph:

### 1. Unlabeled Inputs/Outputs -- PARTIALLY ADDRESSED
No inline labels on wires or nodes. Understanding a method requires reading comments or memorizing arity conventions. **Status: partially addressed.** Pin definitions (`PinDef`) now carry names, and optional type annotations (§4) add labels. Wire names are displayed in the IDE.

### 2. Non-Routable Wiring
Wires could not be manually routed around obstacles, creating visual "spaghetti" for complex methods. **Status: partially addressed.** Automatic wire routing with manual override in the Phograph editor. Reroute/knot nodes (§5) allow explicit wire path control.

### 3. No Inline Grouping -- ADDRESSED
No mechanism to group operations without creating a full named method. **Status: addressed.** Inline expression nodes (§5) handle simple cases. `NodeType::LocalMethod` embeds a complete method body inline without creating a named method. Visual regions for larger groups remain open for the IDE.

### 4. Confusing Conditionals -- ADDRESSED
The if-then-else pattern via case structure and control annotations was "far and away the most confusing construct in the language." **Status: addressed.** Pattern matching guards on cases (§9) -- TypeMatch, ValueMatch, and Wildcard -- let cases declare what they expect rather than imperatively testing for it. The underlying control annotation system is retained for advanced use.

### 5. Window Proliferation (IDE)
Opening methods, classes, and subclasses in the original generated many overlapping windows. **Status: open.** The Phograph IDE should use tabbed interface, split views, and breadcrumb navigation.

### 6. Non-Textual Sharing -- ADDRESSED
Code could not be easily shared via email or text. **Status: addressed.** Phograph's dual visual/textual representation (§2) means every graph has equivalent Swift source code. Diffs, code review, and sharing use the text representation. The graph serialization format is JSON.

### 7. Static Complexity -- PARTIALLY ADDRESSED
Large projects with many side-effects were difficult to reason about. **Status: partially addressed.** Execution wires (§2) make side-effect ordering explicit and visible. Managed effects (§7.3) enable pure dataflow graphs. Error cluster wiring (§7.1) makes error paths visible. Actor-based concurrency (§9) isolates mutable state. Remaining IDE work: effect highlighting, data flow path tracing.

### 8. No Access Control -- ADDRESSED
All attributes were effectively public. **Status: replaced.** Phograph adds public/private/protected/read-only visibility modifiers on attributes and methods (§9). Attributes default to private; methods default to public.

### 9. No Destructors / Cleanup -- ADDRESSED
No automatic cleanup mechanism for objects. **Status: replaced.** Phograph uses ARC with cycle detection and adds `/finalize` methods called automatically on deallocation (§4, Memory Management).

### 10. Window System / UI Framework -- ADDRESSED
The original ABCs (147 classes wrapping Mac Toolbox widgets) were platform-locked, over-engineered, and the Behavior/Inject indirection for event handling was confusing. **Status: replaced.** Phograph uses a canvas-based scene graph with ~25 classes (§11), unified pointer/gesture input (§12), direct drawing via DrawContext, automatic layout, and first-class touch support. No OS-native widgets. No Behavior Editor. No inject indirection. Works on both macOS and iOS from one codebase.

### 11. No Error Information -- ADDRESSED
The Fail mechanism propagated a bare boolean with no message or context. **Status: replaced.** Phograph adds Error values with message/code/details, try annotations for error capture, and `fail-with` for error propagation (§7.1).

### 12. No Concurrency/Async Model -- ADDRESSED
Thread primitives were mentioned but never specified. **Status: replaced.** Phograph defines Futures, dispatch, channels, and async variants of blocking primitives. The dataflow firing rule handles async waiting naturally (§7.2).

### 13. No Dict/Map Data Type -- ADDRESSED
The original used lists of lists for key-value data. **Status: replaced.** Phograph adds a first-class Dict type with full primitive support and list-annotation compatibility (§4).

### 14. Three Confusing "Nothing" Values -- ADDRESSED
NULL, NONE, and Undefined had overlapping, unclear semantics. **Status: replaced.** Phograph has a single `null` value (§4).

### 15. Broken Persistence Model -- ADDRESSED
The interpreter embedded persistent values in the program file (mixing code and data). Compiled programs had no automatic persistence. **Status: replaced.** Phograph stores persistents in a separate data file. Persistence works identically in interpreter and compiled modes (§10).

### 16. External Code Integration Obsolete -- ADDRESSED
Mac Toolbox / Pascal / 68K integration is irrelevant on modern Apple platforms. **Status: replaced.** Phograph targets Swift interop with type marshaling, bidirectional calling, and optional platform API sections (§14).

### 17. Modal I/O Primitives -- ADDRESSED
`show`, `ask`, `answer` were blocking modal dialogs incompatible with modern UI and iOS. **Status: replaced.** Phograph uses `log`/`inspect` for debug output and canvas-based `alert`/`prompt`/`confirm` for user interaction (§6, §12.8).

### 18. No Networking -- ADDRESSED
No primitives for HTTP, JSON, or network communication. **Status: replaced.** Phograph adds `http-get`, `http-post`, `http-request`, `json-parse`, `json-encode`, and related primitives with async Future returns (§6).

### 19. Single Inheritance Only -- ADDRESSED
No mechanism to compose behavior from multiple sources without deep inheritance chains. **Status: replaced.** Phograph adds protocols (§9) -- named sets of method signatures that any class can conform to regardless of its inheritance chain. A class can conform to multiple protocols while inheriting from one superclass.

### 20. No First-Class Method References -- ADDRESSED
The inject construct uses stringly-typed dispatch (pass a method name as a string). No validation, no captured state, no type safety. **Status: replaced.** Phograph adds method references (§5) -- first-class values that can be passed on wires, stored in attributes, and invoked with `call`. Bound references capture `self` for closure-like behavior. Inject is retained for dynamic cases.

### 21. Fixed Arity, No Optional Parameters -- ADDRESSED
Every method had a fixed number of inputs with no defaults. This forced proliferation of methods with slight variations. **Status: replaced.** Phograph adds optional inputs with default values and variadic inputs (§3).

### 22. No Pattern Matching -- ADDRESSED
The case system was a chain of imperative guards. No type matching, value matching, or destructuring. **Status: replaced.** Phograph adds type patterns, value patterns, list destructuring `(head | tail)`, and dict destructuring `{key1, key2, ...rest}` on case inputs (§9).

### 23. No Reactive Observation -- ADDRESSED
No mechanism to react to data changes. UI updates were entirely manual. **Status: replaced.** Phograph adds observable attributes, the `observe` primitive, and reactive bindings (`bind`, `bind-two-way`) that automatically propagate data changes to UI shapes (§9).

### 24. Cryptic Constructor Name -- ADDRESSED
The constructor was named `<<>>`. **Status: replaced.** Renamed to `init` (§9).

### 25. Incomplete String Primitives -- ADDRESSED
Missing split, replace, trim, case conversion, and other basic string operations. **Status: replaced.** Phograph adds `string-split`, `string-replace`, `string-trim`, `uppercase`, `lowercase`, `string-contains?`, `string-starts-with?`, `string-ends-with?`, `char-at`, `string-repeat` (§6).

### 26. Missing Type Introspection -- ADDRESSED
Could check `integer?` but not get the class of an object or test protocol conformance. **Status: replaced.** Phograph adds `type-of`, `class-of`, `instance-of?`, `responds-to?`, `conforms-to?` (§6).

### 27. `log` Name Collision -- ADDRESSED
Debug output primitive and natural logarithm both named `log`. **Status: replaced.** Debug output is `log`; logarithm is `ln` (natural), `log10`, `log2` (§6).

### 28. Obsolete Compiler Target -- ADDRESSED
Original compiler targeted 68K Motorola. **Status: replaced.** Phograph compiler targets ARM64 via Swift intermediate, producing universal macOS binaries and iOS .ipa bundles (§13).

### 29. No Execution Sequencing Beyond Synchro Links -- ADDRESSED
Synchro links were the only way to order side-effectful operations, but they were disconnected from the data model and hard to read. **Status: replaced.** Phograph adds execution wires (§2) -- first-class dashed wires with arrow that enforce sequencing. Execution pins on operations make side-effect ordering explicit and visible.

### 30. No Implicit Parallelism -- ADDRESSED
Original Prograph described the dataflow model as "inherently parallelizable" but the implementation was single-threaded. **Status: replaced.** Phograph compiles independent graph branches to concurrent Swift tasks automatically (§13). The graph structure IS the parallelism specification.

### 31. All-or-Nothing Evaluation -- ADDRESSED
Original Prograph had no incremental evaluation -- any change required full re-execution. **Status: replaced.** Phograph supports incremental evaluation (§13): only affected subgraphs re-evaluate when inputs change. Fill-and-resume from Hazel means edits to a running graph take effect immediately.

### 32. No Debugging Data After Execution -- ADDRESSED
Original Prograph showed values only during step-through debugging. After execution, all wire values were gone. **Status: replaced.** Trace-driven development (§13) records values on every wire. The last execution's data is always visible.

### 33. No Automatic API Binding -- ADDRESSED
Every external function call required a manual bridge declaration. **Status: replaced.** Automatic Swift API surface generation (§14) makes every public Swift type/method/property available as a node without manual binding.

### 34. No Type Visibility -- ADDRESSED
Wire types were invisible -- the programmer had to run the program to discover type mismatches. **Status: replaced.** Pin shapes indicate type category (§2). Pin colors indicate specific types. Visible coercion dots (§2) show implicit conversions. Type annotations propagate through the graph.

### 35. Explicit Loops Required for Collections -- ADDRESSED
Processing a list required explicit loops or ellipsis annotations. **Status: replaced.** Spreads (§8) enable automatic broadcasting: any scalar operation implicitly maps over lists. Most loops are eliminated.

### 36. No Node Discovery Mechanism -- ADDRESSED
The programmer had to know which primitives/methods existed by reading documentation. **Status: replaced.** Fuzzy finder (§5) provides searchable, context-sensitive node creation. Dragging from a pin filters to compatible nodes.

### 37. No Managed Effects / Testability -- ADDRESSED
Side effects were performed directly, making graphs impure and hard to test. **Status: replaced.** Managed effects (§7.3) express side effects as command values executed by the runtime. Graphs are pure and testable.

### 38. Crash Propagation -- ADDRESSED
A runtime error in any operation halted the entire program. **Status: replaced.** Let-it-crash semantics (§7.1) isolate crashes per-node. The errored node shows red; the rest of the graph continues.

### 39. No Binary Data Type -- ADDRESSED
No way to work with raw bytes (file I/O, network payloads, images). **Status: replaced.** Phograph adds a `Data` type with primitives for creation, slicing, encoding/decoding (§4).

### 40. No Date/Time Support -- ADDRESSED
No date, time, or duration primitives. **Status: replaced.** Phograph adds a `Date` type with creation, decomposition, formatting, parsing, arithmetic, and comparison primitives (§4).

### 41. No Algebraic Data Types / Enums -- ADDRESSED
No way to define tagged unions. Pattern matching was limited to duck-typing on primitive types. **Status: replaced.** Phograph adds enum types with variants and optional associated data (§9). Enum variants are the primary target for pattern matching.

### 42. No Higher-Order List Operations -- ADDRESSED
List processing required explicit loops or the ellipsis annotation. No filter, reduce, find, or grouping. **Status: replaced.** Phograph adds `filter`, `reduce`, `map`, `flat-map`, `any?`, `all?`, `find`, `zip`, `enumerate`, `unique`, `group-by` (§6).

### 43. Front Panel / Canvas Confusion -- ADDRESSED
Two UI systems (SwiftUI front panel and canvas scene graph) could confuse users. **Status: addressed.** The relationship is documented: Front Panel is for app-level UI (forms, settings); Canvas is for custom rendering (games, visualizations). Both can coexist in a single class (§9).

### 44. Persistent Variables Not Concurrency-Safe -- ADDRESSED
Global persistent state with concurrent actor access could cause data races. **Status: replaced.** Persistents are mediated by a dedicated PersistentStore actor with serialized access (§10).

### 45. Inconsistent Null/NONE/NULL Usage -- ADDRESSED
Spec used `NULL`, `NONE`, and `null` inconsistently. **Status: replaced.** All "nothing" values are consistently `null` throughout the spec.

---

## 16. References

### Primary Book

- Steinman, Scott B. & Carver, Kevin G. *Visual Programming With Prograph CPX*. Manning Publications / Prentice-Hall, 1995. ISBN 0-13-441163-3 / 1-884777-05-8.

### Academic Papers

- Matwin, S. & Pietrzykowski, T. "PROGRAPH: A preliminary report." *Computer Languages*, Vol. 10, No. 2, 1985, pp. 91-126.
- Pietrzykowski, T., Matwin, S. & Muldner, T. "The programming language PROGRAPH: Yet another application of graphics." January 1983.
- Cox, P.T., Giles, F.R. & Pietrzykowski, T. "Prograph: a step towards liberating programming from textual conditioning." *IEEE Workshop on Visual Languages*, October 1989.
- Cox, P.T. & Pietrzykowski, T. "Using a Pictorial Representation to combine DataFlow and Object-orientation in a language independent programming mechanism." *Proceedings of the International Computer Science Conference*, 1988.
- Cox, P.T. et al. "A Visual Development Environment for Parallel Applications." VL98.
- "Controlled dataflow visual programming languages." ACM, 2011.
- "Static analysis for distributed Prograph." University of Southampton (PhD thesis).

### Magazine Articles

- MacTech Magazine Vol. 8 Issue 1: Prograph 2.5 review
- MacTech Magazine Vol. 10 Issue 3: Prograph CPX review
- MacTech Magazine Vol. 10 Issue 11: Prograph CPX tutorial
- MacTech Magazine Vol. 11 Issue 5: MacApp and Prograph CPX comparison
- *Journal of Visual Languages and Computing*, Vol. 1, 1990: Prograph 2.0 tool review

### Software

- Andescotia Software. Marten IDE 1.6. https://andescotia.com/products/marten/
- Prograph CPX. Macintosh Repository. https://www.macintoshrepository.org/2029-prograph-cpx

### Online Resources

- Wikipedia: Prograph. https://en.wikipedia.org/wiki/Prograph
- Philip T. Cox faculty page, Dalhousie University. https://web.cs.dal.ca/~pcox/
- Noel Rappin: "Prograph." https://noelrappin.com/blog/2018/11/prograph/
- C2 Wiki: PrographLanguage. https://kidneybone.com/c2/wiki/PrographLanguage

### Dataflow Language Influences

The following systems were surveyed for features adopted into Phograph:

- **Unreal Engine Blueprints** -- dual execution+data wires
- **Unity Visual Scripting** -- automatic API node generation, fuzzy finder
- **Houdini** -- inline VEX expressions, digital assets
- **Nuke** -- pull-based demand-driven evaluation
- **TouchDesigner** -- continuous/live evaluation
- **Max/MSP** -- hot/cold inlets
- **vvvv** -- spreads (automatic broadcasting)
- **Cables.gl** -- execution wire model
- **Enso** -- dual visual/textual representation
- **Grasshopper** -- per-node preview toggle
- **Dynamo** -- list@level nesting control
- **LabVIEW** -- front panel/block diagram, error cluster wiring, shift registers, visible coercion dots, automatic parallelism
- **Reaktor** -- snapshot/preset system, multi-level abstraction
- **Scratch/Snap!** -- shape-based type indication on pins
- **Lucid** -- variables as streams, `fby` (followed-by)
- **Lustre/SCADE** -- synchronous tick model, `pre` (previous value)
- **Elm** -- The Elm Architecture (Model/Update/View), managed effects, no runtime exceptions
- **RxSwift/Combine** -- reactive operators as nodes, backpressure
- **Swift Concurrency** -- structured concurrency mapping, actors, Sendable checking
- **Kotlin Flow** -- cold-by-default evaluation, StateFlow/SharedFlow
- **Erlang/Elixir** -- let-it-crash + supervision, hot reloading, process isolation
- **Darklang** -- trace-driven development
- **Hazel** -- typed holes, always-runnable graphs, fill-and-resume
- **Origami** -- physics-based animation nodes, device mirroring
- **SwiftUI** -- property wrapper pin types, view modifier chains, environment
- **React** -- hooks as node design pattern, dependency-driven re-evaluation
