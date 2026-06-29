---
name: running-unit-tests
description: "Use when running, executing, or debugging Catch2 unit tests for the mclo library. Covers building the test executables and running directly via Catch2's command line."
---

# Running Unit Tests

## Policy: Always Run via the Executable, Never CTest

**Never run tests via CTest** (`ctest`, `ctest --preset ...`, etc.). Always build the
module's test executable and run it directly through Catch2's own command line
infrastructure. Running the exe directly executes all of a module's test cases in a
single process, which is far lower overhead than CTest launching a process per test
case, and gives direct access to Catch2's filtering, listing, and reporting flags.

## Prerequisites

The project must be configured and built first. Tests are enabled by default via `BUILD_TESTING` (CMake standard variable). See the **cmake-config** skill for configuring and the **compile-files** skill for building.

## Build the Tests

See the **compile-files** skill. The test target is `tests`:

```powershell
cmake --build build/<preset> --target tests
```

Available presets:
- `msvc-debug`, `msvc-release`, `msvc-relwithdebinfo`
- `clang-debug`, `clang-release`, `clang-relwithdebinfo`

## Run All Tests

Run the test executable directly:

```powershell
./build/<preset>/tests.exe
```

### Filter by Test Name

```powershell
./build/<preset>/tests.exe "test name pattern"
```

### Filter by Tag

```powershell
./build/<preset>/tests.exe [tag]
```

### List Available Tests

```powershell
./build/<preset>/tests.exe --list-tests
```

### List Available Tags

```powershell
./build/<preset>/tests.exe --list-tags
```

### Verbose Output

Show successful assertions too:

```powershell
./build/<preset>/tests.exe --success
```

See `./build/<preset>/tests.exe --help` for the full set of Catch2 command line options.

## Common Workflow

1. Configure: `cmake --preset msvc-debug`
2. Build tests: `cmake --build build/msvc-debug --target tests`
3. Run the tests: `./build/msvc-debug/tests.exe`

Or for targeted testing during development:

1. Build tests only: `cmake --build build/msvc-debug --target tests`
2. Run specific tag: `./build/msvc-debug/tests.exe [slot_map]`
