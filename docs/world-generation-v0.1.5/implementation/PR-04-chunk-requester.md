# PR04 — One-Task-per-chunk ChunkRequester and local generator source

**Layer:** Core. **Dependencies:** PR03. **Status:** design draft.

## Scope
- Virtual `ChunkRequester<TChunk>` acquisition interface accepting one ChunkCoordinate and returning an existing Sparkle Task answer/future for an owned `TChunk`.
- Local implementation wraps a configured Generator with `WorkerPool` and uses exactly one task per requested chunk. Generator builds the chunk through fresh `TChunk::Builder`, ordered passes and `builder.build()`; the requester only receives the completed chunk, never its mutable construction state.
- Distinguish successful non-null result, successful null result (no chunk exists at coordinate), and failed Task; requester never mutates the ChunkCollection.
- The same interface must be implementable for files or existing networking without new protocol-level code.
- Keep requester/generator alive during active operations through a documented lifetime contract.

## Required tests
- Local generator produces completed derived type and Task status; null builder/no chunk and exceptions (including build failures) remain distinguishable.
- Fake remote delayed task uses same interface, no required network stack.
- Destruction/scope tests and independently tracked requests for different coordinates.

## Exclusions
No interest zones, priority ordering, timeout/retry policy, automatic cancellation or transport protocol.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
