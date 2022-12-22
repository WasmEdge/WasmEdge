//! Defines WasmEdge Config struct.

use crate::{error::WasmEdgeError, ffi, WasmEdgeResult};
#[cfg(feature = "aot")]
use wasmedge_types::{CompilerOptimizationLevel, CompilerOutputFormat};

/// Defines Config struct used to check/set the configuration options.
///
/// [Config](crate::Config) manages the configuration options, which are used to initiate WasmEdge [Vm](crate::Vm), [Loader](crate::Loader), [Validator](crate::Validator), [Executor](crate::Executor), and [Compiler](crate::Compiler).
///
/// The configuration options are categorized into the following four groups:
///
/// - **WebAssembly Proposals**
///
///     This group of options are used to turn on/off the WebAssembly proposals. They are effective to any WasmEdge
///     context created with [Config](crate::Config).
///     
///     - `MultiMemories` enables to use multiple memories within a single Wasm module.
///
///       Also see [Multiple Memories for Wasm](https://github.com/WebAssembly/multi-memory/blob/main/proposals/multi-memory/Overview.md)
///     
///     - `ImportExportMutGlobals` supports mutable imported and exported globals.
///
///       Also see [Import/Export Mutable Globals Proposal](https://github.com/WebAssembly/mutable-global/blob/master/proposals/mutable-global/Overview.md#importexport-mutable-globals).
///
///     - `NonTrapFloatToIntConversions` supports the non-trapping float-to-int conversion.
///
///       Also see [Non-trapping Float-to-int Conversions Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/nontrapping-float-to-int-conversion/Overview.md).
///
///     - `SignExtensionOperators` supports new integer instructions for sign-extending 8-bit, 16-bit, and 32-bit values.
///     
///       Also see [Sign-extension Operators Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/sign-extension-ops/Overview.md).
///
///     - `MultiValue` supports functions and instructions with multiple return values, and blocks with inputs.
///     
///       Also see [Multi-value Extension](https://github.com/WebAssembly/spec/blob/main/proposals/multi-value/Overview.md).
///
///     - `BulkMemoryOperations` supports bulk memory operations.
///
///       Also see [Bulk Memory Operations Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/bulk-memory-operations/Overview.md#motivation-for-bulk-memory-operations).
///
///     - `ReferenceTypes` supports reference types.
///
///       Also see [Reference Types Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/reference-types/Overview.md).
///
///     - `SIMD` supports 128-bit packed SIMD extension to WebAssembly.
///
///       Also see [SIMD Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/simd/SIMD.md).
///  
///     - `TailCall` supports tail call optimization.
///
///       Also see [Tail Call Proposal](https://github.com/WebAssembly/tail-call/blob/master/proposals/tail-call/Overview.md).
///
///     - `Annotations` supports annotations in WASM text format.
///
///       Also see [Annotations Proposal](https://github.com/WebAssembly/annotations/blob/master/proposals/annotations/Overview.md).
///
///     - `Memory64` supports 64-bit memory indexes.
///
///       Also see [Memory64 Proposal](https://github.com/WebAssembly/memory64/blob/main/proposals/memory64/Overview.md).
///
///     - `Threads` supports threading feature.
///
///       Also see [Threading Proposal](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md).
///
///     - `ExceptionHandling` supports exception handling.
///     
///       Also see [Exception Handling Proposal](https://github.com/WebAssembly/exception-handling/blob/main/proposals/exception-handling/Exceptions.md).
///
///     - `FunctionReferences` supports typed function references for WebAssembly.
///
///       Also see [Function References Proposal](https://github.com/WebAssembly/function-references/blob/master/proposals/function-references/Overview.md).
///
/// - **Host Registrations**
///     - `Wasi` turns on the `WASI` support in [Vm](crate::Vm).
///
///     - `WasmEdgeProcess` turns on the `wasmedge_process` support in [Vm](crate::Vm).
///     
///     The two options are only effective to [Vm](crate::Vm).
///
/// - **Memory Management**
///     - `maximum_memory_page` limits the page size of [Memory](crate::Memory). This option is only effective to
///       [Executor](crate::Executor) and [Vm](crate::Vm).
///
/// - **AOT Compilation**
///
///     The AOT compiler options configure the behavior about optimization level, output format, dump IR,
///     and generic binary.
///
///     - Compiler Optimization Levels
///         - `O0` performs as many optimizations as possible.
///         
///         - `O1` optimizes quickly without destroying debuggability.
///
///         - `02` optimizes for fast execution as much as possible without triggering significant incremental
///                compile time or code size growth.
///
///         - `O3` optimizes for fast execution as much as possible.
///
///         - `Os` optimizes for small code size as much as possible without triggering significant incremental
///                compile time or execution time slowdowns.
///
///         - `Oz` optimizes for small code size as much as possible.
///
///     - Compiler Output Formats
///         - `Native` specifies the output format is native dynamic library (`*.wasm.so`).
///
///         - `Wasm` specifies the output format is WebAssembly with AOT compiled codes in custom section (`*.wasm`).
///     
///     - `dump_ir` determines if AOT compiler generates IR or not.
///
///     - `generic_binary` determines if AOT compiler generates the generic binary or not.
///     
///     - `interruptible` determines if AOT compiler generates interruptible binary or not.
///     
///     The configuration options above are only effective to [Compiler](crate::Compiler).
///
/// - **Runtime Statistics**
///     - `instr_counting` determines if measuring the count of instructions when running a compiled or pure WASM.
///
///     - `cost_measuring` determines if measuring the instruction costs when running a compiled or pure WASM.
///
///     - `time_measuring` determines if measuring the running time when running a compiled or pure WASM.
///
/// API users can first set the options of interest, such as those related to the WebAssembly proposals,
/// host registrations, AOT compiler options, and etc., then apply the configuration
/// to create other WasmEdge runtime structs.
#[derive(Debug, Clone)]
pub struct Config {
    pub(crate) inner: std::sync::Arc<InnerConfig>,
}
impl Drop for Config {
    fn drop(&mut self) {
        if std::sync::Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ConfigureDelete(self.inner.0) };
        }
    }
}
impl Config {
    /// Creates a new [Config](crate::Config).
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_ConfigureCreate() };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::ConfigCreate)),
            false => Ok(Self {
                inner: std::sync::Arc::new(InnerConfig(ctx)),
            }),
        }
    }

    /// Enables or disables host registration wasi. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn wasi(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_Wasi,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_Wasi,
                )
            }
        }
    }

    /// Checks if host registration wasi turns on or not.
    pub fn wasi_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_Wasi,
            )
        }
    }

    /// Enables or disables host registration WasmEdge process. By default, the option is disabled.
    ///
    /// Notice that to enable the `wasmege_process` option in [Vm](crate::Vm), it MUST be guaranteed that the `wasmedge_process` plugins are loaded first. If not, use the [load_plugin_from_default_paths](crate::utils::load_plugin_from_default_paths) function to load the relevant plugins from the default paths
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            }
        }
    }

    /// Checks if host registration wasmedge process turns on or not.
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasmEdge_Process,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn(&mut self, enable: bool) {
        unsafe {
            if enable {
                // enable wasi option
                self.wasi(enable);

                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiNN,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiNN,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiNN,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Common,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Common,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Common,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Symmetric,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Symmetric,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Symmetric,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Kx,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Kx,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Kx,
            )
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Signatures,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    ffi::WasmEdge_HostRegistration_WasiCrypto_Signatures,
                )
            }
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Signatures,
            )
        }
    }

    /// Sets the maximum number of the memory pages available.
    ///
    /// # Argument
    ///
    /// * `count` - The page count (64KB per page).
    pub fn set_max_memory_pages(&mut self, count: u32) {
        unsafe { ffi::WasmEdge_ConfigureSetMaxMemoryPage(self.inner.0, count) }
    }

    /// Returns the number of the memory pages available.
    pub fn get_max_memory_pages(&self) -> u32 {
        unsafe { ffi::WasmEdge_ConfigureGetMaxMemoryPage(self.inner.0) }
    }

    /// Enables or disables the ImportExportMutGlobals option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn mutable_globals(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            }
        }
    }

    /// Checks if the ImportExportMutGlobals option turns on or not.
    pub fn mutable_globals_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_ImportExportMutGlobals,
            )
        }
    }

    /// Enables or disables the NonTrapFloatToIntConversions option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn non_trap_conversions(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            }
        }
    }

    /// Checks if the NonTrapFloatToIntConversions option turns on or not.
    pub fn non_trap_conversions_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_NonTrapFloatToIntConversions,
            )
        }
    }

    /// Enables or disables the SignExtensionOperators option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn sign_extension_operators(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_SignExtensionOperators,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_SignExtensionOperators,
                )
            }
        }
    }

    /// Checks if the SignExtensionOperators option turns on or not.
    pub fn sign_extension_operators_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_SignExtensionOperators,
            )
        }
    }

    /// Enables or disables the MultiValue option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn multi_value(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_MultiValue)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_MultiValue,
                )
            }
        }
    }

    /// Checks if the MultiValue option turns on or not.
    pub fn multi_value_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_MultiValue)
        }
    }

    /// Enables or disables the BulkMemoryOperations option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn bulk_memory_operations(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_BulkMemoryOperations,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_BulkMemoryOperations,
                )
            }
        }
    }

    /// Checks if the BulkMemoryOperations option turns on or not.
    pub fn bulk_memory_operations_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_BulkMemoryOperations,
            )
        }
    }

    /// Enables or disables the ReferenceTypes option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn reference_types(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ReferenceTypes,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ReferenceTypes,
                )
            }
        }
    }

    /// Checks if the ReferenceTypes option turns on or not.
    pub fn reference_types_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_ReferenceTypes)
        }
    }

    /// Enables or disables the SIMD option. By default, the option is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn simd(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_SIMD)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(self.inner.0, ffi::WasmEdge_Proposal_SIMD)
            }
        }
    }

    /// Checks if the SIMD option turns on or not.
    pub fn simd_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_SIMD) }
    }

    /// Enables or disables the TailCall option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn tail_call(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_TailCall)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(self.inner.0, ffi::WasmEdge_Proposal_TailCall)
            }
        }
    }

    /// Checks if the TailCall option turns on or not.
    pub fn tail_call_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_TailCall) }
    }

    /// Enables or disables the Annotations option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn annotations(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_Annotations)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_Annotations,
                )
            }
        }
    }

    /// Checks if the Annotations option turns on or not.
    pub fn annotations_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_Annotations)
        }
    }

    /// Enables or disables the Memory64 option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn memory64(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_Memory64)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(self.inner.0, ffi::WasmEdge_Proposal_Memory64)
            }
        }
    }

    /// Checks if the Memory64 option turns on or not.
    pub fn memory64_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_Memory64) }
    }

    /// Enables or disables the Threads option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn threads(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(self.inner.0, ffi::WasmEdge_Proposal_Threads)
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(self.inner.0, ffi::WasmEdge_Proposal_Threads)
            }
        }
    }

    /// Checks if the Threads option turns on or not.
    pub fn threads_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_Threads) }
    }

    /// Enables or disables the ExceptionHandling option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn exception_handling(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ExceptionHandling,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_ExceptionHandling,
                )
            }
        }
    }

    /// Checks if the ExceptionHandling option turns on or not.
    pub fn exception_handling_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_ExceptionHandling,
            )
        }
    }

    /// Enables or disables the FunctionReferences option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn function_references(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_FunctionReferences,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_FunctionReferences,
                )
            }
        }
    }

    /// Checks if the FunctionReferences option turns on or not.
    pub fn function_references_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                ffi::WasmEdge_Proposal_FunctionReferences,
            )
        }
    }

    /// Enables or disables the MultiMemories option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn multi_memories(&mut self, enable: bool) {
        unsafe {
            if enable {
                ffi::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_MultiMemories,
                )
            } else {
                ffi::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    ffi::WasmEdge_Proposal_MultiMemories,
                )
            }
        }
    }

    /// Checks if the MultiMemories option turns on or not.
    pub fn multi_memories_enabled(&self) -> bool {
        unsafe {
            ffi::WasmEdge_ConfigureHasProposal(self.inner.0, ffi::WasmEdge_Proposal_MultiMemories)
        }
    }

    /// Enables or disables the `ForceInterpreter` option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn interpreter_mode(&mut self, enable: bool) {
        unsafe { ffi::WasmEdge_ConfigureSetForceInterpreter(self.inner.0, enable) }
    }

    /// Checks if the `ForceInterpreter` option turns on or not.
    pub fn interpreter_mode_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureIsForceInterpreter(self.inner.0) }
    }

    // For AOT compiler

    /// Sets the optimization level of AOT compiler. By default, the optimization level is `O3`.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    ///
    /// # Argument
    ///
    /// * `opt_level` - The optimization level of AOT compiler.
    #[cfg(feature = "aot")]
    pub fn set_aot_optimization_level(&mut self, opt_level: CompilerOptimizationLevel) {
        unsafe {
            ffi::WasmEdge_ConfigureCompilerSetOptimizationLevel(
                self.inner.0,
                opt_level as ffi::WasmEdge_CompilerOptimizationLevel,
            )
        }
    }

    /// Returns the optimization level of AOT compiler.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    #[cfg(feature = "aot")]
    pub fn get_aot_optimization_level(&self) -> CompilerOptimizationLevel {
        let level = unsafe { ffi::WasmEdge_ConfigureCompilerGetOptimizationLevel(self.inner.0) };
        level.into()
    }

    /// Sets the output binary format of AOT compiler. By default, the compiler output format is `Wasm`.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    ///
    /// # Argument
    ///
    /// * `format` - The format of the output binary.
    #[cfg(feature = "aot")]
    pub fn set_aot_compiler_output_format(&mut self, format: CompilerOutputFormat) {
        unsafe {
            ffi::WasmEdge_ConfigureCompilerSetOutputFormat(
                self.inner.0,
                format as ffi::WasmEdge_CompilerOutputFormat,
            )
        }
    }

    /// Returns the output binary format of AOT compiler.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    #[cfg(feature = "aot")]
    pub fn get_aot_compiler_output_format(&self) -> CompilerOutputFormat {
        let value = unsafe { ffi::WasmEdge_ConfigureCompilerGetOutputFormat(self.inner.0) };
        value.into()
    }

    /// Sets the dump IR option of AOT compiler. By default, the option is disabled.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    ///
    /// # Argument
    ///
    /// * `flag` - Whether dump ir or not.
    #[cfg(feature = "aot")]
    pub fn dump_ir(&mut self, flag: bool) {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetDumpIR(self.inner.0, flag) }
    }

    /// Checks if the dump IR option turns on or not.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    #[cfg(feature = "aot")]
    pub fn dump_ir_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureCompilerIsDumpIR(self.inner.0) }
    }

    /// Sets the generic binary option of AOT compiler. By default, the option is disabled.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    ///
    /// # Argument
    ///
    /// * `flag` - Whether generate the generic binary or not when perform AOT compilation.
    #[cfg(feature = "aot")]
    pub fn generic_binary(&mut self, flag: bool) {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetGenericBinary(self.inner.0, flag) }
    }

    /// Checks if the generic binary option of AOT compiler turns on or not.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    #[cfg(feature = "aot")]
    pub fn generic_binary_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureCompilerIsGenericBinary(self.inner.0) }
    }

    /// Enables or Disables the `Interruptible` option of AOT compiler. This option determines to generate interruptible binary or not when compilation in AOT compiler. By default, the option is disabled.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether turn on the `Interruptible` option.
    #[cfg(feature = "aot")]
    pub fn interruptible(&mut self, enable: bool) {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetInterruptible(self.inner.0, enable) }
    }

    /// Checks if the `Interruptible` option of AOT Compiler turns on or not.
    ///
    /// Notice that this function is only available when the `aot` feature is enabled.
    #[cfg(feature = "aot")]
    pub fn interruptible_enabled(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureCompilerIsInterruptible(self.inner.0) }
    }

    // For Statistics

    /// Sets the instruction counting option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `flag` - Whether support instruction counting or not when execution after AOT compilation.
    pub fn count_instructions(&mut self, flag: bool) {
        unsafe { ffi::WasmEdge_ConfigureStatisticsSetInstructionCounting(self.inner.0, flag) }
    }

    /// Checks if the instruction counting option turns on or not.
    pub fn is_instruction_counting(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureStatisticsIsInstructionCounting(self.inner.0) }
    }

    /// Sets the cost measuring option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `flag` - Whether support cost measuring or not when execution after AOT compilation.
    pub fn measure_cost(&mut self, flag: bool) {
        unsafe { ffi::WasmEdge_ConfigureStatisticsSetCostMeasuring(self.inner.0, flag) }
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn is_cost_measuring(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureStatisticsIsCostMeasuring(self.inner.0) }
    }

    /// Sets the time measuring option. By default, the option is disabled.
    ///
    /// # Argument
    ///
    /// * `flag` - Whether support time measuring or not when execution after AOT compilation.
    pub fn measure_time(&mut self, flag: bool) {
        unsafe { ffi::WasmEdge_ConfigureStatisticsSetTimeMeasuring(self.inner.0, flag) }
    }

    /// Checks if the time measuring option turns on or not.
    pub fn is_time_measuring(&self) -> bool {
        unsafe { ffi::WasmEdge_ConfigureStatisticsIsTimeMeasuring(self.inner.0) }
    }
}

