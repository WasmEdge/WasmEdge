# Debugging WasmEdge AOT and JIT code with gdb and lldb

Compile the wasm module with DWARF debug info, for example with
`clang -g` or `rustc -g`. Then turn on native debug info in WasmEdge.

For AOT, compile the module with `wasmedgec` and run the shared library:

```bash
wasmedgec --debug-info --optimize 0 app.wasm app.so
gdb --args wasmedge --run-mode=aot --reactor app.so main
```

For JIT, pass `--debug-info` to `wasmedge`. If the module has the
`.debug_info` and `.debug_abbrev` sections, this option also makes the JIT
compile the code without optimization:

```bash
gdb --args wasmedge --run-mode=jit --debug-info --reactor app.wasm main
```

On macOS, lldb reads JIT debug info only when its JIT loader is on. Run this
command in lldb before you start the process:

```text
settings set plugin.jit-loader.gdb.enable on
```

## Optimization levels

Use `--optimize g` for optimized AOT code that you can still debug:

```bash
wasmedgec --debug-info --optimize g app.wasm app.so
```

`--optimize g` runs the same optimizations as `--optimize 1`. With
`--debug-info`, it also keeps these values available:

- Wasm locals that hold source variables. The compiler keeps each value
  alive until the end of its source scope. For a variable in a block, this
  is the end of the block. For other variables, this is the function
  return. If code jumps out of a block, for example with `break`, the value
  can become unavailable before the jump.
- Variables on the wasm shadow stack.
- `__wasm_memory`.

Some values can still show as `<optimized out>`. For example, a value can
be unavailable between two uses. The compiler can also move or merge code.
Then a breakpoint on a line can stop on a nearby line.

Use `--optimize 0` to see all values. At levels 1, 2, 3, s, and z, variables
on the wasm shadow stack and `__wasm_memory` show as `<optimized out>`.
Other optimized code can also show `<optimized out>`.

## Helpers

Load the helper for your debugger.

gdb: `source utils/debug/wasmedge_gdb.py`

lldb: `command script import utils/debug/wasmedge_lldb.py`

| Helper | gdb | lldb |
|--------|-----|------|
| Follow a wasm pointer | `print *$wasm(p)` | `wasm p` |
| Read a C global for the current instance | `print $wasm_global("name")` | `wasm-global name` |
| Pretty-print wasm pointers | automatic | automatic |

In gdb, the pretty-printer shows a wasm pointer as
`(wasm) 0x<address> -> <value>`. It shows the target one level deep. To see
the raw fields, use `print -raw-values -- p`. This option needs gdb 10 or
newer.

In lldb, the summary shows a wasm pointer as `(wasm) 0x<address>`, or
`(wasm) <unavailable>` if lldb cannot read it. The child `*` holds the
target. lldb expands only the first wasm pointer, so a cyclic list does not
expand without end. To follow a nested pointer, use `wasm p->next`. To see
the raw fields, use `frame variable --raw-output p`.

Both helpers find `__wasm_memory` in the selected frame. If the selected
frame is an inlined function, they also look in the function that it is
inlined into. They do not look in callers, because a caller can belong to
another instance with another memory. If no frame has a value, select the
frame of a wasm function, for example with `up`.

## How wasm values appear

- A wasm pointer appears as a struct with the name `__wasm_ptr<T>`, where
  `T` is the type it points to. The field `__addr` is
  the offset in linear memory. The field `__pointee` is a zero-length array
  of native pointers. It only carries the pointee type. Its value is not
  meaningful.
- `__wasm_memory` in each function is the host address of memory 0.
- Variables on the wasm shadow stack and in linear memory show their real
  values.
- A global that the wasm linker removed shows as `<optimized out>`.

## Limits

- The gdb helper works on Linux. The lldb helper works on Linux, macOS, and
  Windows. Both helpers work with AOT shared libraries. On Linux and macOS,
  they also work with JIT.
- On Windows, the JIT gives the debugger no debug info. Use AOT to debug
  on Windows.
- On macOS, the JIT gives the debugger debug info only with LLVM 15 or
  newer.
- Where the JIT gives the debugger no debug info, `--debug-info` prints a
  warning and has no effect on JIT code. The JIT keeps its usual
  optimization level there.
- On macOS, the AOT compiler keeps the debug info in an object file next to
  the library, named `<name>.<random>.o`, where `<name>` is the library file
  name without its extension. After a successful build, it deletes the
  object file that the replaced library records, but only if the replaced
  library is a WasmEdge library and the object file is unchanged since the
  link. If `ZERO_AR_DATE` is set, the linker records no modification time,
  so the object files stay. If a build fails, with or without debug info,
  the earlier library and object file stay. This cleanup is best effort when
  separate processes build the same library at the same time: the object
  file of one build can stay. The library records the absolute path of the
  object file, so do not move or delete it. To move the library, run
  `dsymutil <library>` first, and move the `.dSYM` bundle with it.
- On macOS, a failed `codesign` does not fail the build. The compiler logs
  the failure. The library then keeps the ad hoc signature of the linker,
  which signs arm64 output. x86-64 code runs without a signature.
- Universal wasm output gets no native debug info. The file still holds
  the wasm DWARF sections of the input module, because it keeps the
  original wasm bytes.
- Debug info sends the DWARF sections of the module to the LLVM DWARF
  parser. Use it only with modules that you trust.
- WasmEdge reads only the DWARF sections inside the module. It does not
  read split DWARF or a separate debug file. A variable on the wasm operand
  stack shows as `<optimized out>`.
- LLVM and the WebAssembly DWARF convention give different meanings to a
  `DW_OP_WASM_location` without `DW_OP_stack_value`. For LLVM, the wasm
  local or global holds the memory address of the variable. For the
  convention, it holds the value. WasmEdge uses the LLVM meaning when the
  `DW_AT_producer` of the compile unit names an LLVM-based compiler, such
  as clang or rustc, and the convention for any other producer. If the
  compile unit has no producer, such a variable shows as `<optimized out>`.
- C globals read the memory base of the last function that ran in that
  compiled module. If two instances of one module run together, or two
  threads run together, a global can show the value of another instance.
  Use `$wasm_global("name")` (gdb) or `wasm-global name` (lldb) to read the
  value through the selected frame.
- Locals of inlined functions show as `<optimized out>`.
- Pointers always refer to memory 0. If memory 0 is a 64-bit memory
  (memory64 proposal), only functions and line numbers get debug info.
  Variables and globals do not.
- On a big-endian target, only functions and line numbers get debug info.
  Wasm memory is little-endian, and the debugger would read its values in
  the byte order of the target.
- A variable whose location list moves between the wasm shadow stack and a
  wasm local shows only its wasm local ranges. A frame base that a
  location list describes is not supported, so the shadow-stack variables
  of that function show as `<optimized out>`.
- The JIT adds and removes its entries in the GDB JIT interface list under
  its own lock. Other JIT code in the same process that registers code
  with the debugger through LLVM uses another lock. Do not run both at the
  same time.
- At a breakpoint on the first line of a function, variables on the wasm
  shadow stack can show old values. The function has not yet set its frame
  base there. Step one line to see the real values.
- In lazy JIT mode (`--run-mode=lazyjit`), static variables inside a
  function have no debug info. Globals at file scope work.
