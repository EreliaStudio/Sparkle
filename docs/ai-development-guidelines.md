# Sparkle — AI Development and Code Review Guide (v2)

> **Audience:** AI coding agents and human reviewers working on Sparkle.
> **Purpose:** Describe how to *design, implement, test, and review code* according to the maintainer's expressed preferences. This is an architectural decision guide, **not** an instruction to rewrite existing code indiscriminately.
> **Status:** Working conventions gathered through an extensive maintainer interview (October 2026). Some are preferred future directions rather than requirements already satisfied throughout the repository.

## 0. Instructions to an AI agent — read first

1. **Read the task/PR description and relevant source code before proposing changes.** Identify the public contract, real responsibilities, ownership boundaries, failure semantics, and thread/lifetime implications.
2. **Do exactly the requested work.** Review broadly to understand implications, but implement only agreed changes within scope. Surface unrelated improvements separately, ideally as proposed follow-up tickets.
3. **Do not guess major architectural decisions.** Present concrete alternatives, tradeoffs, and a recommendation; ask the maintainer to choose where the decision could change the design materially. Do not hold up obvious mechanical work over trivial details.
4. **Prefer fewer concepts and less code that remain easy to follow.** Do not turn a coherent object into a fleet of forwarding wrappers, tiny stateless “manager” objects, or an elaborate generic framework.
5. **Distinguish preference levels:**
   - **Strong:** normally follow unless a technical requirement demonstrably conflicts.
   - **Contextual:** choose from the object's semantics, invariants, and actual use.
   - **Direction:** a desired practice for new code, not permission for unrelated retroactive refactoring.
   - **Undecided:** do not fabricate a rigid project rule.
6. **Never trade correctness or explicit task acceptance criteria for stylistic uniformity.** A concrete PR contract takes priority over examples in this guide; report apparent contradictions.
7. **Before calling a task complete:** inspect the diff; run the relevant unit/integration tests and formatting checks; report what was tested, what could not be tested, and any remaining risk. Do not claim CI passed without checking that precise head.

**Primary motto:** *Nothing is implicit.* Sparkle should perform what the caller explicitly requested, subject to the mechanics naturally necessary to perform that operation. Names, types, ownership, failures, and policies should make behavior understandable at the call site.

---

## 1. Core design philosophy

### 1.1 Meaning before structure — STRONG

- **One meaningful responsibility per class or trait.** A responsibility is identified by coherent semantics, invariants, and dependencies, **not** an arbitrary line count or number of methods.
- A class may coordinate several collaborators when that coordination *is* its legitimate responsibility. Do not introduce an extra coordinator merely to forward calls.
- Split large classes when they actually mix responsibilities or independent state and lifecycle. Do not split solely to make the files shorter.
- Eliminate duplication **when it represents a shared concept**, not just because two pieces of code look similar.
- Prefer a concrete implementation until the need for alternative implementations is real. Avoid YAGNI abstractions and speculative extensibility.
- Keep APIs naturally readable without navigating a cascade of wrappers to learn what an operation does.
- Prefer descriptive, precise names, even if longer. Comments should explain **why**, subtle invariants, or contracts that the API cannot express; code should express **what**.

### 1.2 Explicit behavior — STRONG

- Methods must behave as their names imply. Do not silently reconnect, retry, subscribe, recover, perform unrelated work, or select surprising policies.
- When several legitimate behaviors have materially different consequences, expose an explicit **enum class**, policy type, or method argument. Do not hide important decisions behind ambiguous booleans.
- **Treat comments explaining an implicit behavior choice as an API-design warning.** Review every new or modified public method for an implicit policy (for example reconnect versus throw, restart versus throw, or recipient overflow handling). Where the caller has useful choices, expose a named policy enum at the call site, implement each alternative, and default to the conservative behavior (normally `Throw`/`Reject`). Validate unsupported enum values before changing state.
- Prefer a policy enum nested in its owning API. Reuse an existing policy if its semantics already cover the case; do not create another enum, a boolean switch, or a parallel method with the same responsibility merely to avoid a comment.
- Remove comments that only restate a choice now expressed by the API. Retain concise documentation for guarantees that a type cannot express, such as acknowledgment scope, memory-accounting limitations, and thread/lifetime constraints. Do not invent meaningless policy alternatives for fixed protocol invariants or mandatory safety rules.
- Optional parameters may have reasonable defaults; the requirement is to make **consequential** decisions visible.
- Make invalid calls fail clearly and early. Do not silently transform an invalid operation into a different operation.
- Prefer direct, simple code over architectural ceremony. Short methods (often around 25 lines) are a useful readability heuristic, **not** a rule requiring meaningless extraction.
- Prefer guard clauses/early returns to deep nesting.

