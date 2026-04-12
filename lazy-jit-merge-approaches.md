# Lazy JIT cumulative IR merge — approaches tried and recommended directions

This note summarises work on the **lazy JIT** path that **links** each lazily compiled LLVM module into a **cumulative** module, then **reloads** a fresh Orc `LLJIT` with the merged IR (to address **Darwin / JITLink** issues with incremental `addLLVMIRModule`). It captures **what was tried**, **what failed or partially worked**, and **what is most likely to fix** the remaining failures.

---

## Problems observed

1. **PIC / module flags at IR link time**  
   Error: `linking module flags 'PIC Level': IDs have conflicting behaviors in 'wasm' and 'wasm'`.  
   **Cause (LLVM behaviour):** `IRMover::linkModuleFlagsMetadata` runs `UpgradeModuleFlags` on the **source** module before merging. That can change flag *behaviour* vs the destination, so a naive copy of flags from the cumulative module onto the lazy module still diverges after upgrade.

2. **Semantic regressions after merge reload**  
   Spectests reported **out-of-bounds memory**, **undefined element**, **validation noise**, line mismatches — consistent with **wrong code pointers**, **wrong intrinsics / globals**, or **stale `Executable` lifetimes**.

3. **`loadExecutable` + lazy `getCodes`**  
   For lazy JIT, failed symbol lookups return **empty** symbols. A full **`loadExecutable`** overwrites **every** code-segment symbol. Clearing previously valid symbols for functions that still have no JIT symbol leaves **null** AST symbols; **refresh** then skips (`if (!SymV) return`) while **`FunctionInstance`** may still hold **stale native pointers** into a **freed** JIT generation unless the old `Executable` is kept alive via `Symbol`’s `shared_ptr` to the library.

4. **Multiply-defined symbols at link**  
   Each `compileFunction` builds a **full** module (infrastructure + declarations + one body). Linking that into the cumulative module can produce **`symbol multiply defined`** for the same wasm function (e.g. `f25`) when both sides carry a definition.

5. **`Linker::OverrideFromSrc`**  
   Resolves function conflicts by preferring the lazy module, but the same flag applies to **other** linkages — it can **override cumulative globals** (memory, tables, intrinsics-related globals) with the lazy copy, breaking a single coherent module.

6. **Erasing the old function from the cumulative module**  
   Removing the previous `Prefix+fN` before link avoids duplicate **function** definitions, but **`Function::eraseFromParent()`** on a function that still has **uses** (including **metadata**) is invalid and can **crash** (e.g. `LazyJITCoreTest` suite `/1` **SIGSEGV** when the merge path was enabled, while disabling the merge path and using **`JIT::add` only** made `/1` **pass**).

7. **Full-suite run**  
   Running all `LazyJITCoreTest` cases exited early with **SIGSEGV** in some environments when the merge path was enabled — merge path stability is not yet proven across the matrix.

---

## Approaches tried

| Approach | Intent | Outcome |
|----------|--------|---------|
| **Copy cumulative module flags onto lazy** (`getModuleFlagsMetadata` + `addModuleFlag`) | Align PIC / PIE behaviour before `linkInModule` | Still failed: **UpgradeModuleFlags** mutates source after copy. |
| **Strip lazy module flags entirely** | Skip merging conflicting flags (empty source → early exit in `linkModuleFlagsMetadata`) | **PIC error resolved** for the failing case. |
| **`loadModule` merged JIT with `IsLazy = false`** | Surface missing symbols instead of silent empty | Showed many **symbols not found** for unmaterialised functions (expected for 0-BB stubs); not a viable strict mode for lazy codegen. |
| **Preserve AST code-segment symbols when new lazy symbol is empty** (`loadExecutable` + `Exec->isLazy() && !CodeSymbols[I]`) | Avoid clearing valid pointers / keep old `Executable` alive via existing `Symbol` | Plausible for **OOB** mitigation; needs validation alongside merge correctness. |
| **`Linker::OverrideFromSrc` on every link** | Resolve multiply-defined functions | Risk: **overrides globals** → memory / table corruption → **OOB** / wrong behaviour. |
| **Prune lazy module: drop duplicate functions** (keep only `Prefix+f{GlobalFuncIndex}`, remove other `f*` that already exist in cumulative) | Reduce multiply-defined **functions** without global override | Helps **multiply-defined** on functions; must be paired with safe link flags. |
| **Erase old `fN` from cumulative before link** | Let link add the new definition without override | **SIGSEGV** when the function still has uses; **unsafe** without clearing uses / metadata first. |
| **Conditional: erase if `use_empty()`, else `OverrideFromSrc`** | Avoid erase when IR still references the old function | Merge path still **crashed** test `/1` in the session — needs more investigation (crash site may be elsewhere: JIT materialisation, bad IR after link, etc.). |
| **Disable merge path (`JIT::add` only)** | Sanity check | **Test `/1` passed** — confirms **merge path** is implicated in at least one **hard crash**, not only soft test failures. |