#[derive(Debug)]
pub(crate) struct InnerConfig(pub(crate) *mut ffi::WasmEdge_ConfigureContext);
unsafe impl Send for InnerConfig {}
unsafe impl Sync for InnerConfig {}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        borrow::BorrowMut,
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_config_options() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();

        // check default settings
        assert!(!config.multi_memories_enabled());
        assert!(!config.annotations_enabled());
        assert!(config.bulk_memory_operations_enabled());
        assert!(!config.exception_handling_enabled());
        assert!(!config.function_references_enabled());
        assert!(!config.memory64_enabled());
        assert!(config.multi_value_enabled());
        assert!(config.mutable_globals_enabled());
        assert!(config.non_trap_conversions_enabled());
        assert!(config.sign_extension_operators_enabled());
        assert!(config.reference_types_enabled());
        assert!(config.simd_enabled());
        assert!(!config.tail_call_enabled());
        assert!(!config.threads_enabled());
        assert!(!config.wasi_enabled());
        #[cfg(target_os = "linux")]
        assert!(!config.wasmedge_process_enabled());
        assert!(!config.is_cost_measuring());
        #[cfg(feature = "aot")]
        assert!(!config.dump_ir_enabled());
        #[cfg(feature = "aot")]
        #[cfg(feature = "aot")]
        assert!(!config.generic_binary_enabled());
        assert!(!config.is_instruction_counting());
        assert!(!config.is_time_measuring());
        assert_eq!(config.get_max_memory_pages(), 65536);
        #[cfg(feature = "aot")]
        assert_eq!(
            config.get_aot_optimization_level(),
            CompilerOptimizationLevel::O3,
        );
        #[cfg(feature = "aot")]
        assert_eq!(
            config.get_aot_compiler_output_format(),
            CompilerOutputFormat::Wasm,
        );
        assert!(!config.interpreter_mode_enabled());
        #[cfg(feature = "aot")]
        assert!(!config.interruptible_enabled());

        // set options
        config.multi_memories(true);
        config.annotations(true);
        config.bulk_memory_operations(false);
        config.exception_handling(true);
        config.function_references(true);
        config.memory64(true);
        config.multi_value(false);
        config.mutable_globals(false);
        config.non_trap_conversions(false);
        config.sign_extension_operators(false);
        config.reference_types(false);
        config.simd(false);
        config.tail_call(true);
        config.threads(true);
        config.measure_cost(true);
        config.measure_time(true);
        #[cfg(feature = "aot")]
        config.dump_ir(true);
        #[cfg(feature = "aot")]
        config.generic_binary(true);
        config.count_instructions(true);
        config.interpreter_mode(true);

        // check new settings
        assert!(config.multi_memories_enabled());
        assert!(config.annotations_enabled());
        assert!(!config.bulk_memory_operations_enabled());
        assert!(config.exception_handling_enabled());
        assert!(config.function_references_enabled());
        assert!(config.memory64_enabled());
        assert!(!config.multi_value_enabled());
        assert!(!config.mutable_globals_enabled());
        assert!(!config.non_trap_conversions_enabled());
        assert!(!config.sign_extension_operators_enabled());
        assert!(!config.reference_types_enabled());
        assert!(!config.simd_enabled());
        assert!(config.tail_call_enabled());
        assert!(config.threads_enabled());
        assert!(config.is_cost_measuring());
        #[cfg(feature = "aot")]
        assert!(config.dump_ir_enabled());
        #[cfg(feature = "aot")]
        assert!(config.generic_binary_enabled());
        assert!(config.is_instruction_counting());
        assert!(config.is_time_measuring());
        assert!(config.interpreter_mode_enabled());

        // set maxmimum memory pages
        config.set_max_memory_pages(10);
        assert_eq!(config.get_max_memory_pages(), 10);
        #[cfg(feature = "aot")]
        config.set_aot_optimization_level(CompilerOptimizationLevel::Oz);
        #[cfg(feature = "aot")]
        assert_eq!(
            config.get_aot_optimization_level(),
            CompilerOptimizationLevel::Oz
        );
        #[cfg(feature = "aot")]
        config.set_aot_compiler_output_format(CompilerOutputFormat::Native);
        #[cfg(feature = "aot")]
        assert_eq!(
            config.get_aot_compiler_output_format(),
            CompilerOutputFormat::Native,
        );
    }

    #[test]
    fn test_config_send() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();

        let handle = thread::spawn(move || {
            // check default settings
            assert!(!config.multi_memories_enabled());
            assert!(!config.annotations_enabled());
            assert!(config.bulk_memory_operations_enabled());
            assert!(!config.exception_handling_enabled());
            assert!(!config.function_references_enabled());
            assert!(!config.memory64_enabled());
            assert!(config.reference_types_enabled());
            assert!(config.simd_enabled());
            assert!(!config.tail_call_enabled());
            assert!(!config.threads_enabled());
            assert!(!config.is_cost_measuring());
            #[cfg(feature = "aot")]
            assert!(!config.dump_ir_enabled());
            #[cfg(feature = "aot")]
            assert!(!config.generic_binary_enabled());
            assert!(!config.is_instruction_counting());
            assert!(!config.is_time_measuring());
            assert_eq!(config.get_max_memory_pages(), 65536);
            #[cfg(feature = "aot")]
            assert_eq!(
                config.get_aot_optimization_level(),
                CompilerOptimizationLevel::O3,
            );
            #[cfg(feature = "aot")]
            assert_eq!(
                config.get_aot_compiler_output_format(),
                CompilerOutputFormat::Wasm,
            );

            // set options
            config.multi_memories(true);
            config.annotations(true);
            config.bulk_memory_operations(false);
            config.exception_handling(true);
            config.function_references(true);
            config.memory64(true);
            config.reference_types(false);
            config.simd(false);
            config.tail_call(true);
            config.threads(true);
            config.measure_cost(true);
            config.measure_time(true);
            #[cfg(feature = "aot")]
            config.dump_ir(true);
            #[cfg(feature = "aot")]
            config.generic_binary(true);
            config.count_instructions(true);

            // check new settings
            assert!(config.multi_memories_enabled());
            assert!(config.annotations_enabled());
            assert!(!config.bulk_memory_operations_enabled());
            assert!(config.exception_handling_enabled());
            assert!(config.function_references_enabled());
            assert!(config.memory64_enabled());
            assert!(!config.reference_types_enabled());
            assert!(!config.simd_enabled());
            assert!(config.tail_call_enabled());
            assert!(config.threads_enabled());
            assert!(config.is_cost_measuring());
            #[cfg(feature = "aot")]
            assert!(config.dump_ir_enabled());
            #[cfg(feature = "aot")]
            assert!(config.generic_binary_enabled());
            assert!(config.is_instruction_counting());
            assert!(config.is_time_measuring());
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_config_sync() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let config = Arc::new(Mutex::new(result.unwrap()));

        let config_cloned = Arc::clone(&config);
        let handle = thread::spawn(move || {
            let result = config_cloned.lock();
            assert!(result.is_ok());
            let mut config = result.unwrap();

            // check default settings
            assert!(!config.multi_memories_enabled());
            assert!(!config.annotations_enabled());
            assert!(config.bulk_memory_operations_enabled());
            assert!(!config.exception_handling_enabled());
            assert!(!config.function_references_enabled());
            assert!(!config.memory64_enabled());
            assert!(config.reference_types_enabled());
            assert!(config.simd_enabled());
            assert!(!config.tail_call_enabled());
            assert!(!config.threads_enabled());
            assert!(!config.is_cost_measuring());
            #[cfg(feature = "aot")]
            assert!(!config.dump_ir_enabled());
            #[cfg(feature = "aot")]
            assert!(!config.generic_binary_enabled());
            assert!(!config.is_instruction_counting());
            assert!(!config.is_time_measuring());
            assert_eq!(config.get_max_memory_pages(), 65536);
            #[cfg(feature = "aot")]
            assert_eq!(
                config.get_aot_optimization_level(),
                CompilerOptimizationLevel::O3,
            );
            #[cfg(feature = "aot")]
            assert_eq!(
                config.get_aot_compiler_output_format(),
                CompilerOutputFormat::Wasm,
            );

            // set options
            let config_mut = config.borrow_mut();
            config_mut.multi_memories(true);
            config_mut.annotations(true);
            config_mut.bulk_memory_operations(false);
            config_mut.exception_handling(true);
            config_mut.function_references(true);
            config_mut.memory64(true);
            config_mut.reference_types(false);
            config_mut.simd(false);
            config_mut.tail_call(true);
            config_mut.threads(true);
            config_mut.measure_cost(true);
            config_mut.measure_time(true);
            #[cfg(feature = "aot")]
            config_mut.dump_ir(true);
            #[cfg(feature = "aot")]
            config_mut.generic_binary(true);
            config_mut.count_instructions(true);

            // check new settings
            assert!(config.multi_memories_enabled());
            assert!(config.annotations_enabled());
            assert!(!config.bulk_memory_operations_enabled());
            assert!(config.exception_handling_enabled());
            assert!(config.function_references_enabled());
            assert!(config.memory64_enabled());
            assert!(!config.reference_types_enabled());
            assert!(!config.simd_enabled());
            assert!(config.tail_call_enabled());
            assert!(config.threads_enabled());
            assert!(config.is_cost_measuring());
            #[cfg(feature = "aot")]
            assert!(config.dump_ir_enabled());
            #[cfg(feature = "aot")]
            assert!(config.generic_binary_enabled());
            assert!(config.is_instruction_counting());
            assert!(config.is_time_measuring());
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_config_clone() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        assert_eq!(std::sync::Arc::strong_count(&config.inner), 1);

        // set options
        config.multi_memories(true);
        config.annotations(true);
        config.bulk_memory_operations(false);
        config.exception_handling(true);
        config.function_references(true);
        config.memory64(true);
        config.multi_value(false);
        config.mutable_globals(false);
        config.non_trap_conversions(false);
        config.sign_extension_operators(false);
        config.reference_types(false);
        config.simd(false);
        config.tail_call(true);
        config.threads(true);
        config.measure_cost(true);
        config.measure_time(true);
        #[cfg(feature = "aot")]
        config.dump_ir(true);
        #[cfg(feature = "aot")]
        config.generic_binary(true);
        config.count_instructions(true);

        let config_clone = config.clone();
        assert_eq!(std::sync::Arc::strong_count(&config.inner), 2);
        // check new settings
        assert!(config_clone.multi_memories_enabled());
        assert!(config_clone.annotations_enabled());
        assert!(!config_clone.bulk_memory_operations_enabled());
        assert!(config_clone.exception_handling_enabled());
        assert!(config_clone.function_references_enabled());
        assert!(config_clone.memory64_enabled());
        assert!(!config_clone.multi_value_enabled());
        assert!(!config_clone.mutable_globals_enabled());
        assert!(!config_clone.non_trap_conversions_enabled());
        assert!(!config_clone.sign_extension_operators_enabled());
        assert!(!config_clone.reference_types_enabled());
        assert!(!config_clone.simd_enabled());
        assert!(config_clone.tail_call_enabled());
        assert!(config_clone.threads_enabled());
        assert!(config_clone.is_cost_measuring());
        #[cfg(feature = "aot")]
        assert!(config_clone.dump_ir_enabled());
        #[cfg(feature = "aot")]
        assert!(config_clone.generic_binary_enabled());
        assert!(config_clone.is_instruction_counting());
        assert!(config_clone.is_time_measuring());

        drop(config);
        assert_eq!(std::sync::Arc::strong_count(&config_clone.inner), 1);
        drop(config_clone);
    }
}
