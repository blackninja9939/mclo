---
name: compile-files
description: "Use when compiling individual files, specific targets, or the full project. Covers Ninja single-file compilation, target builds, and preset-based full builds."
---

# Compiling Files

## Build System

This project uses **Ninja** as the build generator (configured via presets). All build outputs go to `build/<preset>/`.

## Preferred Method — CMake Tools Extension

Use the **CMake Tools extension** (`Build_CMakeTools` tool) for all configure and build operations. It handles developer-environment setup (compiler, Ninja, vcpkg) automatically.

A plain PowerShell terminal does **not** have `cmake`, `cl.exe`, or `ninja` on PATH. If you must use the terminal, open a **Developer PowerShell for VS**.

## Prerequisites

If a new source file has been added to any `CMakeLists.txt`, CMake must be reconfigured before compiling. See the **cmake-config** skill for when and how to reconfigure.

## Full Build

```powershell
cmake --build --preset <preset>
```

## Build a Specific Target

```powershell
cmake --build build/<preset> --target <target>
```

| Target       | What it builds             |
|--------------|----------------------------|
| `mclo`       | The library only           |
| `tests`      | The test executable only   |
| `benchmarks` | The benchmark executable   |

## Compile a Single File

Ninja supports compiling individual translation units without linking. Use the `.obj` path corresponding to the source file:

```powershell
cmake --build build/<preset> --target source/CMakeFiles/mclo.dir/<path>.cpp.obj
```

For example, to compile just `source/string/ascii_string_utils.cpp`:

```powershell
cmake --build build/msvc-debug --target source/CMakeFiles/mclo.dir/string/ascii_string_utils.cpp.obj
```

For test files:

```powershell
cmake --build build/msvc-debug --target tests/CMakeFiles/tests.dir/<file>.cpp.obj
```

This is useful for checking a single file compiles without building the full target.

## Parallel Builds

Ninja builds in parallel by default. To limit jobs:

```powershell
cmake --build build/<preset> -j 4
```

## Common Workflow

1. Edit source files
2. Compile single file to check for errors: `cmake --build build/msvc-debug --target source/CMakeFiles/mclo.dir/path/file.cpp.obj`
3. Build full target: `cmake --build build/msvc-debug --target mclo`
4. Build and run tests: `cmake --build build/msvc-debug --target tests; ctest --preset msvc-debug`
