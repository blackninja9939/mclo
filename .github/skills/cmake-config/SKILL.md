---
name: cmake-config
description: "Use when configuring, reconfiguring, or modifying the CMake build system for the project. Covers presets, options, adding source files, and when reconfiguration is needed."
---

# CMake Configuration

## Project Structure

| CMakeLists.txt             | Builds                       |
|----------------------------|------------------------------|
| Root `CMakeLists.txt`      | Project options, subdirs     |
| `source/CMakeLists.txt`    | `mclo` library target        |
| `tests/CMakeLists.txt`     | `tests` executable           |
| `benchmarks/CMakeLists.txt`| `benchmarks` executable      |

## Configure Presets

Use the **CMake Tools extension** (`Build_CMakeTools` tool) to configure and build. It automatically handles developer-environment setup (compiler, Ninja, vcpkg) that is **not** available in a plain terminal.

If you must configure from the terminal, use a **Developer PowerShell for VS** — a plain PowerShell will not have `cmake`, `cl.exe`, or `ninja` on PATH.

```powershell
cmake --preset <preset>
```

| Preset                  | Compiler | Config          |
|-------------------------|----------|-----------------|
| `msvc-debug`            | MSVC     | Debug           |
| `msvc-release`          | MSVC     | Release         |
| `msvc-relwithdebinfo`   | MSVC     | RelWithDebInfo  |
| `clang-debug`           | Clang    | Debug           |
| `clang-release`         | Clang    | Release         |
| `clang-relwithdebinfo`  | Clang    | RelWithDebInfo  |

All presets use Ninja, target x64, and use vcpkg for dependency management.

Build output goes to `build/<preset>/`.

## CMake Options

| Option             | Default | Effect                              |
|--------------------|---------|-------------------------------------|
| `BUILD_TESTING`    | `ON`    | Build the test executable           |
| `BUILD_BENCHMARKS` | `OFF`   | Build the benchmark executable      |

## When to Reconfigure

You must reconfigure when:

- Adding or removing source files from any `CMakeLists.txt`
- Changing CMake options (`-DBUILD_BENCHMARKS=ON`)
- Modifying `CMakePresets.json`
- **After a rebase or merge** (e.g. `git rebase`, `git merge`, `git pull`) — upstream changes frequently add or remove source files, requiring reconfiguration

To reconfigure, run the VS Code command `cmake.configure` (via `run_vscode_command`). If the build directory is corrupted (e.g. missing `CMakeFiles/rules.ninja`), delete `build/<preset>/` and reconfigure from scratch.

You do **not** need to reconfigure when:
- Editing existing `.cpp` or `.hpp` files (Ninja tracks these automatically)
- Adding new header files in `include` (they are found via the include directory, not listed in `add_library()`)
- Changing code within existing source files

## Adding Source Files

Add the file path to `add_library()` in `source/CMakeLists.txt`:

```cmake
add_library( ${LIBRARY_TARGET_NAME}
    "existing/file.cpp"
    "new/file.cpp"          # Add here
)
```

Header-only additions in `include/mclo/` do not need to be listed — they are found via the include directory.

For adding test or benchmark files, see the **writing-unit-tests** and **writing-benchmarks** skills.

## Compile Options

The `mclo_compile_options` interface target holds internal warning flags and conformance options. It is linked `PRIVATE` to all targets so it does not leak to consumers. The library target exports `cxx_std_20` as a `PUBLIC` compile feature.
