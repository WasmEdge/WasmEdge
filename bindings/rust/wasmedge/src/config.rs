use crate::{error::Result, wasmedge, CompilerOptimizationLevel, CompilerOutputFormat};

#[derive(Debug)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}
impl Config {
    pub fn new() -> Result<Self> {
        let inner = wasmedge::Config::create()?;
        Ok(Self { inner })
    }

    pub fn wasi(self, enable: bool) -> Self {
        let inner = self.inner.wasi(enable);
        Self { inner }
    }

    pub fn wasi_enabled(&self) -> bool {
        self.inner.wasi_enabled()
    }

    pub fn wasmedge_process(self, enable: bool) -> Self {
        let inner = self.inner.wasmedge_process(enable);
        Self { inner }
    }

    pub fn wasmedge_process_enabled(&self) -> bool {
        self.inner.wasmedge_process_enabled()
    }

    pub fn set_max_memory_pages(self, count: u32) -> Self {
        let inner = self.inner.set_max_memory_pages(count);
        Self { inner }
    }

    pub fn get_max_memory_pages(&self) -> u32 {
        self.inner.get_max_memory_pages()
    }

    pub fn mutable_globals(self, enable: bool) -> Self {
        let inner = self.inner.mutable_globals(enable);
        Self { inner }
    }

    pub fn mutable_globals_enabled(&self) -> bool {
        self.inner.mutable_globals_enabled()
    }

    pub fn non_trap_conversions(self, enable: bool) -> Self {
        let inner = self.inner.non_trap_conversions(enable);
        Self { inner }
    }

    pub fn non_trap_conversions_enabled(&self) -> bool {
        self.inner.non_trap_conversions_enabled()
    }

    pub fn sign_extension_operators(self, enable: bool) -> Self {
        let inner = self.inner.sign_extension_operators(enable);
        Self { inner }
    }

    pub fn sign_extension_operators_enabled(&self) -> bool {
        self.inner.sign_extension_operators_enabled()
    }

    pub fn multi_value(self, enable: bool) -> Self {
        let inner = self.inner.multi_value(enable);
        Self { inner }
    }

    pub fn multi_value_enabled(&self) -> bool {
        self.inner.multi_value_enabled()
    }

    pub fn bulk_memory_operations(self, enable: bool) -> Self {
        let inner = self.inner.bulk_memory_operations(enable);
        Self { inner }
    }

    pub fn bulk_memory_operations_enabled(&self) -> bool {
        self.inner.bulk_memory_operations_enabled()
    }

    pub fn reference_types(self, enable: bool) -> Self {
        let inner = self.inner.reference_types(enable);
        Self { inner }
    }

    pub fn reference_types_enabled(&self) -> bool {
        self.inner.reference_types_enabled()
    }

    pub fn simd(self, enable: bool) -> Self {
        let inner = self.inner.simd(enable);
        Self { inner }
    }

    pub fn simd_enabled(&self) -> bool {
        self.inner.simd_enabled()
    }

    pub fn tail_call(self, enable: bool) -> Self {
        let inner = self.inner.tail_call(enable);
        Self { inner }
    }

    pub fn tail_call_enabled(&self) -> bool {
        self.inner.tail_call_enabled()
    }

    pub fn annotations(self, enable: bool) -> Self {
        let inner = self.inner.annotations(enable);
        Self { inner }
    }

    pub fn annotations_enabled(&self) -> bool {
        self.inner.annotations_enabled()
    }

    pub fn memory64(self, enable: bool) -> Self {
        let inner = self.inner.memory64(enable);
        Self { inner }
    }

    pub fn memory64_enabled(&self) -> bool {
        self.inner.memory64_enabled()
    }

    pub fn threads(self, enable: bool) -> Self {
        let inner = self.inner.threads(enable);
        Self { inner }
    }

    pub fn threads_enabled(&self) -> bool {
        self.inner.threads_enabled()
    }

    pub fn exception_handling(self, enable: bool) -> Self {
        let inner = self.inner.exception_handling(enable);
        Self { inner }
    }

    pub fn exception_handling_enabled(&self) -> bool {
        self.inner.exception_handling_enabled()
    }

    pub fn function_references(self, enable: bool) -> Self {
        let inner = self.inner.function_references(enable);
        Self { inner }
    }