### 1.3 Classes versus data — CONTEXTUAL

- Use **struct** for value-like data, even when it has legitimate methods. Aggregates and designated initializers are welcome for passive data.
- Use **class** for objects with identity, invariants, responsibilities, encapsulation, or meaningful lifecycle.
- Introduce a domain-specific result/configuration type only when it forms a coherent concept. Use `std::pair` or `std::tuple` for incidental multiple results.
- A configuration struct is preferred **when its parameters conceptually belong together**, not merely when a constructor has “too many arguments.”

---

## 2. Object-oriented architecture, inheritance, composition, and traits

### 2.1 Runtime polymorphism — STRONG/CONTEXTUAL

- Prefer ordinary **virtual base classes** and derived implementations for genuinely interchangeable runtime behaviors; do not automatically replace them with template hierarchies.
- Abstract bases should express meaningful contracts, not exist in anticipation of future variants or just to support mocks.
- Name a base after its concept (for example **FileSystem**), not with an obligatory `I` prefix.
- Let an abstract base hold common behavior and state if conceptually appropriate.
- Use **protected virtual hooks** when a base orchestrates a process and descendants customize a step. Choose pure virtual for mandatory behavior; use defaults only for truly optional hooks.
- Use public virtual operations when polymorphic callers directly invoke those operations.
- Write `override` without redundantly repeating `virtual` on overriding declarations.
- Avoid `final` unless there is a concrete design or technical reason.

### 2.2 Traits/mixins — STRONG

Traits are **inherited behavioral capabilities**, not a collection of free functions dressed as classes.

- A trait may own meaningful state, mutexes, subscriptions, or resources required by its responsibility.
- Small traits can inherit other traits **when their capability genuinely requires those capabilities**.
- Extract a trait only when the new capability is independently meaningful and composable.
- Do not manufacture dozens of traits to achieve a cosmetic one-method-per-class rule.
- Prefer capability-oriented names such as **Activable**, **Updatable**, **Replicable** over automatically appending `Trait`. This is a **direction** for new/refactored code: rename for meaning, not mechanically.
- If consumers need **multiple independent instances** of a capability (for example, several channels), prefer composition with separate owned objects over multi-inheriting the same stateful trait and accidentally merging instances.

### 2.3 Composition, dependency injection, and relationships — CONTEXTUAL

- Inject externally shared services explicitly rather than relying on global service location. Historical facilities such as `spk::Logger` do not authorize an unrelated singleton rewrite.
- Put fundamental ownership and essential dependencies in constructors. Use `bind()` for associations established separately from construction.
- A **single-target** `bind()` should reject binding again until explicitly `unbind()`. For multiple independent associations use explicit additive/removal names such as `addBinding()` / `removeBinding()`.
- RAII contracts are preferred for subscriptions and similar registrations; destruction revokes registration.
- Direct subsystem references are fine when genuinely meaningful. Do not invent an interface merely to satisfy a dogmatic layer diagram.
- Bidirectional **non-owning references** may model legitimate domain relationships. Distinguish them from cyclic strong ownership and circular library dependencies.
- Avoid `detail` namespaces by default. Use nested classes for tightly owned concepts/privileged collaborators, or PImpl where implementation details would otherwise dominate a header. `friend` is acceptable when nesting is not appropriate; do not expose private mechanics publicly to avoid it.

