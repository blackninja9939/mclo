---
name: running-benchmarks
description: "Use when running, executing, or analysing Google Benchmark microbenchmarks for the library. Covers building with BUILD_BENCHMARKS, running, filtering, and output formats."
---

# Running Benchmarks

## Prerequisites

The project must be configured and built first. Benchmarks are enabled via `BUILD_BENCHMARKS` (CMake standard variable). See the **cmake-config** skill for configuring and the **compile-files** skill for building.

## Configure with Benchmarks Enabled

If not already configured:

```powershell
cmake --preset <preset> -DBUILD_BENCHMARKS=ON
```

The default presets already enable benchmark dependencies via vcpkg, but the CMake option `BUILD_BENCHMARKS` defaults to `OFF` and must be passed explicitly.

## Build the Benchmarks

```powershell
cmake --build build/<preset> --target benchmarks
```

## Run All Benchmarks

```powershell
./build/<preset>/benchmarks.exe
```

## Filter Benchmarks

Run only benchmarks matching a regex:

```powershell
./build/<preset>/benchmarks.exe --benchmark_filter="BM_Hash.*"
```

## Output Formats

### Console (default)

```powershell
./build/<preset>/benchmarks.exe
```

### JSON output

```powershell
./build/<preset>/benchmarks.exe --benchmark_format=json
```

### CSV output

```powershell
./build/<preset>/benchmarks.exe --benchmark_format=csv
```

### Save to file

```powershell
./build/<preset>/benchmarks.exe --benchmark_out=results.json --benchmark_out_format=json
```

## Repetitions and Statistics

Run multiple iterations for statistical reliability:

```powershell
./build/<preset>/benchmarks.exe --benchmark_repetitions=5 --benchmark_report_aggregates_only=true
```

## Important Notes

- Always run benchmarks in **Release** or **RelWithDebInfo** — debug builds are not representative.
- Use `--benchmark_min_time=1` to increase minimum benchmark time for noisy benchmarks.
