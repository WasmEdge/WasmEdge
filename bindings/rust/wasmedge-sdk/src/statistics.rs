//! Defines WasmEdge Statistics struct.

use crate::WasmEdgeResult;
use wasmedge_sys as sys;

/// Used to collect statistics of the WasmEdge runtime, such as the count of instructions in execution.
#[derive(Debug, Clone)]
pub struct Statistics {
    pub(crate) inner: sys::Statistics,
}
impl Statistics {
    /// Creates a new [Statistics].
    ///
    /// # Error
    ///
    /// If fail to create a [Statistics], then an error is returned.
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = sys::Statistics::create()?;
        Ok(Self { inner })
    }

    /// Returns the instruction count in execution.
    pub fn count(&self) -> u64 {
        self.inner.instr_count()
    }

    /// Returns the instruction count per second in execution.
    ///
    /// # Notice
    ///
    /// For the following cases,
    /// - [Statistics] is not enabled, or
    /// - the total execution time is 0
    ///
    /// The instructions per second could be `NaN`, which represents `divided-by-zero`.
    /// Use the `is_nan` function of F64 to check the return value before use it,
    /// for example,
    ///
    /// ```
    /// use wasmedge_sdk::Statistics;
    ///
    /// // create a Statistics instance
    /// let stat = Statistics::new().expect("fail to create a Statistics");
    ///
    /// // check instruction count per second
    /// assert!(stat.count_per_second().is_nan());
    /// ```
    pub fn count_per_second(&self) -> f64 {
        self.inner.instr_per_sec()
    }

    /// Returns the total cost in execution.
    pub fn cost(&self) -> u64 {
        self.inner.cost_in_total()
    }

    /// Sets the cost of instructions.
    ///
    /// # Arguments
    ///
    /// - `cost_table` specifies the slice of cost table.
    pub fn set_cost_table(&mut self, cost_table: impl AsRef<[u64]>) {
        self.inner.set_cost_table(cost_table)
    }

    /// Sets the cost limit in execution.
    ///
    /// # Arguments
    ///
    /// - `limit` specifies the cost limit.
    pub fn set_cost_limit(&mut self, limit: u64) {
        self.inner.set_cost_limit(limit)
    }
}