    pub fn function_references_enabled(&self) -> bool {
        self.inner.function_references_enabled()
    }

    pub fn set_aot_opt_level(self, opt_level: CompilerOptimizationLevel) -> Self {
        let inner = self.inner.set_optimization_level(opt_level);
        Self { inner }
    }

    pub fn get_aot_opt_level(&self) -> CompilerOptimizationLevel {
        self.inner.get_optimization_level()
    }

    pub fn set_aot_out_format(self, format: CompilerOutputFormat) -> Self {
        let inner = self.inner.set_compiler_output_format(format);
        Self { inner }
    }

    pub fn get_aot_out_format(&self) -> CompilerOutputFormat {
        self.inner.get_compiler_output_format()
    }

    pub fn aot_dump_ir(self, enable: bool) -> Self {
        let inner = self.inner.dump_ir(enable);
        Self { inner }
    }

    pub fn is_aot_dump_ir(&self) -> bool {
        self.inner.is_dump_ir()
    }

    pub fn aot_generic_binary(self, enable: bool) -> Self {
        let inner = self.inner.generic_binary(enable);
        Self { inner }
    }

    pub fn is_aot_generic_binary(&self) -> bool {
        self.inner.is_generic_binary()
    }

    pub fn aot_instr_counting(self, enable: bool) -> Self {
        let inner = self.inner.count_instructions(enable);
        Self { inner }
    }

    pub fn is_aot_instruction_counting(&self) -> bool {
        self.inner.is_instruction_counting()
    }

    pub fn aot_cost_measuring(self, enable: bool) -> Self {
        let inner = self.inner.measure_cost(enable);
        Self { inner }
    }

    pub fn is_aot_cost_measuring(&self) -> bool {
        self.inner.is_cost_measuring()
    }

    pub fn aot_time_measuring(self, enable: bool) -> Self {
        let inner = self.inner.measure_cost(enable);
        Self { inner }
    }

    pub fn is_aot_time_measuring(&self) -> bool {
        self.inner.is_time_measuring()
    }

    pub fn aot_interruptible(self, enable: bool) -> Self {
        let inner = self.inner.interruptible(enable);
        Self { inner }
    }

    pub fn is_aot_interruptible(&self) -> bool {
        self.inner.interruptible_enabled()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_config_create() {
        {
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap();

            assert!(!config.annotations_enabled());
            assert!(config.bulk_memory_operations_enabled());
            assert!(!config.exception_handling_enabled());
            // assert!(config.function_references_enabled());
            assert_eq!(config.get_max_memory_pages(), 65536);
            assert!(!config.memory64_enabled());
            assert!(config.multi_value_enabled());
            assert!(config.mutable_globals_enabled());
            assert!(config.non_trap_conversions_enabled());
            assert!(config.reference_types_enabled());
            assert!(config.sign_extension_operators_enabled());
            assert!(config.simd_enabled());
            assert!(!config.tail_call_enabled());
            assert!(!config.threads_enabled());
        }

        {
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap().wasi(true).wasmedge_process(true);

            assert!(config.wasi_enabled());
            assert!(config.wasmedge_process_enabled());
        }

        {
            let result = Config::new();
            assert!(result.is_ok());
            let config = result
                .unwrap()
                .annotations(true)
                .bulk_memory_operations(false)
                .exception_handling(true)
                // .function_references()
                .set_max_memory_pages(20)
                .memory64(true)
                .multi_value(false)
                .mutable_globals(false)
                .non_trap_conversions(false)
                .reference_types(false)
                .sign_extension_operators(false)
                .simd(false)
                .tail_call(true)
                .threads(true);

            assert!(config.annotations_enabled());
            assert!(!config.bulk_memory_operations_enabled());
            assert!(config.exception_handling_enabled());
            // assert!(config.function_references_enabled());
            assert_eq!(config.get_max_memory_pages(), 20);
            assert!(config.memory64_enabled());
            assert!(!config.multi_value_enabled());
            assert!(!config.mutable_globals_enabled());
            assert!(!config.non_trap_conversions_enabled());
            assert!(!config.reference_types_enabled());
            assert!(!config.sign_extension_operators_enabled());
            assert!(!config.simd_enabled());
            assert!(config.tail_call_enabled());
            assert!(config.threads_enabled());
        }
    }
}
