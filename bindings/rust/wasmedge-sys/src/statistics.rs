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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Config, Executor, Loader, Validator};

    #[test]
    fn test_stat() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        // enable Statistics
        let config = config
            .count_instructions(true)
            .measure_time(true)
            .measure_cost(true);

        // load module from a wasm file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // validate module
        let result = Validator::create(Some(&config));
        assert!(result.is_ok());
        let validator = result.unwrap();
        let result = validator.validate(&module);
        assert!(result.is_ok());

        // create a Statistics context
        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        // set cost table
        let mut cost_table = vec![20u64; 512];
        stat.set_cost_table(&mut cost_table);

        // set cost limit
        stat.set_cost_limit(100_000_000_000_000);

        // create an Executor context
        let result = Executor::create(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        // let executor = result.unwrap();

        // register an ImportObj module
        // let import_obj = create_extern_module("extern", false);

        // todo: Add more test code
    }

    // fn create_extern_module(name: impl AsRef<str>, wrapped: bool) -> ImportObj {
    //     // create an ImportObj module
    //     let result = ImportObj::create(name);
    //     assert!(result.is_ok());
    //     let import_obj = result.unwrap();

    //     // // create a FuncType instance
    //     // let params = vec![ValType::ExternRef, ValType::I32];
    //     // let returns = vec![ValType::I32];
    //     // let result = FuncType::create(params, returns);

    //     import_obj
    // }
}
