# WasmEdge Stable Diffusion Plug-in: Ownership, Lifetime, and Concurrency Design

This note is for maintainers of the plug-in. `ABI.md` is the contract with
guests. The comments in the code give the reason for each implementation
decision. "The library" is the pinned stable-diffusion.cpp.

## Layers

```text
ABI layer       sd_func, sd_abi, sd_guest_file     the six host functions; guest memory and guest
                                                   paths in, error codes out; nothing outlives a call
lifetime core   sd_session, sd_request, sd_image   sessions and their states, typed requests, images,
                                                   results; knows neither guest memory nor the library
backend seam    sd_backend.h                       the interface the core calls
                sd_backend_sdcpp, _bind            the adapter for the library
                sd_adapter_check                   what a LoRA or embedding file must look like
```

- Only the adapter files and `sd_env.cpp`, which creates the real backend,
  include the library header.
- All sources except `sd_env.cpp` and `sd_backend_sdcpp_bind.cpp` form a
  static core library. The plug-in and the test binary link it.
- The tests drive the core with a fake backend, and the adapter with a
  scripted `LibraryApi`. They reach the loaded plug-in only through its six
  host functions.

## Ownership model

| Lifetime | Object | Owner | Released |
|---|---|---|---|
| Process | the library lock; the log and progress callbacks | the adapter | never |
| Module instance | `SDEnvironment { backend, creation time, slot count, SessionTable }` | `SDModule` | in its destructor: all sessions first, then the backend |
| Session | `Session { SessionSlot, BackendContext, Capabilities, results, encoded PNGs, poisoned flag, operation lock }` | the table (`shared_ptr`) and each call that holds it | at the last reference: `session_close`, teardown, or the end of a call in progress |
| Adapter context | `SdcppContext { FilePolicy, library context, upscaler, Capabilities, embeddings }` | the session | with the session |
| Results | `ImageBatch`, plus one `EncodedPng` per image, made on first use | the session | at the next `generate` that reaches the backend, or with the session |
| Library handle | an option object, a tensor, a result | `Owned<T>`; its deleter calls the free function of the library under the library lock | at scope exit |
| Call | texts, blob spans, requests, host paths, decoded inputs | the frame of the host function | at scope exit |

- A session is published only when it is complete. Its id is never used
  again (`Plugin::ResourceTable`).
- A module instance has `Limits::MaxSessions` sessions at most.
  `session_open` takes a `SessionSlot` before the backend creates the
  context. The session gives the slot back after its context is freed.
- A session uses almost no memory until its first generation. After that, it
  keeps its models until it is released. Two sessions on one file keep two
  copies.
- No guest data stays after a call: no guest pointer, no guest path, and no
  descriptor on a guest file. The backend gets host paths only.
- Declaration order gives destruction order: `SDEnvironment` declares the
  backend and the slot count before the table, `Session` declares its slot
  before its context, and `SdcppContext` declares its file policy before the
  library context.
- Manual allocation occurs only in `sd_image.cpp`, the library lock of the
  adapter, and the module factory in `sd_env.cpp`.

## Session states

`Session` (`sd_session.cpp`) stores `Ready` or `Poisoned`, and its results.
`Busy` means that the operation lock is held. `Closed` means that the id is
not in the table. `ABI.md`, section 2.9, has the table of calls, states, and
codes.

- `Session::generate` takes the operation lock, decodes the input images,
  and then drops the results, before the backend runs. Thus a session never
  holds two batches, and a call that waits for the lock holds no decoded
  image.
- A refusal by the host function (request, capability, LoRA path, input
  image) comes before the drop and keeps the previous results. A refusal by
  the adapter (prompt rule, LoRA file check) comes after it and leaves no
  results.
- `Session::generate` passes `InvalidArgument` and `LoadFailed` from the
  backend to the guest. Every other code becomes `RuntimeError`.
- `Poisoned` has one cause: an exception crossed a library call. The library
  state is then unknown. The session refuses more work and keeps its models
  until it is released.
