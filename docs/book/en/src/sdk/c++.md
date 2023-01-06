# WasmEdge C++ SDK

The WasmEdge C++ SDK is a collection of headers and libraries that allow you to build and deploy WebAssembly (Wasm) modules for execution on WasmEdge devices. It includes a CMake project and a set of command-line tools that you can use to build and deploy your Wasm modules.

## Quick Start Guide

To get started with WasmEdge, follow these steps:

Install the WasmEdge C/C++ SDK: Download C++ SDK from the WasmEdge [website](https://wasmedge.org/book/en/quick_start/install.html) and follow the instructions to install it on your development machine

```cpp
#include <wasmedge/wasmedge.h>
#include <iostream>

int main(int argc, char** argv) {
  /* Create the configure context and add the WASI support. */
  /* This step is not necessary unless you need WASI support. */
  WasmEdge_ConfigureContext* conf_cxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(conf_cxt, WasmEdge_HostRegistration_Wasi);
  /* The configure and store context to the VM creation can be NULL. */
  WasmEdge_VMContext* vm_cxt = WasmEdge_VMCreate(conf_cxt, nullptr);

  /* The parameters and returns arrays. */
  WasmEdge_Value params[1] = { WasmEdge_ValueGenI32(40) };
  WasmEdge_Value returns[1];
  /* Function name. */
  WasmEdge_String func_name = WasmEdge_StringCreateByCString("fib");
  /* Run the WASM function from file. */
  WasmEdge_Result res = WasmEdge_VMRunWasmFromFile(vm_cxt, argv[1], func_name, params, 1, returns, 1);

  if (WasmEdge_ResultOK(res)) {
    std::cout << "Get result: " << WasmEdge_ValueGetI32(returns[0]) << std::endl;
  } else {
    std::cout << "Error message: " << WasmEdge_ResultGetMessage(res) << std::endl;
  }

  /* Resources deallocations. */
  WasmEdge_VMDelete(vm_cxt);
  WasmEdge_ConfigureDelete(conf_cxt);
  WasmEdge_StringDelete(func_name);
  return 0;
}
```

You can use the -I flag to specify the include directories and the -L and -l flags to specify the library directories and library names, respectively.
Then you can compile the code and run: ( the 40th fibonacci number is 102334155)

```bash
gcc example.cpp -x c++ -I/path/to/wasmedge/include -L/path/to/wasmedge/lib -lwasmedge -o example
```

To run the `example` executable that was created in the previous step, you can use the following command

```bash
./example
```

## Quick Start Guide in AOT compiler

```cpp
#include <wasmedge/wasmedge.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
  // Create the configure context and add the WASI support.
  // This step is not necessary unless you need WASI support.
  wasmedge_configure_context* conf_cxt = wasmedge_configure_create();
  wasmedge_configure_add_host_registration(conf_cxt, WASMEDGE_HOST_REGISTRATION_WASI);

  // Create the VM context in AOT mode.
  wasmedge_vm_context* vm_cxt = wasmedge_vm_create_aot(conf_cxt, NULL);

  // The parameters and returns arrays.
  wasmedge_value params[1] = { wasmedge_value_gen_i32(32) };
  wasmedge_value returns[1];
  // Function name.
  wasmedge_string func_name = wasmedge_string_create_by_cstring("fib");
  // Run the WASM function from file.
  wasmedge_result res = wasmedge_vm_run_wasm_from_file(vm_cxt, argv[1], func_name, params, 1, returns, 1);

  if (wasmedge_result_ok(res)) {
    printf("Get result: %d\n", wasmedge_value_get_i32(returns[0]));
  } else {
    printf("Error message: %s\n", wasmedge_result_get_message(res));
  }

  // Resources deallocations.
  wasmedge_vm_delete(vm_cxt);
  wasmedge_configure_delete(conf_cxt);
  wasmedge_string_delete(func_name);
  return 0;
}
```

In this example, the wasmedge_vm_create_aot function is used to create a wasmedge_vm_context object in AOT mode, which is then passed as the second argument to the wasmedge_vm_run_wasm_from_file function to execute the Wasm module in AOT mode.
