//! Defines WasmEdge Vm struct.

use crate::{
    config::Config,
    error::{VmError, WasmEdgeError},
    wasi::WasiInstance,
    Executor, HostRegistration, ImportObject, Instance, Module, Statistics, Store, WasmEdgeResult,
    WasmValue,
};
use std::{collections::HashMap, path::Path};
use wasmedge_sys as sys;

/// Constructs a [Vm] instance.
#[derive(Debug, Default)]
pub struct VmBuilder {
    config: Option<Config>,
    stat: Option<Statistics>,
    store: Option<Store>,
    plugins: Vec<(String, String)>,
}
impl VmBuilder {
    /// Creates a new [VmBuilder].
    pub fn new() -> Self {
        Self::default()
    }

    /// Sets the [Config] for the [Vm] to build.
    ///
    /// # Argument
    ///
    /// * `config` - The [Config] to set.
    pub fn with_config(mut self, config: Config) -> Self {
        self.config = Some(config);
        self
    }

    /// Sets the [Statistics] for the [Vm] to build.
    ///
    /// # Argument
    ///
    /// * `stat` - The [Statistics] to set.
    pub fn with_statistics(mut self, stat: Statistics) -> Self {
        self.stat = Some(stat);
        self
    }

    /// Sets the [Store] for the [Vm] to build.
    ///
    /// # Argument
    ///
    /// * `store` - The [Store] to set.
    pub fn with_store(mut self, store: Store) -> Self {
        self.store = Some(store);
        self
    }

    /// Sets the `wasi_nn` plugin for the [Vm] to build. The `wasi_nn` plugin should be deployed with WasmEdge library.
    pub fn with_plugin_wasi_nn(mut self) -> Self {
        self.plugins.push(("wasi_nn".into(), "wasi_nn".into()));
        self
    }

    /// Sets the `wasi_crypto` plugin for the [Vm] to build. The `wasi_crypto` plugin should be deployed with WasmEdge library.
    pub fn with_plugin_wasi_crypto(mut self) -> Self {
        self.plugins
            .push(("wasi_crypto".into(), "wasi_crypto_common".into()));
        self.plugins
            .push(("wasi_crypto".into(), "wasi_crypto_asymmetric_common".into()));
        self.plugins
            .push(("wasi_crypto".into(), "wasi_crypto_kx".into()));
        self.plugins
            .push(("wasi_crypto".into(), "wasi_crypto_signatures".into()));
        self.plugins
            .push(("wasi_crypto".into(), "wasi_crypto_symmetric".into()));
        self
    }

    /// Sets the `wasmedge_process` plugin for the [Vm] to build. The `wasmedge_process` plugin should be deployed with WasmEdge library.
    pub fn with_plugin_wasmedge_process(mut self) -> Self {
        self.plugins
            .push(("wasmedge_process".into(), "wasmedge_process".into()));
        self
    }

    /// Sets the `wasmedge_sgx` plugin for the [Vm] to build. The `wasmedge_httpsreq` plugin should be deployed with WasmEdge library.
    pub fn with_plugin_wasmedge_httpsreq(mut self) -> Self {
        self.plugins
            .push(("wasmedge_httpsreq".into(), "wasmedge_httpsreq".into()));
        self
    }

    /// Set the third-party plugin for the [Vm] to build.
    ///
    /// # Arguments
    ///
    /// * `pname` - The name of the plugin.
    ///
    /// * `mname` - The name of the plugin module.
    pub fn with_plugin(mut self, pname: impl AsRef<str>, mname: impl AsRef<str>) -> Self {
        self.plugins
            .push((pname.as_ref().into(), mname.as_ref().into()));
        self
    }

    /// Creates a new [Vm].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn build(mut self) -> WasmEdgeResult<Vm> {
        // executor
        let executor = Executor::new(self.config.as_ref(), self.stat.as_mut())?;

        // store
        let store = match self.store {
            Some(store) => store,
            None => Store::new()?,
        };

        // create a Vm instance
        let mut vm = Vm {
            config: self.config,
            stat: self.stat,
            executor,
            store,
            named_instances: HashMap::new(),
            active_instance: None,
            imports: Vec::new(),
            builtin_host_instances: HashMap::new(),
            plugin_host_instances: Vec::new(),
        };

        // * built-in host instances
        if let Some(cfg) = vm.config.as_ref() {
            if cfg.wasi_enabled() {
                if let Ok(wasi_module) = sys::WasiModule::create(None, None, None) {
                    vm.executor.inner.register_import_object(
                        &mut vm.store.inner,
                        &sys::ImportObject::Wasi(wasi_module.clone()),
                    )?;

                    vm.builtin_host_instances.insert(
                        HostRegistration::Wasi,
                        HostRegistrationInstance::Wasi(WasiInstance { inner: wasi_module }),
                    );
                }
            }
        }