- `Capabilities` never changes after the session is built. A read needs no
  lock.

## The backend seam

`Backend::open` returns a `BackendContext` for one model set.
`BackendContext::generate` returns an `ImageBatch` or a `BackendFailure`,
which tells if the context is still usable (`sd_backend.h`). Every
implementation must obey these rules:

- No exception leaves it. If a library call throws, the context becomes
  unusable, and only frees are called after that.
- The interface has no library type and no library key.
- It serializes its own library calls as the library needs.
- Inputs are borrowed for the call. Outputs are owned values.
- It never gives library text to the guest, and it keeps the library off
  stdout.

`sd_backend_sdcpp.cpp` calls the library only through `LibraryApi`, a struct
of function pointers with the types of the library header.
`sd_backend_sdcpp_bind.cpp` fills it with the real functions, and the tests
fill it with scripted functions.

## Concurrency rules

| Lock | Guards | Held |
|---|---|---|
| the table mutex (`ResourceTable`) | the id map | inside `insert`, `get`, `remove`, `size`; no plug-in code runs under it |
| the operation lock of a session (`Session::Op`) | the results, the encoded PNGs, the poisoned flag | by `generate`, `resultInfo`, and `resultRead` for the whole call, including the library call |
| the name-check lock (`SdcppBackend::NamesLock`) | the result of the name check | by every `open`; for the whole check at the first `open` |
| the library lock (`libraryLock()`) | every library call | around each library call, including frees |

- Lock order: the operation lock before the library lock; the name-check lock
  before the library lock. No other two locks are held together.
- `session_close` takes no operation lock and never waits for a call on its
  own session. If no call holds the session, it frees the models before it
  returns. That free waits for the library lock.
- The library callbacks (`logBridge`, `ignoreProgress`, `allowFile`) take
  none of these locks. The first two are process-wide. They are registered
  only when the library lock is free, so the creation of a module instance
  never waits for a generation. `allowFile` gets the file policy of its
  context, which outlives the library context.
- File resolution runs without these locks. The input images of a
  `generate` are decoded under the operation lock of its session.
- `ABI.md`, section 2.8, gives the result for guests.

## Files

Every path in the ABI is a guest path. `sd_guest_file` opens it with the
filesystem of the calling module and takes the host path that the kernel
reports. It refuses the file with `NotReadOnly` if a descriptor that the WASI
environment started with could have changed the file. This check needs
`WASI::Environ::getInitialDescriptors()`, the only addition of this plug-in
to the core. The comments in `sd_guest_file.h` and `sd_guest_file.cpp` give
the rule and its reasons. `ABI.md`, section 6, describes it for guests.

## What can still end the host process

No number, name, or prompt text of a request is known to end the process.
This result comes from the pinned library and the SD 1.5 fixture of the
contract tests. Other model families are not tested. These classes remain:

| Class | Bound |
|---|---|
| a file with tensors that fail a library assertion | The guest can name only files that it could never change, so the embedder controls the files. `sd_adapter_check` accepts only the LoRA and embedding layouts that it has rules for. |
| a LoRA that does not fit a model that is quantized in its file | The adapter merges LoRAs into weights that are known to be plain, and the library then skips a pair that does not fit. A session with a quantized `weight_type` accepts no LoRA. For a model that the embedder supplies quantized, the fit is not checked. |
| host or device memory is exhausted | The request limits and the session limit bound what a guest adds. The embedder controls the size of the models and of the machine. The memory of one generation grows faster than its pixel count: about 17 GiB above the weights for 1024x1024 with an SD 1.5 model on the CPU. |
| a CPU without the instructions of the build | The build does not use the CPU of the build machine. An x86_64 build needs SSE 4.2, AVX, AVX2, BMI2, FMA, and F16C. The plug-in does not check the CPU. |
| a defect in the library | The pin is exact. CI runs the contract tests on the CPU builds only, not on the library code of the CUDA and Metal assets. |
| a file changed after the check, from outside the WASI environment of the guest | The plug-in cannot see it. |

