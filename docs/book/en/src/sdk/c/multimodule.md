# Multiple WASM Module Example

For those WASM modules export their functions, the other WASM modules can import them as a library.
This will cause a situation that linking multiple modules for the dependencies when in execution.
This chapter will introduce the examples to linking and executing multiple WASM modules in WasmEdge.

## Example WASM file

### The Library WASM

Assume that there's a WASM which exports it's function:

```wasm
(module
  (func (export "add") (param i32 i32) (result i32)
    ;; Function to add 2 number and exported as "add".
    (i32.add (local.get 0) (local.get 1))
  )
  (func (export "mul") (param i32 i32) (result i32)
    ;; Function to mul 2 number and exported as "mul".
    (i32.mul (local.get 0) (local.get 1))
  )
)
```

Users can convert `wat` to `wasm` through [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) live tool.
Assume that this `wat` is converted into the WASM binary format and saved as `lib.wasm`.

### The Entering WASM

Assume that there's a WASM which imports some functions from the `lib.wasm`, and this WASM exports the functions which can be invoked:

```wasm
(module
  (type $type0 (func (param i32 i32)(result i32)))
  ;; Import the "add" function which calculate "a + b".
  (import "math" "add" (func $math-add (type $type0)))
  ;; Import the "mul" function which calculate "a * b".
  (import "math" "mul" (func $math-mul (type $type0)))
  (func (export "add_and_square") (param i32 i32) (result i32)
    ;; Function to add 2 numbers and square it ((a + b)^2).
    ;; Exported as "add_and_square".
    (call $math-mul
      (call $math-add (local.get 0) (local.get 1))
      (call $math-add (local.get 0) (local.get 1))
    )
  )
  (func (export "sum_of_squares") (param i32 i32) (result i32)
    ;; Function to calculate the sum of squares (a^2 + b^2).
    ;; Exported as "sum_of_squares".
    (call $math-add
      (call $math-mul (local.get 0) (local.get 0))
      (call $math-mul (local.get 1) (local.get 1))
    )
  )
)
```

