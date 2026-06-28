# Third-Party Notices

This project (mclo) is licensed under the MIT License — see [../LICENSE.txt](../LICENSE.txt).

It incorporates, links against, or builds with the third-party components listed
below. Each component's full license text is provided as a sibling `.txt` file in
this folder. Versions track the vcpkg baseline pinned in [../vcpkg.json](../vcpkg.json).

## Vendored source (copied into this repository)

These components have source copied directly into the repository and are therefore
redistributed as part of mclo's own source tree.

| Component | License | File |
|-----------|---------|------|
| rapidhash (in `source/hash/rapidhash.cpp`, modified) | MIT | [rapidhash.txt](rapidhash.txt) |
| MurmurHash3 / PMurHash (in `source/hash/murmur_hash_3.cpp`) | Public domain | [murmurhash3.txt](murmurhash3.txt) |

## Library dependencies (linked into the distributed artifact)

Pulled in via vcpkg directly or transitively.

| Component | License | File |
|-----------|---------|------|
| xsimd | BSD-3-Clause | [xsimd.txt](xsimd.txt) |
| xxHash | BSD-2-Clause | [xxhash.txt](xxhash.txt) |

## Build & test tooling (NOT distributed)

These are used only to build tests and benchmarks; their code is not part of any
distributed mclo artifact, so their licenses impose no redistribution obligation
here. Listed for completeness.

| Component | License | Used by |
|-----------|---------|---------|
| Catch2 | BSL-1.0 | `tests` feature |
| Google Benchmark | Apache-2.0 | `benchmarks` feature |