The embedder must do these things:

- Preopen the directories of models, LoRAs, and embeddings with `:readonly`,
  outside every guest-writable directory. Put the files in place as copies
  before the VM is created.
- Do not load a plug-in that writes host paths for a guest without the
  filesystem of the caller.
- Do not give a guest a second, writable view of the model files: the lower
  directory of an overlay, a share that is mounted twice, a directory with a
  second name (HFS+), or a file system that mounts on demand below a writable
  directory.
- While a guest runs, do not move directories or change mounts below a
  guest-writable directory, and do not set the clock back. The rule relies on
  file change times, so the file system must keep them from the local clock.
- Use a new WASI module for each guest run.
- For a model that is quantized in its file, supply only LoRA files made for
  that model.
- Start the host with the descriptors 0 to 2 open. A file that gets one of
  these numbers counts as a standard stream that the guest can write, and the
  plug-in then refuses files.
- Set `GGML_NO_BACKTRACE` in the environment of the host. Without it, a
  failed ggml assertion runs a debugger against the host on Linux before it
  aborts.

## Moving to a newer revision of the library

A bump must change only the adapter, not the ABI. Before you accept a new
revision:

1. Make sure that the pin in `CMakeLists.txt` is the full hash and that the
   adapter compiles against the new header.
2. Read again what the compiler does not check: the option keys that the
   adapter sets, the field order of `sd_lora_t` and `sd_embedding_t`, the
   shape convention of `sd_tensor_create`, the placement syntax, the
   weight-type spelling, and the statuses that `codeOf` maps.
3. Make sure that `wasmedgeStableDiffusionTests` passes in full. If a name
   table of `sd_abi.h` no longer maps, `checkNames` fails every `open`. If a
   scheduler returns no image at a step count
   (`EverySamplerAndSchedulerReturnsAtOneAndTwoSteps`), give it a minimum in
   `kSchedulerMinSteps`. Read again how the library chooses the scheduler
   when a request names none.
4. Read the prompt handling of the library again: the markers that name
   files (`namesAFile`), the characters that it does not keep in an embedding
   path (`notKeptByTheLibrary`), and how it finds the mentions of an
   embedding (`mentionedRows` must never count fewer).
5. Read the file access of the library again. The policy (`allowFile`)
   compares the exact host paths from the plug-in. If the library opens a
   file without the callback, correct that first.
6. Compare the code that loads LoRA and embedding files with
   `sd_adapter_check`. Try a newly accepted layout against the library alone
   first. Also check what makes the library treat weights as quantized, how
   it chooses between a merge and a run-time LoRA, and what each method does
   with a pair that does not fit (`hasPlainWeights`,
   `asksForQuantizedWeights`).
7. Make sure that the ranges of `ABI.md` are within the limits of the
   library. `MaxClipSkip` is the layer count of the smallest CLIP text
   encoder. Compare the per-model defaults of the sampler parameters that the
   ABI does not expose with the valid range of each sampler (the adapter sets
   `eta` for `tcd`).
8. Read the log lines of the library from `INFO` up, its last-error texts,
   and its exception texts again for prompt text, because they go to the host
   log (`NoPromptReachesTheHostLog`). Make sure that the library still writes
   nothing to stdout (`OnlyThePluginsOwnLogLinesReachStdout`).
9. Check the upstream-ggml switch in `CMakeLists.txt` again: does the CUDA
   extension of the library compile with the CI toolkits, and do its Metal
   kernels still need Objective-C++ for the whole project?
10. Compare the bundled stb (`thirdparty/stb_image.h` and `stb_image_write.h`
    in the library tree) with the previous pin, run `SDImageTest`, and make
    sure that `GGML_NO_BACKTRACE` still disables the debugger of ggml.
11. Make sure that ggml still builds for the fixed CPU features of
    `CMakeLists.txt`. The configure output lists the flags of its CPU
    backend. A new or renamed x86_64 switch of ggml needs a line there.
