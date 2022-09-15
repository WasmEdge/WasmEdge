//! Defines WasmEdge Statistics struct.

use crate::{error::WasmEdgeError, ffi, WasmEdgeResult};

#[derive(Debug)]
/// Struct of WasmEdge Statistics.
pub struct Statistics {
    pub(crate) inner: InnerStat,
    pub(crate) registered: bool,
}
impl Statistics {
    /// Creates a new [Statistics].
    ///
    /// # Error
    ///
    /// If fail to create a [Statistics], then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_StatisticsCreate() };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::StatisticsCreate)),
            false => Ok(Statistics {
                inner: InnerStat(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the instruction count in execution.
    pub fn instr_count(&self) -> u64 {
        unsafe { ffi::WasmEdge_StatisticsGetInstrCount(self.inner.0) }
    }

    /// Returns the instruction count per second in execution.
    ///
    /// # Notice
    ///
    /// For the following cases,
    /// * [Statistics] is not enabled, or
    /// * the total execution time is 0
    ///
    /// The instructions per second could be `NaN`, which represents `divided-by-zero`.
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
        unsafe { ffi::WasmEdge_StatisticsGetInstrPerSecond(self.inner.0) }
    }

    /// Returns the total cost in execution.
    pub fn cost_in_total(&self) -> u64 {
        unsafe { ffi::WasmEdge_StatisticsGetTotalCost(self.inner.0) }
    }

    /// Sets the cost of instructions.
    ///
    /// # Arguments
    ///
    /// * `cost_table` - The slice of cost table.
    pub fn set_cost_table(&mut self, cost_table: impl AsRef<[u64]>) {
        unsafe {
            ffi::WasmEdge_StatisticsSetCostTable(
                self.inner.0,
                cost_table.as_ref().as_ptr() as *mut _,
                cost_table.as_ref().len() as u32,
            )
        }
    }

    /// Sets the cost limit in execution.
    ///
    /// # Arguments
    ///
    /// * `limit` - The cost limit.
    pub fn set_cost_limit(&mut self, limit: u64) {
        unsafe { ffi::WasmEdge_StatisticsSetCostLimit(self.inner.0, limit) }
    }

    /// Clears the data in this statistics.
    pub fn clear(&mut self) {
        unsafe { ffi::WasmEdge_StatisticsClear(self.inner.0) }
    }
}
impl Drop for Statistics {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_StatisticsDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerStat(pub(crate) *mut ffi::WasmEdge_StatisticsContext);
unsafe impl Send for InnerStat {}
unsafe impl Sync for InnerStat {}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_stat_send() {
        let result = Statistics::create();
        assert!(result.is_ok());
        let stat = result.unwrap();

        let handle = thread::spawn(move || {
            assert!(!stat.inner.0.is_null());
            println!("{:?}", stat.inner);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_stat_sync() {
        let result = Statistics::create();
        assert!(result.is_ok());
        let stat = Arc::new(Mutex::new(result.unwrap()));

        let stat_cloned = Arc::clone(&stat);
        let handle = thread::spawn(move || {
            let result = stat_cloned.lock();
            assert!(result.is_ok());
            let stat = result.unwrap();

            assert!(!stat.inner.0.is_null());
        });

        handle.join().unwrap();
    }
}
