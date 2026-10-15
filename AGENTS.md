# Instructions for AI contributors to Sparkle

Before generating, modifying, or reviewing Sparkle code, read and apply
[AI Development and Code Review Guide (v2)](docs/ai-development-guidelines.md).

The guide describes the maintainer's architectural philosophy, C++23 conventions,
ownership/lifetime expectations, error handling, concurrency, testing, and PR
review workflow. Treat it as a guide for reasoning about code, not an excuse
to refactor unrelated files.

## Working rules

- Read the current task or pull request's acceptance criteria and relevant source
  before editing. A specific task contract takes precedence over general style.
- Favor explicit behavior, meaningful single-responsibility objects/traits, and
  smaller cohesive designs rather than unnecessary wrappers or speculative
  generic abstractions.
- Ask the maintainer about significant unresolved architecture decisions rather
  than silently choosing between materially different designs.
- Keep changes inside the agreed scope, preserve existing public contracts unless
  an API change is authorized, and report unrelated improvements separately.
- Add appropriate public-API, failure-path, lifecycle, and concurrency tests.
  Do not claim tests or CI passed unless they were run against the changed head.
- Apply the repository's pinned clang-format configuration to changed C++ code.

For PR #29 specifically, the PR description remains authoritative for transport
protocol, handshake, admission, memory budgets, and supported-platform behavior.
This guide does not authorize changing those decisions or merging the PR.
