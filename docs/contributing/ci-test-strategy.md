# Sparkle CI test strategy

Sparkle separates test responsibilities so a failure identifies the kind of defect, rather than running a single large and ambiguous stress/sanitizer job. CI jobs are organized by **test purpose** and may run on several operating systems.

| Category | What it validates | Selection and environment |
| --- | --- | --- |
| Functional Core | Public contracts and deterministic behavior | `Core.*` excluding `*StressTest` and `*MemorySafetyTest`; Windows/Linux Debug and Release |
| Functional Graphics | Rendering and graphics contracts | `Graphics.*`; Windows Debug and Release, with image comparisons |
| Memory safety | Lifetime, resource cleanup, use-after-free, buffer errors, double-free, and leaks where supported | `*MemorySafetyTest` plus selected legacy lifecycle regressions; Windows ASan and Linux ASan/LSan |
| Stress | High workload, randomized inputs, concurrency, progress, and exact computational results | `*StressTest`; Windows/Linux Release; dedicated bounded repetitions |
| Coverage | Instrumented test coverage and reports | Existing LLVM coverage job |
| Packaging | Install, relocation, vcpkg consumers | Existing packaging jobs |

The test naming and CI routing conventions are intended to support multiple Core domains without mixing functional, memory-safety and stress workloads.

## Functional tests

Name suites after the component and behavior being tested. Tests should be quick and deterministic. Core functional CI deliberately excludes stress and memory-safety suites to keep responsibility and failure reporting separate.

## Memory-safety tests

Name a suite `<Domain>MemorySafetyTest` (for example `NetworkMemorySafetyTest`). Run it against the same instrumented Core library and test executable with AddressSanitizer.

- On Linux, enable LeakSanitizer through `ASAN_OPTIONS=detect_leaks=1`. A detected leak or memory violation must fail the job.
- On Windows, ASan verifies memory access and ownership errors. Windows ASan does **not** provide the same supported leak-detection capability as Linux LSan; do not report Windows ASan as proof of no leaks.
- Test complete ownership lifecycles: start, use, disconnect, release handles, destroy objects, and cancel pending operations.
- Run sanitizer tests once per CI matrix execution rather than repeating randomized/stress workloads here.
- Keep explicit leak tests small and reproducible. A clean ASan run does not prove that every possible resource leak or race has been eliminated.

Add `*MemorySafetyTest` suites for other domains when concrete lifecycle and ownership risks warrant the coverage.

## Stress and randomized tests

Name a suite `<Domain>StressTest` (for example `NetworkStressTest`). A stress test must define its **workload, seed, quantitative expectations, and termination bounds**.

1. Use a fixed and logged PRNG seed in mandatory CI. Failed workloads must be reproducible. Additional seeds can be introduced explicitly as a matrix later.
2. Generate varying message sizes, page boundaries, priority mixes, concurrency counts, or component-specific workloads.
3. Assert exact measurable results: number of messages, pages and bytes processed; no duplicate/missing data; expected output and accounted resources.
4. Report processed operation counts and bytes (and optional elapsed time) as diagnostics. Avoid hard failure thresholds on elapsed time on shared GitHub-hosted runners: scheduling and machine contention fluctuate.
5. Bound every run and every repetition; distinguish timeouts/deadlocks from data-integrity failures.
6. Use bounded reruns for high-value concurrency and randomized regression cases. Do not run stress repetitions under the sanitizer job.

Existing networking stress coverage is grouped under `NetworkStressTest`, including the many-client exchange test. New seeded tests verify complete payload integrity and page-scheduler byte accounting under randomized sizes and priorities. CI selects all future Core stress suites with `^Core[.].*StressTest[.]`.

## Future job categories

- **Benchmarks / performance regressions:** dedicated measurements of throughput, latency and allocations, ideally on controlled hardware with statistically meaningful baselines. Do not confuse randomized correctness with machine-specific speed assertions.
- **Fuzzing:** mutation-based, corpus-guided tests (particularly parsing, framing, page assembly, and serialization). Keep corpus retention and crash reproducers explicit; nightly or scheduled workflows may be more suitable than PR-time fuzzing.
- **ThreadSanitizer:** Linux concurrency race detection on compatible builds, separate from ASan, because ASan does not detect general data races.

Do not add a CI job until it has real relevant tests, clear failure criteria, and an acceptable execution budget.

## Updating the CI strategy

A feature's [approved GitHub issue](feature-workflow.md) should define the relevant test levels and proposed CI changes. Add tests for ByteStream, containers, worker pools, UI, rendering, and other domains to the appropriate existing categories. Propose new CI jobs only when an existing category cannot validate the intended contract. Check the full required workflow matrix against the latest pull-request head before concluding that CI passes.
