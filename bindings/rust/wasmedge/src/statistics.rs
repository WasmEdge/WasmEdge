use crate::{error::Result, sys};

#[derive(Debug)]
pub struct Statistics {
    pub(crate) inner: sys::Statistics,
}
impl Statistics {
    pub fn new() -> Result<Self> {
        let inner = sys::Statistics::create()?;
        Ok(Self { inner })
    }

    pub fn count_of_instr(&self) -> u64 {
        self.inner.instr_count()
    }

    pub fn instr_per_sec(&self) -> f64 {
        self.inner.instr_per_sec()
    }

    pub fn cost_in_total(&self) -> u64 {
        self.inner.cost_in_total()
    }

    pub fn set_cost_table(&mut self, cost_table: impl AsRef<[u64]>) {
        self.inner.set_cost_table(cost_table)
    }

    pub fn set_cost_limit(&mut self, limit: u64) {
        self.inner.set_cost_limit(limit)
    }
}
