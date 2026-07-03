# WASI-NN Plugin: Ownership and Lifetime Design

This note describes how the WASI-NN plugin manages graph and execution-context
lifetimes, what the concurrency rules are, and what a backend author must (and
must not) do. It reflects the shared-ownership redesign of the core.

## Ownership model

```text
ResourceTable<Graph>   NNGraph   --shared_ptr-->  Graph   (variant payload,
ResourceTable<Context> NNContext --shared_ptr-->  Context  status, op mutex)
                                                     |
                                                     +-- shared_ptr<Graph>
```

- Guest-visible handles are opaque `uint32_t` keys into two `ResourceTable`s
  (`resource_table.h`). Handles are allocated monotonically and **never
  reused**: a stale handle stops resolving instead of aliasing a resource
  created later.
- Every host operation resolves its handle to a `shared_ptr` and holds it for
  the whole call. An in-flight operation therefore structurally cannot lose
  its graph or context to a concurrent `unload`/`finalize_execution_context`.
- A `Context` owns a `shared_ptr` to its parent `Graph`, so a live context
  keeps the model alive to drain after the guest unloads the graph.
- Backend resources are released by the payload destructors (RAII) at the last
  `shared_ptr` release — never under a lock, and on whichever thread drops the
  final reference. There are no per-backend `unload`/`finalizeExecCtx`
  functions.

## Publish discipline

`load` and `init_execution_context` fully construct the wrapper and its
backend payload **before** inserting it into a table. A table entry is always
a complete, usable resource; a failed load simply never publishes and the
discarded wrapper cleans itself up through the payload destructors. There is
no observable "uninitialized" state and no rollback bookkeeping.

## Status model

`GraphStatus` (atomic, on the wrapper): `Ready`, `Invalid` (a set_input
metadata reload failed; reloadable), `Detached` (unloaded; drain-only).
`Detached` is terminal: `setReady`/`setInvalid` CAS and give up once they
observe it, so a reload that was in flight when the guest unloaded cannot
resurrect the graph. Contexts need no status: they are only visible fully
built.

Each host op declares the graph status it requires in `hostOpPolicy()`
(`wasinnenv.h`) via `GraphReq`:

| Op | Tier | Meaning |
|---|---|---|
| init_execution_context | Ready | needs a usable model |
| compute / compute_single | Ready | needs a usable model |
| set_input | NotDetached | admits Invalid for metadata reload |
| get_output | Any | drains a live context regardless of its graph |
| get_output_single | Drainable | reads the model, so Ready or Detached |
| fini_single | Any | resets context-owned streaming state |

The table is the single place the drain-after-unload contract lives; a new op
must add itself there.

## Concurrency rules

- The table mutexes only guard the id maps (nanosecond critical sections).
- Each `Graph` carries one `std::mutex` (`opMutex()`); `withGraphOp` /
  `withContextOp` take it around every backend dispatch, so all operations on
  one graph — including operations on different contexts of that graph — are
  serialized. This is required because several backends keep mutable inference
  state on the graph payload (e.g. the GGML `llama_context`).
- Status is re-checked after acquiring the op mutex, closing the race with a
  concurrent unload or a failed set_input reload.
- `unload` / `finalize_execution_context` do **not** take the op mutex: they
  detach the handle immediately and let the last release run the destructor.
  An unload during a long compute returns at once; memory is reclaimed when
  the compute finishes.
- Lock order: `MdMutex` before a table mutex (only in `mdGet`). The op mutex
  is never held while acquiring either. `RawMdMap` (preloaded model bytes) is
  immutable after construction and read without locks; `MdMutex` guards only
  the name→handle map and is never held across a model load.
- Global third-party log callbacks (llama.cpp, mtmd, whisper.cpp) are bound
  once with **no user pointer** and gated by ref-counted atomic counters
  (`wasinn_log_gate.h`): each graph that wants logs holds a RAII `LogToken`.

## Writing a backend

Backend entry points receive resolved wrapper references and must not touch
the tables (they cannot — no table API is exposed to them):

```cpp
Expect<WASINN::ErrNo> load(WasiNNEnvironment &Env, WASINN::Graph &G,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WasiNNEnvironment &Env, WASINN::Graph &G,
                                  WASINN::Context &C) noexcept;
Expect<WASINN::ErrNo> setInput(WasiNNEnvironment &Env, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WasiNNEnvironment &Env, WASINN::Graph &G,
                                WASINN::Context &C, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WasiNNEnvironment &Env, WASINN::Graph &G,
                              WASINN::Context &C) noexcept;
```

Rules of thumb:

- Resolve payloads once at the top: `auto &GraphRef = G.get<Graph>();`,
  `auto &CxtRef = C.get<Context>();`.
- `load` populates `GraphRef` and returns; the core publishes on success. Do
  not allocate ids, set ready flags, or clean up on failure.
- Own every resource in the payload structs via smart pointers or explicit
  destructors. Value-initialize raw members so destroying a partially built
  payload frees nothing invalid. If you add a destructor, delete the copy
  operations (and re-default the default constructor for `Graph` payloads —
  the core default-constructs them).
- A failed set_input metadata reload marks the graph with `G.setInvalid()`;
  a successful one restores it with `G.setReady()`. Both are no-ops on a
  `Detached` graph, so a reload finishing after an unload cannot undo it.
- Never register a global callback with a pointer to your payload; use a
  ref-counted gate like the llama/whisper log gates.
- Your ops run under the graph's op mutex: you may assume no other op touches
  the same graph concurrently, and you must not block on other graphs.

## Known limitations / future work

- Per-graph serialization costs backends with genuinely independent
  per-context state (e.g. OpenVINO `InferRequest`) their intra-graph
  parallelism. A per-backend concurrency trait could relax the dispatch to a
  per-context mutex for those backends.
- `mdBuild` runs concurrent same-name loads independently; the last one wins
  the name cache and the loser stays reachable only through its returned
  handle. Per-name build deduplication would remove the redundant load.
- Handles exhaust after 2^32 loads per table; `insert` then fails and the
  host call reports `RuntimeError`.
