### 0.14.1 (2024-09-16)

Features:

* Supported LLVM 17.0.6.
* Bumpped `spdlog` to `v1.13.0`.
* Bumpped `fmt` to `11.0.2`.
* Bumpped `simdjson` to `v3.10.0`.
* Bumpped `googletest` to `1.15.2`.
* [WASI-NN] ggml backend:
  * Bump llama.cpp to b3651.
  * Static link `libggml` and `libllama`.
  * Refined the CMake to support multiple backends of WASI-NN with ggml backend.
  * Supported compute single in RPC mode.
* [WASI-NN] Added support for whisper.cpp backend.
* [WASI-NN] Added support for piper backend.
* [WASI-NN] Added support for ChatTTS backend.
* [WASI-NN] Added support for Burn.rs backend.
  * Supported `squeezenet` and `whisper` models.
* [Plugin] Supported `wasmedge_stablediffusion` plug-in.
  * Enabled CUBLAS.
  * Enabled metal support on MacOS.
* [Plugin] Moved `wasi_logging` into built-in plug-in.
  * Instead of installing `wasi_logging` plug-in shared library, developers can find and get this plug-in after calling `WasmEdge_PluginLoadWithDefaultPaths()` API.
  * In the WasmEdge CLI tools, the built-in plug-ins will automatically be loaded.
* [Proposal] Initial support for instantiation phase of component model.
  * Due to the breaking change of API, bump the plug-in `API_VERSION` to `3`.
* [Proposal] Supported WASM Relaxed-SIMD proposal.
  * Added the `WasmEdge_Proposal_RelaxSIMD` for the configuration in WasmEdge C API.
  * Users can use the `--enable-relaxed-simd` to enable the proposal in `wasmedge` and `wasmedgec` tools.

Fixed issues:

* Fixed warnings on GCC-14.
* Fixed the `fmt` related header inclusion for error logging.
* Fixed WASI test error in Windows.
* Fixed version checking in source tarball.
* Fixed version detection issue when building from source.
* Fixed the visibility of internal symbols.
* [Loader] Fixed alignment checking in loading immediates for memory instructions.
* [Runtime] Fixed allocation issue when configured the limited memory page size.
* Used `fmt::format` instead of string stream in error logging.

Tests:

* Added WASI test suites on Windows.

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.

Thank all the contributors who made this release possible!

Biswapriyo Nath, Elmira, Faidon Liambotis, Fusaaaann, Han-Wen Tsao, Jun Zhang, Kefu Chai, Lîm Tsú-thuàn, Michael Morris, PeterD1524, Shen-Ta Hsieh, Shreyas Atre, Sylveon, Yi Huang, Yi-Ying He, alabulei1, dm4, grorge, hydai, junxiangMu, vincent

If you want to build from source, please use WasmEdge-0.14.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.14.0 (2024-05-22)

Breaking changes:

* [Version]: Bump the version of the WasmEdge shared library.
  * Due to the breaking change of API, bump the `SOVERSION` to `0.1.0`.
  * Due to the breaking change of API, bump the plug-in `API_VERSION` to `3`.
* [C API]: Changes for applying Typed Function References Proposal.
  * New `WasmEdge_ValType` structure for replacing `enum WasmEdge_ValType`.
    * Merge the `enum WasmEdge_ValType` and `enum WasmEdge_RefType` into the `enum WasmEdge_TypeCode`.
  * Refactored the error code. The error code number may different from previous versions.
    * Extend the error code to 2 bytes.
  * Updated the related APIs for using `enum WasmEdge_ValType` as parameters.
    * `WasmEdge_FunctionTypeCreate()`
    * `WasmEdge_FunctionTypeGetParameters()`
    * `WasmEdge_FunctionTypeGetReturns()`
    * `WasmEdge_TableTypeCreate()`
    * `WasmEdge_TableTypeGetRefType()`
    * `WasmEdge_GlobalTypeCreate()`
    * `WasmEdge_GlobalTypeGetValType()`
  * Removed `WasmEdge_ValueGenNullRef()` API.
  * Due to non-defaultable values after this proposal, the following APIs return the result instead of void.
    * `WasmEdge_GlobalInstanceSetValue()`
  * Introduced the `WasmEdge_Bytes` structure.
    * This structure is for packaging the `uint8_t` buffers. The old `FromBuffer` related APIs will be replaced by the corresponding APIs in the future versions.
    * `WasmEdge_CompilerCompileFromBytes()` API has the same function as `WasmEdge_CompilerCompileFromBuffer()` and will replace it in the future.
    * `WasmEdge_LoaderParseFromBytes()` API has the same function as `WasmEdge_LoaderParseFromBuffer()` and will replace it in the future.
    * `WasmEdge_VMRegisterModuleFromBytes()` API has the same function as `WasmEdge_VMRegisterModuleFromBuffer()` and will replace it in the future.
    * `WasmEdge_VMRunWasmFromBytes()` API has the same function as `WasmEdge_VMRunWasmFromBuffer()` and will replace it in the future.
    * `WasmEdge_VMAsyncRunWasmFromBytes()` API has the same function as `WasmEdge_VMAsyncRunWasmFromBuffer()` and will replace it in the future.
    * `WasmEdge_VMLoadWasmFromBytes()` API has the same function as `WasmEdge_VMLoadWasmFromBuffer()` and will replace it in the future.
  * New APIs for WASM Exception-Handling proposal.
    * Added the `WasmEdge_TagTypeContext` struct.
    * Added the `WasmEdge_TagInstanceContext` struct.
    * Added the `WasmEdge_TagTypeGetFunctionType()` API for retrieving the function type from a tag type.
    * Added the `WasmEdge_ImportTypeGetTagType()` API for retrieving the tag type from an import type.
    * Added the `WasmEdge_ExportTypeGetTagType()` API for retrieving the tag type from an export type.
    * Added the `WasmEdge_ModuleInstanceFindTag()` API for finding an exported tag instance from a module instance.
    * Added the `WasmEdge_ModuleInstanceListTagLength()` and `WasmEdge_ModuleInstanceListTag()` APIs for listing the exported tag instances of a module instance.
* Refactored the `OpCode` mechanism for speeding up and supporting WASM multi-bytes instruction OpCodes.

Features:

* Bumpped `spdlog` to `v1.13.0`.
* Bumpped `simdjson` to `v3.9.1`.
* [Proposal]: Apply new propoals.
  * Supported WASM Typed Function References proposal.
    * Added the `WasmEdge_Proposal_FunctionReferences` for the configuration in WasmEdge C API.
    * Users can use the `--enable-function-reference` to enable the proposal in `wasmedge` and `wasmedgec` tools.
  * Supported WASM GC proposal (interpreter only).
    * Added the `WasmEdge_Proposal_GC` for the configuration in WasmEdge C API.
    * Users can use the `--enable-gc` to enable the proposal in `wasmedge` and `wasmedgec` tools.
  * Supported WASM Exception-Handling proposal (interpreter only).
    * Added the `WasmEdge_Proposal_ExceptionHandling` for the configuration in WasmEdge C API.
    * Users can use the `--enable-exception-handling` to enable the proposal in `wasmedge` and `wasmedgec` tools.
    * This proposal supports old deprecated `try`, `catch`, and `catch_all` instructions, and will remove them in the future version.
  * Component Model proposal (experimental, loader phase only).
    * Added the `WasmEdge_Proposal_Component` for the configuration in WasmEdge C API.
    * Users can use the `--enable-component` to enable the proposal in `wasmedge` tool.
* [JIT]: Support LLVM JIT.
* [C API]: New C API for supporting the new proposals.
  * `WasmEdge_ValType` related APIs can help developers to generate or compare value types.
    * `WasmEdge_ValTypeGenI32()` (replacing `WasmEdge_ValType_I32`)
    * `WasmEdge_ValTypeGenI64()` (replacing `WasmEdge_ValType_I64`)
    * `WasmEdge_ValTypeGenF32()` (replacing `WasmEdge_ValType_F32`)
    * `WasmEdge_ValTypeGenF64()` (replacing `WasmEdge_ValType_F64`)
    * `WasmEdge_ValTypeGenV128()` (replacing `WasmEdge_ValType_V128`)
    * `WasmEdge_ValTypeGenFuncRef()` (replacing `WasmEdge_ValType_FuncRef`)
    * `WasmEdge_ValTypeGenExternRef()` (replacing `WasmEdge_ValType_ExternRef`)
    * `WasmEdge_ValTypeIsEqual()`
    * `WasmEdge_ValTypeIsI32()`
    * `WasmEdge_ValTypeIsI64()`
    * `WasmEdge_ValTypeIsF32()`
    * `WasmEdge_ValTypeIsF64()`
    * `WasmEdge_ValTypeIsV128()`
    * `WasmEdge_ValTypeIsFuncRef()`
    * `WasmEdge_ValTypeIsExternRef()`
    * `WasmEdge_ValTypeIsRef()`
    * `WasmEdge_ValTypeIsRefNull()`
  * `WasmEdge_Bytes` related APIs can help developers to control the buffers.
    * `WasmEdge_BytesCreate()`
    * `WasmEdge_BytesWrap()`
    * `WasmEdge_BytesDelete()`
  * `WasmEdge_TableInstanceCreateWithInit()` to create a table instance with non-defaultable elements with assigning the initial value.
* [Serializer]: Supported WASM module serialization (experimental).
  * This is the API-level feature. Developers can use the `WasmEdge_LoaderSerializeASTModule()` API to serialize a loaded WASM module into bytes.
