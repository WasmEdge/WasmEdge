use crate::{
    types::{CompilerOptimizationLevel, CompilerOutputFormat},
    wasmedge, Error, WasmEdgeResult,
};

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

impl Config {
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_ConfigureCreate() };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Config instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    // For Vm
    pub fn enable_wasi(self) -> Self {
        unsafe {
            wasmedge::WasmEdge_ConfigureAddHostRegistration(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        };
        self
    }

    /// Set the maximum number of memory pages available to running modules.
    pub fn set_max_memory_pages(self, num_pages: u32) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureSetMaxMemoryPage(self.ctx, num_pages) };
        self
    }

    /// Get the page limit of memory instances.
    pub fn get_max_memory_pages(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ConfigureGetMaxMemoryPage(self.ctx) }
    }

    // For AOT compiler

    /// Set the optimization level of AOT compiler.
    pub fn set_optimization_level(self, opt_level: CompilerOptimizationLevel) -> Self {
        unsafe {
            wasmedge::WasmEdge_ConfigureCompilerSetOptimizationLevel(self.ctx, opt_level as u32)
        };
        self
    }

    /// Get the optimization level of AOT compiler.
    pub fn get_optimization_level(&self) -> CompilerOptimizationLevel {
        let level = unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOptimizationLevel(self.ctx) };
        level.into()
    }

    /// Set the output binary format of AOT compiler.
    pub fn set_compiler_output_format(self, format: CompilerOutputFormat) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetOutputFormat(self.ctx, format as u32) };
        self
    }

    /// Get the output binary format of AOT compiler.
    pub fn get_compiler_output_format(&self) -> CompilerOutputFormat {
        let value = unsafe { wasmedge::WasmEdge_ConfigureCompilerGetOutputFormat(self.ctx) };
        value.into()
    }

    /// Set the dump IR boolean value of AOT compiler.
    pub fn dump_ir(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetDumpIR(self.ctx, enable) };
        self
    }

    /// Get the dump IR option of AOT compiler.
    pub fn is_dump_ir(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsDumpIR(self.ctx) }
    }

    /// Set the generic binary option of AOT compiler.
    pub fn generic_binary(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetGenericBinary(self.ctx, enable) };
        self
    }

    /// Get the generic binary option of AOT compiler.
    pub fn is_generic_binary(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerIsGenericBinary(self.ctx) }
    }

    /// Enable or disable instruction counting.
    pub fn count_instructions(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetInstructionCounting(self.ctx, enable) };
        self
    }

    /// Get the instruction counting option.
    pub fn is_instruction_counting(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsInstructionCounting(self.ctx) }
    }

    /// Enable or disable cost cost measuring.
    pub fn measure_costs(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetCostMeasuring(self.ctx, enable) };
        self
    }

    /// Get the cost measuring option.
    pub fn is_cost_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsCostMeasuring(self.ctx) }
    }

    /// Set the time measuring option.
    pub fn measure_time(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsSetTimeMeasuring(self.ctx, enable) };
        self
    }

    /// Get the cost measuring option.
    pub fn is_time_measuring(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ConfigureStatisticsIsTimeMeasuring(self.ctx) }
    }
}

// # TODO: WasmEdge_HostRegistration
