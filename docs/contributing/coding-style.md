# Sparkle C++23 coding style

The **canonical detailed guide** is [AI Development and Code Review Guide](../ai-development-guidelines.md), especially section 7. This page is a short contributor entry point, not a competing rule set. Follow the repository's [`.clang-format`](../../.clang-format).

## File organization

- Prefer one meaningful class per file, with cohesive nested helpers.
- Non-template declarations belong in `.hpp`, implementations in `.cpp`. Define templates in headers, not `.tpp` files.
- Put an empty virtual default hook with body `{}` directly in the header. Use `= 0` only if overriding is mandatory; these have different semantics.
- Prefer domain folders and include explicit relevant headers.
- No explanatory comments in new or modified headers; encode meaning in names, types, and policy enums. Put longer explanations here in documentation.

## Naming

| Kind | Convention |
| --- | --- |
| Class, struct, enum/type | `PascalCase` |
| Public function | `camelCase()` |
| Private/protected method | `_camelCase()` |
| Private data member | `_camelCase` |

## Design and behavior

Prefer explicit operations, precise semantic types, `enum class` policies instead of ambiguous booleans, and small coherent responsibilities. Invalid operations should fail explicitly; expected outcomes should use appropriate status/result types. Prefer standard allocation and RAII, not speculative generalizations.

See the [full guide](../ai-development-guidelines.md) for class member order, ownership, exceptions, concurrency, testing and review conventions.
