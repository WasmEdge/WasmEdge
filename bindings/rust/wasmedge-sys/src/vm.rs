//! Defines WasmEdge Vm struct.

use super::wasmedge;
use crate::{
    error::{check, VmError, WasmEdgeError, WasmEdgeResult},
    instance::function::FuncType,
    types::WasmEdgeString,
    utils,
    wasmedge::{WasmEdge_HostRegistration_Wasi, WasmEdge_HostRegistration_WasmEdge_Process},
    Config, ImportObject, Module, Statistics, Store, Value,
};
use std::path::Path;

/// Struct of WasmEdge Vm.
///
/// A [`Vm`] defines a virtual environment for managing WebAssembly programs.
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
}
impl Vm {
    /// Creates a new [`Vm`] to be associated with the given [configuration](crate::Config) and [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// - `config` specifies a configuration for the new [`Vm`].
    ///
    /// - `store` specifies an external WASM [store](crate::Store) used by the new [`Vm`]. The instantiation and
    /// execution of the new [`Vm`] will consume this store context. If no store context is specified when creating
    /// a [`Vm`], then the [`Vm`] itself will allocate and own a [`store`](crate::Store).
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create(config: Option<Config>, store: Option<Store>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => {
                let vm_ctx = match store {
                    Some(mut store) => {
                        let vm_ctx = unsafe { wasmedge::WasmEdge_VMCreate(config.ctx, store.ctx) };
                        store.ctx = std::ptr::null_mut();
                        vm_ctx
                    }
                    None => unsafe {
                        wasmedge::WasmEdge_VMCreate(config.ctx, std::ptr::null_mut())
                    },
                };
                config.ctx = std::ptr::null_mut();
                vm_ctx
            }
            None => match store {
                Some(mut store) => {
                    let vm_ctx =
                        unsafe { wasmedge::WasmEdge_VMCreate(std::ptr::null_mut(), store.ctx) };
                    store.ctx = std::ptr::null_mut();
                    vm_ctx
                }
                None => unsafe {
                    wasmedge::WasmEdge_VMCreate(std::ptr::null_mut(), std::ptr::null_mut())
                },
            },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::Create)),
            false => Ok(Self { ctx }),
        }
    }

    /// Registers and instantiates a WASM module into the [store](crate::Store) of the [`Vm`] from a WASM file.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads a WASM module from a given path, then
    ///
    /// - Registers all exported instances in the WASM module into the [store](crate::Store) of the [`Vm`];
    ///
    /// - Finally, instantiates the exported instances.
    ///
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name for the WASM module to be registered.
    ///
    /// - `path` specifies the file path to the target WASM file.
    ///
    /// # Error
    ///
    /// If fail to register the target WASM, then an error is returned.
    pub fn register_wasm_from_file(
        &mut self,
        mod_name: impl AsRef<str>,
        path: impl AsRef<Path>,
    ) -> WasmEdgeResult<()> {
        let path = utils::path_to_cstring(path.as_ref())?;
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromFile(
                self.ctx,
                mod_name.as_raw(),
                path.as_ptr(),
            ))?
        };

        Ok(())
    }

    /// Consumes a given WasmEdge [ImportObject](crate::ImportObject) module to register and instantiate a WASM module
    /// into the [store](crate::Store) of the [`Vm`].
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, registers the exported instances in the [ImportObject](crate::ImportObject) module into the
    /// [store](crate::Store) of the [`Vm`], then
    ///
    /// - Instatiates the exported instances.
    ///
    ///
    /// # Argument
    ///
    /// - `import` specifies the [ImportObject](crate::ImportObject) module to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_wasm_from_import(&mut self, mut import: ImportObject) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromImport(
                self.ctx, import.ctx,
            ))?;
        }
        import.ctx = std::ptr::null_mut();

        Ok(())
    }

    /// Registers and instantiates a WASM module into the [store](crate::Store) of the [`Vm`] from a given WASM
    /// binary buffer.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads a WASM module from the given WASM binary buffer, then
    ///
    /// - Registers all exported instances in the WASM module into the [store](crate::Store) of the [`Vm`];
    ///
    /// - Finally, instantiates the exported instances.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the WASM module to be registered.
    ///
    /// - `buffer` specifies the buffer of a WASM binary.
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_wasm_from_buffer(
        &mut self,
        mod_name: impl AsRef<str>,
        buffer: &[u8],
    ) -> WasmEdgeResult<()> {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromBuffer(
                self.ctx,
                mod_name.as_raw(),
                buffer.as_ptr(),
                buffer.len() as u32,
            ))?;
        }

        Ok(())
    }

    /// Consumes a given WasmEdge AST [Module](crate::Module) to register and instantiate a WASM module into the
    /// [store](crate::Store) of the [`Vm`].
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads a WASM module from the given WasmEdge AST [Module](crate::Module), then
    ///
    /// - Registers all exported instances in the WASM module into the [store](crate::Store) of the [`Vm`];
    ///
    /// - Finally, instantiates the exported instances.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the WASM module to be registered.
    ///
    /// - `module` specifies the WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or
    /// [Compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_wasm_from_module(
        &mut self,
        mod_name: impl AsRef<str>,
        mut module: Module,
    ) -> WasmEdgeResult<()> {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromASTModule(
                self.ctx,
                mod_name.as_raw(),
                module.ctx,
            ))?;
        }
        module.ctx = std::ptr::null_mut();
        Ok(())
    }

    /// Runs a [function](crate::Function) defined in a WASM file.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads the WASM module from a given WASM file, and validates the loaded module; then,
    ///
    /// - Instantiates the WASM module; finally,
    ///
    /// - Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// - `path` specifies the file path to a WASM file.
    ///
    /// - `func_name` specifies the name of the [function](crate::Function).
    ///
    /// - `params` specifies the the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_file(
        &mut self,
        path: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        // load
        self.load_wasm_from_file(path)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Instantiate a WASM module from a buffer and invokes a function by name.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads and instantiates the WASM module from a buffer, then
    ///
    /// - Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// - `buffer` specifies the buffer of a WASM binary.
    ///
    /// - `func_name` specifies the name of the [function](crate::Function).
    ///
    /// - `params` specifies the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_buffer(
        &mut self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        // load
        self.load_wasm_from_buffer(buffer)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Instantiates a WASM module from a WasmEdge AST [Module](crate::Module) and invokes a function by name.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads and instantiates the WASM module from a WasmEdge AST [Module](crate::Module), then
    ///
    /// - Invokes a function by name and parameters.
    ///
    /// # Arguments
    ///
    /// - `module` specifies the WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader)
    /// or [Compiler](crate::Compiler).
    ///
    /// - `func_name` specifies the name of the [function](crate::Function).
    ///
    /// - `params` specifies the parameter values which are used by the [function](crate::Function).
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub fn run_wasm_from_module(
        &mut self,
        module: Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        // load
        self.load_wasm_from_module(module)?;

        // validate
        self.validate()?;

        // instantiate
        self.instantiate()?;

        // invoke
        self.run_function(func_name, params)
    }

    /// Loads a WASM module from a WasmEdge AST [Module](crate::Module).
    ///
    /// This is the first step to invoke a WASM function step by step.
    ///
    /// # Argument
    ///
    /// - `module` specifies a WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or
    /// [Compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_module(&mut self, mut module: Module) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromASTModule(
                self.ctx, module.ctx,
            ))?;
        }
        module.ctx = std::ptr::null_mut();
        Ok(())
    }

    /// Loads a WASM module from a WASM binary buffer.
    ///
    /// This is the first step to invoke a WASM function step by step.
    ///
    /// # Argument
    ///
    /// - `buffer` specifies the buffer of a WASM binary.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_buffer(&mut self, buffer: &[u8]) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromBuffer(
                self.ctx,
                buffer.as_ptr() as *const _,
                buffer.len() as u32,
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
    /// - `path` specifies the path to a WASM file.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_file(&mut self, path: impl AsRef<Path>) -> WasmEdgeResult<()> {
        let path = utils::path_to_cstring(path.as_ref())?;
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromFile(
                self.ctx,
                path.as_ptr(),
            ))?;
        }
        Ok(())
    }

    /// Validates a WASM module loaded into the [`Vm`].
    ///
    /// This is the second step to invoke a WASM function step by step. After loading a WASM module into the [`Vm`],
    /// call this function to validate it. Note that only validated WASM modules can be instantiated in the [`Vm`].
    ///
    /// # Error
    ///
    /// If fail to validate, then an error is returned.
    pub fn validate(&self) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(())
    }

    /// Instantiates a validated WASM module in the [`Vm`].
    ///
    /// This is the third step to invoke a WASM function step by step. After validating a WASM module in the [`Vm`],
    /// call this function to instantiate it; then, call `execute` to invoke the exported function in this WASM module.
    ///
    /// # Error
    ///
    /// If fail to instantiate, then an error is returned.
    pub fn instantiate(&mut self) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(())
    }

    /// Runs an exported WASM function by name. The WASM function is hosted by the anonymous [module](crate::Module) in
    /// the [store](crate::Store) of the [`Vm`].
    ///
    /// This is the final step to invoke a WASM function step by step.
    /// After instantiating a WASM module in the [`Vm`], the WASM module is registered into the [store](crate::Store)
    /// of the [`Vm`] as an anonymous module. Then repeatedly call this function to invoke the exported WASM functions
    /// by their names until the [`Vm`] is reset or a new WASM module is registered
    /// or loaded.
    ///
    /// # Arguments
    ///
    /// - `func_name` specifies the name of the exported WASM function to run.
    ///
    /// - `params` specifies the parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_function(
        &self,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        // prepare parameters
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // prepare returns
        let func_type = self.get_function_type(func_name.as_ref())?;

        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type.ctx) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_VMExecute(
                self.ctx,
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

    /// Runs an exported WASM function by its name and the module's name in which the WASM function is hosted.
    ///
    /// After registering a WASM module in the [`Vm`], repeatedly call this function to run exported WASM functions by
    /// their function names and the module names until the [`Vm`] is reset.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the WASM module registered into the [store](crate::Store) of the [`Vm`].
    ///
    /// - `func_name` specifies the name of the exported WASM function to run.
    ///
    /// - `params` specifies the parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_registered_function(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        // prepare parameters
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // prepare returns
        let func_type = self.get_registered_function_type(mod_name.as_ref(), func_name.as_ref())?;

        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type.ctx) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_VMExecuteRegistered(
                self.ctx,
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

    /// Returns the function type of a WASM function by its name. The function is hosted in the anonymous
    /// [module](crate::Module) of the [`Vm`].
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the name of the target WASM function.
    ///
    /// # Error
    ///
    /// If fail to get the function type, then an error is returned.
    pub fn get_function_type(&self, func_name: impl AsRef<str>) -> WasmEdgeResult<FuncType> {
        // check if func_name is one of the names of the functions in the store
        self.contains_func_name(func_name.as_ref())?;

        let ty_ctx = unsafe {
            let func_name: WasmEdgeString = func_name.as_ref().into();
            wasmedge::WasmEdge_VMGetFunctionType(self.ctx, func_name.as_raw())
        };

        match ty_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::NotFoundFuncType(
                func_name.as_ref().to_string(),
            ))),
            false => Ok(FuncType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Returns the function type of a WASM function by its name and the name of the registered [module](crate::Module)
    /// which hosts the WASM function.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the registered [module](crate::Module).
    ///
    /// - `func_name` specifies the name of the target WASM function.
    pub fn get_registered_function_type(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<FuncType> {
        // check if func_name is one of the names of the registered functions in the store
        self.contains_reg_func_name(mod_name.as_ref(), func_name.as_ref())?;

        let ty_ctx = unsafe {
            let mod_name: WasmEdgeString = mod_name.as_ref().into();
            let func_name: WasmEdgeString = func_name.as_ref().into();
            wasmedge::WasmEdge_VMGetFunctionTypeRegistered(
                self.ctx,
                mod_name.as_raw(),
                func_name.as_raw(),
            )
        };

        match ty_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::NotFoundFuncType(
                func_name.as_ref().to_string(),
            ))),
            false => Ok(FuncType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Resets the [`Vm`].
    pub fn reset(&mut self) {
        unsafe { wasmedge::WasmEdge_VMCleanup(self.ctx) }
    }

    /// Returns the length of the exported function list.
    pub fn function_list_len(&self) -> usize {
        unsafe { wasmedge::WasmEdge_VMGetFunctionListLength(self.ctx) as usize }
    }

    /// Returns an iterator of the exported functions.
    pub fn function_iter(&self) -> impl Iterator<Item = (Option<String>, Option<FuncType>)> {
        let len = self.function_list_len();
        let mut names = Vec::with_capacity(len);
        let mut types = Vec::with_capacity(len);
        unsafe {
            wasmedge::WasmEdge_VMGetFunctionList(
                self.ctx,
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
                    ctx: ty as *mut _,
                    registered: true,
                }),
            };
            (Some(name), func_ty)
        })
    }

    /// Returns the mutable Wasi [ImportObject](crate::ImportObject)
    pub fn wasi_import_module_mut(&mut self) -> WasmEdgeResult<ImportObject> {
        let io_ctx = unsafe {
            wasmedge::WasmEdge_VMGetImportModuleContext(self.ctx, WasmEdge_HostRegistration_Wasi)
        };
        match io_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::NotFoundWasiImportObjectModule)),
            false => Ok(ImportObject {
                ctx: io_ctx,
                registered: true,
            }),
        }
    }

    /// Returns the mutable WasmEdgeProcess [ImportObject](crate::ImportObject).
    pub fn wasmedge_process_import_module_mut(&mut self) -> WasmEdgeResult<ImportObject> {
        let io_ctx = unsafe {
            wasmedge::WasmEdge_VMGetImportModuleContext(
                self.ctx,
                WasmEdge_HostRegistration_WasmEdge_Process,
            )
        };
        match io_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(
                VmError::NotFoundWasmEdgeProcessImportObjectModule,
            )),
            false => Ok(ImportObject {
                ctx: io_ctx,
                registered: true,
            }),
        }
    }

    /// Returns the mutable [Store](crate::Store) from the [`Vm`].
    pub fn store_mut(&self) -> WasmEdgeResult<Store> {
        let store_ctx = unsafe { wasmedge::WasmEdge_VMGetStoreContext(self.ctx) };
        match store_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::NotFoundStore)),
            false => Ok(Store {
                ctx: store_ctx,
                registered: true,
            }),
        }
    }

    /// Returns the mutable [Statistics](crate::Statistics) from the [`Vm`].
    pub fn statistics_mut(&self) -> WasmEdgeResult<Statistics> {
        let stat_ctx = unsafe { wasmedge::WasmEdge_VMGetStatisticsContext(self.ctx) };
        match stat_ctx.is_null() {
            true => Err(WasmEdgeError::Vm(VmError::NotFoundStatistics)),
            false => Ok(Statistics {
                ctx: stat_ctx,
                registered: true,
            }),
        }
    }

    /// Checks if the [store](crate::Store) of the [`Vm`] contains a function of which the name matches the given
    /// `func_name`.
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the function's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [store](crate::Store), then an error is returned.
    pub fn contains_func_name(&self, func_name: impl AsRef<str>) -> WasmEdgeResult<()> {
        let store = self.store_mut()?;
        store.contains_func(func_name.as_ref())
    }

    /// Checks if the [store](crate::Store) of the [`Vm`] contains a registered function of which the name matches the
    /// given `func_name`.
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the registered function's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [store](crate::Store), then an error is returned.
    pub fn contains_reg_func_name(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<()> {
        let store = self.store_mut()?;
        store.contains_reg_func(mod_name.as_ref(), func_name.as_ref())
    }

    /// Checks if the [`Vm`] contains a registered module of which the name matches the given
    /// `mod_name`.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the registered module's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [store](crate::Store), then an error is returned.
    pub fn contains_mod_name(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<()> {
        let store = self.store_mut()?;
        store.contains_mod_name(mod_name.as_ref())
    }
}
impl Drop for Vm {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Vm;
    use crate::{
        error::{
            CoreCommonError, CoreError, CoreExecutionError, CoreLoadError, StoreError, VmError,
        },
        Config, FuncType, Function, ImportObject, Loader, Module, Store, ValType, Value,
        WasmEdgeError,
    };

    #[test]
    fn test_vm_create() {
        {
            // create a Vm context without Config and Store
            let result = Vm::create(None, None);
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.ctx.is_null());
        }

        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let config = config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Store context
            let result = Store::create();
            assert!(result.is_ok(), "Failed to create Store instance");
            let store = result.unwrap();

            // create a Vm context with the given Config and Store
            let result = Vm::create(Some(config), Some(store));
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.ctx.is_null());
        }

        {
            // create a Config context
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let config = config.bulk_memory_operations(true);
            assert!(config.bulk_memory_operations_enabled());

            // create a Vm context with the given Config
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.ctx.is_null());
        }

        {
            // create a Store context
            let result = Store::create();
            assert!(result.is_ok(), "Failed to create Store instance");
            let store = result.unwrap();

            // create a Vm context with the given Store
            let result = Vm::create(None, Some(store));
            assert!(result.is_ok());
            let vm = result.unwrap();
            assert!(!vm.ctx.is_null());
        }
    }

    #[test]
    fn test_vm_load_wasm_from_file() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let config = conf.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // load wasm module from a specified file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.load_wasm_from_file(path);
        assert!(result.is_ok());

        // load a wasm module from a non-existent file
        let result = vm.load_wasm_from_file("no_file");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
        );
    }

    #[test]
    fn test_vm_load_wasm_from_buffer() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.bulk_memory_operations(true);
        assert!(conf.bulk_memory_operations_enabled());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(conf), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // load wasm module from buffer
        let wasm_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(wasm_path);
        assert!(result.is_ok());
        let buffer = result.unwrap();
        let result = vm.load_wasm_from_buffer(&buffer);
        assert!(result.is_ok());

        // load wasm module from an empty buffer
        let empty_buffer: Vec<u8> = vec![];
        let result = vm.load_wasm_from_buffer(&empty_buffer);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Load(CoreLoadError::UnexpectedEnd))
        );
    }

    #[test]
    fn test_vm_load_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // load wasm module from an ast module
        let result = vm.load_wasm_from_module(module);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_validate() {
        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        let result = vm.validate();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Common(CoreCommonError::WrongVMWorkflow))
        );

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // load a wasm module from a ast module
        let result = vm.load_wasm_from_module(module);
        assert!(result.is_ok());

        let result = vm.validate();
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_instantiate() {
        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        let result = vm.instantiate();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Common(CoreCommonError::WrongVMWorkflow))
        );

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        let result = vm.load_wasm_from_module(module);
        assert!(result.is_ok());

        // call instantiate before validate
        let result = vm.instantiate();
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Common(CoreCommonError::WrongVMWorkflow))
        );

        // call validate, then instantiate
        let result = vm.validate();
        assert!(result.is_ok());
        let result = vm.instantiate();
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_invoke_wasm_function_step_by_step() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
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
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // load wasm module from a ast module instance
        let result = vm.load_wasm_from_module(ast_module);
        assert!(result.is_ok());

        // validate vm instance
        let result = vm.validate();
        assert!(result.is_ok());

        // instantiate
        let result = vm.instantiate();
        assert!(result.is_ok());

        // run function
        let result = vm.run_function("fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let values = result.unwrap();
        assert_eq!(values[0].to_i32(), 8);

        // run function with empty parameter
        let result = vm.run_function("fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a function with the parameters of wrong type
        let result = vm.run_function("fib", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a function: the specified function name is non-existant
        let result = vm.run_function("fib2", [Value::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Store(StoreError::NotFoundFunc("fib2".into()))
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
    fn test_vm_register_wasm_from_file() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // register a wasm module from a non-existed file
        let result = vm.register_wasm_from_file("reg-wasm-file", "no_file");
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
        );

        // register a wasm module from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.register_wasm_from_file("reg-wasm-file", path);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_register_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // create a loader
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap().wasi(true);
        assert!(config.wasi_enabled());
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();

        // load a AST module from a file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();

        // register a wasm module from an AST module
        let mod_name = "reg-wasm-ast";
        let result = vm.register_wasm_from_module(mod_name, module);
        assert!(result.is_ok());

        // run a registered function
        let result = vm.run_registered_function(mod_name, "fib", [Value::from_i32(5)]);
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
            WasmEdgeError::Store(StoreError::NotFoundModule("non-existent-module".into()))
        );

        // run a registered function with empty parameters
        let empty_params: Vec<Value> = vec![];
        let result = vm.run_registered_function(mod_name, "fib", empty_params);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a registered function with the parameters of wrong type
        let result = vm.run_registered_function(mod_name, "fib", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a registered function but give a wrong function name.
        let result = vm.run_registered_function(mod_name, "fib2", [Value::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Store(StoreError::NotFoundFuncRegistered {
                func_name: "fib2".into(),
                mod_name: "reg-wasm-ast".into(),
            })
        );
    }

    #[test]
    fn test_vm_register_wasm_from_importobj() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        let config = config.wasi(true);
        assert!(config.wasi_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // create ImportObject module
        let result = ImportObject::create("extern");
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import_obj.add_func("add", host_func);

        // register the import_obj module
        let result = vm.register_wasm_from_import(import_obj);
        assert!(result.is_ok());

        vm.reset();

        // get ImportObj module
        let result = vm.wasi_import_module_mut();
        assert!(result.is_ok());
        let result = vm.wasmedge_process_import_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessImportObjectModule)
        );

        // get store
        let result = vm.store_mut();
        assert!(result.is_ok());

        // get statistics
        let result = vm.statistics_mut();
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_register_wasm_from_buffer() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // register a wasm module from a buffer
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(path);
        assert!(result.is_ok());
        let buffer = result.unwrap();
        let result = vm.register_wasm_from_buffer("reg-wasm-buffer", &buffer);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_run_wasm_from_file() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.run_wasm_from_file(&path, "fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from a non-existent file
        let result = vm.run_wasm_from_file("no_file", "fib", [Value::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Load(CoreLoadError::IllegalPath))
        );

        // run a function from a WASM file with the empty parameters
        let result = vm.run_wasm_from_file(&path, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a function from a WASM file with the parameters of wrong type
        let result = vm.run_wasm_from_file(&path, "fib", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // fun a function: the specified function name is non-existant
        let result = vm.run_wasm_from_file(&path, "fib2", [Value::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Store(StoreError::NotFoundFunc("fib2".into()))
        );
    }

    #[test]
    fn test_vm_run_wasm_from_buffer() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a WASM buffer
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = std::fs::read(path);
        assert!(result.is_ok());
        let buffer = result.unwrap();
        let result = vm.run_wasm_from_buffer(&buffer, "fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function from an empty buffer
        let empty_buffer: Vec<u8> = Vec::new();
        let result = vm.run_wasm_from_buffer(&empty_buffer, "fib", [Value::from_i32(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Load(CoreLoadError::UnexpectedEnd))
        );

        // run a function with the empty parameters
        let result = vm.run_wasm_from_buffer(&buffer, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a function with the parameters of wrong type
        let result = vm.run_wasm_from_buffer(&buffer, "fib", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // fun a function: the specified function name is non-existant
        let result = vm.run_wasm_from_buffer(&buffer, "fib2", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Store(StoreError::NotFoundFunc("fib2".into()))
        );
    }

    #[test]
    fn test_vm_run_wasm_from_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let config = config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // run a function from a module
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", [Value::from_i32(5)]);
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 8);

        // run a function with the empty parameters
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", []);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // run a function with the parameters of wrong type
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::FuncTypeMismatch))
        );

        // fun a function: the specified function name is non-existant
        let module = load_fib_module();
        let result = vm.run_wasm_from_module(module, "fib2", [Value::from_i64(5)]);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Store(StoreError::NotFoundFunc("fib2".into()))
        );
    }

    fn load_fib_module() -> Module {
        // load a module
        let result = Loader::create(None);
        assert!(result.is_ok());
        let loader = result.unwrap();
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = loader.from_file(path);
        assert!(result.is_ok());
        result.unwrap()
    }

    fn real_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
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

        Ok(vec![Value::from_i32(c)])
    }
}