---

## Root causes (concise)

1. **Module flags:** Do not fight `UpgradeModuleFlags` on the source; **empty lazy flags** (or a single consistent strategy) is the robust fix for the PIC merge diagnostic.  
2. **Full lazy module link:** Re-emitting **full** wasm LLVM state per `compileFunction` duplicates **functions and globals**; the linker needs either **override rules that don’t clobber memory**, or **a thin module** containing **only the new function** (and remapped references to cumulative globals).  
3. **Executor / loader contract:** After replacing `Exec`, **either** every refreshed function gets a valid symbol from the new JIT **or** old symbols must remain valid because the old `Executable` stays referenced — **preserving non-empty symbols** when the new lookup is empty is aligned with that.  
4. **Darwin vs incremental `add`:** **Linux** can likely stay on **`JIT::add`** for lazy chunks if stable; **Darwin** may still need a **merge or different object-model strategy** — but the **merge** implementation must not corrupt IR or crash.

---

## What is most likely to fix the errors (ordered)

1. **Thin lazy artifact (compiler + link)**  
   Emit **only** the lazily compiled function (and truly private comdat-internal helpers) into the lazy IR, with **references** to **extern** or **linkonce_odr** symbols that match the **cumulative** module’s memory / table / intrinsics. Then **link** without blanket **`OverrideFromSrc`**, avoiding global clobber and multiply-defined stubs.  
   *This is the structural fix; it is more work but matches how LTO/thin imports behave.*

2. **If full-module link must stay short-term**  
   - Keep **stripped lazy module flags** (PIC fix).  
   - **Prune** duplicate **functions** from the lazy side as today.  
   - **Never** use global **`OverrideFromSrc`** unless you can prove globals won’t conflict — or **strip / remap duplicate globals** from the lazy module before link (requires **in-module** use rewriting or **IRMover**-style moves, not naive cross-module `replaceAllUsesWith`).

3. **`loadExecutable` policy for lazy reload**  
   **Skip** `setSymbol` when the new code symbol is empty **and** you intend to retain the previous native pointer / keep the previous `Executable` alive (already partially explored).

4. **Replacing a function in cumulative IR**  
   Prefer **LLVM-supported** patterns: **link with override only for that GV**, or **erase + link** only after proving **no uses** (including **debug metadata**), or **replace function body** in place instead of erase+link.

5. **Platform split (pragmatic)**  
   **`#ifdef __APPLE__`**: merged reload (once safe). **Else**: **`JIT::add`** only for lazy chunks — restores **Linux** CI behaviour and isolates **Darwin** complexity.  
   *Use only if thin IR or safe link is not ready yet; product may still require Darwin merge eventually.*

6. **Testing**  
   Run **`LazyJITCoreTest` /1** and **/25** with merge on after each change (crash vs OOB vs PIC). Use **`lldb`** / **assert builds** to pin **SIGSEGV** inside link vs JIT vs executor.

---

## References in tree (for implementers)

- `lib/llvm/jit.cpp` — `prepareLazyModuleForLink`, `pruneLazyModuleForCumulativeLink`, `linkLazyModuleIntoCumulative`, `lazyJITReloadAfterLink`  
- `lib/vm/vm.cpp` — `unsafeLazyCompileFunction` merge vs `JIT::add`  
- `lib/loader/ast/module.cpp` — `Loader::loadExecutable` and lazy code symbols  
- LLVM: `IRLinker::linkModuleFlagsMetadata` (UpgradeModuleFlags on source), `ModuleLinker` / `OverrideFromSrc` semantics  

---

*Written as a working summary of investigation; adjust as the implementation lands.*
