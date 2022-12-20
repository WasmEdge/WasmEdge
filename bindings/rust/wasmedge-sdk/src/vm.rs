//! Defines WasmEdge Vm struct.

#[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
use crate::wasi::WasiNnInstance;
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
use crate::wasi::{
    WasiCryptoAsymmetricCommonInstance, WasiCryptoCommonInstance, WasiCryptoKxInstance,
    WasiCryptoSignaturesInstance, WasiCryptoSymmetricInstance,
};
#[cfg(target_os = "linux")]
use crate::WasmEdgeProcessInstance;
use crate::{
    config::Config, error::WasmEdgeError, wasi::WasiInstance, Engine, Func, FuncRef, FuncType,
    ImportObject, Instance, Module, Statistics, WasmEdgeResult, WasmValue,
};
use std::{marker::PhantomData, path::Path};
use wasmedge_sys::{self as sys, Engine as sys_engine};

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
#[derive(Debug, Clone)]
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
    /// * `file` - The `wasm` or `wat` file.
    ///
    /// # Error
    ///
    /// If fail to register the target WASM, then an error is returned.
    pub fn register_module_from_file(
        self,
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
        self,
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
        self,
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
        &self,
        mod_name: Option<&str>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
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

    /// Asynchronously runs an exported WASM function registered in a named or active module.
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
    #[cfg(feature = "async")]
    pub async fn run_func_async<M, N, A>(
        &self,
        mod_name: Option<M>,
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        M: AsRef<str> + Send,
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        match mod_name {
            Some(mod_name) => {
                // run a function in the registered module
                self.inner
                    .run_registered_function_async(mod_name, func_name.as_ref(), args)
                    .await
            }
            None => {
                // run a function in the active module
                self.inner
                    .run_function_async(func_name.as_ref(), args)
                    .await
            }
        }
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

    /// Runs an exported function from the given wasm file.
    ///
    /// # Arguments
    ///
    /// * `file` - The `wasm` or `wat` file.
    ///
    /// * `func_name` - The name of the target exported function to run.
    ///
    /// * `args` - The arguments passed to the target exported function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_func_from_file(
        &self,
        file: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        match file.as_ref().extension() {
            Some(extension) => match extension.to_str() {
                Some("wasm") => {
                    self.inner
                        .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)
                }
                #[cfg(target_os = "macos")]
                Some("dylib") => {
                    self.inner
                        .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)
                }
                #[cfg(target_os = "linux")]
                Some("so") => {
                    self.inner
                        .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)
                }
                #[cfg(target_os = "windows")]
                Some("dll") => {
                    self.inner
                        .run_wasm_from_file(file.as_ref(), func_name.as_ref(), args)
                }
                Some("wat") => {
                    let bytes = wat::parse_file(file.as_ref())
                        .map_err(|_| WasmEdgeError::Operation("Failed to parse wat file".into()))?;
                    self.inner.run_wasm_from_bytes(&bytes, func_name, args)
                }
                _ => Err(Box::new(WasmEdgeError::Operation(
                    "Invalid file extension".into(),
                ))),
            },
            None => Err(Box::new(WasmEdgeError::Operation(
                "Invalid file extension".into(),
            ))),
        }
    }

    /// Asynchronously runs an exported function from the given wasm file.
    ///
    /// # Arguments
    ///
    /// * `file` - The `wasm` or `wat` file.
    ///
    /// * `func_name` - The name of the target exported function to run.
    ///
    /// * `args` - The arguments passed to the target exported function.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    #[cfg(feature = "async")]
    pub async fn run_func_from_file_async<P, N, A>(
        &self,
        file: P,
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        P: AsRef<Path>,
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        match file.as_ref().extension() {
            Some(extension) => match extension.to_str() {
                Some("wasm") => {
                    self.inner
                        .run_wasm_from_file_async(file.as_ref(), func_name.as_ref(), args)
                        .await
                }
                #[cfg(target_os = "macos")]
                Some("dylib") => {
                    self.inner
                        .run_wasm_from_file_async(file.as_ref(), func_name.as_ref(), args)
                        .await
                }
                #[cfg(target_os = "linux")]
                Some("so") => {
                    self.inner
                        .run_wasm_from_file_async(file.as_ref(), func_name.as_ref(), args)
                        .await
                }
                #[cfg(target_os = "windows")]
                Some("dll") => {
                    self.inner
                        .run_wasm_from_file_async(file.as_ref(), func_name.as_ref(), args)
                        .await
                }
                Some("wat") => {
                    let bytes = wat::parse_file(file.as_ref())
                        .map_err(|_| WasmEdgeError::Operation("Failed to parse wat file".into()))?;
                    self.inner
                        .run_wasm_from_bytes_async(&bytes, func_name.as_ref(), args)
                        .await
                }
                _ => Err(Box::new(WasmEdgeError::Operation(
                    "Invalid file extension".into(),
                ))),
            },
            None => Err(Box::new(WasmEdgeError::Operation(
                "Invalid file extension".into(),
            ))),
        }
    }

    /// Runs an exported function from the given in-memory wasm bytes.
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
        &self,
        bytes: &[u8],
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.inner
            .run_wasm_from_bytes(bytes, func_name.as_ref(), args)
    }

    /// Runs an exported function from the given in-memory wasm bytes.
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
    #[cfg(feature = "async")]
    pub async fn run_func_from_bytes_async<N, A>(
        &self,
        bytes: &[u8],
        func_name: N,
        args: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        self.inner
            .run_wasm_from_bytes_async(bytes, func_name.as_ref(), args)
            .await
    }

    /// Runs an exported function from the given [compiled module](crate::Module).
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
        &self,
        module: Module,
        func_name: impl AsRef<str>,
        args: impl IntoIterator<Item = sys::WasmValue>,
    ) -> WasmEdgeResult<Vec<sys::WasmValue>> {
        self.inner
            .run_wasm_from_module(module.inner, func_name.as_ref(), args)
    }

    /// Runs an exported function from the given [compiled module](crate::Module).
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
    #[cfg(feature = "async")]
    pub async fn run_func_from_module_async<N, A>(
        &self,
        module: Module,
        func_name: N,
        params: A,
    ) -> WasmEdgeResult<Vec<WasmValue>>
    where
        N: AsRef<str> + Send,
        A: IntoIterator<Item = WasmValue> + Send,
    {
        self.inner
            .run_wasm_from_module_async(module.inner, func_name, params)
            .await
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

    /// Returns the [Wasi module instance](crate::wasi::WasiInstance).
    ///
    /// Notice that this function is only available when a [config](crate::config::Config) with the enabled [wasi](crate::config::HostRegistrationConfigOptions::wasi) option is used in the creation of this [Vm].
    ///
    /// # Error
    ///
    /// If fail to get the [Wasi module instance](crate::wasi::WasiInstance), then an error is returned.
    pub fn wasi_module(&mut self) -> WasmEdgeResult<WasiInstance> {
        let inner_wasi_module = self.inner.wasi_module_mut()?;

        Ok(WasiInstance {
            inner: inner_wasi_module,
        })
    }

    /// Returns the mutable [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance).
    ///
    /// Notice that this function is only available when a [config](crate::config::Config) with the enabled [wasmedge_process](crate::config::HostRegistrationConfigOptions::wasmedge_process) option is used in the creation of this [Vm]. In addition, the [PluginManager::load_from_default_paths](crate::PluginManager::load_from_default_paths) method must be invoked to load the `wasmedge_process` plugin before calling this method.
    ///
    /// # Error
    ///
    /// If fail to get the [WasmEdgeProcess module instance](crate::WasmEdgeProcessInstance), then an error is returned.
    #[cfg(target_os = "linux")]
    pub fn wasmedge_process_module(&mut self) -> WasmEdgeResult<WasmEdgeProcessInstance> {
        let inner_process_module = self.inner.wasmedge_process_module_mut()?;

        Ok(WasmEdgeProcessInstance {
            inner: inner_process_module,
        })
    }

    /// Returns the [WasiNnInstance module instance](crate::wasi::WasiNnInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
    pub fn wasi_nn_module(&mut self) -> WasmEdgeResult<WasiNnInstance> {
        let inner_wasi_nn_module = self.inner.wasi_nn_module()?;

        Ok(WasiNnInstance {
            inner: inner_wasi_nn_module,
        })
    }

    /// Returns the [WasiCryptoCommonInstance module instance](crate::wasi::WasiCryptoCommonInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_common_module(&mut self) -> WasmEdgeResult<WasiCryptoCommonInstance> {
        let inner_wasi_crypto_common_module = self.inner.wasi_crypto_common_module()?;

        Ok(WasiCryptoCommonInstance {
            inner: inner_wasi_crypto_common_module,
        })
    }

    /// Returns the [WasiCryptoAsymmetricCommonInstance module instance](crate::wasi::WasiCryptoAsymmetricCommonInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_asymmetric_common_module(
        &mut self,
    ) -> WasmEdgeResult<WasiCryptoAsymmetricCommonInstance> {
        let inner_wasi_crypto_asymmetric_common_module =
            self.inner.wasi_crypto_asymmetric_common_module()?;

        Ok(WasiCryptoAsymmetricCommonInstance {
            inner: inner_wasi_crypto_asymmetric_common_module,
        })
    }

    /// Returns the [WasiCryptoSymmetricInstance module instance](crate::wasi::WasiCryptoSymmetricInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_symmetric_module(&mut self) -> WasmEdgeResult<WasiCryptoSymmetricInstance> {
        let inner_wasi_crypto_symmetric_module = self.inner.wasi_crypto_symmetric_module()?;

        Ok(WasiCryptoSymmetricInstance {
            inner: inner_wasi_crypto_symmetric_module,
        })
    }

    /// Returns the [WasiCryptoKxInstance module instance](crate::wasi::WasiCryptoKxInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_kx_module(&mut self) -> WasmEdgeResult<WasiCryptoKxInstance> {
        let inner_wasi_crypto_kx_module = self.inner.wasi_crypto_kx_module()?;

        Ok(WasiCryptoKxInstance {
            inner: inner_wasi_crypto_kx_module,
        })
    }

    /// Returns the [WasiCryptoSignaturesInstance module instance](crate::wasi::WasiCryptoSignaturesInstance).
    #[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
    pub fn wasi_crypto_signatures_module(
        &mut self,
    ) -> WasmEdgeResult<WasiCryptoSignaturesInstance> {
        let inner_wasi_crypto_signatures_module = self.inner.wasi_crypto_signatures_module()?;

        Ok(WasiCryptoSignaturesInstance {
            inner: inner_wasi_crypto_signatures_module,
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
impl Engine for Vm {
    fn run_func(
        &self,
        func: &Func,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let executor = self.inner.executor()?;
        let returns = executor.run_func(&func.inner, params)?;
        Ok(returns)
    }

    fn run_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let executor = self.inner.executor()?;
        let returns = executor.run_func_ref(&func_ref.inner, params)?;
        Ok(returns)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(target_os = "linux")]
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
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

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
    #[allow(clippy::assertions_on_result_states)]
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
        let host_reg_options = HostRegistrationConfigOptions::default().wasi(true);
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
    #[cfg(target_os = "linux")]
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
    #[allow(clippy::assertions_on_result_states)]
    fn test_vm_register_module_from_file() {
        {
            // create a Vm context
            let result = Vm::new(None);
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert_eq!(vm.named_instance_count().unwrap(), 1);
            assert!(vm.instance_names().is_ok());
            assert_eq!(vm.instance_names().unwrap(), ["extern"]);
        }

        {
            // create a Vm context
            let result = Vm::new(None);
            assert!(result.is_ok());
            let vm = result.unwrap();

            // register a wasm module from a specified wasm file
            let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
                .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wat");
            let result = vm.register_module_from_file("extern", file);
            assert!(result.is_ok());
            let vm = result.unwrap();

            assert_eq!(vm.named_instance_count().unwrap(), 1);
            assert!(vm.instance_names().is_ok());
            assert_eq!(vm.instance_names().unwrap(), ["extern"]);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
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

    #[test]
    fn test_vm_impl_engine_trait() {
        // create a Config
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        // create a Vm context
        let result = Vm::new(Some(config));
        assert!(result.is_ok());
        let vm = result.unwrap();

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

        let result = vm.register_module_from_bytes("extern", wasm_bytes);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        let result = vm.named_module("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        let result = instance.func("fib");
        assert!(result.is_some());
        let fib = result.unwrap();

        let result = fib.call(&mut vm, params!(5));
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8)
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
