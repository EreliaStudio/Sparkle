# Architectural contribution guidelines

The canonical decision guide is [AI Development and Code Review Guide](../ai-development-guidelines.md), sections 1–6 and 9.

Core expectations:

1. Start from the actual public use case and responsibility, not a preferred abstraction.
2. A class or trait should represent one coherent responsibility; coordination can itself be a responsibility.
3. Do not introduce recovery, caches, wrappers, configuration, or special cases without a demonstrated need.
4. Express consequential policy choices using named types and enums.
5. Make ownership and lifetimes explicit and keep concurrency boundaries understandable.
6. Test observable API behavior, including error, lifecycle and concurrency paths.
7. Preserve external API behavior unless a change was explicitly approved.

For architecture already implemented, navigate [architecture documentation](../architecture/README.md) and related source. Do not use historical PR plans as evidence of current behavior.
