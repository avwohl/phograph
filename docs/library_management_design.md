# Library Management Design

How Phograph discovers, loads, and integrates external primitive libraries.


## Overview

Phograph ships with a built-in set of primitives (+, -, *, /, log, etc.). The
library system extends this with user-installed and third-party primitive
packages. Each library is a self-contained directory that bundles a native
dynamic library (.dylib) with a JSON manifest describing the primitives it
provides.


## Library Format

A library is a directory with this structure:

    my_library/
        library.json        manifest (required)
        my_library.dylib    compiled native code (required)
        README.md           optional documentation

The manifest (library.json):

    {
      "name": "my_library",
      "version": "1.0.0",
      "description": "Useful extra primitives",
      "author": "Jane Doe",
      "min_engine_version": "0.1.0",
      "primitives": [
        {
          "name": "http-get",
          "num_inputs": 1,
          "num_outputs": 1,
          "description": "Fetch a URL and return the response body"
        },
        {
          "name": "http-post",
          "num_inputs": 2,
          "num_outputs": 1,
          "description": "POST data to a URL"
        }
      ]
    }

Fields:
  name               Unique library identifier (lowercase, hyphens OK)
  version            Semantic version string
  description        Short human-readable summary
  author             Optional author name or org
  min_engine_version Minimum Phograph engine version required
  primitives         Array of primitive descriptors


## Discovery

Libraries are found at two locations, checked in order:

  1  App bundle        Phograph.app/Contents/Resources/Libraries/
  2  User directory    ~/.phograph/libraries/

If the same library name appears in both locations the user-directory copy
takes precedence, allowing users to override built-in versions during
development.

On launch the engine scans both paths, reads each library.json, and builds
a catalog of available libraries and their primitives.


## Loading

Libraries are loaded on demand when a project references them (see Project
Integration below). The loading sequence:

  1  Locate the .dylib in the library directory
  2  Call dlopen() to load the dynamic library
  3  Look up the C registration function: phograph_register_library
  4  Call it, passing a pointer to the PrimitiveRegistry
  5  The function registers each primitive with the engine

The registration function signature (C ABI):

    void phograph_register_library(PrimitiveRegistry* registry);

Inside, the library calls registry->register_primitive() for each primitive,
providing the name, input/output counts, and a C function pointer that
implements the primitive's logic.


## IDE Integration

Once a library is loaded its primitives become available everywhere built-in
primitives appear:

  Fuzzy Finder     Cmd+Space shows library primitives alongside built-ins.
                   Library name is shown in gray after the primitive name.

  Right-Click      The "Add Node" context menu gains a submenu for each
                   loaded library.

  Inspector        Selecting a library node shows the library name, version,
                   and primitive description in the inspector panel.

  Sidebar          A "Libraries" section in the class browser lists loaded
                   libraries and their primitives.


## Project Integration

The project JSON gains a top-level "libraries" array listing required
libraries by name and version:

    {
      "name": "My Project",
      "libraries": [
        { "name": "http_utils", "version": "1.0.0" },
        { "name": "json_tools", "version": "2.1.0" }
      ],
      "sections": [ ... ]
    }

When a project is opened the engine:
  1  Reads the libraries array
  2  Resolves each library from the discovery paths
  3  Loads each library via dlopen + registration
  4  Reports any missing or version-incompatible libraries to the user


## Built-in vs External

Built-in primitives are compiled directly into the engine binary. They are
always available and cannot be removed. External libraries are optional and
loaded dynamically.

If a project references a library that is not installed, the IDE shows a
warning banner with a list of missing libraries and instructions for
installing them.


## Security

Dynamic libraries execute native code with full process privileges. To
mitigate risk:

  Sandboxing       macOS App Sandbox limits file and network access. Library
                   code inherits the app's sandbox profile.

  Code Signing     Libraries distributed through official channels must be
                   code-signed. The engine verifies the signature before
                   dlopen.

  User Consent     The first time a project references a new library the IDE
                   shows a confirmation dialog naming the library, its author,
                   and the primitives it provides.

  Quarantine       Libraries downloaded from the internet are quarantined by
                   macOS Gatekeeper and must be explicitly trusted.


## Versioning

Libraries follow semantic versioning (major.minor.patch). The engine checks
compatibility as follows:

  Exact match      If the project specifies version "1.2.3" the engine looks
                   for exactly that version first.

  Compatible       If exact match is not found the engine accepts any version
                   with the same major version and equal-or-higher minor
                   (e.g. project wants 1.2.0, installed is 1.3.1 -> OK).

  Incompatible     Different major versions are never substituted. The user
                   is prompted to install the correct version.


## Distribution

Libraries can be distributed as:

  1  Zip archive     my_library.zip containing the library directory.
                     User extracts to ~/.phograph/libraries/.

  2  Git repository  Clone into ~/.phograph/libraries/. Supports easy updates
                     via git pull.

  3  Future: registry  A central package registry (like npm or PyPI) where
                     users can search, install, and update libraries from the
                     IDE.


## Future: Python Bridge

Many valuable algorithms are available as Python packages (see
top_100_python_libraries.md). A future Python bridge library would:

  1  Embed a Python interpreter (via libpython or PyO3)
  2  Provide a generic "py-call" primitive that invokes a Python function
  3  Handle type conversion between Phograph values and Python objects
  4  Ship adapter libraries that wrap popular Python packages (numpy, pandas,
     scikit-learn, etc.) as Phograph primitive sets

This would let Phograph users access the entire Python ecosystem as visual
dataflow nodes without writing Python code. Each adapter library would:

  a  Declare primitives in library.json (e.g. "np-array", "pd-read-csv")
  b  Implement them in the .dylib by calling into the embedded Python
  c  Convert results back to Phograph types (lists, dicts, numbers, strings)

Performance-critical operations would stay in native code while the Python
bridge handles the long tail of domain-specific functionality.
