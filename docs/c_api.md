# WasmEdge C API Documentation

[WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge/wasmedge.h) denotes an interface to access the WasmEdge runtime. The followings are the guides to working with the C APIs of WasmEdge.

## Table of Contents

* [WasmEdge Installation](#WasmEdge-Installation)
  * [Download And Install](#Download-And-Install)
  * [Compile Sources](#Compile-Sources)
* [WasmEdge Basics](#WasmEdge-Basics)
  * [Version](#Version)
  * [Logging Settings](#Logging-Settings)
  * [Contexts](#Contexts)
  * [Strings](#Strings)
  * [Results](#Results)
  * [Value Types](#Value-Types)
  * [Configurations](#Configurations)
  * [Statistics](#Statistics)
* [WasmEdge VM](#WasmEdge-VM)
  * [WASM Execution Example With VM Context](#WASM-Execution-Example-With-VM-Context)
  * [VM Creations](#VM-Creations)
  * [Preregistrations](#Preregistrations)
  * [Host Module Registrations](#Host-Module-Registrations)
  * [WASM Registrations And Executions](#WASM-Registrations-And-Executions)
  * [Instance Tracing](#Instance-Tracing)
* [WasmEdge Runtime](#WasmEdge-Runtime)
  * [WASM Execution Example Step-By-Step](#WASM-Execution-Example-Step-By-Step)
  * [Loader](#Loader)
  * [Validator](#Validator)
  * [Interpreter](#Interpreter)
  * [Store](#Store)
  * [Instances](#Instances)
  * [Host Functions](#Host-Functions)
* [WasmEdge AOT Compiler](#WasmEdge-AOT-Compiler)
  * [Compilation Example](#Compilation-Example)
  * [Compiler Options](#Compiler-Options)

## WasmEdge Installation

### Download And Install

For `Ubuntu` or `Debian`, WasmEdge can be fully installed by the following commands:

```bash
$ wget https://github.com/WasmEdge/WasmEdge/releases/download/0.8.1/WasmEdge-0.8.1.deb
$ sudo dpkg -i WasmEdge-0.8.1.deb
```

Or developers can install the shared library step-by-step (take the `manylinux2014` package for example):

```bash
$ wget https://github.com/WasmEdge/WasmEdge/releases/download/0.8.1/WasmEdge-0.8.1-manylinux2014_x86_64.tar.gz
$ tar -xzf WasmEdge-0.8.1-manylinux2014_x86_64.tar.gz
# Copy the headers to /usr/local/include
$ sudo cp -r WasmEdge-0.8.1-Linux/include /usr/local/include
# Copy the shared library to /usr/local/lib
$ sudo cp WasmEdge-0.8.1-Linux/lib64/libwasmedge_c.so /usr/local/lib/
$ sudo ldconfig
```

### Compile Sources

After the installation of WasmEdge, the following guide can help you to test for the availability of the WasmEdge C API.

1. Prepare the test C file (and assumed saved as `test.c`):

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>
    int main() {
      printf("WasmEdge version: %s\n", WasmEdge_VersionGet());
      return 0;
    }
    ```

2. Compile the file with `gcc` or `clang`.

    ```bash
    $ gcc test.c -lwasmedge_c
    ```

3. Run and get the expected output.

    ```bash
    $ ./a.out
    WasmEdge version: 0.8.1
    ```

## WasmEdge Basics

In this partition, we will introduce the utilities and concepts of WasmEdge shared library.

### Version

The `Version` related APIs provide developers to check for the WasmEdge shared library version.

```c
#include <wasmedge/wasmedge.h>
printf("WasmEdge version: %s\n", WasmEdge_VersionGet());
printf("WasmEdge version major: %u\n", WasmEdge_VersionGetMajor());
printf("WasmEdge version minor: %u\n", WasmEdge_VersionGetMinor());
printf("WasmEdge version patch: %u\n", WasmEdge_VersionGetPatch());
```

### Logging Settings

The `WasmEdge_LogSetErrorLevel()` and `WasmEdge_LogSetDebugLevel()` APIs can set the logging system to debug level or error level. By default, the error level is set, and the debug info is hidden.

### Contexts

The objects, such as `VM`, `Store`, and `HostFunction`, are composed of `Context`s.
All of the contexts can be created by calling the corresponding creation APIs and should be destroyed by calling the corresponding deletion APIs. Developers have responsibilities to manage the contexts for memory management.

```c
/* Create the configure context. */
WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
/* Delete the configure context. */
WasmEdge_ConfigureDelete(ConfCxt);
```

The details of other contexts will be introduced later.

### Strings

The `WasmEdge_String` object is for the instance names when invoking a WASM function or finding the contexts of instances.

1. Create a `WasmEdge_String` from a C string (`const char *` with NULL termination) or a buffer with length.

    The content of the C string or buffer will be copied into the `WasmEdge_String` object.

    ```c
    char Buf[4] = {50, 55, 60, 65};
    WasmEdge_String Str1 = WasmEdge_StringCreateByCString("test");
    WasmEdge_String Str2 = WasmEdge_StringCreateByBuffer(Buf, 4);
    /* The objects should be deleted by `WasmEdge_StringDelete()`. */
    WasmEdge_StringDelete(Str1);
    WasmEdge_StringDelete(Str2);
    ```

2. Wrap a `WasmEdge_String` to a buffer with length.

    The content will not be copied, and the caller should guarantee the life cycle of the input buffer.

    ```c
    const char CStr[] = "test";
    WasmEdge_String Str = WasmEdge_StringWrap(CStr, 4);
    /* The object should __NOT__ be deleted by `WasmEdge_StringDelete()`. */
    ```

3. String comparison

    ```c
    const char CStr[] = "abcd";
    char Buf[4] = {0x61, 0x62, 0x63, 0x64};
    WasmEdge_String Str1 = WasmEdge_StringWrap(CStr, 4);
    WasmEdge_String Str2 = WasmEdge_StringCreateByBuffer(Buf, 4);
    bool IsEq = WasmEdge_StringIsEqual(Str1, Str2);
    /* The `IsEq` will be `TRUE`. */
    WasmEdge_StringDelete(Str2);
    ```

4. Convert to C string

    ```c
    char Buf[256];
    WasmEdge_String Str = WasmEdge_StringCreateByCString("test_wasmedge_string");
    uint32_t StrLength = WasmEdge_StringCopy(Str, Buf, sizeof(Buf));
    /* StrLength will be 20 */
    printf("String: %s\n", Buf);
    /* Will print "test_wasmedge_string". */
    ```

### Results

The `WasmEdge_Result` object specifies the execution status.
APIs about WASM execution will return the `WasmEdge_Result` to denote the status.

```c
WasmEdge_Result Res = WasmEdge_Result_Success;
bool IsSucceeded = WasmEdge_ResultOK(Res);
/* The `IsSucceeded` will be `TRUE`. */
uint32_t Code = WasmEdge_ResultGetCode(Res);
/* The `Code` will be 0. */
const char *Msg = WasmEdge_ResultGetMessage(Res);
/* The `Msg` will be "success". */
```

### Value Types

In WasmEdge, developers should convert the values to `WasmEdge_Value` objects through APIs for matching to the WASM value types.

1. Number types: `i32`, `i64`, `f32`, `f64`, and `v128` for the `SIMD` proposal

    ```c
    WasmEdge_Value Val;
    Val = WasmEdge_ValueGenI32(123456);
    printf("%d\n", WasmEdge_ValueGetI32(Val));
    /* Will print "123456" */
    Val = WasmEdge_ValueGenI64(1234567890123LL);
    printf("%ld\n", WasmEdge_ValueGetI64(Val));
    /* Will print "1234567890123" */
    Val = WasmEdge_ValueGenF32(123.456f);
    printf("%f\n", WasmEdge_ValueGetF32(Val));
    /* Will print "123.456001" */
    Val = WasmEdge_ValueGenF64(123456.123456789);
    printf("%.10f\n", WasmEdge_ValueGetF64(Val));
    /* Will print "123456.1234567890" */
    ```

2. Reference types: `funcref` and `externref` for the `Reference-Types` proposal

    ```c
    WasmEdge_Value Val;
    void *Ptr;
    bool IsNull;
    uint32_t Num = 10;
    /* Genreate a externref to NULL. */
    Val = WasmEdge_ValueGenNullRef(WasmEdge_RefType_ExternRef);
    IsNull = WasmEdge_ValueIsNullRef(Val);
    /* The `IsNull` will be `TRUE`. */
    Ptr = WasmEdge_ValueGetExternRef(Val);
    /* The `Ptr` will be `NULL`. */

    /* Genreate a funcref with function index 20. */
    Val = WasmEdge_ValueGenFuncRef(20);
    uint32_t FuncIdx = WasmEdge_ValueGetFuncIdx(Val);
    /* The `FuncIdx` will be 20. */

    /* Genreate a externref to `Num`. */
    Val = WasmEdge_ValueGenExternRef(&Num);
    Ptr = WasmEdge_ValueGetExternRef(Val);
    /* The `Ptr` will be `&Num`. */
    printf("%u\n", *(uint32_t *)Ptr);
    /* Will print "10" */
    Num += 55;
    printf("%u\n", *(uint32_t *)Ptr);
    /* Will print "65" */
    ```

### Configurations

The configuration context, `WasmEdge_ConfigureContext`, manages the configurations for `Loader`, `Validator`, `Interpreter`, `VM`, and `Compiler`.
Developers can adjust the settings about the proposals, VM host pre-registrations (such as `WASI`), and AOT compiler options, and then apply the `Configure` context to create other runtime contexts.

1. Proposals

    WasmEdge supports turning on or off the WebAssembly proposals.
    This configuration is only effective in `Loader`, `Validator`, `Interpreter`, `VM`, and `Compiler` contexts.

    ```c
    enum WasmEdge_Proposal {
      WasmEdge_Proposal_BulkMemoryOperations = 0,
      WasmEdge_Proposal_ReferenceTypes,
      WasmEdge_Proposal_SIMD,
      WasmEdge_Proposal_TailCall,
      WasmEdge_Proposal_Annotations,
      WasmEdge_Proposal_Memory64,
      WasmEdge_Proposal_Threads,
      WasmEdge_Proposal_ExceptionHandling,
      WasmEdge_Proposal_FunctionReferences
    };
    ```

    Developers can add or remove the proposals into the `Configure` context.

    ```c
    /* By default, the bulk-memory-operations and reference-types proposals have turned on initially. */
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddProposal(ConfCxt, WasmEdge_Proposal_SIMD);
    WasmEdge_ConfigureRemoveProposal(ConfCxt, WasmEdge_Proposal_ReferenceTypes);
    bool IsBulkMem = WasmEdge_ConfigureHasProposal(ConfCxt, WasmEdge_Proposal_BulkMemoryOperations);
    /* The `IsBulkMem` will be `TRUE`. */
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

2. Host registrations

    This configuration is used for the `VM` context to turn on the `WASI` or `wasmedge_process` supports and only effective in `VM` contexts.

    ```c
    enum WasmEdge_HostRegistration {
      WasmEdge_HostRegistration_Wasi = 0,
      WasmEdge_HostRegistration_WasmEdge_Process
    };
    ```

    The details will be introduced in the [preregistrations of VM context](###Preregistrations).

    ```c
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

3. Maximum memory pages

    Developers can limit the page size of memory instances by this configuration.
    When growing the page size of memory instances in WASM execution and exceeding the limited size, the page growing will fail.
    This configuration is only effective in the `Interpreter` and `VM` contexts.

    ```c
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureSetMaxMemoryPage(ConfCxt, 1024);
    /* The memory size of each memory instance should not larger than 1024 pages (64 MiB). */
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

4. AOT compiler options

    The AOT compiler options configure the behavior about optimization level, dump IR, compiled-WASM instruction counting, and cost measuring in runtime.
    These configurations are only effective in `Compiler` contexts.

    ```c
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    /* By default, the optimization level is O3. */
    WasmEdge_ConfigureCompilerGetOptimizationLevel(ConfCxt, WasmEdge_CompilerOptimizationLevel_O2);
    /* By default, the dump IR is `FALSE`. */
    WasmEdge_ConfigureCompilerSetDumpIR(ConfCxt, TRUE);
    /* By default, the intruction counting is `FALSE` when running a compiled-WASM. */
    WasmEdge_ConfigureCompilerSetInstructionCounting(ConfCxt, TRUE);
    /* By default, the cost measurement is `FALSE` when running a compiled-WASM. */
    WasmEdge_ConfigureCompilerSetCostMeasuring(ConfCxt, TRUE);
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

### Statistics

The statistics context, `WasmEdge_StatisticsContext`, provides the instruction counter, cost summation, and cost limitation at runtime.

1. Instruction counter

    The instruction counter can help developers to profile the performance of WASM running.
    Developers can retrieve the `Statistics` context from the `VM` context, or create a new one for the `Interpreter` creation.
    The details will be introduced in the next partitions.

    ```c
    WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
    /* ....
     * After running the WASM functions with the `Statistics` context
     */
    uint32_t Count = WasmEdge_StatisticsGetInstrCount(StatCxt);
    double IPS = WasmEdge_StatisticsGetInstrPerSecond(StatCxt);
    WasmEdge_StatisticsDelete(StatCxt);
    ```

2. Cost table

    The cost table is to accumulate the cost of instructions with their weights.
    Developers can set the cost table array (the indices are the byte code value of instructions, and the values are the cost of instructions) into the `Statistics` context.
    If the cost limit value is set, the execution will return the `cost limit exceeded` error immediately when exceeds the cost limit in runtime.

    ```c
    WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
    uint64_t CostTable[16] = {
      0, 0,
      10, /* 0x02: Block */
      11, /* 0x03: Loop */
      12, /* 0x04: If */
      12, /* 0x05: Else */
      0, 0, 0, 0, 0, 0, 
      20, /* 0x0C: Br */
      21, /* 0x0D: Br_if */
      22, /* 0x0E: Br_table */
      0
    };
    /* Developers can set the costs of each instruction. The value not covered will be 0. */
    WasmEdge_StatisticsSetCostTable(StatCxt, CostTable, 16);
    WasmEdge_StatisticsSetCostLimit(StatCxt, 5000000);
    /* ....
     * After running the WASM functions with the `Statistics` context
     */
    uint64_t Cost = WasmEdge_StatisticsGetTotalCost(StatCxt);
    WasmEdge_StatisticsDelete(StatCxt);
    ```

## WasmEdge VM

In this partition, we will introduce the functions of `WasmEdge_VMContext` object and show examples of executing WASM functions.

### WASM Execution Example With VM Context

The following shows the example of running the WASM for getting the Fibonacci.
This example uses the [fibonacci.wasm](../tools/wasmedge/examples/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](../tools/wasmedge/examples/fibonacci.wat).

```
(module
 (export "fib" (func $fib))
 (func $fib (param $n i32) (result i32)
  (if
   (i32.lt_s (get_local $n)(i32.const 2))
   (return (i32.const 1))
  )
  (return
   (i32.add
    (call $fib (i32.sub (get_local $n)(i32.const 2)))
    (call $fib (i32.sub (get_local $n)(i32.const 1)))
   )
  )
 )
)
```

1. Run WASM functions rapidly

    Assume that the WASM file [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and the C file `test.c` is as following:

    ```c
    #include <wasmedge/wasmedge.h>
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

    Then you can compile and run: (the 5th Fibonacci number is 8 in 0-based index)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Get the result: 8
    ```

2. Instantiate and run WASM functions manually

    Besides the above example, developers can run the WASM functions step-by-step with `VM` context APIs:

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>
    int main() {
      /* Create the configure context and add the WASI support. */
      /* This step is not necessary unless you need the WASI support. */
      WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
      WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
      /* The configure and store context to the VM creation can be NULL. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);

      /* The parameters and returns arrays. */
      WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(10) };
      WasmEdge_Value Returns[1];
      /* Function name. */
      WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
      /* Result. */
      WasmEdge_Result Res;
      
      /* Step 1: Load WASM file. */
      Res = WasmEdge_VMLoadWasmFromFile(VMCxt, "fibonacci.wasm");
      /* 
       * Developers can load the WASM binary from buffer with the `WasmEdge_VMLoadWasmFromBuffer()` API,
       * or from `WasmEdge_ASTModuleContext` object with the `WasmEdge_VMLoadWasmFromASTModule()` API.
       */
      if (!WasmEdge_ResultOK(Res)) {
        printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return 1;
      }
      /* Step 2: Validate the WASM module. */
      Res = WasmEdge_VMValidate(VMCxt);
      if (!WasmEdge_ResultOK(Res)) {
        printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return 1;
      }
      /* Step 3: Instantiate the WASM module. */
      Res = WasmEdge_VMInstantiate(VMCxt);
      /* 
       * Developers can load, validate, and instantiate another WASM module to replace the
       * instantiated one. In this case, the old module will be cleared, but the registered
       * modules are still kept.
       */
      if (!WasmEdge_ResultOK(Res)) {
        printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return 1;
      }
      /* Step 4: Execute WASM functions. You can execute functions repeatedly after instantiation. */
      Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 1, Returns, 1);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execution phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      }
      return 0;
    }
    ```

    Then you can compile and run: (the 10th Fibonacci number is 89 in 0-based index)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Get the result: 89
    ```

    The following graph explains the status of the `VM` context.

    ```
                           |========================|
                  |------->|      VM: Initiated     |
                  |        |========================|
                  |                    |
                  |                 LoadWasm
                  |                    |
                  |                    v
                  |        |========================|
                  |--------|       VM: Loaded       |<-------|
                  |        |========================|        |
                  |              |            ^              |
                  |         Validate          |              |
              Cleanup            |          LoadWasm         |
                  |              v            |            LoadWasm
                  |        |========================|        |
                  |--------|      VM: Validated     |        |
                  |        |========================|        |
                  |              |            ^              |
                  |      Instantiate          |              |
                  |              |          RegisterModule   |
                  |              v            |              |
                  |        |========================|        |
                  |--------|    VM: Instantiated    |--------|
                           |========================|
                                 |            ^
                                 |            |
                                 --------------
                    Instantiate, Execute, ExecuteRegistered
    ```

    The status of the `VM` context would be `Inited` when created.
    After loading WASM successfully, the status will be `Loaded`.
    After validating WASM successfully, the status will be `Validated`.
    After instantiating WASM successfully, the status will be `Instantiated`, and developers can invoke functions.
    Developers can register WASM or import objects in any status, but they should instantiate WASM again.
    Developers can also load WASM in any status, and they should validate and instantiate the WASM module before function invocation.
    When in the `Instantiated` status, developers can instantiate the WASM module again to reset the old WASM runtime structures.

### VM Creations

The `VM` creation API accepts the `Configure` context and the `Store` context.
If developers only need the default settings, just pass `NULL` to the creation API.
The details of the `Store` context will be introduced in [Store](#Store).

```c
WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, StoreCxt);
/* The caller should guarantee the life cycle if the store context. */
WasmEdge_StatisticsContext *StatCxt = WasmEdge_VMGetStatisticsContext(VMCxt);
/* The VM context already contains the statistics context and can be retrieved by this API. */
/* 
 * Note that the retrieved store and statistics contexts from the VM contexts by VM APIs
 * should __NOT__ be destroyed and owned by the VM contexts.
 */
WasmEdge_VMDelete(VMCxt);
WasmEdge_StoreDelete(StoreCxt);
WasmEdge_ConfigureDelete(ConfCxt);
```

### Preregistrations

WasmEdge provides the following built-in pre-registrations.

1. [WASI (WebAssembly System Interface)](https://github.com/WebAssembly/WASI)

    Developers can turn on the WASI support for VM in the `Configure` context.

    ```c
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);
    /* The following API can retrieve the pre-registration import objects from the VM context. */
    /* This API will return `NULL` if the corresponding pre-registration is not set into the configuration. */
    WasmEdge_ImportObjectContext *WasiObject =
      WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
    /* Initialize the WASI. */
    WasmEdge_ImportObjectInitWASI(WasiObject, /* ... ignored */ );
    WasmEdge_VMDelete(VMCxt);
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

    And also can create the WASI import object from API. The details will be introduced in the [Host Functions](#Host-Functions) and the [Host Module Registrations](#Host-Module-Registrations).

2. [WasmEdge_Process](https://crates.io/crates/wasmedge_process_interface)

    This pre-registration is for the process interface for WasmEdge on `Rust` sources.
    After turning on this pre-registration, the VM will support the `wasmedge_process` host functions.

    ```c
    WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_WasmEdge_Process);
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);
    /* The following API can retrieve the pre-registration import objects from the VM context. */
    /* This API will return `NULL` if the corresponding pre-registration is not set into the configuration. */
    WasmEdge_ImportObjectContext *ProcObject =
      WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_WasmEdge_Process);
    /* Initialize the WasmEdge_Process. */
    WasmEdge_ImportObjectInitWasmEdgeProcess(ProcObject, /* ... ignored */ );
    WasmEdge_VMDelete(VMCxt);
    WasmEdge_ConfigureDelete(ConfCxt);
    ```

    And also can create the WasmEdge_Process import object from API. The details will be introduced in the [Host Functions](#Host-Functions) and the [Host Module Registrations](#Host-Module-Registrations).

### Host Module Registrations

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
In WasmEdge, the host functions are composed into host modules as `WasmEdge_ImportObjectContext` objects with module names.
Please refer to the [Host Functions in WasmEdge Runtime](#Host-Functions) for the details.
In this chapter, we show the example for registering the host modules into a `VM` context.

```c
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
WasmEdge_ImportObjectContext *WasiObject =
  WasmEdge_ImportObjectCreateWASI( /* ... ignored ... */ );
/* You can also create and register the WASI host modules by this API. */
WasmEdge_Result Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, WasiObject);
/* The result status should be checked. */
WasmEdge_ImportObjectDelete(WasiObject);
/* The created import objects should be deleted. */
WasmEdge_VMDelete(VMCxt);
```

### WASM Registrations And Executions

In WebAssembly, the instances in WASM modules can be exported and can be imported by other WASM modules.
WasmEdge VM provides APIs for developers to register and export any WASM modules, and execute the functions or host functions in the registered WASM modules.

1. Register the WASM modules with exported module names

    Unless the import objects have already contained the module names, every WASM module should be named uniquely when registering.
    Assume that the WASM file [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory.

    ```c
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("mod");
    WasmEdge_Result Res = WasmEdge_VMRegisterModuleFromFile(VMCxt, ModName, "fibonacci.wasm");
    /* 
     * Developers can register the WASM module from buffer with the `WasmEdge_VMRegisterModuleFromBuffer()` API,
     * or from `WasmEdge_ASTModuleContext` object with the `WasmEdge_VMRegisterModuleFromASTModule()` API.
     */
    /* 
     * The result status should be checked.
     * The error will occur if the WASM module instantiation failed or the module name conflicts.
     */
    WasmEdge_StringDelete(ModName);
    WasmEdge_VMDelete(VMCxt);
    ```

2. Execute the functions in registered WASM modules

    Assume that the C file `test.c` is as follows:

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>
    int main() {
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* The parameters and returns arrays. */
      WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(20) };
      WasmEdge_Value Returns[1];
      /* Names. */
      WasmEdge_String ModName = WasmEdge_StringCreateByCString("mod");
      WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
      /* Result. */
      WasmEdge_Result Res;

      /* Register the WASM module into VM. */
      Res = WasmEdge_VMRegisterModuleFromFile(VMCxt, ModName, "fibonacci.wasm");
      /* 
      * Developers can register the WASM module from buffer with the `WasmEdge_VMRegisterModuleFromBuffer()` API,
      * or from `WasmEdge_ASTModuleContext` object with the `WasmEdge_VMRegisterModuleFromASTModule()` API.
      */
      if (!WasmEdge_ResultOK(Res)) {
        printf("WASM registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return 1;
      }
      /* 
      * The function "fib" in the "fibonacci.wasm" was exported with the module name "mod".
      * As the same as host functions, other modules can import the function `"mod" "fib"`.
      */

      /* 
      * Execute WASM functions in registered modules.
      * Unlike the execution of functions, the registered functions can be invoked without
      * `WasmEdge_VMInstantiate()` because the WASM module was instantiated when registering.
      * Developers can also invoke the host functions directly with this API.
      */
      Res = WasmEdge_VMExecuteRegistered(VMCxt, ModName, FuncName, Params, 1, Returns, 1);
      if (WasmEdge_ResultOK(Res)) {
        printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Execution phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      }
      WasmEdge_StringDelete(ModName);
      WasmEdge_StringDelete(FuncName);
      WasmEdge_VMDelete(VMCxt);
      return 0;
    }
    ```

    Then you can compile and run: (the 20th Fibonacci number is 89 in 0-based index)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Get the result: 10946
    ```

### Instance Tracing

Sometimes the developers may have requirements to get the instances of the WASM runtime.
The `VM` context supplies the APIs to retrieve the instances.

1. Store

    If the `VM` context is created without assigning a `Store` context, the `VM` context will allocate and own a `Store` context.

    ```c
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
    WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
    /* The object should __NOT__ be deleted by `WasmEdge_StoreDelete()`. */
    WasmEdge_VMDelete(VMCxt);
    ```

    Developers can also create the `VM` context with a `Store` context.
    In this case, developers should guarantee the life cycle of the `Store` context.
    Please refer to the [Store Contexts](#Store) for the details about the `Store` context APIs.

    ```c
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, StoreCxt);
    WasmEdge_StoreContext *StoreCxtMock = WasmEdge_VMGetStoreContext(VMCxt);
    /* The `StoreCxt` and the `StoreCxtMock` are the same. */
    WasmEdge_VMDelete(VMCxt);
    WasmEdge_StoreDelete(StoreCxt);
    ```

2. List exported functions

    After the WASM module instantiation, developers can use the `WasmEdge_VMExecute()` API to invoke the exported WASM functions. For this purpose, developers may need information about the exported WASM function list.
    Please refer to the [Instances in runtime](#Instances) for the details about the function types.
    Assume that the WASM file [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and the C file `test.c` is as following:

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>
    int main() {
      WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, StoreCxt);

      WasmEdge_VMLoadWasmFromFile(VMCxt, "fibonacci.wasm");
      WasmEdge_VMValidate(VMCxt);
      WasmEdge_VMInstantiate(VMCxt);

      /* List the exported functions. */
      /* Get the number of exported functions. */
      uint32_t FuncNum = WasmEdge_VMGetFunctionListLength(VMCxt);
      /* Create the name buffers and the function type buffers. */
      WasmEdge_String FuncNames[256];
      WasmEdge_FunctionTypeContext *FuncTypes[256];
      /* 
       * Get the export function list.
       * The `FuncNames` and `FuncTypes` can be NULL if developers don't need them.
       */
      uint32_t GotFuncNum = WasmEdge_VMGetFunctionList(VMCxt, FuncNames, FuncTypes, FuncNum);

      for (uint32_t I = 0; I < GotFuncNum; I++) {
        char Buf[256];
        uint32_t Size = WasmEdge_StringCopy(FuncNames[I], Buf, sizeof(Buf));
        printf("Get exported function string length: %u, name: %s\n", Size, Buf);
        /* The function names and the function types should be destroyed. */
        WasmEdge_StringDelete(FuncNames[I]);
        WasmEdge_FunctionTypeDelete(FuncTypes[I]);
      }
      return 0;
    }
    ```

    Then you can compile and run: (the only exported function in `fibonacci.wasm` is `fib`)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Get exported function string length: 3, name: fib
    ```

    If developers want to get the exported function names in the registered WASM modules, please retrieve the `Store` context from the `VM` context and refer to the APIs of [Store Contexts](#Store) to list the registered functions by the module name.

3. Get function types

    The `VM` context provides APIs to find the function type by function name.
    Please refer to the [Instances in runtime](#Instances) for the details about the function types.

    ```c
    /* 
     * ...
     * Assume that a WASM module is instantiated in `VMCxt`.
     */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
    WasmEdge_FunctionTypeContext *FuncType = WasmEdge_VMGetFunctionType(VMCxt, FuncName);
    /* 
     * Developers can get the function types of functions in the registered modules
     * via the `WasmEdge_VMGetFunctionTypeRegistered()` API with the module name.
     * If the function is not found, these APIs will return `NULL`.
     * The returned function type contexts should be destroyed.
     */
    WasmEdge_FunctionTypeDelete(FuncType);
    WasmEdge_StringDelete(FuncName);
    ```

## WasmEdge Runtime

In this partition, we will introduce the objects of WasmEdge runtime manually.

### WASM Execution Example Step-By-Step

Besides the WASM execution through the [`VM` context](#WasmEdge-VM), developers can execute the WASM functions or instantiate WASM modules step-by-step with the `Loader`, `Validator`, `Interpreter`, and `Store` contexts.
Assume that the WASM file [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and the C file `test.c` is as following:

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main() {
  /* Create the configure context. This step is not necessary because we didn't adjust any setting. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  /* Create the statistics context. This step is not necessary if the statistics in runtime is not needed. */
  WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
  /* Create the store context. The store context is the WASM runtime structure core. */
  WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
  /* Result. */
  WasmEdge_Result Res;

  /* Create the loader context. The configure context can be NULL. */
  WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(ConfCxt);
  /* Create the validator context. The configure context can be NULL. */
  WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(ConfCxt);
  /* Create the interpreter context. The configure context and the statistics context can be NULL. */
  WasmEdge_InterpreterContext *InterpCxt = WasmEdge_InterpreterCreate(ConfCxt, StatCxt);

  /* Load the WASM file or the compiled-WASM file and convert into the AST module context. */
  WasmEdge_ASTModuleContext *ASTCxt = NULL;
  Res = WasmEdge_LoaderParseFromFile(LoadCxt, &ASTCxt, "fibonacci.wasm");
  if (!WasmEdge_ResultOK(Res)) {
    printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return 1;
  }
  /* Validate the WASM module. */
  Res = WasmEdge_ValidatorValidate(ValidCxt, ASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return 1;
  }
  /* Instantiate the WASM module into store context. */
  Res = WasmEdge_InterpreterInstantiate(InterpCxt, StoreCxt, ASTCxt);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return 1;
  }

  /* Try to list the exported functions of the instantiated WASM module. */
  uint32_t FuncNum = WasmEdge_StoreListFunctionLength(StoreCxt);
  /* Create the name buffers. */
  WasmEdge_String FuncNames[256];
  uint32_t GotFuncNum = WasmEdge_StoreListFunction(StoreCxt, FuncNames, FuncNum);
  for (uint32_t I = 0; I < GotFuncNum; I++) {
    char Buf[256];
    uint32_t Size = WasmEdge_StringCopy(FuncNames[I], Buf, sizeof(Buf));
    printf("Get exported function string length: %u, name: %s\n", Size, Buf);
    /* The function names should be destroyed. */
    WasmEdge_StringDelete(FuncNames[I]);
  }

  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(18) };
  WasmEdge_Value Returns[1];
  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
  /* Invoke the WASM fnction. */
  Res = WasmEdge_InterpreterInvoke(InterpCxt, StoreCxt, FuncName, Params, 1, Returns, 1);
  if (WasmEdge_ResultOK(Res)) {
    printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Execution phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  }

  /* Resources deallocations. */
  WasmEdge_StringDelete(FuncName);
  WasmEdge_ASTModuleDelete(ASTCxt);
  WasmEdge_LoaderDelete(LoadCxt);
  WasmEdge_ValidatorDelete(ValidCxt);
  WasmEdge_InterpreterDelete(InterpCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_StoreDelete(StoreCxt);
  WasmEdge_StatisticsDelete(StatCxt);
  return 0;
}
```

Then you can compile and run: (the 18th Fibonacci number is 4181 in 0-based index)

```bash
$ gcc test.c -lwasmedge_c
$ ./a.out
Get exported function string length: 3, name: fib
Get the result: 4181
```

### Loader

The `Loader` context loads the WASM binary from files or buffers.
Both the WASM and the compiled-WASM from the [WasmEdge AOT Compiler](#WasmEdge-AOT-Compiler) are supported.

```c
uint32_t Buf[4096];
/* ... Read the WASM code to the buffer. */
uint32_t FileSize = ...;
/* The `FileSize` is the length of the WASM code. */

/* Developers can adjust settings in the configure context. */
WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
/* Create the loader context. The configure context can be NULL. */
WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(ConfCxt);

WasmEdge_ASTModuleContext *ASTCxt = NULL;
WasmEdge_Result Res;

/* Load WASM or compiled-WASM from the file. */
Res = WasmEdge_LoaderParseFromFile(LoadCxt, &ASTCxt, "fibonacci.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
}
/* The output AST module context should be destroyed. */
WasmEdge_ASTModuleDelete(ASTCxt);

/* Load WASM or compiled-WASM from the file. */
Res = WasmEdge_LoaderParseFromBuffer(LoadCxt, &ASTCxt, Buf, FileSize);
if (!WasmEdge_ResultOK(Res)) {
  printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
}
/* The output AST module context should be destroyed. */
WasmEdge_ASTModuleDelete(ASTCxt);

WasmEdge_LoaderDelete(LoadCxt);
WasmEdge_ConfigureDelete(ConfCxt);
```

### Validator

The `Validator` context can validate the WASM module.
Every WASM module should be validated before instantiation.

```c
/* 
 * ...
 * Assume that the `ASTCxt` is the output AST module context from the loader context.
 * Assume that the `ConfCxt` is the configure context.
 */
/* Create the validator context. The configure context can be NULL. */
WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(ConfCxt);
WasmEdge_Result Res = WasmEdge_ValidatorValidate(ValidCxt, ASTCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
}
WasmEdge_ValidatorDelete(ValidCxt);
```

### Interpreter

The `Interpreter` context is the executor for both WASM and compiled-WASM.
This object should work base on the `Store` context. For the details of the `Store` context, please refer to the [next chapter](#Store).

1. Register modules

    As the same of [registering host modules](#Host-Module-Registrations) or [importing WASM modules](#WASM-Registrations-And-Executions) in `VM` context, developers can register `Import Object` or `AST module` contexts into the `Store` context by the `Interpreter` APIs.
    For the details of import objects, please refer to the [Host Functions](#Host-Functions).

    ```c
    /* 
    * ...
    * Assume that the `ASTCxt` is the output AST module context from the loader context
    * and has passed the validation.
    * Assume that the `ConfCxt` is the configure context.
    */
    /* Create the statistics context. This step is not necessary. */
    WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
    /* Create the interpreter context. The configure and the statistics contexts can be NULL. */
    WasmEdge_InterpreterContext *InterpCxt = WasmEdge_InterpreterCreate(ConfCxt, StatCxt);
    /* Create the store context. The store context is the WASM runtime structure core. */
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    /* Result. */
    WasmEdge_Result Res;

    /* Register the WASM module into store with the export module name "mod". */
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("mod");
    Res = WasmEdge_InterpreterRegisterModule(InterpCxt, StoreCxt, ASTCxt, ModName);
    if (!WasmEdge_ResultOK(Res)) {
      printf("WASM registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    WasmEdge_StringDelete(ModName);

    /* 
     * Assume that the `ImpCxt` is the import object context for host functions.
     */
    WasmEdge_ImportObjectContext *ImpCxt = ...;
    /* The import module context has already contained the export module name. */
    Res = WasmEdge_InterpreterRegisterImport(InterpCxt, StoreCxt, ImpCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Import object registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }

    WasmEdge_InterpreterDelete(InterpCxt);
    WasmEdge_StatisticsDelete(StatCxt);
    WasmEdge_StoreDelete(StoreCxt);
    ```

2. Instantiate modules

    WASM or compiled-WASM modules should be instantiated before the function invocation.
    Note that developers can only instantiate one module into the `Store` context, and in that case, the old instantiated module will be cleaned.
    Before instantiating a WASM module, please check the [import section](https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import) for ensuring the imports are registered into the `Store` context.

    ```c
    /* 
    * ...
    * Assume that the `ASTCxt` is the output AST module context from the loader context
    * and has passed the validation.
    * Assume that the `ConfCxt` is the configure context.
    */
    /* Create the statistics context. This step is not necessary. */
    WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
    /* Create the interpreter context. The configure and the statistics contexts can be NULL. */
    WasmEdge_InterpreterContext *InterpCxt = WasmEdge_InterpreterCreate(ConfCxt, StatCxt);
    /* Create the store context. The store context is the WASM runtime structure core. */
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();

    /* Instantiate the WASM module. */
    WasmEdge_Result Res = WasmEdge_InterpreterInstantiate(InterpCxt, StoreCxt, ASTCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("WASM instantiation failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }

    WasmEdge_InterpreterDelete(InterpCxt);
    WasmEdge_StatisticsDelete(StatCxt);
    WasmEdge_StoreDelete(StoreCxt);
    ```

3. Invoke functions

    As the same as function invocation via the `VM` context, developers can invoke the functions of the instantiated or registered modules.
    The APIs, `WasmEdge_InterpreterInvoke()` and `WasmEdge_InterpreterInvokeRegistered()`, are similar as the APIs of the `VM` context.
    Please refer to the [VM context workflows](#WASM-Execution-Example-With-VM-Context) for details.

### Store

[Store](https://webassembly.github.io/spec/core/exec/runtime.html#store) is the runtime structure for the representation of all instances of `Function`s, `Table`s, `Memory`s, and `Global`s that have been allocated during the lifetime of the abstract machine.
The `Store` context in WasmEdge provides APIs to list the exported instances with their names or find the instances by exported names. For adding instances into `Store` contexts, please instantiate or register WASM modules or `Import Object` contexts via the `Interpreter` context.

1. List instances

    ```c
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    /* ... Instantiate a WASM module via the interpreter context. */
    ...

    /* Try to list the exported functions of the instantiated WASM module. */
    /* Take the function instances for example here. */
    uint32_t FuncNum = WasmEdge_StoreListFunctionLength(StoreCxt);
    /* Create the name buffers. */
    WasmEdge_String FuncNames[256];
    uint32_t GotFuncNum = WasmEdge_StoreListFunction(StoreCxt, FuncNames, FuncNum);
    for (uint32_t I = 0; I < GotFuncNum; I++) {
      /* Working with the function name `FuncNames[I]` ... */
      /* The function names should be destroyed. */
      WasmEdge_StringDelete(FuncNames[I]);
    }
    ```

    Developers can list the function instance exported names of the registered modules via the `WasmEdge_StoreListFunctionRegisteredLength()` and the `WasmEdge_StoreListFunctionRegistered()` APIs with the module name.

2. Find instances

    ```c
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    /* ... Instantiate a WASM module via the interpreter context. */
    ...

    /* Try to find the exported instance of the instantiated WASM module. */
    /* Take the function instances for example here. */
    /* Function name. */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
    WasmEdge_FunctionInstanceContext *FuncCxt = WasmEdge_StoreFindFunction(StoreCxt, FuncName);
    /* `FuncCxt` will be `NULL` if the function not found. */
    /* The returned instance is owned by the store context and should __NOT__ be destroyed. */
    ```

    Developers can retrieve the exported function instances of the registered modules via the `WasmEdge_StoreFindFunctionRegistered()` API with the module name.

3. List registered modules

    With the module names, developers can list the exported instances of the registered modules with their names.

    ```c
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    /* ... Register a WASM module via the interpreter context. */
    ...

    /* Try to list registered WASM module. */
    uint32_t ModNum = WasmEdge_StoreListModuleLength(StoreCxt);
    /* Create the name buffers. */
    WasmEdge_String ModNames[256];
    uint32_t GotModNum = WasmEdge_StoreListModule(StoreCxt, ModNames, ModNum);
    for (uint32_t I = 0; I < GotModNum; I++) {
      /* Working with the module name `ModNames[I]` ... */
      /* The function names should be destroyed. */
      WasmEdge_StringDelete(ModNames[I]);
    }
    ```

### Instances

The instances are the runtime structures of WASM. Developers can retrieve the instances from the `Store` contexts.
The `Store` contexts will allocate instances when a WASM module or `Import Object` is registered or instantiated through the `Interpreter`.
A single instance can be allocated by its creation function. Developers can construct instances into an `Import Object` for registration. Please refer to the [Host Functions](#Host-Functions) for details.
The instances created by their creation functions should be destroyed, EXCEPT they are added into an `Import Object` context.

1. Function instance

    The `Function` context has no creation and deletion API.
    Developers can only retrieve the `Function` context from the `Store` context, and retrieve the `Function Type` from the `Function` contexts.

    ```
    /* Retrieve the function instance from the store context. */
    WasmEdge_FunctionInstanceContext *FuncCxt = ...;
    WasmEdge_FunctionTypeContext *FuncTypeCxt = WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
    /* The `FuncTypeCxt` is owned by the `FuncCxt` and should __NOT__ be destroyed. */
    ```

2. Function type

    The `Function Type` context is used for the `Host Function` creation or checking the value types of a `Function` instance. Developers can use the `Function Type` context APIs to get the value types information.

    ```
    enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I64 };
    enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_FuncRef };
    WasmEdge_FunctionTypeContext *FuncTypeCxt = WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);

    enum WasmEdge_ValType Buf[16];
    uint32_t ParamLen = WasmEdge_FunctionTypeGetParametersLength(FuncTypeCxt);
    /* `ParamLen` will be 2. */
    uint32_t GotParamLen = WasmEdge_FunctionTypeGetParameters(FuncTypeCxt, Buf, 16);
    /* `GotParamLen` will be 2, and `Buf[0]` and `Buf[1]` will be the same as `ParamList`. */
    uint32_t ReturnLen = WasmEdge_FunctionTypeGetReturnsLength(FuncTypeCxt);
    /* `ReturnLen` will be 1. */
    uint32_t GotReturnLen = WasmEdge_FunctionTypeGetReturns(FuncTypeCxt, Buf, 16);
    /* `GotReturnLen` will be 1, and `Buf[0]` will be the same as `ReturnList`. */

    WasmEdge_FunctionTypeDelete(FuncTypeCxt);
    ```

3. Host function instance

    [Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
    In WasmEdge, developers can create the `Host Function` contexts and add them into an `Import Object` context for registering into a `VM` or a `Store`.
    For the details of the `Host Function` guide, please refer to the [next chapter](#Host-Functions).

4. Table instance

    In WasmEdge, developers can create the `Table` contexts and add them into an `Import Object` context for registering into a `VM` or a `Store`.
    The `Table` contexts supply APIs to control the data in table instances.

    ```c
    WasmEdge_Limit TabLimit = {.HasMax = true, .Min = 10, .Max = 20};
    /* Create the table instance with limit and the `FuncRef` element type. */
    WasmEdge_TableInstanceContext *HostTable = WasmEdge_TableInstanceCreate(WasmEdge_RefType_FuncRef, TabLimit);
    WasmEdge_Result Res;
    WasmEdge_Value Data;

    enum WasmEdge_RefType RefType = WasmEdge_TableInstanceGetRefType(HostTable);
    /* `RefType` will be `WasmEdge_RefType_FuncRef`. */
    Data = WasmEdge_ValueGenFuncRef(5);
    Res = WasmEdge_TableInstanceSetData(HostTable, Data, 3);
    /* Set the function index 5 to the table[3]. */
    /*
     * This will get an "out of bounds table access" error:
     *   Res = WasmEdge_TableInstanceSetData(HostTable, Data, 13);
     */
    Res = WasmEdge_TableInstanceGetData(HostTable, &Data, 3);
    /* Get the FuncRef value of the table[3]. */
    /*
     * This will get an "out of bounds table access" error:
     *   Res = WasmEdge_TableInstanceGetData(HostTable, &Data, 13);
     */

    uint32_t Size = WasmEdge_TableInstanceGetSize(HostTable);
    /* `Size` will be 10. */
    Res = WasmEdge_TableInstanceGrow(HostTable, 6);
    /* Grow the table size of 6, the table size will be 16. */
    /*
     * This will get an "out of bounds table access" error because
     * the size (16 + 6) will reach the table limit:
     *   Res = WasmEdge_TableInstanceGrow(HostTable, 6);
     */

    WasmEdge_TableInstanceDelete(HostTable);
    ```

5. Memory instance

    In WasmEdge, developers can create the `Memory` contexts and add them into an `Import Object` context for registering into a `VM` or a `Store`.
    The `Memory` contexts supply APIs to control the data in memory instances.

    ```c
    WasmEdge_Limit MemLimit = {.HasMax = true, .Min = 1, .Max = 5};
    /* Create the memory instance with limit. The memory page size is 64KiB. */
    WasmEdge_MemoryInstanceContext *HostMemory = WasmEdge_MemoryInstanceCreate(MemLimit);
    WasmEdge_Result Res;
    uint8_t Buf[256];

    Buf[0] = 0xAA;
    Buf[1] = 0xBB;
    Buf[2] = 0xCC;
    Res = WasmEdge_MemoryInstanceSetData(HostMemory, Buf, 0x1000, 3);
    /* Set the data[0:2] to the memory[4096:4098]. */
    /*
     * This will get an "out of bounds memory access" error
     * because [65535:65537] is out of 1 page size (65536):
     *   Res = WasmEdge_MemoryInstanceSetData(HostMemory, Buf, 0xFFFF, 3);
     */
    Buf[0] = 0;
    Buf[1] = 0;
    Buf[2] = 0;
    Res = WasmEdge_MemoryInstanceGetData(HostMemory, Buf, 0x1000, 3);
    /* Get the memory[4096:4098]. Buf[0:2] will be `{0xAA, 0xBB, 0xCC}`. */
    /*
     * This will get an "out of bounds memory access" error:
     *   Res = WasmEdge_MemoryInstanceSetData(HostMemory, Buf, 0xFFFF, 3);
     */

    uint32_t PageSize = WasmEdge_MemoryInstanceGetPageSize(HostMemory);
    /* `PageSize` will be 1. */
    Res = WasmEdge_MemoryInstanceGrowPage(HostMemory, 2);
    /* Grow the page size of 2, the page size of the memory instance will be 3. */
    /*
     * This will get an "out of bounds memory access" error because
     * the page size (3 + 3) will reach the memory limit:
     *   Res = WasmEdge_MemoryInstanceGrowPage(HostMemory, 3);
     */

    WasmEdge_MemoryInstanceDelete(HostMemory);
    ```

6. Global instance

    In WasmEdge, developers can create the `Global` contexts and add them into an `Import Object` context for registering into a `VM` or a `Store`.
    The `Global` contexts supply APIs to control the value in global instances.

    ```c
    /* Create the global instance with value and mutation. */
    WasmEdge_Value Val = WasmEdge_ValueGenI64(1000);
    WasmEdge_GlobalInstanceCreate *HostGlobal = WasmEdge_GlobalInstanceCreate(Val, WasmEdge_Mutability_Var);
    WasmEdge_Result Res;

    enum WasmEdge_ValType ValType = WasmEdge_GlobalInstanceGetValType(HostGlobal);
    /* `ValType` will be `WasmEdge_ValType_I64`. */
    enum WasmEdge_Mutability ValMut = WasmEdge_GlobalInstanceGetMutability(HostGlobal);
    /* `ValMut` will be `WasmEdge_Mutability_Var`. */
    
    WasmEdge_GlobalInstanceSetValue(HostGlobal, WasmEdge_ValueGenI64(888));
    /* 
     * Set the value u64(888) to the global.
     * This function will do nothing if the value type mismatched or
     * the global mutability is `WasmEdge_Mutability_Const`.
     */
    WasmEdge_Value GlobVal = WasmEdge_GlobalInstanceGetValue(HostGlobal);
    /* Get the value (888 now) of the global context. */

    WasmEdge_GlobalInstanceDelete(HostGlobal);
    ```

### Host Functions

[Host functions](https://webassembly.github.io/spec/core/exec/runtime.html#syntax-hostfunc) are functions outside WebAssembly and passed to WASM modules as imports.
In WasmEdge, developers can create the `Host Function`, `Memory`, `Table`, and `Global` contexts and add them into an `Import Object` context for registering into a `VM` or a `Store`.

1. Host function allocation

    Developers can define C functions with the following function signature as the host function body:

    ```c
    typedef WasmEdge_Result (*WasmEdge_HostFunc_t)(
      void *Data,
      WasmEdge_MemoryInstanceContext *MemCxt,
      const WasmEdge_Value *Params,
      WasmEdge_Value *Returns);
    ```

    The example of an `add` host function to add 2 `i32` values:

    ```c
    WasmEdge_Result Add(void *, WasmEdge_MemoryInstanceContext *,
                        const WasmEdge_Value *In, WasmEdge_Value *Out) {
      /*
      * Params: {i32, i32}
      * Returns: {i32}
      * Developers should take care about the function type.
      */ 
      /* Retrieve the value 1. */
      int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
      /* Retrieve the value 2. */
      int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
      /* Output value 1 is Val1 + Val2. */
      Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
      /* Return the status of success. */
      return WasmEdge_Result_Success;
    }
    ```

    Then developers can create `Host Function` contexts with the function types:

    ```c
    enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
    enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
    /* Create a function type: {i32, i32} -> {i32}. */
    HostFType = WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
    /* 
     * Create a host function context with the function type and body.
     * The `Cost` parameter can be 0 if developers do not need the cost measuring.
     */
    WasmEdge_HostFunctionContext *HostFunc = WasmEdge_HostFunctionCreate(HostFType, Add, NULL, 0);
    /*
     * The second parameter is the pointer to the additional data.
     * Developers should guarantee the life cycle of the data, and it can be
     * `NULL` if the external data is not needed.
     */

    /* If the host function instance is not added into an import object context, it should be deleted. */
    WasmEdge_HostFunctionDelete(HostFunc);
    ```

2. Import object context

    The `Import Object` context holds an exporting module name and the instances. Developers can add the `Host Function`, `Memory`, `Table`, and `Global` instances with their exporting names.

    ```c
    /* Host function body definition. */
    WasmEdge_Result Add(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                        const WasmEdge_Value *In, WasmEdge_Value *Out) {
      int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
      int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
      Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
      return WasmEdge_Result_Success;
    }

    /* Create the import object. */
    WasmEdge_String ExportName = WasmEdge_StringCreateByCString("module");
    WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(ExportName);
    WasmEdge_StringDelete(ExportName);

    /* Create and add a host function instance into the import object. */
    enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
    enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
    WasmEdge_FunctionTypeContext *HostFType = 
      WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
    WasmEdge_HostFunctionContext *HostFunc =
      WasmEdge_HostFunctionCreate(HostFType, Add, NULL, 0);
    /*
     * The third parameter is the pointer to the additional data object.
     * Developers should guarantee the life cycle of the data, and it can be
     * `NULL` if the external data is not needed.
     */
    WasmEdge_FunctionTypeDelete(HostFType);
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("add");
    WasmEdge_ImportObjectAddHostFunction(ImpObj, FuncName, HostFunc);
    WasmEdge_StringDelete(FuncName);

    /* Create and add a table instance into the import object. */
    WasmEdge_Limit TableLimit = {.HasMax = true, .Min = 10, .Max = 20};
    WasmEdge_TableInstanceContext *HostTable =
      WasmEdge_TableInstanceCreate(WasmEdge_RefType_FuncRef, TabLimit);
    WasmEdge_String TableName = WasmEdge_StringCreateByCString("table");
    WasmEdge_ImportObjectAddTable(ImpObj, TableName, HostTable);
    WasmEdge_StringDelete(TableName);

    /* Create and add a memory instance into the import object. */
    WasmEdge_Limit MemoryLimit = {.HasMax = true, .Min = 1, .Max = 2};
    WasmEdge_MemoryInstanceContext *HostMemory = 
      WasmEdge_MemoryInstanceCreate(MemLimit);
    WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
    WasmEdge_ImportObjectAddMemory(ImpObj, MemoryName, HostMemory);
    WasmEdge_StringDelete(MemoryName);

    /* Create and add a global instance into the import object. */
    WasmEdge_GlobalInstanceContext *HostGlobal =
      WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenI32(666), WasmEdge_Mutability_Var);
    WasmEdge_String GlobalName = WasmEdge_StringCreateByCString("global");
    WasmEdge_ImportObjectAddGlobal(ImpObj, GlobalName, HostGlobal);
    WasmEdge_StringDelete(GlobalName);

    /*
     * Developers should destroy the import object context if it is not registered into a
     * store context or a VM context.
     * Developers should __NOT__ destroy the instances added into the import object contexts.
     */
    WasmEdge_ImportObjectDelete(ImpObj);
    ```

3. Specified import object

    `WasmEdge_ImportObjectCreateWASI()` API can create and initialize the `WASI` import object.
    `WasmEdge_ImportObjectCreateWasmEdgeProcess()` API can create and initializae the `wasmedge_process` import object.
    Developers can create these import object contexts and register them into the `Store` or `VM` contexts rather than adjust the settings in the `Configure` contexts.

    ```c
    WasmEdge_ImportObjectContext *WasiObj = WasmEdge_ImportObjectCreateWASI( /* ... ignored */ );
    WasmEdge_ImportObjectContext *ProcObj = WasmEdge_ImportObjectCreateWasmEdgeProcess( /* ... ignored */ );
    WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);
    /* Register the WASI and WasmEdge_Process into the VM context. */
    WasmEdge_VMRegisterModuleFromImport(VMCxt, WasiObj);
    WasmEdge_VMRegisterModuleFromImport(VMCxt, ProcObj);
    WasmEdge_VMDelete(VMCxt);
    ```

4. Example

    Assume that a simple WASM from the WAT as following:

    ```
    (module
      (type $t0 (func (param i32 i32) (result i32)))
      (import "extern" "func-add" (func $f-add (type $t0)))
      (func (export "addTwo") (param i32 i32) (result i32)
        local.get 0
        local.get 1
        call $f-add)
    )
    ```

    And the `test.c` as following:

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>

    /* Host function body definition. */
    WasmEdge_Result Add(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                        const WasmEdge_Value *In, WasmEdge_Value *Out) {
      int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
      int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
      printf("Host function \"Add\": %d + %d\n", Val1, Val2);
      Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
      return WasmEdge_Result_Success;
    }

    int main() {
      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* The WASM module buffer. */
      uint8_t WASM[] = {
        /* WASM header */
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
        /* Type section */
        0x01, 0x07, 0x01,
        /* function type {i32, i32} -> {i32} */
        0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
        /* Import section */
        0x02, 0x13, 0x01,
        /* module name: "extern" */
        0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E,
        /* extern name: "func-add" */
        0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
        /* import desc: func 0 */
        0x00, 0x00,
        /* Function section */
        0x03, 0x02, 0x01, 0x00,
        /* Export section */
        0x07, 0x0A, 0x01,
        /* export name: "addTwo" */
        0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F,
        /* export desc: func 0 */
        0x00, 0x01,
        /* Code section */
        0x0A, 0x0A, 0x01,
        /* code body */
        0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B
      };

      /* Create the import object. */
      WasmEdge_String ExportName = WasmEdge_StringCreateByCString("extern");
      WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(ExportName);
      enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
      enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
      WasmEdge_FunctionTypeContext *HostFType = WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
      WasmEdge_HostFunctionContext *HostFunc = WasmEdge_HostFunctionCreate(HostFType, Add, NULL, 0);
      WasmEdge_FunctionTypeDelete(HostFType);
      WasmEdge_String HostFuncName = WasmEdge_StringCreateByCString("func-add");
      WasmEdge_ImportObjectAddHostFunction(ImpObj, HostFuncName, HostFunc);
      WasmEdge_StringDelete(HostFuncName);

      WasmEdge_VMRegisterModuleFromImport(VMCxt, ImpObj);

      /* The parameters and returns arrays. */
      WasmEdge_Value Params[2] = { WasmEdge_ValueGenI32(1234), WasmEdge_ValueGenI32(5678) };
      WasmEdge_Value Returns[1];
      /* Function name. */
      WasmEdge_String FuncName = WasmEdge_StringCreateByCString("addTwo");
      /* Run the WASM function from file. */
      WasmEdge_Result Res = WasmEdge_VMRunWasmFromBuffer(
        VMCxt, WASM, sizeof(WASM), FuncName, Params, 2, Returns, 1);

      if (WasmEdge_ResultOK(Res)) {
        printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
      }

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      WasmEdge_StringDelete(FuncName);
      WasmEdge_ImportObjectDelete(ImpObj);
      return 0;
    }
    ```

    Then you can compile and run: (the result of 1234 + 5678 is 6912)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Host function "Add": 1234 + 5678
    Get the result: 6912
    ```

5. Host Data Example

    Developers can set a external data object to the host function context, and access to the object in the function body.
    Assume that a simple WASM from the WAT as following:

    ```
    (module
      (type $t0 (func (param i32 i32) (result i32)))
      (import "extern" "func-add" (func $f-add (type $t0)))
      (func (export "addTwo") (param i32 i32) (result i32)
        local.get 0
        local.get 1
        call $f-add)
    )
    ```

    And the `test.c` as following:

    ```c
    #include <wasmedge/wasmedge.h>
    #include <stdio.h>

    /* Host function body definition. */
    WasmEdge_Result Add(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                        const WasmEdge_Value *In, WasmEdge_Value *Out) {
      int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
      int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
      printf("Host function \"Add\": %d + %d\n", Val1, Val2);
      Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
      /* Also set the result to the data. */
      int32_t *DataPtr = (int32_t *)Data;
      *DataPtr = Val1 + Val2;
      return WasmEdge_Result_Success;
    }

    int main() {
      /* Create the VM context. */
      WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, NULL);

      /* The WASM module buffer. */
      uint8_t WASM[] = {
        /* WASM header */
        0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
        /* Type section */
        0x01, 0x07, 0x01,
        /* function type {i32, i32} -> {i32} */
        0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F,
        /* Import section */
        0x02, 0x13, 0x01,
        /* module name: "extern" */
        0x06, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6E,
        /* extern name: "func-add" */
        0x08, 0x66, 0x75, 0x6E, 0x63, 0x2D, 0x61, 0x64, 0x64,
        /* import desc: func 0 */
        0x00, 0x00,
        /* Function section */
        0x03, 0x02, 0x01, 0x00,
        /* Export section */
        0x07, 0x0A, 0x01,
        /* export name: "addTwo" */
        0x06, 0x61, 0x64, 0x64, 0x54, 0x77, 0x6F,
        /* export desc: func 0 */
        0x00, 0x01,
        /* Code section */
        0x0A, 0x0A, 0x01,
        /* code body */
        0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0B
      };

      /* The external data object: an integer. */
      int32_t Data;

      /* Create the import object. */
      WasmEdge_String ExportName = WasmEdge_StringCreateByCString("extern");
      WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(ExportName);
      enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
      enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
      WasmEdge_FunctionTypeContext *HostFType = WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
      WasmEdge_HostFunctionContext *HostFunc = WasmEdge_HostFunctionCreate(HostFType, Add, &Data, 0);
      WasmEdge_FunctionTypeDelete(HostFType);
      WasmEdge_String HostFuncName = WasmEdge_StringCreateByCString("func-add");
      WasmEdge_ImportObjectAddHostFunction(ImpObj, HostFuncName, HostFunc);
      WasmEdge_StringDelete(HostFuncName);

      WasmEdge_VMRegisterModuleFromImport(VMCxt, ImpObj);

      /* The parameters and returns arrays. */
      WasmEdge_Value Params[2] = { WasmEdge_ValueGenI32(1234), WasmEdge_ValueGenI32(5678) };
      WasmEdge_Value Returns[1];
      /* Function name. */
      WasmEdge_String FuncName = WasmEdge_StringCreateByCString("addTwo");
      /* Run the WASM function from file. */
      WasmEdge_Result Res = WasmEdge_VMRunWasmFromBuffer(
        VMCxt, WASM, sizeof(WASM), FuncName, Params, 2, Returns, 1);

      if (WasmEdge_ResultOK(Res)) {
        printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
      } else {
        printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
      }
      printf("Data value: %d\n", Data);

      /* Resources deallocations. */
      WasmEdge_VMDelete(VMCxt);
      WasmEdge_StringDelete(FuncName);
      WasmEdge_ImportObjectDelete(ImpObj);
      return 0;
    }
    ```

    Then you can compile and run: (the result of 1234 + 5678 is 6912)

    ```bash
    $ gcc test.c -lwasmedge_c
    $ ./a.out
    Host function "Add": 1234 + 5678
    Get the result: 6912
    Data value: 6912
    ```

## WasmEdge AOT Compiler

In this partition, we will introduce the WasmEdge AOT compiler and the options.
WasmEdge runs the WASM files in interpreter mode, and WasmEdge also supports the AOT (ahead-of-time) mode running without modifying any code.
The WasmEdge AOT (ahead-of-time) compiler compiles the WASM files for running in AOT mode which is much faster than interpreter mode. Developers can compile the WASM files into the compiled-WASM files for the AOT mode running.

### Compilation Example

Assume that the WASM file [`fibonacci.wasm`](../tools/wasmedge/examples/fibonacci.wasm) is copied into the current directory, and the C file `test.c` is as following:

```c
#include <wasmedge/wasmedge.h>
#include <stdio.h>
int main() {
  /* Create the configure context. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  /* ... Adjust settings in the configure context. */
  /* Result. */
  WasmEdge_Result Res;

  /* Create the compiler context. The configure context can be NULL. */
  WasmEdge_CompilerContext *CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
  /* Compile the WASM file with input and output paths. */
  Res = WasmEdge_CompilerCompile(CompilerCxt, "fibonacci.wasm", "fibonacci.wasm.so");
  if (!WasmEdge_ResultOK(Res)) {
      printf("Compilation failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return 1;
  }

  WasmEdge_CompilerDelete(CompilerCxt);
  WasmEdge_ConfigureDelete(ConfCxt);
  return 0;
}
```

Then you can compile and run (the output file is "fibonacci.wasm.so"):

```bash
$ gcc test.c -lwasmedge_c
$ ./a.out
[2021-07-02 11:08:08.651] [info] compile start
[2021-07-02 11:08:08.653] [info] verify start
[2021-07-02 11:08:08.653] [info] optimize start
[2021-07-02 11:08:08.670] [info] codegen start
[2021-07-02 11:08:08.706] [info] compile done
```

### Compiler Options

Developers and add options for AOT compilers such as optimization level:

```c
/// AOT compiler optimization level enumeration.
enum WasmEdge_CompilerOptimizationLevel {
  /// Disable as many optimizations as possible.
  WasmEdge_CompilerOptimizationLevel_O0 = 0,
  /// Optimize quickly without destroying debuggability.
  WasmEdge_CompilerOptimizationLevel_O1,
  /// Optimize for fast execution as much as possible without triggering
  /// significant incremental compile time or code size growth.
  WasmEdge_CompilerOptimizationLevel_O2,
  /// Optimize for fast execution as much as possible.
  WasmEdge_CompilerOptimizationLevel_O3,
  /// Optimize for small code size as much as possible without triggering
  /// significant incremental compile time or execution time slowdowns.
  WasmEdge_CompilerOptimizationLevel_Os,
  /// Optimize for small code size as much as possible.
  WasmEdge_CompilerOptimizationLevel_Oz
};
```

Please refer to the [AOT compiler options configuration](#Configurations) for details.
