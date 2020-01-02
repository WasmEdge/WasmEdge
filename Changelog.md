### 0.3.1 (2020-01-02)

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
