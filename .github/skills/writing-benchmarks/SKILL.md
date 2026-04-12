---
name: writing-benchmarks
description: "Use when writing, adding, or modifying Google Benchmark microbenchmarks for the library. Covers benchmark structure, parameterisation, optimization barriers, and registration."
---

# Writing Benchmarks

## Framework

Google Benchmark. Benchmarks link against `benchmark::benchmark` and `benchmark::benchmark_main` which provides `main()` automatically.

## File Conventions

- Benchmark files go in `benchmarks/` named `<component>_benchmarks.cpp` (or `<component>_benchmark.cpp`).
- New benchmark files must be added to the `add_executable(benchmarks ...)` list in `benchmarks/CMakeLists.txt`.
- Benchmarks are only built when `BUILD_BENCHMARKS=ON`.

## Required Include

```cpp
#include <benchmark/benchmark.h>
```

## Benchmark Structure

```cpp
void operation_name( benchmark::State& state )
{
    // Setup outside the loop
    for ( auto _ : state )
    {
        auto result = operation_under_test();
        benchmark::DoNotOptimize( result );
    }
}
BENCHMARK( operation_name );
```

- **Timing loop**: `for ( auto _ : state )` — only code inside this loop is timed.
- **Setup**: Place setup that is constant between runs (e.g. allocating test data, seeding RNG) before the loop. Setup that varies per iteration or is part of the operation being measured must remain inside the timing loop.

## Preventing Optimization

### `DoNotOptimize`

Forces the compiler to keep a value in a register or memory, preventing it from discarding the result of a computation. The argument **must not be `const`** — pass it as a non-const lvalue.

Use it on the final result of any computation you are benchmarking:

```cpp
for ( auto _ : state )
{
    auto result = compute_something();
    benchmark::DoNotOptimize( result );
}
```

Note: `DoNotOptimize` does not prevent optimizations *within* the expression — only ensures the result is kept. The compiler may still simplify the expression itself.

### `ClobberMemory`

Forces the compiler to perform all pending writes to global memory. Must be used together with `DoNotOptimize` when the memory being written is managed by block-scope variables (e.g. a `std::vector` on the stack), since the compiler cannot see that their memory has been "escaped" otherwise.

```cpp
for ( auto _ : state )
{
    std::vector<int> v;
    v.reserve( 1 );
    auto data = v.data();
    benchmark::DoNotOptimize( data );  // Escape the pointer
    v.push_back( 42 );
    benchmark::ClobberMemory();        // Force the write to be visible
}
```

## Parameterised Benchmarks

Access parameters via `state.range(0)`, `state.range(1)`, etc.

### Single argument with `Arg`

```cpp
BENCHMARK( my_benchmark )->Arg( 8 )->Arg( 64 )->Arg( 512 );
```

### Range with multiplier

`Range( min, max )` generates arguments in multiples of 8 by default. Use `RangeMultiplier` to change the step:

```cpp
// Generates: 8, 16, 32, 64, ..., 8192
BENCHMARK( my_benchmark )->RangeMultiplier( 2 )->Range( 8, 8 << 10 );
```

### Dense range

```cpp
// Generates: 0, 128, 256, 384, ..., 1024
BENCHMARK( my_benchmark )->DenseRange( 0, 1024, 128 );
```

### Multiple arguments with `Args`

Access via `state.range(0)`, `state.range(1)`:

```cpp
BENCHMARK( my_benchmark )->Args( { 1024, 128 } )->Args( { 4096, 512 } );
```

### Multiple ranges

Generates the cartesian product of ranges:

```cpp
BENCHMARK( my_benchmark )->Ranges( { { 1 << 10, 8 << 10 }, { 128, 512 } } );
```

### Custom parameter function

For complex parameterisation, use `Apply`:

```cpp
void custom_args( benchmark::internal::Benchmark* b )
{
    b->RangeMultiplier( 2 )->Range( 4, max_size );
}
BENCHMARK( my_benchmark )->Apply( custom_args );
```

## Template Benchmarks

Use function templates for comparing implementations:

```cpp
template <typename Impl>
void hash_benchmark( benchmark::State& state )
{
    for ( auto _ : state )
    {
        Impl hasher;
        auto result = hasher.compute( data );
        benchmark::DoNotOptimize( result );
    }
}
BENCHMARK( hash_benchmark<mclo::fnv1a_hasher> );
BENCHMARK( hash_benchmark<mclo::rapidhash> );
```

## Best Practices

- Keep benchmark setup outside the timing loop.
- Use deterministic data (e.g. seeded RNG with fixed seed like `42`) for reproducible results.
- Use `DoNotOptimize` on the final result of every benchmarked computation (must be non-const).
- Use `ClobberMemory` after in-place mutations to block-scope memory.
- Place benchmarks in an anonymous namespace.

## After Writing Benchmarks

Always compile and run new or modified benchmarks to confirm they pass. See the **compile-files** and **running-benchmarks** skills for commands.
