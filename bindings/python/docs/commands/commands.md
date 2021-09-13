## Commands

* `py_run` example
    * Takes `.wasm` path and function name

=== "Python"
    ```python linenums="1"
    import WasmEdge as we
    import os

    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../.."))
    fib_wasm = os.path.join(
        wasm_base_path, "tools/wasmedge/examples/fibonacci.wasm"
    )

    print(we.py_run(
                fib_wasm,
                "fib",
            ))
    ```
=== "C"

    ```c linenums="1"
    #include <wasmedge.h>
    #include <stdio.h>

    int main() {
        /* Create the configure context and add the WASI support. */
        /* This step is not necessary unless you need WASI support. */
        WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
        WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
        /* The configure and store context to the VM creation can be NULL. */
        WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

        /* The parameters and returns arrays. */
        WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(5) };
        WasmEdge_Value Returns[1];
        /* Function name. */
        WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
        /* Run the WASM function from file. */
        WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, "fibonacci.wasm", FuncName, Params, 1, Returns, 1);
        /* 
        * Developers can run the WASM binary from buffer with the `WasmEdge_VMRunWasmFromBuffer()` API,
        * or from `WasmEdge_ASTModuleContext` object with the `WasmEdge_VMRunWasmFromASTModule()` API.
        */

        if (WasmEdge_ResultOK(Res)) {
            printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
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