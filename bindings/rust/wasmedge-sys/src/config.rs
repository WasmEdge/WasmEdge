//! Defines WasmEdge Config struct.

use crate::{
    wasmedge, CompilerOptimizationLevel, CompilerOutputFormat, WasmEdgeError, WasmEdgeResult,
};

/// Struct of WasmEdge Config.
///
/// [`Config`] manages the configuration options, which are used in WasmEdge [Vm](crate::Vm), [Loader](crate::Loader),
/// [Validator](crate::Validator), [Executor](crate::Executor), and [Compiler](crate::Compiler).
///
/// The configuration options are categorized into the following four groups:
///
/// - **WebAssembly Proposals**
///
///     This group of options are used to turn on/off the WebAssembly proposals. They are effective to any WasmEdge
///     context created with [`Config`].
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
///       Also see [](https://github.com/WebAssembly/tail-call/blob/master/proposals/tail-call/Overview.md).
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
#[derive(Debug)]
pub struct Config {
    pub(crate) inner: InnerConfig,
}
impl Drop for Config {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            unsafe { wasmedge::WasmEdge_ConfigureDelete(self.inner.0) };
        }
    }
}
impl Config {
    /// Creates a new [`Config`].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_ConfigureCreate() };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ConfigCreate),
            false => Ok(Self {
                inner: InnerConfig(ctx),
            }),
        }
    }

    /// Creates a new [`Config`] from an existed one.
    ///
    /// - `src` specifies the source [`Config`].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn copy_from(src: &Config) -> WasmEdgeResult<Self> {
        let mut config = Config::create()?;

        if src.annotations_enabled() {
            config.annotations(true)
        } else {
            config.annotations(false)
        };

        if src.bulk_memory_operations_enabled() {
            config.bulk_memory_operations(true)
        } else {
            config.bulk_memory_operations(false)
        };

        if src.exception_handling_enabled() {
            config.exception_handling(true)
        } else {
            config.exception_handling(false)
        };

        config.function_references(src.function_references_enabled());

        if src.memory64_enabled() {
            config.memory64(true)
        } else {
            config.memory64(false)
        };

        if src.multi_value_enabled() {
            config.multi_value(true)
        } else {
            config.multi_value(false)
        };

        if src.mutable_globals_enabled() {
            config.mutable_globals(true)
        } else {
            config.mutable_globals(false)
        };

        if src.non_trap_conversions_enabled() {
            config.non_trap_conversions(true)
        } else {
            config.non_trap_conversions(false)
        };

        if src.reference_types_enabled() {
            config.reference_types(true)
        } else {
            config.reference_types(false)
        };

        if src.sign_extension_operators_enabled() {
            config.sign_extension_operators(true)
        } else {
            config.sign_extension_operators(false)
        };

        if src.simd_enabled() {
            config.simd(true)
        } else {
            config.simd(false)
        };

        if src.tail_call_enabled() {
            config.tail_call(true)
        } else {
            config.tail_call(false)
        };

        if src.threads_enabled() {
            config.threads(true)
        } else {
            config.threads(false)
        };

        if src.wasi_enabled() {
            config.wasi(true)
        } else {
            config.wasi(false)
        };

        if src.wasmedge_process_enabled() {
            config.wasmedge_process(true)
        } else {
            config.wasmedge_process(false)
        };

        if src.is_aot_cost_measuring() {
            config.aot_measure_cost(true)
        } else {
            config.aot_measure_cost(false)
        };

        if src.is_aot_instruction_counting() {
            config.aot_count_instructions(true)
        } else {
            config.aot_count_instructions(false)
        };

        if src.is_aot_time_measuring() {
            config.aot_measure_time(true)
        } else {
            config.aot_measure_time(false)
        };

        config.set_max_memory_pages(src.get_max_memory_pages());

        // let config = config.aot_interruptible(src.aot_interruptible_enabled());

        if src.is_aot_dump_ir() {
            config.aot_dump_ir(true)
        } else {
            config.aot_dump_ir(false)
        };

        if src.is_aot_generic_binary() {
            config.aot_generic_binary(true)
        } else {
            config.aot_generic_binary(false)
        };

        config.set_aot_compiler_output_format(src.get_aot_compiler_output_format());

        config.set_aot_optimization_level(src.get_aot_optimization_level());

        Ok(config)
    }

    /// Enables or disables host registration wasi.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn wasi(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    wasmedge::WasmEdge_HostRegistration_Wasi,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    wasmedge::WasmEdge_HostRegistration_Wasi,
                )
            }
        }
    }

    /// Checks if host registration wasi turns on or not.
    pub fn wasi_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        }
    }

    /// Enables or disables host registration WasmEdge process.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn wasmedge_process(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddHostRegistration(
                    self.inner.0,
                    wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveHostRegistration(
                    self.inner.0,
                    wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            }
        }
    }

    /// Checks if host registration wasmedge process turns on or not.
    pub fn wasmedge_process_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasHostRegistration(
                self.inner.0,
                wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
            )
        }
    }

    /// Sets the maximum number of the memory pages available.
    ///
    /// # Argument
    ///
    /// - `count` specifies the page count (64KB per page).
    pub fn set_max_memory_pages(&mut self, count: u32) {
        unsafe { wasmedge::WasmEdge_ConfigureSetMaxMemoryPage(self.inner.0, count) }
    }

    /// Returns the number of the memory pages available.
    pub fn get_max_memory_pages(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ConfigureGetMaxMemoryPage(self.inner.0) }
    }

    /// Enables or disables the ImportExportMutGlobals option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn mutable_globals(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            }
        }
    }

    /// Checks if the ImportExportMutGlobals option turns on or not.
    pub fn mutable_globals_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
            )
        }
    }

    /// Enables or disables the NonTrapFloatToIntConversions option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn non_trap_conversions(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            }
        }
    }

    /// Checks if the NonTrapFloatToIntConversions option turns on or not.
    pub fn non_trap_conversions_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
            )
        }
    }

    /// Enables or disables the SignExtensionOperators option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn sign_extension_operators(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_SignExtensionOperators,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_SignExtensionOperators,
                )
            }
        }
    }

    /// Checks if the SignExtensionOperators option turns on or not.
    pub fn sign_extension_operators_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_SignExtensionOperators,
            )
        }
    }

    /// Enables or disables the MultiValue option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn multi_value(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_MultiValue,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_MultiValue,
                )
            }
        }
    }

    /// Checks if the MultiValue option turns on or not.
    pub fn multi_value_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_MultiValue,
            )
        }
    }

    /// Enables or disables the BulkMemoryOperations option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn bulk_memory_operations(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
                )
            }
        }
    }

    /// Checks if the BulkMemoryOperations option turns on or not.
    pub fn bulk_memory_operations_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
            )
        }
    }

    /// Enables or disables the ReferenceTypes option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn reference_types(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ReferenceTypes,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ReferenceTypes,
                )
            }
        }
    }

    /// Checks if the ReferenceTypes option turns on or not.
    pub fn reference_types_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_ReferenceTypes,
            )
        }
    }

    /// Enables or disables the SIMD option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn simd(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_SIMD,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_SIMD,
                )
            }
        }
    }

    /// Checks if the SIMD option turns on or not.
    pub fn simd_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(self.inner.0, wasmedge::WasmEdge_Proposal_SIMD)
        }
    }

    /// Enables or disables the TailCall option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn tail_call(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_TailCall,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_TailCall,
                )
            }
        }
    }

    /// Checks if the TailCall option turns on or not.
    pub fn tail_call_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_TailCall,
            )
        }
    }

    /// Enables or disables the Annotations option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn annotations(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Annotations,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Annotations,
                )
            }
        }
    }

    /// Checks if the Annotations option turns on or not.
    pub fn annotations_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_Annotations,
            )
        }
    }

    /// Enables or disables the Memory64 option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn memory64(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Memory64,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Memory64,
                )
            }
        }
    }

    /// Checks if the Memory64 option turns on or not.
    pub fn memory64_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_Memory64,
            )
        }
    }

    /// Enables or disables the Threads option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn threads(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Threads,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_Threads,
                )
            }
        }
    }

    /// Checks if the Threads option turns on or not.
    pub fn threads_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_Threads,
            )
        }
    }

    /// Enables or disables the ExceptionHandling option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn exception_handling(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ExceptionHandling,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_ExceptionHandling,
                )
            }
        }
    }

    /// Checks if the ExceptionHandling option turns on or not.
    pub fn exception_handling_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_ExceptionHandling,
            )
        }
    }

    /// Enables or disables the FunctionReferences option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn function_references(&mut self, enable: bool) {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_FunctionReferences,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.inner.0,
                    wasmedge::WasmEdge_Proposal_FunctionReferences,
                )
            }
        }
    }

    /// Checks if the FunctionReferences option turns on or not.
    pub fn function_references_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.inner.0,
                wasmedge::WasmEdge_Proposal_FunctionReferences,
            )
        }
    }

    // For AOT compiler

    /// Sets the optimization level of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `opt_level` specifies the optimization level of AOT compiler.
    pub fn set_aot_optimization_level(&mut self, opt_level: CompilerOptimizationLevel) {
        unsafe {
            wasmedge::WasmEdge_ConfigureCompilerSetOptimizationLevel(self.inner.0, opt_level as u32)
        }
    }

    /// Returns the optimization level of AOT compiler.
    pub fn get_aot_optimization_level(&self) -> CompilerOptimizationLevel {
        let level =
            unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOptimizationLevel(self.inner.0) };
        level.into()
    }

    /// Sets the output binary format of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `format` specifies the format of the output binary.
    pub fn set_aot_compiler_output_format(&mut self, format: CompilerOutputFormat) {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetOutputFormat(self.inner.0, format as u32) }
    }

    /// Returns the output binary format of AOT compiler.
    pub fn get_aot_compiler_output_format(&self) -> CompilerOutputFormat {
        let value = unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOutputFormat(self.inner.0) };
        value.into()
    }

    /// Sets the dump IR option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if dump ir or not.
    pub fn aot_dump_ir(&mut self, flag: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetDumpIR(self.inner.0, flag) }
    }

    /// Checks if the dump IR option turns on or not.
    pub fn is_aot_dump_ir(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsDumpIR(self.inner.0) }
    }

    /// Sets the generic binary option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if generate the generic binary or not when perform AOT compilation.
    pub fn aot_generic_binary(&mut self, flag: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetGenericBinary(self.inner.0, flag) }
    }

    /// Checks if the generic binary option of AOT compiler turns on or not.
    pub fn is_aot_generic_binary(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsGenericBinary(self.inner.0) }
    }

    /// Enables or Disables the `Interruptible` option of AOT compiler.
    ///
    /// This option determines to generate interruptible binary or not when compilation in AOT compiler.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if turn on the `Interruptible` option.
    pub fn aot_interruptible(&mut self, enable: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetInterruptible(self.inner.0, enable) }
    }

    /// Checks if the `Interruptible` option of AOT Compiler turns on or not.
    pub fn aot_interruptible_enabled(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsInterruptible(self.inner.0) }
    }

    // For Statistics

    /// Sets the instruction counting option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support instruction counting or not when execution after AOT compilation.
    pub fn aot_count_instructions(&mut self, flag: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetInstructionCounting(self.inner.0, flag) }
    }

    /// Checks if the instruction counting option turns on or not.
    pub fn is_aot_instruction_counting(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsInstructionCounting(self.inner.0) }
    }

    /// Sets the cost measuring option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support cost measuring or not when execution after AOT compilation.
    pub fn aot_measure_cost(&mut self, flag: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetCostMeasuring(self.inner.0, flag) }
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn is_aot_cost_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsCostMeasuring(self.inner.0) }
    }

    /// Sets the time measuring option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support time measuring or not when execution after AOT compilation.
    pub fn aot_measure_time(&mut self, flag: bool) {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetTimeMeasuring(self.inner.0, flag) }
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn is_aot_time_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsTimeMeasuring(self.inner.0) }
    }
}

