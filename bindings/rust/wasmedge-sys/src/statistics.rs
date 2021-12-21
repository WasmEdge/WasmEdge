//! Defines WasmEdge Statistics struct.

use crate::{wasmedge, Error, WasmEdgeResult};

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
            true => Err(Error::OperationError(String::from(
                "fail to create Statistics instance",
            ))),
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
    pub fn set_cost_table(&mut self, cost_table: &mut [u64]) {
        unsafe {
            wasmedge::WasmEdge_StatisticsSetCostTable(
                self.ctx,
                cost_table.as_mut_ptr(),
                cost_table.len() as u32,
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_stat() {
        // create a Statistics instance
        let result = Statistics::create();
        assert!(result.is_ok());
        let stat = result.unwrap();

        // check instruction count
        let count = stat.instr_count();
        assert_eq!(stat.instr_count(), 0);

        // check instruction count per second
        let count = stat.instr_per_sec();
        assert!(count.is_nan());
        // assert!(count, 0.0);

        // check total cost
        assert_eq!(stat.cost_in_total(), 0);
    }
}
