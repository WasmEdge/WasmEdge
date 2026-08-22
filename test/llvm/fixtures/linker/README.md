# AOT Object Relocation Inventory

The native object linker inventory records the relocations and disassembly
produced by representative AOT compilations. Each fixture should be collected
for this matrix:

| Optimization | CPU target | Interruptible |
| --- | --- | --- |
| O0 | generic | normal |
| O0 | generic | interruptible |
| O0 | tuned | normal |
| O0 | tuned | interruptible |
| O2 | generic | normal |
| O2 | generic | interruptible |
| O2 | tuned | normal |
| O2 | tuned | interruptible |

The inventory covers scalar operations, direct and indirect calls, globals,
tables, memory, SIMD, atomics, and exceptions.

Cross-target tests use explicit tuned CPUs rather than claiming they represent
the build host: Cortex-A8, Cortex-A53, generic-rv64 with the A extension, and
z13. ARM exceptions use a defined cantunwind table because standard LLVM ARM
personality tables import `__aeabi_unwind_cpp_pr0`, which the linker rejects.

Run `utils/llvm/collect-aot-relocations.sh` manually for each configuration,
using a distinct output directory for every report set:

```bash
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O0-generic-normal --optimize 0 --generic-binary
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O0-generic-interruptible --optimize 0 --generic-binary --interruptible
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O0-tuned-normal --optimize 0
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O0-tuned-interruptible --optimize 0 --interruptible
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O2-generic-normal --optimize 2 --generic-binary
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O2-generic-interruptible --optimize 2 --generic-binary --interruptible
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O2-tuned-normal --optimize 2
utils/llvm/collect-aot-relocations.sh build/tools/wasmedge/wasmedgec input.wasm inventory/O2-tuned-interruptible --optimize 2 --interruptible
```

Each output directory contains `object.txt` and `disassembly.txt`. Do not
commit generated reports, AOT modules, object files, or other binaries. The
output directory must be absent or empty; the script refuses to overwrite a
non-empty directory so reports from different configurations cannot be mixed.
