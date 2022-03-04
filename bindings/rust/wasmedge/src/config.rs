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

    pub fn copy_from(src: &Config) -> Result<Self> {
        let inner = wasmedge::Config::copy_from(&src.inner)?;
        Ok(Self { inner })
    }

    pub fn wasi(&mut self, enable: bool) {
        self.inner.wasi(enable);
    }

    pub fn wasi_enabled(&self) -> bool {
        self.inner.wasi_enabled()
    }

    pub fn wasmedge_process(&mut self, enable: bool) {
        self.inner.wasmedge_process(enable)
    }

    pub fn wasmedge_process_enabled(&self) -> bool {
        self.inner.wasmedge_process_enabled()
    }

    pub fn set_max_memory_pages(&mut self, count: u32) {
        self.inner.set_max_memory_pages(count)
    }

    pub fn get_max_memory_pages(&self) -> u32 {
        self.inner.get_max_memory_pages()
    }

    pub fn mutable_globals(&mut self, enable: bool) {
        self.inner.mutable_globals(enable)
    }

    pub fn mutable_globals_enabled(&self) -> bool {
        self.inner.mutable_globals_enabled()
    }

    pub fn non_trap_conversions(&mut self, enable: bool) {
        self.inner.non_trap_conversions(enable)
    }

    pub fn non_trap_conversions_enabled(&self) -> bool {
        self.inner.non_trap_conversions_enabled()
    }

    pub fn sign_extension_operators(&mut self, enable: bool) {
        self.inner.sign_extension_operators(enable)
    }

    pub fn sign_extension_operators_enabled(&self) -> bool {
        self.inner.sign_extension_operators_enabled()
    }

    pub fn multi_value(&mut self, enable: bool) {
        self.inner.multi_value(enable)
    }

    pub fn multi_value_enabled(&self) -> bool {
        self.inner.multi_value_enabled()
    }

    pub fn bulk_memory_operations(&mut self, enable: bool) {
        self.inner.bulk_memory_operations(enable)
    }

    pub fn bulk_memory_operations_enabled(&self) -> bool {
        self.inner.bulk_memory_operations_enabled()
    }

    pub fn reference_types(&mut self, enable: bool) {
        self.inner.reference_types(enable)
    }

    pub fn reference_types_enabled(&self) -> bool {
        self.inner.reference_types_enabled()
    }

    pub fn simd(&mut self, enable: bool) {
        self.inner.simd(enable)
    }

    pub fn simd_enabled(&self) -> bool {
        self.inner.simd_enabled()
    }

    pub fn tail_call(&mut self, enable: bool) {
        self.inner.tail_call(enable)
    }

    pub fn tail_call_enabled(&self) -> bool {
        self.inner.tail_call_enabled()
    }

    pub fn annotations(&mut self, enable: bool) {
        self.inner.annotations(enable)
    }

    pub fn annotations_enabled(&self) -> bool {
        self.inner.annotations_enabled()
    }

    pub fn memory64(&mut self, enable: bool) {
        self.inner.memory64(enable)
    }

    pub fn memory64_enabled(&self) -> bool {
        self.inner.memory64_enabled()
    }

    pub fn threads(&mut self, enable: bool) {
        self.inner.threads(enable)
    }

    pub fn threads_enabled(&self) -> bool {
        self.inner.threads_enabled()
    }

    pub fn exception_handling(&mut self, enable: bool) {
        self.inner.exception_handling(enable)
    }

    pub fn exception_handling_enabled(&self) -> bool {
        self.inner.exception_handling_enabled()
    }

    pub fn function_references(&mut self, enable: bool) {
        self.inner.function_references(enable)
    }

    pub fn function_references_enabled(&self) -> bool {
        self.inner.function_references_enabled()
    }

    // For AOT Compiler

    pub fn set_aot_opt_level(&mut self, opt_level: CompilerOptimizationLevel) {
        self.inner.set_aot_optimization_level(opt_level)
    }

    pub fn get_aot_opt_level(&self) -> CompilerOptimizationLevel {
        self.inner.get_aot_optimization_level()
    }

    pub fn set_aot_out_format(&mut self, format: CompilerOutputFormat) {
        self.inner.set_aot_compiler_output_format(format)
    }

    pub fn get_aot_out_format(&self) -> CompilerOutputFormat {
        self.inner.get_aot_compiler_output_format()
    }

    pub fn aot_dump_ir(&mut self, enable: bool) {
        self.inner.aot_dump_ir(enable)
    }

    pub fn is_aot_dump_ir(&self) -> bool {
        self.inner.is_aot_dump_ir()
    }

    pub fn aot_generic_binary(&mut self, enable: bool) {
        self.inner.aot_generic_binary(enable)
    }

    pub fn is_aot_generic_binary(&self) -> bool {
        self.inner.is_aot_generic_binary()
    }

    pub fn aot_interruptible(&mut self, enable: bool) {
        self.inner.aot_interruptible(enable)
    }

    pub fn is_aot_interruptible(&self) -> bool {
        self.inner.aot_interruptible_enabled()
    }

    // For Statistics

    pub fn aot_instr_counting(&mut self, enable: bool) {
        self.inner.aot_count_instructions(enable)
    }

    pub fn is_aot_instruction_counting(&self) -> bool {
        self.inner.is_aot_instruction_counting()
    }

    pub fn aot_cost_measuring(&mut self, enable: bool) {
        self.inner.aot_measure_cost(enable)
    }

    pub fn is_aot_cost_measuring(&self) -> bool {
        self.inner.is_aot_cost_measuring()
    }

    pub fn aot_time_measuring(&mut self, enable: bool) {
        self.inner.aot_measure_cost(enable)
    }

    pub fn is_aot_time_measuring(&self) -> bool {
        self.inner.is_aot_time_measuring()
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
            let mut config = result.unwrap();
            config.wasi(true);
            config.wasmedge_process(true);

            assert!(config.wasi_enabled());
            assert!(config.wasmedge_process_enabled());
        }

        {
            let result = Config::new();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.annotations(true);
            config.bulk_memory_operations(false);
            config.exception_handling(true);
            // config.function_references()
            config.set_max_memory_pages(20);
            config.memory64(true);
            config.multi_value(false);
            config.mutable_globals(false);
            config.non_trap_conversions(false);
            config.reference_types(false);
            config.sign_extension_operators(false);
            config.simd(false);
            config.tail_call(true);
            config.threads(true);

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

    #[test]
    fn test_config_copy() {
        let result = Config::new();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.memory64(true);
        config.set_max_memory_pages(520);
        config.set_aot_opt_level(CompilerOptimizationLevel::O0);

        // make a copy
        let result = Config::copy_from(&config);
        assert!(result.is_ok());
        let config_copied = result.unwrap();
        assert!(config_copied.memory64_enabled());
        assert_eq!(config_copied.get_max_memory_pages(), 520);
        assert_eq!(
            config_copied.get_aot_opt_level(),
            CompilerOptimizationLevel::O0
        );
    }
}
