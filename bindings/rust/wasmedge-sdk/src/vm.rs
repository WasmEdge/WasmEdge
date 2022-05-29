//! Defines WasmEdge Vm struct.

use crate::{
    config::Config, ImportObject, Instance, Module, Statistics, WasiInstance,
    WasmEdgeProcessInstance, WasmEdgeResult,
};
use std::{marker::PhantomData, path::Path};
use wasmedge_sys as sys;
use wasmedge_types::FuncType;

/// A [Vm] defines a virtual environment for managing WebAssembly programs.
///
/// # Example
///
/// The example below presents how to register a module as named module in a Vm instance and run a target wasm function.
///
/// ```rust
/// #![feature(explicit_generic_args_with_impl_trait)]
///
/// use wasmedge_sdk::{params, Vm, WasmVal};
/// use wasmedge_types::{wat2wasm, ValType};
///
/// #[cfg_attr(test, test)]
/// fn main() -> Result<(), Box<dyn std::error::Error>> {
///     // create a Vm context
///     let vm = Vm::new(None)?;
///
///     // register a wasm module from the given in-memory wasm bytes
///     let wasm_bytes = wat2wasm(
///         br#"(module
///         (export "fib" (func $fib))
///         (func $fib (param $n i32) (result i32)
///          (if
///           (i32.lt_s
///            (get_local $n)
///            (i32.const 2)
///           )
///           (return
///            (i32.const 1)
///           )
///          )
///          (return
///           (i32.add
///            (call $fib
///             (i32.sub
///              (get_local $n)
///              (i32.const 2)
///             )
///            )
///            (call $fib
///             (i32.sub
///              (get_local $n)
///              (i32.const 1)
///             )
///            )
///           )
///          )
///         )
///        )
///     "#,
///     )?;
///     let mut vm = vm.register_module_from_bytes("extern", wasm_bytes)?;
///
///     // get func type of `fib`
///     let func_ty = vm.func_ty(Some("extern"), "fib")?;
///
///     // get the argument types
///     assert_eq!(func_ty.args_len(), 1);
///     let args = func_ty.args().expect("No argument types.");
///     assert_eq!(args, [ValType::I32]);
///
///     // get the return types
///     assert_eq!(func_ty.returns_len(), 1);
///     let returns = func_ty.returns().expect("No return types.");
///     assert_eq!(returns, [ValType::I32]);
///
///     // run `fib` function in the named module instance
///     let returns = vm.run_func(Some("extern"), "fib", params!(10))?;
///     assert_eq!(returns.len(), 1);
///     assert_eq!(returns[0].to_i32(), 89);
///
///     Ok(())
/// }
/// ```
#[derive(Debug)]
pub struct Vm {
    pub(crate) inner: sys::Vm,
    active_module: Option<Module>,
}
impl Vm {
    /// Creates a new [Vm] to be associated with the given [configuration](crate::config::Config).
    ///
    /// # Arguments
    ///
    /// * `config` - A optional [configuration](crate::config::Config) for the new [Vm].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn new(config: Option<Config>) -> WasmEdgeResult<Self> {
        // load wasmedge_process plugins
        sys::utils::load_plugin_from_default_paths();

