---
name: writing-unit-tests
description: "Use when writing, adding, or modifying unit tests. Covers test structure, naming, assertions, helper utilities, and how to register new test files."
---

# Writing Unit Tests

## Policy

Everything should be unit tested unless it is totally impossible or redundant. When adding or modifying a component, write tests alongside the code.

## Framework

Catch2 v3. Tests link against `Catch2::Catch2WithMain` which provides `main()` automatically.

## File Conventions

- One test file per component: `<component>_tests.cpp` in `tests/`.
- New test files must be added to the `add_executable(tests ...)` list in `tests/CMakeLists.txt`.
- Test discovery is automatic via `catch_discover_tests(tests)`.
- Helper functions and types needed for tests should be in an anonymous namespace at the top of the file.

## Required Includes

```cpp
#include <catch2/catch_test_macros.hpp>                 // TEST_CASE, CHECK, REQUIRE, SECTION
#include <catch2/catch_template_test_macros.hpp>        // TEMPLATE_LIST_TEST_CASE
#include <catch2/generators/catch_generators.hpp>       // GENERATE()
#include <catch2/matchers/catch_matchers_all.hpp>       // Matchers
```

Only include what you need.

## Test Naming

Use **Given, When, Then** format: `"given state, when action, then outcome"`.

It should be possible to read a test name and understand:
- What preconditions exist
- What operation is being performed
- What the expected outcome is — so a failure immediately tells you what went wrong

Examples:
- `"empty work_stealing_deque, pop, is nullopt"`
- `"slot_map with one element, erase by handle, is empty"`
- `"circular_buffer at capacity, push_back, overwrites oldest"`

Avoid vague names like `"is correct"`, `"is valid"`, or just stating an operation without context.

## Test Structure

Follow **Given / When / Then** in the test body. Separate each block with a blank line:

```cpp
TEST_CASE( "empty circular_buffer, push_back, size is one", "[circular_buffer]" )
{
    // Given
    circular_buffer<int, 4> buf;

    // Push one element
    buf.push_back( 42 );

    // Size reflects the insertion
    CHECK( buf.size() == 1 );
    CHECK( buf.front() == 42 );
}
```

- **Tags**: One or more component names in square brackets, e.g. `"[math][checked_math]"`. Use a general tag plus a specific tag when the component is a sub-area.

## Sections

Use `SECTION` when multiple tests share significant setup code. Each section re-runs the setup above it:

```cpp
TEST_CASE( "slot_map with three elements", "[slot_map]" )
{
    // Given — shared setup
    dense_slot_map<std::string> map;
    auto h1 = map.emplace( "a" );
    auto h2 = map.emplace( "b" );
    auto h3 = map.emplace( "c" );

    SECTION( "erase first, size is two" )
    {
        map.erase( h1 );

        CHECK( map.size() == 2 );
    }

    SECTION( "clear, is empty" )
    {
        map.clear();

        CHECK( map.empty() );
    }
}
```

## Assertions

**Always prefer `CHECK`** over `REQUIRE`. Use `REQUIRE` only when a subsequent operation would crash or be meaningless if the check fails (e.g. null pointer dereference, accessing an empty optional, indexing past size).

Prefer the `_FALSE` variant over negating with `!`.

### Assertion Macro Reference

| Macro | When to use | Include |
|---|---|---|
| `CHECK` / `REQUIRE` | Runtime-only expressions (I/O, `std::random_device`, non-constexpr calls). | `catch2/catch_test_macros.hpp` |
| `CHECK_FALSE` / `REQUIRE_FALSE` | Negated runtime checks. | `catch2/catch_test_macros.hpp` |
| `static_assert` | Type traits and compile-time properties that must always hold. Place at file scope. | — |
| `STATIC_CHECK` / `STATIC_REQUIRE` | Fully `constexpr` expressions that only need compile-time verification (e.g. a `static constexpr` result). | `catch2/catch_test_macros.hpp` |
| `CONSTEVAL_CHECK` / `CONSTEVAL_REQUIRE` | User-defined `constexpr` functions that *could* diverge between compile time and runtime (e.g. `if consteval`, `std::is_constant_evaluated()`). Runs the check at both compile time and runtime. | `"consteval_check.hpp"` |
| `CHECK_ASSERTS` / `REQUIRE_ASSERTS` | Verifies that an expression fires a project assertion macro with a matching message. | `"assert_macros.hpp"` |
| `CHECK_THROWS_AS` / `REQUIRE_THROWS_AS` | Verifies that an expression throws a specific exception type. | `catch2/catch_test_macros.hpp` |
| `CHECK_THAT` / `REQUIRE_THAT` | Expressive checks using matchers (see Matchers below). | `catch2/matchers/catch_matchers_all.hpp` |

### Examples

