//! Defines WasmEdge Vm struct.

#[cfg(feature = "custom_wasi")]
use crate::instance::custom_wasi_module::CustomWasiModule;
#[cfg(not(feature = "custom_wasi"))]
use crate::instance::module::WasiModule;
#[cfg(feature = "async")]
use crate::r#async::FiberFuture;
#[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
use crate::WasiNnModule;
#[cfg(target_os = "linux")]
use crate::WasmEdgeProcessModule;
use crate::{
    error::{VmError, WasmEdgeError},
    executor::Executor,
    ffi,
    instance::function::{FuncRef, Function},
    loader::Loader,
    statistics::Statistics,
    store::Store,
    utils::check,
    validator::Validator,
    Config, Engine, ImportObject, Instance, Module, WasmEdgeResult, WasmValue,
};
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
use crate::{
    WasiCrypto, WasiCryptoAsymmetricCommonModule, WasiCryptoCommonModule, WasiCryptoKxModule,
    WasiCryptoSignaturesModule, WasiCryptoSymmetricModule,
};
use std::{collections::HashMap, path::Path};
use wasmedge_types::HostRegistration;

pub struct Vm {
    imports: HashMap<String, ImportObject>,
    host_registered_modules: HashMap<HostRegistration, ImportObject>,
    executor: Executor,
    store: Store,
    config: Config,
    stat: Statistics,
    loader: Loader,
    validator: Validator,
    named_instances: HashMap<String, Instance>,
    active_instance: Option<Instance>,
}
impl Vm {
    /// Creates a new [Vm] to be associated with the given [configuration](crate::Config) and [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// * `config` - A configuration for the new [Vm].
    ///
    /// * `store` - An external WASM [store](crate::Store) used by the new [Vm]. The instantiation and execution of the new [Vm] will consume this store context. If no store context is specified when creating a [Vm], then the [Vm] itself will allocate and own a [store](crate::Store).
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create(config: Option<Config>) -> WasmEdgeResult<Self> {
        let config = match config {
            Some(config) => config,
            None => Config::create()?,
        };

        let store = Store::create()?;
        let mut stat = Statistics::create()?;
        let executor = Executor::create(Some(config.clone()), Some(&mut stat))?;
        let loader = Loader::create(Some(config.clone()))?;
        let validator = Validator::create(Some(config.clone()))?;

        let mut vm = Self {
            imports: HashMap::new(),
            host_registered_modules: HashMap::new(),
            executor,
            store,
            config,
            stat,
            loader,
            validator,
            named_instances: HashMap::new(),
            active_instance: None,
        };

        if vm.config.wasi_enabled() {
            #[cfg(not(feature = "custom_wasi"))]
            vm.register_instance_from_import(ImportObject::Wasi(WasiModule::create(
                None, None, None,
            )?))?;
            #[cfg(feature = "custom_wasi")]
            vm.register_instance_from_import(ImportObject::CustomWasi(CustomWasiModule::create(
                None, None, None,
            )?))?;
        }

        #[cfg(target_os = "linux")]
        if vm.config.wasmedge_process_enabled() {
            vm.register_instance_from_import(ImportObject::WasmEdgeProcess(
                WasmEdgeProcessModule::create(None, false)?,
            ))?;
        }

        Ok(vm)
    }

