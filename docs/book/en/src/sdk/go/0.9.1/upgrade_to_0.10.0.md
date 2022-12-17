# Upgrade to WasmEdge-Go v0.10.0

Due to the WasmEdge-Go API breaking changes, this document shows the guideline of programming with WasmEdge-Go API to upgrade from the `v0.9.2` to the `v0.10.0` version.

**Due to the `v0.9.1` is retracted, we use the version `v0.9.2` here.**

## Concepts

1. Merged the `ImportObject` into the `Module`.

   The `ImportObject` struct which is for the host functions is merged into `Module`.
   Developers can use the related APIs to construct host modules.

   * `wasmedge.NewImportObject()` is changed to `wasmedge.NewModule()`.
   * `(*wasmedge.ImportObject).Release()` is changed to `(*wasmedge.Module).Release()`.
   * `(*wasmedge.ImportObject).AddFunction()` is changed to `(*wasmedge.Module).AddFunction()`.
   * `(*wasmedge.ImportObject).AddTable()` is changed to `(*wasmedge.Module).AddTable()`.
   * `(*wasmedge.ImportObject).AddMemory()` is changed to `(*wasmedge.Module).AddMemory()`.
   * `(*wasmedge.ImportObject).AddGlobal()` is changed to `(*wasmedge.Module).AddGlobal()`.
   * `(*wasmedge.ImportObject).NewWasiImportObject()` is changed to `(*wasmedge.Module).NewWasiModule()`.
   * `(*wasmedge.ImportObject).NewWasmEdgeProcessImportObject()` is changed to `(*wasmedge.Module).NewWasmEdgeProcessModule()`.
   * `(*wasmedge.ImportObject).InitWASI()` is changed to `(*wasmedge.Module).InitWASI()`.
   * `(*wasmedge.ImportObject).InitWasmEdgeProcess()` is changed to `(*wasmedge.Module).InitWasmEdgeProcess()`.
   * `(*wasmedge.ImportObject).WasiGetExitCode()` is changed to `(*wasmedge.Module).WasiGetExitCode`.
   * `(*wasmedge.VM).RegisterImport()` is changed to `(*wasmedge.VM).RegisterModule()`.
   * `(*wasmedge.VM).GetImportObject()` is changed to `(*wasmedge.VM).GetImportModule()`.

   For the new host function examples, please refer to [the example below](#host-functions).

2. Used the pointer to `Function` instead of the index in the `FuncRef` value type.

   For the better performance and security, the `FuncRef` related APIs used the `*wasmedge.Function` for the parameters and returns.

   * `wasmedge.NewFuncRef()` is changed to use the `*Function` as it's argument.
   * Added `(wasmedge.FuncRef).GetRef()` to retrieve the `*Function`.

3. Supported multiple anonymous WASM module instantiation.

   In the version before `v0.9.2`, WasmEdge only supports 1 anonymous WASM module to be instantiated at one time. If developers instantiate a new WASM module, the old one will be replaced.
   After the `v0.10.0` version, developers can instantiate multiple anonymous WASM module by `Executor` and get the `Module` instance. But for the source code using the `VM` APIs, the behavior is not changed.
   For the new examples of instantiating multiple anonymous WASM modules, please refer to [the example below](#wasmedge-executor-changes).

4. Behavior changed of `Store`.

   The `Function`, `Table`, `Memory`, and `Global` instances retrievement from the `Store` is moved to the `Module` instance. The `Store` only manage the module linking when instantiation and the named module searching after the `v0.10.0` version.

   * `(*wasmedge.Store).ListFunction()` and `(*wasmedge.Store).ListFunctionRegistered()` is replaced by `(*wasmedge.Module).ListFunction()`.
   * `(*wasmedge.Store).ListTable()` and `(*wasmedge.Store).ListTableRegistered()` is replaced by `(*wasmedge.Module).ListTable()`.
   * `(*wasmedge.Store).ListMemory()` and `(*wasmedge.Store).ListMemoryRegistered()` is replaced by `(*wasmedge.Module).ListMemory()`.
   * `(*wasmedge.Store).ListGlobal()` and `(*wasmedge.Store).ListGlobalRegistered()` is replaced by `(*wasmedge.Module).ListGlobal()`.
   * `(*wasmedge.Store).FindFunction()` and `(*wasmedge.Store).FindFunctionRegistered()` is replaced by `(*wasmedge.Module).FindFunction()`.
   * `(*wasmedge.Store).FindTable()` and `(*wasmedge.Store).FindTableRegistered()` is replaced by `(*wasmedge.Module).FindTable()`.
   * `(*wasmedge.Store).FindMemory()` and `(*wasmedge.Store).FindMemoryRegistered()` is replaced by `(*wasmedge.Module).FindMemory()`.
   * `(*wasmedge.Store).FindGlobal()` and `(*wasmedge.Store).FindGlobalRegistered()` is replaced by `(*wasmedge.Module).FindGlobal()`.

   For the new examples of retrieving instances, please refer to [the example below](#instances-retrievement).

5. The `Module`-based resource management.

   Except the creation of `Module` instance for the host functions, the `Executor` will output a `Module` instance after instantiation. No matter the anonymous or named modules, developers have the responsibility to destroy them by `(*wasmedge.Module).Release()` API.
   The `Store` will link to the named `Module` instance after registering. After the destroyment of a `Module` instance, the `Store` will unlink to that automatically; after the destroyment of the `Store`, the all `Module` instances the `Store` linked to will unlink to that `Store` automatically.

## WasmEdge-Go VM changes

The `VM` APIs are basically not changed, except the `ImportObject` related APIs.

The following is the example of WASI initialization in WasmEdge-Go `v0.9.2`:

```go
conf := wasmedge.NewConfigure(wasmedge.WASI)
vm := wasmedge.NewVMWithConfig(conf)

// The following API can retrieve the pre-registration import objects from the VM object.
// This API will return `nil` if the corresponding pre-registration is not set into the configuration.
wasiobj := vm.GetImportObject(wasmedge.WASI)
// Initialize the WASI.
wasiobj.InitWasi(
  os.Args[1:],     // The args
  os.Environ(),    // The envs
  []string{".:."}, // The mapping preopens
)

// ...

vm.Release()
conf.Release()
```

Developers can change to use the WasmEdge-Go `v0.10.0` as follows, with only replacing the `ImportObject` into `Module`:

```go
conf := wasmedge.NewConfigure(wasmedge.WASI)
vm := wasmedge.NewVMWithConfig(conf)

// The following API can retrieve the pre-registration module instances from the VM object.
// This API will return `nil` if the corresponding pre-registration is not set into the configuration.
wasiobj := vm.GetImportModule(wasmedge.WASI)
// Initialize the WASI.
wasiobj.InitWasi(
  os.Args[1:],     // The args
  os.Environ(),    // The envs
  []string{".:."}, // The mapping preopens
)

// ...

vm.Release()
conf.Release()
```

The `VM` provides a new API for getting the current instantiated anonymous `Module` instance.
For example, if developer want to get the exported `Global` instance:

```go
// Assume that a WASM module is instantiated in `vm`, and exports the "global_i32".
store := vm.GetStore()

globinst := store.FindGlobal("global_i32")
```

After the WasmEdge-Go `v0.10.0`, developers can use the `(*wasmedge.VM).GetActiveModule()` to get the module instance:

```go
// Assume that a WASM module is instantiated in `vm`, and exports the "global_i32".
mod := vm.GetActiveModule()

// The example of retrieving the global instance.
globinst := mod.FindGlobal("global_i32")
```

## WasmEdge Executor changes

`Executor` helps to instantiate a WASM module, register a WASM module into `Store` with module name, register the host modules with host functions, or invoke functions.

1. WASM module instantiation

    In WasmEdge-Go `v0.9.2` version, developers can instantiate a WASM module by the `Executor` API:

    ```go
    var ast *wasmedge.AST
    // Assume that `ast` is a loaded WASM from file or buffer and has passed the validation.
    // Assume that `executor` is a `*wasmedge.Executor`.
    // Assume that `store` is a `*wasmedge.Store`.
    err := executor.Instantiate(store, ast)
    if err != nil {
      fmt.Println("Instantiation FAILED:", err.Error())
    }
    ```

    Then the WASM module is instantiated into an anonymous module instance and handled by the `Store`.
    If a new WASM module is instantiated by this API, the old instantiated module instance will be cleaned.
    After the WasmEdge-Go `v0.10.0` version, the instantiated anonymous module will be outputted and handled by caller, and not only 1 anonymous module instance can be instantiated.
    Developers have the responsibility to release the outputted module instances.

    ```go
    var ast1 *wasmedge.AST
    var ast2 *wasmedge.AST
    // Assume that `ast1` and `ast2` are loaded WASMs from different files or buffers,
    // and have both passed the validation.
    // Assume that `executor` is a `*wasmedge.Executor`.
    // Assume that `store` is a `*wasmedge.Store`.
    mod1, err1 := executor.Instantiate(store, ast1)
    if err1 != nil {
      fmt.Println("Instantiation FAILED:", err1.Error())
    }
    mod2, err2 := executor.Instantiate(store, ast2)
    if err2 != nil {
      fmt.Println("Instantiation FAILED:", err2.Error())
    }
    mod1.Release()
    mod2.Release()
    ```

2. WASM module registration with module name

    When instantiating and registering a WASM module with module name, developers can use the `(*wasmedge.Executor).RegisterModule()` API before WasmEdge-Go `v0.9.2`.

    ```go
    var ast *wasmedge.AST
    // Assume that `ast` is a loaded WASM from file or buffer and has passed the validation.
    // Assume that `executor` is a `*wasmedge.Executor`.
    // Assume that `store` is a `*wasmedge.Store`.

    // Register the WASM module into store with the export module name "mod".
    err := executor.RegisterModule(store, ast, "mod")
    if err != nil {
      fmt.Println("WASM registration FAILED:", err.Error())
    }
    ```

    The same feature is implemented in WasmEdge-Go `v0.10.0`, but in different API `(*wasmedge.Executor).Register()`:

    ```go
    var ast *wasmedge.AST
    // Assume that `ast` is a loaded WASM from file or buffer and has passed the validation.
    // Assume that `executor` is a `*wasmedge.Executor`.
    // Assume that `store` is a `*wasmedge.Store`.

    // Register the WASM module into store with the export module name "mod".
    mod, err := executor.Register(store, ast, "mod")
    if err != nil {
      fmt.Println("WASM registration FAILED:", err.Error())
    }
    mod.Release()
    ```

    Developers have the responsibility to release the outputted module instances.

3. Host module registration

    In WasmEdge-Go `v0.9.2`, developers can create an `ImportObject` and register into `Store`.

    ```go
    // Create the import object with the export module name.
    impobj := wasmedge.NewImportObject("module")

    // ...
    // Add the host functions, tables, memories, and globals into the import object.

    // The import object has already contained the export module name.
    err := executor.RegisterImport(store, impobj)
    if err != nil {
      fmt.Println("Import object registration FAILED:", err.Error())
    }
    ```

    After WasmEdge-Go `v0.10.0`, developers should use the `Module` instance instead:

    ```go
    // Create the module instance with the export module name.
    impmod := wasmedge.NewModule("module")

    // ...
    // Add the host functions, tables, memories, and globals into the module instance.

    // The module instance has already contained the export module name.
    err := executor.RegisterImport(store, impmod)
    if err != nil {
      fmt.Println("Module instance registration FAILED:", err.Error())
    }
    ```

    Developers have the responsibility to release the created module instances.

4. WASM function invocation

    This example uses the [fibonacci.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wat).
    In WasmEdge-Go `v0.9.2` version, developers can invoke a WASM function with the export function name:

    ```go
    // Create the store object. The store object holds the instances.
    store := wasmedge.NewStore()
    // Error.
    var err error
    // AST object.
    var ast *wasmedge.AST
    // Return values.
    var res []interface{}

    // Create the loader object.
    loader := wasmedge.NewLoader()
    // Create the validator object.
    validator := wasmedge.NewValidator()
    // Create the executor object.
    executor := wasmedge.NewExecutor()

    // Load the WASM file or the compiled-WASM file and convert into the AST object.
    ast, err = loader.LoadFile("fibonacci.wasm")
    if err != nil {
      fmt.Println("Load WASM from file FAILED:", err.Error())
      return
    }
    // Validate the WASM module.
    err = validator.Validate(ast)
    if err != nil {
      fmt.Println("Validation FAILED:", err.Error())
      return
    }
    // Instantiate the WASM module into the Store object.
    err = executor.Instantiate(store, ast)
    if err != nil {
      fmt.Println("Instantiation FAILED:", err.Error())
      return
    }
    // Invoke the function which is exported with the function name "fib".
    res, err = executor.Invoke(store, "fib", int32(30))
    if err == nil {
      fmt.Println("Get fibonacci[30]:", res[0].(int32))
    } else {
      fmt.Println("Run failed:", err.Error())
    }

    ast.Release()
    loader.Release()
    validator.Release()
    executor.Release()
    store.Release()
    ```

    After the WasmEdge-Go `v0.10.0`, developers should retrieve the `Function` instance by function name first.

    ```go
    // ...
    // Ignore the unchanged steps before validation. Please refer to the sample code above.

    var mod *wasmedge.Module
    // Instantiate the WASM module and get the output module instance.
    mod, err = executor.Instantiate(store, ast)
    if err != nil {
      fmt.Println("Instantiation FAILED:", err.Error())
      return
    }
    // Retrieve the function instance by name.
    funcinst := mod.FindFunction("fib")
    if funcinst == nil {
      fmt.Println("Run FAILED: Function name `fib` not found")
      return
    }
    res, err = executor.Invoke(store, funcinst, int32(30))
    if err == nil {
      fmt.Println("Get fibonacci[30]:", res[0].(int32))
    } else {
      fmt.Println("Run FAILED:", err.Error())
    }

    ast.Release()
    mod.Release()
    loader.Release()
    validator.Release()
    executor.Release()
    store.Release()
    ```

## Instances retrievement

This example uses the [fibonacci.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wasm), and the corresponding WAT file is at [fibonacci.wat](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/fibonacci.wat).

Before the WasmEdge-Go `v0.9.2` versions, developers can retrieve all exported instances of named or anonymous modules from `Store`:

```go
// Create the store object. The store object holds the instances.
store := wasmedge.NewStore()
// Error.
var err error
// AST object.
var ast *wasmedge.AST

// Create the loader object.
loader := wasmedge.NewLoader()
// Create the validator object.
validator := wasmedge.NewValidator()
// Create the executor object.
executor := wasmedge.NewExecutor()

// Load the WASM file or the compiled-WASM file and convert into the AST object.
ast, err = loader.LoadFile("fibonacci.wasm")
if err != nil {
  fmt.Println("Load WASM from file FAILED:", err.Error())
  return
}
// Validate the WASM module.
err = validator.Validate(ast)
if err != nil {
  fmt.Println("Validation FAILED:", err.Error())
  return
}
// Example: register and instantiate the WASM module with the module name "module_fib".
err = executor.RegisterModule(store, ast, "module_fib")
if err != nil {
  fmt.Println("Instantiation FAILED:", err.Error())
  return
}
// Example: Instantiate the WASM module into the Store object.
err = executor.Instantiate(store, ast)
if err != nil {
  fmt.Println("Instantiation FAILED:", err.Error())
  return
}

// Now, developers can retrieve the exported instances from the store.
// Take the exported functions as example. This WASM exports the function "fib".
// Find the function "fib" from the instantiated anonymous module.
func1 := store.FindFunction("fib")
// Find the function "fib" from the registered module "module_fib".
func2 := store.FindFunctionRegistered("module_fib", "fib")

ast.Release()
store.Release()
loader.Release()
validator.Release()
executor.Release()
```

After the WasmEdge-Go `v0.10.0`, developers can instantiate several anonymous `Module` instances, and should retrieve the exported instances from named or anonymous `Module` instances:

```go
// Create the store object. The store is the object to link the modules for imports and exports.
store := wasmedge.NewStore()
// Error.
var err error
// AST object.
var ast *wasmedge.AST
// Module instances.
var namedmod *wasmedge.Module
var anonymousmod *wasmedge.Module

// Create the loader object.
loader := wasmedge.NewLoader()
// Create the validator object.
validator := wasmedge.NewValidator()
// Create the executor object.
executor := wasmedge.NewExecutor()

// Load the WASM file or the compiled-WASM file and convert into the AST object.
ast, err = loader.LoadFile("fibonacci.wasm")
if err != nil {
  fmt.Println("Load WASM from file FAILED:", err.Error())
  return
}
// Validate the WASM module.
err = validator.Validate(ast)
if err != nil {
  fmt.Println("Validation FAILED:", err.Error())
  return
}
// Example: register and instantiate the WASM module with the module name "module_fib".
namedmod, err = executor.Register(store, ast, "module_fib")
if err != nil {
  fmt.Println("Instantiation FAILED:", err.Error())
  return
}
// Example: Instantiate the WASM module and get the output module instance.
anonymousmod, err = executor.Instantiate(store, ast)
if err != nil {
  fmt.Println("Instantiation FAILED:", err.Error())
  return
}

// Now, developers can retrieve the exported instances from the module instances.
// Take the exported functions as example. This WASM exports the function "fib".
// Find the function "fib" from the instantiated anonymous module.
func1 := anonymousmod.FindFunction("fib")
// Find the function "fib" from the registered module "module_fib".
func2 := namedmod.FindFunction("fib")
// Or developers can get the named module instance from the store:
gotmod := store.FindModule("module_fib")
func3 := gotmod.FindFunction("fib")

namedmod.Release()
anonymousmod.Release()
ast.Release()
store.Release()
loader.Release()
validator.Release()
executor.Release()
```

## Host functions

The difference of host functions are the replacement of `ImportObject` struct.

```go
// Host function body definition.
func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // add: i32, i32 -> i32
  res := params[0].(int32) + params[1].(int32)

  // Set the returns
  returns := make([]interface{}, 1)
  returns[0] = res

  // Return
  return returns, wasmedge.Result_Success
}

// ...

// Create an import object with the module name "module".
impobj := wasmedge.NewImportObject("module")

// Create and add a function instance into the import object with export name "add".
functype := wasmedge.NewFunctionType(
  []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
  []wasmedge.ValType{wasmedge.ValType_I32},
)
hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
// The third parameter is the pointer to the additional data object.
// Developers should guarantee the life cycle of the data, and it can be `nil`
// if the external data is not needed.
functype.Release()
impobj.AddFunction("add", hostfunc)

// The import object should be released.
// Developers should __NOT__ release the instances added into the import objects.
impobj.Release()
```

Developers can use the `Module` struct to upgrade to WasmEdge `v0.10.0` easily.

```go
// Host function body definition.
func host_add(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // add: i32, i32 -> i32
  res := params[0].(int32) + params[1].(int32)

  // Set the returns
  returns := make([]interface{}, 1)
  returns[0] = res

  // Return
  return returns, wasmedge.Result_Success
}

// ...

// Create a module instance with the module name "module".
mod := wasmedge.NewModule("module")

// Create and add a function instance into the module instance with export name "add".
functype := wasmedge.NewFunctionType(
  []wasmedge.ValType{wasmedge.ValType_I32, wasmedge.ValType_I32},
  []wasmedge.ValType{wasmedge.ValType_I32},
)
hostfunc := wasmedge.NewFunction(functype, host_add, nil, 0)
// The third parameter is the pointer to the additional data object.
// Developers should guarantee the life cycle of the data, and it can be `nil`
// if the external data is not needed.
functype.Release()
mod.AddFunction("add", hostfunc)

// The module instances should be released.
// Developers should __NOT__ release the instances added into the module instance objects.
mod.Release()
```