        // * load and register plugin instances
        for (pname, mname) in self.plugins.iter() {
            if let Some(instance) = Self::create_plugin_instance(pname, mname) {
                vm.plugin_host_instances.push(instance);
                vm.executor.inner.register_plugin_instance(
                    &mut vm.store.inner,
                    &vm.plugin_host_instances.last().unwrap().inner,
                )?;
            }
        }

        Ok(vm)
    }

    fn create_plugin_instance(pname: impl AsRef<str>, mname: impl AsRef<str>) -> Option<Instance> {
        match crate::plugin::PluginManager::find(pname.as_ref()) {
            Some(plugin) => plugin.mod_instance(mname.as_ref()),
            None => None,
        }
    }
}

/// A [Vm] defines a virtual environment for managing WebAssembly programs.
///
/// # Example
///
/// The example below presents how to register a module as named module in a Vm instance and run a target wasm function.
///
/// ```rust
/// // If the version of rust used is less than v1.63, please uncomment the follow attribute.
/// // #![feature(explicit_generic_args_with_impl_trait)]
///
/// use wasmedge_sdk::{params, VmBuilder, WasmVal};
/// use wasmedge_types::{wat2wasm, ValType};
///
/// #[cfg_attr(test, test)]
/// fn main() -> Result<(), Box<dyn std::error::Error>> {
///     // create a Vm context
///     let vm = VmBuilder::new().build()?;
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
    pub(crate) config: Option<Config>,
    stat: Option<Statistics>,
    executor: Executor,
    store: Store,
    named_instances: HashMap<String, Instance>,
    active_instance: Option<Instance>,
    imports: Vec<ImportObject>,
    builtin_host_instances: HashMap<HostRegistration, HostRegistrationInstance>,
    plugin_host_instances: Vec<Instance>,
}
impl Vm {
    /// Registers a [wasm module](crate::Module) into this vm as a named or active module [instance](crate::Instance).
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name for the registered module. If `None`, then the module is registered as an active instance.
    ///
    /// * `module` - The module to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    ///
    pub fn register_module(
        mut self,
        mod_name: Option<&str>,
        module: Module,
    ) -> WasmEdgeResult<Self> {
        match mod_name {
            Some(name) => {
                let named_instance =
                    self.store
                        .register_named_module(&mut self.executor, name, &module)?;
                self.named_instances.insert(name.into(), named_instance);
            }
            None => {
                self.active_instance = Some(
                    self.store
                        .register_active_module(&mut self.executor, &module)?,
                );
            }
        };

        Ok(self)
    }

