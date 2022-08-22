# Upgrade to WasmEdge 0.10.0

Due to the WasmEdge C API breaking changes, this document shows the guideline of programming with WasmEdge C API to upgrade from the `0.9.1` to the `0.10.0` version.

## Concepts

1. Merged the `WasmEdge_ImportObjectContext` into the `WasmEdge_ModuleInstanceContext`.

   The `WasmEdge_ImportObjectContext` which is for the host functions is merged into `WasmEdge_ModuleInstanceContext`.
   Developers can use the related APIs to construct host modules.

   * `WasmEdge_ImportObjectCreate()` is changed to `WasmEdge_ModuleInstanceCreate()`.
   * `WasmEdge_ImportObjectDelete()` is changed to `WasmEdge_ModuleInstanceDelete()`.
   * `WasmEdge_ImportObjectAddFunction()` is changed to `WasmEdge_ModuleInstanceAddFunction()`.
   * `WasmEdge_ImportObjectAddTable()` is changed to `WasmEdge_ModuleInstanceAddTable()`.
   * `WasmEdge_ImportObjectAddMemory()` is changed to `WasmEdge_ModuleInstanceAddMemory()`.
   * `WasmEdge_ImportObjectAddGlobal()` is changed to `WasmEdge_ModuleInstanceAddGlobal()`.
   * `WasmEdge_ImportObjectCreateWASI()` is changed to `WasmEdge_ModuleInstanceCreateWASI()`.
   * `WasmEdge_ImportObjectCreateWasmEdgeProcess()` is changed to `WasmEdge_ModuleInstanceCreateWasmEdgeProcess()`.
   * `WasmEdge_ImportObjectInitWASI()` is changed to `WasmEdge_ModuleInstanceInitWASI()`.
   * `WasmEdge_ImportObjectInitWasmEdgeProcess()` is changed to `WasmEdge_ModuleInstanceInitWasmEdgeProcess()`.

   For the new host function examples, please refer to [the example below](#host-functions).

2. Used the pointer to `WasmEdge_FunctionInstanceContext` instead of the index in the `FuncRef` value type.

   For the better performance and security, the `FuncRef` related APIs used the `const WasmEdge_FunctionInstanceContext *` for the parameters and returns.

   * `WasmEdge_ValueGenFuncRef()` is changed to use the `const WasmEdge_FunctionInstanceContext *` as it's argument.
   * `WasmEdge_ValueGetFuncRef()` is changed to return the `const WasmEdge_FunctionInstanceContext *`.

3. Supported multiple anonymous WASM module instantiation.

   In the version before `0.9.1`, WasmEdge only supports 1 anonymous WASM module to be instantiated at one time. If developers instantiate a new WASM module, the old one will be replaced.
   After the `0.10.0` version, developers can instantiate multiple anonymous WASM module by `Executor` and get the `Module` instance. But for the source code using the `VM` APIs, the behavior is not changed.
   For the new examples of instantiating multiple anonymous WASM modules, please refer to [the example below](#wasmedge-executor-changes).

4. Behavior changed of `WasmEdge_StoreContext`.

   The `Function`, `Table`, `Memory`, and `Global` instances retrievement from the `Store` is moved to the `Module` instance. The `Store` only manage the module linking when instantiation and the named module searching after the `0.10.0` version.

   * `WasmEdge_StoreListFunctionLength()` and `WasmEdge_StoreListFunctionRegisteredLength()` is replaced by `WasmEdge_ModuleInstanceListFunctionLength()`.
   * `WasmEdge_StoreListTableLength()` and `WasmEdge_StoreListTableRegisteredLength()` is replaced by `WasmEdge_ModuleInstanceListTableLength()`.
   * `WasmEdge_StoreListMemoryLength()` and `WasmEdge_StoreListMemoryRegisteredLength()` is replaced by `WasmEdge_ModuleInstanceListMemoryLength()`.
   * `WasmEdge_StoreListGlobalLength()` and `WasmEdge_StoreListGlobalRegisteredLength()` is replaced by `WasmEdge_ModuleInstanceListGlobalLength()`.
   * `WasmEdge_StoreListFunction()` and `WasmEdge_StoreListFunctionRegistered()` is replaced by `WasmEdge_ModuleInstanceListFunction()`.
   * `WasmEdge_StoreListTable()` and `WasmEdge_StoreListTableRegistered()` is replaced by `WasmEdge_ModuleInstanceListTable()`.
   * `WasmEdge_StoreListMemory()` and `WasmEdge_StoreListMemoryRegistered()` is replaced by `WasmEdge_ModuleInstanceListMemory()`.
   * `WasmEdge_StoreListGlobal()` and `WasmEdge_StoreListGlobalRegistered()` is replaced by `WasmEdge_ModuleInstanceListGlobal()`.
   * `WasmEdge_StoreFindFunction()` and `WasmEdge_StoreFindFunctionRegistered()` is replaced by `WasmEdge_ModuleInstanceFindFunction()`.
   * `WasmEdge_StoreFindTable()` and `WasmEdge_StoreFindTableRegistered()` is replaced by `WasmEdge_ModuleInstanceFindTable()`.
   * `WasmEdge_StoreFindMemory()` and `WasmEdge_StoreFindMemoryRegistered()` is replaced by `WasmEdge_ModuleInstanceFindMemory()`.
   * `WasmEdge_StoreFindGlobal()` and `WasmEdge_StoreFindGlobalRegistered()` is replaced by `WasmEdge_ModuleInstanceFindGlobal()`.

   For the new examples of retrieving instances, please refer to [the example below](#instances-retrievement).

5. The `WasmEdge_ModuleInstanceContext`-based resource management.

   Except the creation of `Module` instance for the host functons, the `Executor` will output a `Module` instance after instantiation. No matter the anonymous or named modules, developers have the responsibility to destroy them by `WasmEdge_ModuleInstanceDelete()` API.
   The `Store` will link to the named `Module` instance after registering. After the destroyment of a `Module` instance, the `Store` will unlink to that automatically; after the destroyment of the `Store`, the all `Module` instances the `Store` linked to will unlink to that `Store` automatically.

## WasmEdge VM changes

The `VM` APIs are basically not changed, except the `ImportObject` related APIs.

The following is the example of WASI initialization in WasmEdge `0.9.1` C API:

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

/* ... */

WasmEdge_VMDelete(VMCxt);
WasmEdge_ConfigureDelete(ConfCxt);
```

Developers can change to use the WasmEdge `0.10.0` C API as follows, with only replacing the `WasmEdge_ImportObjectContext` into `WasmEdge_ModuleInstanceContext`:

```c
WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);
/* The following API can retrieve the pre-registration module instances from the VM context. */
/* This API will return `NULL` if the corresponding pre-registration is not set into the configuration. */
WasmEdge_ModuleInstanceContext *WasiModule =
    WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
/* Initialize the WASI. */
WasmEdge_ModuleInstanceInitWASI(WasiModule, /* ... ignored */ );

/* ... */

WasmEdge_VMDelete(VMCxt);
WasmEdge_ConfigureDelete(ConfCxt);
```

The `VM` provides a new API for getting the current instantiated anonymous `Module` instance.
For example, if developer want to get the exported `Global` instance:

```c
/* Assume that a WASM module is instantiated in `VMCxt`, and exports the "global_i32". */
WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
WasmEdge_String GlobName = WasmEdge_StringCreateByCString("global_i32");
WasmEdge_GlobalInstanceContext *GlobCxt = WasmEdge_StoreFindGlobal(StoreCxt, GlobName);
WasmEdge_StringDelete(GlobName);
```

After the WasmEdge `0.10.0` C API, developers can use the `WasmEdge_VMGetActiveModule()` to get the module instance:

```c
/* Assume that a WASM module is instantiated in `VMCxt`, and exports the "global_i32". */
const WasmEdge_ModuleInstanceContext *ModCxt = WasmEdge_VMGetActiveModule(VMCxt);
/* The example of retrieving the global instance. */
WasmEdge_String GlobName = WasmEdge_StringCreateByCString("global_i32");
WasmEdge_GlobalInstanceContext *GlobCxt = WasmEdge_ModuleInstanceFindGlobal(ModCxt, GlobName);
WasmEdge_StringDelete(GlobName);
```

## WasmEdge Executor changes

`Executor` helps to instantiate a WASM module, register a WASM module into `Store` with module name, register the host modules with host functions, or invoke functions.

1. WASM module instantiation

    In WasmEdge `0.9.1` version, developers can instantiate a WASM module by the `Executor` API:

    ```c
    WasmEdge_ASTModuleContext *ASTCxt;
    /*
     * Assume that `ASTCxt` is a loaded WASM from file or buffer and has passed the validation.
     * Assume that `ExecCxt` is a `WasmEdge_ExecutorContext`.
     * Assume that `StoreCxt` is a `WasmEdge_StoreContext`.
     */
    WasmEdge_Result Res = WasmEdge_ExecutorInstantiate(ExecCxt, StoreCxt, ASTCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

    Then the WASM module is instantiated into an anonymous module instance and handled by the `Store`.
    If a new WASM module is instantiated by this API, the old instantiated module instance will be cleaned.
    After the WasmEdge `0.10.0` version, the instantiated anonymous module will be outputed and handled by caller, and not only 1 anonymous module instance can be instantiated.
    Developers have the responsibility to destroy the outputed module instances.

    ```c
    WasmEdge_ASTModuleContext *ASTCxt1, *ASTCxt2;
    /*
     * Assume that `ASTCxt1` and `ASTCxt2` are loaded WASMs from different files or buffers,
     * and have both passed the validation.
     * Assume that `ExecCxt` is a `WasmEdge_ExecutorContext`.
     * Assume that `StoreCxt` is a `WasmEdge_StoreContext`.
     */
    WasmEdge_ModuleInstanceContext *ModCxt1 = NULL;
    WasmEdge_ModuleInstanceContext *ModCxt2 = NULL;
    WasmEdge_Result Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt1, StoreCxt, ASTCxt1);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt2, StoreCxt, ASTCxt2);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

2. WASM module registration with module name

    When instantiating and registering a WASM module with module name, developers can use the `WasmEdge_ExecutorRegisterModule()` API before WasmEdge `0.9.1`.

    ```c
    WasmEdge_ASTModuleContext *ASTCxt;
    /*
     * Assume that `ASTCxt` is a loaded WASM from file or buffer and has passed the validation.
     * Assume that `ExecCxt` is a `WasmEdge_ExecutorContext`.
     * Assume that `StoreCxt` is a `WasmEdge_StoreContext`.
     */

    /* Register the WASM module into store with the export module name "mod". */
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("mod");
    Res = WasmEdge_ExecutorRegisterModule(ExecCxt, StoreCxt, ASTCxt, ModName);
    WasmEdge_StringDelete(ModName);
    if (!WasmEdge_ResultOK(Res)) {
      printf("WASM registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

    The same feature is implemented in WasmEdge `0.10.0`, but in different API `WasmEdge_ExecutorRegister()`:

    ```c
    WasmEdge_ASTModuleContext *ASTCxt;
    /*
     * Assume that `ASTCxt` is a loaded WASM from file or buffer and has passed the validation.
     * Assume that `ExecCxt` is a `WasmEdge_ExecutorContext`.
     * Assume that `StoreCxt` is a `WasmEdge_StoreContext`.
     */

    /* Register the WASM module into store with the export module name "mod". */
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("mod");
    /* The output module instance. */
    WasmEdge_ModuleInstanceContext *ModCxt = NULL;
    Res = WasmEdge_ExecutorRegister(ExecCxt, &ModCxt, StoreCxt, ASTCxt, ModName);
    WasmEdge_StringDelete(ModName);
    if (!WasmEdge_ResultOK(Res)) {
      printf("WASM registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

    Developers have the responsibility to destroy the outputed module instances.

3. Host module registration

    In WasmEdge `0.9.1`, developers can create a `WasmEdge_ImportObjectContext` and register into `Store`.

    ```c
    /* Create the import object with the export module name. */
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("module");
    WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(ModName);
    WasmEdge_StringDelete(ModName);
    /*
     * ...
     * Add the host functions, tables, memories, and globals into the import object.
     */
    /* The import module context has already contained the export module name. */
    Res = WasmEdge_ExecutorRegisterImport(ExecCxt, StoreCxt, ImpObj);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Import object registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

    After WasmEdge `0.10.0`, developers should use the `WasmEdge_ModuleInstanceContext` instead:

    ```c
    /* Create the module instance with the export module name. */
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("module");
    WasmEdge_ModuleInstanceContext *ModCxt = WasmEdge_ModuleInstanceCreate(ModName);
    WasmEdge_StringDelete(ModName);
    /*
     * ...
     * Add the host functions, tables, memories, and globals into the module instance.
     */
    /* The module instance context has already contained the export module name. */
    Res = WasmEdge_ExecutorRegisterImport(ExecCxt, StoreCxt, ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Module instance registration failed: %s\n", WasmEdge_ResultGetMessage(Res));
    }
    ```

    Developers have the responsibility to destroy the created module instances.

4. WASM function invocation

    This example uses the [fibonacci.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wat).
    In WasmEdge `0.9.1` version, developers can invoke a WASM function with the export function name:

    ```c
    /* Create the store context. The store context holds the instances. */
    WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
    /* Result. */
    WasmEdge_Result Res;

    /* Create the loader context. The configure context can be NULL. */
    WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
    /* Create the validator context. The configure context can be NULL. */
    WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(NULL);
    /* Create the executor context. The configure context and the statistics context can be NULL. */
    WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);

    /* Load the WASM file or the compiled-WASM file and convert into the AST module context. */
    WasmEdge_ASTModuleContext *ASTCxt = NULL;
    Res = WasmEdge_LoaderParseFromFile(LoadCxt, &ASTCxt, "fibonacci.wasm");
    if (!WasmEdge_ResultOK(Res)) {
      printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }
    /* Validate the WASM module. */
    Res = WasmEdge_ValidatorValidate(ValidCxt, ASTCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }
    /* Instantiate the WASM module into the store context. */
    Res = WasmEdge_ExecutorInstantiate(ExecCxt, StoreCxt, ASTCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }
    /* Invoke the function which is exported with the function name "fib". */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
    WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(18) };
    WasmEdge_Value Returns[1];
    Res = WasmEdge_ExecutorInvoke(ExecCxt, StoreCxt, FuncName, Params, 1, Returns, 1);
    if (WasmEdge_ResultOK(Res)) {
      printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
    } else {
      printf("Execution phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }

    WasmEdge_ASTModuleDelete(ASTCxt);
    WasmEdge_LoaderDelete(LoadCxt);
    WasmEdge_ValidatorDelete(ValidCxt);
    WasmEdge_ExecutorDelete(ExecCxt);
    WasmEdge_StoreDelete(StoreCxt);
    ```

    After the WasmEdge `0.10.0`, developers should retrieve the `Function` instance by function name first.

    ```c
    /*
     * ...
     * Ignore the unchanged steps before validation. Please refer to the sample code above.
     */
    WasmEdge_ModuleInstanceContext *ModCxt = NULL;
    /* Instantiate the WASM module. */
    Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt1, StoreCxt, ASTCxt);
    if (!WasmEdge_ResultOK(Res)) {
      printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }
    /* Retrieve the function instance by name. */
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
    WasmEdge_FunctionInstanceContext *FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
    WasmEdge_StringDelete(FuncName);
    /* Invoke the function. */
    WasmEdge_Value Params[1] = { WasmEdge_ValueGenI32(18) };
    WasmEdge_Value Returns[1];
    Res = WasmEdge_ExecutorInvoke(ExecCxt, FuncCxt, Params, 1, Returns, 1);
    if (WasmEdge_ResultOK(Res)) {
      printf("Get the result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
    } else {
      printf("Execution phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
      return -1;
    }

    WasmEdge_ModuleInstanceDelete(ModCxt);
    WasmEdge_ASTModuleDelete(ASTCxt);
    WasmEdge_LoaderDelete(LoadCxt);
    WasmEdge_ValidatorDelete(ValidCxt);
    WasmEdge_ExecutorDelete(ExecCxt);
    WasmEdge_StoreDelete(StoreCxt);
    ```

## Instances retrievement

This example uses the [fibonacci.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wat).

Before the WasmEdge `0.9.1` versions, developers can retrieve all exported instances of named or anonymous modules from `Store`:

```c
/* Create the store context. The store context holds the instances. */
WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
/* Result. */
WasmEdge_Result Res;

/* Create the loader context. The configure context can be NULL. */
WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
/* Create the validator context. The configure context can be NULL. */
WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(NULL);
/* Create the executor context. The configure context and the statistics context can be NULL. */
WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);

/* Load the WASM file or the compiled-WASM file and convert into the AST module context. */
WasmEdge_ASTModuleContext *ASTCxt = NULL;
Res = WasmEdge_LoaderParseFromFile(LoadCxt, &ASTCxt, "fibonacci.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Validate the WASM module. */
Res = WasmEdge_ValidatorValidate(ValidCxt, ASTCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Example: register and instantiate the WASM module with the module name "module_fib". */
WasmEdge_String ModName = WasmEdge_StringCreateByCString("module_fib");
Res = WasmEdge_ExecutorRegisterModule(ExecCxt, StoreCxt, ASTCxt, ModName);
if (!WasmEdge_ResultOK(Res)) {
  printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Example: Instantiate the WASM module into the store context. */
Res = WasmEdge_ExecutorInstantiate(ExecCxt, StoreCxt, ASTCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
WasmEdge_StringDelete(ModName);

/* Now, developers can retrieve the exported instances from the store. */
/* Take the exported functions as example. This WASM exports the function "fib". */
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
WasmEdge_FunctionInstanceContext *FoundFuncCxt;
/* Find the function "fib" from the instantiated anonymous module. */
FoundFuncCxt = WasmEdge_StoreFindFunction(StoreCxt, FuncName);
/* Find the function "fib" from the registered module "module_fib". */
ModName = WasmEdge_StringCreateByCString("module_fib");
FoundFuncCxt = WasmEdge_StoreFindFunctionRegistered(StoreCxt, ModName, FuncName);
WasmEdge_StringDelete(ModName);
WasmEdge_StringDelete(FuncName);

WasmEdge_ASTModuleDelete(ASTCxt);
WasmEdge_LoaderDelete(LoadCxt);
WasmEdge_ValidatorDelete(ValidCxt);
WasmEdge_ExecutorDelete(ExecCxt);
WasmEdge_StoreDelete(StoreCxt);
```

After the WasmEdge `0.10.0`, developers can instantiate several anonymous `Module` instances, and should retrieve the exported instances from named or anonymous `Module` instances:

```c
/* Create the store context. The store context is the object to link the modules for imports and exports. */
WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
/* Result. */
WasmEdge_Result Res;

/* Create the loader context. The configure context can be NULL. */
WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(NULL);
/* Create the validator context. The configure context can be NULL. */
WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(NULL);
/* Create the executor context. The configure context and the statistics context can be NULL. */
WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(NULL, NULL);

/* Load the WASM file or the compiled-WASM file and convert into the AST module context. */
WasmEdge_ASTModuleContext *ASTCxt = NULL;
Res = WasmEdge_LoaderParseFromFile(LoadCxt, &ASTCxt, "fibonacci.wasm");
if (!WasmEdge_ResultOK(Res)) {
  printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Validate the WASM module. */
Res = WasmEdge_ValidatorValidate(ValidCxt, ASTCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Example: register and instantiate the WASM module with the module name "module_fib". */
WasmEdge_ModuleInstanceContext *NamedModCxt = NULL;
WasmEdge_String ModName = WasmEdge_StringCreateByCString("module_fib");
Res = WasmEdge_ExecutorRegister(ExecCxt, &NamedModCxt, StoreCxt, ASTCxt, ModName);
if (!WasmEdge_ResultOK(Res)) {
  printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
/* Example: Instantiate the WASM module and get the output module instance. */
WasmEdge_ModuleInstanceContext *ModCxt = NULL;
Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, StoreCxt, ASTCxt);
if (!WasmEdge_ResultOK(Res)) {
  printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
  return -1;
}
WasmEdge_StringDelete(ModName);

/* Now, developers can retrieve the exported instances from the module instaces. */
/* Take the exported functions as example. This WASM exports the function "fib". */
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("fib");
WasmEdge_FunctionInstanceContext *FoundFuncCxt;
/* Find the function "fib" from the instantiated anonymous module. */
FoundFuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FuncName);
/* Find the function "fib" from the registered module "module_fib". */
FoundFuncCxt = WasmEdge_ModuleInstanceFindFunction(NamedModCxt, FuncName);
/* Or developers can get the named module instance from the store: */
ModName = WasmEdge_StringCreateByCString("module_fib");
const WasmEdge_ModuleInstanceContext *ModCxtGot = WasmEdge_StoreFindModule(StoreCxt, ModName);
WasmEdge_StringDelete(ModName);
FoundFuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxtGot, FuncName);
WasmEdge_StringDelete(FuncName);

WasmEdge_ModuleInstanceDelete(NamedModCxt);
WasmEdge_ModuleInstanceDelete(ModCxt);
WasmEdge_ASTModuleDelete(ASTCxt);
WasmEdge_LoaderDelete(LoadCxt);
WasmEdge_ValidatorDelete(ValidCxt);
WasmEdge_ExecutorDelete(ExecCxt);
WasmEdge_StoreDelete(StoreCxt);
```

## Host functions

The difference of host functions are the replacement of `WasmEdge_ImportObjectContext`.

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

/* Create and add a function instance into the import object. */
enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
WasmEdge_FunctionTypeContext *HostFType = 
    WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
WasmEdge_FunctionInstanceContext *HostFunc =
    WasmEdge_FunctionInstanceCreate(HostFType, Add, NULL, 0);
/*
 * The third parameter is the pointer to the additional data object.
 * Developers should guarantee the life cycle of the data, and it can be
 * `NULL` if the external data is not needed.
 */
WasmEdge_FunctionTypeDelete(HostFType);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("add");
WasmEdge_ImportObjectAddFunction(ImpObj, FuncName, HostFunc);
WasmEdge_StringDelete(FuncName);

/*
 * The import objects should be deleted.
 * Developers should __NOT__ destroy the instances added into the import object contexts.
 */
WasmEdge_ImportObjectDelete(ImpObj);
```

Developers can use the `WasmEdge_ModuleInstanceContext` to upgrade to WasmEdge `0.10.0` easily.

```c
/* Host function body definition. */
WasmEdge_Result Add(void *Data, WasmEdge_MemoryInstanceContext *MemCxt,
                    const WasmEdge_Value *In, WasmEdge_Value *Out) {
  int32_t Val1 = WasmEdge_ValueGetI32(In[0]);
  int32_t Val2 = WasmEdge_ValueGetI32(In[1]);
  Out[0] = WasmEdge_ValueGenI32(Val1 + Val2);
  return WasmEdge_Result_Success;
}

/* Create a module instance. */
WasmEdge_String ExportName = WasmEdge_StringCreateByCString("module");
WasmEdge_ModuleInstanceContext *HostModCxt = WasmEdge_ModuleInstanceCreate(ExportName);
WasmEdge_StringDelete(ExportName);

/* Create and add a function instance into the module instance. */
enum WasmEdge_ValType ParamList[2] = { WasmEdge_ValType_I32, WasmEdge_ValType_I32 };
enum WasmEdge_ValType ReturnList[1] = { WasmEdge_ValType_I32 };
WasmEdge_FunctionTypeContext *HostFType = 
    WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
WasmEdge_FunctionInstanceContext *HostFunc =
    WasmEdge_FunctionInstanceCreate(HostFType, Add, NULL, 0);
/*
 * The third parameter is the pointer to the additional data object.
 * Developers should guarantee the life cycle of the data, and it can be
 * `NULL` if the external data is not needed.
 */
WasmEdge_FunctionTypeDelete(HostFType);
WasmEdge_String FuncName = WasmEdge_StringCreateByCString("add");
WasmEdge_ModuleInstanceAddFunction(HostModCxt, FuncName, HostFunc);
WasmEdge_StringDelete(FuncName);

/*
 * The module instance should be deleted.
 * Developers should __NOT__ destroy the instances added into the module instance contexts.
 */
WasmEdge_ModuleInstanceDelete(HostModCxt);
```