Users can convert `wat` to `wasm` through [wat2wasm](https://webassembly.github.io/wabt/demo/wat2wasm/) live tool.
Assume that this `wat` is converted into the WASM binary format and saved as `test.wasm`.

### Prerequisites

For executing these examples, developers should [install WasmEdge](../../quick_start/install.md).
To improve the performance of executing WASM, developers can also use the [AOT compiler](../../quick_start/run_in_aot_mode.md) to compile the above WASM files.

## Linking WASM Modules With the VM Context

With the `WasmEdge_VMContext`, developers can instantiate and execute WASM quickly.
There's at least 4 ways to linking these multiple WASM modules with the VM context.
For the example codes below, assume that the C code is saved as `example.c`.

1. Register and instantiate the `lib.wasm` from file directly

    ```c
    #include <stdio.h>
    #include <wasmedge/wasmedge.h>

    int main() {
      /* The result. */
      WasmEdge_Result Res;

      /* The params and returns. */
      WasmEdge_Value Params[2], Returns[1];

      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* Register the `lib.wasm` from file with the module name "math". */
      WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("math");
      Res = WasmEdge_VMRegisterModuleFromFile(VMCxt, ModuleName, "lib.wasm");
      WasmEdge_StringDelete(ModuleName);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Register lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Instantiate the `test.wasm`. */
      /*
       * Developers can use the APIs such as `WasmEdge_VMRunWasmFromFile` to
       * instantiate and execute quickly.
       */
      Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "test.wasm");
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMValidate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Validate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMInstantiate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Instantiate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Invoke the functions. */
      /* Invoke the "add_and_square" to calculate (123 + 456)^2 */
      WasmEdge_String FuncName =
          WasmEdge_StringCreateByCString("add_and_square");
      Params[0] = WasmEdge_ValueGenI32(123);
      Params[1] = WasmEdge_ValueGenI32(456);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '(%d + %d)^2' result: %d\n", 123, 456,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'add_and_square' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }
      /* Invoke the "sum_of_squares" to calculate (77^2 + 88^2) */
      FuncName = WasmEdge_StringCreateByCString("sum_of_squares");
      Params[0] = WasmEdge_ValueGenI32(77);
      Params[1] = WasmEdge_ValueGenI32(88);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '%d^2 + %d^2' result: %d\n", 77, 88,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'sum_of_squares' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      return 0;
    }
    ```

    Then compile and execute:

    ```bash
    $ gcc test.c -lwasmedge
    $ ./a.out
    Get the '(123 + 456)^2' result: 335241
    Get the '77^2 + 88^2' result: 13673
    ```

2. Register and instantiate the `lib.wasm` from buffer

    ```c
    #include <stdio.h>
    #include <wasmedge/wasmedge.h>

    int main() {
      /* The result. */
      WasmEdge_Result Res;

      /* The params and returns. */
      WasmEdge_Value Params[2], Returns[1];

      /* The `lib.wasm` buffer example. */
      /* Developers can also load the buffer from file. */
      uint8_t WASM[] = {/* WASM header */
                        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
                        /* Type section */
                        0x01, 0x07, 0x01,
                        /* function type {i32, i32} -> {i32} */
                        0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
                        /* Function section */
                        0x03, 0x03, 0x02, 0x00, 0x00,
                        /* Export section */
                        0x07, 0x0D, 0x02,
                        /* export function: "add" */
                        0x03, 0x61, 0x64, 0x64, 0x00, 0x00,
                        /* export function: "mul" */
                        0x03, 0x6D, 0x75, 0x6C, 0x00, 0x01,
                        /* Code section */
                        0x0A, 0x11, 0x02,
                        /* "add" code body */
                        0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6A, 0x0B,
                        /* "mul" code body */
                        0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6C, 0x0B};

      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* Register the `lib.wasm` from the buffer with the module name "math". */
      WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("math");
      Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, ModuleName, WASM,
                                                sizeof(WASM));
      WasmEdge_StringDelete(ModuleName);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Register lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Instantiate the `test.wasm`. */
      /*
       * Developers can use the APIs such as `WasmEdge_VMRunWasmFromFile` to
       * instantiate and execute quickly.
       */
      Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "test.wasm");
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMValidate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Validate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMInstantiate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Instantiate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Invoke the functions. */
      /* Invoke the "add_and_square" to calculate (123 + 456)^2 */
      WasmEdge_String FuncName =
          WasmEdge_StringCreateByCString("add_and_square");
      Params[0] = WasmEdge_ValueGenI32(123);
      Params[1] = WasmEdge_ValueGenI32(456);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '(%d + %d)^2' result: %d\n", 123, 456,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'add_and_square' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }
      /* Invoke the "sum_of_squares" to calculate (77^2 + 88^2) */
      FuncName = WasmEdge_StringCreateByCString("sum_of_squares");
      Params[0] = WasmEdge_ValueGenI32(77);
      Params[1] = WasmEdge_ValueGenI32(88);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '%d^2 + %d^2' result: %d\n", 77, 88,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'sum_of_squares' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      return 0;
    }
    ```

    Then compile and execute:

    ```bash
    $ gcc test.c -lwasmedge
    $ ./a.out
    Get the '(123 + 456)^2' result: 335241
    Get the '77^2 + 88^2' result: 13673
    ```

3. Load the `lib.wasm` to an AST Context first

    ```c
    #include <stdio.h>
    #include <wasmedge/wasmedge.h>

    int main() {
      /* The result. */
      WasmEdge_Result Res;

      /* The params and returns. */
      WasmEdge_Value Params[2], Returns[1];

      /* Assume that the `lib.wasm` has loaded first. */
      WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
      WasmEdge_ASTModuleContext *LibASTCxt = NULL;
      Res = WasmEdge_LoaderParseFromFile(LoadCxt, &LibASTCxt, "lib.wasm");
      WasmEdge_LoaderDelete(LoadCxt);
      if (!WasmEdge_ResultOK(Res)) {
        printf("Load lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* Register the loaded AST context with the module name "math". */
      WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("math");
      Res =
          WasmEdge_VMRegisterModuleFromASTModule(VMCxt, ModuleName, LibASTCxt);
      WasmEdge_StringDelete(ModuleName);
      WasmEdge_ASTModuleDelete(LibASTCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Register lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Instantiate the `test.wasm`. */
      /*
       * Developers can use the APIs such as `WasmEdge_VMRunWasmFromFile` to
       * instantiate and execute quickly.
       */
      Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "test.wasm");
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMValidate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Validate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMInstantiate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        printf("Instantiate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Invoke the functions. */
      /* Invoke the "add_and_square" to calculate (123 + 456)^2 */
      WasmEdge_String FuncName =
          WasmEdge_StringCreateByCString("add_and_square");
      Params[0] = WasmEdge_ValueGenI32(123);
      Params[1] = WasmEdge_ValueGenI32(456);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '(%d + %d)^2' result: %d\n", 123, 456,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'add_and_square' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }
      /* Invoke the "sum_of_squares" to calculate (77^2 + 88^2) */
      FuncName = WasmEdge_StringCreateByCString("sum_of_squares");
      Params[0] = WasmEdge_ValueGenI32(77);
      Params[1] = WasmEdge_ValueGenI32(88);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '%d^2 + %d^2' result: %d\n", 77, 88,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'sum_of_squares' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      return 0;
    }
    ```

    Then compile and execute:

    ```bash
    $ gcc test.c -lwasmedge
    $ ./a.out
    Get the '(123 + 456)^2' result: 335241
    Get the '77^2 + 88^2' result: 13673
    ```

4. Instantiate the `lib.wasm` first

    ```c
    #include <stdio.h>
    #include <wasmedge/wasmedge.h>

    int main() {
      /* The result. */
      WasmEdge_Result Res;

      /* The params and returns. */
      WasmEdge_Value Params[2], Returns[1];

      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* Assume that the `lib.wasm` has instantiated first. */
      WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
      WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(NULL);
      WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);
      WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
      WasmEdge_ASTModuleContext *LibASTCxt = NULL;
      WasmEdge_ModuleInstanceContext *LibInstCxt = NULL;
      Res = WasmEdge_LoaderParseFromFile(LoadCxt, &LibASTCxt, "lib.wasm");
      WasmEdge_LoaderDelete(LoadCxt);
      if (!WasmEdge_ResultOK(Res)) {
        printf("Load lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_ValidatorValidate(ValidCxt, LibASTCxt);
      WasmEdge_ValidatorDelete(ValidCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_ASTModuleDelete(LibASTCxt);
        printf("Validate lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      /*
       * The module name is determined when instantiation.
       * If use the `WasmEdge_ExecutorInstantiate` API, the module name will be
       * "".
       */
      WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("math");
      Res = WasmEdge_ExecutorRegister(ExecCxt, &LibInstCxt, StoreCxt, LibASTCxt,
                                      ModuleName);
      WasmEdge_ExecutorDelete(ExecCxt);
      WasmEdge_ASTModuleDelete(LibASTCxt);
      WasmEdge_StringDelete(ModuleName);
      WasmEdge_StoreDelete(StoreCxt);
      if (!WasmEdge_ResultOK(Res)) {
        printf("Instantiate lib.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Register the module instance with the module name "math". */
      /* The module name has determined when instantiating the `lib.wasm`. */
      Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, LibInstCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        WasmEdge_ModuleInstanceDelete(LibInstCxt);
        printf("Register lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Instantiate the `test.wasm`. */
      /*
       * Developers can use the APIs such as `WasmEdge_VMRunWasmFromFile` to
       * instantiate and execute quickly.
       */
      Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "test.wasm");
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        WasmEdge_ModuleInstanceDelete(LibInstCxt);
        printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMValidate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        WasmEdge_ModuleInstanceDelete(LibInstCxt);
        printf("Validate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }
      Res = WasmEdge_VMInstantiate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        WasmEdge_VMDelete(VMCxt);
        WasmEdge_ModuleInstanceDelete(LibInstCxt);
        printf("Instantiate test.wasm error: %s\n",
               WasmEdge_ResultGetMessage(Res));
        return -1;
      }

      /* Invoke the functions. */
      /* Invoke the "add_and_square" to calculate (123 + 456)^2 */
      WasmEdge_String FuncName =
          WasmEdge_StringCreateByCString("add_and_square");
      Params[0] = WasmEdge_ValueGenI32(123);
      Params[1] = WasmEdge_ValueGenI32(456);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '(%d + %d)^2' result: %d\n", 123, 456,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'add_and_square' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }
      /* Invoke the "sum_of_squares" to calculate (77^2 + 88^2) */
      FuncName = WasmEdge_StringCreateByCString("sum_of_squares");
      Params[0] = WasmEdge_ValueGenI32(77);
      Params[1] = WasmEdge_ValueGenI32(88);
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
      WasmEdge_StringDelete(FuncName);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the '%d^2 + %d^2' result: %d\n", 77, 88,
               WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execute 'sum_of_squares' error: %s\n",
               WasmEdge_ResultGetMessage(Res));
      }

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      /* The imported module instance should be destroyed. */
      WasmEdge_ModuleInstanceDelete(LibInstCxt);
      return 0;
    }
    ```

    Then compile and execute:

    ```bash
    $ gcc test.c -lwasmedge
    $ ./a.out
    Get the '(123 + 456)^2' result: 335241
    Get the '77^2 + 88^2' result: 13673
    ```

## Linking WASM Modules By the Executor Context

For linking multiple WASM modules, developers should instantiate them first with considering their dependencies.

```c
#include <stdio.h>
#include <wasmedge/wasmedge.h>

int main() {
  /* The result. */
  WasmEdge_Result Res;

  /* The params and returns. */
  WasmEdge_Value Params[2], Returns[1];

  /* Create the contexts. */
  WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
  WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(NULL);
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);
  WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();

  /* Load and register the `lib.wasm` with the module name "math". */
  WasmEdge_ASTModuleContext *LibASTCxt = NULL;
  WasmEdge_ModuleInstanceContext *LibInstCxt = NULL;
  Res = WasmEdge_LoaderParseFromFile(LoadCxt, &LibASTCxt, "lib.wasm");
  if (!WasmEdge_ResultOK(Res)) {
    printf("Load lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  Res = WasmEdge_ValidatorValidate(ValidCxt, LibASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Validate lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("math");
  Res = WasmEdge_ExecutorRegister(ExecCxt, &LibInstCxt, StoreCxt, LibASTCxt,
                                  ModuleName);
  WasmEdge_StringDelete(ModuleName);
  WasmEdge_ASTModuleDelete(LibASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Instantiate lib.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }

  /* Load and instantiate the `test.wasm`. */
  WasmEdge_ASTModuleContext *TestASTCxt = NULL;
  WasmEdge_ModuleInstanceContext *TestInstCxt = NULL;
  Res = WasmEdge_LoaderParseFromFile(LoadCxt, &TestASTCxt, "test.wasm");
  if (!WasmEdge_ResultOK(Res)) {
    printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  Res = WasmEdge_ValidatorValidate(ValidCxt, TestASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Validate test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  Res =
      WasmEdge_ExecutorInstantiate(ExecCxt, &TestInstCxt, StoreCxt, TestASTCxt);
  WasmEdge_ASTModuleDelete(TestASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Instantiate test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }

  /* Invoke the functions. */
  /* Invoke the "add_and_square" to calculate (123 + 456)^2 */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("add_and_square");
  WasmEdge_FunctionInstanceContext *FuncCxt =
      WasmEdge_ModuleInstanceFindFunction(TestInstCxt, FuncName);
  WasmEdge_StringDelete(FuncName);
  if (FuncCxt == NULL) {
    printf("Function 'add_and_square' not found.\n");
    return -1;
  }
  Params[0] = WasmEdge_ValueGenI32(123);
  Params[1] = WasmEdge_ValueGenI32(456);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  if (WasmEdge_ResultOK(Res)) {
    printf("Get the '(%d + %d)^2' result: %d\n", 123, 456,
           WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Execute 'add_and_square' error: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }
  /* Invoke the "sum_of_squares" to calculate (77^2 + 88^2) */
  FuncName = WasmEdge_StringCreateByCString("sum_of_squares");
  FuncCxt = WasmEdge_ModuleInstanceFindFunction(TestInstCxt, FuncName);
  WasmEdge_StringDelete(FuncName);
  if (FuncCxt == NULL) {
    printf("Function 'sum_of_squares' not found.\n");
    return -1;
  }
  Params[0] = WasmEdge_ValueGenI32(77);
  Params[1] = WasmEdge_ValueGenI32(88);
  Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 2, Returns, 1);
  if (WasmEdge_ResultOK(Res)) {
    printf("Get the '%d^2 + %d^2' result: %d\n", 77, 88,
           WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Execute 'sum_of_squares' error: %s\n",
           WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_LoaderDelete(LoadCxt);
  WasmEdge_ValidatorDelete(ValidCxt);
  WasmEdge_ExecutorDelete(ExecCxt);
  WasmEdge_StoreDelete(StoreCxt);
  WasmEdge_ModuleInstanceDelete(LibInstCxt);
  WasmEdge_ModuleInstanceDelete(TestInstCxt);
  return 0;
}
```

Then compile and execute:

```bash
$ gcc test.c -lwasmedge
$ ./a.out
Get the '(123 + 456)^2' result: 335241
Get the '77^2 + 88^2' result: 13673
```
