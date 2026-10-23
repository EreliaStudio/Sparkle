# Instructions for AI contributors to Sparkle

Before designing, reviewing, or changing Sparkle, read the applicable guidance:

1. [Documentation agent instructions](docs/AGENTS.md): required context, feature issue workflow, testing and documentation expectations.
2. [AI Development and Code Review Guide](docs/ai-development-guidelines.md): maintainers' architecture, C++23 style, error/lifetime/concurrency principles, test and review criteria.
3. [Feature workflow](docs/contributing/feature-workflow.md): design and approve a GitHub issue **before** opening an implementation branch; include code, documentation and tests as applicable; open a linked PR and validate required CI.
4. Relevant subsystem [architecture](docs/architecture/README.md), current source and tests. The authoritative contract is the accepted issue plus current implementation, not an older PR plan.

## Working rules

- Start with the consumer's goal and public usage, then derive the smallest coherent implementation.
- Do not silently decide significant architectural questions. Record accepted decisions in the issue and obtain design approval before coding.
- Prefer explicit behavior and meaningful classes/traits over unnecessary layers, speculative generalization or defensive machinery.
- Keep modifications within the approved scope. Preserve public contracts unless their changes are authorized.
- For each new feature, include corresponding source, documentation and test changes as relevant. Prefer observable, failure-path, lifecycle and concurrency tests.
- Consider additional end-to-end/integration tests only when cross-system behavior warrants them; a new CI job is not automatically required.
- Apply the pinned clang-format configuration to modified C++ code. Do not report test or CI success unless checked against the changed head.
- Open a PR against the intended target and link the design issue; wait for required CI and maintainer review before merging.
