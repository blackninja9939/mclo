---
name: running-unit-tests
description: "Use when running, executing, or debugging Catch2 unit tests for the mclo library. Covers CTest, preset-based test execution, filtering, and troubleshooting."
---

# Running Unit Tests

## Prerequisites

The project must be configured and built first. Tests are enabled by default via `BUILD_TESTING` (CMake standard variable). See the **cmake-config** skill for configuring and the **compile-files** skill for building.

## Build the Tests

See the **compile-files** skill. The test target is `tests`:

```powershell
cmake --build build/<preset> --target tests
```

## Run All Tests via CTest

```powershell
ctest --preset <preset>
```

Available test presets:
- `msvc-debug`, `msvc-release`, `msvc-relwithdebinfo`
- `clang-debug`, `clang-release`, `clang-relwithdebinfo`

All test presets output on failure by default.

## Run Tests Directly

Run the test executable for more control:

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

## Verbose Output

```powershell
ctest --preset <preset> --output-on-failure -V
```

## Run a Single CTest Test

```powershell
ctest --preset <preset> -R "test name pattern"
```

## Common Workflow

1. Configure: `cmake --preset msvc-debug`
2. Build: `cmake --build --preset msvc-debug`
3. Test: `ctest --preset msvc-debug`

Or for targeted testing during development:

1. Build tests only: `cmake --build build/msvc-debug --target tests`
2. Run specific tag: `./build/msvc-debug/tests.exe [slot_map]`
