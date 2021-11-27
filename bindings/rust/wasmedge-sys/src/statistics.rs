use super::wasmedge;

pub struct Statistics {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StatisticsContext,
}
impl Statistics {
    pub fn create() -> Option<Self> {
        let raw = unsafe { wasmedge::WasmEdge_StatisticsCreate() };
        match raw.is_null() {
            true => None,
            false => Some(Statistics { ctx: raw }),
        }
    }

    pub fn set_cost_table(&mut self, cost_arr: &mut [u64]) {
        unsafe {
            wasmedge::WasmEdge_StatisticsSetCostTable(
                self.ctx,
                cost_arr.as_mut_ptr(),
                cost_arr.len() as u32,
            )
        }
    }

    pub fn set_cost_limit(&mut self, limit: u64) {
        unsafe { wasmedge::WasmEdge_StatisticsSetCostLimit(self.ctx, limit) }
    }

    pub fn get_instr_count(&self) -> usize {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrCount(self.ctx) as usize }
    }

    pub fn get_instr_per_second(&self) -> usize {
        unsafe { wasmedge::WasmEdge_StatisticsGetInstrPerSecond(self.ctx) as usize }
    }

    pub fn get_total_cost(&self) -> usize {
        unsafe { wasmedge::WasmEdge_StatisticsGetTotalCost(self.ctx) as usize }
    }
}
impl Drop for Statistics {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_StatisticsDelete(self.ctx) }
        }
    }
}
