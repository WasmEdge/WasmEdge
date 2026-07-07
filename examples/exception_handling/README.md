# Exception handling example

Documentation: [Exception Handling](https://wasmedge.org/docs/develop/c/exception_handling) (WasmEdge Book).

WasmEdge supports the latest EH proposal only. Legacy EH from some toolchains fails to load.

## Emscripten repro (expected failure)

`a.cpp`:

```cpp
#include <stdio.h>

int main() {
  try {
    puts("throw...");
    throw 1;
    puts("(never reached)");
  } catch (...) {
    puts("catch!");
  }
  return 0;
}
```

```bash
emcc -O1 -fwasm-exceptions -sSTANDALONE_WASM a.cpp -o a.wasm
wasmedge --enable-exception-handling a.wasm
```

Typical error: `loading failed: illegal opcode, Code: 0x117` (`Deprecated delegate instruction`).

## Passing EH tests

Use `.wasm` files from the `wasm-3.0-exceptions` folder in [wasmedge-spectest](https://github.com/WasmEdge/wasmedge-spectest):

```bash
wasmedge --enable-exception-handling path/to/test.wasm
```

Fixes [#2406](https://github.com/WasmEdge/WasmEdge/issues/2406).
