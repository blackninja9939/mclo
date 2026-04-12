---
name: writing-documentation
description: "Use when writing or modifying Doxygen documentation comments in C++ headers. Covers tag usage, when to use @brief vs @details, documenting templates, warnings, and style conventions."
---

# Writing Documentation

## Style

Use `///` (triple-slash) for all documentation comments. Do not use `/** */` block style.

Use the ampersand form of Doxygen tags (e.g., `@brief`, `@param`) rather than the backslash form (e.g., `\brief`, `\param`).

## What to Document

All public API in headers: classes, structs, functions, type aliases, concepts, constants, macros, etc, should be documented. Anything a user of the library could use in their own code should be documented.

Unless they have non-obvious semantics that would benefit from documentation for maintainers **do not document:**
- `detail` namespace internals.
- Private member functions or data members.
- Code in `.cpp` files (implementation details, not public API).

## Placement

Place the documentation comment on the line(s) immediately before the declaration it documents leaving a blank line between it and the previous declaration. For example:

```cpp
/// @brief The number of elements in the container.
std::size_t m_size = 0;

/// @brief The capacity of the container.
std::size_t m_capacity = 0;
```

```cpp
/// @brief Description of a function
void my_function( int param );

/// @brief Description of a class
class my_class
{
public:
  /// @brief Description of a member function
  void member_function();
};
```

If adding a piece of documentation to an existing declaration, insert it in the appropriate place in the existing comment block, maintaining the order of tags.

## Tags

### `@brief`

Always present. A single concise sentence describing what the entity is or does:

```cpp
/// @brief A lock-free work-stealing deque for managing data between multiple threads.
```

Keep it short enough to be useful in summaries and IDE tooltips.

### `@details`

Extended explanation — usage patterns, algorithmic notes, implementation rationale, thread-safety guarantees. Use when the brief alone is insufficient:

```cpp
/// @brief A flyweight string that stores a single copy of each unique string in a shared pool.
/// @details This class is useful when you have a large number of strings that are mostly the same,
/// and you want to reduce memory usage by only allocating a single copy of each unique string.
/// Reading the string requires no locking as it is a single pointer dereference.
```

Omit `@details` when the brief fully covers the intent.

### `@tparam`

Document each template parameter separately. State what it represents and any constraints:

```cpp
/// @tparam T The type of elements stored in the deque. Must be trivially copyable and destructible.
/// @tparam Domain The domain of the flyweight, used to separate different shared string pools.
```

### `@param`

Document each function parameter:

```cpp
/// @param capacity The initial capacity of the deque. Will be rounded up to the next power of two.
```

### `@return`

Document the return value, including what it represents and edge cases:

```cpp
/// @return The popped value, or std::nullopt if the deque is empty.
```

### `@warning`

Constraints, ownership rules, misuse risks, or thread-safety caveats that the caller must be aware of:

```cpp
/// @warning Can only be called by the thread owning the deque.
/// @warning It is the user's responsibility to ensure that any string views passed with
/// the assume_static tag are valid for the lifetime of the program.
```

Use `@warning` when violating the constraint leads to undefined behaviour or data corruption.
If it's just a performance note or minor caveat, prefer `@note`.
If it's a warning about a specific precondition or postcondition, prefer `@pre` / `@post`.

### `@note`

Supplementary information that is useful but not critical:

```cpp
/// @note Old storage is only deleted on destruction of the deque.
```

### `@pre` / `@post`

Document preconditions and postconditions when they are not obvious from the signature:

```cpp
/// @pre The container must not be empty.
/// @post The size is reduced by one.
```

### `@see`

Cross-reference related classes, functions, or pages:

```cpp
/// @see basic_static_string
```

### `@throws`

Document exceptions a function may throw and the conditions under which they are thrown:

```cpp
/// @throws std::out_of_range If index >= size().
```

### `@deprecated`

Mark entities that should no longer be used, with guidance on the replacement:

```cpp
/// @deprecated Use new_function() instead.
```

## Inline Formatting

### `@c`

Renders the next word in a monospace/code font. Use for type names, values, or keywords that appear in prose:

```cpp
/// @brief Returns @c true if the container is empty.
/// @details Uses @c std::uncaught_exceptions() to detect exception unwinding.
```

Only affects a single word. For multi-word expressions, use backtick-style formatting or repeat `@c` per word.

### `@p`

Renders the next word in a monospace/code font, semantically marking it as a parameter name. Use when referring to a function or template parameter within documentation text:

```cpp
/// @brief Inserts @p value at the given @p index.
/// @warning @p F must be nothrow invocable.
```

Functionally identical to `@c` in output, but conveys that the word is a parameter.

## Code Examples

### `@code` / `@endcode`

Use `@code` and `@endcode` to embed inline code examples within a documentation comment. Place them after `@details` (or `@brief` if there is no `@details`). The block is rendered as a syntax-highlighted code snippet:

```cpp
/// @brief RAII guard that invokes a callable on destruction.
/// @details Ensures cleanup runs when leaving a scope.
/// @code
/// void example()
/// {
///     auto* handle = acquire();
///     mclo::scope_exit guard( [&]() noexcept { release( handle ); } );
///     do_work( handle );
/// }
/// @endcode
```

Do **not** use `@example` for inline snippets — `@example` is a Doxygen command that references an external example file.

## Tag Order

When multiple tags appear, use this order:

1. `@brief`
2. `@details`
3. `@code` / `@endcode`
4. `@tparam` (in declaration order)
5. `@param` (in declaration order)
6. `@return`
7. `@throws`
8. `@pre` / `@post`
9. `@note`
10. `@warning`
11. `@see`
12. `@deprecated`

## Classes vs Functions

- **Classes/structs**: Document at the class level with `@brief`, `@details`, `@tparam`, `@warning` as needed. Individual member functions get their own doc blocks.
- **Functions**: Document with `@brief`, `@param`, `@return`, and any relevant notes/warnings.
- **Overloads**: Each overload should have its own documentation.

## Reference

For the full list of Doxygen commands, see the [Doxygen Special Commands reference](https://www.doxygen.nl/manual/commands.html).
