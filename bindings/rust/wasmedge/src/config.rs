use crate::{error::Result, wasmedge, CompilerOptimizationLevel, CompilerOutputFormat};

#[derive(Debug)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}
impl Config {
    pub fn copy_from(src: &Config) -> Result<Self> {
        let inner = wasmedge::Config::copy_from(&src.inner)?;
        Ok(Self { inner })
    }

    pub fn wasi_enabled(&self) -> bool {
        self.inner.wasi_enabled()
    }

    pub fn wasmedge_process_enabled(&self) -> bool {
        self.inner.wasmedge_process_enabled()
    }

    pub fn max_memory_pages(&self) -> u32 {
        self.inner.get_max_memory_pages()
    }

    pub fn mutable_globals_enabled(&self) -> bool {
        self.inner.mutable_globals_enabled()
    }

    pub fn non_trap_conversions_enabled(&self) -> bool {
        self.inner.non_trap_conversions_enabled()
    }

    pub fn sign_extension_operators_enabled(&self) -> bool {
        self.inner.sign_extension_operators_enabled()
    }

    pub fn multi_value_enabled(&self) -> bool {
        self.inner.multi_value_enabled()
    }

    pub fn bulk_memory_operations_enabled(&self) -> bool {
        self.inner.bulk_memory_operations_enabled()
    }

    pub fn reference_types_enabled(&self) -> bool {
        self.inner.reference_types_enabled()
    }

    pub fn simd_enabled(&self) -> bool {
        self.inner.simd_enabled()
    }

    pub fn tail_call_enabled(&self) -> bool {
        self.inner.tail_call_enabled()
    }

    pub fn annotations_enabled(&self) -> bool {
        self.inner.annotations_enabled()
    }

    pub fn memory64_enabled(&self) -> bool {
        self.inner.memory64_enabled()
    }

    pub fn threads_enabled(&self) -> bool {
        self.inner.threads_enabled()
    }

    pub fn exception_handling_enabled(&self) -> bool {
        self.inner.exception_handling_enabled()
    }

    pub fn function_references_enabled(&self) -> bool {
        self.inner.function_references_enabled()
    }

    // For AOT Compiler

    pub fn optimization_level(&self) -> CompilerOptimizationLevel {
        self.inner.get_aot_optimization_level()
    }

    pub fn out_format(&self) -> CompilerOutputFormat {
        self.inner.get_aot_compiler_output_format()
    }

    pub fn dump_ir(&self) -> bool {
        self.inner.is_aot_dump_ir()
    }

    pub fn generic_binary(&self) -> bool {
        self.inner.is_aot_generic_binary()
    }

    pub fn interruptible(&self) -> bool {
        self.inner.aot_interruptible_enabled()
    }

    // For Statistics

    pub fn is_instruction_counting(&self) -> bool {
        self.inner.is_instruction_counting()
    }

    pub fn is_cost_measuring(&self) -> bool {
        self.inner.is_cost_measuring()
    }

    pub fn is_time_measuring(&self) -> bool {
        self.inner.is_time_measuring()
    }
}

#[derive(Debug)]
pub struct CommonConfigOptions {
    mutable_globals: bool,
    non_trap_conversions: bool,
    sign_extension_operators: bool,
    multi_value: bool,
    bulk_memory_operations: bool,
    reference_types: bool,
    simd: bool,
}
impl CommonConfigOptions {
    pub fn new() -> Self {
        Self {
            mutable_globals: true,
            non_trap_conversions: true,
            sign_extension_operators: true,
            multi_value: true,
            bulk_memory_operations: true,
            reference_types: true,
            simd: true,
        }
    }

    pub fn mutable_globals(self, enable: bool) -> Self {
        Self {
            mutable_globals: enable,
            ..self
        }
    }

    pub fn non_trap_conversions(self, enable: bool) -> Self {
        Self {
            non_trap_conversions: enable,
            ..self
        }
    }

    pub fn sign_extension_operators(self, enable: bool) -> Self {
        Self {
            sign_extension_operators: enable,
            ..self
        }
    }

    pub fn multi_value(self, enable: bool) -> Self {
        Self {
            multi_value: enable,
            ..self
        }
    }

    pub fn bulk_memory_operations(self, enable: bool) -> Self {
        Self {
            bulk_memory_operations: enable,
            ..self
        }
    }

    pub fn reference_types(self, enable: bool) -> Self {
        Self {
            reference_types: enable,
            ..self
        }
    }

