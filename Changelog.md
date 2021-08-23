### 0.8.2 (unrelease)

Features:

* WASI:
  * Supported WASI on macOS(Intel & M1).
  * Supported WASI on Windows 10.
  * Supported WASI Socket functions on Linux.
* C API:
  * Added the static library `libwasmedge_c.a`.
  * Added the `ErrCode` to C declarations.
  * Added the API about converting `WasmEdge_String` to C string.
  * Added the API to get data pointer from the `WasmEdge_MemoryInstanceContext`.
* AOT:
  * Added `--generic-binary` to generate generic binaries and disable using host features.
* Multi platforms:
  * Enabled Ubuntu 20.04 x86\_64 build.
  * Enabled Ubuntu 21.04 x86\_64 build.
  * Enabled manylinux2014 aarch64 build.
  * Enabled ubuntu arm32 build.
* Rust supports:
  * Added the `wasmedge-sys` and `wasmedge-rs` crates.
  * Added the wrapper types to rust.
* Removed binfmt support.

Fixed issues:

* Ensured every platform defines is defined.
* Disabled blake3 AVX512 support on old platforms.
* Avoided vector ternary operator in AOT, which is unspoorted by clang on mac.
* The preopen should be `--dir guest_path:host_path`.
* Fixed usused variables error in API libraries when AOT build is disabled.
* Fixed the signature error of `wasi_snapshot_preview1::path_read_link`.
  * Fixed the signature error with the lost read size output.
  * Added the `Out` comments for parameters with receiving outputs.

Misc:

* Changed all CMake global properties to target specified properties.
  * Added namespace to all cmake options.
* Installed `dpkg-dev` in docker images to enable `dpkg-shlibdeps` when creating the deb release.

Refactor:

* Refactored the WASI VFS architecture.
* Replaced the instances when registering host instances with existing names.

Documentations:

* Added the [Quick Start Javascript](https://github.com/WasmEdge/WasmEdge/blob/master/docs/run_javascript.md) document.
* Added the [Use Cases](https://github.com/WasmEdge/WasmEdge/blob/master/docs/use_cases.md) document.
* Fixed the wrong `printf` type in the C API document.

Tests:

* Added wasi-test for testing basic WASI interface
* Added C API unit tests.
  * Added the `WasmEdge_String` copy tests.
  * Added the `WasmEdge_MemoryInstanceContext` get data pointer tests.
* Removed unnecessary Wagon and Ethereum tests.

### 0.8.1 (2021-06-18)

Features:

* Exported new functions in C API to import the `wasmedge_process` module.
  * `WasmEdge_ImportObjectCreateWasmEdgeProcess()` can create and initialize the `wasmedge_process` import object.
  * `WasmEdge_ImportObjectInitWasmEdgeProcess()` can initialize the given `wasmedge_process` import object.
* Exported new AOT compiler configuration setting C APIs.
  * Users can set the options about AOT optimization level, dump IR, and instruction counting and cost measuring in execution after compilation to the AOT compiler through C APIs.
* Updated error codes according to the [newest test suite of WebAssembly](https://github.com/WebAssembly/spec/tree/master/test/core).
  * Applied the correct error message when trapping in the loading phase.
* Implemented the UTF-8 decoding in file manager.
* Implemented the basic name section parsing in custom sections.
* Added memory-mapped file helper, `MMap` for Linux.
  * Used `mmap` with `MAP_NORESERVE` for overcommited allocation.
  * Used `MMap` for file loading.
  * Merged `FileMgr` variants into one class.

Fixed issues:

* Applied the UTF-8 decoding.
  * Check the UTF-8 validation in custom sections, export sections, and import sections.
* Detected the redundant sections in modules.
  * Fixed this issue hence the sections rather than the custom section should be unique.
* Corrected the logging of data offset in the file while trap occurred in the loading phase.
  * Updated to the correct offset according to the refactored file manager.

Refactor:

* Updated manylinux\* dockerfiles.
  * Upgraded gcc to `11.1.0`.
  * Upgraded llvm to `11.1.0`.
  * Upgraded boost to `1.76`.
  * Moved environment variables to Dockerfile.
  * Used helper scripts to build.
* Moved the options of the AOT compiler into the `Configure` class.
* Refactor the file manager for supporting the `Unexpected end` loading malformed test cases.
  * Added the `setSectionSize` function to specify the reading boundary before the end of the file.
* Adjusted build scripts.
  * Set job pools for ninja generator.
  * Checked for newer compilers in `std::filesystem`.
  * Adjusted library dependency.

Documentations:

* Updated the [WasmEdge ecosystem](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ecosystem.md) document.
  * Renamed the `SSVM` related projects into `WasmEdge`.

Tools:

* Updated the `wasmedgec` AOT compiler tool for API changes of the `Configure`.

Tests:

* Turn on the `assert_malformed` tests for WASM binary in spec tests.
  * Apply the interpreter tests.
  * Apply the AOT tests.
  * Apply the API tests.
* Updated the API unit tests for the new `Configure` APIs.
* Updated the AST and loader unit tests.
  * Added test cases of file manager to raise the coverage.
  * Added test cases of every AST node to raise the coverage.

### 0.8.0 (2021-05-13)

Breaking changes:

* Renamed this project to `WasmEdge` (formerly `ssvm`).
  * The tool `wasmedge` is the WebAssembly runtime (formerly `ssvm`).
  * The tool `wasmedgec` is the WebAssembly AOT compiler (formerly `ssvmc`).
* Renamed the CMake options.
  * Option `BUILD_AOT_RUNTIME` (formerly `SSVM_DISABLE_AOT_RUNTIME` and `OFF` by default), which is `ON` by default, is for enabling the compilation of the ahead-of-Time compiler.
* Turned on the `reference-types` and `bulk-memory-operations` proposals by default in tools.
  * Users can use the `disable-bulk-memory` to disable the `bulk-memory-operations` proposal in `wasmedge` and `wasmedgec`.
  * Users can use the `disable-reference-types` to disable the `reference-types` proposal in `wasmedge` and `wasmedgec`.

Features:

* Added `WasmEdge` C API and shared library.
  * Developers can include the `wasmedge.h` and link the `libwasmedge_c.so` for compiling and running `WASM`.
  * Add CMake option `BUILD_SHARED_LIB` to enable compiling the shared library (`ON` by default).
  * The APIs about the ahead-of-time compiler will always return failed if the CMake option `BUILD_AOT_RUNTIME` is set as `OFF`.
* Added `common/version.h`: define the package version from `cmake`.
* Updated `Configure`.
  * Turned on the `reference-types` and `bulk-memory-operations` proposals by default.
  * Supports memory page limitation for limiting the largest available pages in memory instances.
* Added a function in `Log` to enable the debug logging level.
* Added global options with subcommands into `PO`.
* Added an API into `StoreManager` to list the registered module names.
* Added an API into `TableInstance` to grow table with `ref.null`.
* Updated `SIMD` implementation with the newest [SIMD proposal](https://github.com/webassembly/simd).
* Supported `AOT` compile cache.
  * Added `blake3` hash calculator to calculate hash for caching files.
* Added an API into `VM` for loading `WASM` module from `AST::Module`.

Fixed issues:

* Adjusted and fixed cmake issues.
  * Used `CMAKE_CURRENT_SOURCE_DIR` in this project for supporting to be as a submodule.
  * Assigned a default version number (`0.0.0-unreleased`) when getting the version from git describe failed.
  * Fixed `boost` include variable names.
* Fixed `WASI` `poll_oneoff`.
  * Allow `SIGINT` and `SIGTERM` while waiting for the file descriptor and check `SIGTERM` after `epoll`.
* Rearranged variables for CPU feature detection in `AOT` compiler.
* Fixed `Validator` errors.
  * Fixed the error in `br_table` for pushing wrong types into validation stack.
  * Fixed the error in `global_set` for iterating illegal indices.
* Fixed `Interpreter` errors.
  * Fixed the failed case that not returned the errors except `ErrCode::ExecutionFailed` when invoking the host functions.
  * Not to return success when the `ErrCode::Terminated` occurs.
* Fixed the unmapping size in the destructor of `MemoryInstance`.

Refactor:

* Merged the `CostTable` class into `Statistics`.
  * Simplified the API for getting and setting cost table.
  * Initialized the costs for every instruction as `1` by default.
* Merged the `Proposal` and `HostRegistration` configurations into `Configure`.
  * Adjusted the `Proposal` order.
* Applied the copy of `Configure` in `Loader`, `Validator`, `Interpreter`, and `VM` instead of passing by reference.
* Refactored the functions in the `StoreManager`.
  * Updated the templates of functions to register instances.
  * Forwarded the parameters to reduce moving.
* Refactored and used the `std::variant` to save space in `FunctionInstance`.
* Applied function parameter type checking when invoking a wasm function in `Interpreter`.
* Set the module instantiation as the anonymous active module in `Interpreter`.
* Added the `const` quantifier in `get` and `load` data functions of `MemoryInstance`.

Documentations:

* Added [release process](https://github.com/WasmEdge/WasmEdge/blob/master/ReleaseProcess.md) document.
* Added [contributing](https://github.com/WasmEdge/WasmEdge/blob/master/docs/CONTRIBUTING.md) document.
* Added [code of conduct](https://github.com/WasmEdge/WasmEdge/blob/master/docs/code_of_conduct.md) document.
* Added [roadmap](https://github.com/WasmEdge/WasmEdge/blob/master/docs/roadmap.md) document.
* Updated [external references](https://github.com/WasmEdge/WasmEdge/blob/master/docs/externref.md) document for the VM API changes.
* Updated the [WasmEdge ecosystem](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ecosystem.md) document.
* Added scripts to generate witx documents.
  * Cherry-pick `wasi_ephemeral_sock` APIs from `wasi_snapshot_preview1`.

Tools:

* `wasmedge`: WebAssembly runtime (formerly `ssvm`)
  * Turned on the `bulk-memory-operations` and `reference-types` proposals by default.
    * Users can use the `disable-bulk-memory` to disable the `bulk-memory-operations` proposal.
    * Users can use the `disable-reference-types` to disable the `reference-types` proposal.
  * Updated for the `vm` API changes.
  * Return the exit code in command mode in forced terminated occurs in `WASI`.
* `wasmedgec`: WebAssembly AOT compiler (formerly `ssvmc`)
  * Turned on the `bulk-memory-operations` and `reference-types` proposals by default.
    * Users can use the `disable-bulk-memory` to disable the `bulk-memory-operations` proposal when compiling.
    * Users can use the `disable-reference-types` to disable the `reference-types` proposal when compiling.

Tests:

* Added AOT cache tests.
* Added memory page size limit tests.
* Updated the WASM spec tests.
  * Updated [WasmEdge-unittest](https://github.com/second-state/WasmEdge-unittest) and check out the newest test suites.
    * Updated the `SIMD` test data.
    * For the `WasmEdge 0.8.0`, we use the `wasm-dev-0.8.0` tag for the core tests and the `SIMD` proposal tests.
  * Adjusted the code architecture for core testing.
    * Combined the duplicated functions into the `SpecTest` class.
    * Split out the `spectest` host function definitions for importing repeatedly.
* Added `WasmEdge` C API tests.
  * Added unit tests for APIs in the `WasmEdge` shared library.
  * Applied WASM core tests for the `WasmEdge` shared library in both using `Interpreter` APIs and `VM` APIs.

### 0.7.3 (2021-01-22)

Features:

* Updated the `easylogging++` to v9.97.0.
  * Disabled the file logging.
* Initial supported the `WASI` host functions for old system (CentOS 6).
  * Updated the `WASI` subscription insterface.
  * Used `pipe` for old `GLIBC`.
* Added supporting of subcommand in `PO`.
* Provided options to toggle white lists of `ssvm_process` in `ssvm` tool.
  * `--allow-command COMMAND` to add a command into white list in `ssvm_process` host functions.
  * `--allow-command-all` to allow all commands in `ssvm_process` host functions.
* Added the documentation of [SSVM ecosystem](https://github.com/second-state/SSVM/blob/master/docs/ecosystem.md).

Fixed issues:

* Fixed the loading issues in `file manager`.
  * Refined performance and added error handling in `readBytes`.
  * Fixed `LEB128` and `ULEB128` decoding and error codes.
* Fixed security issues of executing commands in `ssvm_process` host functions.
  * Managed a white list for command execution.

Refactor:

* Used vector of instance instead of `std::unique_ptr` in AST nodes.
* Merged all instruction node classes.
  * Added `OpCode::Else` instruction.
  * Serialized the instruction sequences. Move out the block body of `If`, `Block`, and `Loop` instructions.
* Applied the proposal configuration checking in the loader phase.
  * Moved the `OpCode` and value type validation of proposal configuration checking to loader phase.
  * Fixed the logging message.
  * Added helper functions to clean codes of logging.
* Refactored the validator for instruction serialization.
  * Removed the duplicated proposal configuration checking done at the loader phase.
  * Serialized the instruction iterating when validating.
* Refactored the `Label` in `stack manager`.
  * `Label` will record the `from` instruction iterator that causes entering this label.
  * Removed the `next` instruction getter in `stack manager`.
* Refactored the instruction iterating mechanism in `interpreter`.
  * Used the `program counter` to iterate and execute the instructions.
  * Merged all switch cases of `OpCode`.
* Moved out `AOT` related proxy codes and helper functions in `interpreter` to dependent files.

Tools:

* Added `binfmt` supporting for `WASM` interpreter.
  * Please use the tool `tools/ssvm/ssvm-static` with the same arguments as `ssvm`.
* Provided `manylinux*` support for legacy operatoring systems
  * `manylinux1` is based on CentOS 5.9
  * `manylinux2010` is based on CentOS 6
  * `manylinux2014` is based on CentOS 7

Tests:

* Updated file manager tests for `LEB128` and `ULEB128` decoding.
* Updated AST tests for refactored AST nodes.
* Updated instruction tests for refactored instruction nodes.
* Added `PO` tests.
* Added `ssvm_process` tests.

### 0.7.2 (2020-12-17)

Features:

* Added a cmake option to toggle the compilation of `ssvm` and `ssvmr` tools.
  * This option is `ON` in default.
  * `cmake -DBUILD_TOOLS=Off` to disable the compilation of `tools/ssvm` folder when building.
* Applied the [Fixed-width SIMD](https://github.com/webassembly/simd) proposal.
  * Please refer to the [SIMD document](https://github.com/second-state/SSVM/blob/master/docs/simd.md) for more details.
* Provided options to toggle proposals for the compiler and runtime.
  * `--enable-bulk-memory` to enable bulk-memory operations proposal.
  * `--enable-reference-types` to enable reference types proposal.
  * `--enable-simd` to enable SIMD proposal.
  * `--enable-all` to enable all supported proposals.
* Supported `roundeven` intrinsic in LLVM 11.

Fixed issues:

* Used `std::filesystem::path` for all paths.
* Interpreter
  * Fixed `call_indirect` table index checking in the validation phase.
  * Removed redundant `reinterpret_cast` in interpreter.
* AOT compiler
  * Forced unalignment in load and store instructions in AOT.
  * Not to report error in `terminated` case.
* WASI
  * Updated size of `linkcount` to `u64`.

Refactor:

* Added `uint128_t` into `SSVM::ValVariant`.
  * Added number type `v128`.
* Added `SSVM::RefVariant` for 64bit-width reference variant.
* Refactor AOT for better performance.
  * Added code attribute in AOT to speed up normal execution.
  * Rewrote element-wise boolean operators.
  * Used vector type in stack and function for better code generation.
  * Rewrite `trunc` instructions for readability.

Tools:

* Deprecated `ssvmr` tool, since the functionalities are the same as `ssvm` tool.
  * Please use the tool `tools/ssvm/ssvm` with the same arguments.
* Combined the tools folder. All tools in `tools/ssvm-aot` are moved into `tools/ssvm` now.

Tests:

* Added Wasi test cases.
  * Added test cases for `args` functions.
  * Added test cases for `environ` functions.
  * Added test cases for `clock` functions.
  * Added test cases for `proc_exit` and `random_get`.
* Updated test suites and categorized them into proposals.
  * Added SIMD proposal test suite.
  * [Official test suite](https://github.com/WebAssembly/testsuite)
  * [SSVM unit test in proposals](https://github.com/second-state/ssvm-unittest/tree/wasm-core)

### 0.7.1 (2020-11-06)

Features:

* Applied the [reference types and bulk memory operations](https://webassembly.github.io/reference-types/core/) proposal for AOT.
* Support LLVM 11.

Refactor:

* Refactor symbols in AOT.
  * Removed the symbols in instances.
  * Added instrinsics table for dynamic linking when running a compiled wasm.
* Merged the program counter into `stack manager`.
* Added back the `OpCode::End` instruction.
* Refactored the validator workflow of checking expressions.
* Used `std::bitset` for VM configuration.
* Used `std::array` for cost table storage.
* Conbined `include/support` into `include/common`.
  * Merged `support/castng.h` into `common/types.h`.
  * Merged `Measurement` into `Statistics`.
  * Renamed `support/time.h` into `common/timer.h`. Used standard steady clock instead.
  * Renamed `common/ast.h` into `common/astdef.h`.
  * Moved `common/ast/` to `ast/`.
  * Removed the `SSVM::Support` namespace.

Tests:

* Applied new test suite of the reference types and bulk memory operation proposal for AOT.


### 0.7.0 (2020-10-16)

Features:

* Applied the [reference types and bulk memory operations](https://webassembly.github.io/reference-types/core/) proposal.
  * Added the definition of reference types.
    * Added helper functions for function index to `funcref` conversions.
    * Added helper functions for reference to `externref` conversions.
  * Added the following new instructions.
    * Reference instructions:
      * ref.null
      * ref.is_null
      * ref.func
    * Table instructions:
      * table.get
      * table.set
      * table.init
      * elem.drop
      * table.copy
      * table.grow
      * table.size
      * table.fill
    * Memory instructions:
      * memory.init
      * data.drop
      * memory.copy
      * memory.fill
    * Parametric instructions:
      * select t
  * Updated implementation of the following instructions.
    * call_indirect
    * select
  * Applied the new definition of `data count section`, `data segment`, and `element segment`.
  * Applied validation for `data segment` and `element segment`.
  * Added the `data instance` and `element instance`.
  * Applied the new instantiation flow.

Refactor:

* Completed the enumeration value checking in the loading phase.
* Updated the value type definition.
  * `ValType` is updated to include `NumType` and `RefType`.
  * `NumType` is updated to include `i32`, `i64`, `f32`, and `f64`.
  * `RefType` is updated to include `funcref` and `externref`, which replaced the `ElemType`.
* Updated error codes according to the test suite for the reference types proposal.
* Extended validation context for recording `datas`, `elements`, and `refs`.
* Updated runtime structures.
  * Fixed minimum pages definition in `memory instance`.
  * Applied new definitions of `table instance`.
  * Extended `module instance` for placing `data instance` and `element instance`.
  * Extended `store` for owning `data instance` and `element instance`.
* Updated template typename aliasing in `interpreter`.

Tests:

* Applied new test suite for the proposal.
  * [Official test suite](https://github.com/WebAssembly/reference-types/tree/master/test/core)
  * [SSVM unit test for reference types](https://github.com/second-state/ssvm-unittest/tree/wasm-ref-types)
* Supported `funcref` and `externref` types parameters in core tests.
* Added `externref` tests for testing object binding and samples.
  * Please see the [document](https://github.com/second-state/SSVM/blob/master/docs/externref.md) for detail.

### 0.6.9 (2020-10-16)

Features:

* Added gas and instruction count measurement in AOT.

### 0.6.8 (2020-10-05)

Features:

* Supported loop parameters in AOT.
* Added optimization level settings in the AOT compiler.

Refactor:

* Applied page based allocation in `memory instance`, instead of preserving 4G at once.

Fixed Issues:

* Fixed error marking stdin, stdout, and stderr file descriptor as pre-opened when initializing WASI environment.
* Fixed `ssvm_process` error handling when execution commands.
  * Print error message when command not found or permission denied.
  * Fixed casting of return codes.

Tests:

* Split the core test to helper class for supporting AOT core tests in the future.

### 0.6.7 (2020-09-09)

This is a bug-fix release for the ssvm_process component.

Fixed Issues:

* Handle the large size writing to pipe in `ssvm_process`.

### 0.6.6 (2020-09-02)

Features:

* Add option for dumping LLVM IR in `ssvmc`.
* Add `SSVM_Process` configuration.
  * VM with this option will import `ssvm_process` host modules.
  * `ssvm_process` host functions are SSVM extension for executing commands.
  * This host module is to support wasm compiled from rust with [`rust_process_interface_library` crate.](https://github.com/second-state/rust_process_interface_library).
* Turn on `SSVM_Process` configuration in both `ssvmr` and `ssvm`.

Refactor:

* Apply `mprotect` memory boundary checking in `memory instance`.

Fixed Issues:

* Prevent undefined behavior on shift operations in interpreter and file manager.

### 0.6.5 (2020-08-21)

Features:

* Support WebAssembly reactor mode in both `ssvmr` and `ssvm`.

Refactor:

* Use `vector` instead of `deque` in `Validator`.

Fixed Issues:

* Fixed cost table to support 2-byte instructions.
* Resolved warnings about signed and unsigned comparing.
* Fixed printing error about hex strings in error messages.
* Corrected memory boundary logging in error messages.
* Ignore `SIGINT` when `ssvm` is forced interrupted.

Tests:

* Add ssvm-aot tests.

Tools:

* Updated `ssvm` interpreter.
  * `ssvm` provides interpreter mode of executing wasm.
  * The usage of `ssvm` is the same as `ssvmr`.
  * Added `STATIC_BUILD` mode for linking std::filesystem statically.

### 0.6.4 (2020-07-30)

This is a bug-fix release for the warnings.

Fixed Issues:

* Resolved warnings with compilation flag `-Wall`.
* Add `-Wall` flag in CMakeFile.

Refactor:

* Refactored instruction classes for supporting 2-byte instructions.
* Refined corresponding switch cases in validator, interpreter, and AOT.


### 0.6.3 (2020-07-23)

This is a bug-fix release for the wasi component.

Fixed Issues:

* Change the fd number remap mechanism from static offset to dynamic map.


### 0.6.2 (2020-07-22)

Features:

* New target support:
  * Add aarch64 target support for both ssvm-interpreter and ssvm-aot tools.
* Wasm spec 1.1 support:
  * Implement `multi-value return` proposal.
  * Implement `signed extension` and `saturated convert` instructions.
    * i32.extend8_s
    * i32.extend16_s
    * i64.extend8_s
    * i64.extend16_s
    * i64.extend32_s
    * i32.trunc_sat_f32_s
    * i32.trunc_sat_f32_u
    * i32.trunc_sat_f64_s
    * i32.trunc_sat_f64_u
    * i64.trunc_sat_f32_s
    * i64.trunc_sat_f32_u
    * i64.trunc_sat_f64_s
    * i64.trunc_sat_f64_u
* Wasm spec test suites support:
  * Add [ssvm-unittest](https://github.com/second-state/ssvm-unittest) toolkit for integrating wasm spec test suites.
  * Enable `assert_invalid` tests
* Wasi support:
  * Enable environ variables support:
    * add `--env` option for environment variables.
    * allow developers to append more environment variables from a given env list, e.g. `PATH=/usr/bin`, `SHELL=ZSH`.
  * Enable preopens support:
    * add `--dir` option for preopens directories.
    * allow developers to append more preopens directories from a given dir list, e.g. `/sandbox:/real/path`, `/sandbox2:/real/path2`.
* New Statistics API:
  * With statistics class, developers can get the following information after each execution:
    * Total execution time in `us`. (= `Wasm instruction execution time` + `Host function execution time`)
    * Wasm instruction execution time in `us`.
    * Host function execution time in `us`. A host function can be a evmc function like `evmc::storage_get`, a wasi function like `random_get`, or any customized host function.
    * Instruction count. (Total executed instructions in the previous round.)
    * Total gas cost. (Execution cost by applying ethereum-flavored wasm cost table.)
    * Instruction per second.
* Validator:
  * Support Wasm 1.1 instructions validation.
  * Support blocktype check which is used in multi-value return proposal.
* Logging system:
  * Support 2-byte instructions.

Refactor:

* Remove redundant std::move in return statements.

Fixed Issues:

* Fix std::filesystem link issue in ssvm-aot tool.
* Fix `-Wreorder` warnings in errinfo.h
* Fix several implementation errors in wasi functions.

Tools:

* CI: Update base image from Ubuntu 18.04 to Ubuntu 20.04


### 0.6.1 (2020-06-24)

Features:

* Error Logging System
  * Add information structures to print information when an error occurs.
  * Apply error logging in every phase.

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
* Function invocation
  * Add dummy frame when invoking function from VM.

### 0.6.0 (2020-06-08)

Features:

* Building System
  * Add CMake option `SSVM_DISABLE_AOT_RUNTIME` to disable building ahead of time compilation mode.
* Wasm AST
  * Add support of multiple partitions of sections in wasm module.
* AOT
  * Add SSVM-AOT tools.

Tools:

* SSVM-AOT
  * Enable to compile and run separately.
  * Enable to run compiled module and normal module with the interpreter.

Refactor:

* Internal tuple span mechanism
  * Apply C++20 `span` features in host functions.
* Internal error handling mechanism
  * Apply non-exception version of `expected`.
* Refine CMake files
  * Update file copying macro in `CMakeFile` to support recursively copying.
  * Refine include paths and dependencies in every static library.
  * Modularize static libraries to be included as submodules easier.
* Interpreter
  * Use function address in `Store` for invoking instead of the exported function name.
  * Support invocation of a host function.
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
  * Fix type transforming when function invocation and return.
* Runtime Data Structure
  * Fix the wrong table resizing when initialization in `table instance`.
* Interpreter
  * Instantiation
    * Fix instantiation steps of `data` and `element sections`.
    * Check `memory` and `table instances` boundary according to Wasm spec.
    * Not to replace data in `memory` and `table instances` until all checkings were done.
  * Engine
    * Fix wrong arity assignment in `loop` instruction.
    * Fix wrong answer issue in `trunc` and `clz` instructions.
    * Fix logic of `div` instruction in both integer and floating-point inputs.
    * Fix wrong handling of `NaN` operand in `min` and `max` instructions.
    * Add dummy frame before function invocation according to Wasm spec.
    * Add memory boundary checking when loading value in `memory` instructions.
* AOT
  * Fix wrong handling of the minimum operand in `mod` instructions.
  * Fix wrong handling of `NaN` operand in `min` and `max` instructions.

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
  * Fix data copying in table instance in the instantiation phase.
  * Fix label popping in stack manager.

### 0.5.0 (2020-04-01)

Features:

* Ethereum environment interface
  * Implemented all EEI functions.
  * For more details, please refer to [Ewasm functions design document](docs/evm/design_document.md)
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
  * [Ewasm functions implemented](docs/evm/design_document.md)
  * Update gas costs of Ewasm functions.

Refactor:

* Host functions:
  * Use the template to generate wasm function type of host function body.
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
* Change the arguments and return value formats.
  * Add `argument_types` and `return_types` in input JSON format.
* Expand home directory path
  * Accept ~ in the file path


### 0.3.0 (2019-12-27)

Features:

* WebAssembly Validation
  * Implement the Wasm Validation mechanism. SSVM will validate wasm modules before execution.
* Snapshot and restore execution state
  * SSVM provides restore mechanism from the previous execution state.
  * SSVM provides a snapshot mechanism to dump the current execution state.
* [JSON interface Spec](docs/ssvm-proxy/design_document.md)
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