### 2.4 State patterns — CONTEXTUAL

- For simple finite state: prefer `enum class` with clear conditional logic.
- For a state with substantial behavior, members, and invariants: prefer distinct polymorphic state objects.
- Use virtual behavior for **specialization**; use callbacks or subscriptions for **notifications**. Don't conflate these responsibilities.
- Use polymorphic base behavior for shared contracts, typed collection lookup for heterogeneous objects, and `dynamic_cast` when a downcast is justified. Do not pollute a base API with derived-only capabilities merely to ban downcasts.

---

## 3. Public API and type design

### 3.1 API surface — STRONG

- Prefer straightforward member functions for behavior belonging to the object; use static methods for type operations and free functions for genuinely independent algorithms.
- Use natural property getters such as `name()`, `position()`, `isActive()` and setters like `setName()`.
- Prefer `void` setters and independent statements to fluent `return *this` chains. Fluent builders may exist, but are not the default.
- Use overloads when input types vary but **the operation's meaning stays the same**. Give different semantics different names.
- Use `find(key)` for an optional result (typically a nullable pointer); use `get(key)` for a guaranteed reference that throws `spk::Exception` if missing.
- Prefer returning `const T&` for an existing stored object, provided the owner and reference lifetime remain clear. A `T&` is appropriate when the object is guaranteed and mutation is intended; `T*` when absence is possible.
- When consumers genuinely need a container's full interface, expose the container appropriately rather than forwarding its entire API one method at a time. Use narrower domain methods where they protect invariants or limit a meaningful capability.
- Prefer `const T&` for ordinary read-only parameters. Use `std::span<const T>` for contiguous read-only collections when container-independence is useful; concrete container references are fine internally when the type is already known.
- Use `[[nodiscard]]` when ignoring a result is likely an error, not on every function mechanically.

### 3.2 Meaningful types — STRONG

- Prefer strong domain-specific identifier types rather than bare numeric aliases when they express distinct concepts. Leverage meaningful shared capability traits to reduce repetitive boilerplate.
- Make conversions from strong types **explicit**; avoid accidental implicit conversions to underlying integers or between distinct identifiers.
- Use `enum class` for domain policies and choices.
- Choose type aliases only when the alias communicates a role, not solely to shorten a verbose type.
- Prefer named result structures for meaningful results, `pair`/`tuple` when no distinct concept exists.
- For equality: value objects compare relevant values; entities compare identity (often UUID). Name additional comparisons explicitly (for example `hasSameState()`).
- Hashing should agree with equality. Appropriate value types may supply `std::hash`; mutable entities are normally indexed by stable IDs.
- **Direction:** prefer defaulted `operator<=>` and `operator==` for naturally ordered value types rather than hand-writing every relational operator. Be mindful that defaulted comparison follows member declaration order.

### 3.3 Construction, builders, and copy/move — STRONG/CONTEXTUAL

- A successfully constructed object must satisfy its invariants. Constructors may throw on failed initialization.
- When a value requires **incremental construction**, a mutable nested `Writer` or `Builder` can produce an immutable completed object. Do not make every ordinary constructor into a builder.
- Prefer a builder that validates invalid operations as they occur. Invalid use throws; if finalization can fail normally, `build()` may return `std::expected`.
- When a method **takes ownership**, prefer an explicit `T&&` argument and transfer via `std::move`, rather than silently copying an lvalue.
- Support copying when it is meaningful and practical; do not make all types move-only by default.
- Explicitly declare and `= default` supported copy/move constructors and assignments to make capabilities visible. Write custom implementations when invariants or resource ownership demand them.
- Moved-from objects must remain valid for destruction and operations permitted by their class contract; do not enforce a universal “reset to empty and reusable” state.

### 3.4 Ownership semantics and lifetime — STRONG

