use wasmedge_sys as ffi;

pub struct Config {
    pub(crate) ctx: *mut ffi::WasmEdge_ConfigureContext,
}

impl Drop for Config {
    fn drop(&mut self) {
        unsafe { ffi::WasmEdge_ConfigureDelete(self.ctx) };
    }
}

impl Config {
    pub fn new() -> Self {
        Self::default()
    }
}

impl Default for Config {
    fn default() -> Self {
        let ctx = unsafe { ffi::WasmEdge_ConfigureCreate() };
        if ctx.is_null() {
            panic!("failed to create WasmEdge configuration");
        }
        Self { ctx }
    }
}

macro_rules! impl_proposal_config {
    ($( $proposal:ident ),+ $(,)+) => {
        impl Config {
            paste::paste! {
                $(
                    pub fn [<enable_ $proposal:lower:snake>](self, enable: bool) -> Self {
                        let prop = ffi::[<WasmEdge_Proposal_ $proposal>];
                        if enable {
                            unsafe { ffi::WasmEdge_ConfigureAddProposal(self.ctx, prop) };
                        } else {
                            unsafe { ffi::WasmEdge_ConfigureRemoveProposal(self.ctx, prop) };
                        }
                        self
                    }

                    pub fn [<has_ $proposal:lower:snake>](self) -> bool {
                        let prop = ffi::[<WasmEdge_Proposal_ $proposal>];
                        unsafe { ffi::WasmEdge_ConfigureHasProposal(self.ctx, prop) }
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
    pub fn opt_level(self, opt_level: OptLevel) -> Self {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetOptimizationLevel(self.ctx, opt_level as u32) };
        self
    }

    /// Set the maximum number of memory pages available to running modules.
    pub fn max_memory_pages(self, num_pages: u32) -> Self {
        unsafe { ffi::WasmEdge_ConfigureSetMaxMemoryPage(self.ctx, num_pages) };
        self
    }

    pub fn dump_ir(self, enable: bool) -> Self {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetDumpIR(self.ctx, enable) };
        self
    }

    /// Enable or disable compile instruction counting.
    pub fn count_instructions(self, enable: bool) -> Self {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetInstructionCounting(self.ctx, enable) };
        self
    }

    /// Enable or disable compiler cost cost measuring.
    pub fn measure_costs(self, enable: bool) -> Self {
        unsafe { ffi::WasmEdge_ConfigureCompilerSetCostMeasuring(self.ctx, enable) };
        self
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum OptLevel {
    /// Disable as many optimizations as possible.
    O0 = ffi::WasmEdge_CompilerOptimizationLevel_O0,

    /// Optimize quickly without destroying debuggability.
    O1 = ffi::WasmEdge_CompilerOptimizationLevel_O1,

    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    O2 = ffi::WasmEdge_CompilerOptimizationLevel_O2,

    ///  Optimize for fast execution as much as possible.
    O3 = ffi::WasmEdge_CompilerOptimizationLevel_O3,

    ///  Optimize for small code size as much as possible without triggering
    ///  significant incremental compile time or execution time slowdowns.
    Os = ffi::WasmEdge_CompilerOptimizationLevel_Os,

    /// Optimize for small code size as much as possible.
    Oz = ffi::WasmEdge_CompilerOptimizationLevel_Oz,
}
