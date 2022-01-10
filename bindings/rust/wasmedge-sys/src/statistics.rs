//! Defines WasmEdge Statistics struct.

use crate::{wasmedge, WasmEdgeError, WasmEdgeResult};

/// Struct of WasmEdge Statistics.
pub struct Statistics {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StatisticsContext,
    pub(crate) registered: bool,
}
impl Statistics {
    /// Creates a new [`Statistics`].
    ///
    /// # Error
    ///
    /// If fail to create a [`Statistics`], then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_StatisticsCreate() };
        match ctx.is_null() {
            true => Err(WasmEdgeError::StatisticsCreate),
            false => Ok(Statistics {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the instruction count in execution.
    pub fn instr_count(&self) -> u64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrCount(self.ctx) }
    }

    /// Returns the instruction count per second in execution.
    ///
    /// # Notice
    ///
    /// For the following cases,
    /// - [`Statistics`] is not enabled, or
    /// - the total execution time is 0
    ///
    /// the instructions per second is `NaN`, which represents `divided-by-zero`.
    /// Use the `is_nan` function of F64 to check the return value before use it,
    /// for example,
    ///
    /// ```
    /// use wasmedge_sys::Statistics;
    ///
    /// // create a Statistics instance
    /// let stat = Statistics::create().expect("fail to create a Statistics");
    ///
    /// // check instruction count per second
    /// assert!(stat.instr_per_sec().is_nan());
    /// ```
    pub fn instr_per_sec(&self) -> f64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrPerSecond(self.ctx) }
    }

    /// Returns the total cost in execution.
    pub fn cost_in_total(&self) -> u64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetTotalCost(self.ctx) }
    }

    /// Sets the cost of instructions.
    ///
    /// # Arguments
    ///
    /// - `cost_table` specifies the slice of cost table.
    pub fn set_cost_table(&mut self, cost_table: &mut impl AsMut<[u64]>) {
        unsafe {
            wasmedge::WasmEdge_StatisticsSetCostTable(
                self.ctx,
                cost_table.as_mut().as_mut_ptr(),
                cost_table.as_mut().len() as u32,
            )
        }
    }

    /// Sets the cost limit in execution.
    ///
    /// # Arguments
    ///
    /// - `limit` specifies the cost limit.
    pub fn set_cost_limit(&mut self, limit: u64) {
        unsafe { wasmedge::WasmEdge_StatisticsSetCostLimit(self.ctx, limit) }
    }
}
impl Drop for Statistics {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_StatisticsDelete(self.ctx) }
        }
    }
}