* [Tools]: Print the plug-in versions when using the `--version` option.
* [Installer]: Enabled `ggml-blas` and `rustls` plugin supporting (#3032) (#3108).
* [WASI-NN] ggml backend:
  * Bump llama.cpp to b2963.
  * Support llama.cpp options:
    * `threads`: the thread number for inference.
    * `temp`: set temperature for inference.
    * `repeat-penalty`: set repeat penalty for inference.
    * `top-p`: set top-p for inference.
    * `grammar`: set grammar syntax for inference.
    * `main-gpu`: set the main GPU for inference.
    * `tensor-split`: set the tensor split for inference.
  * Add `enable-debug-log` option to show more debug information.
  * Default enable Metal on macOS.
  * Introduce `load_by_name_with_config()` to load model with metadata.
  * Introduce single token inference by `compute_single`, `get_output_single`, and `fini_single`
  * Introduce `unload()` function to release the model.
  * Add some llama errors to WASI-NN.
    * `EndOfSequence`: returned when encounter `<EOS>` token on single token inferece.
    * `ContextFull`: returned when the context is full.
    * `PromptTooLong`: returned when the input size is too large.
    * `ModelNotFound`: returned when the model is not found.
  * Support Llava and Gemma inference.
    * Add `mmproj` option to set the projection model.
    * Add `image` option to set the image.
  * Improve logging mechanism.
  * Show the version of `llama.cpp` in the metadata.
  * Support Phi-3-Mini model.
  * Support embedding generation.
  * Support Windows build.
* [Plugin] Initial support for `wasmedge_ffmpeg` plug-in.
* [Plugin] Updated `wasi-logging` plug-in for supporting logging into file.

Fixed issues:

* Fixed some API document in the API header.
* [Executor]: Minor fixes.
  * Fixed integer overflow on `memGrow` boundary check.
  * Refined the slice copy in table instances.
  * Cleaned the unused bits of WASM return values to avoid security issues.
* [WASI]: Minor fixes.
  * Fixed the function signature matching for WASI imports when backwarding supporting older version. (#3073)
  * Fixed large timestamp causing overflow (#3106).
  * Handle HUP only events.
  * Checking same file descriptor for `fd_renumber` (#3040).
  * Fixed `path_unlink_file` for trailing slash path.
  * Fixed `path_readlink` for not following symbolic link issue.
  * Fixed `path_open` for checking `O_TRUNC` rights.
  * Fixed `path_open` for removing path relative rights on file.
  * Fixed `fd_allocate` return error value.
  * Checking `path_symlink` for creating a symlink to an absolute path.
  * Checking `fd_prestat_dir_name` buffer size.
  * Checking `filestat_set_times` for invalid flags.
  * Checking validation of file descriptor in `socket_accept` (#3041).
* Fixed duplicated loading of the same plug-in.
* Fixed option toggle for `wasmedge_process` plug-in.
* Fixed the plug-in searching path on Windows.

Tests:

* Updated the WASM spec tests to the date 2024/02/17.
* Updated the spec tests for the Exception Handling proposal.
* Added the spec tests for the Typed Function Reference proposal.
* Added the spec tests for the GC proposal.

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.

Thank all the contributors who made this release possible!

Abhinandan Udupa, Akihiro Suda, Charlie chan, Dhruv Jain, Draco, Harry Chiang, Hrushikesh, Ikko Eltociear Ashimine, Khagan (Khan) Karimov, LFsWang, LO, CHIN-HAO, Little Willy, Lîm Tsú-thuàn, Meenu Yadav, Omkar Acharekar, Saiyam Pathak, Sarrah Bastawala, Shen-Ta Hsieh, Shreyas Atre, Sylveon, Yage Hu, Yi Huang, Yi-Ying He, alabulei1, am009, dm4, hetvishastri, hugo-syn, hydai, redismongo, richzw, tannal, vincent, zhumeme

If you want to build from source, please use WasmEdge-0.14.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.5 (2023-11-03)

Features:

* [Component] share loading entry for component and module (#2945)
  * Initial support for the component model proposal.
  * This PR allows WasmEdge to recognize the component and module format.
* [WASI-NN] ggml backend:
  * Provide options for enabling OpenBLAS, Metal, and cuBLAS.
  * Bump llama.cpp to b1383
  * Build thirdparty/ggml only when the ggml backend is enabled.
  * Enable the ggml plugin on the macOS platform.
  * Introduce `AUTO` detection. Wasm application will no longer need to specify the hardware spec (e.g., CPU or GPU). It will auto-detect by the runtime.
  * Unified the preload options with case-insensitive matching
  * Introduce `metadata` for setting the ggml options.
    * The following options are supported:
      * `enable-log`: `true` to enable logging. (default: `false`)
      * `stream-stdout`: `true` to print the inferred tokens in the streaming mode to standard output. (default: `false`)
      * `ctx-size`: Set the context size the same as the `--ctx-size` parameter in llama.cpp. (default: `512`)
      * `n-predict`: Set the number of tokens to predict, the same as the `--n-predict` parameter in llama.cpp. (default: `512`)
      * `n-gpu-layers`: Set the number of layers to store in VRAM, the same as the `--n-gpu-layers` parameter in llama.cpp. (default: `0`)
      * `reverse-prompt`: Set the token pattern at which you want to halt the generation. Similar to the `--reverse-prompt` parameter in llama.cpp. (default: `""`)
      * `batch-size`: Set the number of batch sizes for prompt processing, the same as the `--batch-size` parameter in llama.cpp. (default: `512`)
  * Notice: Because of the limitation of the WASI-NN proposal, there is no way to set the metadata during the loading process. The current workaround will re-load the model when `n_gpu_layers` is set to a non-zero value.
  * Installer: Support WASI-NN ggml plugin on both macOS Intel model (CPU only) and macOS Apple Silicon model. (#2882)
* [Java Bindings] provide platform-specific jni and jar for Java bindings (#2980)
* [C API]:
  * Provide getData API for FunctionInstance (#2937)
  * Add the API to set WASI-NN preloads. (#2827)
* [Plugin]:
  * [zlib]:
    * initial support of the zlib plugin (#2562)
    * With a simple building guide and basic working examples
* [MSVC] Support MSVC for building WasmEdge
* [AOT] Support LLVM 17

Fixed issues:

* [Installer]: Double quote the strings to prevent splitting in env file (#2994)
* [AOT]:
  * Validate AOT section header fields
  * Add invariant attribute for memory and global pointer
* [C API]:
  * Fix the wrong logic of getting types from exports.
* [Example] Fix get-string with the latest C++ internal getSpan API. Fixes #2887 (#2929)
* [CI] install llvm@16 to fix macOS build (#2878)

Misc:

* [Example] Update wit-bindgen version from 0.7.0 to 0.11.0 (#2770)

Thank all the contributors who made this release possible!

dm4, hydai, Lîm Tsú-thuàn, Meenu Yadav, michael1017, proohit, Saikat Dey, Shen-Ta Hsieh, Shreyas Atre, Wang Jikai, Wck-iipi, YiYing He

If you want to build from source, please use WasmEdge-0.13.5-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.4 (2023-09-05)

Features:

* [C API] Provide API for registering the Pre- and Post- host functions
  * Pre host function will be triggered before calling every host function
  * Post host function will be triggered after calling every host function
* [CI] Update llvm-windows from 13.0.3 to 16.0.6
  * WasmEdge supports multiple LLVM version, users can choose whatever they want.
  * This change is for CI.
* [CI] build alpine static libraries (#2699)
  * This provides pre-built static libraries using musl-libc on alpine.
* [Plugin] add wasmedge\_rustls\_plugin (#2762)
* [Plugin] implement opencvmini `rectangle` and `cvtColor` (#2705)
* [Test] Migrating spec test from RapidJSON to SIMDJSON (#2659)
* [WASI Socket] AF\_UNIX Support (#2216)
  * This is disable by default.
  * How to enable this feature:
    * CLI: Use `--allow-af-unix`.
    * C API: Use `WasmEdge\_ConfigureSetAllowAFUNIX`.
* [WASI-NN] Add ggml backend for llama (#2763)
  * Integrate llama.cpp as a new WASI-NN backend.
* [WASI-NN] Add load\_by\_name implementation into wasi-nn plugin (#2742)
  * Support named\_model feature.
* [WASI-NN] Added support for Tuple Type Output Tensors in Pytorch Backend (#2564)

Fixed issues:

* [AOT] Fix fallback case of `compileVectorExtAddPairwise`. (#2736)
* [AOT] Fix the neontbl1 codegen error on macOS (#2738)
* [Runtime] fix memory.init oob. issue #2743  (#2758)
* [Runtime] fix table.init oob. issue #2744 (#2756)
* [System] Remove "inline" from Fault::emitFault (#2695) (#2720)
* [Test] Use std::filesystem::u8path instead of a `const char*` Path (#2706)
* [Utils] Installer: Fix checking of shell paths (#2752)
* [Utils] Installer: Formatting and Better source message (#2721)
* [WASI] Avoid undefined function `FindHolderBase::reset`
* [WASI] itimerspec with 0 timeout will disarm timer, +1 to workaround (#2730)

Thank all the contributors that made this release possible!

Adithya Krishna, Divyanshu Gupta, Faidon Liambotis, Jorge Prendes, LFsWang, Lev Veyde, Lîm Tsú-thuàn, Sarrah Bastawala, Shen-Ta Hsieh, Shreyas Atre, Vedant R. Nimje, Yi-Ying He, alabulei1, am009, dm4, erxiaozhou, hydai, vincent, zzz

If you want to build from source, please use WasmEdge-0.13.4-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.3 (2023-07-25)

This is a bugfix release.

Features:

* [CMake] Add a flag to disable libtinfo (#2676)
* [Plugin] Implement OpenCV-mini (#2648)
* [CI] Build wasmedge on Nix (#2674)

Fixed issues:

* WASI Socket: Remove unused fds before closing them. (#2675), part of #2662

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.

Thank all the contributors that made this release possible!

Lîm Tsú-thuàn, Tricster, Tyler Rockwood

If you want to build from source, please use WasmEdge-0.13.3-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.2 (2023-07-21)

This is a bugfix release.

Features:

* Provide static library on `x86_64` and `aarch64` Linux (#2666)
* Provide `wasm_bpf` plugins in the release assets (#2610)
* WASI-NN: Updating install script for OpenVino 2023.0.0 version (#2636)
* Installer: Add new tags support for wasmedge-tensorflow (#2608)
* Fuss: Use own implement of `BoyerMooreHorspoolSearcher` (#2657)

Fixed issues:

* WASI Socket: Fix blocking when multiple requests have the same fds. (#2662)
* Utils: devtoolset-11 is not available on manylinux2014 aarch64, downgrade to devtoolset-10 (#2663)

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.

Thank all the contributors that made this release possible!

Divyanshu Gupta, Faidon Liambotis, hydai, Jorge Prendes, Officeyutong, Shen-Ta Hsieh, Shreyas Atre, Tricster, YiYing He

If you want to build from source, please use WasmEdge-0.13.2-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.1 (2023-07-06)

This is a bugfix release.

Fixed issues:

* Rollback the WasmEdge WASI Socket behavior of V1 functions.
  * Related functions: `getlocaladdr`, and `getpeeraddr`
  * Reason:
    * The address type should be INET4(0) and INET6(1).
    * This regrasion is introduced in [#2557](https://github.com/WasmEdge/WasmEdge/pull/2557).
    * However, the original values of the previous version (< 0.13.0): INET4(4) and INET6(6).
    * To avoid this incompatible behavior, we choose to keep the old behavior.

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.

Thank all the contributors that made this release possible!

If you want to build from source, please use WasmEdge-0.13.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.13.0 (2023-06-30)

Features:

* Updated the WasmEdge shared library.
  * Due to the breaking change of API, bump the `SOVERSION` to `0.0.3`.
* Unified the `wasmedge` CLI tool.
  * Supported the subcommand `run` and `compile` for the `wasmedge` CLI.
  * Users now can use the command `wasmedge run [ARGS]` to drive the original `wasmedge` tool.
  * Users now can use the command `wasmedge compile [ARGS]` to drive the original `wasmedgec` AOT compiler tool.
* Made WasmEdge on `armv7l` great again.
* Bumpped `spdlog` to `v1.11.0`.
  * Refactored the logs to use the `fmt` for formatting.
* Bumpped `blake3` to `1.3.3`.
* Added the CMake option `WASMEDGE_ENABLE_UB_SANITIZER` to enable the undefined behavior sanitizer.
* Deprecated the `wasmedge_httpsreq` plug-in.
* Migrated the WasmEdge extensions into plug-ins.
  * Migrated the [WasmEdge-image](https://github.com/second-state/WasmEdge-image) into the `wasmedge_image` plug-in.
  * Migrated the [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow) into the `wasmedge_tensorflow` and `wasmedge_tensorflowlite` plug-ins.
  * Supported `manylinux2014_x86_64`, `manylinux2014_aarch64`, `darwin_x86_64`, and `darwin_arm64` platforms for the above plug-ins.
* Introduced the `wasi_logging` plug-in.
* Added GPU support for WASI-NN PyTorch backend.
* New APIs for containing data into module instances when in creation.
  * Added the `WasmEdge_ModuleInstanceCreateWithData()` API for creating a module instance with data and its finalizer callback function pointer.
  * Added the `WasmEdge_ModuleInstanceGetHostData()` API for accessing the host data set into the module instance.
* Supported the async invocation with executor.
  * Added the `WasmEdge_ExecutorAsyncInvoke()` API for invoking a WASM function asynchronously.
* Added helper functions for Windows CLI.
  * Added the `WasmEdge_Driver_ArgvCreate()` and `WasmEdge_Driver_ArgvDelete()` APIs to convert UTF-16 arguments to UTF-8.
  * Added the `WasmEdge_Driver_SetConsoleOutputCPtoUTF8()` API to set the output code page to UTF-8.
* Added the unified tool API.
  * Added the `WasmEdge_Driver_UniTool()` API to trigger the WasmEdge CLI tool with command line arguments.

Fixed issues:

* Fixed the WasmEdge C API static library linking command for `llvm-ar-14`.
* Fixed the undefined behavior issues in Loader and Validator.
* Fixed the WASI issues.
  * Denied the absolute path accessing.
  * Opened directories with `__WASI_OFLAGS_DIRECTORY` flag.
  * Don't use `O_PATH` unless flag is exactly `__WASI_OFLAGS_DIRECTORY`.
  * Removed seeking rights on directories.
  * Fixed checking wrong rights in `path_open`.
  * Allowed renumbering and closing preopened `fd`.
  * Disallowed accessing parent directory through `..`.
  * Don't write null pointer at end of args/envs pointer array.
  * Don't write first entry when buffer size is zero.
  * Removed unused VFS objects.
  * Fixed the `fd_readdir`.
  * Corrected the readonly inheriting right.
* Fixed plug-in issues.
  * Fixed the error enumeration in WASI-NN.
  * Fixed the error messages of tensor type in WASI-NN Tensorflow-Lite backend.
  * Handled the model data ownership in WASI-NN Tensorflow-Lite backend.
  * Returned error with the following cases in WASI-Crypto, because OpenSSL 3.0 didn't implement context duplication for `aes-gcm` and `chacha20`.

Refactor:

* Moved the Windows API definitions to `include/system/winapi.h`.
* Dropped the `boost` dependency.
  * Replaced the `boost` endian detection by the macros.
  * Used the `std::boyer_moore_horspool_searcher` instead.
* Refactored the functions for accessing slides on memory instances.
* Moved the `WasmEdge::VM::Async` class to the `include/common` for supporting async invocation in executor.
* Refactored the WASI host functions.
  * Removed duplicate codes on `poll_oneoff` with `edge-trigger` configuration.
  * Refactored Poller interface for reusing the same objects.
  * Supported absolute time flags for `poll_oneoff` on MacOS.
  * Used static vector to speedup CI.
  * Refactored the internal APIs of wasi-socket.
* Refactored the WASI-NN plug-in source.
  * Refined the WASI-NN dependency linking in CMake.
  * Separated the source files for different backends.

Documentations:

* Moved and published the WasmEdge document to <https://wasmedge.org/docs/>.
* Removed all WASM binary files in the source tree.

Tests:

* Updated the WASM spec tests to the date 2023/05/11.
* Added the plug-in unit tests and CI for Linux and MacOS platforms.
* Added new test cases of `cxx20::expected`.

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.

Thank all the contributors that made this release possible!

Adithya Krishna, Chris O'Hara, Edward Chen, Louis Tu, Lîm Tsú-thuàn, Maurizio Pillitu, Officeyutong, Shen-Ta Hsieh, Shreyas Atre, Tricster, Tyler Rockwood, Xin Liu, YiYing He, Yu Xingzi, alabulei1, hydai, michael1017, vincent, yanghaku

If you want to build from source, please use WasmEdge-0.13.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.12.1 (2023-05-12)

This is a hotfix release.

Fixed issues:

* WASI:
  * fix rights of pre-open fd cannot write and fix read-only flag parse (#2458)
* WASI Socket:
  * Workaround: reduce the address family size for the old API
  * fix sock opt & add BINDTODEVICE (#2454)
* MacOS
  * Use OpenSSL 3.0 on MacOS when building the plugins.
  * Update the visibility of plugin functions.
  * Fix AOT Error on MacOS; fix #2427
    * Change enumerate attributes value to zero
    * Change import helper function to private linkage to hide symbols
    * Detect OS version
  * Fix building with statically linked LLVM-15 on MacOS.
  * cmake: quote WASMEDGE_LLVM_LINK_LIBS_NAME variable in order to fix arm64-osx AOT build (#2443)
* Windows:
  * Fix missing msvcp140.dll issue (#2455)
  * Revert #2455 temporarily. Use `CMAKE_MSVC_RUNTIME_LIBRARY` instead of `MSVC_RUNTIME_LIBRARY`.
* Rust Binding:
  * Introduce `fiber-for-wasmedge` (#2468). The Rust binding relies on fiber for some features. Because the runwasi project supports both wasmtime and wasmedge, the wasmtime-fiber with different versions will make the compilation complex. To avoid this, we forked wasmtime-fiber as fiber-for-wasmedge.
  * Add a second phase mechanism to load plugins after the VM has already been built. (#2469)
* Documents:
  * Fix the naming of the AOT wasm file.
  * Add wasmedgec use cases for a slim container.
  * Add the Kwasm document.
  * Fix HostFunction with data example (#2441)

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. For a working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.

Thank all the contributors that made this release possible!

Leonid Pospelov, Shen-Ta Hsieh, Tyler Rockwood, Xin Liu, YiYing He, dm4, hydai, vincent, yanghaku, zzz

If you want to build from source, please use WasmEdge-0.12.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.12.0 (2023-04-24)

Breaking changes:

* Updated the WasmEdge shared library.
  * Due to the breaking change of API, bump the `SOVERSION` to `0.0.2`.
* WasmEdge C API changes.
  * Removed the `WasmEdge_HostRegistration` members and the corresponding module creation APIs to standardize the plug-in module creation.
    * Please refer to the [documentation](https://wasmedge.org/book/en/sdk/c/0.11.2/upgrade_to_0.12.0.html) for how to upgrade.
    * Removed the `WasmEdge_HostRegistration_WasiNN` enum and the `WasmEdge_ModuleInstanceCreateWasiNN()` API.
    * Removed the `WasmEdge_HostRegistration_WasiCrypto_Common` enum and the `WasmEdge_ModuleInstanceCreateWasiCryptoCommon()` API.
    * Removed the `WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon` enum and the `WasmEdge_ModuleInstanceCreateWasiCryptoAsymmetricCommon()` API.
    * Removed the `WasmEdge_HostRegistration_WasiCrypto_Kx` enum and the `WasmEdge_ModuleInstanceCreateWasiCryptoKx()` API.
    * Removed the `WasmEdge_HostRegistration_WasiCrypto_Signatures` enum and the `WasmEdge_ModuleInstanceCreateWasiCryptoSignatures()` API.
    * Removed the `WasmEdge_HostRegistration_WasiCrypto_Symmetric` enum and the `WasmEdge_ModuleInstanceCreateWasiCryptoSymmetric()` API.
    * Removed the `WasmEdge_HostRegistration_WasmEdge_Process` enum and the `WasmEdge_ModuleInstanceCreateWasmEdgeProcess()` API.
* Changed the `WasmEdge_VMCleanup()` behavior.
  * After calling this API, the registered modules except the WASI and plug-ins will all be cleaned.
* Standaloned the `WasmEdge-Process` plug-in.
  * After this version, users should use the installer to install the `WasmEdge-Process` plug-in.

Features:

* Introduced the `Plugin` context and related APIs.
  * Added the `WasmEdge_PluginContext` struct.
  * Added the `WasmEdge_PluginLoadFromPath()` API for loading a plug-in from a specific path.
  * Added the `WasmEdge_PluginListPluginsLength()` and `WasmEdge_PluginListPlugins()` APIs for getting the loaded plug-in names.
  * Added the `WasmEdge_PluginFind()` API for retrieving a loaded plug-in by its name.
  * Added the `WasmEdge_PluginGetPluginName()` API for retrieving the plug-in name.
  * Added the `WasmEdge_PluginListModuleLength()` and `WasmEdge_PluginListModule()` APIs for listing the module names of a plug-in.
  * Added the `WasmEdge_PluginCreateModule()` API for creating the specific module instance in a plug-in by its name.
* Introduced the multiple WASI socket API implementation.
  * The `sock_accept()` is compatible with the WASI spec.
  * The V2 socket implementation is using a larger socket address data structures. With this, we can start to supporting `AF_UINX`
* Added the `VM` APIs.
  * Added the `WasmEdge_VMGetRegisteredModule()` API for retrieving a registered module by its name.
  * Added the `WasmEdge_VMListRegisteredModuleLength()` and `WasmEdge_VMListRegisteredModule()` APIs for listing the registered module names.
* Introduced the python version WasmEdge installer.
* Added the `wasm_bpf` plug-in.
* Enabled the read-only WASI filesystem.
  * Users can add the `--dir guest_path:host_path:readonly` option in WasmEdge CLI to assign the read-only configuration.
* Updated the ABI of the `wasi_ephemeral_sock`.
  * Added the output port of the `sock_recv_from`.
  * Updated the API of `sock_getlocaladdr`.
  * Unified the socket address size to 128-bit.
* Allowed the multiple VM instances.
* Supported using `libtool` to archive the WasmEdge static library.
* Supported LLVM 15.0.7.

Fixed issues:

* Fixed WASI issues.
  * Fixed the leaking information about the host STDIN, STDOUT, and STDERR after getting the `filestat`.
  * Fixed the lookup of symbolic link at `path_filestat_set_times`.
  * Fixed `open` for the wchar path issue on windows.
  * Fixed the rights of `path_open`.
* Fixed WASI-NN issues.
  * Fixed the definition of `wasi_nn::TensorType` to prevent from comparing with dirty data.
* Fixed WASI-Crypto issues.
  * Fixed the `keypair_generate` for rsa-pss.
  * Fixed the `keypair_import` read pem as pkcs8.
* Fixed WASI-Socket issues.
  * Fixed the buffer size of `sock_getpeeraddr`.
* Fixed the lost intrinsics table in AOT mode when using the WasmEdge C API.
* Fixed the registration failed of WasmEdge plug-in through the C API.
* Fixed the implementation in `threads` proposal.
  * Fixed the error in `atomic.notify` and `atomic.wait` instructions.
  * Fixed the decoding of `atomic.fence` instruction.
  * Corrected the error message of waiting on unshared memory.
* Handle canonical and arithmetical `NaN` in `runMaxOp()` and `runMinOp()`.

Refactor:

* Refactored the implementation of number loading in the file manager.
  * Supported `s33` and `sn` loading and decoding.
* Refactored the `WasmEdge::ValType`.
  * Removed the `WasmEdge::ValType::None`.
  * Used the flag in `WasmEdge::BlockType` for supporting the type index.
  * Removed the `WasmEdge::Validator::VType` and used the `WasmEdge::ValType` instead.

Known issues:

* Universal WASM format failed on MacOS platforms.
  * In current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause bus error when execution.
  * We are trying to fix this issue. For working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.
* Plug-in linking on MacOS platforms.
  * The plug-in on MacOS platforms will cause symbol not found when dynamic linking.
  * We are trying to fix this issue. For working around, please implement the host modules instead of plug-ins.

Documentations:

* Fixed various typos.
* Updated the C API documents.
* Added the [WasmEdge installer guide](https://wasmedge.org/book/en/contribute/installer.html).
* Updated the [Android NDK example](https://wasmedge.org/book/en/contribute/build_from_src/android/ndk.html).
* Added the [static library linking guide](https://wasmedge.org/book/en/sdk/c/library.html#link-with-wasmedge-static-library).
* Added the [WasmEdge plug-in implementation guide](https://wasmedge.org/book/en/develop_plugin/c.html).

Tests:

* Updated the WASM spec tests to the date 2022/12/15.
* Added the plug-in unit tests on Linux platforms.

Thank all the contributors that made this release possible!

Abhinandan Udupa, Achille, Afshan Ahmed Khan, Daniel Golding, DarumaDocker, Draco, Harry Chiang, Justin Echternach, Kenvi Zhu, LFsWang, Leonid Pospelov, Lîm Tsú-thuàn, MediosZ, O3Ol, Officeyutong, Puelloc, Rafael Fernández López, Shen-Ta Hsieh, Shreyas Atre, Sylveon, Tatsuyuki Kobayashi, Vishv Salvi, Xin Liu, Xiongsheng Wang, YiYing He, alabulei1, dm4, hydai, jeongkyu, little-willy, michael1017, shun murakami, xxchan, 云微

If you want to build from source, please use WasmEdge-0.12.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.11.2 (2022-11-03)

Features:

* Added the new WasmEdge C API.
  * Added the `WasmEdge_ConfigureSetForceInterpreter()` API to set the force interpreter mode.
  * Added the `WasmEdge_ConfigureIsForceInterpreter()` API to check the force interpreter mode in configurations.
  * Added the `WasmEdge_LogOff()` API to turn off the logging.
  * Due to introducing the new APIs, bump the `SOVERSION` to `0.0.1`.
* Added the additional hint messages if import not found when in instantiation.
* Added the forcibly interpreter execution mode in WasmEdge CLI.
  * Users can use the `--force-interpreter` option in the `wasmedge` tool to forcibly execute WASM files (includes the AOT compiled WASM files) in interpreter mode.
* Supported WASI-NN plug-in with TensorFlow-Lite backend on Ubuntu 20.04 x86_64.
  * Users can refer to the [WASI-NN document](https://wasmedge.org/book/en/write_wasm/rust/wasinn.html) for the information.
  * For building with enabling WASI-NN with TensorFlow-Lite backend, please add the `-DWASMEDGE_PLUGIN_WASI_NN_BACKEND="TensorFlowLite"` in `cmake`.
* Bump the `fmt` format of logging to `9.0.0`.
* Added the new experimental edge-triggered epoll API `epollOneoff` in the WASI component.

Fixed issues:

* Detected the valid `_start` function of the WasmEdge CLI command mode.
  * For the invalid `_start` function, the WasmEdge CLI will execute that function in the reactor mode.
* Fixed the non-English WasmEdge CLI arguments error on Windows.
* Fixed the AOT compiler issues.
  * Fixed the operand of `frintn` on `arm64` platforms.
  * Corrected the `unreachable` status to record on every control stacks.
* Refined the Loader performance.
  * Capped the maximum local counts to 67108864 (2^26).
  * Rejected wrong data when loading the universal WASM.
  * Rejected the unreasonable long vector sizes.
* Fixed the lost `std` namespace in the `experimental::expected`.
* Fixed the repeatedly compilation of universal WASM format.
  * If users use the `wasmedgec` tool to compile the universal WASM file, the AOT compiled WASM data will be appended into the output.
  * In the cases of duplicated AOT compiled universal WASM file which has more than 1 section of AOT compiled WASM data, the WasmEdge runtime will use the latest appended one when execution.
* Hidden the local symbols of the WasmEdge shared library.
* Loaded the default plug-in path from the path related to the WasmEdge shared library.
  * This only fixed on the MacOS and Linux platforms now.
* Updated the minimum CMake required version on Android.

Known issues:

* Universal WASM format failed on MacOS platforms.
  * In current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause bus error when execution.
  * We are trying to fix this issue. For working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.
* Plug-in linking on MacOS platforms.
  * The plug-in on MacOS platforms will cause symbol not found when dynamic linking.
  * We are trying to fix this issue. For working around, please implement the host modules instead of plug-ins.

Documentations:

* Updated the [WasmEdge-Go document](https://wasmedge.org/book/en/sdk/go/ref.html) to `v0.11.0`.

Tests:

* Added the WASI-NN TensorFlow-Lite backend unit test.
* Added the new C API unit tests.
* Applied more fuzz tests for WasmEdge CLI.

Thank all the contributors that made this release possible!

Abhinandan Udupa, Gustavo Ye, HangedFish, Harry Chiang, Hiroaki Nakamura, Kenvi Zhu, LFsWang, MediosZ, Shen-Ta Hsieh, Shreyas Atre, Xin Liu, YiYing He, abhinandanudupa, dm4, he11c, hydai, vincent, yyy1000, zhlhahaha

If you want to build from source, please use WasmEdge-0.11.2-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.11.1 (2022-10-03)

Features:

* Supported WASI-NN plug-in with PyTorch backend on Ubuntu 20.04 x86_64.
  * Users can refer to the [WASI-NN document](https://wasmedge.org/book/en/write_wasm/rust/wasinn.html) for the information.
  * For building with enabling WASI-NN with PyTorch backend, please add the `-DWASMEDGE_PLUGIN_WASI_NN_BACKEND="PyTorch"` in `cmake`.
* Updated the WASI-Crypto proposal and supported OpenSSL 3.0.
* Supported LLVM 15.
* Added the plug-in C API.
* Extended WasmEdge CLI.
  * Allow the optimization level assignment in `wasmedgec` tool.
  * Supported the `v128` value type printing in `wasmedge` tool.
* Released Ubuntu 20.04 version with statically linked LLVM.

Fixed issues:

* Fixed the `private` members into the `protected` in the module instance class.
* Fixed the type mismatch for IntrinsicsTable initialization statement in the AOT compiler.

Known issues:

* Universal WASM format failed on MacOS platforms.
  * In current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause bus error when execution.
  * We are trying to fix this issue. For working around, please use the `--optimize=0` to set the compiler optimization level to `O0` in `wasmedgec` CLI.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.
* Plug-in linking on MacOS platforms.
  * The plug-in on MacOS platforms will cause symbol not found when dynamic linking.
  * We are trying to fix this issue. For working around, please implement the host modules instead of plug-ins.

Documentations:

* Refactored the [WasmEdge book](https://wasmedge.org/book/en/).

Tests:

* Added the WASI-NN PyTorch backend unit test.
* Added fuzzing tests for WasmEdge CLI.

Thank all the contributors that made this release possible!

DarumaDocker, Faidon Liambotis, Gustavo Ye, LFsWang, MediosZ, Michael Yuan, Shen-Ta Hsieh, Tricster, Xin Liu, Yeongju Kang, YiYing He, Zhou Zhou, hydai, jeeeerrrpop, sonder-joker, vincent

If you want to build from source, please use WasmEdge-0.11.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.11.0 (2022-08-31)

Breaking changes:

* WasmEdge C API changes.
  * Refactored the host function definition to export the calling frame.
    * The first parameter of `WasmEdge_HostFunc_t` is replaced by `const WasmEdge_CallingFrameContext *`.
    * The first parameter of `WasmEdge_WrapFunc_t` is replaced by `const WasmEdge_CallingFrameContext *`.
  * Extended the content of `WasmEdge_Result`.
  * Added the const qualifier of some APIs.
    * Added the const qualifier of the first parameter of `WasmEdge_StoreFindModule()`.
    * Added the const qualifier of the first parameter of `WasmEdge_AsyncWait()`.
    * Added the const qualifier of the first parameter of `WasmEdge_AsyncWaitFor()`.
    * Added the const qualifier of the first parameter of `WasmEdge_AsyncGetReturnsLength()`.
    * Added the const qualifier of the first parameter of `WasmEdge_AsyncGet()`.
    * Added the const qualifier of the first parameter of `WasmEdge_VMGetFunctionType()`.
    * Added the const qualifier of the first parameter of `WasmEdge_VMGetFunctionTypeRegistered()`.
    * Added the const qualifier of the first parameter of `WasmEdge_VMGetFunctionListLength()`.
    * Added the const qualifier of the first parameter of `WasmEdge_VMGetFunctionList()`.
    * Added the const qualifier of the first parameter of `WasmEdge_VMGetImportModuleContext()`.
  * Renamed the plugin API.
    * Renamed `WasmEdge_Plugin_loadWithDefaultPluginPaths()` to `WasmEdge_PluginLoadWithDefaultPaths()`.
* Dropped the manylinux1 and manylinux2010 support. Please refer to the [deprecation notice](https://github.com/WasmEdge/WasmEdge/discussions/1780).
* Standardize the SONAME and SOVERSION for WasmEdge C API
  * The name of the library is changed to `libwasmedge.so`, `libwasmedge.dyld`, and `wasmedge.dll`.
  * Users should change the linker flag from `lwasmedge_c` to `lwasmedge`.
  * The initialized SONAME is set to `libwasmedge.so.0`.
  * The initialized SOVERSION is set to `libwasmedge.so.0.0.0`.

Features:

* Updated CMake options of WasmEdge project.
  * Added `WASMEDGE_LINK_LLVM_STATIC` option to link the LLVM statically into WasmEdge shared library or tools.
  * Removed the `WASMEDGE_BUILD_STATIC_TOOLS` option and replaced by the `WASMEDGE_LINK_TOOLS_STATIC` option.
  * For details, please refer to the [documentation](https://wasmedge.org/book/en/extend/build.html#building-options).
  * After this version, our releases on MacOS platforms will link the LLVM library statically to reduce the installation of LLVM from Homebrew for the users.
* Supported the user-defined error code for host functions.
  * The 24-bit size user-defined error code is supported (smaller than 16777216).
  * Developers can use the `WasmEdge_ResultGen()` API to generate the result and return.
* Exported the `CallingFrame` instead of the memory instance in host functions.
  * New `WasmEdge_CallingFrameContext` struct.
  * Developers can use `WasmEdge_CallingFrameGetModuleInstance()` API to get the module instance of current top frame in calling stack in host function body.
  * Developers can use `WasmEdge_CallingFrameGetMemoryInstance()` API to get the memory instance by index in host function body.
    * To quickly upgrade from the previous WasmEdge versions, developer can use the `WasmEdge_CallingFrameGetMemoryInstance(Context, 0)` to get the same memory instance of the previous host function definition.
  * Developers can use `WasmEdge_CallingFrameGetExecutor()` API to get the executor context in host function body.
* Extended the `WasmEdge_Result` struct to support user defined error codes of host functions.
  * Added `WasmEdge_ResultGen()` API to generate the `WasmEdge_Result` struct of user defined error code.
  * Added `WasmEdge_ResultGetCategory()` API to get the error code category.
* Added a new API for looking up the native handler from a given WASI mapped Fd/Handler.
  * Added `WasmEdge_ModuleInstanceWASIGetNativeHandler` to get the native handler.
* Added a new API for compiling a given WASM byte array.
  * Added `WasmEdge_CompilerCompileFromBuffer` to compile from buffer.
* Added `httpsreq` plugin on Linux platforms.

Fixed issues:

* Fixed the binary format loading.
  * Fixed the error of immediate loading of const instructions in debug mode.
  * Updated the `memarg` of memory instructions for the multiple memories proposal changes.
* Fixed the AOT issues.
  * Fixed the missed mask of shift operands.
  * Fixed the fallback case of vector instructions if the `SSE4.1` is not supported on the x86_64 platforms or the `NEON` is not supported on the aarch64 platforms.
  * Fixed the `sdk_version` of `lld` warning on MacOS with LLVM 14.
* Fixed the unexpected error message when execution.
  * Refined the terminated case to prevent from printing the unexpected error message.
* Refined the symbols of output WasmEdge shared libraries.
  * Removed the weak symbol of WasmEdge plugins.
  * Hide the `lld` symbols of WasmEdge shared library.
* Fixed the release packaging.
  * Fixed the lost of statically linking LLVM into WasmEdge shared library.
  * Fixed the lost of files when packaging on Windows.

Refactor:

* Reorganized the CI workflows to reuse the similar jobs.
* Refactored the enum related headers.
  * Separated the C and C++ enum definition headers.
  * Not to package the C++ related headers.
* Updated the WASI and plugin host functions for the API change.

Known issues:

* Universal WASM format failed on MacOS platforms.
  * In current status, the universal WASM format output of the AOT compiler with the `O1` or upper optimizations on MacOS platforms will cause bus error when execution.
  * We are trying to fix this issue. For working around, please use the shared library format output of the AOT mode, or set the compiler optimization level to `O0` in WasmEdge C API.
  * Developers can specify the extension name as `.dylib` on MacOS for the shared library format output when using `wasmedgec` tool.
* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.
* Plug-in linking on MacOS platforms.
  * The plug-in on MacOS platforms will cause symbol not found when dynamic linking.
  * We are trying to fix this issue. For working around, please implement the host modules instead of plug-ins.

Documentations:

* Updated the [WasmEdge build options documentation](https://wasmedge.org/book/en/extend/build.html#building-options).
* Updated the [WasmEdge C API documentation](https://wasmedge.org/book/en/embed/c/ref.html) for the breaking change.
  * For upgrading from `0.10.1` to `0.11.0`, please refer to [the document](https://wasmedge.org/book/en/embed/c/0.10.1/upgrade_to_0.11.0.html).
  * For the old API of `0.10.1`, please refer to [the document](https://wasmedge.org/book/en/embed/c/0.10.1/ref.html).

Tests:

* Updated the spec tests to the date `20220712`.
* Updated the test suite of the multiple memories proposal.
* Updated the plugin tests for the host function API breaking change.

Thank all the contributors that made this release possible!

Cheng-En Lee, Chih-Hsuan Yen, Galden, GreyBalloonYU, HeZean, Michael Yuan, Shen-Ta Hsieh, Xin Liu, Yi Huang, Yi-Ying He, Zhenghao Lu, Zhou Zhou, dm4, hydai

If you want to build from source, please use WasmEdge-0.11.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.10.1 (2022-07-28)

Features:

* Supported WASI-NN plug-in with OpenVINO backend on Ubuntu 20.04 x86_64.
  * Users can refer to the [standard extension status](https://wasmedge.org/book/en/intro/standard.html) for the information.
  * For building with enabling WASI-NN with OpenVINO backend, please add the `-DWASMEDGE_PLUGIN_WASI_NN_BACKEND="OpenVINO"` in `cmake`.
* Supported WASI-crypto plug-in on Ubuntu 20.04 x86_64, manylinux2014 x86_64, and manylinux2014 aarch64.
  * Users can refer to the [standard extension status](https://wasmedge.org/book/en/intro/standard.html) for the information.
  * For building with enabling WASI-crypto with OpenSSL 1.1, please add the `-DWASMEDGE_PLUGIN_WASI_CRYPTO=ON` in `cmake`.
* Added the static tool building option.
  * By default, WasmEdge tools will depend on the WasmEdge shared library.
  * Developers can add the `-DWASMEDGE_BUILD_STATIC_LIB=On` and `-DWASMEDGE_BUILD_STATIC_TOOLS=On` to build the stand-alone WasmEdge CLI tools.
* Exported the components of `WasmEdge_VMContext` in WasmEdge C API.
  * Added the `WasmEdge_VMGetLoaderContext` API for retrieving the `WasmEdge_LoaderContext` in VM.
  * Added the `WasmEdge_VMGetValidatorContext` API for retrieving the `WasmEdge_ValidatorContext` in VM.
  * Added the `WasmEdge_VMGetExecutorContext` API for retrieving the `WasmEdge_ExecutorContext` in VM.
* Added the API for CLI tools.
  * Developers can use the `WasmEdge_Driver_Compiler` API to trigger the WasmEdge AOT compiler tool.
  * Developers can use the `WasmEdge_Driver_Tool` API to trigger the WasmEdge runtime tool.
* Supported the WASM `threads` proposal.
  * Added the `WasmEdge_Proposal_Threads` for the configuration in WasmEdge C API.
  * Users can use the `--enable-threads` to enable the proposal in `wasmedge` and `wasmedgec` tools.
* Supported LLVM 14 on MacOS.
  * Used the new `macho` in lld on LLVM-14 envronment.
  * Bumpped IWYU to 0.18 to be compatible with LLVM 14 on MacOS.
* Bumpped the MacOS x86_64 build to MacOS 11.

Fixed issues:

* Fixed the universal WASM format failed on MacOS platforms.
  * Developers can specify the extension name as `.wasm` on MacOS as the universal WASM format output of the AOT compiler to enable the AOT mode.
* Fixed the WasmEdge C API static library on MacOS with LLVM 14.
  * The WasmEdge C API static library is in experimental and not guaranteed. The shared library is recommended.
* Reduced the branch miss when instantiating AOT-compiled WASM.

Refactor:

* Moved the code of WasmEdge CLI tools into `WasmEdge::Driver`.
* Moved the plugin tests into the `test/plugins` folder.

Known issues:

* WasmEdge CLI failed on Windows 10 issue.
  * Please refer to [here for the workaround](https://github.com/WasmEdge/WasmEdge/issues/1559) if the `msvcp140.dll is missing` occurs.
* Plug-in linking on MacOS platforms.
  * The plug-in on MacOS platforms will cause symbol not found when dynamic linking.
  * We are trying to fix this issue. For working around, please implement the host modules instead of plug-ins.

Documentations:

* Added the [documentation for WASI-NN supporting on WasmEdge](https://wasmedge.org/book/en/dev/rust/wasinn.html).

Tests:

* Added the spec tests for the `threads` proposal.
* Added the WASI-NN unit tests.

Thank all the contributors that made this release possible!

Abhinandan Udupa, Chris Ho, Faidon Liambotis, Frank Lin, Jianbai Ye, Kevin O'Neal, LFsWang, Lokesh Mandvekar, Michael Yuan, O3Ol, RichardAH, Shen-Ta Hsieh, Shreyas Atre, Sylveon, Tricster, William Wen, 罗泽轩, Xin Liu, Yi Huang, Yi-Ying He, Yixing Jia, Yukang, abhinandanudupa, alabulei1, dm4, eat4toast, eee4017, hydai, sonder-joker, spacewander, swartz-k, yale

If you want to build from source, please use WasmEdge-0.10.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.10.0 (2022-05-26)

Breaking changes:

* WasmEdge C API changes.
  * Merged the `WasmEdge_ImportObjectContext` into the `WasmEdge_ModuleInstanceContext`.
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
  * Used the pointer to `WasmEdge_FunctionInstanceContext` instead of the index in the `FuncRef` value type.
    * `WasmEdge_ValueGenFuncRef()` is changed to use the `const WasmEdge_FunctionInstanceContext *` as it's argument.
    * `WasmEdge_ValueGetFuncRef()` is changed to return the `const WasmEdge_FunctionInstanceContext *`.
  * Moved the functions of `WasmEdge_StoreContext` to the `WasmEdge_ModuleInstanceContext`.
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
  * Updated the `WasmEdge_VMContext` APIs.
    * Added the `WasmEdge_VMGetActiveModule()`.
    * `WasmEdge_VMGetImportModuleContext()` is changed to return the `WasmEdge_FunctionInstanceContext *`.
    * `WasmEdge_VMRegisterModuleFromImport()` is changed to use the `const WasmEdge_ModuleInstanceContext *` as it's argument.
  * For upgrading from `0.9.1` to `0.10.0`, please refer to [the document](https://wasmedge.org/book/en/embed/c/0.9.1/upgrade_to_0.10.0.html).

Features:

* Supported LLVM 14.
* Supported the WASM `tail-call` proposal.
  * Added the `WasmEdge_Proposal_TailCall` for the configuration in WasmEdge C API.
  * Users can use the `--enable-tail-call` to enable the proposal in `wasmedge` and `wasmedgec` tools.
* Supported the WASM `extended-const` proposal.
  * Added the `WasmEdge_Proposal_ExtendedConst` for the configuration in WasmEdge C API.
  * Users can use the `--enable-extended-const` to enable the proposal in `wasmedge` and `wasmedgec` tools.
* Supported thread-safe in `WasmEdge_VMContext`, `WasmEdge_ConfigureContext`, `WasmEdge_ModuleInstanceContext`, and `WasmEdge_StoreContext` APIs.
* Supported the gas limit in AOT mode.
* New supporting of the wasi-socket proposal.
  * Supported `send_to`.
  * Supported `resv_from`.
* Plugin support
  * Add loadable plugin support.
  * Move `wasmedge_process` to a loadable plugin.

Fixed issues:

* Fixed wasi-socket proposal issues.
  * Fixed wasi-socket on MacOS.
  * Fixed error when calling `poll_oneoff` with the same `fd` twice.
  * Fixed error when calling `fd_close` on socket.
  * Forged zero-terminated string for `::getaddrinfo`.
  * Checked the socket options enumeration for valid value.
* Fixed the statistics enable/disable routine.
* Fixed the output format by the file extension name detection on multiple platforms.

Known issues:

* Universal WASM format failed on MacOS platforms.
  * In current status, the universal WASM format output of the AOT compiler on MacOS platforms will cause bus error when execution.
  * We are trying to fix this issue. For working around, please use the shared library format output of the AOT mode.
  * Developers can specify the extension name as `.dylib` on MacOS, `.so` on Linux, and `.dll` on Windows for the shared library format output of the AOT compiler.

Refactor:

* Supported multi-thread execution.
  * Changed the `StackManager` in `Executor` as thread local to support the multi-thread.
  * Used atomic operations for cost measuring.
  * Supported multi-thread timer.
* Refactored the enumerations.
  * Replaced the `std::unordered_map` of the enumeration strings with `DenseMap` and `SpareMap`.
  * Merged the both C and C++ enumeration definitions into the `enum.inc` file.
  * Updated the `ErrCode` enumeration for the newest spec tests.
* Refactored the code architecture for supporting `tail-call` proposal.
  * Split the `call_indirect` execution routine in compiler into AOT and interpreter path.
  * Updated the pop frame mechanism in the `StackManager`.
  * Updated the enter function mechanism.
* Refined the file manager in `Loader`.
  * Supported the offset seeking in file and buffer.
  * Skipped the instructions parsing in AOT mode for better loading performance.
* Refined the branch mechanism in the `StackManager` for better performance in the interpreter mode.
  * Pre-calculated the stack offset for branch in the validation phase.
  * Removed the label stack in the `StackManager` and used the pre-calculated data for branch.
  * Removed the dummy frame mechanism in the `StackManager`.
* Supplied the pointer-based retrieving mechanism in the `StoreManager` and `ModuleInstance`.
  * Removed the address mechanism for instances in the `StoreManager`.
  * Added the unsafe getter functions for the instances.
* Refactored the `StoreManager`, `ModuleInstance`, and `Executor`.
  * Used the `ModuleInstance`-based resource management instead of `StoreManager`-based.
  * Moved the ownership of instances from the `StoreManager` into the `ModuleInstance`.
  * Merged the `ImportObject` into the `ModuleInstance`.
  * Invoking functions by `FunctionInstance` rather than the function name in `Executor`.

Documentations:

* Updated the [WasmEdge C API documentation](https://wasmedge.org/book/en/embed/c/ref.html) for the breaking change.
  * For upgrading from `0.9.1` to `0.10.0`, please refer to [the document](https://wasmedge.org/book/en/embed/c/0.9.1/upgrade_to_0.10.0.html).
  * For the old API of `0.9.1`, please refer to [the document](https://wasmedge.org/book/en/embed/c/0.9.1/ref.html).
* Updated the [WasmEdge GO documentation](https://wasmedge.org/book/en/embed/go/ref.html) for the breaking change.
  * For upgrading from `v0.9.2` to `v0.10.0`, please refer to [the document](https://wasmedge.org/book/en/embed/go/0.9.1/upgrade_to_0.10.0.html).
  * For the old API of `v0.9.2`, please refer to [the document](https://wasmedge.org/book/en/embed/go/0.9.1/ref.html).

Tests:

* Updated the spec tests to the date `20220504`.
* Added the spec tests for the `tail-call` proposal.
* Added the spec tests for the `extended-const` proposal.
* Added the mixed invocation tests between interpreter mode and AOT mode WASM functions.
* Added the thread-safe and multi-thread execution tests.
* Added wasi-socket tests for `poll_oneoff`, `send_to`, and `recv_from`.

Thank all the contributors that made this release possible!

朱亚光, Abhinandan Udupa, Ang Lee, Binbin Zhang, Chin Zhi Wei, DarumaDocker, Elon Cheng, FlyingOnion, Hanged Fish, Herschel Wang, JIAN ZHONG, JcJinChen, Jeremy, JessesChou, JieDing, Kodalien, Kunshuai Zhu, LFsWang, LaingKe, MediosZ, Michael Yuan, Nicholas Zhan, 华德禹, O3Ol, Rui Li, Shen-Ta Hsieh, Shreyas Atre, Sylveon, TheLightRunner, Vaniot, Vinson, 罗泽轩, Xin Liu, Yi Huang, YiYing He, YoungLH, abhinandanudupa, border1px, dm4, eat4toast, hydai, jerbmarx, luckyJ-nj, meoww-bot, mydreamer4134, situ2001, tpmccallum, treeplus, wangyuan249, yale, 王琦

If you want to build from source, please use WasmEdge-0.10.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.9.1 (2022-02-10)

Features:

* WASI
  * Added the `sock_getsockopt`, `sock_setsockopt`, `sock_getlocaladdr`, `sock_getpeeraddr`, and `sock_getaddrinfo` host functions for the WASI socket proposal.
* Supported the interruptible execution.
  * Added the `WasmEdge_Async` struct in WasmEdge C API for the asynchronous execution.
    * Added the `WasmEdge_AsyncWait` API for waiting an asynchronous execution.
    * Added the `WasmEdge_AsyncWaitFor` API for waiting an asynchronous execution with timeout.
    * Added the `WasmEdge_AsyncCancel` API for canceling an asynchronous execution.
    * Added the `WasmEdge_AsyncGetReturnsLength` API for waiting and getting the return value length of asynchronous execution.
    * Added the `WasmEdge_AsyncGet` API for waiting and getting the asynchronous execution results.
    * Added the `WasmEdge_AsyncDelete` API for destroying the `WasmEdge_Async` object.
  * Added the asynchronous mode execution APIs.
    * Added the `WasmEdge_VMAsyncRunWasmFromFile` API for executing WASM from a file asynchronously.
    * Added the `WasmEdge_VMAsyncRunWasmFromBuffer` API for executing WASM from a buffer asynchronously.
    * Added the `WasmEdge_VMAsyncRunWasmFromASTModule` API for executing WASM from an `WasmEdge_ASTModuleContext` asynchronously.
    * Added the `WasmEdge_VMAsyncExecute` API for invoking a WASM function asynchronously.
    * Added the `WasmEdge_VMAsyncExecuteRegistered` API for invoking a registered WASM function asynchronously.
  * Added the option for timeout settings of the AOT compiler.
    * Added the `WasmEdge_ConfigureCompilerSetInterruptible` API for setting the interruptibility of the AOT compiler.
    * Added the `WasmEdge_ConfigureCompilerIsInterruptible` API for getting the interruptibility of the AOT compiler.
* Supported the WASM `multi-memories` proposal.
  * Added the `WasmEdge_Proposal_MultiMemories` for the configuration in WasmEdge C API.
  * Users can use the `--enable-multi-memory` to enable the proposal in `wasmedge` and `wasmedgec` tools.
* Enabled the gas limitation of the `wasmedge` CLI.
  * Users can use the `--gas-limit` to assign the limitation of costs.
* Beautified and colorized the WasmEdge CLI help information.

Fixed issues:

* Fixed the memory leak in function instances.
* Reduced the memory usage of the instruction class.
* Fixed the return value of the `fread` and `fwrite` WASI functions on Windows.

Refactor:

* Used `assumingUnreachable` instead of `__builtin_unreachable` to help the compiler to generate better codes.
* Updated the order of the members in the proposal enumeration.
* Refactored the instruction class for reducing the memory usage.
  * Refactored the `WasmEdge::BlockType` into a struct.
  * Categorized the members of the instruction class into a union.

Documentations:

* Added the [build-on-Windows-10](docs/build_on_windows.md) documentation.
* Added the [Reference Book](https://wasmedge.org/book/en/).
* Updated the [Release process](ReleaseProcess.md).

Tests:

* Handled the tests for the 32-bit platforms.
* Added the spec tests for the `multi-memories` proposal.
* Added the test cases for `getaddrinfo` host function.
* Added the interruptible execution tests.
* Added the unit tests of async APIs.

Misc:

* Updated the `blake3` library to `1.2.0`.
* Added the copyright text.
* Fixed the coding style of the comments.
* Added the Windows installer release CI.
* Added the aarch64 Android support based on r23b.
* Added the Android example for WasmEdge C API.

Thank all the contributors that made this release possible!

2021, Antonio Yang, AvengerMoJo, Hanged Fish, Harinath Nampally, KernelErr, Michael Yuan, MileyFu, O3Ol, Saksham Sharma, Shen-Ta Hsieh(BestSteve), Shreyas Atre, SonOfMagic, Stephan Renatus, Sven Pfennig, Vaughn Dice, Xin Liu, Yi, Yi-Ying He, Yukang Chen, ZefengYu, ZhengX, alabulei1, alittlehorse, baiyutang, 董哲, hydai, javadoors, majinghe, meoww-bot, pasico, peterbi, villanel, wangshishuo, wangyuan249, wby, wolfishLamb, 王琦

If you want to build from source, please use WasmEdge-0.9.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.9.0 (2021-12-09)

Breaking changes:

* Turned on the `SIMD` proposal by default.
  * The `WasmEdge_ConfigureContext` will turn on the `SIMD` proposal automatically.
  * Users can use the `--disable-simd` to disable the `SIMD` proposal in `wasmedge` and `wasmedgec`.
* For better performance, the Statistics module is disabled by default.
  * To enable instruction counting, please use `--enable-instruction-count`.
  * To enable gas measuring, please use `--enable-gas-measuring`.
  * To enable time  measuring, please use `--enable-time-measuring`.
  * For the convenience, use `--enable-all-statistics` will enable all available statistics options.
* `wasmedgec` AOT compiler tool behavior changes.
  * For the output file name with extension `.so`, `wasmedgec` will output the AOT compiled WASM in shared library format.
  * For the output file name with extension `.wasm` or other cases, `wasmedgec` will output the WASM file with adding the AOT compiled binary in custom sections. `wasmedge` runtime will run in AOT mode when it executes the output WASM file.
* Modulized the API Headers.
  * Moved the API header into the `wasmedge` folder. Developers should include the `wasmedge/wasmedge.h` for using the WasmEdge shared library after installation.
  * Moved the enumeration definitions into `enum_errcode.h`, `enum_types.h`, and `enum_configure.h` in the `wasmedge` folder.
  * Added the `201402L` C++ standard checking if developer includes the headers with a C++ compiler.
* Adjusted the error code names.
  * Please refer to the [ErrCode enum](https://github.com/WasmEdge/WasmEdge/blob/master/include/common/enum_errcode.h) definition.
* Renamed the `Interpreter` into `Executor`.
  * Renamed the `Interpreter` namespace into `Executor`.
  * Moved the headers and sources in the `Interpreter` folder into `Executor` folder.
  * Renamed the `Interpreter` APIs and listed below.
* WasmEdge C API changes.
  * Updated the host function related APIs.
    * Deleted the data object column in the creation function of `ImportObject` context.
    * Merged the `HostFunctionContext` into `FunctionInstanceContext`.
      * Deleted the `WasmEdge_HostFunctionContext` object. Please use the `WasmEdge_FunctionInstanceContext` object instead.
      * Deleted the `WasmEdge_HostFunctionCreate` function. Please use the `WasmEdge_FunctionInstanceCreate` function instead.
      * Deleted the `WasmEdge_HostFunctionCreateBinding` function. Please use the `WasmEdge_FunctionInstanceCreateBinding` function instead.
      * Deleted the `WasmEdge_HostFunctionDelete` function. Please use the `WasmEdge_FunctionInstanceDelete` function instead.
      * Deleted the `WasmEdge_ImportObjectAddHostFunction` function. Please use the `WasmEdge_ImportObjectAddFunction` function instead.
    * Added the data object column in the creation function of `FunctionInstance` context.
    * Instead of the unified data object of the host functions in the same import object before, the data objects are independent in every host function now.
  * Added the WASM types contexts.
    * Added the `WasmEdge_TableTypeContext`, which is used for table instances creation.
    * Added the `WasmEdge_MemoryTypeContext`, which is used for memory instances creation.
    * Added the `WasmEdge_GlobalTypeContext`, which is used for global instances creation.
    * Added the member getter functions of the above contexts.
  * Updated the instances creation APIs.
    * Used `WasmEdge_TableTypeContext` for table instances creation.
      * Removed `WasmEdge_TableInstanceGetRefType` API.
      * Developers can use the `WasmEdge_TableInstanceGetTableType` API to get the table type instead.
    * Used `WasmEdge_MemoryTypeContext` for memory instances creation.
      * Added `WasmEdge_MemoryInstanceGetMemoryType` API.
    * Used `WasmEdge_GlobalTypeContext` for global instances creation.
      * Removed `WasmEdge_GlobalInstanceGetValType` and `WasmEdge_GlobalInstanceGetMutability` API.
      * Developers can use the `WasmEdge_GlobalInstanceGetGlobalType` API to get the global type instead.
  * Refactored for the objects' life cycle to reduce copying.
    * Developers should NOT destroy the `WasmEdge_FunctionTypeContext` objects returned from `WasmEdge_VMGetFunctionList`, `WasmEdge_VMGetFunctionType`, and `WasmEdge_VMGetFunctionTypeRegistered` functions.
    * Developers should NOT destroy the `WasmEdge_String` objects returned from `WasmEdge_StoreListFunction`, `WasmEdge_StoreListFunctionRegistered`, `WasmEdge_StoreListTable`, `WasmEdge_StoreListTableRegistered`, `WasmEdge_StoreListMemory`, `WasmEdge_StoreListMemoryRegistered`, `WasmEdge_StoreListGlobal`, `WasmEdge_StoreListGlobalRegistered`, `WasmEdge_StoreListModule`, and `WasmEdge_VMGetFunctionList` functions.
  * Renamed the `Interpreter` related APIs.
    * Replaced `WasmEdge_InterpreterContext` struct with `WasmEdge_ExecutorContext` struct.
    * Replaced `WasmEdge_InterpreterCreate` function with `WasmEdge_ExecutorCreate` function.
    * Replaced `WasmEdge_InterpreterInstantiate` function with `WasmEdge_ExecutorInstantiate` function.
    * Replaced `WasmEdge_InterpreterRegisterImport` function with `WasmEdge_ExecutorRegisterImport` function.
    * Replaced `WasmEdge_InterpreterRegisterModule` function with `WasmEdge_ExecutorRegisterModule` function.
    * Replaced `WasmEdge_InterpreterInvoke` function with `WasmEdge_ExecutorInvoke` function.
    * Replaced `WasmEdge_InterpreterInvokeRegistered` function with `WasmEdge_ExecutorInvokeRegistered` function.
    * Replaced `WasmEdge_InterpreterDelete` function with `WasmEdge_ExecutorDelete` function.
  * Refactored for statistics options
    * Renamed `WasmEdge_ConfigureCompilerSetInstructionCounting` to `WasmEdge_ConfigureStatisticsSetInstructionCounting`.
    * Renamed `WasmEdge_ConfigureCompilerSetCostMeasuring` to `WasmEdge_ConfigureStatisticsSetCostMeasuring`.
    * Renamed `WasmEdge_ConfigureCompilerSetTimeMeasuring` to `WasmEdge_ConfigureStatisticsSetTimeMeasuring`.
    * Renamed `WasmEdge_ConfigureCompilerGetInstructionCounting` to `WasmEdge_ConfigureStatisticsGetInstructionCounting`.
    * Renamed `WasmEdge_ConfigureCompilerGetCostMeasuring` to `WasmEdge_ConfigureStatisticsGetCostMeasuring`.
    * Renamed `WasmEdge_ConfigureCompilerGetTimeMeasuring` to `WasmEdge_ConfigureStatisticsGetTimeMeasuring`.
  * Simplified the WASI creation and initialization APIs.
    * Removed the `Dirs` and `DirLen` parameters in the `WasmEdge_ImportObjectCreateWASI`.
    * Removed the `Dirs` and `DirLen` parameters in the `WasmEdge_ImportObjectInitWASI`.

Features:

* Applied the old WebAssembly proposals options (All turned on by default).
  * Developers can use the `disable-import-export-mut-globals` to disable the Import/Export mutable globals proposal in `wasmedge` and `wasmedgec`.
  * Developers can use the `disable-non-trap-float-to-int` to disable the Non-trapping float-to-int conversions proposal in `wasmedge` and `wasmedgec`.
  * Developers can use the `disable-sign-extension-operators` to disable the Sign-extension operators proposal in `wasmedge` and `wasmedgec`.
  * Developers can use the `disable-multi-value` to disable the Multi-value proposal in `wasmedge` and `wasmedgec`.
* New WasmEdge C API for listing imports and exports from AST module contexts.
  * Developers can query the `ImportTypeContext` and `ExportTypeContext` from the `ASTModuleContext`.
  * New object `WasmEdge_ImportTypeContext`.
  * New object `WasmEdge_ExportTypeContext`.
  * New AST module context functions to query the import and export types.
    * `WasmEdge_ASTModuleListImportsLength` function can query the imports list length from an AST module context.
    * `WasmEdge_ASTModuleListExportsLength` function can query the exports list length from an AST module context.
    * `WasmEdge_ASTModuleListImports` function can list all import types of an AST module context.
    * `WasmEdge_ASTModuleListExports` function can list all export types of an AST module context.
  * New import type context functions to query data.
    * `WasmEdge_ImportTypeGetExternalType` function can get the external type of an import type context.
    * `WasmEdge_ImportTypeGetModuleName` function can get the import module name.
    * `WasmEdge_ImportTypeGetExternalName` function can get the import external name.
    * `WasmEdge_ImportTypeGetFunctionType` function can get the function type of an import type context.
    * `WasmEdge_ImportTypeGetTableType` function can get the table type of an import type context.
    * `WasmEdge_ImportTypeGetMemoryType` function can get the memory type of an import type context.
    * `WasmEdge_ImportTypeGetGlobalType` function can get the global type of an import type context.
  * New export type context functions to query data.
    * `WasmEdge_ExportTypeGetExternalType` function can get the external type of an export type context.
    * `WasmEdge_ExportTypeGetExternalName` function can get the export external name.
    * `WasmEdge_ExportTypeGetFunctionType` function can get the function type of an export type context.
    * `WasmEdge_ExportTypeGetTableType` function can get the table type of an export type context.
    * `WasmEdge_ExportTypeGetMemoryType` function can get the memory type of an export type context.
    * `WasmEdge_ExportTypeGetGlobalType` function can get the global type of an export type context.
  * For more details of the usages of imports and exports, please refer to the [C API documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md).
* Exported the WasmEdge C API for getting exit code from WASI.
  * `WasmEdge_ImportObjectWASIGetExitCode` function can get the exit code from WASI after execution.
* Exported the WasmEdge C API for AOT compiler related configurations.
  * `WasmEdge_ConfigureCompilerSetOutputFormat` function can set the AOT compiler output format.
  * `WasmEdge_ConfigureCompilerGetOutputFormat` function can get the AOT compiler output format.
  * `WasmEdge_ConfigureCompilerSetGenericBinary` function can set the option of AOT compiler generic binary output.
  * `WasmEdge_ConfigureCompilerIsGenericBinary` function can get the option of AOT compiler generic binary output.
* Provided install and uninstall script for installing/uninstalling  WasmEdge on linux(amd64 and aarch64) and macos(amd64 and arm64).
* Supported compiling WebAssembly into a new WebAssembly file with a packed binary section.
* Supported the automatically pre-open mapping with the path name in WASI.

Fixed issues:

* Refined the WasmEdge C API behaviors.
  * Handle the edge cases of `WasmEdge_String` creation.
* Fixed the instruction iteration exception in interpreter mode.
  * Forcely added the capacity of instruction vector to prevent from connection of instruction vectors in different function instances.
* Fixed the loader of AOT mode WASM.
  * Checked the file header instead of file name extension when loading from file.
  * Showed the error message when loading AOT compiled WASM from buffer. For AOT mode, please use the universal WASM binary.
  * Fixed the zero address used in AOT mode in load manager.
  * Fixed the loading failed for the AOT compiled WASM without intrinsics table.
* Fixed the `VM` creation issue.
  * Added the loss of intrinsics table setting when creating a VM instance.
* Fixed wasi-socket issues.
  * Support wasi-socket on MacOS.
  * Remove the port parameter from `sock_accept`.

Refactor:

* Refined headers inclusion in all files.
* Refactor the common headers.
  * Removed the unnecessary `genNullRef()`.
  * Merged the building environment-related definitions into `common`.
  * Merged the `common/values.h` into `common/types.h`.
  * Separated all enumeration definitions.
* Refactored the AST nodes.
  * Simplified the AST nodes definitions into header-only classes.
  * Moved the binary loading functions into `loader`.
  * Updated the `validator`, `executor`, `runtime`, `api`, and `vm` for the AST node changes.
* Refactored the runtime objects.
  * Used `AST::FunctionType`, `AST::TableType`, `AST::MemoryType`, and `AST::GlobalType` for instance creation and member handling.
  * Removed `Runtime::Instance::FType` and used `AST::FunctionType` instead.
  * Added routines to push function instances into import objects.
  * Removed the exported map getter in `StoreManager`. Used the getter from `ModuleInstance` instead.
  * Added the module name mapping in `StoreManager`.
* Refactored the VM class.
  * Returned the reference to function type instead of copying when getting the function list.
  * Returned the vector of return value and value type pair when execution.
* Updated the include path for rust binding due to the API headers refactoring.

Documentations:

* Updated the `wasmedge` commands in the [Run](https://github.com/WasmEdge/WasmEdge/blob/master/docs/run.md) and [SIMD documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/simd.md)
* Updated the examples in the [C API documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md).
* Updated the examples in the [host function documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md).
* Updated the examples in the [external reference documentation](https://github.com/WasmEdge/WasmEdge/blob/master/docs/externref.md).

Bindings:

* Move rust crate from root path to `bindings/rust`.

Tests:

* Updated the core test suite to the newest WASM spec.
* Updated and fixed the value comparison in core tests.
* Added `ErrInfo` unit tests.
* Added instruction tests for turning on/off the old proposals.
* Moved and updated the `AST` unit tests into `loader`.
* Moved and updated the `Interpreter` tests into `Executor` folder.
* Added the unit tests for new APIs.
* Applied the WasmEdge C API in the `ExternRef` tests.

Misc:

* Enabled GitHub CodeSpaces
* Added `assuming` for `assert` checking to help compiler to generate better codes.

Thank all the contributors that made this release possible!

2021, actly, alabulei1, Alex, Antonio Yang, Ashutosh Sharma, Avinal Kumar, blackanger, Chojan Shang, dm4, eee4017, fossabot, hydai, Jayita Pramanik, Kenvi Zhu, luishsu, LuisHsu, MaazKhan711635, Michael Yuan, MileyFu, Nick Hynes, O3Ol, Peter Chang, robnanarivo, Shen-Ta Hsieh, Shreyas Atre, slidoooor, Sylveon, Timothy McCallum, Vikas S Shetty, vincent, Xin Liu, Yi Huang, yiying, YiYing He, Yona, Yukang, 牟展佑

If you want to build from source, please use WasmEdge-0.9.0-src.tar.gz instead of the zip or tarball provided by GitHub directly.

### 0.8.2 (2021-08-25)

Features:

* WASI:
  * Supported WASI on macOS(Intel & M1).
  * Supported WASI on Windows 10.
  * Supported WASI Socket functions on Linux.
* C API:
  * Supported 32-bit environment.
  * Added the static library target `libwasmedge_c.a` (`OFF` by default).
  * Added the `ErrCode` to C declarations.
  * Added the API about converting `WasmEdge_String` to C string.
  * Added the API to get data pointer from the `WasmEdge_MemoryInstanceContext`.
* AOT:
  * Added `--generic-binary` to generate generic binaries and disable using host features.
* Multi platforms:
  * Enabled Ubuntu 20.04 x86\_64 build.
  * Enabled Ubuntu 21.04 x86\_64 build.
  * Enabled manylinux2014 aarch64 build.
  * Enabled Ubuntu 21.04 arm32 build.
* Rust supports:
  * Added the `wasmedge-sys` and `wasmedge-rs` crates.
  * Added the wrapper types to rust.
* Removed binfmt support.

Fixed issues:

* Ensured every platform defines is defined.
* Disabled blake3 AVX512 support on old platforms.
* Avoided vector ternary operator in AOT, which is unsupported by clang on mac.
* The preopen should be `--dir guest_path:host_path`.
* Fixed usused variables error in API libraries when AOT build is disabled.
* Fixed the WASI function signature error.
  * `wasi_snapshot_preview1::path_read_link`
    * Fixed the signature error with the lost read size output.
    * Added the `Out` comments for parameters with receiving outputs.
  * `wasi_snapshot_preview1::path_filestat_set_times`
    * Corrected the time signature to the `u64`.

Misc:

* Changed all CMake global properties to target specified properties.
  * Added namespace to all cmake options.
* Added the CMake option `WASMEDGE_FORCE_DISABLE_LTO` to forcibly disable link time optimization (`OFF` by default).
  * WasmEdge project enables LTO by default in Release/RelWithDeb build. If you would like to disable the LTO forcibly, please turn on the `WASMEDGE_FORCE_DISABLE_LTO` option.
* Installed `dpkg-dev` in docker images to enable `dpkg-shlibdeps` when creating the deb release.

Refactor:

* Refactored the WASI VFS architecture.
* Simplified the memory indexing in validator.
* Renamed the file names in interpreter.
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
  * Added intrinsics table for dynamic linking when running a compiled wasm.
* Merged the program counter into `stack manager`.
* Added back the `OpCode::End` instruction.
* Refactored the validator workflow of checking expressions.
* Used `std::bitset` for VM configuration.
* Used `std::array` for cost table storage.
* Combined `include/support` into `include/common`.
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
