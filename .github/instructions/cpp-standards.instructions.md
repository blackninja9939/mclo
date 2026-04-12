---
applyTo: "**/*.{cpp,hpp}"
description: "Use when writing or modifying C++ source and header files. Enforces modern C++20 coding standards, naming conventions, and project-specific patterns for the library."
---

# C++20 Coding Standards

## Language Standard

- Target **C++20**. Use C++20 features freely: concepts, constraints, `std::bit_cast`, three-way comparison (`<=>`), `if constexpr`, `consteval`, designated initializers, ranges.
- Do not use C++23 features directly; use project compatibility macros (e.g., `MCLO_STATIC_CALL_OPERATOR`) when available.

## Naming Conventions

| Element              | Convention                  | Example                          |
|----------------------|-----------------------------|----------------------------------|
| Classes / Structs    | `snake_case`                | `slot_map`, `circular_buffer`    |
| Functions / Methods  | `snake_case`                | `bit_reverse()`, `get_combined()`|
| Template Parameters  | `PascalCase` or single letter | `T`, `Wrapped`, `Tag`, `Buffer` |
| Member Variables     | `m_` prefix + `snake_case`  | `m_ptr`, `m_buffer`, `m_size`    |
| Namespaces           | `snake_case`                | `mclo`, `mclo::detail`           |
| Macros               | `MCLO_` prefix + `UPPER_SNAKE_CASE` | `MCLO_FORCE_INLINE`     |
| Concepts             | `snake_case`                | Same style as types              |

## Header Files

- Use `#pragma once` — never use traditional include guards.
- Place all public API in the `mclo` namespace; internal helpers in `mclo::detail`.
- Keep headers self-contained: each header should compile independently.

## Attributes & Qualifiers

- Mark functions `[[nodiscard]]` when:
  - The call is pointless if the result is ignored (e.g. const member functions, pure free functions).
  - The return value is a resource requiring management (e.g. a raw owning pointer).
  - The return value must be checked (e.g. error codes) and the type itself is not already `[[nodiscard]]`.
  - If all uses of a class/struct should be captured, prefer marking the type itself `[[nodiscard]]` instead of individual functions.
- Mark functions `noexcept` wherever possible. Use conditional noexcept for templates: `noexcept(std::is_nothrow_move_constructible_v<T>)`.
- If a function should be `noexcept` but its body contains any assertion macro (`DEBUG_ASSERT`, `ASSERT`, `ASSUME`, etc.), use `MCLO_NOEXCEPT_TESTS` instead of `noexcept`, or `MCLO_NOEXCEPT_TESTS_IF(condition)` instead of `noexcept(condition)`. In test builds, assertions throw exceptions so they can be validated — these macros expand to nothing in tests to avoid crashing on throw-in-noexcept.
- Use `constexpr` aggressively — constructors, operators, utility functions.

## Concepts & Templates

- Prefer concept-constrained template parameters over unconstrained `typename`.
- Use `requires` clauses for complex constraints.
- Always use `template <typename T>` (not `class`).

## Error Handling

- This codebase does not use exceptions for control flow.
- Prefer `tl::expected` (aliased as `mclo::expected`) for recoverable errors.
- Use the following assertion macros:
  - Conditional: `MACRO( condition, optional message, optional diagnostics... )`
  - Unconditional (`PANIC`, `UNREACHABLE`): `MACRO( optional message, optional diagnostics... )`

| Macro            | Debug          | Release              | Use when                                                                                        |
|------------------|----------------|----------------------|-------------------------------------------------------------------------------------------------|
| `DEBUG_ASSERT`   | Checked        | Compiled out         | Internal invariants that are programmer error — should be caught and fixed before shipping.      |
| `ASSERT`         | Checked        | Checked              | Invariants that are not recoverable but could occur due to invalid user input.                   |
| `ASSUME`         | Checked        | UB optimization hint | Conditions that are impossible by design and provide useful information to the compiler.         |
| `PANIC`          | Always fires   | Always fires         | Unconditional `ASSERT` — same use cases as `ASSERT` but for code paths that should never run.   |
| `UNREACHABLE`    | Always fires   | UB optimization hint | Unconditional `ASSUME` — same use cases as `ASSUME` for code paths that should be unreachable.  |

## Formatting

- Formatting is enforced by `.clang-format` and checked by CI on every push. Do not manually reformat — let clang-format handle it. See the **clang-format** skill for how to run it.

## Documentation

- Use `///` (triple-slash) for documentation comments with Doxygen tags.
- Document public API; internal `detail` code does not require doc comments.
- See the **writing-documentation** skill for tag usage and conventions.

## Compiler Compatibility

- **Compiler vs. platform are distinct** — use compiler detection (`MCLO_COMPILER_MSVC`, `MCLO_COMPILER_GCC_COMPATIBLE`) for compiler-specific behaviour, and platform detection (`MCLO_PLATFORM_WINDOWS`, etc.) for OS-specific behaviour. Do not conflate them.
- **Intrinsics**: Wrap compiler intrinsics in portable functions guarded by `#ifdef` per compiler. Never expose raw intrinsics at call sites.
- **Attributes & extensions**: Use project macros to provide a single name for features with varying compiler spellings (e.g. `MCLO_FORCE_INLINE`, `MCLO_NO_UNIQUE_ADDRESS`, `MCLO_RESTRICT`).
- Never use raw compiler-specific pragmas or attributes directly (except `#pragma once`).
