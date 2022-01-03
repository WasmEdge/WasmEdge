//! Defines WasmEdge Vm struct.

use super::wasmedge;
use crate::{
    error::{check, Error, WasmEdgeResult},
    instance::function::FuncType,
    types::HostRegistration,
    utils, Config, ImportObj, Module, Statistics, Store, Value,
};
use std::path::Path;

/// Struct of WasmEdge Vm.
///
/// A [`Vm`] defines a virtual environment for managing WebAssembly programs.
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    import_objects: Vec<ImportObj>,
}
impl Vm {
    /// Creates a new [`Vm`] to be associated with the given [configuration](crate::Config) and [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// - `config` specifies a configuration for the new [`Vm`].
    ///
    /// - `store` specifies an external WASM [store](crate::Store) used by the new [`Vm`]. The instantiation and
    /// execution of the new [`Vm`] will refer to this store context. If no store context is specified when creating
    /// a [`Vm`], then the [`Vm`] itself will allocate and own a [`store`](crate::Store).
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create(config: Option<&Config>, store: Option<&Store>) -> WasmEdgeResult<Self> {
        let conf = match config {
            Some(conf) => conf.ctx,
            None => std::ptr::null(),
        };
        let store = match store {
            Some(store) => store.ctx,
            None => std::ptr::null_mut(),
        };
        let vm = unsafe { wasmedge::WasmEdge_VMCreate(conf, store) };
        match vm.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Vm instance",
            ))),
            false => Ok(Self {
                ctx: vm,
                import_objects: vec![],
            }),
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
        self,
        mod_name: impl AsRef<str>,
        path: impl AsRef<Path>,
    ) -> WasmEdgeResult<Self> {
        let path = utils::path_to_cstring(path.as_ref())?;
        let raw_mod_name = mod_name.into();
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromFile(
                self.ctx,
                raw_mod_name,
                path.as_ptr(),
            ))?
        };

        Ok(self)
    }

    /// Registers and instantiates a WASM module into the [store](crate::Store) of the [`Vm`] from a given WasmEdge
    /// [ImportObj](crate::ImportObj) module.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, registers the exported instances in the [ImportObj](crate::ImportObj) module into the
    /// [store](crate::Store) of the [`Vm`], then
    ///
    /// - Instatiates the exported instances.
    ///
    ///
    /// # Argument
    ///
    /// - `import_obj` specifies the [ImportObj](crate::ImportObj) module to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the WASM module, then an error is returned.
    pub fn register_wasm_from_import(self, import_obj: &mut ImportObj) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromImport(
                self.ctx,
                import_obj.ctx,
            ))?;
        }
        import_obj.ctx = std::ptr::null_mut();
        import_obj.registered = true;

        Ok(self)
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
        self,
        mod_name: impl AsRef<str>,
        buffer: &[u8],
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromBuffer(
                self.ctx,
                mod_name.into(),
                buffer.as_ptr(),
                buffer.len() as u32,
            ))?;
        }

        Ok(self)
    }

    /// Registers and instantiates a WASM module into the [store](crate::Store) of the [`Vm`] from a WasmEdge AST
    /// [Module](crate::Module).
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
        self,
        mod_name: impl AsRef<str>,
        module: &mut Module,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromASTModule(
                self.ctx,
                mod_name.into(),
                module.ctx,
            ))?;
        }
        module.ctx = std::ptr::null_mut();
        module.registered = true;

        Ok(self)
    }

    /// Instantiates a WASM module from a WASM file and invokes a [function](crate::Function) by name.
    ///
    /// The workflow of the function can be summarized as the following steps:
    ///
    /// - First, loads and instantiates the WASM module from a given file, then
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
        &self,
        path: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let path = utils::path_to_cstring(path.as_ref())?;
        let raw_func_name = func_name.into();

        // prepare parameters
        let raw_params = params
            .into_iter()
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // prepare returns
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }
        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(wasmedge::WasmEdge_VMRunWasmFromFile(
                self.ctx,
                path.as_ptr(),
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into))
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
        &self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name = func_name.into();

        // prepare parameters
        let raw_params = params
            .into_iter()
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // prepare returns
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }
        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(wasmedge::WasmEdge_VMRunWasmFromBuffer(
                self.ctx,
                buffer.as_ptr(),
                buffer.len() as u32,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into))
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
        &self,
        module: &mut Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name = func_name.into();

        // prepare parameters
        let raw_params = params
            .into_iter()
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // prepare returns
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }
        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(wasmedge::WasmEdge_VMRunWasmFromASTModule(
                self.ctx,
                module.ctx,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            module.ctx = std::ptr::null_mut();
            module.registered = true;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into))
    }

    /// Loads a WASM module from a WasmEdge AST [Module](crate::Module).
    ///
    /// # Argument
    ///
    /// - `module` specifies a WasmEdge AST [Module](crate::Module) generated by [Loader](crate::Loader) or
    /// [Compiler](crate::Compiler).
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_module(self, module: &mut Module) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromASTModule(
                self.ctx, module.ctx,
            ))?;
            module.ctx = std::ptr::null_mut();
            module.registered = true;
        }
        Ok(self)
    }

    /// Loads a WASM module from a WASM binary buffer.
    ///
    /// # Argument
    ///
    /// - `buffer` specifies the buffer of a WASM binary.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_buffer(self, buffer: &[u8]) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromBuffer(
                self.ctx,
                buffer.as_ptr() as *const _,
                buffer.len() as u32,
            ))?;
        }
        Ok(self)
    }

    /// Loads a WASM module from a WASM file.
    ///
    /// # Argument
    ///
    /// - `path` specifies the path to a WASM file.
    ///
    /// # Error
    ///
    /// If fail to load, then an error is returned.
    pub fn load_wasm_from_file<P: AsRef<Path>>(self, path: P) -> WasmEdgeResult<Self> {
        let path = utils::path_to_cstring(path.as_ref())?;
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromFile(
                self.ctx,
                path.as_ptr(),
            ))?;
        }
        Ok(self)
    }

    /// Validates a WASM module loaded into the [`Vm`].
    ///
    /// This is the second step to invoke a WASM function step by step. After loading a WASM module into the [`Vm`],
    /// call this function to validate it. Note that only validated WASM modules can be instantiated in the [`Vm`].
    ///
    /// # Error
    ///
    /// If fail to validate, then an error is returned.
    pub fn validate(self) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(self)
    }

    /// Instantiates a validated WASM module in the [`Vm`].
    ///
    /// This is the third step to invoke a WASM function step by step. After validating a WASM module in the [`Vm`],
    /// call this function to instantiate it; then, call `execute` to invoke the exported function in this WASM module.
    ///
    /// # Error
    ///
    /// If fail to instantiate, then an error is returned.
    pub fn instantiate(self) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(self)
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
        &mut self,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name = func_name.into();

        // prepare parameters
        let raw_params = params
            .into_iter()
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // prepare returns
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }
        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(wasmedge::WasmEdge_VMExecute(
                self.ctx,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into))
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
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_mod_name = mod_name.into();
        let raw_func_name = func_name.into();

        // prepare parameters
        let raw_params = params
            .into_iter()
            .map(wasmedge::WasmEdge_Value::from)
            .collect::<Vec<_>>();

        // prepare returns
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }
        // get the info of the funtion return
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(wasmedge::WasmEdge_VMExecuteRegistered(
                self.ctx,
                raw_mod_name,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into))
    }

    /// Returns the function type of a WASM function by its name. The function is hosted in the anonymous
    /// [module](crate::Module) of the [`Vm`].
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the name of the target WASM function.
    pub fn get_function_type(&self, func_name: impl AsRef<str>) -> Option<FuncType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, func_name.into()) };
        match ty_ctx.is_null() {
            true => None,
            false => Some(FuncType {
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
    ) -> Option<FuncType> {
        let ty_ctx = unsafe {
            wasmedge::WasmEdge_VMGetFunctionTypeRegistered(
                self.ctx,
                mod_name.into(),
                func_name.into(),
            )
        };
        match ty_ctx.is_null() {
            true => None,
            false => Some(FuncType {
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
    pub fn function_iter(
        &self,
    ) -> WasmEdgeResult<impl Iterator<Item = (Option<String>, Option<FuncType>)>> {
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

        let returns = names.into_iter().zip(types.into_iter()).map(|(name, ty)| {
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
        });
        Ok(returns)
    }

    /// Returns the mutable [ImportObj](crate::ImportObj) corresponding to the HostRegistration settings.
    pub fn import_obj_mut(&mut self, reg: HostRegistration) -> WasmEdgeResult<ImportObj> {
        let io_ctx = unsafe { wasmedge::WasmEdge_VMGetImportModuleContext(self.ctx, reg.into()) };
        match io_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get ImportObj instance from the VM instance",
            ))),
            false => Ok(ImportObj {
                ctx: io_ctx,
                registered: true,
            }),
        }
    }

    /// Returns the mutable [Store](crate::Store) from the [`Vm`].
    pub fn store_mut(&self) -> WasmEdgeResult<Store> {
        let store_ctx = unsafe { wasmedge::WasmEdge_VMGetStoreContext(self.ctx) };
        match store_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get Store instance from the VM instance",
            ))),
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
            true => Err(Error::OperationError(String::from(
                "fail to get Statistics instance from the VM instance",
            ))),
            false => Ok(Statistics {
                ctx: stat_ctx,
                registered: true,
            }),
        }
    }

    /// Initializes the WASI host module with the given parameters.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the commandline arguments. The first argument is the program name.
    ///
    /// - `envs` specifies the environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// - `preopens` specifies the directories to pre-open. The required format is `DIR1:DIR2`.
    pub fn init_wasi_obj<T, E>(
        &mut self,
        args: Option<T>,
        envs: Option<T>,
        preopens: Option<T>,
    ) -> WasmEdgeResult<()>
    where
        T: Iterator<Item = E>,
        E: AsRef<str>,
    {
        let io_ctx = unsafe {
            wasmedge::WasmEdge_VMGetImportModuleContext(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        };
        if io_ctx.is_null() {
            return Err(Error::OperationError(String::from(
                "fail to get ImportObj instace from the Vm instance.",
            )));
        }

        let mut import_obj = ImportObj {
            ctx: io_ctx,
            registered: true,
        };
        import_obj.init_wasi(args, envs, preopens);

        Ok(())
    }
}
impl Drop for Vm {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
        }
        self.import_objects.drain(..);
    }
}

#[cfg(test)]
mod tests {
    use super::Vm;
    use crate::{Config, Module, Store};

    #[test]
    fn test_vm_create() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_load_wasm_from_file() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from a specified file
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = vm.load_wasm_from_file(path);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_load_wasm_from_buffer() {
        // create Config instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from buffer
        let wasm_path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = std::fs::read(wasm_path);
        assert!(result.is_ok());
        let buf = result.unwrap();
        let result = vm.load_wasm_from_buffer(&buf);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_load_wasm_from_ast_module() {
        // create ast module instance
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::create_from_file(&conf, path);
        assert!(result.is_ok());
        let mut ast_module = result.unwrap();

        // create Vm instance
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let store = result.unwrap();

        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_ok());
        let vm = result.unwrap();

        // load wasm module from a ast module instance
        let result = vm.load_wasm_from_module(&mut ast_module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // validate vm instance
        let result = vm.validate();
        assert!(result.is_ok());
    }
}
