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
    pub(crate) ctx: *mut wasmedge::WasmEdge_ConfigureContext,
}
impl Drop for Config {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ConfigureDelete(self.ctx) };
        }
    }
}
impl Config {
    /// Creates a new [`Config`].
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_ConfigureCreate() };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ConfigCreate),
            false => Ok(Self { ctx }),
        }
    }

    /// Enables or disables host registration wasi.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn wasi(self, enable: bool) -> Self {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddHostRegistration(
                    self.ctx,
                    wasmedge::WasmEdge_HostRegistration_Wasi,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveHostRegistration(
                    self.ctx,
                    wasmedge::WasmEdge_HostRegistration_Wasi,
                )
            }
        };
        self
    }

    /// Checks if host registration wasi turns on or not.
    pub fn wasi_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasHostRegistration(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        }
    }

    /// Checks if host registration wasmedge process turns on or not.
    pub fn wasmedge_process_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasHostRegistration(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
            )
        }
    }

    /// Enables or disables host registration WasmEdge process.
    pub fn wasmedge_process(self, enable: bool) -> Self {
        unsafe {
            if enable {
                wasmedge::WasmEdge_ConfigureAddHostRegistration(
                    self.ctx,
                    wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            } else {
                wasmedge::WasmEdge_ConfigureRemoveHostRegistration(
                    self.ctx,
                    wasmedge::WasmEdge_HostRegistration_WasmEdge_Process,
                )
            }
        };
        self
    }

    /// Sets the maximum number of the memory pages available.
    ///
    /// # Argument
    ///
    /// - `count` specifies the page count (64KB per page).
    pub fn set_max_memory_pages(self, count: u32) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureSetMaxMemoryPage(self.ctx, count) };
        self
    }

    /// Returns the number of the memory pages available.
    pub fn get_max_memory_pages(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ConfigureGetMaxMemoryPage(self.ctx) }
    }

    /// Enables the ImportExportMutGlobals option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_mutable_globals(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.mutable_globals_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            } else if !flag && self.mutable_globals_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
                )
            }
        };
        self
    }

    /// Checks if the ImportExportMutGlobals option turns on or not.
    pub fn mutable_globals_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_ImportExportMutGlobals,
            )
        }
    }

    /// Enables the NonTrapFloatToIntConversions option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_non_trap_conversions(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.non_trap_conversions_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            } else if !flag && self.non_trap_conversions_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
                )
            }
        };
        self
    }

    /// Checks if the NonTrapFloatToIntConversions option turns on or not.
    pub fn non_trap_conversions_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_NonTrapFloatToIntConversions,
            )
        }
    }

    /// Enables the SignExtensionOperators option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_sign_extension_operators(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.sign_extension_operators_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_SignExtensionOperators,
                )
            } else if !flag && self.sign_extension_operators_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_SignExtensionOperators,
                )
            }
        };
        self
    }

    /// Checks if the SignExtensionOperators option turns on or not.
    pub fn sign_extension_operators_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_SignExtensionOperators,
            )
        }
    }

    /// Enables the MultiValue option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_multi_value(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.multi_value_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_MultiValue,
                )
            } else if !flag && self.multi_value_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_MultiValue,
                )
            }
        };
        self
    }

    /// Checks if the MultiValue option turns on or not.
    pub fn multi_value_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_MultiValue,
            )
        }
    }

    /// Enables the BulkMemoryOperations option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_bulk_memory_operations(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.bulk_memory_operations_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
                )
            } else if !flag && self.bulk_memory_operations_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
                )
            }
        };
        self
    }

    /// Checks if the BulkMemoryOperations option turns on or not.
    pub fn bulk_memory_operations_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_BulkMemoryOperations,
            )
        }
    }

    /// Enables the ReferenceTypes option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_reference_types(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.reference_types_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ReferenceTypes,
                )
            } else if !flag && self.reference_types_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ReferenceTypes,
                )
            }
        };
        self
    }

    /// Checks if the ReferenceTypes option turns on or not.
    pub fn reference_types_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_ReferenceTypes,
            )
        }
    }

    /// Enables the SIMD option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_simd(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.simd_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(self.ctx, wasmedge::WasmEdge_Proposal_SIMD)
            } else if !flag && self.simd_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_SIMD,
                )
            }
        };
        self
    }

    /// Checks if the SIMD option turns on or not.
    pub fn simd_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, wasmedge::WasmEdge_Proposal_SIMD)
        }
    }

    /// Enables the TailCall option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_tail_call(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.tail_call_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_TailCall,
                )
            } else if !flag && self.tail_call_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_TailCall,
                )
            }
        };
        self
    }

    /// Checks if the TailCall option turns on or not.
    pub fn tail_call_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, wasmedge::WasmEdge_Proposal_TailCall)
        }
    }

    /// Enables the Annotations option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_annotations(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.annotations_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Annotations,
                )
            } else if !flag && self.annotations_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Annotations,
                )
            }
        };
        self
    }

    /// Checks if the Annotations option turns on or not.
    pub fn annotations_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_Annotations,
            )
        }
    }

    /// Enables the Memory64 option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_memory64(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.memory64_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Memory64,
                )
            } else if !flag && self.memory64_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Memory64,
                )
            }
        };
        self
    }

    /// Checks if the Memory64 option turns on or not.
    pub fn memory64_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, wasmedge::WasmEdge_Proposal_Memory64)
        }
    }

    /// Enables the Threads option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_threads(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.threads_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Threads,
                )
            } else if !flag && self.threads_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_Threads,
                )
            }
        };
        self
    }

    /// Checks if the Threads option turns on or not.
    pub fn threads_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, wasmedge::WasmEdge_Proposal_Threads)
        }
    }

    /// Enables the ExceptionHandling option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_exception_handling(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.exception_handling_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ExceptionHandling,
                )
            } else if !flag && self.exception_handling_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_ExceptionHandling,
                )
            }
        };
        self
    }

    /// Checks if the ExceptionHandling option turns on or not.
    pub fn exception_handling_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
                wasmedge::WasmEdge_Proposal_ExceptionHandling,
            )
        }
    }

    /// Enables the FunctionReferences option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if the option turns on or not.
    pub fn enable_function_references(self, flag: bool) -> Self {
        unsafe {
            if flag && !self.function_references_enabled() {
                wasmedge::WasmEdge_ConfigureAddProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_FunctionReferences,
                )
            } else if !flag && self.function_references_enabled() {
                wasmedge::WasmEdge_ConfigureRemoveProposal(
                    self.ctx,
                    wasmedge::WasmEdge_Proposal_FunctionReferences,
                )
            }
        };
        self
    }

    /// Checks if the FunctionReferences option turns on or not.
    pub fn function_references_enabled(&self) -> bool {
        unsafe {
            wasmedge::WasmEdge_ConfigureHasProposal(
                self.ctx,
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
    pub fn set_optimization_level(self, opt_level: CompilerOptimizationLevel) -> Self {
        unsafe {
            wasmedge::WasmEdge_ConfigureCompilerSetOptimizationLevel(self.ctx, opt_level as u32)
        };
        self
    }

    /// Returns the optimization level of AOT compiler.
    pub fn get_optimization_level(&self) -> CompilerOptimizationLevel {
        let level = unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOptimizationLevel(self.ctx) };
        level.into()
    }

    /// Sets the output binary format of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `format` specifies the format of the output binary.
    pub fn set_compiler_output_format(self, format: CompilerOutputFormat) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetOutputFormat(self.ctx, format as u32) };
        self
    }

    /// Returns the output binary format of AOT compiler.
    pub fn get_compiler_output_format(&self) -> CompilerOutputFormat {
        let value = unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOutputFormat(self.ctx) };
        value.into()
    }

    /// Sets the dump IR option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if dump ir or not.
    pub fn dump_ir(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetDumpIR(self.ctx, flag) };
        self
    }

    /// Checks if the dump IR option turns on or not.
    pub fn is_dump_ir(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsDumpIR(self.ctx) }
    }

    /// Sets the generic binary option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if generate the generic binary or not when perform AOT compilation.
    pub fn generic_binary(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetGenericBinary(self.ctx, flag) };
        self
    }

    /// Checks if the generic binary option of AOT compiler turns on or not.
    pub fn is_generic_binary(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsGenericBinary(self.ctx) }
    }

    /// Sets the instruction counting option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support instruction counting or not when execution after AOT compilation.
    pub fn count_instructions(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetInstructionCounting(self.ctx, flag) };
        self
    }

    /// Checks if the instruction counting option turns on or not.
    pub fn is_instruction_counting(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsInstructionCounting(self.ctx) }
    }

    /// Sets the cost measuring option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support cost measuring or not when execution after AOT compilation.
    pub fn measure_cost(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetCostMeasuring(self.ctx, flag) };
        self
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn is_cost_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsCostMeasuring(self.ctx) }
    }

    /// Sets the time measuring option.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if support time measuring or not when execution after AOT compilation.
    pub fn measure_time(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetTimeMeasuring(self.ctx, flag) };
        self
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn is_time_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsTimeMeasuring(self.ctx) }
    }

    /// Enables or Disables the `Interruptible` option of AOT compiler.
    ///
    /// This option determines to generate interruptible binary or not when compilation in AOT compiler.
    ///
    /// # Argument
    ///
    /// - `flag` specifies if turn on the `Interruptible` option.
    pub fn interruptible(self, enable: bool) -> Self {
        unsafe {
            wasmedge::WasmEdge_ConfigureCompilerSetInterruptible(self.ctx, enable);
        }
        self
    }

    /// Checks if the `Interruptible` option of AOT Compiler turns on or not.
    pub fn interruptible_enabled(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsInterruptible(self.ctx) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_config_options() {
        // create a Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

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
        assert!(!config.is_cost_measuring());
        assert!(!config.is_dump_ir());
        assert!(!config.is_generic_binary());
        assert!(!config.is_instruction_counting());
        assert!(!config.is_time_measuring());
        assert_eq!(config.get_max_memory_pages(), 65536);
        assert_eq!(
            config.get_optimization_level(),
            CompilerOptimizationLevel::O3,
        );
        assert_eq!(
            config.get_compiler_output_format(),
            CompilerOutputFormat::Wasm,
        );

        // set options
        let config = config
            .enable_annotations(true)
            .enable_bulk_memory_operations(false)
            .enable_exception_handling(true)
            .enable_function_references(true)
            .enable_memory64(true)
            .enable_reference_types(false)
            .enable_simd(false)
            .enable_tail_call(true)
            .enable_threads(true)
            .measure_cost(true)
            .measure_time(true)
            .dump_ir(true)
            .generic_binary(true)
            .count_instructions(true);

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
        assert!(config.is_cost_measuring());
        assert!(config.is_dump_ir());
        assert!(config.is_generic_binary());
        assert!(config.is_instruction_counting());
        assert!(config.is_time_measuring());

        // set maxmimum memory pages
        let config = config.set_max_memory_pages(10);
        assert_eq!(config.get_max_memory_pages(), 10);
        let config = config.set_optimization_level(CompilerOptimizationLevel::Oz);
        assert_eq!(
            config.get_optimization_level(),
            CompilerOptimizationLevel::Oz
        );
        let config = config.set_compiler_output_format(CompilerOutputFormat::Native);
        assert_eq!(
            config.get_compiler_output_format(),
            CompilerOutputFormat::Native,
        );
    }
}