#[derive(Debug)]
pub(crate) struct InnerConfig(pub(crate) *mut wasmedge::WasmEdge_ConfigureContext);
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
        assert!(!config.is_aot_cost_measuring());
        assert!(!config.is_aot_dump_ir());
        assert!(!config.is_aot_generic_binary());
        assert!(!config.is_aot_instruction_counting());
        assert!(!config.is_aot_time_measuring());
        assert_eq!(config.get_max_memory_pages(), 65536);
        assert_eq!(
            config.get_aot_optimization_level(),
            CompilerOptimizationLevel::O3,
        );
        assert_eq!(
            config.get_aot_compiler_output_format(),
            CompilerOutputFormat::Wasm,
        );

        // set options
        config.annotations(true);
        config.bulk_memory_operations(false);
        config.exception_handling(true);
        config.function_references(true);
        config.memory64(true);
        config.reference_types(false);
        config.simd(false);
        config.tail_call(true);
        config.threads(true);
        config.aot_measure_cost(true);
        config.aot_measure_time(true);
        config.aot_dump_ir(true);
        config.aot_generic_binary(true);
        config.aot_count_instructions(true);

        // check new settings
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
        assert!(config.is_aot_cost_measuring());
        assert!(config.is_aot_dump_ir());
        assert!(config.is_aot_generic_binary());
        assert!(config.is_aot_instruction_counting());
        assert!(config.is_aot_time_measuring());

        // set maxmimum memory pages
        config.set_max_memory_pages(10);
        assert_eq!(config.get_max_memory_pages(), 10);
        config.set_aot_optimization_level(CompilerOptimizationLevel::Oz);
        assert_eq!(
            config.get_aot_optimization_level(),
            CompilerOptimizationLevel::Oz
        );
        config.set_aot_compiler_output_format(CompilerOutputFormat::Native);
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
            assert!(!config.annotations_enabled());
            assert!(config.bulk_memory_operations_enabled());
            assert!(!config.exception_handling_enabled());
            assert!(!config.function_references_enabled());
            assert!(!config.memory64_enabled());
            assert!(config.reference_types_enabled());
            assert!(config.simd_enabled());
            assert!(!config.tail_call_enabled());
            assert!(!config.threads_enabled());
            assert!(!config.is_aot_cost_measuring());
            assert!(!config.is_aot_dump_ir());
            assert!(!config.is_aot_generic_binary());
            assert!(!config.is_aot_instruction_counting());
            assert!(!config.is_aot_time_measuring());
            assert_eq!(config.get_max_memory_pages(), 65536);
            assert_eq!(
                config.get_aot_optimization_level(),
                CompilerOptimizationLevel::O3,
            );
            assert_eq!(
                config.get_aot_compiler_output_format(),
                CompilerOutputFormat::Wasm,
            );

            // set options
            config.annotations(true);
            config.bulk_memory_operations(false);
            config.exception_handling(true);
            config.function_references(true);
            config.memory64(true);
            config.reference_types(false);
            config.simd(false);
            config.tail_call(true);
            config.threads(true);
            config.aot_measure_cost(true);
            config.aot_measure_time(true);
            config.aot_dump_ir(true);
            config.aot_generic_binary(true);
            config.aot_count_instructions(true);

            // check new settings
            assert!(config.annotations_enabled());
            assert!(!config.bulk_memory_operations_enabled());
            assert!(config.exception_handling_enabled());
            assert!(config.function_references_enabled());
            assert!(config.memory64_enabled());
            assert!(!config.reference_types_enabled());
            assert!(!config.simd_enabled());
            assert!(config.tail_call_enabled());
            assert!(config.threads_enabled());
            assert!(config.is_aot_cost_measuring());
            assert!(config.is_aot_dump_ir());
            assert!(config.is_aot_generic_binary());
            assert!(config.is_aot_instruction_counting());
            assert!(config.is_aot_time_measuring());
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
            assert!(!config.annotations_enabled());
            assert!(config.bulk_memory_operations_enabled());
            assert!(!config.exception_handling_enabled());
            assert!(!config.function_references_enabled());
            assert!(!config.memory64_enabled());
            assert!(config.reference_types_enabled());
            assert!(config.simd_enabled());
            assert!(!config.tail_call_enabled());
            assert!(!config.threads_enabled());
            assert!(!config.is_aot_cost_measuring());
            assert!(!config.is_aot_dump_ir());
            assert!(!config.is_aot_generic_binary());
            assert!(!config.is_aot_instruction_counting());
            assert!(!config.is_aot_time_measuring());
            assert_eq!(config.get_max_memory_pages(), 65536);
            assert_eq!(
                config.get_aot_optimization_level(),
                CompilerOptimizationLevel::O3,
            );
            assert_eq!(
                config.get_aot_compiler_output_format(),
                CompilerOutputFormat::Wasm,
            );

            // set options
            let config_mut = config.borrow_mut();
            config_mut.annotations(true);
            config_mut.bulk_memory_operations(false);
            config_mut.exception_handling(true);
            config_mut.function_references(true);
            config_mut.memory64(true);
            config_mut.reference_types(false);
            config_mut.simd(false);
            config_mut.tail_call(true);
            config_mut.threads(true);
            config_mut.aot_measure_cost(true);
            config_mut.aot_measure_time(true);
            config_mut.aot_dump_ir(true);
            config_mut.aot_generic_binary(true);
            config_mut.aot_count_instructions(true);

            // check new settings
            assert!(config.annotations_enabled());
            assert!(!config.bulk_memory_operations_enabled());
            assert!(config.exception_handling_enabled());
            assert!(config.function_references_enabled());
            assert!(config.memory64_enabled());
            assert!(!config.reference_types_enabled());
            assert!(!config.simd_enabled());
            assert!(config.tail_call_enabled());
            assert!(config.threads_enabled());
            assert!(config.is_aot_cost_measuring());
            assert!(config.is_aot_dump_ir());
            assert!(config.is_aot_generic_binary());
            assert!(config.is_aot_instruction_counting());
            assert!(config.is_aot_time_measuring());
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_config_clone() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.memory64(true);

        let result = Config::copy_from(&config);
        assert!(result.is_ok());
        let config_cloned = result.unwrap();
        assert!(config_cloned.memory64_enabled());
    }
}
