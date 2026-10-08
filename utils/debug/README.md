# Debugging WasmEdge AOT and JIT code with gdb and lldb

Compile the wasm module with DWARF debug info, for example with
`clang -g` or `rustc -g`. Then turn on native debug info in WasmEdge.

For AOT, compile the module with `wasmedgec` and run the shared library:

```bash
wasmedgec --debug-info --optimize 0 app.wasm app.so
gdb --args wasmedge --run-mode=aot --reactor app.so main
```

For JIT, pass `--debug-info` to `wasmedge`. This option also makes the
JIT compile the code without optimization:

```bash
gdb --args wasmedge --run-mode=jit --debug-info --reactor app.wasm main
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

The pretty-printer shows a wasm pointer as `(wasm) 0x<address> -> <value>`.
It shows the target one level deep. To see the raw fields in gdb, use
`print -raw-values -- p`. This option needs gdb 10 or newer.

## How wasm values appear

- A wasm pointer appears as a `__wasm_ptr` struct. The field `__addr` is
  the offset in linear memory. The field `__pointee` is a zero-length array
  of native pointers. It only carries the pointee type. Its value is not
  meaningful.
- `__wasm_memory` in each function is the host address of memory 0.
- Variables on the wasm shadow stack and in linear memory show their real
  values.

## Limits

- Linux only. The helpers work with AOT shared libraries and with JIT.
  Universal wasm output keeps no debug info.
- C globals read the memory base of the last function that ran in that
  compiled module. If two instances of one module run together, or two
  threads run together, a global can show the value of another instance.
  Use `$wasm_global("name")` (gdb) or `wasm-global name` (lldb) to read the
  value through the selected frame.
- Locals of inlined functions show as `<optimized out>`.
- Pointers always refer to memory 0. The memory64 proposal is not supported.
- At a breakpoint on the first line of a function, variables on the wasm
  shadow stack can show old values. The function has not yet set its frame
  base there. Step one line to see the real values.
- In lazy JIT mode (`--run-mode=lazyjit`), static variables inside a
  function have no debug info. Globals at file scope work.
