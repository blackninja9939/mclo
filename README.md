# mclo

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![CMake](https://img.shields.io/badge/build-CMake-064F8C.svg)

A modern C++20 utility library providing a broad collection of containers, algorithms, numeric utilities, hashing, randomness, threading primitives, and more. It complements the standard library with well-tested building blocks under a single `mclo::` namespace.

It grew out of code written for game development, so the components lean towards low overhead, high performance, and being hard to mis-use.

> **Stability:** This is an early-stage project (version 0.1.0). Nothing is promised stable, the API may change at any time without notice.

## Features

- **C++20** throughout, supporting MSVC, Clang, and GCC.
- Header-based public API under `include/mclo/`, with a small compiled core library for components that need translation units (SIMD string ops, hashing, RNG, threading, platform).
- Extensively unit tested with [Catch2](https://github.com/catchorg/Catch2) and benchmarked with [Google Benchmark](https://github.com/google/benchmark).
- Distributed as a CMake package with an installable `mclo::mclo` target.

## What's inside

The public API is organised by domain under `include/mclo/`, each directory grouping a related family of components: `container/`, `enum/`, `hash/`, `memory/`, `numeric/`, `random/`, `strong_typedef/`, `string/`, `threading/`, and `utility/`. Browse the headers for the full set, a few highlights from each:

- **Containers** - `bitset`, `dynamic_bitset`, `small_vector`, `dense_slot_map`, and packed integer storage.
- **Enum** - `enum_map`, `enum_set`, and `enum_range` for treating enums as keys and iterables.
- **Hash** - a single streaming hash API where you pick the hasher (fnv1a, murmur3, rapidhash, xxhash) and append values to it, decoupling how types hash from which algorithm runs.
- **Memory** - value-semantic `indirect` / `polymorphic`, `copy_on_write`, `tagged_ptr`, and `intrusive_ptr`.
- **Numeric** - `fixed_point`, `log2` / `pow2` helpers, and checked/saturated/overflowing math.
- **Random** - `chacha` and `xoshiro` generators you can use directly, plus `random_generator`, a wrapper around any generator for convenient RNG operations.
- **Strong typedef** - compose strong type aliases from mixins, opting into only the operations a type should support.
- **String** - case-insensitive operations, efficient concat/join/append, and `string_flyweight`.
- **Threading** - `atomic_shared_ptr`, `spin_mutex`, instanced thread locals, and a work-stealing deque.
- **Utility** - `expected`, `small_optional`, `state_machine`, and `uuid`.

## Requirements

- A C++20 compiler (MSVC, Clang, or GCC).
- CMake 3.28+.
- [vcpkg](https://vcpkg.io) for dependency management (`xsimd`, `xxhash`, plus `catch2` for tests and `benchmark` for benchmarks).

## Building

The project uses CMake presets with the vcpkg toolchain. Set `VCPKG_ROOT` to your vcpkg installation, then configure and build a preset:

```sh
cmake --preset msvc-release
cmake --build --preset msvc-release
```

Available presets: `msvc-debug`, `msvc-release`, `msvc-relwithdebinfo`, `clang-debug`, `clang-release`, `clang-relwithdebinfo`.

### Tests

```sh
ctest --preset msvc-release
```

### Benchmarks

Benchmarks are off by default, enable them with `-DBUILD_BENCHMARKS=ON` at configure time. The presets already enable the `benchmarks` vcpkg feature.

## Using the library

After installing, consume it from CMake:

```cmake
find_package(mclo CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE mclo::mclo)
```

The library requires C++20 (`cxx_std_20`) for consumers and links its public dependencies (`xsimd`, `xxHash`) transitively.

## License

mclo is released under the [MIT License](LICENSE.txt). It uses third-party components, see [licenses/THIRD_PARTY_NOTICES.md](licenses/THIRD_PARTY_NOTICES.md) for details.