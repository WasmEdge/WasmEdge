## Commands

* `run` example
    * Takes `.wasm` path and function name , list of parameters and length of expected return values

=== "Python"
    ```python linenums="1"
    import WasmEdge
    import os

    # WasmEdge/bindings/python/example.py
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../.."))
    fib_wasm = os.path.join(
        wasm_base_path, "tools/wasmedge/examples/fibonacci.wasm"
    )
    log = WasmEdge.Logging()
    log.debug()
    vm = WasmEdge.VM()
    res, l = vm.run(fib_wasm, 'fib', [10], 1)
    print(l)
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
=== "Output"
    ```bash
    [2021-10-14 02:42:27.458] [debug]  Execution succeeded.
    [2021-10-14 02:42:27.458] [debug] 
    ====================  Statistics  ====================
    Total execution time: 53764 ns
    Wasm instructions execution time: 53764 ns
    Host functions execution time: 0 ns
    Executed wasm instructions count: 1854
    Gas costs: 1854
    Instructions per second: 34484041
    ['89']
    ```