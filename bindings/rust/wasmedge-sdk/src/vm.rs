//! Defines WasmEdge Vm struct.

#[cfg(all(
    target_os = "linux",
    feature = "wasi_nn",
    target_arch = "x86_64",
    not(feature = "static")
))]
use crate::wasi::WasiNnInstance;
#[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
use crate::wasi::{
    WasiCryptoAsymmetricCommonInstance, WasiCryptoCommonInstance, WasiCryptoKxInstance,
    WasiCryptoSignaturesInstance, WasiCryptoSymmetricInstance,
};
#[cfg(all(
    target_os = "linux",
    feature = "wasmedge_process",
    not(feature = "static")
))]
use crate::WasmEdgeProcessInstance;
use crate::{
    config::Config,
    error::{VmError, WasmEdgeError},
    wasi::WasiInstance,
    Executor, HostRegistration, ImportObject, Instance, Module, Statistics, Store, WasmEdgeResult,
    WasmValue,
};
use std::{collections::HashMap, path::Path};
use wasmedge_sys as sys;

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
/// use wasmedge_sdk::{params, Vm, WasmVal};
/// use wasmedge_types::{wat2wasm, ValType};
///
/// #[cfg_attr(test, test)]
/// fn main() -> Result<(), Box<dyn std::error::Error>> {
///     // create a Vm context
///     let vm = Vm::new(None, None)?;
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
    executor: Executor,
    store: Store,
    named_instances: HashMap<String, Instance>,
    active_instance: Option<Instance>,
    imports: Vec<ImportObject>,
    host_registrations: HashMap<HostRegistration, HostRegistrationInstance>,
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
    pub fn new(config: Option<Config>, stat: Option<&mut Statistics>) -> WasmEdgeResult<Self> {
        // create an executor
        let executor = Executor::new(config.as_ref(), stat)?;

        // create a store
        let store = Store::new()?;

        let mut vm = Self {
            config,
            executor,
            store,
            named_instances: HashMap::new(),
            active_instance: None,
            imports: Vec::new(),
            host_registrations: HashMap::new(),
        };

        if let Some(cfg) = vm.config.as_ref() {
            if cfg.wasi_enabled() {
                if let Ok(wasi_module) = sys::WasiModule::create(None, None, None) {
                    vm.executor.inner.register_import_object(
                        &mut vm.store.inner,
                        &sys::ImportObject::Wasi(wasi_module.clone()),
                    )?;

                    vm.host_registrations.insert(
                        HostRegistration::Wasi,
                        HostRegistrationInstance::Wasi(WasiInstance { inner: wasi_module }),
                    );
                }
            }

            #[cfg(all(
                target_os = "linux",
                feature = "wasmedge_process",
                not(feature = "static")
            ))]
            if cfg.wasmedge_process_enabled() {
                if let Ok(wasmedge_process_module) = sys::WasmEdgeProcessModule::create(None, false)
                {
                    vm.executor.inner.register_import_object(
                        &mut vm.store.inner,
                        &sys::ImportObject::WasmEdgeProcess(wasmedge_process_module.clone()),
                    )?;

                    vm.host_registrations.insert(
                        HostRegistration::WasmEdgeProcess,
                        HostRegistrationInstance::WasmEdgeProcess(WasmEdgeProcessInstance {
                            inner: wasmedge_process_module,
                        }),
                    );
                }
            }

            #[cfg(all(
                target_os = "linux",
                feature = "wasi_nn",
                target_arch = "x86_64",
                not(feature = "static")
            ))]
            if cfg.wasi_nn_enabled() {
                if let Ok(wasi_nn_module) = sys::WasiNnModule::create() {
                    vm.executor.inner.register_import_object(
                        &mut vm.store.inner,
                        &sys::ImportObject::Nn(wasi_nn_module.clone()),
                    )?;

                    vm.host_registrations.insert(
                        HostRegistration::WasiNn,
                        HostRegistrationInstance::WasiNn(WasiNnInstance {
                            inner: wasi_nn_module,
                        }),
                    );
                }
            }

            #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
            {
                if cfg.wasi_crypto_common_enabled() {
                    if let Ok(wasi_crypto_common_module) = sys::WasiCryptoCommonModule::create() {
                        vm.executor.inner.register_import_object(
                            &mut vm.store.inner,
                            &sys::ImportObject::Crypto(sys::WasiCrypto::Common(
                                wasi_crypto_common_module.clone(),
                            )),
                        )?;
                        vm.host_registrations.insert(
                            HostRegistration::WasiCryptoCommon,
                            HostRegistrationInstance::WasiCryptoCommon(WasiCryptoCommonInstance {
                                inner: wasi_crypto_common_module,
                            }),
                        );
                    }
                }

                if cfg.wasi_crypto_asymmetric_common_enabled() {
                    if let Ok(wasi_crypto_asymmetric_common_module) =
                        sys::WasiCryptoAsymmetricCommonModule::create()
                    {
                        vm.executor.inner.register_import_object(
                            &mut vm.store.inner,
                            &sys::ImportObject::Crypto(sys::WasiCrypto::AsymmetricCommon(
                                wasi_crypto_asymmetric_common_module.clone(),
                            )),
                        )?;
                        vm.host_registrations.insert(
                            HostRegistration::WasiCryptoAsymmetricCommon,
                            HostRegistrationInstance::WasiCryptoAsymmetricCommon(
                                WasiCryptoAsymmetricCommonInstance {
                                    inner: wasi_crypto_asymmetric_common_module,
                                },
                            ),
                        );
                    }
                }

                if cfg.wasi_crypto_kx_enabled() {
                    if let Ok(wasi_crypto_kx_module) = sys::WasiCryptoKxModule::create() {
                        vm.executor.inner.register_import_object(
                            &mut vm.store.inner,
                            &sys::ImportObject::Crypto(sys::WasiCrypto::KeyExchange(
                                wasi_crypto_kx_module.clone(),
                            )),
                        )?;
                        vm.host_registrations.insert(
                            HostRegistration::WasiCryptoKx,
                            HostRegistrationInstance::WasiCryptoKeyExchange(WasiCryptoKxInstance {
                                inner: wasi_crypto_kx_module,
                            }),
                        );
                    }
                }

                if cfg.wasi_crypto_signatures_enabled() {
                    if let Ok(wasi_crypto_signature_module) =
                        sys::WasiCryptoSignaturesModule::create()
                    {
                        vm.executor.inner.register_import_object(
                            &mut vm.store.inner,
                            &sys::ImportObject::Crypto(sys::WasiCrypto::Signatures(
                                wasi_crypto_signature_module.clone(),
                            )),
                        )?;
                        vm.host_registrations.insert(
                            HostRegistration::WasiCryptoSignatures,
                            HostRegistrationInstance::WasiCryptoSignatures(
                                WasiCryptoSignaturesInstance {
                                    inner: wasi_crypto_signature_module,
                                },
                            ),
                        );
                    }
                }

                if cfg.wasi_crypto_symmetric_enabled() {
                    if let Ok(wasi_crypto_symmetric_module) =
                        sys::WasiCryptoSymmetricModule::create()
                    {
                        vm.executor.inner.register_import_object(
                            &mut vm.store.inner,
                            &sys::ImportObject::Crypto(sys::WasiCrypto::SymmetricOptionations(
                                wasi_crypto_symmetric_module.clone(),
                            )),
                        )?;
                        vm.host_registrations.insert(
                            HostRegistration::WasiCryptoSymmetric,
                            HostRegistrationInstance::WasiCryptoSymmetric(
                                WasiCryptoSymmetricInstance {
                                    inner: wasi_crypto_symmetric_module,
                                },
                            ),
                        );
                    }
                }
            }
        }

        Ok(vm)
    }

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
        match self.host_registrations.get(&HostRegistration::Wasi) {
            Some(HostRegistrationInstance::Wasi(wasi_instance)) => Some(wasi_instance),
            _ => None,
        }
    }

    /// Returns a mutable reference to the [wasi module instance](crate::wasi::WasiInstance) from this vm.
    ///
    /// To retrieve the [wasi module instance], a [config](crate::config::Config) with the enabled [wasi](crate::config::HostRegistrationConfigOptions::wasi) option should be given when create this vm.
    ///
    pub fn wasi_module_mut(&mut self) -> Option<&mut WasiInstance> {
        match self.host_registrations.get_mut(&HostRegistration::Wasi) {
            Some(HostRegistrationInstance::Wasi(wasi_instance)) => Some(wasi_instance),
            _ => None,
        }
    }

    /// Returns a reference to the [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance) from this vm.
    ///
    /// To retrieve the [WasmEdgeProcess module instance], a [config](crate::config::Config) with the enabled [wasmedge_process](crate::config::HostRegistrationConfigOptions::wasmedge_process) option should be given when create this vm.
    ///
    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    pub fn wasmedge_process_module(&mut self) -> Option<&WasmEdgeProcessInstance> {
        match self
            .host_registrations
            .get(&HostRegistration::WasmEdgeProcess)
        {
            Some(HostRegistrationInstance::WasmEdgeProcess(wasmedge_process_instance)) => {
                Some(wasmedge_process_instance)
            }
            _ => None,
        }
    }

    /// Returns a mutable reference to the [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance) from this vm.
    ///
    /// To retrieve the [WasmEdgeProcess module instance], a [config](crate::config::Config) with the enabled [wasmedge_process](crate::config::HostRegistrationConfigOptions::wasmedge_process) option should be given when create this vm.
    ///
    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    pub fn wasmedge_process_module_mut(&mut self) -> Option<&mut WasmEdgeProcessInstance> {
        match self
            .host_registrations
            .get_mut(&HostRegistration::WasmEdgeProcess)
        {
            Some(HostRegistrationInstance::WasmEdgeProcess(wasmedge_process_instance)) => {
                Some(wasmedge_process_instance)
            }
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
    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    WasmEdgeProcess(crate::instance::WasmEdgeProcessInstance),
    #[cfg(all(
        target_os = "linux",
        feature = "wasi_nn",
        target_arch = "x86_64",
        not(feature = "static")
    ))]
    WasiNn(crate::wasi::WasiNnInstance),
    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    WasiCryptoCommon(crate::wasi::WasiCryptoCommonInstance),
    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    WasiCryptoAsymmetricCommon(crate::wasi::WasiCryptoAsymmetricCommonInstance),
    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    WasiCryptoSignatures(crate::wasi::WasiCryptoSignaturesInstance),
    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    WasiCryptoSymmetric(crate::wasi::WasiCryptoSymmetricInstance),
    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    WasiCryptoKeyExchange(crate::wasi::WasiCryptoKxInstance),
}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(all(target_os = "linux", not(feature = "static")))]
    use crate::PluginManager;
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
        let result = Vm::new(None, None);
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
        let result = Vm::new(None, None);
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
        let result = Vm::new(None, None);
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
        let result = Vm::new(None, None);
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
        let result = Vm::new(None, None);
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
            let result = Vm::new(None, None);
            assert!(result.is_ok());
        }

        {
            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config), None);
            assert!(result.is_ok());
            let _vm = result.unwrap();
        }

        #[cfg(all(target_os = "linux", not(feature = "static")))]
        {
            // load wasmedge_process plugin
            PluginManager::load_from_default_paths();

            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default())
                .with_host_registration_config(
                    HostRegistrationConfigOptions::default().wasmedge_process(true),
                )
                .build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config), None);
            assert!(result.is_ok());
            let _vm = result.unwrap();
        }

        #[cfg(all(target_os = "linux", not(feature = "static"), feature = "wasi_crypto"))]
        {
            // load wasi_crypto plugin
            PluginManager::load_from_default_paths();

            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default())
                .with_host_registration_config(
                    HostRegistrationConfigOptions::default()
                        .wasi_crypto_common(true)
                        .wasi_crypto_asymmetric_common(true)
                        .wasi_crypto_kx(true)
                        .wasi_crypto_signatures(true)
                        .wasi_crypto_symmetric(true),
                )
                .build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config), None);
            assert!(result.is_ok());
            let _vm = result.unwrap();
        }

        #[cfg(all(
            target_os = "linux",
            not(feature = "static"),
            feature = "wasi_nn",
            target_arch = "x86_64"
        ))]
        {
            // load wasi_nn plugin
            PluginManager::load_from_default_paths();

            // create a Config
            let result = ConfigBuilder::new(CommonConfigOptions::default())
                .with_host_registration_config(
                    HostRegistrationConfigOptions::default().wasi_nn(true),
                )
                .build();
            assert!(result.is_ok());
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config), None);
            assert!(result.is_ok());
            let _vm = result.unwrap();
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
        let result = Vm::new(Some(config), None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get the wasi module
        let result = vm.wasi_module();
        assert!(result.is_some());
        let wasi_instance = result.unwrap();

        assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
    }

    #[test]
    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    fn test_vm_wasmedge_process_module() {
        // load wasmedge_process plugin
        PluginManager::load_from_default_paths();

        let host_reg_options = HostRegistrationConfigOptions::default().wasmedge_process(true);
        let result = ConfigBuilder::new(CommonConfigOptions::default())
            .with_host_registration_config(host_reg_options)
            .build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a vm with the config settings
        let result = Vm::new(Some(config), None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the wasmedge_process module
        let result = vm.wasmedge_process_module();
        assert!(result.is_some());
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
        let result = Vm::new(Some(config), None);
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
            let result = Vm::new(None, None);
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert_eq!(vm.named_instance_count(), 1);
            assert_eq!(vm.instance_names(), ["extern"]);
        }

        {
            // create a Vm context
            let result = Vm::new(None, None);
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert_eq!(vm.named_instance_count(), 1);
            assert_eq!(vm.instance_names(), ["extern"]);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_module_from_bytes() {
        // create a Vm context
        let result = Vm::new(None, None);
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

        assert_eq!(vm.named_instance_count(), 1);
        assert_eq!(vm.instance_names(), ["extern"]);
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
        let result = Vm::new(None, None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register an import module into vm
        let result = vm.register_import_module(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        assert_eq!(vm.named_instance_count(), 1);
        assert_eq!(vm.instance_names(), ["extern-module"]);

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
        let result = Vm::new(None, None);
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
        assert_eq!(vm.named_instance_count(), 0);
        let result = vm.register_module(Some("extern"), module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // check the exported functions in the "extern" module
        assert_eq!(vm.named_instance_count(), 1);
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
        let result = Vm::new(None, None);
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