    #[cfg(feature = "custom_wasi")]
    pub fn custom_wasi_module(&self) -> WasmEdgeResult<&CustomWasiModule> {
        if self.config.wasi_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::Wasi)
            {
                true => {
                    let wasi_module = self
                        .host_registered_modules
                        .get(&HostRegistration::Wasi)
                        .unwrap();
                    match wasi_module {
                        ImportObject::CustomWasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        } else {
            match self.imports.contains_key("wasi_snapshot_preview1") {
                true => {
                    let wasi_module = self.imports.get("wasi_snapshot_preview1").unwrap();
                    match wasi_module {
                        ImportObject::CustomWasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        }
    }

    #[cfg(feature = "custom_wasi")]
    pub fn custom_wasi_module_mut(&mut self) -> WasmEdgeResult<&mut CustomWasiModule> {
        if self.config.wasi_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::Wasi)
            {
                true => {
                    let wasi_module = self
                        .host_registered_modules
                        .get_mut(&HostRegistration::Wasi)
                        .unwrap();
                    match wasi_module {
                        ImportObject::CustomWasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        } else {
            match self.imports.contains_key("wasi_snapshot_preview1") {
                true => {
                    let wasi_module = self.imports.get_mut("wasi_snapshot_preview1").unwrap();
                    match wasi_module {
                        ImportObject::CustomWasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        }
    }

    #[cfg(not(feature = "custom_wasi"))]
    pub fn wasi_module(&self) -> WasmEdgeResult<&WasiModule> {
        if self.config.wasi_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::Wasi)
            {
                true => {
                    let wasi_module = self
                        .host_registered_modules
                        .get(&HostRegistration::Wasi)
                        .unwrap();
                    match wasi_module {
                        ImportObject::Wasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        } else {
            match self.imports.contains_key("wasi_snapshot_preview1") {
                true => {
                    let wasi_module = self.imports.get("wasi_snapshot_preview1").unwrap();
                    match wasi_module {
                        ImportObject::Wasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        }
    }

    #[cfg(not(feature = "custom_wasi"))]
    pub fn wasi_module_mut(&mut self) -> WasmEdgeResult<&mut WasiModule> {
        if self.config.wasi_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::Wasi)
            {
                true => {
                    let wasi_module = self
                        .host_registered_modules
                        .get_mut(&HostRegistration::Wasi)
                        .unwrap();
                    match wasi_module {
                        ImportObject::Wasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        } else {
            match self.imports.contains_key("wasi_snapshot_preview1") {
                true => {
                    let wasi_module = self.imports.get_mut("wasi_snapshot_preview1").unwrap();
                    match wasi_module {
                        ImportObject::Wasi(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            }
        }
    }

    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_module(&self) -> WasmEdgeResult<&WasmEdgeProcessModule> {
        if self.config.wasmedge_process_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::WasmEdgeProcess)
            {
                true => {
                    let wasmedge_process_module = self
                        .host_registered_modules
                        .get(&HostRegistration::WasmEdgeProcess)
                        .unwrap();
                    match wasmedge_process_module {
                        ImportObject::WasmEdgeProcess(wasmedge_process_module) => {
                            Ok(wasmedge_process_module)
                        }
                        _ => Err(Box::new(WasmEdgeError::Vm(
                            VmError::NotFoundWasmEdgeProcessModule,
                        ))),
                    }
                }
                false => {
                    return Err(Box::new(WasmEdgeError::Vm(
                        VmError::NotFoundWasmEdgeProcessModule,
                    )))
                }
            }
        } else {
            match self.imports.contains_key("wasmedge_process") {
                true => {
                    let wasi_module = self.imports.get("wasmedge_process").unwrap();
                    match wasi_module {
                        ImportObject::WasmEdgeProcess(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(
                            VmError::NotFoundWasmEdgeProcessModule,
                        ))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(
                    VmError::NotFoundWasmEdgeProcessModule,
                ))),
            }
        }
    }

    /// Returns the mutable [WasmEdgeProcess module instance](crate::WasmEdgeProcessModule).
    ///
    /// Notice that this function is only available when a [config](crate::Config) with the enabled [wasmedge_process](crate::Config::wasmedge_process) option is used in the creation of this [Vm].
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_module_mut(&mut self) -> WasmEdgeResult<&mut WasmEdgeProcessModule> {
        if self.config.wasmedge_process_enabled() {
            match self
                .host_registered_modules
                .contains_key(&HostRegistration::WasmEdgeProcess)
            {
                true => {
                    let wasmedge_process_module = self
                        .host_registered_modules
                        .get_mut(&HostRegistration::WasmEdgeProcess)
                        .unwrap();
                    match wasmedge_process_module {
                        ImportObject::WasmEdgeProcess(wasmedge_process_module) => {
                            Ok(wasmedge_process_module)
                        }
                        _ => Err(Box::new(WasmEdgeError::Vm(
                            VmError::NotFoundWasmEdgeProcessModule,
                        ))),
                    }
                }
                false => {
                    return Err(Box::new(WasmEdgeError::Vm(
                        VmError::NotFoundWasmEdgeProcessModule,
                    )))
                }
            }
        } else {
            match self.imports.contains_key("wasmedge_process") {
                true => {
                    let wasi_module = self.imports.get_mut("wasmedge_process").unwrap();
                    match wasi_module {
                        ImportObject::WasmEdgeProcess(wasi_module) => Ok(wasi_module),
                        _ => Err(Box::new(WasmEdgeError::Vm(
                            VmError::NotFoundWasmEdgeProcessModule,
                        ))),
                    }
                }
                false => Err(Box::new(WasmEdgeError::Vm(
                    VmError::NotFoundWasmEdgeProcessModule,
                ))),
            }
        }
    }

    pub fn store(&self) -> &Store {
        &self.store
    }

    /// Returns the internal [Store](crate::Store) from the [Vm].
    pub fn store_mut(&mut self) -> &mut Store {
        &mut self.store
    }

    pub fn statistics(&self) -> &Statistics {
        &self.stat
    }

    /// Returns the internal [Statistics](crate::Statistics) from the [Vm].
    pub fn statistics_mut(&mut self) -> &mut Statistics {
        &mut self.stat
    }

    /// Returns the internal [Executor](crate::Executor) from the [Vm].
    pub fn executor(&self) -> &Executor {
        &self.executor
    }

    pub fn executor_mut(&mut self) -> &mut Executor {
        &mut self.executor
    }

    pub fn register_instance_from_import(&mut self, import: ImportObject) -> WasmEdgeResult<()> {
        let io_name: String = import.name().into();

        match import {
            ImportObject::Import(import_mod) => {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports
                        .insert(io_name.clone(), ImportObject::Import(import_mod));
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            }
            #[cfg(not(feature = "custom_wasi"))]
            ImportObject::Wasi(import_mod) => {
                if self.config.wasi_enabled() {
                    if self
                        .host_registered_modules
                        .contains_key(&HostRegistration::Wasi)
                    {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.host_registered_modules
                            .insert(HostRegistration::Wasi, ImportObject::Wasi(import_mod));
                    }

                    self.executor.register_import_object(
                        &mut self.store,
                        self.host_registered_modules
                            .get(&HostRegistration::Wasi)
                            .unwrap(),
                    )
                } else {
                    if self.imports.contains_key(&io_name) {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.imports
                            .insert(io_name.clone(), ImportObject::Wasi(import_mod));
                    }
                    self.executor.register_import_object(
                        &mut self.store,
                        self.imports.get(&io_name).unwrap(),
                    )
                }
            }
            #[cfg(feature = "custom_wasi")]
            ImportObject::CustomWasi(import_mod) => {
                if self.config.wasi_enabled() {
                    if self
                        .host_registered_modules
                        .contains_key(&HostRegistration::Wasi)
                    {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.host_registered_modules
                            .insert(HostRegistration::Wasi, ImportObject::CustomWasi(import_mod));
                    }

                    self.executor.register_import_object(
                        &mut self.store,
                        self.host_registered_modules
                            .get(&HostRegistration::Wasi)
                            .unwrap(),
                    )
                } else {
                    if self.imports.contains_key(&io_name) {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.imports
                            .insert(io_name.clone(), ImportObject::CustomWasi(import_mod));
                    }
                    self.executor.register_import_object(
                        &mut self.store,
                        self.imports.get(&io_name).unwrap(),
                    )
                }
            }
            #[cfg(target_os = "linux")]
            ImportObject::WasmEdgeProcess(import_mod) => {
                if self.config.wasmedge_process_enabled() {
                    if self
                        .host_registered_modules
                        .contains_key(&HostRegistration::WasmEdgeProcess)
                    {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.host_registered_modules.insert(
                            HostRegistration::WasmEdgeProcess,
                            ImportObject::WasmEdgeProcess(import_mod),
                        );
                    }
                    self.executor.register_import_object(
                        &mut self.store,
                        self.host_registered_modules
                            .get(&HostRegistration::WasmEdgeProcess)
                            .unwrap(),
                    )
                } else {
                    if self.imports.contains_key(&io_name) {
                        return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                    } else {
                        self.imports
                            .insert(io_name.clone(), ImportObject::WasmEdgeProcess(import_mod));
                    }
                    self.executor.register_import_object(
                        &mut self.store,
                        self.imports.get(&io_name).unwrap(),
                    )
                }
            }
            #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
            ImportObject::Nn(import_mod) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports
                        .insert(io_name.clone(), ImportObject::Import(import_mod));
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::Common(import_mod)) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports.insert(
                        io_name.clone(),
                        ImportObject::Crypto(WasiCrypto::Common(import_mod)),
                    );
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::AsymmetricCommon(import_mod)) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports.insert(
                        io_name.clone(),
                        ImportObject::Crypto(WasiCrypto::AsymmetricCommon(import_mod)),
                    );
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::SymmetricOptionations(import_mod)) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports.insert(
                        io_name.clone(),
                        ImportObject::Crypto(WasiCrypto::SymmetricOptionations(import_mod)),
                    );
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::KeyExchange(import_mod)) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports.insert(
                        io_name.clone(),
                        ImportObject::Crypto(WasiCrypto::KeyExchange(import_mod)),
                    );
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::Signatures(import_mod)) => unsafe {
                if self.imports.contains_key(&io_name) {
                    return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
                } else {
                    self.imports.insert(
                        io_name.clone(),
                        ImportObject::Crypto(WasiCrypto::Signatures(import_mod)),
                    );
                }
                self.executor
                    .register_import_object(&mut self.store, self.imports.get(&io_name).unwrap())
            },
        }
    }

    /// Registers a given WasmEdge AST [Module](crate::Module) into the [Vm], and instantiates it. The module is consumed.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads a WASM module from the given WasmEdge AST [Module](crate::Module), then
    ///
    /// - Registers all exported instances in the WASM module into the [Vm];
    ///
    /// - Finally, instantiates the exported instances.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the WASM module to be registered.
    ///
    /// * `module` - The WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or [Compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_instance_from_module(
        &mut self,
        mod_name: impl AsRef<str>,
        module: &Module,
    ) -> WasmEdgeResult<()> {
        // validate module
        self.validator.validate(&module)?;

        // register module
        let instance =
            self.executor
                .register_named_module(&mut self.store, &module, mod_name.as_ref())?;
        match self.named_instances.contains_key(mod_name.as_ref()) {
            true => {
                return Err(Box::new(WasmEdgeError::Vm(
                    VmError::DuplicateModuleInstance(mod_name.as_ref().to_string()),
                )));
            }
            false => {
                self.named_instances
                    .insert(mod_name.as_ref().to_string(), instance);
            }
        }

        Ok(())
    }

    /// Registers a WASM module from a given in-memory wasm bytes into the [Vm], and instantiates it.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads a WASM module from the given in-memory wasm bytes, then
    ///
    /// - Registers all exported instances in the WASM module into the [Vm];
    ///
    /// - Finally, instantiates the exported instances.
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
    pub fn register_instance_from_bytes(
        &mut self,
        mod_name: impl AsRef<str>,
        bytes: impl AsRef<[u8]>,
    ) -> WasmEdgeResult<()> {
        // load module from bytes
        let ast_module = self.loader.from_bytes(bytes)?;

        // register module
        self.register_instance_from_module(mod_name, &ast_module)
    }

    /// Registers and instantiates a WASM module from a wasm file into the [Vm].
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// * First, loads a WASM module from a given path, then
    ///
    /// * Registers all exported instances in the WASM module into the [Vm];
    ///
    /// * Finally, instantiates the exported instances.
    ///
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name for the WASM module to be registered.
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// # Error
    ///
    /// If fail to register the target WASM, then an error is returned.
    pub fn register_instance_from_file(
        &mut self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        match file.as_ref().extension() {
            Some(extension) => match extension.to_str() {
                Some("wasm") => self.register_instance_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "macos")]
                Some("dylib") => self.register_instance_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "linux")]
                Some("so") => self.register_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "windows")]
                Some("dll") => self.register_from_wasm_or_aot_file(mod_name, file),
                Some("wat") => {
                    let bytes = wat::parse_file(file.as_ref())
                        .map_err(|_| WasmEdgeError::Operation("Failed to parse wat file".into()))?;
                    self.register_instance_from_bytes(mod_name, &bytes)
                }
                _ => Err(Box::new(WasmEdgeError::Operation(
                    "The source file's extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.".into(),
                ))),
            },
            None => self.register_instance_from_wasm_or_aot_file(mod_name, file),
        }
    }

    fn register_instance_from_wasm_or_aot_file(
        &mut self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        // load module from file
        let ast_module = self.loader.from_file(file.as_ref())?;

        // register module
        self.register_instance_from_module(mod_name, &ast_module)
    }

    pub fn register_active_instance_from_file(
        &mut self,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        // load module from file
        let ast_module = self.loader.from_file(file.as_ref())?;

        // register active instance
        self.register_active_instance_from_module(&ast_module)
    }

    pub fn register_active_instance_from_bytes(
        &mut self,
        bytes: impl AsRef<[u8]>,
    ) -> WasmEdgeResult<()> {
        // load module from file
        let ast_module = self.loader.from_bytes(bytes.as_ref())?;

        // register active instance
        self.register_active_instance_from_module(&ast_module)
    }

    pub fn register_active_instance_from_module(&mut self, module: &Module) -> WasmEdgeResult<()> {
        // validate module
        self.validator.validate(module)?;

        // register module
        let instance = self
            .executor
            .register_active_module(&mut self.store, module)?;

        self.active_instance = Some(instance);

        Ok(())
    }

    /// Runs a [function](crate::Function) from a WasmEdge compiled [Module](crate::Module).
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// * First, loads and instantiates the WASM module from a WasmEdge AST [Module](crate::Module), then
    ///
    /// * Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// * `module` - The WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or [Compiler](crate::Compiler).
    ///
    /// * `func_name` - The name of the [function](crate::Function).
    ///
    /// * `params` - The parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_module(
        &mut self,
        module: &Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // validate module
        self.validator.validate(&module)?;

        // register active instance
        self.register_active_instance_from_module(&module)?;

        // run function
        self.run_function(func_name.as_ref(), params)
    }

    #[cfg(feature = "async")]
    pub async fn run_wasm_from_module_async(
        &mut self,
        module: &Module,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // validate module
        self.validator.validate(&module)?;

        // register active instance
        self.register_active_instance_from_module(&module)?;

        // run function
        self.run_function_async(func_name, params).await
    }

    /// Runs a [function](crate::Function) defined in a WASM file.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// * First, loads the WASM module from a given WASM file, and validates the loaded module; then,
    ///
    /// * Instantiates the WASM module; finally,
    ///
    /// * Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// * `file` - A wasm file or an AOT wasm file.
    ///
    /// * `func_name` - The name of the [function](crate::Function).
    ///
    /// * `params` - The the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_file(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load module from file
        let ast_module = self.loader.from_file(file.as_ref())?;

        self.run_wasm_from_module(&ast_module, func_name.as_ref(), params)
    }

