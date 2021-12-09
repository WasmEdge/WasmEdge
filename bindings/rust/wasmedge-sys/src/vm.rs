use super::wasmedge;
use crate::{
    error::{check, Error, WasmEdgeResult},
    instance::function::FuncType,
    string::StringRef,
    utils, wasi, Config, ImportObj, Module, Statistics, Store, Value,
};
use std::{os::raw::c_char, path::Path};

// If there is a third-party sdk based on wasmedge-sys
// the private encapsulation  here can force the third-party sdk to use the api
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    import_objects: Vec<ImportObj>,
}

impl Vm {
    pub fn create(config: Option<&Config>, store: Option<&Store>) -> Option<Self> {
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
            true => None,
            false => Some(Self {
                ctx: vm,
                import_objects: vec![],
            }),
        }
    }

    pub fn load_wasm_from_ast_module(self, module: &mut Module) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMLoadWasmFromASTModule(
                self.ctx, module.ctx,
            ))?;
            module.ctx = std::ptr::null_mut();
            module.registered = true;
        }
        Ok(self)
    }

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

    /// Get the function type by function name.
    pub fn func_type(&self, func_name: impl AsRef<str>) -> Option<FuncType> {
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
    pub fn func_type_registered(
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
    pub fn func_list_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_VMGetFunctionListLength(self.ctx) }
    }

    pub fn init_wasi_obj(
        &self,
        args: Option<&Vec<&str>>,
        envs: Option<&Vec<&str>>,
        preopens: Option<&Vec<&str>>,
    ) {
        let import_mod_ctx = unsafe {
            wasmedge::WasmEdge_VMGetImportModuleContext(
                self.ctx,
                wasmedge::WasmEdge_HostRegistration_Wasi,
            )
        };

        let mut import_obj = wasi::ImportObjInitParams::new();

        if let Some(args) = args {
            import_obj.args = args
                .iter()
                .map(|arg| {
                    let raw_arg: wasmedge::WasmEdge_String = StringRef::from(*arg).into();
                    raw_arg.Buf
                })
                .collect::<Vec<*const c_char>>();
            import_obj.args_len = args.len() as u32;
        }

        if let Some(envs) = envs {
            import_obj.envs = envs
                .iter()
                .map(|env| {
                    let raw_env: wasmedge::WasmEdge_String = StringRef::from(*env).into();
                    raw_env.Buf
                })
                .collect::<Vec<*const c_char>>();
            import_obj.envs_len = envs.len() as u32;
        }

        if let Some(preopens) = preopens {
            import_obj.preopens = preopens
                .iter()
                .map(|preopen| {
                    let raw_preopen: wasmedge::WasmEdge_String = StringRef::from(*preopen).into();
                    raw_preopen.Buf
                })
                .collect::<Vec<*const c_char>>();
            import_obj.preopens_len = preopens.len() as u32;
        }

        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWASI(
                import_mod_ctx,
                import_obj.args.as_ptr(),
                import_obj.args_len,
                import_obj.envs.as_ptr(),
                import_obj.envs_len,
                import_obj.preopens.as_ptr(),
                import_obj.preopens_len,
            )
        }
    }

    pub fn register_module_from_import(mut self, import_obj: ImportObj) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMRegisterModuleFromImport(
                self.ctx,
                import_obj.ctx,
            ))?;
        }
        self.import_objects.push(import_obj);
        Ok(self)
    }

    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> WasmEdgeResult<Vec<Value>> {
        // construct func
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

        let raw_params: Vec<_> = params
            .as_ref()
            .iter()
            .copied()
            .map(wasmedge::WasmEdge_Value::from)
            .collect();

        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge Vm failed to get function type!",
            )));
        }

        // construct returns
        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        // execute
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
        Ok(returns.into_iter().map(Into::into).collect())
    }

    pub fn statistics(&self) -> Statistics {
        let ctx = unsafe { wasmedge::WasmEdge_VMGetStatisticsContext(self.ctx) };
        Statistics {
            ctx,
            registered: true,
        }
    }

    /// Get the store context used in the WasmEdge_VMContext.
    pub fn get_store(&self) -> Option<Store> {
        let store_ctx = unsafe { wasmedge::WasmEdge_VMGetStoreContext(self.ctx) };
        match store_ctx.is_null() {
            true => None,
            false => Some(Store {
                ctx: store_ctx,
                registered: true,
            }),
        }
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
        assert!(result.is_some());
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
        assert!(result.is_some());
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
        assert!(result.is_some());
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

        let result = Module::load_from_file(&conf, path);
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
        assert!(result.is_some());
        let vm = result.unwrap();

        // load wasm module from a ast module instance
        let result = vm.load_wasm_from_ast_module(&mut ast_module);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // validate vm instance
        let result = vm.validate();
        assert!(result.is_ok());
    }
}
