### 0.15.0-alpha.1 (2025-03-10)

Features:

* [Component Model]
  * Update import name rule (spec changes)
  * Partial resource support
  * Support more WASM interface types
* [Executor]
  * Save thread-local variables before executing the nested VM call (#3969)
  * Merge `prepare` and `SavedThreadLocal`
  * Re-order and lint the instructions. (#4040)
* [Runtime]
  * coredump: Implement WASM coredump feature when the trap occurs (#3860)
  * coredump: Fix type conversion issue (#3948)
* [Installer]
  * v1: Show error message when it is triggered on Windows (#3925)
  * v1: Update the assets' URLs for the 0.13.5 and 0.14.0 ggmlbn (#3895)
  * v2: Enhance the message for -c/--ggmlcuda (#3888)
  * v2: Provide a better output for the usage of the --version option (#3778)
  * v2: Update the assets' URLs for the 0.14.1 ggmlbn (#3891)
  * v2: Use 0.14.1 by default (#3848)
  * v2: Enhance the troubleshooting message (#3992)
* [LLVM] Support LLVM 19 (#3809)
* [Log]
  * Support all log levels in the API and add user callback support (#3789)
  * Allow unregistering the log callback function. (#3915)
* [Loader] Support more proposals in Serializer:
  * Add Exception Handling proposal (#3876)
  * Add tests for relaxed-SIMD instructions (#3800)
  * Add Function References Instructions and their tests (#3759)
  * Add GC proposal types and instructions with corresponding tests (#3821)
  * Add tests for Typed Function References proposal's types (#3748)
  * Add tests for composite and sub types (#3865)
  * Fix nested component load (#3938)
* [Validator] Add detection of missing atomic instructions for memory alignment. (#3987)
* [Validator] Non-imported global is valid in const expression validation with function-reference proposal.
* [Validator] Fix the error code of call_indirection instruction validation.
* [Plugin] Image: Use `stb_image` to replace libpng and libjpeg.
* [Plugin] Stable Diffusion:
  * Bump to dcf91 (#3950)
  * Add more tests (#3843)
  * Add option for enabling OpenMP (#3810)
  * Fix the build failure on macOS Metal
  * Fix reuse context segmentation fault (#3824)
  * Support clip\_g option
* [Plugin] New proposals: Implement wasi-poll poll-oneoff
* [Plugin] wasm\_bpf: Fix error of poisoned identifier
* [WASI-NN]
  * Add finalize\_execution\_context function  (#3917)
  * Bump plugin version to 0.1.14
  * Fix incorrect function dispatch. (#3979)
  * Refactor dependency CMake
  * Refactor the graph and context management mechanism for all backends.
  * Apply WASMEDGE\_WASI\_NN\_VERSION to the plugin's version (#4017)
* [WASI-NN] Torch backend:
  * Update PyTorch version (#3818, #3901)
  * Support Torch AOTI
* [WASI-NN] llama.cpp backend:
  * Bump llama.cpp to b4818
  * Do not append SEP when getting embeddings
  * Fix accessing freed data after unload. (#3785)
  * Fix `fmt::format` error in embedding (#3779)
  * Fix reloading llama context
  * Reload llama context if embedding status changes
  * Support new vision models: Qwen2VL
  * Support new options: seed, split-mode, and warmup
  * Disable warmup by default to match previous behavior
  * Use the cached image embed instead to reduce costs (#3964)
  * Support text-to-speech
  * Add TTS speaker profile support (#4020)
  * Check the projection model before loading
* [WASI-NN] whisper.cpp backend:
  * Fix the token timestamp option
  * Move the whisper.cpp linking out of the header.
  * Add more options: no-timestamp and audio-ctx (#3931)
  * Support Metal on macOS
  * Support CUDA on Linux
  * Fix the modified test file path
* [WASI-NN] mlx backend:
  * Support `mlx` backend for the WASI-NN plugin
* [WASI-NN] ChatTTS backend:
  * Fix GIL problem and do not call Py\_Finalize (#3940)
* [WASI-NN] piper backend:
  * Extend the json\_input functionality to allow setting various parameters at runtime (#3825)
  * Fix arguments for target linking and including in piper patch (#3798)
* [WASI-NN] openvino backend:
  * Update to 2025.0.0 (#4016)
  * Add dependency installer for openvino-genai (#4032)
* [Debug] Add stack trace while an error occurred (#3967)

Fixed issues:

* [Compiler] Use pointer to pointer of data of memory instance in execution context for passing into AOT mode. (#4052)
* [Loader] Fix the wrong default elem type.
* [Loader] Fix the error code and detection of malformed memory operation flags.
* [Loader] Fix the element segment format in wasm-1.0.
* [Loader] Sections in module should in ordered.
* [Loader] The multi-value checking should in validation phase.
* [LLVM] Check unsupported proposals in configure (#3981)
* [LLVM] Ignore C++17 deprecated warning in <ciso646> header
* [WASI] win: Use `ReadFile/WriteFile` instead of `ReadFileEx/WriteFileEx` (#3870)
* [WASI-crypto] Fix: secretkey\_export on RSA with ENCODING\_PKCS8 (#3963)
* [VM] Support VM.getFunctionList for the component instance

Tests:

* [Component Model] Provide more CM tests for loading, validating, and executing phases
* [Plugin] wasmedge\_zlib: Fix `-Wformat-truncation` warning
* [Test] Fix the spec test tag to 0.15.0.
* [Test] Switch to the new spectest repo.

Misc:

* [CMake] Set the minimum required version of CMake to 3.18
* [CMake] Link `libfmt` to the target in the static build (#3909)
* [CMake] Apply the WasmEdge component in the CPack. (#3690)
* [CMake] Add `zstd` for all platforms with LLVM 16+
* [CMake] Add option to disable cxx11-abi and turn off cxx11-abi on manylinux.
* [CMake] Link libfmt to the target in the static build (#3909)
* [CMake] Remove post-build copy for stable-diffusion
* [CMake] Enable Metal for stable-diffusion by default
* [Deps] Remove patch for fmt, add support for fmt version 9, fix #3782
* [Deps] Support fmt v11.1.2
* [Docs] Update supported version to 0.14.1 (#3913)
* [Docs] Update the GOVERNANCE, CONTRIBUTOR\_LADDER, and GOVERNANCE (#3927)
* [Docs] Rust 1.84 uses wasm32-wasip1 instead of wasm32-wasi (#3965)
* [Internal] Add fast string hash for speed
* [Misc] Add warning if the ABI may be incompatible
* [Misc] Use `string_view` literal suffix for spdlog functions
* [Misc] Add macro for returning `unexpected` error

CI:

* [CI] Add groups and prefixes for dependabot (#4007)
* [CI] Bump advanced-security/sbom-generator-action from 0.0.1 to 0.0.2 (#3970)
* [CI] Bump `clang-format` version from 15 to 18
* [CI] Bump docker/bake-action from 5 to 6 (#3971)
* [CI] Fix version for alpine static and debian static
* [CI] Windows: Disable progress bar when installing via choco
* [CI] Windows: Use the preinstalled CMake as a workaround
* [CI] Update for SD\_Metal being enabled by default
* [CI] Test WasmEdge (Core) on Ubuntu 20.04 (x86\_64) (#4050)
* [CI] dependabot: Bump uraimo/run-on-arch-action from 2 to 3 (#4042)
* [Linux] Deprecate all CI workflows related to the manylinux2014 image
* [nix] flake update and use LLVM 18 (#3839)
* [macOS] The minimum macOS version is set to macos-13
* [Misc] Fix build failure in Windows Server 2022 CI (#3899)

Known issues:

* Universal WASM format failed on macOS platforms.
  * In the current status, the universal WASM format output of the AOT compiler with the `O1` or higher optimizations on macOS platforms will cause a bus error during execution.
  * We are trying to fix this issue. As a workaround, please use the `--optimize=0` to set the compiler optimization level to `O0` in the `wasmedgec` CLI.

Thank all the contributors who made this release possible!

Deveshi Dwivedi, Dmytrol, Gianpaolo Macario, Han-Wen Tsao, LFsWang, Lîm Tsú-thuàn, Maxim Evtush, Oleg, PeterD1524, Ruslan Tushov, Shen-Ta Hsieh, Sylveon, Tenderyi, Vladimir Cherkasov, Yi Huang, YiYing He, abdelkoddous LHAJOUJI, alabulei1, dependabot[bot], dm4, elhewaty, fancybody, grorge, hydai, junxiangMu, kilavvy, omahs, vincent

If you want to build from source, please use WasmEdge-0.15.0-alpha.1-src.tar.gz instead of the zip or tarball provided by GitHub directly.
