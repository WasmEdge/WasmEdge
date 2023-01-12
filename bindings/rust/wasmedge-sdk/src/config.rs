//! Defines the structs used to construct configurations.

use crate::WasmEdgeResult;
#[cfg(feature = "aot")]
use crate::{CompilerOptimizationLevel, CompilerOutputFormat};
use wasmedge_sys as sys;

/// Defines a builder for creating a [Config].
#[derive(Debug, Default)]
pub struct ConfigBuilder {
    common_config: CommonConfigOptions,
    stat_config: Option<StatisticsConfigOptions>,
    #[cfg(feature = "aot")]
    compiler_config: Option<CompilerConfigOptions>,
    runtime_config: Option<RuntimeConfigOptions>,
    host_config: Option<HostRegistrationConfigOptions>,
}
impl ConfigBuilder {
    /// Creates a new [ConfigBuilder] with the given [CommonConfigOptions] setting.
    pub fn new(options: CommonConfigOptions) -> Self {
        Self {
            common_config: options,
            stat_config: None,
            #[cfg(feature = "aot")]
            compiler_config: None,
            runtime_config: None,
            host_config: None,
        }
    }

    /// Sets the [StatisticsConfigOptions] for the [ConfigBuilder].
    ///
    /// # Argument
    ///
    /// - `options` specifies the [StatisticsConfigOptions] settings to set.
    pub fn with_statistics_config(self, options: StatisticsConfigOptions) -> Self {
        Self {
            stat_config: Some(options),
            ..self
        }
    }

    /// Sets the [RuntimeConfigOptions] for the [ConfigBuilder].
    ///
    /// # Argument
    ///
    /// - `options` specifies the [RuntimeConfigOptions] settings to set.
    pub fn with_runtime_config(self, options: RuntimeConfigOptions) -> Self {
        Self {
            runtime_config: Some(options),
            ..self
        }
    }

    /// Sets the [CompilerConfigOptions] for the [ConfigBuilder].
    ///
    /// # Argument
    ///
    /// - `options` specifies the [CompilerConfigOptions] settings to set.
    #[cfg(feature = "aot")]
    pub fn with_compiler_config(self, options: CompilerConfigOptions) -> Self {
        Self {
            compiler_config: Some(options),
            ..self
        }
    }

    /// Sets the [HostRegistrationConfigOptions] for the [ConfigBuilder].
    ///
    /// # Argument
    ///
    /// - `options` specifies the [HostRegistrationConfigOptions] settings to set.
    pub fn with_host_registration_config(self, options: HostRegistrationConfigOptions) -> Self {
        Self {
            host_config: Some(options),
            ..self
        }
    }

    /// Creates a new [Config] from the [ConfigBuilder].
    ///
    /// # Errors
    ///
    /// If fail to create a [Config], then an error is returned.
    pub fn build(self) -> WasmEdgeResult<Config> {
        let mut inner = sys::Config::create()?;
        inner.mutable_globals(self.common_config.mutable_globals);
        inner.non_trap_conversions(self.common_config.non_trap_conversions);
        inner.sign_extension_operators(self.common_config.sign_extension_operators);
        inner.multi_value(self.common_config.multi_value);
        inner.bulk_memory_operations(self.common_config.bulk_memory_operations);
        inner.reference_types(self.common_config.reference_types);
        inner.simd(self.common_config.simd);
        inner.multi_memories(self.common_config.multi_memories);
        inner.threads(self.common_config.threads);
        inner.tail_call(self.common_config.tail_call);
        inner.function_references(self.common_config.function_references);
        inner.interpreter_mode(self.common_config.interpreter_mode);

        if let Some(stat_config) = self.stat_config {
            inner.count_instructions(stat_config.count_instructions);
            inner.measure_cost(stat_config.measure_cost);
            inner.measure_time(stat_config.measure_time);
        }
        #[cfg(feature = "aot")]
        if let Some(compiler_config) = self.compiler_config {
            inner.set_aot_compiler_output_format(compiler_config.out_format);
            inner.set_aot_optimization_level(compiler_config.opt_level);
            inner.dump_ir(compiler_config.dump_ir);
            inner.generic_binary(compiler_config.generic_binary);
            inner.interruptible(compiler_config.interruptible);
        }
        if let Some(runtim_config) = self.runtime_config {
            inner.set_max_memory_pages(runtim_config.max_memory_pages);
        }
        if let Some(host_config) = self.host_config {
            inner.wasi(host_config.wasi);
            #[cfg(target_os = "linux")]
            inner.wasmedge_process(host_config.wasmedge_process);
            #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
            inner.wasi_nn(host_config.wasi_nn);
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            inner.wasi_crypto_common(host_config.wasi_crypto_common);
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            inner.wasi_crypto_asymmetric_common(host_config.wasi_crypto_asymmetric_common);
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            inner.wasi_crypto_symmetric(host_config.wasi_crypto_symmetric);
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            inner.wasi_crypto_kx(host_config.wasi_crypto_kx);
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            inner.wasi_crypto_signatures(host_config.wasi_crypto_signatures);
        }

        Ok(Config { inner })
    }
}

