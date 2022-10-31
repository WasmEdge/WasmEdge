# WasmEdge C SDK

The WasmEdge C API denotes an interface to embed the WasmEdge runtime into a C program. The followings are the quick start guide for working with the C APIs of WasmEdge. For the details of the WasmEdge C API, please refer to the [full documentation](c/ref.md). Before programming with the WasmEdge C API, please [install WasmEdge](../quick_start/install.md) first.

The WasmEdge C API is also the fundamental API for other languages' SDK.

## Quick Start Guide for the WasmEdge runner

The following is an example for running a WASM file.
Assume that the WASM file [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) is copied into the current directory, and the C file `test_wasmedge.c` is as following:

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* Create the configure context and add the WASI support. */
  /* This step is not necessary unless you need WASI support. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(32) };
  WasmEdge_Value Returns[1];
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  /* Run the WASM function from file. */
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, Argv[1], FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StringDelete(FuncName);
  return 0;
}
```

Then you can compile and run: (the 32th fibonacci number is 3524578 in 0-based index)

```bash
$ gcc test_wasmedge.c -lwasmedge -o test_wasmedge
$ ./test_wasmedge fibonacci.wasm
Get result: 3524578
```

## Quick Start Guide for the WasmEdge AOT compiler

Assume that the WASM file [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) is copied into the current directory, and the C file `test_wasmedge_compiler.c` is as following:

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main(int Argc, const char* Argv[]) {
  /* Create the configure context. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  /* ... Adjust settings in the configure context. */
  /* Result. */
  WasmEdge_Result Res;

  /* Create the compiler context. The configure context can be NULL. */
  WasmEdge_CompilerContext *CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
  /* Compile the WASM file with input and output paths. */
  Res = WasmEdge_CompilerCompile(CompilerCxt, Argv[1], Argv[2]);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Compilation failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return 1;
  }

  WasmEdge_CompilerDelete(CompilerCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  return 0;
}
```

Then you can compile and run (the output file is `fibonacci_aot.wasm`):

```bash
$ gcc test_wasmedge_compiler.c -lwasmedge -o test_wasmedge_compiler
$ ./test_wasmedge_compiler fibonacci.wasm fibonacci_aot.wasm
[2021-07-02 11:08:08.651] [info] compile start
[2021-07-02 11:08:08.653] [info] verify start
[2021-07-02 11:08:08.653] [info] optimize start
[2021-07-02 11:08:08.670] [info] codegen start
[2021-07-02 11:08:08.706] [info] compile done
```

The compiled-WASM file can be used as a WASM input for the WasmEdge runner.
The following is the comparison of the interpreter mode and the AOT mode:

```bash
$ time ./test_wasmedge fibonacci.wasm
Get result: 5702887

real 0m2.715s
user 0m2.700s
sys 0m0.008s

$ time ./test_wasmedge fibonacci_aot.wasm
Get result: 5702887

real 0m0.036s
user 0m0.022s
sys 0m0.011s
```

## API References

- [0.11.1](c/ref.md)
- [0.10.1](c/0.10.1/ref.md)
  - [Upgrade to 0.11.0](c/0.10.1/upgrade_to_0.11.0.md)
- [0.9.1](c/0.9.1/ref.md)
  - [Upgrade to 0.10.0](c/0.9.1/upgrade_to_0.10.0.md)

## Examples

- Use the [external reference](c/externref.md) of WebAssembly input and output in C/C++
- Implement the [host functions](c/hostfunction.md) in C/C++