```cpp
// Runtime-only value — plain CHECK
auto* ptr = map.find( handle );
REQUIRE( ptr != nullptr );    // Dereferencing nullptr below would crash
CHECK( *ptr == "expected" );

// Type traits — static_assert at file scope
static_assert( std::is_nothrow_move_constructible_v<my_type> );
static_assert( std::three_way_comparable<my_type> );

// Fully constexpr value — compile-time check is sufficient
static constexpr auto result = try_parse( "..." );
STATIC_REQUIRE( result.has_value() );
STATIC_CHECK( *result == expected );

// User-defined constexpr function that may diverge — test both paths
CONSTEVAL_CHECK( my_constexpr_func() == expected );
CONSTEVAL_CHECK_FALSE( my_constexpr_func_returning_false() );

// Project assertion macro fires with expected message
#include "assert_macros.hpp"
CHECK_ASSERTS( function_that_should_assert(), "expected assertion message" );
```

In test builds, project assertion macros (`ASSERT`, `PANIC`, etc.) throw exceptions instead of aborting so they can be caught and validated. Functions that are `noexcept` but contain assertions use `MCLO_NOEXCEPT_TESTS` / `MCLO_NOEXCEPT_TESTS_IF` instead — these expand to nothing in test builds, allowing the exception to propagate.

### Matchers

Matchers provide expressive checks via two macros:
- `CHECK_THAT( value, matcher )` — non-fatal, continues on failure
- `REQUIRE_THAT( value, matcher )` — fatal, stops on failure

```cpp
#include <catch2/matchers/catch_matchers_all.hpp>

using namespace Catch::Matchers;
```

It is standard in this project to add `using namespace Catch::Matchers` after includes when a test file uses matchers.

| Matcher                         | Example                                  | Description                                                        |
|---------------------------------|------------------------------------------|--------------------------------------------------------------------|
| `IsEmpty()`                     | `IsEmpty()`           | Range has no elements                                              |
| `SizeIs( n )`                   | `SizeIs( 3 )`                            | Range has exactly `n` elements                                     |
| `Contains( element )`           | `Contains( 42 )`                         | Range contains a specific element                                  |
| `RangeEquals( range )`          | `RangeEquals( expected_vec )`            | All elements match in order                                        |
| `UnorderedRangeEquals( range )` | `UnorderedRangeEquals( expected_vec )`   | Same elements regardless of order                                  |
| `AllMatch( predicate )`         | `AllMatch( Predicate<int>( pred ) )`     | Every element satisfies the predicate                              |
| `AnyMatch( predicate )`         | `AnyMatch( Predicate<int>( pred ) )`     | At least one element satisfies the predicate                       |
| `NoneMatch( predicate )`        | `NoneMatch( Predicate<int>( pred ) )`    | No element satisfies the predicate                                 |
| `StartsWith( str )`             | `StartsWith( "hello" )`                  | String starts with the given prefix                                |
| `EndsWith( str )`               | `EndsWith( "world" )`                    | String ends with the given suffix                                  |
| `ContainsSubstring( str )`      | `ContainsSubstring( "llo wo" )`          | String contains the given substring                                |
| `WithinAbs( target, margin )`   | `WithinAbs( 3.14, 0.01 )`               | `\|value - target\| <= margin` (absolute tolerance)                |
| `WithinRel( target, eps )`      | `WithinRel( 3.14, 0.001 )`              | `\|value - target\| <= eps * max(\|value\|, \|target\|)` (relative tolerance, eps defaults to `epsilon * 100`) |

Matchers can be combined with `&&`, `||`, and `!`:

```cpp
CHECK_THAT( str, StartsWith( "he" ) && EndsWith( "lo" ) );
```

## Template Tests

Use `TEMPLATE_LIST_TEST_CASE` when the same test logic should be verified across multiple types — e.g. testing a container with both a standard allocator and a custom allocator, or testing a numeric function across all integer types. This avoids duplicating identical test bodies.

Use `mclo::meta::type_list` to define the type list:

```cpp
using test_types = mclo::meta::type_list<type_a, type_b>;

TEMPLATE_LIST_TEST_CASE( "default constructed, is empty", "[component]", test_types )
{
    TestType obj;

    CHECK( obj.empty() );
}
```

`TestType` is the alias for the current type being tested. The test is instantiated and run once per type in the list.

Pre-defined type lists for numeric and character types are available in `mclo/meta/type_aliases.hpp`.

## Generators

`GENERATE` produces multiple values and re-runs the test for each one. Useful when the same test logic applies to several inputs:

```cpp
#include <catch2/generators/catch_generators.hpp>

TEST_CASE( "absolute value, negative inputs, returns positive", "[math]" )
{
    auto value = GENERATE( -1, -5, -100 );

    CHECK( std::abs( value ) > 0 );
}
```

Can be combined with `range` and `values`:

```cpp
auto size = GENERATE( range( 1, 10 ) );      // 1 through 9
auto kind = GENERATE( values( { "a", "b" } ) ); // explicit list
```

Each `GENERATE` call creates an independent axis — multiple calls produce the cartesian product of all values.

## Custom StringMakers

When test output needs readable formatting for custom types, specialise `Catch::StringMaker`:

```cpp
template <>
struct Catch::StringMaker<my_type>
{
    static std::string convert( const my_type& value ) { /* ... */ }
};
```

## Global State

Tests should avoid manipulating global state wherever possible. If a test must modify global state, use an RAII guard type so the state is reset when the test completes, regardless of assertion failures.

## After Writing Tests

Always compile and run new or modified tests to confirm they pass. See the **compile-files** and **running-unit-tests** skills for commands.
