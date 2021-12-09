use crate::{wasmedge, Error, WasmEdgeResult};

pub struct Statistics {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StatisticsContext,
    pub(crate) registered: bool,
}
impl Statistics {
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

    /// Get the instruction count in execution.
    pub fn get_instr_count(&self) -> u64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrCount(self.ctx) }
    }

    /// Get the instruction count per second in execution.
    pub fn get_instr_per_second(&self) -> f64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrPerSecond(self.ctx) }
    }

    /// Get the total cost in execution.
    pub fn get_total_cost(&self) -> u64 {
        unsafe { wasmedge::WasmEdge_StatisticsGetTotalCost(self.ctx) }
    }

    /// Set the costs of instructions.
    pub fn set_cost_table(&mut self, cost_arr: &mut [u64]) {
        unsafe {
            wasmedge::WasmEdge_StatisticsSetCostTable(
                self.ctx,
                cost_arr.as_mut_ptr(),
                cost_arr.len() as u32,
            )
        }
    }

    /// Set the cost limit in execution.
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