    /// Registers a wasm module into the vm from a wasm file.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name for the registered module.
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// # Error
    ///
    /// If fail to register, then an error is returned.
    pub fn register_module_from_file(
        self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<Self> {
        // load module from file
        let module = Module::from_file(self.config.as_ref(), file.as_ref())?;

        // register the named module
        self.register_module(Some(mod_name.as_ref()), module)
    }

    /// Registers a wasm module from the given in-memory wasm bytes into this vm.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name for the registered module.
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// # Error
    ///
    /// If fail to register, then an error is returned.
    pub fn register_module_from_bytes(
        self,
        mod_name: impl AsRef<str>,
        bytes: impl AsRef<[u8]>,
    ) -> WasmEdgeResult<Self> {
        // load module from bytes
        let module = Module::from_bytes(self.config.as_ref(), bytes)?;

        // register the named module
        self.register_module(Some(mod_name.as_ref()), module)
    }

    /// Registers an [import object](crate::ImportObject) into this vm.
    ///
    /// # Arguments
    ///
    /// * `import` - The import object to be registered.
    ///
    /// # Error
    ///
    /// If fail to register, then an error is returned.
    pub fn register_import_module(mut self, import: ImportObject) -> WasmEdgeResult<Self> {
        match &import.0 {
            sys::ImportObject::Import(_) => {
                self.store
                    .register_import_module(&mut self.executor, &import)?;

                let import_instance = self.store.named_instance(import.name())?;
                self.named_instances
                    .insert(import.name().into(), import_instance);
            }
            _ => panic!("unsupported ImportObject type"),
        }

        self.imports.push(import);

        Ok(self)
    }

    /// Runs an exported wasm function in a (named or active) [module instance](crate::Instance).
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name of the module instance, which holds the target function. If `None`, then the active module is used.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run the wasm function, then an error is returned.
    pub fn run_func(
        &self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        match mod_name {
            Some(mod_name) => match self.named_instances.get(mod_name) {
                Some(named_instance) => named_instance
                    .func(func_name.as_ref())?
                    .run(self.executor(), args),
                None => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                    mod_name.into(),
                )))),
            },
            None => match &self.active_instance {
                Some(active_instance) => active_instance
                    .func(func_name.as_ref())?
                    .run(self.executor(), args),
                None => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule))),
            },
        }
    }

    /// Asynchronously runs an exported wasm function in a (named or active) [module instance](crate::Instance).
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The exported name of the module instance, which holds the target function. If `None`, then the active module is used.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run the wasm function, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_func_async(
        &self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str> + Send,
        args: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        match mod_name {
            Some(mod_name) => match self.named_instances.get(mod_name) {
                Some(named_instance) => {
                    named_instance
                        .func(func_name.as_ref())?
                        .run_async(self.executor(), args)
                        .await
                }
                None => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                    mod_name.into(),
                )))),
            },
            None => match self.active_instance.as_ref() {
                Some(active_instance) => {
                    active_instance
                        .func(func_name.as_ref())?
                        .run_async(self.executor(), args)
                        .await
                }
                None => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule))),
            },
        }
    }

    /// Runs an exported wasm function from the given [wasm module](crate::Module).
    ///
    /// This method is a shortcut of calling `register_module` and `run_func` in sequence.
    ///
    /// # Arguments
    ///
    /// * `module` - A [wasm module](crate::Module).
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_module(
        &mut self,
        module: Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.active_instance = Some(
            self.store
                .register_active_module(&mut self.executor, &module)?,
        );

        self.run_func(None, func_name, args)
    }

    /// Runs an exported wasm function from the given [wasm module](crate::Module).
    ///
    /// To use this method, turn on the `async` feature.
    ///
    /// # Arguments
    ///
    /// * `module` - A [wasm module](crate::Module).
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_func_from_module_async<N, A>(
        &mut self,
        module: Module,
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        self.active_instance = Some(
            self.store
                .register_active_module(&mut self.executor, &module)?,
        );

        self.run_func_async(None, func_name, args).await
    }

    /// Runs an exported wasm function from the given wasm file.
    ///
    /// # Arguments
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_file(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load module from file
        let module = Module::from_file(self.config.as_ref(), file.as_ref())?;

        self.run_func_from_module(module, func_name.as_ref(), args)
    }

    /// Asynchronously runs an exported wasm function from the given wasm file.
    ///
    /// # Arguments
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_func_from_file_async<P, N, A>(
        &mut self,
        file: P,
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        P: AsRef<Path>,
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        // load module from file
        let module = Module::from_file(self.config.as_ref(), file.as_ref())?;

        self.run_func_from_module_async(module, func_name.as_ref(), args)
            .await
    }

    /// Runs an exported wasm function from the given in-memory wasm bytes.
    ///
    /// # Arguments
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_bytes(
        &mut self,
        bytes: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        // load module from bytes
        let module = Module::from_bytes(self.config.as_ref(), bytes)?;

        self.run_func_from_module(module, func_name.as_ref(), args)
    }

    /// Runs an exported wasm function from the given in-memory wasm bytes.
    ///
    /// # Arguments
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// * `func_name` - The exported name of the target wasm function.
    ///
    /// * `args` - The arguments to be passed to the target wasm function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_func_from_bytes_async<N, A>(
        &mut self,
        bytes: &[u8],
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        // load module from bytes
        let module = Module::from_bytes(self.config.as_ref(), bytes)?;

        self.run_func_from_module_async(module, func_name.as_ref(), args)
            .await
    }

    /// Returns a reference to the internal [statistics](crate::Statistics) from this vm.
    pub fn statistics(&self) -> Option<&Statistics> {
        self.stat.as_ref()
    }

    /// Returns a mutable reference to the internal [statistics](crate::Statistics) from this vm.
    pub fn statistics_mut(&mut self) -> Option<&mut Statistics> {
        self.stat.as_mut()
    }

    /// Returns a reference to the internal [executor](crate::Executor) from this vm.
    pub fn executor(&self) -> &Executor {
        &self.executor
    }

    /// Returns a mutable reference to the internal [executor](crate::Executor) from this vm.
    pub fn executor_mut(&mut self) -> &mut Executor {
        &mut self.executor
    }

    /// Returns a reference to the internal [store](crate::Store) from this vm.
    pub fn store(&self) -> &Store {
        &self.store
    }

    /// Returns a mutable reference to the internal [store](crate::Store) from this vm.
    pub fn store_mut(&mut self) -> &mut Store {
        &mut self.store
    }

    /// Returns a reference to the [wasi module instance](crate::wasi::WasiInstance) from this vm.
    ///
    /// To retrieve  the [wasi module instance], a [config](crate::config::Config) with the enabled [wasi](crate::config::HostRegistrationConfigOptions::wasi) option should be given when create this vm.
    ///
    pub fn wasi_module(&self) -> Option<&WasiInstance> {
        match self.builtin_host_instances.get(&HostRegistration::Wasi) {
            Some(HostRegistrationInstance::Wasi(wasi_instance)) => Some(wasi_instance),
            _ => None,
        }
    }

    /// Returns a mutable reference to the [wasi module instance](crate::wasi::WasiInstance) from this vm.
    ///
    /// To retrieve the [wasi module instance], a [config](crate::config::Config) with the enabled [wasi](crate::config::HostRegistrationConfigOptions::wasi) option should be given when create this vm.
    ///
    pub fn wasi_module_mut(&mut self) -> Option<&mut WasiInstance> {
        match self.builtin_host_instances.get_mut(&HostRegistration::Wasi) {
            Some(HostRegistrationInstance::Wasi(wasi_instance)) => Some(wasi_instance),
            _ => None,
        }
    }

    /// Returns a reference to the named [module instance](crate::Instance) with the given name from this vm.
    ///
    /// # Argument
    ///
    /// * `name` - The exported name of the target module instance.
    ///
    /// # Error
    ///
    /// If fail to get the reference to the target module instance, then an error is returned.
    pub fn named_module(&self, name: impl AsRef<str>) -> WasmEdgeResult<&Instance> {
        self.named_instances.get(name.as_ref()).ok_or_else(|| {
            Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                name.as_ref().into(),
            )))
        })
    }

    /// Returns a mutable reference to the named [module instance](crate::Instance) with the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The exported name of the target module instance.
    ///
    /// # Error
    ///
    /// If fail to get the mutable reference to the target module instance, then an error is returned.
    pub fn named_module_mut(&mut self, name: impl AsRef<str>) -> WasmEdgeResult<&mut Instance> {
        self.named_instances.get_mut(name.as_ref()).ok_or_else(|| {
            Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                name.as_ref().into(),
            )))
        })
    }

    /// Returns a reference to the active [module instance](crate::Instance) from this vm.
    ///
    /// # Error
    ///
    /// If fail to get the reference to the active module instance, then an error is returned.
    pub fn active_module(&self) -> WasmEdgeResult<&Instance> {
        self.active_instance
            .as_ref()
            .ok_or_else(|| Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule)))
    }

    /// Returns a mutable reference to the active [module instance](crate::Instance) from this vm.
    ///
    /// # Error
    ///
    /// If fail to get the mutable reference to the active module instance, then an error is returned.
    pub fn active_module_mut(&mut self) -> WasmEdgeResult<&mut Instance> {
        self.active_instance
            .as_mut()
            .ok_or_else(|| Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule)))
    }

    /// Checks if the vm contains a named module instance.
    ///
    /// # Argument
    ///
    /// * `mod_name` - The exported name of the target module instance.
    ///
    pub fn contains_module(&self, mod_name: impl AsRef<str>) -> bool {
        self.store.contains(mod_name)
    }

    /// Returns the count of the named [module instances](crate::Instance) this vm holds.
    pub fn named_instance_count(&self) -> u32 {
        self.store.named_instance_count()
    }

    /// Returns the names of all named [module instances](crate::Instance) this vm holds.
    pub fn instance_names(&self) -> Vec<String> {
        self.store.instance_names()
    }
}