    pub fn simd(self, enable: bool) -> Self {
        Self {
            simd: enable,
            ..self
        }
    }
}
impl Default for CommonConfigOptions {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug)]
pub struct CompilerConfigOptions {
    out_format: CompilerOutputFormat,
    opt_level: CompilerOptimizationLevel,
    dump_ir: bool,
    generic_binary: bool,
    interruptible: bool,
}
impl CompilerConfigOptions {
    pub fn new() -> Self {
        Self {
            out_format: CompilerOutputFormat::Wasm,
            opt_level: CompilerOptimizationLevel::O3,
            dump_ir: false,
            generic_binary: false,
            interruptible: false,
        }
    }

    pub fn out_format(self, format: CompilerOutputFormat) -> Self {
        Self {
            out_format: format,
            ..self
        }
    }

    pub fn optimization_level(self, level: CompilerOptimizationLevel) -> Self {
        Self {
            opt_level: level,
            ..self
        }
    }

    pub fn dump_ir(self, enable: bool) -> Self {
        Self {
            dump_ir: enable,
            ..self
        }
    }

    pub fn generic_binary(self, enable: bool) -> Self {
        Self {
            generic_binary: enable,
            ..self
        }
    }

    pub fn interruptible(self, enable: bool) -> Self {
        Self {
            interruptible: enable,
            ..self
        }
    }
}
impl Default for CompilerConfigOptions {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug)]
pub struct RuntimeConfigOptions {
    max_memory_pages: u32,
}
impl RuntimeConfigOptions {
    pub fn new() -> Self {
        Self {
            max_memory_pages: 65536,
        }
    }

    pub fn max_memory_pages(self, pages: u32) -> Self {
        Self {
            max_memory_pages: pages,
        }
    }
}
impl Default for RuntimeConfigOptions {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug, Default)]
pub struct StatisticsConfigOptions {
    count_instructions: bool,
    measure_cost: bool,
    measure_time: bool,
}
impl StatisticsConfigOptions {
    pub fn new() -> Self {
        Self::default()
    }

    /// Sets the instruction counting option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if support instruction counting or not when execution after AOT compilation.
    pub fn count_instructions(self, enable: bool) -> Self {
        Self {
            count_instructions: enable,
            ..self
        }
    }

    /// Sets the cost measuring option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if support cost measuring or not when execution after AOT compilation.
    pub fn measure_cost(self, enable: bool) -> Self {
        Self {
            measure_cost: enable,
            ..self
        }
    }

    /// Sets the time measuring option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if support time measuring or not when execution after AOT compilation.
    pub fn measure_time(self, enable: bool) -> Self {
        Self {
            measure_time: enable,
            ..self
        }
    }
}

#[derive(Debug, Default)]
pub struct HostRegistrationConfigOptions {
    wasi: bool,
    wasmedge_process: bool,
}
impl HostRegistrationConfigOptions {
    pub fn new() -> Self {
        Self {
            wasi: false,
            wasmedge_process: false,
        }
    }

    pub fn wasi(self, enable: bool) -> Self {
        Self {
            wasi: enable,
            ..self
        }
    }

    pub fn wasmedge_process(self, enable: bool) -> Self {
        Self {
            wasmedge_process: enable,
            ..self
        }
    }
}

#[derive(Debug)]
pub struct ConfigBuilder {
    common_config: CommonConfigOptions,
    stat_config: Option<StatisticsConfigOptions>,
    compiler_config: Option<CompilerConfigOptions>,
    runtim_config: Option<RuntimeConfigOptions>,
    host_config: Option<HostRegistrationConfigOptions>,
}
impl ConfigBuilder {
    pub fn new(common: CommonConfigOptions) -> Self {
        Self {
            common_config: common,
            stat_config: None,
            compiler_config: None,
            runtim_config: None,
            host_config: None,
        }
    }

    pub fn with_statistics_config(self, config: StatisticsConfigOptions) -> Self {
        Self {
            stat_config: Some(config),
            ..self
        }
    }

    pub fn with_runtime_config(self, config: RuntimeConfigOptions) -> Self {
        Self {
            runtim_config: Some(config),
            ..self
        }
    }

    pub fn with_compiler_config(self, config: CompilerConfigOptions) -> Self {
        Self {
            compiler_config: Some(config),
            ..self
        }
    }

    pub fn with_host_registration_config(self, config: HostRegistrationConfigOptions) -> Self {
        Self {
            host_config: Some(config),
            ..self
        }
    }