/// Defines [Config] struct used to check/set the configuration options.
///
/// # Example
///
/// The following code shows how to create a [Config] with [ConfigBuilder].
///
/// ```rust
///
/// use wasmedge_sdk::{config::{Config, ConfigBuilder, CommonConfigOptions, StatisticsConfigOptions, RuntimeConfigOptions, HostRegistrationConfigOptions}};
/// use wasmedge_types::{CompilerOutputFormat, CompilerOptimizationLevel};
///
/// let common_options = CommonConfigOptions::default()
///     .bulk_memory_operations(true)
///     .multi_value(true)
///     .mutable_globals(true)
///     .non_trap_conversions(true)
///     .reference_types(true)
///     .sign_extension_operators(true)
///     .simd(true);
///
/// let stat_options = StatisticsConfigOptions::default()
///     .count_instructions(true)
///     .measure_cost(true)
///     .measure_time(true);
///
/// let runtime_options = RuntimeConfigOptions::default().max_memory_pages(1024);
///
/// let host_options = HostRegistrationConfigOptions::default()
///     .wasi(true);
///
/// let result = ConfigBuilder::new(common_options)
///     .with_statistics_config(stat_options)
///     .with_runtime_config(runtime_options)
///     .with_host_registration_config(host_options)
///     .build();
/// assert!(result.is_ok());
/// let config = result.unwrap();
/// ```
#[derive(Debug, Clone)]
pub struct Config {
    pub(crate) inner: sys::Config,
}
impl Config {
    /// Checks if the host registration wasi option turns on or not.
    pub fn wasi_enabled(&self) -> bool {
        self.inner.wasi_enabled()
    }

