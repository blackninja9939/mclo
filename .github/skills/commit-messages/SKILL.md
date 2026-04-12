---
name: commit-messages
description: "Use when writing git commit messages. Covers format, heading style, body content, and best practices."
---

# Writing Commit Messages

## Format

```
Short summary of the change (imperative mood, max ~72 chars)

Longer explanation of what changed and why. Wrap lines at ~72 characters.
Focus on the reasoning behind the change, not a line-by-line restatement
of the diff.
```

## Rules

- **Heading**: Imperative mood ("Add X", "Fix Y", not "Added X" or "Fixes Y"). Keep under ~72 characters.
- **Blank line**: Always separate heading from body.
- **Body**: Explain *what* and *why*, not *how* — the diff shows how. Keep it concise — a few sentences to a short paragraph, not a wall of text.
- Omit the body for trivial changes (e.g. typo fixes, formatting-only).
- Do not restate every individual line-level change — summarise the intent.
- Use the body to explain the rationale, design decisions, and any non-obvious consequences of the change.
- If the change is large or complex, consider breaking it into multiple commits with focused scopes.
- Always proofread before committing — check for clarity, grammar, and typos.