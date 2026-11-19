# v0.1.5 — PR11: Interest zones, chunk requests and streaming

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core. **Dependencies:** PR02, PR05.

## Objective

Integrate lazy generation with optional disk or network sourcing, without blocking the engine update loop or coupling Core to the renderer.

## Proposed scope

- Design a small request interface supporting coordinate, priority, result/failure and cancellation.
- Provide position-centered and fixed interest descriptions as examples, independent of camera/player.
- Merge overlapping interests by stable maximum priority; apply retention/hysteresis.
- Deduplicate requests, cap concurrent work, invalidate stale results and unload unneeded chunks.
- Publish completed immutable chunks on a controlled owner thread, with monotonic revisions.
- Keep local generation synchronous internally where possible; async request workers wrap it.

## Required automated tests

- Fake requester for priority changes, cancellation, retry, duplicate demand, stale result and retention.
- Memory budget and deterministic update ordering; no real sockets or external server.
- No data race when generation threads complete after chunk unload.

## Explicit exclusions

No new networking protocol, auth/matchmaking, camera dependency, GPU uploads or custom task scheduler if Sparkle has reusable infrastructure.

## Architecture review before coding

Choose requester/result pattern and cancellation semantics after observing Sparkle's current threading primitives.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