    /// Checks if host registration wasmedge process turns on or not.
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_enabled(&self) -> bool {
        self.inner.wasmedge_process_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn_enabled(&self) -> bool {
        self.inner.wasi_nn_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common_enabled(&self) -> bool {
        self.inner.wasi_crypto_common_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common_enabled(&self) -> bool {
        self.inner.wasi_crypto_asymmetric_common_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric_enabled(&self) -> bool {
        self.inner.wasi_crypto_symmetric_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx_enabled(&self) -> bool {
        self.inner.wasi_crypto_kx_enabled()
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures_enabled(&self) -> bool {
        self.inner.wasi_crypto_signatures_enabled()
    }

    /// Returns the number of the memory pages available.
    pub fn max_memory_pages(&self) -> u32 {
        self.inner.get_max_memory_pages()
    }

    /// Checks if the ImportExportMutGlobals option turns on or not.
    pub fn mutable_globals_enabled(&self) -> bool {
        self.inner.mutable_globals_enabled()
    }

    /// Checks if the NonTrapFloatToIntConversions option turns on or not.
    pub fn non_trap_conversions_enabled(&self) -> bool {
        self.inner.non_trap_conversions_enabled()
    }

    /// Checks if the SignExtensionOperators option turns on or not.
    pub fn sign_extension_operators_enabled(&self) -> bool {
        self.inner.sign_extension_operators_enabled()
    }

    /// Checks if the MultiValue option turns on or not.
    pub fn multi_value_enabled(&self) -> bool {
        self.inner.multi_value_enabled()
    }

    /// Checks if the BulkMemoryOperations option turns on or not.
    pub fn bulk_memory_operations_enabled(&self) -> bool {
        self.inner.bulk_memory_operations_enabled()
    }

    /// Checks if the ReferenceTypes option turns on or not.
    pub fn reference_types_enabled(&self) -> bool {
        self.inner.reference_types_enabled()
    }

    /// Checks if the SIMD option turns on or not.
    pub fn simd_enabled(&self) -> bool {
        self.inner.simd_enabled()
    }

    /// Checks if the MultiMemories option turns on or not.
    pub fn multi_memories_enabled(&self) -> bool {
        self.inner.multi_memories_enabled()
    }

    /// Checks if the Threads option turns on or not.
    pub fn threads_enabled(&self) -> bool {
        self.inner.threads_enabled()
    }

    /// Checks if the TailCall option turns on or not.
    pub fn tail_call_enabled(&self) -> bool {
        self.inner.tail_call_enabled()
    }

    /// Checks if the FunctionReferences option turns on or not.
    pub fn function_references_enabled(&self) -> bool {
        self.inner.function_references_enabled()
    }

    /// Checks if the `ForceInterpreter` option turns on or not.
    pub fn interpreter_mode_enabled(&self) -> bool {
        self.inner.interpreter_mode_enabled()
    }

    /// Returns the optimization level of AOT compiler.
    #[cfg(feature = "aot")]
    pub fn optimization_level(&self) -> CompilerOptimizationLevel {
        self.inner.get_aot_optimization_level()
    }

    /// Returns the output binary format of AOT compiler.
    #[cfg(feature = "aot")]
    pub fn out_format(&self) -> CompilerOutputFormat {
        self.inner.get_aot_compiler_output_format()
    }

    /// Checks if the dump IR option turns on or not.
    #[cfg(feature = "aot")]
    pub fn dump_ir_enabled(&self) -> bool {
        self.inner.dump_ir_enabled()
    }

    /// Checks if the generic binary option of AOT compiler turns on or not.
    #[cfg(feature = "aot")]
    pub fn generic_binary_enabled(&self) -> bool {
        self.inner.generic_binary_enabled()
    }

    /// Checks if the `Interruptible` option of AOT Compiler turns on or not.
    #[cfg(feature = "aot")]
    pub fn interruptible_enabled(&self) -> bool {
        self.inner.interruptible_enabled()
    }

    /// Checks if the instruction counting option turns on or not.
    pub fn instruction_counting_enabled(&self) -> bool {
        self.inner.is_instruction_counting()
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn cost_measuring_enabled(&self) -> bool {
        self.inner.is_cost_measuring()
    }

    /// Checks if the cost measuring option turns on or not.
    pub fn time_measuring_enabled(&self) -> bool {
        self.inner.is_time_measuring()
    }
}

/// Defines the common configuration options.
///
/// [CommonConfigOptions] is used to set the common configuration options, which are
///     
///  - `ImportExportMutGlobals` supports mutable imported and exported globals.
///
///    Also see [Import/Export Mutable Globals Proposal](https://github.com/WebAssembly/mutable-global/blob/master/proposals/mutable-global/Overview.md#importexport-mutable-globals).
///
///  - `NonTrapFloatToIntConversions` supports the non-trapping float-to-int conversion.
///
///    Also see [Non-trapping Float-to-int Conversions Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/nontrapping-float-to-int-conversion/Overview.md).
///
///  - `SignExtensionOperators` supports new integer instructions for sign-extending 8-bit, 16-bit, and 32-bit values.
///     
///    Also see [Sign-extension Operators Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/sign-extension-ops/Overview.md).
///
///  - `MultiValue` supports functions and instructions with multiple return values, and blocks with inputs.
///     
///    Also see [Multi-value Extension](https://github.com/WebAssembly/spec/blob/main/proposals/multi-value/Overview.md).
///
///  - `BulkMemoryOperations` supports bulk memory operations.
///
///    Also see [Bulk Memory Operations Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/bulk-memory-operations/Overview.md#motivation-for-bulk-memory-operations).
///
///  - `ReferenceTypes` supports reference types.
///
///    Also see [Reference Types Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/reference-types/Overview.md).
///
///  - `SIMD` supports 128-bit packed SIMD extension to WebAssembly.
///
///    Also see [SIMD Proposal](https://github.com/WebAssembly/spec/blob/main/proposals/simd/SIMD.md).
#[derive(Debug)]
pub struct CommonConfigOptions {
    mutable_globals: bool,
    non_trap_conversions: bool,
    sign_extension_operators: bool,
    multi_value: bool,
    bulk_memory_operations: bool,
    reference_types: bool,
    simd: bool,
    multi_memories: bool,
    threads: bool,
    tail_call: bool,
    function_references: bool,
    interpreter_mode: bool,
}
impl CommonConfigOptions {
    /// Creates a new instance of [CommonConfigOptions].
    ///
    /// The default options are:
    /// * mutable_globals: true,
    /// * non_trap_conversions: true,
    /// * sign_extension_operators: true,
    /// * multi_value: true,
    /// * bulk_memory_operations: true,
    /// * reference_types: true,
    /// * simd: true,
    /// * multi_memories: false,
    /// * threads: false,
    /// * tail_call: false,
    /// * function_references: false,
    /// * interpreter_mode: false,
    pub fn new() -> Self {
        Self {
            mutable_globals: true,
            non_trap_conversions: true,
            sign_extension_operators: true,
            multi_value: true,
            bulk_memory_operations: true,
            reference_types: true,
            simd: true,
            multi_memories: false,
            threads: false,
            tail_call: false,
            function_references: false,
            interpreter_mode: false,
        }
    }

    /// Enables or disables the ImportExportMutGlobals option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn mutable_globals(self, enable: bool) -> Self {
        Self {
            mutable_globals: enable,
            ..self
        }
    }

    /// Enables or disables the NonTrapFloatToIntConversions option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn non_trap_conversions(self, enable: bool) -> Self {
        Self {
            non_trap_conversions: enable,
            ..self
        }
    }

    /// Enables or disables the SignExtensionOperators option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn sign_extension_operators(self, enable: bool) -> Self {
        Self {
            sign_extension_operators: enable,
            ..self
        }
    }

    /// Enables or disables the MultiValue option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn multi_value(self, enable: bool) -> Self {
        Self {
            multi_value: enable,
            ..self
        }
    }

    /// Enables or disables the BulkMemoryOperations option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn bulk_memory_operations(self, enable: bool) -> Self {
        Self {
            bulk_memory_operations: enable,
            ..self
        }
    }

    /// Enables or disables the ReferenceTypes option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn reference_types(self, enable: bool) -> Self {
        Self {
            reference_types: enable,
            ..self
        }
    }

    /// Enables or disables the SIMD option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn simd(self, enable: bool) -> Self {
        Self {
            simd: enable,
            ..self
        }
    }

