use super::wasmedge;
use crate::{
    error::{check, Error, WasmEdgeResult},
    instance::function::FuncType,
    string::StringRef,
    types::HostRegistration,
    utils, Config, ImportObj, Module, Statistics, Store, Value,
};
use std::path::Path;

// If there is a third-party sdk based on wasmedge-sys
// the private encapsulation  here can force the third-party sdk to use the api
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    import_objects: Vec<ImportObj>,
}
impl Vm {
    /// Create a Vm instance
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

    /// Register and instantiate WASM into the store in VM from a WASM file.
    pub fn register_wasm_from_file(
        self,
        mod_name: impl AsRef<str>,
        path: impl AsRef<Path>,
    ) -> WasmEdgeResult<Self> {
        let path = utils::path_to_cstring(path.as_ref())?;
        let raw_mod_name: wasmedge::WasmEdge_String = StringRef::from(mod_name.as_ref()).into();
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromFile(
                self.ctx,
                raw_mod_name,
                path.as_ptr(),
            ))?
        };

        Ok(self)
    }

    /// Register and instantiate WASM into the store in VM from a WasmEdge ImportObj instance
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

    /// Register and instantiate WASM into the store in VM from a buffer.
    pub fn register_wasm_from_buffer(
        self,
        mod_name: impl AsRef<str>,
        buffer: &[u8],
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromBuffer(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                buffer.as_ptr(),
                buffer.len() as u32,
            ))?;
        }

        Ok(self)
    }

    /// Register and instantiate WASM into the store in VM from a WasmEdge Module instance
    pub fn register_wasm_from_module(
        self,
        mod_name: impl AsRef<str>,
        module: &mut Module,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromASTModule(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                module.ctx,
            ))?;
        }
        module.ctx = std::ptr::null_mut();
        module.registered = true;

        Ok(self)
    }

    /// Instantiate a WASM module from a WASM file and invoke a function by name.
    pub fn run_wasm_from_file(
        &self,
        path: impl AsRef<Path>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let path = utils::path_to_cstring(path.as_ref())?;
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

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

    /// Instantiate a WASM module from a buffer and invoke a function by name.
    pub fn run_wasm_from_buffer(
        &self,
        buffer: &[u8],
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

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

    /// Instantiate a WASM module from a WasmEdge AST Module and invoke a function by name.
    pub fn run_wasm_from_module(
        &self,
        module: &mut Module,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

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

    /// Load the WASM module from a loaded WasmEdge AST Module.
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

    /// Load the WASM module from a buffer.
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

    /// Load the WASM module from a WASM file.
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

    /// Validate the WASM module loaded into the VM instance.
    pub fn validate(self) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(self)
    }

    /// Instantiate the validated WASM module in the VM context.
    ///
    /// This is the third step to invoke a WASM function step by step.
    /// After validating a WASM module in the VM context, You can call this function
    /// to instantiate it. And you can then call `execute` for invoking
    /// the exported function in this WASM module.
    ///
    pub fn instantiate(self) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(self)
    }

    /// Invoke a function by name.
    pub fn run_function(
        &mut self,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

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

    /// Invoke a WASM function by its module name and function name.
    pub fn run_registered_function(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<impl Iterator<Item = Value>> {
        let raw_mod_name: wasmedge::WasmEdge_String = StringRef::from(mod_name.as_ref()).into();
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

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

    /// Get the function type by function name.
    pub fn get_function_type(&self, func_name: impl AsRef<str>) -> Option<FuncType> {
        let ty_ctx = unsafe {
            wasmedge::WasmEdge_VMGetFunctionType(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(func_name.as_ref())),
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

    /// Get the function type by function name.
    pub fn get_registered_function_type(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> Option<FuncType> {
        let ty_ctx = unsafe {
            wasmedge::WasmEdge_VMGetFunctionTypeRegistered(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                wasmedge::WasmEdge_String::from(StringRef::from(func_name.as_ref())),
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

    /// Reset of WasmEdge_VMContext.
    pub fn reset(&mut self) {
        unsafe { wasmedge::WasmEdge_VMCleanup(self.ctx) }
    }

    /// Get the length of exported function list.
    pub fn function_list_len(&self) -> usize {
        unsafe { wasmedge::WasmEdge_VMGetFunctionListLength(self.ctx) as usize }
    }

    /// Get the exported function list.
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

    /// Get the mutable ImportObj instance corresponding to the HostRegistration settings.
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

    /// Get the mutable Store instance from VM.
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

    /// Get the mutable Statistics instance from VM
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
