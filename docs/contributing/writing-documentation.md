# Writing Sparkle documentation

Maintain documentation in English Markdown under `docs/`. Follow [docs/AGENTS.md](../AGENTS.md) and the [issue-first feature workflow](feature-workflow.md) when documentation accompanies a feature.

Choose the folder by the reader's question:

- `getting-started/`: setup, first usage, and decisions needed to begin.
- `architecture/`: why subsystems exist, what they own, and how they interact.
- `reference/`: the current public contract of individual important classes.
- `guides/`: how to accomplish one task with API usage.
- `contributing/`: contribution style, workflow and quality criteria.
- `internal/`: historical evidence, validation reports and still-relevant legacy material, clearly marked as such.

Keep new feature design and implementation plans in GitHub issues rather than creating new planning documents in `docs/`. When a historical plan is completed or superseded, remove it unless it contains distinctive evidence still worth retaining. Git history preserves prior versions.

## Class reference template

1. Responsibility and `#include` path.
2. Relationships and ownership/lifetime assumptions.
3. Important methods, enums, outcomes and constraints.
4. A small correct usage example or a link to a focused guide.
5. Links to source, relevant tests and neighboring classes.

Keep the API's current behavior authoritative; inspect the feature's target branch source and tests. Mark any code as illustrative if it has not been compiled.

## Mermaid

Keep Mermaid diagrams inline with the page they explain; use flowcharts for navigable subsystem views, class diagrams for relationships and sequence diagrams for operations. Mermaid supports `click Node href "relative/path.md"`, subject to the host renderer's security restrictions. **Always add ordinary Markdown links near clickable diagrams**, and ensure every clickable target exists. Avoid storing duplicate generated SVGs.

## Validation

Check that all relative Markdown links resolve, that Mermaid parses, that source links point to real headers, and that runnable examples compile on their intended target. Change documentation alongside public API behavior. Keep old URLs usable via small forwarding pages when moving existing content.