    /// Enables or disables the MultiMemories option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn multi_memories(self, enable: bool) -> Self {
        Self {
            multi_memories: enable,
            ..self
        }
    }

    /// Enables or disables the Threads option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn threads(self, enable: bool) -> Self {
        Self {
            threads: enable,
            ..self
        }
    }

    /// Enables or disables the TailCall option.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn tail_call(self, enable: bool) -> Self {
        Self {
            tail_call: enable,
            ..self
        }
    }

    /// Enables or disables the FunctionReferences option.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn function_references(self, enable: bool) -> Self {
        Self {
            function_references: enable,
            ..self
        }
    }

    /// Enables or disables the `ForceInterpreter` option.
    ///
    /// # Argument
    ///
    /// * `enable` - Whether the option turns on or not.
    pub fn interpreter_mode(self, enable: bool) -> Self {
        Self {
            interpreter_mode: enable,
            ..self
        }
    }
}
impl Default for CommonConfigOptions {
    /// Creates a new default instance of [CommonConfigOptions].
    ///
    /// The default options are:
    /// * mutable_globals: true,
    /// * non_trap_conversions: true,
    /// * sign_extension_operators: true,
    /// * multi_value: true,
    /// * bulk_memory_operations: true,
    /// * reference_types: true,
    /// * simd: true,
    /// * multi_memories: false,
    /// * threads: false,
    /// * tail_call: false,
    /// * function_references: false,
    /// * interpreter_mode: false,
    fn default() -> Self {
        Self::new()
    }
}

/// Defines a group of configuration options for AOT compiler.
///
/// [CompilerConfigOptions] is used to set the AOT compiler related configuration options, which are
///
///  - Compiler Optimization Levels
///    - `O0` performs as many optimizations as possible.
///    
///    - `O1` optimizes quickly without destroying debuggability  
///    - `02` optimizes for fast execution as much as possible without triggering significant incremental
///           compile time or code size growth  
///    - `O3` optimizes for fast execution as much as possible  
///    - `Os` optimizes for small code size as much as possible without triggering significant incremental
///           compile time or execution time slowdowns  
///    - `Oz` optimizes for small code size as much as possible  
///  - Compiler Output Formats
///    - `Native` specifies the output format is native dynamic library (`*.wasm.so`)  
///    - `Wasm` specifies the output format is WebAssembly with AOT compiled codes in custom section (`*.wasm`).
///  
///  - `dump_ir` determines if AOT compiler generates IR or not  
///  - `generic_binary` determines if AOT compiler generates the generic binary or not.
///  - `interruptible` determines if AOT compiler generates interruptible binary or not.
///  
///  The configuration options above are only effective to [AOT compiler](crate::Compiler).
#[cfg(feature = "aot")]
#[derive(Debug)]
pub struct CompilerConfigOptions {
    out_format: CompilerOutputFormat,
    opt_level: CompilerOptimizationLevel,
    dump_ir: bool,
    generic_binary: bool,
    interruptible: bool,
}
#[cfg(feature = "aot")]
impl CompilerConfigOptions {
    /// Creates a new instance of [CompilerConfigOptions].
    pub fn new() -> Self {
        Self {
            out_format: CompilerOutputFormat::Wasm,
            opt_level: CompilerOptimizationLevel::O3,
            dump_ir: false,
            generic_binary: false,
            interruptible: false,
        }
    }

