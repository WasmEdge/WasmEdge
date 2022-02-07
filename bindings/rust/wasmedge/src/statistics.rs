use crate::{wasmedge, Vm};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Statistics<'vm> {
    pub(crate) inner: wasmedge::Statistics,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> Statistics<'vm> {
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
