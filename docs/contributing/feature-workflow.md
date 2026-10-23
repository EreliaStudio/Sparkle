# Feature development workflow

New features and significant behavioral fixes are designed **in a GitHub issue**, not in a documentation-only PR. The issue remains the authoritative design record until implementation is accepted.

~~~mermaid
flowchart TD
    Issue["Open an issue: problem and goal"]
    Design["Refine issue: alternatives, decisions, API, tests"]
    Approval{"Design accepted?"}
    Branch["Create feature branch from target"]
    Work["Implement code + docs + tests"]
    Gap{"New design decision?"}
    PR["Open PR linked to issue"]
    CI{"Required CI passes?"}
    Review{"Maintainer review accepted?"}
    Merge["Merge PR"]
    Issue --> Design --> Approval
    Approval -->|"No"| Design
    Approval -->|"Yes"| Branch --> Work --> Gap
    Gap -->|"Yes: update issue"| Design
    Gap -->|"No"| PR --> CI
    CI -->|"No: fix on branch"| Work
    CI -->|"Yes"| Review
    Review -->|"Changes requested"| Work
    Review -->|"Yes"| Merge
~~~

## Phase 1: design the issue

Describe the problem from the **end user's perspective**, including expected usage before committing to classes or internal architecture. Refine the issue body as choices are accepted so it always contains the latest coherent plan (as earlier architecture PR plans did).

An appropriately detailed issue may include:

- Goal, motivation, use cases and what is intentionally out of scope.
- Expected consumer-facing C++ code and proposed public API.
- Relevant existing Sparkle classes, constraints and dependencies.
- Alternatives considered, open questions and explicit maintainer decisions.
- Ownership, lifetime, error handling, threading and behavior policies where relevant.
- Expected documentation changes and optional Mermaid diagrams.
- Test matrix and acceptance criteria, including integration tests if needed.

**Exit criterion:** the maintainer has explicitly accepted a sufficiently complete design. No feature branch or implementation is required during this phase.

## Phase 2: implement on a feature branch

Start from the agreed target branch. The branch is responsible for all parts of the feature:

| Deliverable | Expectation |
| --- | --- |
| Implementation | Public API and internal code implement the accepted design |
| Documentation | Relevant user guide, reference, architecture and contribution pages are updated |
| Tests | Automated public behavior, regression and edge-case coverage are added or updated |

Not all changes need all three categories (a documentation-only change has no new runtime code), but a genuine new feature should not be merged without documenting and testing it.

Do not silently redefine the issue during coding. If the plan is wrong or incomplete, amend the issue and validate the new decision before proceeding.

## Phase 3: PR and CI validation

Create a PR linked to the issue, summarizing the implemented contract, test evidence and any approved deviations. Review the full required pipeline against the latest commit: building, formatting, functional tests and applicable memory-safety/stress jobs. Fix failures on the branch and re-run checks. Do not merge merely because the pipeline is green: maintainer review remains required.

## Phase 4: additional integration testing (conditional)

When the feature has meaningful cross-subsystem behavior that small tests cannot demonstrate, the issue may propose an integration-test scenario or a future dedicated CI job inspired by Erelia's integration validation. This is a possibility to evaluate, **not a required immediate pipeline refactor**. Specify the tested scenario, success criteria, test environment, reliability and cost before implementing it.

## Related guidance

- [Agent instructions](../AGENTS.md)
- [Development and architecture guidelines](../ai-development-guidelines.md)
- [CI test strategy](ci-test-strategy.md)
- [Writing documentation](writing-documentation.md)