- Encapsulate ownership mechanics in meaningful domain objects (for example a copyable handle sharing a `Data` instance), instead of spreading `shared_ptr` throughout unrelated public APIs.
- Use non-owning pointers/references when ownership belongs elsewhere.
- **Important explicit preference:** an asynchronous method may accept borrowed data (`std::span`, reference, etc.) with a **caller-managed lifetime contract**; do **not** automatically copy or require shared ownership merely because the operation is asynchronous.
- This borrowing must be explicit and safe: define when the borrowed data must stay valid, what completion/failure/cancellation ends the borrow, and which thread may access it. Review for dangling references and concurrent mutation.
- Prefer RAII to manage actual resource ownership and registration lifetimes.

---

## 4. Error handling, validation, and diagnostics

### 4.1 Classify failures by semantics — STRONG

- **Invalid public operation or violated API contract:** throw `spk::Exception`.
- **Expected operational failure with meaningful recoverable information:** prefer `std::expected<T, E>` (C++23).
- **Simple absence/query:** use bool, optional, or a nullable pointer when no detailed reason is needed.
- **Internal programmer invariant:** assert where appropriate; do not rely on a Release-disabled assertion to validate untrusted public input.
- **Irrecoverable unsafe corruption:** termination can be appropriate.
- Use domain-specific error-code enums and a simple reusable shape such as `ErrorType<TEnum>` containing **code + message**; do not invent elaborate metadata fields before needed.
- Translate lower-level errors to appropriate domain errors at actual architecture boundaries, retaining useful context rather than leaking implementation-specific codes.

### 4.2 Context, validation, and exception safety — STRONG

- Validate at **public API boundaries**, then trust internal helpers under maintained invariants; avoid repeated defensive checks at every call depth.
- Validate and fail on the **first** detected error rather than collecting error lists by default.
- Check runtime configuration changes immediately; reject invalid values without partially applying them.
- Prefer `spk::Exception` with explanatory context and `std::current_exception()` as a cause when rethrowing at a meaningful boundary.
- Don't wrap every tiny function in `try/catch`. Add context when it changes the caller's understanding; otherwise propagate.
- Prefer logging exceptions once at the **final handling boundary**, not at every layer that rethrows them.
- Keep objects valid after failure (basic exception safety). Use rollback/atomic behavior where the **specific operation's contract** requires it; valid partial progress is acceptable where explicitly intended.
- Mark functions `noexcept` conservatively, when there is a real contractual or technical basis. Destructors should **not throw**.
- For destructor cleanup failures: use explicit fallible close/commit operations when callers must recover; catch and log unexpected nonfatal cleanup errors at the appropriate final boundary; terminate only when continuing would be unsafe.
- When allocation failure needs reporting through Sparkle, prefer contextual `spk::Exception` carrying the underlying `std::bad_alloc` as cause **at a meaningful boundary**, rather than catching every allocation site.
- Distinguish intentional memory-budget exhaustion (which can be a normal operational outcome) from actual allocator failure.

---

## 5. Serialization, data, and immutable publication

### 5.1 Format responsibility — STRONG

- Keep human-readable diagnostics (`toString()`) distinct from serialization.
- Prefer `toString()` as the primary text representation; an `operator<<(std::ostream&, const T&)` may delegate to it.
- Prefer **dedicated codec objects** for encoding/decoding, separate from the domain object's behavior. Encoder + decoder for one coherent format may be one codec responsibility.
- **Tentative placement preference:** a codec conceptually owned by one type can be nested (for example `Vector3::Codec`). This is not an architecture mandate; a protocol's codec may belong to the protocol.
- Avoid making domain objects know network framing or storage formats merely for convenience.
- Prefer a nested mutable `Writer` that finalizes into an immutable message/bytestream value when it matches the semantics.
- Actual examples in this PR branch: `spk::ByteStream::Writer` builds immutable buffer storage, `ByteStream::Reader` has an independent cursor, and `spk::Message::Writer` composes a byte-stream writer with message header construction.
- Do not introduce duplicate operator/serialization layers when the existing writer or codec already supplies the operation.

### 5.2 Immutability for concurrency — STRONG

