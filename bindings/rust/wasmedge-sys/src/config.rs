use super::wasmedge;
pub struct Config {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ConfigureContext,
    _private: (),
}

impl Drop for Config {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ConfigureDelete(self.ctx) };
    }
}

impl Config {
    pub fn new() -> Self {
        Self::default()
    }
}

impl Default for Config {
    fn default() -> Self {
        let ctx = unsafe { wasmedge::WasmEdge_ConfigureCreate() };
        if ctx.is_null() {
            panic!("failed to create WasmEdge configuration");
        }
        Self { ctx,  _private: ()}
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
///     pub fn has_bulkmemoryoperations(self) -> bool {
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

                    pub fn [<has_$proposal:lower:snake>](self) -> bool {
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

    // For Vm
    pub fn reg_wasi(self) -> Self{
        unsafe{wasmedge::WasmEdge_ConfigureAddHostRegistration(self.ctx, wasmedge::WasmEdge_HostRegistration_Wasi)};
        self
    }

    // For AOT compiler

    /// Set the optimization level of AOT compiler.
    pub fn opt_level(self, opt_level: OptLevel) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetOptimizationLevel(self.ctx, opt_level as u32) };
        self
    }

    /// Set the maximum number of memory pages available to running modules.
    pub fn max_memory_pages(self, num_pages: u32) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureSetMaxMemoryPage(self.ctx, num_pages) };
        self
    }

    /// Set the dump IR boolean value of AOT compiler.
    pub fn dump_ir(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetDumpIR(self.ctx, enable) };
        self
    }

    /// Enable or disable compile instruction counting.
    pub fn count_instructions(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetInstructionCounting(self.ctx, enable) };
        self
    }

    /// Enable or disable compiler cost cost measuring.
    pub fn measure_costs(self, enable: bool) -> Self {
        unsafe { wasmedge::WasmEdge_ConfigureCompilerSetCostMeasuring(self.ctx, enable) };
        self
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum OptLevel {
    /// Disable as many optimizations as possible.
    O0 = wasmedge::WasmEdge_CompilerOptimizationLevel_O0,

    /// Optimize quickly without destroying debuggability.
    O1 = wasmedge::WasmEdge_CompilerOptimizationLevel_O1,

    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    O2 = wasmedge::WasmEdge_CompilerOptimizationLevel_O2,

    ///  Optimize for fast execution as much as possible.
    O3 = wasmedge::WasmEdge_CompilerOptimizationLevel_O3,

    ///  Optimize for small code size as much as possible without triggering
    ///  significant incremental compile time or execution time slowdowns.
    Os = wasmedge::WasmEdge_CompilerOptimizationLevel_Os,

    /// Optimize for small code size as much as possible.
    Oz = wasmedge::WasmEdge_CompilerOptimizationLevel_Oz,
}


// # TODO: WasmEdge_HostRegistration
