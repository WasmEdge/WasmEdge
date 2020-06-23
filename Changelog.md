### 0.6.1 (2020-06-24)

Features:

* Error Logging System
  * Add information structures to print information when error occurs.
  * Apply error logging in every phases.

Refactor:
* Internal tuple span mechanism
  * Apply C++20 `span` features instead of `std::vector &`.
* Internal string passing mechanism
  * Apply C++17 `std::string_view` for passing strings.
* Move enumeration definitions
  * Add string mapping of types, instructions, and AST nodes.
  * Move enumerations to SSVM top scope.
* Memory instance passing in host functions
  * Pass pointer instead of reference of memory instance to allow `nullptr`.

Fixed Issues:
* Instantiation Phase
  * Fixed boundary checking bugs when initializing data sections.
* Function invokation
  * Add dummy frame when invoking function from VM.

### 0.6.0 (2020-06-08)

Features:

* Building System
  * Add cmake option `SSVM_DISABLE_AOT_RUNTIME` to disable building ahead of time compilation mode.
* Wasm AST
  * Add support of multiple partition of sections in wasm module.
* AOT
  * Add SSVM-AOT tools.

Tools:

* SSVM-AOT
  * Enable to compile and run separatedly.
  * Enable to run compiled module and normal module with interpreter.

Refactor:

* Internal tuple span mechanism
  * Apply C++20 `span` features in host functions.
* Internal error handling mechanism
  * Apply non-exception version of `expected`.
* Refine cmake files
  * Update file copying macro in `CMakeFile` to support resursively copying.
  * Refine include paths and dependencies in every static libraries.
  * Modularize static libraries to be included as submodules easier.
* Interpreter
  * Use function address in `Store` for invoking instead of exported function name.
  * Support invokation of a host function.
* Host functions
  * Return `Expect` instead of `ErrCode` in host functions.
  * Return function return values in `Expect` class rather than in function parameter.
* New VM APIs
  * Add routine to invoke a function of registered and named module in `Store`.
  * Removed old `executor` and use `interpreter` instead.
  * Renamed `ExpVM` to `VM` and removed the old one.
  * Apply new `VM` to all tools.
* AOT
  * Integrated into new VM API and HostFunctions
  * Generate minimum machine code for `nearestint` instructions.

Fixed Issues:

* Loader
  * Add checking Wasm header and version when loading.
* Validation
  * Fix `export section` checking to support `""` function name.
  * Fix type transforming when function invokation and returning.
* Runtime Data Structure
  * Fix wrong table resizing when initialization in `table instance`.
* Interpreter
  * Instantiation
    * Fix instantiation steps of `data` and `element sections`.
    * Check `memory` and `table instances` boundary according to Wasm spec.
    * Not to replace data in `memory` and `table instances` until all checkings were done.
  * Engine
    * Fix wrong arity assignment in `loop` instruction.
    * Fix wrong answer issue in `trunc` and `clz` instructions.
    * Fix logic of `div` instruction in both integer and floating point inputs.
    * Fix wrong handling of `NaN` oprend in `min` and `max` instructions.
    * Add dummy frame before function invokation according to Wasm spec.
    * Add memory boundary checking when loading value in `memory` instructions.
* AOT
  * Fix wrong handling of minimum oprend in `mod` instructions.
  * Fix wrong handling of `NaN` oprend in `min` and `max` instructions.

Tests:

* Remove `ssvm-evmc` tests.
* (Experimental) Add unit tests for C++ `span` feature.

Deprecated:

* SSVM-Proxy is removed.
* SSVM-EVMC is removed.
  * [SSVM-EVMC](https://github.com/second-state/ssvm-evmc) is separated from this project as an independent repository.

### 0.5.1 (2020-04-09)

SSVM 0.5.1 is a bug-fix release from 0.5.0.

* Issues:
  * Set correct reset timing of the interpreter.
  * Fix data copying in table instance in the instanciation phase.
  * Fix label popping in stack manager.

### 0.5.0 (2020-04-01)

Features:

* Ethereum environment interface
  * Implemented all EEI functions.
  * For more details, please refer to [Ewasm functions design document](doc/evm/design_document.md)
* Validation
  * Completed validations for wasm sections.
  * Completed checkings in const expressions.
* Runtime Wasm module registering
  * WASM modules can be registered into `Store` for importing.
  * Host modules, which may contain host functions and `global`s, can be registered into `Store`.
* (Experimental) New VM APIs
  * New VM is refactoring from legacy’s VM and provides a rapidly running process for WASM.
  * Export `Store` for external access.
* Node.js addon
  * Integrate SSVM with Node.js Addon API.
  * [SSVM Node.js addon](https://github.com/second-state/ssvm-napi) is separated from this project as an independent repository.

Refactor:

* Code structure layout
  * Create `common` namespace for cross-component data structures and type definitions.
  * Extract AST structures from ast to `common`.
  * Extract duplicate enumerations to `common`.
  * Collects all error code classes into `common`.
* Internal error handling mechanism
  * Apply C++ p0323r9 `expected` features
  * Add several helper functions for wrapping return values with error code.
* Wasm loader
  * Simplify workflow.
  * Take a wasm input and return an `AST` object directly.
* Wasm validator
  * Simplify workflow.
  * Take an `AST` object and return the results.
  * Rename `validator/vm` to `formchecker`.
* Refine runtime data structure
  * Extract `instance`s, `host function`s, `stack manager`, and `store manager` classes to `runtime` folder.
  * Extract `frame`, `label`, and `value` entry classes into `stack manager`.
  * Delete redundant checks in `stack manager`. All of these checks are verified in the validation stage.
  * Add `ImportObj` class for handling the host modules registration.
* Interpreter
  * Create `interpreter` namespace.
  * Extract `executor` class to `interpreter`.
  * Add instantiation methods for registering host modules.
* Host functions
  * Create `host` namespace.
  * Extract `EEI`, `Wasi-core`, and `ONNC` host functions to `host`.
  * Make host functions construction in host modules.
  * Extract `host environment`s from `environment manager` to respective `host module`s.
* Refactoring from legacy VM.
  * Simplify workflow. Provide two approaches for invoking a wasm function.
    * All-in-one way: Calling `runWasmFile` can instantiate and invoke a wasm function directly.
    * Step-by-step way: Calling `loadWasm`, `validate`, `instantiate`, `execute` sequentially can make developers control the workflow manually.
  * External access APIs
    * Access `export`ed wasm functions.
    * Export `Store`.
    * Export measurement data class including instruction counter, timer, and cost meter.
  * Provide registration API for wasm modules and host modules.
  * Extract `host environment`s of `EEI` and `Wasi-core` into respective `host module`s.
  * Apply experimental VM to `ssvm-proxy` and `ssvm-evmc` tools.

Tools:

* Remove unused ssvm-evm
  * `ssvm-evm` is replaced by `ssvm-evmc`.
* (Experimental) Add sub-project `ssvm-aot`
  * `ssvm-aot` provides ahead-of-time(AOT) compilation mechanism for general wasm applications.

Tests:

* Remove redundant `ssvm-evm` tests.
* (Experimental) Add integration tests for `ssvm-aot`.
* (Experimental) Add unit tests for C++ `expected` feature.
* Move `AST` tests to the test top folder.

Fixed issues:

* Ethereum Environment Interface
  * Fix function signatures.
  * Return `fail` instead of `revert` when the execution state is `out of gas`.
  * Handle memory edge case when loading and storing from memory instance.
  * Add missing check for evmc flags.
  * Set running code to evmc environment.
* Complete import matching when instantiation in the interpreter.
* Fix lost of validation when importing `global`s.

### 0.4.0 (2020-01-17)

Features:

* Ethereum environment interface implementation
  * Add EVMC library.
  * [Ewasm functions implemented](doc/evm/design_document.md)
  * Update gas costs of Ewasm functions.

Refactor:

* Host functions:
  * Use template to generate wasm function type of host function body.
  * Move function module name and function name to host function class.

Tools:

* Sub-project EVM with evmc
  * SSVM-EVMC integrates EVMC and Ethereum Environment Interface(EEI).
  * SSVM-EVMC is a shared library for EVMC-compatible clients.

Tests:

* ERC20 contracts for SSVM-EVMC
  * Create an example VM for testing.
  * Test the following functionalities of ERC20 contracts:
    * Deploy ERC20 contract
    * Check balance
    * Check total supply
    * Transfer
    * Approve
    * Check allowance


### 0.3.2 (2020-01-09)

Fixed issues:

* Handle empty length of memory in `vm_snapshot`.
* Correct error message when execution failed in SSVM proxy mode.

### 0.3.1 (2020-01-07)

Fixed issues:

* Change the naming style of JSON format in SSVM proxy mode
  * Use snake case for the keys of JSON files instead
* Change the arguements and return values formats.
  * Add `argument_types` and `return_types` in input JSON format.
* Expand home directory path
  * Accept ~ in the file path


### 0.3.0 (2019-12-27)

Features:

* WebAssembly Validation
  * Implement Wasm Validation mechanism. SSVM will validate wasm modules before execution.
* Snapshot and restore execution state
  * SSVM provides restore mechanism from the previous execution state.
  * SSVM provides snapshot mechanism to dump the current execution state.
* [JSON interface Spec](doc/ssvm-proxy/design_document.md)
  * Initialize and set up SSVM via input JSON format.
  * Retrieve execution results via output JSON format.

Tools:

* Sub-project RPC service proxy mode
  * SSVM-PROXY is a component of [SSVMRPC service](https://github.com/second-state/SSVMRPC).
  * SSVM-PROXY can archive current execution states and serialize these data into output JSON format.
  * SSVM-PROXY can restore previous program states from input JSON format.


### 0.2.0 (2019-12-18)

Features:

* Native Cost Metering
  * SSVM provides CostTab for each instruction including Wasm, Wasi, Ewasm.
  * With this feature, users can set the cost limit for measuring the execution cost.
* Built-in performance timer
  * TimeRecord collects execution time for the performance analysis.
  * TimeRecord supports multiple timers.
  * SSVM also provides Wasi timer API for developers to customize TimeRecord.
* Multiple Virtual Machine Environment
  * Wasm mode: Support general Wasm program.
  * Wasi mode: In addition to Wasm mode, this mode contains basic Wasi functions like print.
  * QITC mode: In addition to Wasi mode, this mode is designed for ONNC runtime to execute AI models by leveraging Qualcomm Hexagon SDK.
  * Ewasm mode: In addition to Wasm mode, this mode is designed for Ethereum flavor WebAssembly.
* Start functions enhancement
  * Support start function assignment. This makes users invoke an exported function with a given function name.
  * Support start function arguments and return value. This makes users can insert arguments and retrieve result after execution.
* Simple statistics output
  * Dump total execution time and instruction per second for benchmarking.
  * Print used gas costs for Ewasm mode.
  * Print storage and return values.

Tools:

* Sub-project Qualcomm Innovate in Taiwan Challenge(a.k.a QITC) 2019
  * SSVM-QITC enables AI model execution by integrating [ONNC](https://github.com/ONNC/onnc) runtime and Qualcomm Hexagon SDK.
  * With this tool, users can run AI model inference within a WebAssembly Virtual Machine.
* Sub-project Ethereum
  * SSVM-EVM integrates the Ethereum Environment Interface(EEI) as a WebAssembly extension.
  * With this tool, users can run blockchain applications, which are compiled into Ewasm bytecodes.
* Sub-project General Wasi Support
  * SSVM tool provides basic Wasi functions support, such as print function.


### 0.1.0 (2019-11-29)

Features:

* Lexer: Support full wasm bytecode format
* AST: Be able to load a wasm module
* Instantiate: Support wasm module instantiation

Runtime:

* Support Wasi-core functions
* Support Ewasm functions

Test:

* Support ERC20 token contracts
