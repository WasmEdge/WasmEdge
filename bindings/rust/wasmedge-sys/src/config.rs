//! Defines WasmEdge Config struct.

use crate::{
    types::{CompilerOptimizationLevel, CompilerOutputFormat},
    wasmedge, Error, WasmEdgeResult,
};

/// Struct of WasmEdge Config.
///
/// [`Config`] is used to declare a group of global environment settings.
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
            true => Err(Error::OperationError(String::from(
                "fail to create Config instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    /// Enables a host pre-registration setting
    pub fn enable_wasi(self) -> Self {
        unsafe {
            wasmedge::WasmEdge_ConfigureAddHostRegistration(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        };
        self
    }

    /// Sets the maximum number of the memory pages available.
    pub fn set_max_memory_pages(self, count: u32) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureSetMaxMemoryPage(self.ctx, count) };
        self
    }

    /// Returns the number of the memory pages available.
    pub fn get_max_memory_pages(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ConfigureGetMaxMemoryPage(self.ctx) }
    }

    // For AOT compiler

    /// Sets the optimization level of AOT compiler.
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
    pub fn enable_dump_ir(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetDumpIR(self.ctx, flag) };
        self
    }

    /// Returns the dump IR option of AOT compiler.
    pub fn is_dump_ir(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsDumpIR(self.ctx) }
    }

    /// Sets the generic binary option of AOT compiler.
    pub fn enable_generic_binary(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetGenericBinary(self.ctx, flag) };
        self
    }

    /// Returns the generic binary option of AOT compiler.
    pub fn is_generic_binary(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsGenericBinary(self.ctx) }
    }

    /// Sets the instruction counting option.
    pub fn enable_instruction_counting(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetInstructionCounting(self.ctx, flag) };
        self
    }

    /// Returns the instruction counting option.
    pub fn is_instruction_counting(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsInstructionCounting(self.ctx) }
    }

    /// Sets the cost measuring option.
    pub fn enable_cost_measuring(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetCostMeasuring(self.ctx, flag) };
        self
    }

    /// Returns the cost measuring option.
    pub fn is_cost_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsCostMeasuring(self.ctx) }
    }

    /// Sets the time measuring option.
    pub fn enable_time_measuring(self, flag: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetTimeMeasuring(self.ctx, flag) };
        self
    }

    /// Returns the cost measuring option.
    pub fn is_time_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsTimeMeasuring(self.ctx) }
    }
}

/// Code Generate for `WasmEdge_Proposal` :
///
/// ```rust
/// impl Config {
///     pub fn enable_bulkmemoryoperations(self, enable: bool) -> Self {
///         let prop = wasmedge::WasmEdge_Proposal_BulkMemoryOperations;
///         if enable {
///             unsafe { wasmedge::WasmEdge_ConfigureAddProposal(self.ctx, prop) };
///         } else {
///             unsafe { wasmedge::WasmEdge_ConfigureRemoveProposal(self.ctx, prop) };
///         }
///         self
///     }
///     pub fn has_bulkmemoryoperations(&self) -> bool {
///         let prop = wasmedge::WasmEdge_Proposal_BulkMemoryOperations;
///         unsafe { wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, prop) }
///     }
///     // ...
/// }
/// ```
macro_rules! impl_proposal_config {
    ($( $proposal:ident ),+ $(,)+) => {
        impl Config {
            paste::paste! {
                $(
                    pub fn [<enable_$proposal:lower:snake>](self, enable: bool) -> Self {
                        let prop = wasmedge::[<WasmEdge_Proposal_$proposal>];
                        if enable {
                            unsafe { wasmedge::WasmEdge_ConfigureAddProposal(self.ctx, prop) };
                        } else {
                            unsafe { wasmedge::WasmEdge_ConfigureRemoveProposal(self.ctx, prop) };
                        }
                        self
                    }

                    pub fn [<has_$proposal:lower:snake>](&self) -> bool {
                        let prop = wasmedge::[<WasmEdge_Proposal_$proposal>];
                        unsafe { wasmedge::WasmEdge_ConfigureHasProposal(self.ctx, prop) }
                    }
                )+
            }
        }
    }
}
impl_proposal_config! {
    BulkMemoryOperations,
    ReferenceTypes,
    SIMD,
    TailCall,
    Annotations,
    Memory64,
    Threads,
    ExceptionHandling,
    FunctionReferences,
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
        assert!(!config.has_annotations());
        assert!(config.has_bulkmemoryoperations());
        assert!(!config.has_exceptionhandling());
        assert!(!config.has_functionreferences());
        assert!(!config.has_memory64());
        assert!(config.has_referencetypes());
        assert!(config.has_simd());
        assert!(!config.has_tailcall());
        assert!(!config.has_threads());
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
            .enable_bulkmemoryoperations(false)
            .enable_exceptionhandling(true)
            .enable_functionreferences(true)
            .enable_memory64(true)
            .enable_referencetypes(false)
            .enable_simd(false)
            .enable_tailcall(true)
            .enable_threads(true)
            .enable_cost_measuring(true)
            .enable_time_measuring(true)
            .enable_dump_ir(true)
            .enable_generic_binary(true)
            .enable_instruction_counting(true);

        // check new settings
        assert!(config.has_annotations());
        assert!(!config.has_bulkmemoryoperations());
        assert!(config.has_exceptionhandling());
        assert!(config.has_functionreferences());
        assert!(config.has_memory64());
        assert!(!config.has_referencetypes());
        assert!(!config.has_simd());
        assert!(config.has_tailcall());
        assert!(config.has_threads());
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