    /// Sets the output binary format of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `format` specifies the format of the output binary.
    pub fn out_format(self, format: CompilerOutputFormat) -> Self {
        Self {
            out_format: format,
            ..self
        }
    }

    /// Sets the optimization level of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `level` specifies the optimization level of AOT compiler.
    pub fn optimization_level(self, level: CompilerOptimizationLevel) -> Self {
        Self {
            opt_level: level,
            ..self
        }
    }

    /// Sets the dump IR option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if dump ir or not.
    pub fn dump_ir(self, enable: bool) -> Self {
        Self {
            dump_ir: enable,
            ..self
        }
    }

    /// Sets the generic binary option of AOT compiler.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if generate the generic binary or not when perform AOT compilation.
    pub fn generic_binary(self, enable: bool) -> Self {
        Self {
            generic_binary: enable,
            ..self
        }
    }

    /// Enables or Disables the `Interruptible` option of AOT compiler.
    ///
    /// This option determines to generate interruptible binary or not when compilation in AOT compiler.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if turn on the `Interruptible` option.
    pub fn interruptible(self, enable: bool) -> Self {
        Self {
            interruptible: enable,
            ..self
        }
    }
}
#[cfg(feature = "aot")]
impl Default for CompilerConfigOptions {
    fn default() -> Self {
        Self::new()
    }
}

/// Defines a group of runtime configuration options.
///
/// [RuntimeConfigOptions] is used to set the runtime configuration options, which are
///
/// - `maximum_memory_page` limits the page size of [Memory](crate::Memory). This option is only effective to
///       [Executor](crate::Executor).
#[derive(Debug)]
pub struct RuntimeConfigOptions {
    max_memory_pages: u32,
}
impl RuntimeConfigOptions {
    /// Creates a new instance of [RuntimeConfigOptions].
    pub fn new() -> Self {
        Self {
            max_memory_pages: 65536,
        }
    }

    /// Sets the maximum number of the memory pages available.
    ///
    /// # Argument
    ///
    /// - `count` specifies the page count (64KB per page).
    pub fn max_memory_pages(self, count: u32) -> Self {
        Self {
            max_memory_pages: count,
        }
    }
}
impl Default for RuntimeConfigOptions {
    fn default() -> Self {
        Self::new()
    }
}

/// Defines a group of the statistics configuration options.
///
/// [StatisticsConfigOptions] is used to set the statistics configuration options, which are
///
///  - `count_instructions` determines if measuring the count of instructions when running a compiled or pure WASM.
///   
///  - `measure_cost` determines if measuring the instruction costs when running a compiled or pure WASM.
///   
///  - `measure_time` determines if measuring the running time when running a compiled or pure WASM.
#[derive(Debug, Default)]
pub struct StatisticsConfigOptions {
    count_instructions: bool,
    measure_cost: bool,
    measure_time: bool,
}
impl StatisticsConfigOptions {
    /// Creates a new instance of [StatisticsConfigOptions].
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

/// Defines the host registration configuration options.
///
/// [HostRegistrationConfigOptions] is used to set the host registration configuration options, which are
///
///   - `Wasi` turns on the `WASI` support.
///
///   - `WasmEdgeProcess` turns on the `wasmedge_process` support.
#[derive(Debug, Default)]
pub struct HostRegistrationConfigOptions {
    wasi: bool,
    #[cfg(target_os = "linux")]
    wasmedge_process: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    wasi_nn: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    wasi_crypto_common: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    wasi_crypto_asymmetric_common: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    wasi_crypto_symmetric: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    wasi_crypto_kx: bool,
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    wasi_crypto_signatures: bool,
}
impl HostRegistrationConfigOptions {
    /// Enables or disables host registration wasi.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    pub fn wasi(self, enable: bool) -> Self {
        Self {
            wasi: enable,
            #[cfg(target_os = "linux")]
            wasmedge_process: self.wasmedge_process,
            #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
            wasi_nn: self.wasi_nn,
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            wasi_crypto_common: self.wasi_crypto_common,
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            wasi_crypto_asymmetric_common: self.wasi_crypto_asymmetric_common,
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            wasi_crypto_symmetric: self.wasi_crypto_symmetric,
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            wasi_crypto_kx: self.wasi_crypto_kx,
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            wasi_crypto_signatures: self.wasi_crypto_signatures,
        }
    }

