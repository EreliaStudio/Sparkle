# Instructions for agents working with Sparkle documentation

This file applies to the `docs/` subtree. Agents changing any part of Sparkle must also follow the repository-wide [AGENTS.md](../AGENTS.md). This documentation is written for users and contributors, not only for AI agents.

## Required reading

Before proposing an architectural design, editing implementation code, reviewing a PR, or authoring public API documentation:

1. Read [Sparkle's development and code review guide](ai-development-guidelines.md) and the applicable [coding style](contributing/coding-style.md).
2. Read the [feature workflow](contributing/feature-workflow.md) before creating an issue, design, branch, implementation, or PR.
3. Read the relevant [architecture](architecture/README.md), [API reference](reference/README.md) and [documentation rules](contributing/writing-documentation.md).
4. Read relevant current public headers (`include/`), implementations (`src/`), and tests (`tests/TU/`). Never rely on a historical plan as proof of current behavior.
5. Read the feature's GitHub issue (including its latest accepted plan and decisions) before writing or changing code. If no issue/accepted design exists, return to the design phase rather than inventing an implementation.

Do not read every document indiscriminately: consult the pages and source files relevant to the task, plus these foundational instructions.

## Feature development lifecycle

The standard workflow for a new feature or significant fix is **Issue → approved design → branch → implementation, documentation and tests → PR → CI and review → merge**.

### 1. Issue and design: no implementation branch yet

- Create a GitHub issue identifying the user-visible problem or feature and why it is needed.
- Refine **the issue body itself** into the detailed, up-to-date design. The issue is the living specification, replacing the earlier practice of maintaining a long design-only PR.
- Discuss alternatives with the maintainer and record accepted decisions, rejected alternatives, expected user-facing code, public API changes, ownership/lifetime, failure semantics, threading, scope, test plan and acceptance criteria when applicable.
- Do not invent unresolved consequential decisions. Ask focused questions; update the issue after each accepted decision.
- Begin implementation only after the maintainer accepts the design.

### 2. Branch and development

- Create a dedicated feature branch from the agreed target branch after approval of the design.
- Implement only the approved scope. In the same branch, update **implementation code, relevant public documentation, and automated tests** appropriate to the change.
- Favor tests for observable public API behavior, failures, edge cases and lifecycle. Use appropriate memory-safety, stress and concurrency coverage when relevant.
- If implementation exposes a design gap, return to the issue, update the design and obtain agreement before changing the contract.
- Maintain the documentation alongside each API change. Explain how to use the feature and, when important, how it interacts with other subsystems; include focused Mermaid diagrams with Markdown navigation links.
- A documentation-only or test-only task need not introduce artificial production code. The three areas must be addressed as applicable, not changed for the sake of touching files.

### 3. Pull request and validation

- Open a PR from the feature branch to its intended target and link the design issue. The PR describes implementation and verified deviations, not a second independent architecture plan.
- Check required CI: compilation, formatting, functional tests and relevant specialist checks. Wait for the applicable pipeline to finish; diagnose and fix failures; re-run after changes.
- Do not claim a check passed without reading its status for the actual PR head. CI success alone is not approval: request maintainer review before merging.
- Keep the issue synchronized with decisions made during PR review. Merge only when agreed review and required checks have passed.

### 4. Integration tests, when justified

- Some behaviors span multiple components, processes or systems. For these, consider an additional **integration or end-to-end validation stage**, as done conceptually in Erelia's pipeline.
- Such a stage is **not a mandatory new pipeline requirement today** and should not be created preemptively. Propose it in the issue when unit or narrow integration tests cannot reasonably validate the user-visible behavior.
- Specify observable scenarios, environments, reproducibility, acceptable duration and failure reporting before introducing a new job.
- Avoid converting every feature into a slow whole-application test. Prefer the smallest test level that demonstrates the required contract.

## Scope and quality boundaries

- Favor minimal coherent code with explicit decisions and clear ownership. No speculative defensive frameworks, retries or recovery states.
- Keep test fixtures and sample code aligned with the actual API. If tests cannot be run, say so; never invent success.
- Preserve old documentation links where practical; do not delete engineering history silently.
- Follow `.clang-format` for C++ changes and write user-facing docs in English.
- Never commit, merge, or mark an architectural decision approved merely because an AI considers it preferable; the maintainer validates the design.