        let inner_config = config.map(|c| c.inner);
        let inner = sys::Vm::create(inner_config, None)?;
        Ok(Self {
            inner,
            active_module: None,
        })
    }

    /// Registers a WASM module into the [vm](crate::Vm) from a wasm file, and instantiates it.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name for the WASM module to be registered.
    ///
    /// * `file` - The target WASM file.
    ///
    /// # Error
    ///
    /// If fail to register the target WASM, then an error is returned.
    pub fn register_module_from_file(
        mut self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_file(mod_name, file.as_ref())?;

        Ok(self)
    }

    /// Registers a WASM module from then given in-memory wasm bytes into the [Vm], and instantiates it.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the WASM module to be registered.
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_module_from_bytes(
        mut self,
        mod_name: impl AsRef<str>,
        bytes: impl AsRef<[u8]>,
    ) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_bytes(mod_name, bytes.as_ref())?;

        Ok(self)
    }

    /// Registers and instantiates a WasmEdge [import object](crate::ImportObject) into this [vm](crate::Vm).
    ///
    /// # Arguments
    ///
    /// * `import` - The WasmEdge [import object](crate::ImportObject) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [import object](crate::ImportObject), then an error is returned.
    pub fn register_import_module(mut self, import: ImportObject) -> WasmEdgeResult<Self> {
        self.inner.register_wasm_from_import(import.0)?;

        Ok(self)
    }

    /// Registers and instantiates a WasmEdge [compiled module](crate::Module) into this [vm](crate::Vm) as a named or active [module instance](crate::Instance).
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name of the registered [module](crate::Module). If `None`, then the [module](crate::Module) is registered as an active [module instance](crate::Instance).
    ///
    /// * `module` - The validated [module](crate::Module) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    pub fn register_module(self, mod_name: Option<&str>, module: Module) -> WasmEdgeResult<Self> {
        match mod_name {
            Some(name) => self.register_named_module(module, name),
            None => self.register_active_module(module),
        }
    }

    fn register_named_module(
        mut self,
        module: Module,
        mod_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Self> {
        self.inner
            .register_wasm_from_module(mod_name.as_ref(), module.inner)?;

        Ok(self)
    }

    fn register_active_module(mut self, module: Module) -> WasmEdgeResult<Self> {
        self.active_module = Some(module);
        self.inner
            .load_wasm_from_module(&self.active_module.as_ref().unwrap().inner)?;
        self.inner.validate()?;
        self.inner.instantiate()?;

        Ok(self)
    }

    /// Resets the [Vm].
    pub fn reset(&mut self) {
        self.inner.reset()
    }

    /// Runs an exported WASM function registered in a named or active module.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the module instance, which holds the target exported function. If `None`, then the active module is used.
    ///
    /// * `func_name` - The name of the exported WASM function to run.
    ///
    /// * `params` - The parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_func(
        &mut self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        let returns = match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                self.inner
                    .run_registered_function(mod_name, func_name.as_ref(), args)?
            }
            None => {
                // run a function in the active module
                self.inner.run_function(func_name.as_ref(), args)?
            }
        };

        Ok(returns)
    }

    /// Returns the type of a WASM function.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the module [instance](crate::Instance), which holds the target function. if `None`, then the active module is used.
    ///
    /// * `func_name` - The name of the target function.
    ///
    /// # Error
    ///
    /// If fail to get the function type, then an error is returned.
    pub fn func_ty(
        &mut self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<FuncType> {
        let func_ty = match mod_name {
            Some(mod_name) => self
                .inner
                .get_registered_function_type(mod_name, func_name.as_ref())?,
            None => self.inner.get_function_type(func_name.as_ref())?,
        };

        Ok(func_ty.into())
    }

    /// Invokes an exported function from the given wasm file.
    ///
    /// # Arguments
    ///
    /// * `file` - The WASM file.
    ///
    /// * `func_name` - The name of the target exported function to run.
    ///
    /// * `args` - The arguments passed to the target exported function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_file(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.inner
            .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)
    }

    /// Invokes an exported function from the given in-memory wasm bytes.
    ///
    /// # Arguments
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// * `func_name` - The name of the target exported function to run.
    ///
    /// * `args` - The arguments passed to the target exported function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_bytes(
        mut self,
        bytes: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.inner
            .run_wasm_from_bytes(bytes, func_name.as_ref(), args)
    }

    /// Invokes an exported function from the given [compiled module](crate::Module).
    ///
    /// # Arguments
    ///
    /// * `module` - A [compiled module](crate::Module).
    ///
    /// * `func_name` - The name of the target exported function to run.
    ///
    /// * `args` - The arguments passed to the target exported function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_module(
        mut self,
        module: Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.inner
            .run_wasm_from_module(module.inner, func_name.as_ref(), args)
    }

    /// Returns the count of the named [module instances](crate::Instance) in this [store](crate::Store).
    ///
    /// # Error
    ///
    /// If fail to get the count, then an error is returned.
    pub fn named_instance_count(&self) -> WasmEdgeResult<u32> {
        let count = self.inner.store_mut()?.module_len();
        Ok(count)
    }

    /// Returns the names of all registered named [module instances](crate::Instance).
    ///
    /// # Error
    /// If fail to get the instance names, then an error is returned.
    pub fn instance_names(&self) -> WasmEdgeResult<Vec<String>> {
        let names = self.inner.store_mut()?.module_names();
        match names {
            Some(names) => Ok(names),
            None => Ok(vec![]),
        }
    }

    /// Returns the named [module instance](crate::Instance) with the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target [module instance](crate::Instance) to be returned.
    ///
    /// # Error
    ///
    /// If fail to get the named [module instance](crate::Instance), then an error is returned.
    pub fn named_module(&self, name: impl AsRef<str>) -> WasmEdgeResult<Instance> {
        let mut inner_store = self.inner.store_mut()?;
        let inner_instance = inner_store.module(name.as_ref())?;

        Ok(Instance {
            inner: inner_instance,
        })
    }

    /// Returns the active [module instance](crate::Instance).
    ///
    /// # Error
    ///
    /// If fail to get the active [module instance](crate::Instance), then an error is returned.
    pub fn active_module(&self) -> WasmEdgeResult<Instance> {
        let inner_instance = self.inner.active_module()?;

        Ok(Instance {
            inner: inner_instance,
        })
    }

    /// Returns the internal [statistics instance](crate::Statistics) from the [Vm].
    pub fn statistics(&self) -> WasmEdgeResult<Statistics> {
        let inner_stat = self.inner.statistics_mut()?;

        Ok(Statistics {
            inner: inner_stat,
            _marker: PhantomData,
        })
    }

    /// Returns the [Wasi module instance](crate::WasiInstance).
    ///
    /// Notice that this function is only available when a [config](crate::config::Config) with the enabled [wasi](crate::config::HostRegistrationConfigOptions::wasi) option is used in the creation of this [Vm].
    ///
    /// # Error
    ///
    /// If fail to get the [Wasi module instance](crate::WasiInstance), then an error is returned.
    pub fn wasi_module(&mut self) -> WasmEdgeResult<WasiInstance> {
        let inner_wasi_module = self.inner.wasi_module_mut()?;

        Ok(WasiInstance {
            inner: inner_wasi_module,
        })
    }

    /// Returns the mutable [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance).
    ///
    /// Notice that this function is only available when a [config](crate::config::Config) with the enabled [wasmedge_process](crate::config::HostRegistrationConfigOptions::wasmedge_process) option is used in the creation of this [Vm].
    ///
    /// # Error
    ///
    /// If fail to get the [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance), then an error is returned.
    pub fn wasmedge_process_module(&mut self) -> WasmEdgeResult<WasmEdgeProcessInstance> {
        let inner_process_module = self.inner.wasmedge_process_module_mut()?;

        Ok(WasmEdgeProcessInstance {
            inner: inner_process_module,
        })
    }

    /// Checks if the [vm](crate::Vm) contains a named module instance.
    ///
    /// # Argument
    ///
    /// * `mod_name` - The registered module's name to check.
    ///
    pub fn contains_module(&self, mod_name: impl AsRef<str>) -> bool {
        self.inner.contains_module(mod_name.as_ref())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{
            CommonConfigOptions, ConfigBuilder, HostRegistrationConfigOptions,
            StatisticsConfigOptions,
        },
        io::WasmVal,
        params,
        types::Val,
        Global, ImportObjectBuilder, Memory, Table,
    };
    use wasmedge_sys::WasmValue;
    use wasmedge_types::{
        wat2wasm, GlobalType, MemoryType, Mutability, RefType, TableType, ValType,
    };

    #[test]
    fn test_vm_run_func_from_file() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // register a wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");

        // run `fib` function from the wasm file
        let result = vm.run_func_from_file(file, "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    fn test_vm_run_func_from_bytes() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from the given in-memory wasm bytes
        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();

        // run `fib` function from the wasm bytes
        let result = vm.run_func_from_bytes(&wasm_bytes, "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    fn test_vm_run_func_from_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // run `fib` function from the compiled module
        let result = vm.run_func_from_module(module, "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    fn test_vm_run_func_in_named_module_instance() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from the given in-memory wasm bytes
        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = vm.register_module_from_bytes("extern", wasm_bytes);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get func type of `fib`
        let result = vm.func_ty(Some("extern"), "fib");
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.args_len(), 1);
        let args = func_ty.args();
        assert!(args.is_some());
        let args = args.unwrap();
        assert_eq!(args, [ValType::I32]);
        assert_eq!(func_ty.returns_len(), 1);
        let returns = func_ty.returns();
        assert!(returns.is_some());
        let returns = returns.unwrap();
        assert_eq!(returns, [ValType::I32]);

        // run `fib` function in the named module instance
        let result = vm.run_func(Some("extern"), "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    fn test_vm_run_func_in_active_module_instance() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        let result = vm.register_module(None, module);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get func type of `fib`
        let result = vm.func_ty(None, "fib");
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.args_len(), 1);
        let args = func_ty.args();
        assert!(args.is_some());
        let args = args.unwrap();
        assert_eq!(args, [ValType::I32]);
        assert_eq!(func_ty.returns_len(), 1);
        let returns = func_ty.returns();
        assert!(returns.is_some());
        let returns = returns.unwrap();
        assert_eq!(returns, [ValType::I32]);

        // run `fib` function in the active module instance
        let result = vm.run_func(None, "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    fn test_vm_create() {
        {
            let result = Vm::new(None);
            assert!(result.is_ok());
        }

        {
            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let vm = result.unwrap();

            // get statistics
            let result = vm.statistics();
            assert!(result.is_ok());
        }
    }

    #[test]
    fn test_vm_wasi_module() {
        let host_reg_options = HostRegistrationConfigOptions::new().wasi(true);
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_host_registration_config(host_reg_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a vm with the config settings
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the wasi module
        let result = vm.wasi_module();
        assert!(result.is_ok());
        let wasi_instance = result.unwrap();

        assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
    }

    #[test]
    fn test_vm_wasmedge_process_module() {
        let host_reg_options = HostRegistrationConfigOptions::new().wasmedge_process(true);
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_host_registration_config(host_reg_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a vm with the config settings
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the wasmedge_process module
        let result = vm.wasmedge_process_module();
        assert!(result.is_ok());
        let wasmedge_process_instance = result.unwrap();

        assert_eq!(wasmedge_process_instance.name(), "wasmedge_process");
    }

    #[test]
    fn test_vm_statistics() {
        // set config options related to Statistics
        let stat_config_options = StatisticsConfigOptions::new()
            .measure_cost(true)
            .measure_time(true)
            .count_instructions(true);
        // create a Config
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_statistics_config(stat_config_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a Vm context
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let _vm = result.unwrap();

        // get the statistics
        // let _stat = vm.statistics_mut();
    }

    #[test]
    fn test_vm_register_module_from_file() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.register_module_from_file("extern", file);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert_eq!(vm.named_instance_count().unwrap(), 1);
        assert!(vm.instance_names().is_ok());
        assert_eq!(vm.instance_names().unwrap(), ["extern"]);
    }

    #[test]
    fn test_vm_register_module_from_bytes() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from the given in-memory wasm bytes
        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = vm.register_module_from_bytes("extern", wasm_bytes);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert_eq!(vm.named_instance_count().unwrap(), 1);
        assert!(vm.instance_names().is_ok());
        assert_eq!(vm.instance_names().unwrap(), ["extern"]);
    }

    #[test]
    fn test_vm_register_import_module() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = Memory::new(MemoryType::new(10, None));
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 5, None));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an ImportModule instance
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host function")
            .with_global("global", global_const)
            .expect("failed to add const global")
            .with_memory("mem", memory)
            .expect("failed to add memory")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register an import module into vm
        let result = vm.register_import_module(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert_eq!(vm.named_instance_count().unwrap(), 1);
        assert!(vm.instance_names().is_ok());
        assert_eq!(vm.instance_names().unwrap(), ["extern-module"]);

        // get active module instance
        let result = vm.named_module("extern-module");
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern-module");

        let result = instance.global("global");
        assert!(result.is_some());
        let global = result.unwrap();
        let result = global.ty();
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_register_named_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        assert_eq!(vm.named_instance_count().unwrap(), 0);
        let result = vm.register_module(Some("extern"), module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        assert_eq!(vm.named_instance_count().unwrap(), 1);
        let result = vm.named_module("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        assert_eq!(instance.func_count(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        let func_names = result.unwrap();
        assert_eq!(func_names, ["fib"]);

        // get host_func
        let result = instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        // check the type of host_func
        let result = fib.ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }

    #[test]
    fn test_vm_register_active_module() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module
        let result = wat2wasm(
            br#"(module
            (export "fib" (func $fib))
            (func $fib (param $n i32) (result i32)
             (if
              (i32.lt_s
               (get_local $n)
               (i32.const 2)
              )
              (return
               (i32.const 1)
              )
             )
             (return
              (i32.add
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 2)
                )
               )
               (call $fib
                (i32.sub
                 (get_local $n)
                 (i32.const 1)
                )
               )
              )
             )
            )
           )
        "#,
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = Module::from_bytes(None, wasm_bytes);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register the wasm module into vm
        let result = vm.register_module(None, module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        let result = vm.active_module();
        assert!(result.is_ok());
        let instance = result.unwrap();

        assert_eq!(instance.func_count(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        let func_names = result.unwrap();
        assert_eq!(func_names, ["fib"]);

        // get host_func
        let result = instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        // check the type of host_func
        let result = fib.ty();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [ValType::I32]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }

    fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