- Prefer **immutable shared data**, especially for cross-thread published messages, snapshots, and other read-mostly results.
- Use mutable working state/builders, then publish a complete immutable result. Readers may retain old versions without racing against mutation.
- Favor fresh snapshots first; use structural sharing/copy-on-write when a concrete cost justifies it.
- Synchronized mutable shared state remains appropriate when the object's semantics genuinely require it.
- Immutability must be real: don't expose a mutable alias to published storage or hide unsynchronized mutation behind `mutable` caches.

---

## 6. Concurrency and async execution

### 6.1 Ownership, locks, and synchronization — STRONG/CONTEXTUAL

- Start with simple synchronization primitives; optimize contention only when useful. Choose lock granularity according to **shared invariants**, not according to a universal one-lock-per-class rule.
- Prefer **raw data objects unaware of locking**, with a wrapper/coordinator such as `spk::ProtectedData<T>` responsible for synchronization and scoped access.
- Group data under one lock when it has atomic cross-field invariants. Use independent protected containers when their invariants are independent.
- Do not let borrowed references to protected data escape the lifetime of the guard.
- Prefer performing external callbacks outside locks when safe, so user code cannot unexpectedly hold an internal mutex or induce deadlock.
- Query getters should generally be const and observational; exceptions such as thread-safe memoization or lazily evaluated state need a clear semantic justification.

### 6.2 Work scheduling and callbacks — CONTEXTUAL

- Keep synchronous domain behavior independently usable. A task can wrap it for dispatch via a separate worker pool; don't make the domain object own the global scheduler without a responsibility-based reason.
- Async tasks may deliver results through completion callbacks, executed on the worker thread when that is their documented contract. Receivers own synchronization with their state.
- Use callbacks/subscriptions to **notify**, not to replace natural virtual customization hooks.
- Think through task cancellation, destruction, callback reentrancy, failure, and lifetime boundaries rather than treating only the successful path.
- Use distinct subscriptions/channels as composed objects when several independent instances are required.

### 6.3 Platform boundaries — STRONG

- Public interfaces should remain platform-neutral when possible.
- Platform-specific implementation is usually **PImpl or a nested private implementation** selected with platform macros, with separate platform implementation files when substantial. Avoid public WinAPI types when a domain-neutral type exists.
- Prefer typed C++ compile-time configuration. Use preprocessor macros when code/dependencies must actually be conditionally compiled; do not use macros for ordinary behavior expressible in C++.

---

## 7. C++23 coding style and repository structure

### 7.1 File and declaration organization — STRONG

- Prefer **one class per file** by default, with meaningful nested types/close helpers where cohesive.
- Put non-template declarations in `.hpp` and method implementations in `.cpp`. Keep template definitions in headers; **no `.tpp` files**.
- Favor domain-first directories (such as `network/` and `rendering/`), subdivided by architecture where it improves navigation.
- Prefer direct includes; use forward declarations to break circular dependencies; use PImpl for large or platform-specific internals.
- For class declarations, preferred logical order is:
  1. Public nested types/aliases.
  2. Protected nested types/aliases.
  3. Private nested types/aliases.
  4. Repeat access sections if type dependencies require it.
  5. Private attributes and methods.
  6. Protected attributes and methods.
  7. Constructors.
  8. Destructor.
  9. Copy/move operations.
  10. Public methods.
- `.cpp` implementation order should generally follow the header, except where a small rearrangement materially improves readability.

### 7.2 Naming, language idioms, and loops — STRONG/CONTEXTUAL

- Private data members: `_camelCase`.
- Private methods: `_camelCase()`.
- Protected methods, including virtual hooks: `_camelCase()`.
- Public methods: `camelCase()`.
- Use meaningful, precise type names; prefer capability names over redundant suffixes.
- Prefer `const` wherever valid for locals, references, and intermediary values.
- Prefer explicit local types when helpful; `auto` is fine for iterators, range-based iteration, and verbose/inferable types.
- Prefer explicit loops and direct control flow over unnecessarily clever algorithms/ranges.
- Use lambdas for callback adaptation; move significant named behavior into an appropriately named member method.
- Default member initializers are preferred for normal default state; constructors' initializer lists for supplied values/dependencies.
- Use template constraints inline when local; extract a named concept when it is reused and meaningful.
- Do not implement template-heavy machinery where simple runtime polymorphism or concrete code expresses the responsibility better.
- `constexpr` is commonly used for static constants. There is **no strong preference** to mark ordinary functions `constexpr` proactively; `consteval` should not be introduced without a clear use.

