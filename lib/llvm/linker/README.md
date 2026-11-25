# Native AOT linker

The AOT compiler emits one object file. The native linker in this directory
links that object into a shared library or a universal Wasm file without lld.

## Select the linker

| Build configuration | Default linker | How to select the native linker |
| --- | --- | --- |
| `WASMEDGE_USE_LLD=ON` (default) | lld | `wasmedgec --enable-native-linker`, or `setNativeLinker(true)` on the compiler configuration |
| `WASMEDGE_USE_LLD=OFF` | native linker | Always used. lld is not built or required. |

## Supported hosts and outputs

The native linker links only for the build host. The output format comes
from the host operating system.

| Host | Architectures | Shared library | Universal Wasm |
| --- | --- | --- | --- |
| Linux | x86-64, AArch64, RISC-V 64, s390x | ELF | Yes |
| Linux | ARMv7 (EABI version 5 with VFP) | ELF | Yes |
| macOS | x86-64, arm64 | Mach-O, signed with `/usr/bin/codesign` | Yes |
| Windows | x86-64, ARM64 | PE DLL | Yes |

AOT compilation with lld supports the same hosts. Other hosts, for example
32-bit x86, have no AOT target for either linker. On 32-bit x86, CMake stops
with an error unless you configure with `-DWASMEDGE_USE_LLVM=OFF`.

On ARMv7, a native stack trace does not walk through AOT code that the loader
reads from a universal Wasm file. The universal Wasm file keeps the ARM
exception index table as plain data, and the loader does not register it with
the unwinder. A shared library has a `PT_ARM_EXIDX` program header, so the
system unwinder finds that table.

## Output files

The native linker replaces an existing output file in the same way as lld:

- A shared library goes to a temporary file, which then replaces the output
  by rename. Other hard links to the old file keep the old contents.
- A universal Wasm file that has other hard links is written over the
  existing file, so all of its names show the new output.
- An output that is not a regular file, such as a named pipe, receives the
  output directly. A Mach-O output must be signed, so the native linker
  rejects a Mach-O output that is not a regular file.

## ARM float requirements

On ARMv7 Linux, the native linker accepts the hard-float and the softfp ABI.
Both need VFP. The native linker does not support a WasmEdge build for the
pure soft-float ABI. AOT compilation then fails with this error:
`ARM pure-soft float ABI is unsupported by the native linker.`

The hard-float and softfp ABIs pass floating-point values in different
registers, so a universal Wasm file records the ABI in its architecture tag.
Tag 4 is softfp, and tag 6 is hard-float. A universal Wasm file from an earlier
WasmEdge release always carries tag 4. If the tag does not match the host ABI,
the loader ignores the AOT code and runs the module in the interpreter.

The native linker links no runtime library such as libgcc. Thus, when the
native linker links an ARMv7 object, the compiler adds its own helpers for
integer division, float conversion, and rounding to the object. On x86-64
hosts without SSE4.1, it adds rounding helpers for the same reason.

## Input restrictions

The native linker is not a general linker. It accepts only the object that the
WasmEdge LLVM compiler emits. For other objects, it gives a diagnostic that
names the section, the symbol, or the relocation type.

- The input is exactly one relocatable object. Archives and extra objects are
  not supported.
- Undefined symbols are rejected. The exceptions are the `_fltused` marker
  that MSVC-style COFF objects reference and the ARM exception-handling
  personality markers that `R_ARM_NONE` references.
- Each relocation type must be one that the target relocation table lists.
  `utils/llvm/collect-aot-relocations.sh` reports the relocation types that an
  LLVM version emits.
- Only AArch64 gets branch thunks. On ARMv7, a branch to a target more than
  32 MiB away in ARM code, or 16 MiB away in Thumb code, fails with an error.
  A `R_ARM_JUMP24` branch that changes between ARM and Thumb state also fails.
- Universal Wasm output does not support absolute address fixups, because the
  universal Wasm loader applies no relocations. Shared library output stores
  these fixups as dynamic relocations.

## When to keep lld

Keep lld available if a new LLVM version or a custom compiler option makes
LLVM emit a relocation, a section, or an undefined symbol that the native
linker rejects. The AOT compilation then fails with an error, so the output is
never wrong silently. Rebuild with `WASMEDGE_USE_LLD=ON`, and report the error
message.
