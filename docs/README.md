# Sparkle documentation

Sparkle is a C++23 library with a headless Core target and an optional graphics/UI target. This directory documents **using** the public API and **contributing** to its implementation. These pages describe the `Version-0.1.3` code, not unmerged proposals.

## Find your way

~~~mermaid
flowchart TD
    Start["Sparkle documentation"]
    Start --> Begin["Getting started"]
    Start --> Architecture["Architecture"]
    Start --> Reference["API reference"]
    Start --> Guides["How-to guides"]
    Start --> Contribute["Contributing"]
    click Begin href "getting-started/README.md" "Setup and first examples"
    click Architecture href "architecture/README.md" "Subsystem responsibilities"
    click Reference href "reference/README.md" "Classes and API contracts"
    click Guides href "guides/README.md" "Task-oriented examples"
    click Contribute href "contributing/README.md" "Contribute to Sparkle"
~~~

Some Markdown renderers, including secured GitHub views, disable Mermaid click links. All destinations are available below independently of diagram interaction.

| I want to... | Start here |
| --- | --- |
| Install and build Sparkle | [Getting started](getting-started/README.md) |
| Understand how subsystems work together | [Architecture](architecture/README.md) |
| Understand a class or public contract | [Reference](reference/README.md) |
| Accomplish a task with example code | [Guides](guides/README.md) |
| Change the library or write tests | [Contributing](contributing/README.md) |
| Plan and deliver a feature | [Issue-first feature workflow](contributing/feature-workflow.md) |
| Give a coding agent its required context | [Agent instructions](AGENTS.md) |
| Find old engineering notes, plans or validation reports | [Historical/internal documents](internal/README.md) |

## Documentation policy

Source headers and implementations under `include/` and `src/` define current behavior; tests under `tests/TU/` demonstrate validated scenarios. Docs explain those contracts without replacing them. A proposed change must never be presented as released behavior. Mermaid is the standard diagram format; use ordinary Markdown links alongside `click ... href` directives.

See [writing documentation](contributing/writing-documentation.md) for conventions.