    pub fn build(self) -> Result<Config> {
        let mut inner = wasmedge::Config::create()?;
        inner.mutable_globals(self.common_config.mutable_globals);
        inner.non_trap_conversions(self.common_config.non_trap_conversions);
        inner.sign_extension_operators(self.common_config.sign_extension_operators);
        inner.multi_value(self.common_config.multi_value);
        inner.bulk_memory_operations(self.common_config.bulk_memory_operations);
        inner.reference_types(self.common_config.reference_types);
        inner.simd(self.common_config.simd);

        if let Some(stat_config) = self.stat_config {
            inner.count_instructions(stat_config.count_instructions);
            inner.measure_cost(stat_config.measure_cost);
            inner.measure_time(stat_config.measure_time);
        }
        if let Some(compiler_config) = self.compiler_config {
            inner.set_aot_compiler_output_format(compiler_config.out_format);
            inner.set_aot_optimization_level(compiler_config.opt_level);
            inner.aot_dump_ir(compiler_config.dump_ir);
            inner.aot_generic_binary(compiler_config.generic_binary);
            inner.aot_interruptible(compiler_config.interruptible);
        }
        if let Some(runtim_config) = self.runtim_config {
            inner.set_max_memory_pages(runtim_config.max_memory_pages);
        }
        if let Some(host_config) = self.host_config {
            inner.wasi(host_config.wasi);
            inner.wasmedge_process(host_config.wasmedge_process);
        }

        Ok(Config { inner })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{CompilerOptimizationLevel, CompilerOutputFormat};

    #[test]
    fn test_config_create() {
        let common_config = CommonConfigOptions::default()
            .bulk_memory_operations(true)
            .multi_value(true)
            .mutable_globals(true)
            .non_trap_conversions(true)
            .reference_types(true)
            .sign_extension_operators(true)
            .simd(true);

        let compiler_config = CompilerConfigOptions::default()
            .dump_ir(true)
            .generic_binary(true)
            .interruptible(true)
            .optimization_level(CompilerOptimizationLevel::O0)
            .out_format(CompilerOutputFormat::Native);

        let stat_config = StatisticsConfigOptions::default()
            .count_instructions(true)
            .measure_cost(true)
            .measure_time(true);

        let runtime_config = RuntimeConfigOptions::default().max_memory_pages(1024);

        let host_config = HostRegistrationConfigOptions::default()
            .wasi(true)
            .wasmedge_process(true);

        let result = ConfigBuilder::new(common_config)
            .with_statistics_config(stat_config)
            .with_compiler_config(compiler_config)
            .with_runtime_config(runtime_config)
            .with_host_registration_config(host_config)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // check common config options
        assert!(config.bulk_memory_operations_enabled());
        assert!(config.multi_value_enabled());
        assert!(config.mutable_globals_enabled());
        assert!(config.non_trap_conversions_enabled());
        assert!(config.reference_types_enabled());
        assert!(config.sign_extension_operators_enabled());
        assert!(config.simd_enabled());

        // check compiler config options
        assert!(config.dump_ir());
        assert!(config.generic_binary());
        assert!(config.interruptible());
        assert_eq!(config.optimization_level(), CompilerOptimizationLevel::O0);
        assert_eq!(config.out_format(), CompilerOutputFormat::Native);

        // check statistics config options
        assert!(config.is_instruction_counting());
        assert!(config.is_cost_measuring());
        assert!(config.is_time_measuring());

        // check runtime config options
        assert_eq!(config.max_memory_pages(), 1024);
    }

    #[test]
    fn test_config_copy() {
        let common_config = CommonConfigOptions::default().simd(false);
        let compiler_config =
            CompilerConfigOptions::default().optimization_level(CompilerOptimizationLevel::O0);
        let stat_config = StatisticsConfigOptions::default().measure_time(false);
        let runtime_config = RuntimeConfigOptions::default().max_memory_pages(1024);
        let host_config = HostRegistrationConfigOptions::default().wasi(true);

        let result = ConfigBuilder::new(common_config)
            .with_statistics_config(stat_config)
            .with_compiler_config(compiler_config)
            .with_runtime_config(runtime_config)
            .with_host_registration_config(host_config)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();
        assert!(!config.simd_enabled());
        assert_eq!(config.optimization_level(), CompilerOptimizationLevel::O0);
        assert!(!config.is_time_measuring());
        assert_eq!(config.max_memory_pages(), 1024);
        assert!(config.wasi_enabled());

        // make a copy
        let result = Config::copy_from(&config);
        assert!(result.is_ok());
        let config_copied = result.unwrap();
        assert!(!config_copied.simd_enabled());
        assert_eq!(
            config_copied.optimization_level(),
            CompilerOptimizationLevel::O0
        );
        assert!(!config.is_time_measuring());
        assert_eq!(config_copied.max_memory_pages(), 1024);
        assert!(config_copied.wasi_enabled());
    }
}