#[derive(Debug)]
enum HostRegistrationInstance {
    Wasi(crate::wasi::WasiInstance),
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{
            CommonConfigOptions, ConfigBuilder, HostRegistrationConfigOptions,
            StatisticsConfigOptions,
        },
        error::HostFuncError,
        io::WasmVal,
        params,
        types::Val,
        wat2wasm, AsInstance, CallingFrame, Global, GlobalType, ImportObjectBuilder, Memory,
        MemoryType, Mutability, RefType, Table, TableType, ValType, WasmValue,
    };

    #[test]
    fn test_vm_run_func_from_file() {
        // create a Vm context
        let result = VmBuilder::new().build();
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // register a wasm module from a specified wasm file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");

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
        let result = VmBuilder::new().build();
        assert!(result.is_ok());
        let mut vm = result.unwrap();

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
        let result = VmBuilder::new().build();
        assert!(result.is_ok());
        let mut vm = result.unwrap();

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
        let result = VmBuilder::new().build();
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
        let result = VmBuilder::new().build();
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

        // run `fib` function in the active module instance
        let result = vm.run_func(None, "fib", params!(10));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 89);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_create() {
        {
            let result = VmBuilder::new().build();
            assert!(result.is_ok());
        }

        {
            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = VmBuilder::new().with_config(config).build();
            assert!(result.is_ok());
            let _vm = result.unwrap();
        }

        #[cfg(all(target_os = "linux", not(feature = "static")))]
        {
            use crate::plugin::PluginManager;

            // load wasmedge_process plugin
            let result = PluginManager::load(None);
            assert!(result.is_ok());

            // create a Vm context
            let result = VmBuilder::new()
                .with_plugin_wasmedge_process()
                .with_plugin_wasi_crypto()
                .build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert!(vm.contains_module("wasmedge_process"));

            #[cfg(feature = "wasi_crypto")]
            {
                assert!(vm.contains_module("wasi_ephemeral_crypto_common"));
                assert!(vm.contains_module("wasi_ephemeral_crypto_asymmetric_common"));
                assert!(vm.contains_module("wasi_ephemeral_crypto_kx"));
                assert!(vm.contains_module("wasi_ephemeral_crypto_signatures"));
                assert!(vm.contains_module("wasi_ephemeral_crypto_symmetric"));
            }

            #[cfg(all(feature = "wasi_nn", target_arch = "x86_64"))]
            assert!(vm.contains_module("wasi_nn"));
        }
    }

    #[test]
    fn test_vm_wasi_module() {
        let host_reg_options = HostRegistrationConfigOptions::default().wasi(true);
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_host_registration_config(host_reg_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a vm with the config settings
        let result = VmBuilder::new().with_config(config).build();
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get the wasi module
        let result = vm.wasi_module();
        assert!(result.is_some());
        let wasi_instance = result.unwrap();

        assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
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
        let result = VmBuilder::new().with_config(config).build();
        assert!(result.is_ok());
        let _vm = result.unwrap();

        // get the statistics
        // let _stat = vm.statistics_mut();
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_module_from_file() {
        {
            // create a Vm context
            let result = VmBuilder::new().build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert!(vm.named_instance_count() >= 1);
            assert!(vm.instance_names().iter().any(|x| x == "extern"));
        }

        {
            // create a Vm context
            let result = VmBuilder::new().build();
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert!(vm.named_instance_count() >= 1);
            assert!(vm.instance_names().iter().any(|x| x == "extern"));
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_module_from_bytes() {
        // create a Vm context
        let result = VmBuilder::new().build();
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

        assert!(vm.named_instance_count() >= 1);
        assert!(vm.instance_names().iter().any(|x| x == "extern"));
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_import_module() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = MemoryType::new(10, None, false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
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
        let result = VmBuilder::new().build();
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register an import module into vm
        let result = vm.register_import_module(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert!(vm.named_instance_count() >= 1);
        assert!(vm.instance_names().iter().any(|x| x == "extern-module"));

        // get active module instance
        let result = vm.named_module("extern-module");
        assert!(result.is_ok());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern-module");

        let result = instance.global("global");
        assert!(result.is_ok());
        let global = result.unwrap();
        let ty = global.ty();
        assert_eq!(*ty, GlobalType::new(ValType::F32, Mutability::Const));
    }

    #[test]
    fn test_vm_register_named_module() {
        // create a Vm context
        let result = VmBuilder::new().build();
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
        let result = vm.register_module(Some("extern"), module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        assert!(vm.named_instance_count() >= 1);
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
        assert!(result.is_ok());
        let fib = result.unwrap();

        // check the type of host_func
        let ty = fib.ty();
        assert!(ty.args().is_some());
        assert_eq!(ty.args().unwrap(), [ValType::I32]);
        assert!(ty.returns().is_some());
        assert_eq!(ty.returns().unwrap(), [ValType::I32]);
    }

    #[test]
    fn test_vm_register_active_module() {
        // create a Vm context
        let result = VmBuilder::new().build();
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
        assert!(result.is_ok());
        let fib = result.unwrap();

        // check the type of host_func
        let ty = fib.ty();
        assert!(ty.args().is_some());
        assert_eq!(ty.args().unwrap(), [ValType::I32]);
        assert!(ty.returns().is_some());
        assert_eq!(ty.returns().unwrap(), [ValType::I32]);
    }

    fn real_add(
        _frame: CallingFrame,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
