//! Defines WasmEdge Vm struct.

#[cfg(feature = "async")]
use crate::r#async::FiberFuture;
#[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
use crate::WasiNnModule;
#[cfg(target_os = "linux")]
use crate::WasmEdgeProcessModule;
use crate::{
    error::{VmError, WasmEdgeError},
    executor::{Executor, InnerExecutor},
    ffi,
    instance::{
        function::{FuncRef, FuncType, Function, InnerFuncType},
        module::InnerInstance,
    },
    loader::{InnerLoader, Loader},
    statistics::{InnerStat, Statistics},
    store::{InnerStore, Store},
    types::WasmEdgeString,
    utils::{self, check},
    validator::{InnerValidator, Validator},
    Config, Engine, ImportObject, Instance, Module, WasiModule, WasmEdgeResult, WasmValue,
};
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
use crate::{
    WasiCrypto, WasiCryptoAsymmetricCommonModule, WasiCryptoCommonModule, WasiCryptoKxModule,
    WasiCryptoSignaturesModule, WasiCryptoSymmetricModule,
};
use std::{collections::HashMap, path::Path, sync::Arc};

/// A [Vm] defines a virtual environment for managing WebAssembly programs.
#[derive(Debug, Clone)]
pub struct Vm {
    pub(crate) inner: Arc<InnerVm>,
    imports: HashMap<String, ImportObject>,
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
    pub fn create(config: Option<Config>, store: Option<&mut Store>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let vm_ctx = match store {
                    Some(store) => unsafe { ffi::WasmEdge_VMCreate(config.inner.0, store.inner.0) },
                    None => unsafe { ffi::WasmEdge_VMCreate(config.inner.0, std::ptr::null_mut()) },
                };

                let inner_config = &mut *std::sync::Arc::get_mut(&mut config.inner).unwrap();
                inner_config.0 = std::ptr::null_mut();

                vm_ctx
            }
            None => match store {
                Some(store) => unsafe {
                    ffi::WasmEdge_VMCreate(std::ptr::null_mut(), store.inner.0)
                },
                None => unsafe {
                    ffi::WasmEdge_VMCreate(std::ptr::null_mut(), std::ptr::null_mut())
                },
            },
        };

        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::Create))),
            false => Ok(Self {
                inner: Arc::new(InnerVm(ctx)),
                imports: HashMap::new(),
            }),
        }
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
    /// * `file` - The wasm file, of which the file extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.
    ///
    /// # Error
    ///
    /// If fail to register the target WASM, then an error is returned.
    pub fn register_wasm_from_file(
        &self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        match file.as_ref().extension() {
            Some(extension) => match extension.to_str() {
                Some("wasm") => self.register_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "macos")]
                Some("dylib") => self.register_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "linux")]
                Some("so") => self.register_from_wasm_or_aot_file(mod_name, file),
                #[cfg(target_os = "windows")]
                Some("dll") => self.register_from_wasm_or_aot_file(mod_name, file),
                Some("wat") => {
                    let bytes = wat::parse_file(file.as_ref())
                        .map_err(|_| WasmEdgeError::Operation("Failed to parse wat file".into()))?;
                    self.register_wasm_from_bytes(mod_name, &bytes)
                }
                _ => Err(Box::new(WasmEdgeError::Operation(
                    "The source file's extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.".into(),
                ))),
            },
            None => Err(Box::new(WasmEdgeError::Operation(
                "The source file's extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.".into(),
            ))),
        }
    }

    fn register_from_wasm_or_aot_file(
        &self,
        mod_name: impl AsRef<str>,
        file: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        let path = utils::path_to_cstring(file.as_ref())?;
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_VMRegisterModuleFromFile(
                self.inner.0,
                mod_name.as_raw(),
                path.as_ptr(),
            ))?
        };

        Ok(())
    }

    /// Registers a given WasmEdge module instance that implements the [ImportObject](crate::ImportObject) trait into the [Vm], and instantiates it.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// * First, registers the exported module instance into the [Vm], then
    ///
    /// * Instatiates the exported instances.
    ///
    ///
    /// # Argument
    ///
    /// * `import` - The module instance to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_wasm_from_import(&mut self, import: ImportObject) -> WasmEdgeResult<()> {
        let io_name: String = import.name().into();

        if self.imports.contains_key(&io_name) {
            return Err(Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule)));
        } else {
            self.imports.insert(io_name.clone(), import);
        }

        let import = self
            .imports
            .get(&io_name)
            .ok_or(WasmEdgeError::Vm(VmError::NotFoundImportModule(io_name)))?;

        match import {
            ImportObject::Import(import) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            ImportObject::Wasi(import) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(target_os = "linux")]
            ImportObject::WasmEdgeProcess(import) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
            ImportObject::Nn(import) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::Common(import)) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::AsymmetricCommon(import)) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::SymmetricOptionations(import)) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::KeyExchange(import)) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
            ImportObject::Crypto(WasiCrypto::Signatures(import)) => unsafe {
                check(ffi::WasmEdge_VMRegisterModuleFromImport(
                    self.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
        };

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
    pub fn register_wasm_from_bytes(
        &self,
        mod_name: impl AsRef<str>,
        bytes: &[u8],
    ) -> WasmEdgeResult<()> {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_VMRegisterModuleFromBuffer(
                self.inner.0,
                mod_name.as_raw(),
                bytes.as_ptr(),
                bytes.len() as u32,
            ))?;
        }

        Ok(())
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
    pub fn register_wasm_from_module(
        &self,
        mod_name: impl AsRef<str>,
        mut module: Module,
    ) -> WasmEdgeResult<()> {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_VMRegisterModuleFromASTModule(
                self.inner.0,
                mod_name.as_raw(),
                module.inner.0,
            ))?;
        }
        let inner_module = &mut *std::sync::Arc::get_mut(&mut module.inner).unwrap();
        inner_module.0 = std::ptr::null_mut();

        Ok(())
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
    /// * `file` - The wasm file, of which the file extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.
    ///
    /// * `func_name` - The name of the [function](crate::Function).
    ///
    /// * `params` - The the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_file(
        &self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_file(file)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Asynchrously runs a [function](crate::Function) defined in a WASM file.
    ///
    /// # Arguments
    ///
    /// * `file` - The wasm file, of which the file extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.
    ///
    /// * `func_name` - The name of the [function](crate::Function).
    ///
    /// * `params` - The the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_wasm_from_file_async(
        &self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_file(file)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function_async(func_name, params).await
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
        &self,
        bytes: &[u8],
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_bytes(bytes)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Asychronously runs a [function](crate::Function) from a in-memory wasm bytes.
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
    #[cfg(feature = "async")]
    pub async fn run_wasm_from_bytes_async(
        &self,
        bytes: &[u8],
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_bytes(bytes)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function_async(func_name, params).await
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
        &self,
        module: Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_module(&module)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Asynchrously runs a [function](crate::Function) from a WasmEdge compiled [Module](crate::Module).
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
    #[cfg(feature = "async")]
    pub async fn run_wasm_from_module_async(
        &self,
        module: Module,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // load
        self.load_wasm_from_module(&module)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function_async(func_name, params).await
    }

    /// Loads a WASM module from a WasmEdge AST [Module](crate::Module).
    ///
    /// This is the first step to invoke a WASM function step by step.
    ///
    /// # Argument
    ///
    /// * `module` - A WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or [Compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_module(&self, module: &Module) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_VMLoadWasmFromASTModule(
                self.inner.0,
                module.inner.0 as *const _,
            ))?;
        }
        Ok(())
    }

    /// Loads a WASM module from a in-memory WASM bytes. The loaded module is not validated.
    ///
    /// This is the first step to invoke a WASM function step by step.
    ///
    /// # Argument
    ///
    /// * `bytes` - The in-memory wasm bytes.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.  The loaded module is not validated.
    pub fn load_wasm_from_bytes(&self, bytes: &[u8]) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_VMLoadWasmFromBuffer(
                self.inner.0,
                bytes.as_ptr() as *const _,
                bytes.len() as u32,
            ))?;
        }
        Ok(())
    }

    /// Loads a WASM module from a WASM file.
    ///
    /// This is the first step to invoke a WASM function step by step.
    ///
    /// # Argument
    ///
    /// * `file` - The wasm file, of which the file extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.  The loaded module is not validated.
    pub fn load_wasm_from_file(&self, file: impl AsRef<Path>) -> WasmEdgeResult<()> {
        match file.as_ref().extension() {
            Some(extension) => match extension.to_str() {
                Some("wasm") => self.load_from_wasm_or_aot_file(&file),
                #[cfg(target_os = "macos")]
                Some("dylib") => self.load_from_wasm_or_aot_file(&file),
                #[cfg(target_os = "linux")]
                Some("so") => self.load_from_wasm_or_aot_file(&file),
                #[cfg(target_os = "windows")]
                Some("dll") => self.load_from_wasm_or_aot_file(&file),
                Some("wat") => {
                    let bytes = wat::parse_file(file.as_ref())
                        .map_err(|_| WasmEdgeError::Operation("Failed to parse wat file".into()))?;
                    self.load_wasm_from_bytes(&bytes)
                }
                _ => Err(Box::new(WasmEdgeError::Operation(
                    "The source file's extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.".into(),
                ))),
            },
            None => Err(Box::new(WasmEdgeError::Operation(
                "The source file's extension should be one of `wasm`, `wat`, `dylib` on macOS, `so` on Linux or `dll` on Windows.".into(),
            ))),
        }
    }

    fn load_from_wasm_or_aot_file(&self, path: impl AsRef<Path>) -> WasmEdgeResult<()> {
        let path = utils::path_to_cstring(path.as_ref())?;
        unsafe {
            check(ffi::WasmEdge_VMLoadWasmFromFile(
                self.inner.0,
                path.as_ptr(),
            ))?;
        }
        Ok(())
    }

    /// Validates a WASM module loaded into the [Vm].
    ///
    /// This is the second step to invoke a WASM function step by step. After loading a WASM module into the [Vm], call this function to validate it. Note that only validated WASM modules can be instantiated in the [Vm].
    ///
    /// # Error
    ///
    /// If fail to validate, then an error is returned.
    pub fn validate(&self) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_VMValidate(self.inner.0))?;
        }
        Ok(())
    }

    /// Instantiates a validated WASM module in the [Vm].
    ///
    /// This is the third step to invoke a WASM function step by step. After validating a WASM module in the [Vm], call this function to instantiate it; then, call `execute` to invoke the exported function in this WASM module.
    ///
    /// # Error
    ///
    /// If fail to instantiate, then an error is returned.
    pub fn instantiate(&self) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_VMInstantiate(self.inner.0))?;
        }
        Ok(())
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
        &self,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // prepare parameters
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // prepare returns
        let func_type = self.get_function_type(func_name.as_ref())?;

        // get the info of the funtion return
        let returns_len = unsafe { ffi::WasmEdge_FunctionTypeGetReturnsLength(func_type.inner.0) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_VMExecute(
                self.inner.0,
                func_name.as_raw(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }

    /// Asynchronously runs an exported WASM function by name. The WASM function is hosted by the anonymous [module](crate::Module) in the [store](crate::Store) of the [Vm].
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
    #[cfg(feature = "async")]
    pub async fn run_function_async(
        &self,
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
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        // prepare parameters
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // prepare returns
        let func_type = self.get_registered_function_type(mod_name.as_ref(), func_name.as_ref())?;

        // get the info of the funtion return
        let returns_len = unsafe { ffi::WasmEdge_FunctionTypeGetReturnsLength(func_type.inner.0) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_VMExecuteRegistered(
                self.inner.0,
                mod_name.as_raw(),
                func_name.as_raw(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }

    /// Asynchronously runs an exported WASM function by its name and the module's name in which the WASM function is hosted.
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
    #[cfg(feature = "async")]
    pub async fn run_registered_function_async(
        &self,
        mod_name: impl AsRef<str> + Send,
        func_name: impl AsRef<str> + Send,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(|| self.run_registered_function(mod_name, func_name, params))
            .await
            .unwrap()
    }

    /// Returns the function type of a WASM function by its name. The function is hosted in the anonymous [module](crate::Module) of the [Vm].
    ///
    /// # Argument
    ///
    /// * `func_name` - The name of the target WASM function.
    ///
    /// # Error
    ///
    /// If fail to get the function type, then an error is returned.
    pub fn get_function_type(&self, func_name: impl AsRef<str>) -> WasmEdgeResult<FuncType> {
        let active_instance = self.active_module()?;
        let func = active_instance.get_func(func_name.as_ref())?;
        func.ty()
    }

    /// Returns the function type of a WASM function by its name and the name of the registered [module](crate::Module)
    /// which hosts the WASM function.
    ///
    /// # Arguments
    ///
    /// * `mod_name` - The name of the registered [module](crate::Module).
    ///
    /// * `func_name` - The name of the target WASM function.
    pub fn get_registered_function_type(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<FuncType> {
        if !self.contains_module(mod_name.as_ref()) {
            return Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                mod_name.as_ref().into(),
            ))));
        }

        self.store_mut()?
            .module(mod_name.as_ref())?
            .get_func(func_name.as_ref())?
            .ty()
    }

    /// Resets the [`Vm`].
    pub fn reset(&mut self) {
        unsafe { ffi::WasmEdge_VMCleanup(self.inner.0) }
    }

    /// Returns the number of the exported functions.
    pub fn function_list_len(&self) -> usize {
        unsafe { ffi::WasmEdge_VMGetFunctionListLength(self.inner.0) as usize }
    }

    /// Returns an iterator of pairs of the exported function's name and type.
    pub fn function_iter(&self) -> impl Iterator<Item = (Option<String>, Option<FuncType>)> {
        let len = self.function_list_len();
        let mut names = Vec::with_capacity(len);
        let mut types = Vec::with_capacity(len);
        unsafe {
            ffi::WasmEdge_VMGetFunctionList(
                self.inner.0,
                names.as_mut_ptr(),
                types.as_mut_ptr(),
                len as u32,
            );
            names.set_len(len);
            types.set_len(len);
        };

        names.into_iter().zip(types.into_iter()).map(|(name, ty)| {
            let name = unsafe { std::ffi::CStr::from_ptr(name.Buf as *const _) };
            let name = name.to_string_lossy().into_owned();
            let func_ty = match ty.is_null() {
                true => None,
                false => Some(FuncType {
                    inner: InnerFuncType(ty as *mut _),
                    registered: true,
                }),
            };
            (Some(name), func_ty)
        })
    }

    /// Returns the mutable [Wasi module instance](crate::WasiModule).
    ///
    /// Notice that this function is only available when a [config](crate::Config) with the enabled [wasi](crate::Config::wasi) option is used in the creation of this [Vm].
    pub fn wasi_module_mut(&mut self) -> WasmEdgeResult<WasiModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_Wasi,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))),
            false => Ok(WasiModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the mutable [WasmEdgeProcess module instance](crate::WasmEdgeProcessModule).
    ///
    /// Notice that this function is only available when a [config](crate::Config) with the enabled [wasmedge_process](crate::Config::wasmedge_process) option is used in the creation of this [Vm].
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_module_mut(&mut self) -> WasmEdgeResult<WasmEdgeProcessModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasmEdge_Process,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasmEdgeProcessModule,
            ))),
            false => Ok(WasmEdgeProcessModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiNnModule module instance](crate::WasiNnModule).
    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn_module(&mut self) -> WasmEdgeResult<WasiNnModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiNN,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiNnModule))),
            false => Ok(WasiNnModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiCryptoCommonModule module instance](crate::WasiCryptoCommonModule).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common_module(&mut self) -> WasmEdgeResult<WasiCryptoCommonModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Common,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasiCryptoCommonModule,
            ))),
            false => Ok(WasiCryptoCommonModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiCryptoAsymmetricCommonModule module instance](crate::WasiCryptoAsymmetricCommonModule).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common_module(
        &mut self,
    ) -> WasmEdgeResult<WasiCryptoAsymmetricCommonModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_AsymmetricCommon,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasiCryptoAsymmetricCommonModule,
            ))),
            false => Ok(WasiCryptoAsymmetricCommonModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiCryptoSymmetricModule module instance](crate::WasiCryptoSymmetricModule).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric_module(&mut self) -> WasmEdgeResult<WasiCryptoSymmetricModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Symmetric,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasiCryptoSymmetricModule,
            ))),
            false => Ok(WasiCryptoSymmetricModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiCryptoKxModule module instance](crate::WasiCryptoKxModule).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx_module(&mut self) -> WasmEdgeResult<WasiCryptoKxModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Kx,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasiCryptoKxModule,
            ))),
            false => Ok(WasiCryptoKxModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the [WasiCryptoSignaturesModule module instance](crate::WasiCryptoSignaturesModule).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures_module(&mut self) -> WasmEdgeResult<WasiCryptoSignaturesModule> {
        let io_ctx = unsafe {
            ffi::WasmEdge_VMGetImportModuleContext(
                self.inner.0,
                ffi::WasmEdge_HostRegistration_WasiCrypto_Signatures,
            )
        };
        match io_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(
                VmError::NotFoundWasiCryptoSignaturesModule,
            ))),
            false => Ok(WasiCryptoSignaturesModule {
                inner: Arc::new(InnerInstance(io_ctx)),
                registered: true,
            }),
        }
    }

    /// Returns the internal [Store](crate::Store) from the [Vm].
    pub fn store_mut(&self) -> WasmEdgeResult<Store> {
        let store_ctx = unsafe { ffi::WasmEdge_VMGetStoreContext(self.inner.0) };
        match store_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundStore))),
            false => Ok(Store {
                inner: InnerStore(store_ctx),
                registered: true,
            }),
        }
    }

    /// Returns the internal [Statistics](crate::Statistics) from the [Vm].
    pub fn statistics_mut(&self) -> WasmEdgeResult<Statistics> {
        let stat_ctx = unsafe { ffi::WasmEdge_VMGetStatisticsContext(self.inner.0) };
        match stat_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundStatistics))),
            false => Ok(Statistics {
                inner: InnerStat(stat_ctx),
                registered: true,
            }),
        }
    }

    /// Returns the active (also called anonymous) module instance.
    pub fn active_module(&self) -> WasmEdgeResult<Instance> {
        let ctx = unsafe { ffi::WasmEdge_VMGetActiveModule(self.inner.0 as *const _) };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundActiveModule))),
            false => Ok(Instance {
                inner: std::sync::Arc::new(InnerInstance(ctx as *mut _)),
                registered: true,
            }),
        }
    }

    /// Checks if the [Vm] contains a registered module of which the name matches the given `mod_name`.
    ///
    /// # Argument
    ///
    /// * `mod_name` - The registered module's name to check.
    ///
    pub fn contains_module(&self, mod_name: impl AsRef<str>) -> bool {
        match self.store_mut() {
            Ok(store) => store.contains(mod_name),
            Err(_) => false,
        }
    }

    /// Returns the internal [Loader](crate::Loader) from the [Vm].
    pub fn loader(&self) -> WasmEdgeResult<Loader> {
        let loader_ctx = unsafe { ffi::WasmEdge_VMGetLoaderContext(self.inner.0) };
        match loader_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundLoader))),
            false => Ok(Loader {
                inner: InnerLoader(loader_ctx),
                registered: true,
            }),
        }
    }

    /// Returns the internal [Validator](crate::Validator) from the [Vm].
    pub fn validator(&self) -> WasmEdgeResult<Validator> {
        let validator_ctx = unsafe { ffi::WasmEdge_VMGetValidatorContext(self.inner.0) };
        match validator_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundValidator))),
            false => Ok(Validator {
                inner: InnerValidator(validator_ctx),
                registered: true,
            }),
        }
    }

    /// Returns the internal [Executor](crate::Executor) from the [Vm].
    pub fn executor(&self) -> WasmEdgeResult<Executor> {
        let executor_ctx = unsafe { ffi::WasmEdge_VMGetExecutorContext(self.inner.0) };
        match executor_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Vm(VmError::NotFoundExecutor))),
            false => Ok(Executor {
                inner: InnerExecutor(executor_ctx),
                registered: true,
            }),
        }
    }
}
impl Drop for Vm {
    fn drop(&mut self) {
        if Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_VMDelete(self.inner.0) };
        }

        // drop imports
        self.imports.drain();
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

        let executor = self.executor()?;
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

        let executor = self.executor()?;
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