    /// Enables or disables host registration WasmEdge process.
    ///
    /// # Argument
    ///
    /// - `enable` specifies if the option turns on or not.
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process(self, enable: bool) -> Self {
        Self {
            wasmedge_process: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn(self, enable: bool) -> Self {
        Self {
            wasi_nn: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common(self, enable: bool) -> Self {
        Self {
            wasi_crypto_common: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common(self, enable: bool) -> Self {
        Self {
            wasi_crypto_asymmetric_common: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric(self, enable: bool) -> Self {
        Self {
            wasi_crypto_symmetric: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx(self, enable: bool) -> Self {
        Self {
            wasi_crypto_kx: enable,
            ..self
        }
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures(self, enable: bool) -> Self {
        Self {
            wasi_crypto_signatures: enable,
            ..self
        }
    }
}

#[cfg(test)]
#[cfg(feature = "aot")]
mod tests {
    use super::*;

    #[test]
    fn test_config_create() {
        let common_options = CommonConfigOptions::default()
            .bulk_memory_operations(true)
            .multi_value(true)
            .mutable_globals(true)
            .non_trap_conversions(true)
            .reference_types(true)
            .sign_extension_operators(true)
            .simd(true)
            .multi_memories(true)
            .interpreter_mode(true);

        let compiler_options = CompilerConfigOptions::default()
            .dump_ir(true)
            .generic_binary(true)
            .interruptible(true)
            .optimization_level(CompilerOptimizationLevel::O0)
            .out_format(CompilerOutputFormat::Native);

        let stat_options = StatisticsConfigOptions::default()
            .count_instructions(true)
            .measure_cost(true)
            .measure_time(true);

        let runtime_options = RuntimeConfigOptions::default().max_memory_pages(1024);

        let host_options = HostRegistrationConfigOptions::default().wasi(true);

        let result = ConfigBuilder::new(common_options)
            .with_statistics_config(stat_options)
            .with_compiler_config(compiler_options)
            .with_runtime_config(runtime_options)
            .with_host_registration_config(host_options)
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
        assert!(config.multi_memories_enabled());
        assert!(config.interpreter_mode_enabled());

        // check compiler config options
        assert!(config.dump_ir_enabled());
        assert!(config.generic_binary_enabled());
        assert!(config.interruptible_enabled());
        assert_eq!(config.optimization_level(), CompilerOptimizationLevel::O0);
        assert_eq!(config.out_format(), CompilerOutputFormat::Native);

        // check statistics config options
        assert!(config.instruction_counting_enabled());
        assert!(config.cost_measuring_enabled());
        assert!(config.time_measuring_enabled());

        // check runtime config options
        assert_eq!(config.max_memory_pages(), 1024);

        assert!(config.wasi_enabled());
    }

    #[test]
    fn test_config_copy() {
        let common_config = CommonConfigOptions::default()
            .simd(false)
            .multi_memories(true);
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
        assert!(config.multi_memories_enabled());
        assert_eq!(config.optimization_level(), CompilerOptimizationLevel::O0);
        assert!(!config.time_measuring_enabled());
        assert_eq!(config.max_memory_pages(), 1024);
        assert!(config.wasi_enabled());

        // make a copy
        let config_copied = config.clone();
        assert!(!config_copied.simd_enabled());
        assert!(config_copied.multi_memories_enabled());
        assert_eq!(
            config_copied.optimization_level(),
            CompilerOptimizationLevel::O0
        );
        assert!(!config.time_measuring_enabled());
        assert_eq!(config_copied.max_memory_pages(), 1024);
        assert!(config_copied.wasi_enabled());
    }
}