### 7.3 Allocation and formatting — STRONG

- Prefer STL containers and standard allocation. A pool such as `spk::Pool` makes sense for obvious high-churn resources like frequently recreated buffers or tasks; do not pool everything speculatively.
- Compose STL facilities into meaningful higher-level capabilities (such as thread-safe queues), but **do not inherit from STL containers** merely to expose their methods.
- Enable and address strict compiler warnings, but do **not** universally treat every warning as a build error.
- Use the project's **pinned clang-format version**. Enforce formatting on changed code through CI while avoiding unrelated whitespace churn.
- Maintain the current project's actual clang-format configuration rather than improvising a competing style.

---

## 8. Testing policy

### 8.1 What tests should observe — STRONG

- Test **public contracts**, not private implementation details. No test-only public helpers or `friend` access by default.
- If an internal algorithm seems to need its own public tests, first ask whether it has a **genuine independent production responsibility** and belongs in a meaningful class. Never split solely to enable a test.
- Prefer **mocks to isolate dependent objects** when a meaningful abstraction makes mocking natural.
- If mocking requires an artificial production interface, use real collaborators instead. Testability can reveal a missing *genuine* abstraction but never proves that an interface is needed.
- Assert externally observable results. Verify interactions with mocks when the interaction itself matters to correctness (for example, resource release or calls that must occur exactly once); don't lock tests to arbitrary implementation sequences.

### 8.2 Coverage, concurrency, and platforms — STRONG

- Cover success, rejection/failure, edge values, invalid input, lifecycle, cleanup, and regression cases appropriate to the behavior.
- Use actual threads and actual public APIs when verifying concurrent responsibilities.
- Use `std::latch`, `std::barrier`, condition variables, and bounded deadlines to establish reproducible interleavings; avoid sleep-based guesswork.
- Add stress tests for less predictable scheduling and resource-exhaustion patterns.
- For networking, test both isolated protocol logic and real-socket integration across supported platforms, including connection/disconnection, in-flight operations, malformed input, and shutdown.
- Run Debug and Release where practical; use sanitizers where available. Do not substitute passing unit tests for critical integration behavior.
- Use TestLibrary golden-image testing in graphical components when relevant, not in headless core/networking by default.
- Keep CI assertions factual: “passed” means the **current pushed commit**, not an earlier head.

---

## 9. Reviewing and changing a pull request — REQUIRED AI WORKFLOW

### 9.1 Understand before editing

1. Read PR description, base branch, latest head, changed files, and relevant public APIs.
2. Separate **contractual requirements** from architectural preferences and incidental current implementation.
3. Trace main use cases and edge cases end to end: state transitions, ownership, error paths, synchronization, cleanup. Audit new/modified method signatures for hidden conditional behavior; turn genuine caller-selectable branches into named policies and test every branch, its default, and invalid enum values.
4. Identify each class's actual responsibility and reason for existence. Flag:
   - unrelated concerns mixed inside one class;
   - unnecessary forwarding/proxy layers;
   - apparently generic abstractions without a second meaningful use;
   - traits that don't represent a reusable capability;
   - accidental lifetime coupling or shared mutable state;
   - ambiguous policies and surprising implicit behavior.
5. Do **not** rank code quality based on line count, number of files, or whether every design pattern appears.

### 9.2 Classify findings and ask good questions

Report each finding with a concrete file/location, observed behavior, violated contract or invariant, consequence, and proportionate fix. Separate:

- **Blocking correctness/safety defect:** violation of acceptance criteria, race, lifetime bug, deadlock, broken API contract, data loss, resource leak.
- **Important architectural concern:** responsibilities, ownership, extension point, or API semantics likely to create real maintenance problems.
- **Optional style/readability suggestion:** naming, organization, modest cleanup with no material risk.

For architectural choices with several valid options, ask the maintainer a **focused question with alternatives and tradeoffs** before implementing a design direction. Do not substitute sycophantic agreement for technical assessment. Explain disagreements when a proposed choice compromises correctness.

### 9.3 Implementation discipline

- Make the smallest **cohesive** change that solves the stated problem correctly.
- Match surrounding file style and maintain public contracts unless change is explicitly approved.
- Prefer many small responsible classes when warranted, but avoid wrappers whose only effect is an extra call depth.
- Add tests for new behavior and regressions, including failure/lifecycle/concurrency when relevant.
- Format changed code, review final diff, run relevant tests, and report exact verification.
- Do not merge, mark ready, change unrelated API families, or undertake adjacent refactors without authorization.

### 9.4 PR #29 context — task-specific notes, not general coding rules

At the time this guide was added, **Sparkle PR #29** is a **draft transport-infrastructure PR** on `feat/network-connection-and-paged-transport` targeting `Version-0.1.3`. Its PR description is the authoritative source for up-to-date acceptance criteria and test status:

https://github.com/EreliaStudio/Sparkle/pull/29

A reviewing AI should particularly check:

- Explicit connection lifecycle and Pending/Connected/Disconnected contract.
- No implicit reconnect, retry, silent Pending queueing, or invented transfer cancellation.
- Correct separation of synchronous send **admission** from asynchronous transfer **outcome/acknowledgment**.
- Atomic multi-recipient broadcast admission under the chosen capacity policy.
- Shared outgoing memory reservations and incoming reassembly memory reclamation on every termination path.
- Page framing, malicious/duplicate fragments, replay protection, transfer timeouts, handshake, and peer isolation.
- Cross-thread read/write, callback reentrancy, asynchronous borrowed-data lifetime, and shutdown races.
- Linux/Windows backend parity, transport-only scope, real-thread and real-socket tests.

**Do not silently change PR #29 decisions based on a generic principle in this guide.** Where the PR specifies a concrete transport policy, honor that policy; report concerns separately.

---

## 10. Quick decision checklist for AI-generated code

Before submitting a patch, answer **yes** to these questions or explicitly explain the exception:

1. Can each new class/trait be described in one *meaningful* responsibility sentence?
2. Does every abstraction justify its dependencies, invariant, or independent reuse?
3. Are API names, types, state transitions, policies, and ownership understandable at the call site?
4. Are constructors, moves, copies, borrows, callbacks, and resource lifetimes coherent?
5. Are normal failures distinguishable from invalid operations and programmer mistakes?
6. Are locking, cross-thread visibility, immutable publication, and cleanup paths safe?
7. Are public APIs tested, with real-thread/real-socket tests when behavior depends on concurrency or I/O?
8. Did the change stay inside the approved scope?
9. Are code formatting and verification results tied to the final changed commit?
10. Were nontrivial tradeoffs surfaced instead of guessed?

### Anti-patterns to avoid

- “Abstract everything” or “make everything templated.”
- Split methods/classes solely because they are longer than an arbitrary limit.
- Introduce a `detail` namespace, generic `Manager`, or forwarding `Service` without semantic justification.
- Introduce a fake interface solely because mocking is desirable.
- Make every setter fluent or wrap every STL container.
- Hide consequential API policy decisions behind default side effects or unnamed booleans.
- Automatically copy async data that the explicit API contract says remains borrowed.
- Repeat input validation in every private method despite an established boundary invariant.
- Catch-and-log an exception at every layer, losing the original cause or double-reporting it.
- Update unrelated code because a cleaner future design is imaginable.
- Claim a CI result from a different head, or claim a test was run when it was not.

**Bottom line:** design like an object-oriented library author, not a collection of procedural wrappers. Make the meaningful responsibilities small and composable, preserve explicit contracts, and deliver a tested, readable change of the right scope.