#[derive(Debug)]
pub(crate) struct InnerVm(pub(crate) *mut ffi::WasmEdge_VMContext);
unsafe impl Send for InnerVm {}
unsafe impl Sync for InnerVm {}

#[cfg(test)]
mod tests {
    use super::Vm;
    use crate::{
        error::{
            CoreCommonError, CoreError, CoreExecutionError, CoreLoadError, InstanceError, VmError,
            WasmEdgeError,
        },
        Config, Loader, Module, Store, WasmValue,
    };
    #[cfg(unix)]
    use crate::{
        error::{CoreInstantiationError, HostFuncError},
        AsImport, CallingFrame, FuncType, Function, ImportObject, WasiModule,
    };
    #[cfg(target_os = "linux")]
    use crate::{utils, ImportModule, WasmEdgeProcessModule};
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{wat2wasm, ValType};

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_create() {
        {
            // create a Vm context without Config and Store
            let result = Vm::create(None, None);
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.inner.0.is_null());

            // get store
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();
            assert!(!store.inner.0.is_null() && store.registered);
        }

        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Store context
            let result = Store::create();
            assert!(result.is_ok(), "Failed to create Store instance");
            let mut store = result.unwrap();

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), Some(&mut store));
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.inner.0.is_null());

            // get store
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();
            assert!(!store.inner.0.is_null() && store.registered);
        }

        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.inner.0.is_null());

            // get store
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();
            assert!(!store.inner.0.is_null() && store.registered);
        }

        {
            // create a Store context
            let result = Store::create();
            assert!(result.is_ok(), "Failed to create Store instance");
            let mut store = result.unwrap();

            // create a Vm context with the given Store
            let result = Vm::create(None, Some(&mut store));
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.inner.0.is_null());

            // get store
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();
            assert!(!store.inner.0.is_null() && store.registered);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_load_wasm_from_file() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from a specified file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = vm.load_wasm_from_file(path);
        assert!(result.is_ok());

        // load a wasm module from a non-existent file
        let result = vm.load_wasm_from_file("no_file.wasm");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::IllegalPath
            )))
        );
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_load_wasm_from_buffer() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from buffer
        let result = wat2wasm(
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
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();

        let result = vm.load_wasm_from_bytes(&wasm_bytes);
        assert!(result.is_ok());

        // load wasm module from an empty buffer
        let empty_buffer: Vec<u8> = vec![];
        let result = vm.load_wasm_from_bytes(&empty_buffer);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::UnexpectedEnd
            )))
        );
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_load_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // load wasm module from an ast module
        let result = vm.load_wasm_from_module(&module);
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_validate() {
        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        let result = vm.validate();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Common(
                CoreCommonError::WrongVMWorkflow
            )))
        );

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // load a wasm module from a ast module
        let result = vm.load_wasm_from_module(&module);
        assert!(result.is_ok());

        let result = vm.validate();
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_instantiate() {
        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        let result = vm.instantiate();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Common(
                CoreCommonError::WrongVMWorkflow
            )))
        );

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        let result = vm.load_wasm_from_module(&module);
        assert!(result.is_ok());

        // call instantiate before validate
        let result = vm.instantiate();
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Common(
                CoreCommonError::WrongVMWorkflow
            )))
        );

        // call validate, then instantiate
        let result = vm.validate();
        assert!(result.is_ok());
        let result = vm.instantiate();
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_invoke_wasm_function_step_by_step() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let ast_module = result.unwrap();

        // create Vm instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from a ast module instance
        let result = vm.load_wasm_from_module(&ast_module);
        assert!(result.is_ok());

        // validate vm instance
        let result = vm.validate();
        assert!(result.is_ok());

        // instantiate
        let result = vm.instantiate();
        assert!(result.is_ok());

        // run function
        let result = vm.run_function("fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let values = result.unwrap();
        assert_eq!(values[0].to_i32(), 8);

        // run function with empty parameter
        let result = vm.run_function("fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function with the parameters of wrong type
        let result = vm.run_function("fib", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function: the specified function name is non-existant
        let result = vm.run_function("fib2", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );

        // check function types
        let result = vm.get_function_type("fib");
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.params_len(), 1);
        assert_eq!(
            func_ty.params_type_iter().collect::<Vec<_>>(),
            vec![ValType::I32]
        );
        assert_eq!(func_ty.returns_len(), 1);
        assert_eq!(
            func_ty.returns_type_iter().collect::<Vec<_>>(),
            vec![ValType::I32]
        );

        // check functions
        let functions = vm.function_iter().collect::<Vec<_>>();
        assert_eq!(functions.len(), 1);
        let pair = &functions[0];
        let func_name = pair.0.as_ref();
        assert!(func_name.is_some());
        assert_eq!(func_name.unwrap(), "fib");
        let func_ty = pair.1.as_ref();
        assert!(func_ty.is_some());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_wasm_from_file() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from a non-existed file
        let result = vm.register_wasm_from_file("reg-wasm-file", "no_file.wasm");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Load(
                CoreLoadError::IllegalPath
            )))
        );

        // register a wasm module from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = vm.register_wasm_from_file("reg-wasm-file", path);
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a wasm module from an AST module
        let mod_name = "reg-wasm-ast";
        let result = vm.register_wasm_from_module(mod_name, module);
        assert!(result.is_ok());

        // run a registered function
        let result = vm.run_registered_function(mod_name, "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // get the registered function type
        let result = vm.get_registered_function_type(mod_name, "fib");
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.params_len(), 1);
        assert_eq!(
            func_ty.params_type_iter().collect::<Vec<_>>(),
            vec![ValType::I32]
        );
        assert_eq!(func_ty.returns_len(), 1);
        assert_eq!(
            func_ty.returns_type_iter().collect::<Vec<_>>(),
            vec![ValType::I32]
        );

        // get the registered function type by a wrong module name
        let result = vm.get_registered_function_type("non-existent-module", "fib");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Vm(VmError::NotFoundModule(
                "non-existent-module".into()
            )))
        );

        // run a registered function with empty parameters
        let empty_params: Vec<WasmValue> = vec![];
        let result = vm.run_registered_function(mod_name, "fib", empty_params);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a registered function with the parameters of wrong type
        let result = vm.run_registered_function(mod_name, "fib", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a registered function but give a wrong function name.
        let result = vm.run_registered_function(mod_name, "fib2", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );
    }

    #[test]
    #[cfg(target_os = "linux")]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_wasm_from_import() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // create import module
        let result = ImportModule::create("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // register the import_obj module
        let result = vm.register_wasm_from_import(ImportObject::Import(import));
        assert!(result.is_ok());

        vm.reset();

        // get ImportObj module
        let result = vm.wasi_module_mut();
        assert!(result.is_ok());
        let result = vm.wasmedge_process_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule))
        );

        // get store
        let result = vm.store_mut();
        assert!(result.is_ok());

        // get statistics
        let result = vm.statistics_mut();
        assert!(result.is_ok());
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_wasm_from_buffer() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register a wasm module from a buffer
        let result = wat2wasm(
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
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();
        let result = vm.register_wasm_from_bytes("reg-wasm-buffer", &wasm_bytes);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_run_wasm_from_file() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

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

        // fun a function: the specified function name is non-existant
        let result = vm.run_wasm_from_file(&path, "fib2", [WasmValue::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );
    }

    #[cfg(feature = "async")]
    #[tokio::test]
    async fn test_vm_run_wasm_from_file_async() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.interruptible(true);
        assert!(config.interruptible_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
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

        // fun a function: the specified function name is non-existant
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

    #[test]
    fn test_vm_run_wasm_from_bytes() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

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
        let result = vm.run_wasm_from_bytes(&wasm_bytes, "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
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

        // fun a function: the specified function name is non-existant
        let result = vm.run_wasm_from_bytes(&wasm_bytes, "fib2", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );
    }

    #[cfg(feature = "async")]
    #[tokio::test]
    async fn test_vm_run_wasm_from_bytes_async() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.interruptible(true);
        assert!(config.interruptible_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let vm = result.unwrap();

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

        // fun a function: the specified function name is non-existant
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

    #[test]
    fn test_vm_run_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // run a function from a module
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", [WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function with the empty parameters
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function with the parameters of wrong type
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existant
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib2", [WasmValue::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".into()
            )))
        );
    }

    #[cfg(feature = "async")]
    #[tokio::test]
    async fn test_vm_run_wasm_from_module_async() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.interruptible(true);
        assert!(config.interruptible_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // run a function from a module
        let module = load_fib_module();
        let result = vm
            .run_wasm_from_module_async(module, "fib", [WasmValue::from_i32(5)])
            .await;
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function with the empty parameters
        let module = load_fib_module();
        let result = vm.run_wasm_from_module_async(module, "fib", []).await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // run a function with the parameters of wrong type
        let module = load_fib_module();
        let result = vm
            .run_wasm_from_module_async(module, "fib", [WasmValue::from_i64(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Core(CoreError::Execution(
                CoreExecutionError::FuncTypeMismatch
            )))
        );

        // fun a function: the specified function name is non-existant
        let module = load_fib_module();
        let result = vm
            .run_wasm_from_module_async(module, "fib2", [WasmValue::from_i64(5)])
            .await;
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                "fib2".to_string()
            )))
        );
    }

    #[test]
    fn test_vm_send() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = result.unwrap();
        assert!(!vm.inner.0.is_null());

        let handle = thread::spawn(move || {
            // run a function from a wasm file
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
            let result = vm.run_wasm_from_file(path, "fib", [WasmValue::from_i32(5)]);
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 8);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_vm_sync() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let vm = Arc::new(Mutex::new(result.unwrap()));

        let vm_cloned = Arc::clone(&vm);
        let handle = thread::spawn(move || {
            let result = vm_cloned.lock();
            assert!(result.is_ok());
            let vm = result.unwrap();

            // run a function from a wasm file
            let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
            let result = vm.run_wasm_from_file(path, "fib", [WasmValue::from_i32(5)]);
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 8);
        });

        handle.join().unwrap();
    }

    #[test]
    #[cfg(unix)]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_get_wasi_module() {
        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());
            config.wasi(true);
            assert!(config.wasi_enabled());

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the Wasi module
            let result = vm.wasi_module_mut();
            assert!(result.is_ok());

            // *** try to add another Wasi module, that causes error.

            // create a Wasi module
            let result = WasiModule::create(None, None, None);
            assert!(result.is_ok());
            let import_wasi = result.unwrap();

            let result = vm.register_wasm_from_import(ImportObject::Wasi(import_wasi));
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Core(CoreError::Instantiation(
                    CoreInstantiationError::ModuleNameConflict
                )))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the Wasi module
            let result = vm.wasi_module_mut();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))
            );

            // *** try to add a Wasi module.

            // create a Wasi module
            let result = WasiModule::create(None, None, None);
            assert!(result.is_ok());
            let mut import_wasi = result.unwrap();

            // add host function
            let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            let result = Function::create(&func_ty, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import_wasi.add_func("add", host_func);

            let result = vm.register_wasm_from_import(ImportObject::Wasi(import_wasi));
            assert!(result.is_ok());

            // get the Wasi module
            let result = vm.wasi_module_mut();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

            // get wasi module instance
            let result = store.module("wasi_snapshot_preview1");
            assert!(result.is_ok());
            let instance = result.unwrap();

            // get "add" function
            let result = instance.get_func("add");
            assert!(result.is_ok());
        }
    }

    #[test]
    #[cfg(target_os = "linux")]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_get_wasmedge_process_module() {
        // load wasmedge_process plugins
        utils::load_plugin_from_default_paths();

        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());
            config.wasmedge_process(true);
            assert!(config.wasmedge_process_enabled());

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module_mut();
            assert!(result.is_ok());

            // *** try to add another WasmEdgeProcess module, that causes error.

            // create a WasmEdgeProcess module
            let result = WasmEdgeProcessModule::create(None, false);
            assert!(result.is_ok());
            let import_process = result.unwrap();

            let result =
                vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process));
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Core(CoreError::Instantiation(
                    CoreInstantiationError::ModuleNameConflict
                )))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = Config::create();
            assert!(result.is_ok());
            let mut config = result.unwrap();
            config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module_mut();
            assert!(result.is_err());

            // *** try to add a WasmEdgeProcess module.

            // create a WasmEdgeProcess module
            let result = WasmEdgeProcessModule::create(None, false);
            assert!(result.is_ok());
            let mut import_process = result.unwrap();

            // add host function
            let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            let result = Function::create(&func_ty, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import_process.add_func("add", host_func);

            let result =
                vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process));
            assert!(result.is_ok());

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module_mut();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            // check registered modules
            assert_eq!(store.module_len(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);

            // get wasmedge_process module instance
            let result = store.module("wasmedge_process");
            assert!(result.is_ok());
            let instance = result.unwrap();

            // get "add" function
            let result = instance.get_func("add");
            assert!(result.is_ok());
        }
    }

    #[test]
    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_get_wasinn_module() {
        use crate::AsInstance;

        utils::load_plugin_from_default_paths();

        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        config.wasi_nn(true);
        assert!(config.wasi_nn_enabled());

        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        let result = vm.wasi_nn_module();
        assert!(result.is_ok());
        let wasi_nn_module = result.unwrap();
        assert_eq!(wasi_nn_module.func_len(), 5);

        wasi_nn_module
            .func_names()
            .unwrap()
            .iter()
            .for_each(|name| println!("func name: {}", name));

        let result = wasi_nn_module.get_func("load");
        assert!(result.is_ok());
        let load = result.unwrap();
        let result = load.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        println!("load: len of params: {}", ty.params_len());
        ty.params_type_iter()
            .for_each(|p| println!("load: param ty: {:?}", p));
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_impl_engine_trait() {
        // read the wasm bytes of fibonacci.wasm
        let result = wat2wasm(
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
        );
        assert!(result.is_ok());
        let wasm_bytes = result.unwrap();

        // create Vm instance
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // load wasm from bytes
        let result = vm.load_wasm_from_bytes(&wasm_bytes);
        assert!(result.is_ok());

        // validate vm instance
        let result = vm.validate();
        assert!(result.is_ok());

        // instantiate
        let result = vm.instantiate();
        assert!(result.is_ok());

        // get active module
        let result = vm.active_module();
        assert!(result.is_ok());
        let active_module = result.unwrap();

        // get the exported function "fib"
        let result = active_module.get_func("fib");
        assert!(result.is_ok());
        let fib = result.unwrap();

        let result = fib.call(&mut vm, vec![WasmValue::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);
    }

    fn load_fib_module() -> Module {
        // load a module
        let result = Loader::create(None);
        assert!(result.is_ok());
        let loader = result.unwrap();
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        result.unwrap()
    }

    #[cfg(unix)]
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
