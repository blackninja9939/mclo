# Third-Party Notices

This project (mclo) is licensed under the MIT License — see [../LICENSE.txt](../LICENSE.txt).

It incorporates, links against, or builds with the third-party components listed
below. Each component's full license text is provided as a sibling `.txt` file in
this folder. Versions track the vcpkg baseline pinned in [../vcpkg.json](../vcpkg.json).

## Library dependencies (linked into the distributed artifact)

Pulled in via vcpkg directly or transitively.

| Component | License | File |
|-----------|---------|------|
| xsimd | BSD-3-Clause | [xsimd.txt](xsimd.txt) |

## Build & test tooling (NOT distributed)

These are used only to build tests and benchmarks; their code is not part of any
distributed mclo artifact, so their licenses impose no redistribution obligation
here. Listed for completeness.

| Component | License | Used by |
|-----------|---------|---------|
| Catch2 | BSL-1.0 | `tests` feature |
| Google Benchmark | Apache-2.0 | `benchmarks` feature |
