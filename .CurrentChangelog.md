### 0.15.0-alpha.3 (2025-06-16)

Features:

* [Compiler]
  * feat(compiler): gc proposal - support array instructions
  * feat(compiler): gc proposal - support br instructions
  * feat(compiler): gc proposal - support ref instructions
  * feat(compiler): gc proposal - support struct instructions
  * feat(compiler): support GC types in AOT compiler
  * refactor(compiler): refine the value load/store of allocated array
* [Component Model]
  * Update import name rule (spec changes)
  * Partial resource support
  * Support more WASM interface types
  * Validate instantiate imports and arguments (#4062)
  * Validate CoreInstance Section, Improve Context, add tests for nested components (#4077)
  * Add validation for sortidx (#4082)
  * refactor(ast,loader): unify the coding style of canonical in component model
  * refactor(ast): adjust the AST nodes for component model and add implementation comments
  * refactor(ast): rename the basic types of component model
  * refactor(loader,ast): adjust the AST nodes for component model (#4123)
  * refactor(loader): fix the error info and re-order the functions of component model loader
  * feat(component-model/validation): validate alias section (#4094)
* [Executor]
  * Save thread-local variables before executing the nested VM call (#3969)
  * Merge `prepare` and `SavedThreadLocal`
  * Re-order and lint the instructions. (#4040)
  * refactor(executor): move the packVal/unpackVal functions
  * refactor(executor): refine the logging codes
  * refactor(executor,compiler): reduce the redundant code between aot proxy and interpreter
* [Runtime]
  * coredump: Implement WASM coredump feature when the trap occurs (#3860)
  * coredump: Fix type conversion issue (#3948)
  * feat(ast,runtime): apply some resource type and component hostfunc conversion
* [Installer]
  * v1: Show error message when it is triggered on Windows (#3925)
  * v1: Update the assets' URLs for the 0.13.5 and 0.14.0 ggmlbn (#3895)
  * v1: add plugin wasi\_nn-tensorflowlite for mac arm from 0.14.1 (#4122)
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
  * unify the coding style of EXPECTED\_TRY (#4136)
  * use error instead of info for error logging
* [Validator] Add detection of missing atomic instructions for memory alignment. (#3987)
* [Validator] Non-imported global is valid in const expression validation with function-reference proposal.
* [Validator] Fix the error code of call\_indirection instruction validation.
* [Plugin] Image: Use `stb_image` to replace libpng and libjpeg.
  * Bump to f056911 (#4059)
  * chore(plugin): wasmedge-images: bump to stb\_image\_resize2 2.14 (#4141)
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
  * Bump plugin version to 0.1.20
  * Fix incorrect function dispatch. (#3979)
  * Refactor dependency CMake
  * Refactor the graph and context management mechanism for all backends.
  * Apply WASMEDGE\_WASI\_NN\_VERSION to the plugin's version (#4017)
  * Update tensor type index (#4069)
* [WASI-NN] Torch backend:
  * Update PyTorch version (#3818, #3901)
  * Support Torch AOTI
* [WASI-NN] llama.cpp backend:
  * Bump llama.cpp to b5640 (#4164)
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
  * use the new libmtmd for multimodal models (#4112)
  * clear the context before mtmd evaluation (#4143)
  * fix n\_ubatch assignment (#4163)
* [WASI-NN] whisper.cpp backend:
  * Fix the token timestamp option
  * Move the whisper.cpp linking out of the header.
  * Add more options: no-timestamp and audio-ctx (#3931)
  * Support Metal on macOS
  * Support CUDA on Linux
  * Fix the modified test file path
* [WASI-NN] mlx backend:
  * Support `mlx` backend for the WASI-NN plugin
  * support gemma3 for mlx plugin (#4085)
  * support quantized gemma3 model (#4099)
* [WASI-NN] ChatTTS backend:
  * Fix GIL problem and do not call Py\_Finalize (#3940)
  * Update compute function to be compatible with v0.2.1
* [WASI-NN] piper backend:
  * Extend the json\_input functionality to allow setting various parameters at runtime (#3825)
  * Fix arguments for target linking and including in piper patch (#3798)
* [WASI-NN] openvino backend:
  * Update to 2025.0.0 (#4016)
  * Add openvino-genai support (#4034)
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
* [LLVM] `LLVMX86_MMXTypeKind` moved in LLVM-20 (#4064)
* [LLVM] Correctness the bit cast of relaxed-simd instructions
* [WASI] win: Use `ReadFile/WriteFile` instead of `ReadFileEx/WriteFileEx` (#3870)
* [WASI-crypto] Fix: secretkey\_export on RSA with ENCODING\_PKCS8 (#3963)
* [VM] Support VM.getFunctionList for the component instance
* fix: add Python 3.9 setup for OpenWRT build dependencies (#4140)
* fix(executor): fix the copy of valtype in the ref\_test instructions
* fix(executor): pick the first nan value for min and max instructions
* fix(loader,validator,test): shared memory must-have-max case should be in validation phase.

Tests:

* [Component Model] Provide more CM tests for loading, validating, and executing phases
* [Plugin] wasmedge\_zlib: Fix `-Wformat-truncation` warning
* [Test] Fix the spec test tag to 0.15.0.
* [Test] Switch to the new spectest repo.
* test(spec): turn on the AOT/JIT tests for GC proposal
* feat(test): the trap cases should fit their wasm phase

Misc:

* [CMake] Set the minimum required version of CMake to 3.18
* [CMake] Link `libfmt` to the target in the static build (#3909)
* [CMake] Apply the WasmEdge component in the CPack. (#3690)
* [CMake] Add `zstd` for all platforms with LLVM 16+
* [CMake] Add option to disable cxx11-abi and turn off cxx11-abi on manylinux.
* [CMake] Link libfmt to the target in the static build (#3909)
* [CMake] Remove post-build copy for stable-diffusion
* [CMake] Enable Metal for stable-diffusion by default
* chore(cmake): false the shallow mode for stb\_image (#4153)
* chore(cmake): separate the version between wasi\_nn and wasi\_nn\_rpc
* chore(docker): bump the llvm from 12 to 18 on ubuntu 20.04
* [Deps] Remove patch for fmt, add support for fmt version 9, fix #3782
* [Deps] Support fmt v11.1.2
* [Docs] Update supported version to 0.14.1 (#3913)
* [Docs] Update the GOVERNANCE, CONTRIBUTOR\_LADDER, and GOVERNANCE (#3927)
* [Docs] Rust 1.84 uses wasm32-wasip1 instead of wasm32-wasi (#3965)
* [Internal] Add fast string hash for speed
* [Misc] Add warning if the ABI may be incompatible
* [Misc] Use `string_view` literal suffix for spdlog functions
* [Misc] Add macro for returning `unexpected` error
* refactor(example): migrate zlib example to use WIT (#4124)
* feat(examples): wasi-cryptography-signature: add proper error handling (#4117)

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
* [CI] dependabot: Bump cachix/install-nix-action from 30 to 31 (#4061)
* [CI] enable commitlint for all commits
* ci: add postfix to avoid naming conflicts when uploading the assets
* ci: reduce the name for a better log on workflows
* ci(IWYU): add patch to avoid fedora failure (#4146)
* ci(runner): use github hosted arm runner instead of ours
* [Linux] Deprecate all CI workflows related to the manylinux2014 image
* [nix] flake update and use LLVM 18 (#3839)
* [macOS] The minimum macOS version is set to macos-13
* [Misc] Fix build failure in Windows Server 2022 CI (#3899)

Thank all the contributors who made this release possible!

abdelkoddous LHAJOUJI, alabulei1, dependabot[bot], Deveshi Dwivedi, dm4, Dmytrol, elhewaty, Fahd Ashour, fancybody, Gianpaolo Macario, grorge, Han-Wen Tsao, hydai, Jacob, junxiangMu, Karan, khongtrunght, kilavvy, LFsWang, Lîm Tsú-thuàn, Maxim Evtush, Michael Yuan, Oleg, omahs, operagxsasha, PeterD1524, Ruslan Tushov, Shen-Ta Hsieh, sridamul, Sridhar Sivakumar, Sylveon, Tenderyi, varunrmallya, vincent, Vladimir Cherkasov, Yi Huang, Yi-Ying He

If you want to build from source, please use WasmEdge-0.15.0-alpha.3-src.tar.gz instead of the zip or tarball provided by GitHub directly.