    #[cfg(feature = "async")]
    pub async fn run_wasm_from_file_async(
        &mut self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load module from file
        let ast_module = self.loader.from_file(file.as_ref())?;

        self.run_wasm_from_module_async(&ast_module, func_name.as_ref(), params)
            .await
    }

    /// Runs a [function](crate::Function) from a in-memory wasm bytes.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// * First, loads and instantiates the WASM module from a buffer, then
    ///
    /// * Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// * `func_name` - The name of the [function](crate::Function).
    ///
    /// * `params` - The parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_bytes(
        &mut self,
        bytes: impl AsRef<[u8]>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load module from bytes
        let ast_module = self.loader.from_bytes(bytes.as_ref())?;

        self.run_wasm_from_module(&ast_module, func_name.as_ref(), params)
    }

    #[cfg(feature = "async")]
    pub async fn run_wasm_from_bytes_async(
        &mut self,
        bytes: impl AsRef<[u8]>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load module from bytes
        let ast_module = self.loader.from_bytes(bytes.as_ref())?;

        self.run_wasm_from_module_async(&ast_module, func_name.as_ref(), params)
            .await
    }

    /// Runs an exported WASM function by name. The WASM function is hosted by the anonymous [module](crate::Module) in
    /// the [store](crate::Store) of the [Vm].
    ///
    /// This is the final step to invoke a WASM function step by step. After instantiating a WASM module in the [Vm], the WASM module is registered into the [store](crate::Store) of the [Vm] as an anonymous module. Then repeatedly call this function to invoke the exported WASM functions by their names until the [Vm] is reset or a new WASM module is registered or loaded.
    ///
    /// # Arguments
    ///
    /// * `func_name` - The name of the exported WASM function to run.
    ///
    /// * `params` - The parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_function(
        &mut self,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        match self.active_instance() {
            Some(instance) => {
                let func = instance.get_func(func_name.as_ref())?;
                self.executor.run_func(&func, params)
            }
            None => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule))),
        }
    }

    #[cfg(feature = "async")]
    pub async fn run_function_async(
        &mut self,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(|| self.run_function(func_name, params))
            .await
            .unwrap()
    }

    /// Runs an exported WASM function by its name and the module's name in which the WASM function is hosted.
    ///
    /// After registering a WASM module in the [Vm], repeatedly call this function to run exported WASM functions by their function names and the module names until the [Vm] is reset.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the WASM module registered into the [store](crate::Store) of the [Vm].
    ///
    /// * `func_name` - The name of the exported WASM function to run.
    ///
    /// * `params` - The parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_registered_function(
        &mut self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // get the registered instance
        let instance = self.store.module(mod_name.as_ref())?;

        // get the target function
        let func = instance.get_func(func_name.as_ref())?;

        // run the function
        self.executor().run_func(&func, params)
    }

    #[cfg(feature = "async")]
    pub async fn run_registered_function_async(
        &mut self,
        mod_name: impl AsRef<str> + Send,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(|| self.run_registered_function(mod_name, func_name, params))
            .await
            .unwrap()
    }

    /// Returns the active (also called anonymous) module instance.
    pub fn active_instance(&self) -> Option<&Instance> {
        self.active_instance.as_ref()
    }

    pub fn active_instance_mut(&mut self) -> Option<&mut Instance> {
        self.active_instance.as_mut()
    }

    pub fn registered_module(&mut self, mod_name: impl AsRef<str>) -> WasmEdgeResult<Instance> {
        self.store.module(mod_name.as_ref())
    }
}
impl Drop for Vm {
    fn drop(&mut self) {
        // drop imports
        self.imports.drain();

        // drop host_registered_modules
        self.host_registered_modules.drain();
    }
}
impl Engine for Vm {
    fn run_func(
        &self,
        func: &Function,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let func_ty = func.ty()?;
        let returns_len = func_ty.returns_len();
        let mut returns = Vec::with_capacity(returns_len as usize);

        let executor = self.executor();
        unsafe {
            check(ffi::WasmEdge_ExecutorInvoke(
                executor.inner.0,
                func.inner.0 as *const _,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }

    fn run_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let func_ty = func_ref.ty()?;
        let returns_len = func_ty.returns_len();
        let mut returns = Vec::with_capacity(returns_len as usize);

        let executor = self.executor();
        unsafe {
            check(ffi::WasmEdge_ExecutorInvoke(
                executor.inner.0,
                func_ref.inner.0 as *const _,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(feature = "custom_wasi")]
    use crate::instance::custom_wasi_module::CustomWasiModule;
    #[cfg(all(unix, not(feature = "custom_wasi")))]
    use crate::instance::module::WasiModule;
    // #[cfg(target_os = "linux")]
    use crate::ImportModule;
    use crate::{
        error::{
            CoreCommonError, CoreError, CoreExecutionError, CoreLoadError, InstanceError, VmError,
            WasmEdgeError,
        },
        AsInstance, Config, Loader, Module, Store, WasmValue,
    };
    #[cfg(unix)]
    use crate::{
        error::{CoreInstantiationError, HostFuncError},
        AsImport, CallingFrame, FuncType, Function, ImportObject,
    };
    use wasmedge_types::{wat2wasm, ValType};

    #[test]
    fn test_vm_new_create() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        assert!(!config.wasi_enabled());

        // create a Vm context with the given Config and Store
        let vm = Vm::create(Some(config))?;

        #[cfg(feature = "custom_wasi")]
        let result = vm.custom_wasi_module();
        #[cfg(not(feature = "custom_wasi"))]
        let result = vm.wasi_module();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))
        );

        Ok(())
    }

    #[test]
    fn test_vm_new_register_instance_from_import() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;

        // create import module
        let mut import = ImportModule::create("extern")?;

        // add host function
        let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
        let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
        import.add_func("add", host_func);

        // register the import_obj module
        vm.register_instance_from_import(ImportObject::Import(import))?;

        // get ImportObj module
        #[cfg(not(feature = "custom_wasi"))]
        assert!(vm.wasi_module().is_ok());
        #[cfg(feature = "custom_wasi")]
        assert!(vm.custom_wasi_module().is_ok());
        #[cfg(target_os = "linux")]
        {
            let result = vm.wasmedge_process_module_mut();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule))
            );
        }

        Ok(())
    }

    #[test]
    fn test_vm_new_register_instance_from_file() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;

        // register a wasm module from a buffer
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        vm.register_instance_from_file("reg-wasm-buffer", path)?;

        Ok(())
    }

    #[test]
    fn test_vm_new_register_instance_from_bytes() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;

        // register a wasm module from a buffer
        let wasm_bytes = wat2wasm(
            br#"
            (module
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
        )?;
        vm.register_instance_from_bytes("reg-wasm-buffer", &wasm_bytes)?;

        Ok(())
    }

    #[test]
    #[cfg(unix)]
    fn test_vm_new_get_wasi_module() -> Result<(), Box<dyn std::error::Error>> {
        {
            // create a Config context
            let mut config = Config::create()?;
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());
            config.wasi(true);
            assert!(config.wasi_enabled());

            // create a Vm context with the given Config and Store
            let mut vm = Vm::create(Some(config))?;

            // get the default wasi module
            #[cfg(not(feature = "custom_wasi"))]
            let result = vm.wasi_module();
            #[cfg(feature = "custom_wasi")]
            let result = vm.custom_wasi_module();
            assert!(result.is_ok());

            // *** try to add another Wasi module, that causes error.

            // create a Wasi module
            #[cfg(not(feature = "custom_wasi"))]
            let import_wasi = WasiModule::create(None, None, None)?;
            #[cfg(feature = "custom_wasi")]
            let import_wasi = CustomWasiModule::create(None, None, None)?;

            #[cfg(not(feature = "custom_wasi"))]
            let result = vm.register_instance_from_import(ImportObject::Wasi(import_wasi));
            #[cfg(feature = "custom_wasi")]
            let result = vm.register_instance_from_import(ImportObject::CustomWasi(import_wasi));
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule))
            );

            // get store from vm
            let store = vm.store();
            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let mut config = Config::create()?;
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config and Store
            let mut vm = Vm::create(Some(config))?;

            // get the Wasi module
            #[cfg(not(feature = "custom_wasi"))]
            let result = vm.wasi_module();
            #[cfg(feature = "custom_wasi")]
            let result = vm.custom_wasi_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))
            );

            // *** try to add a Wasi module.

            // create a Wasi module
            #[cfg(not(feature = "custom_wasi"))]
            let mut import_wasi = WasiModule::create(None, None, None)?;
            #[cfg(feature = "custom_wasi")]
            let mut import_wasi = CustomWasiModule::create(None, None, None)?;

            // add host function
            let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
            let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
            import_wasi.add_func("add", host_func);

            #[cfg(not(feature = "custom_wasi"))]
            let result = vm.register_instance_from_import(ImportObject::Wasi(import_wasi));
            #[cfg(feature = "custom_wasi")]
            let result = vm.register_instance_from_import(ImportObject::CustomWasi(import_wasi));
            assert!(result.is_ok());
        }

        Ok(())
    }

    #[test]
    #[cfg(all(not(feature = "static"), target_os = "linux"))]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_new_get_wasmedge_process_module() -> Result<(), Box<dyn std::error::Error>> {
        use crate::{utils, WasmEdgeProcessModule};

        // load wasmedge_process plugins
        utils::load_plugin_from_default_paths();

        {
            // create a Config context
            let mut config = Config::create()?;
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());
            config.wasmedge_process(true);
            assert!(config.wasmedge_process_enabled());

            // create a Vm context with the given Config and Store
            let mut vm = Vm::create(Some(config))?;

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module();
            assert!(result.is_ok());

            // *** try to add another WasmEdgeProcess module, that causes error.

            // create a WasmEdgeProcess module
            let import_process = WasmEdgeProcessModule::create(None, false)?;
            let result =
                vm.register_instance_from_import(ImportObject::WasmEdgeProcess(import_process));
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule))
            );

            // get store from vm
            let store = vm.store();
            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let mut config = Config::create()?;
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config and Store
            let mut vm = Vm::create(Some(config))?;

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule,))
            );

            // *** try to add a WasmEdgeProcess module.

            // create a WasmEdgeProcess module
            let mut import_process = WasmEdgeProcessModule::create(None, false)?;

            // add host function
            let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
            let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
            import_process.add_func("add", host_func);

            let result =
                vm.register_instance_from_import(ImportObject::WasmEdgeProcess(import_process));
            assert!(result.is_ok());
        }

        Ok(())
    }

    #[test]
    fn test_vm_new_run_wasm_from_file() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config
        let result = Vm::create(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
        let result = vm.run_wasm_from_file(&path, "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from a non-existent file
        let result = vm.run_wasm_from_file("no_file.wasm", "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::IllegalPath
            )))
        );

        // run a function from a WASM file with the empty parameters
        let result = vm.run_wasm_from_file(&path, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function from a WASM file with the parameters of wrong type
        let result = vm.run_wasm_from_file(&path, "fib", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existent
        let result = vm.run_wasm_from_file(&path, "fib2", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );

        Ok(())
    }

    #[test]
    fn test_vm_new_run_wasm_from_bytes() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;

        // run a function from a in-memory wasm bytes
        let wasm_bytes = wat2wasm(
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
        )?;
        let returns = vm.run_wasm_from_bytes(&wasm_bytes, "fib", [WasmValue::from_i32(5)])?;
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from an empty buffer
        let empty_buffer: Vec<u8> = Vec::new();
        let result = vm.run_wasm_from_bytes(&empty_buffer, "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::UnexpectedEnd
            )))
        );

        // run a function with the empty parameters
        let result = vm.run_wasm_from_bytes(&wasm_bytes, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function with the parameters of wrong type
        let result = vm.run_wasm_from_bytes(&wasm_bytes, "fib", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existent
        let result = vm.run_wasm_from_bytes(&wasm_bytes, "fib2", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );

        Ok(())
    }

    #[cfg(feature = "async")]
    #[tokio::test]
    async fn test_vm_new_run_wasm_from_file_async() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
        let result = vm
            .run_wasm_from_file_async(&path, "fib", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from a non-existent file
        let result = vm
            .run_wasm_from_file_async("no_file", "fib", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::IllegalPath
            )))
        );

        // run a function from a WASM file with the empty parameters
        let result = vm.run_wasm_from_file_async(&path, "fib", []).await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function from a WASM file with the parameters of wrong type
        let result = vm
            .run_wasm_from_file_async(&path, "fib", [WasmValue::from_i64(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existent
        let result = vm
            .run_wasm_from_file_async(&path, "fib2", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".to_string()
            )))
        );
    }

    #[cfg(feature = "async")]
    #[tokio::test]
    async fn test_vm_new_run_wasm_from_bytes_async() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a in-memory wasm bytes
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
        let result = vm
            .run_wasm_from_bytes_async(&wasm_bytes, "fib", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from an empty buffer
        let empty_buffer: Vec<u8> = Vec::new();
        let result = vm
            .run_wasm_from_bytes_async(&empty_buffer, "fib", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::UnexpectedEnd
            )))
        );

        // run a function with the empty parameters
        let result = vm.run_wasm_from_bytes_async(&wasm_bytes, "fib", []).await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function with the parameters of wrong type
        let result = vm
            .run_wasm_from_bytes_async(&wasm_bytes, "fib", [WasmValue::from_i64(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existent
        let result = vm
            .run_wasm_from_bytes_async(&wasm_bytes, "fib2", [WasmValue::from_i64(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".to_string()
            )))
        );
    }

    fn real_add(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
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

        // simulate a long running operation
        std::thread::sleep(std::time::Duration::from_millis(100));

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
