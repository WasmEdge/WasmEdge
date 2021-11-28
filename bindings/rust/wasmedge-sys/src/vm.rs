use super::wasmedge;
use crate::{
    raw_result::{check, WasmEdgeResult},
    string::StringRef,
    utils, wasi, Config, ImportObj, Module, Statistics, Store, Value,
};
use std::os::raw::c_char;
use std::path::Path;

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

    pub fn instantiate(self) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(self)
    }

    pub fn init_wasi_obj(
        &self,
        args: Option<&Vec<&str>>,
        envs: Option<&Vec<&str>>,
        dirs: Option<&Vec<&str>>,
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

        if let Some(dirs) = dirs {
            import_obj.dirs = dirs
                .iter()
                .map(|dir| {
                    let raw_dir: wasmedge::WasmEdge_String = StringRef::from(*dir).into();
                    raw_dir.Buf
                })
                .collect::<Vec<*const c_char>>();
            import_obj.dirs_len = dirs.len() as u32;
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
                import_obj.dirs.as_ptr(),
                import_obj.dirs_len,
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

        assert!(
            !func_type.is_null(),
            "WasmEdge Vm failed to get function type!"
        );

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
}

impl Drop for Vm {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
        }
        if self.import_objects.len() > 0 {
            self.import_objects.drain(..);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Vm;
    use crate::{Config, Module, Store};

    #[test]
    fn test_vm_create() {
        // create Config instance
        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_some(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_some());
    }

    #[test]
    fn test_vm_load_wasm_from_file() {
        // create Config instance
        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_some(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_some());
        let vm = result.unwrap();

        // load wasm module from a specified file
        let path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
            .join("test/api/apiTestData/test.wasm");
        let result = vm.load_wasm_from_file(path);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_load_wasm_from_buffer() {
        // create Config instance
        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        // create Store instance
        let result = Store::create();
        assert!(result.is_some(), "Failed to create Store instance");
        let store = result.unwrap();

        // create Vm instance
        let result = Vm::create(Some(&conf), Some(&store));
        assert!(result.is_some());
        let vm = result.unwrap();

        // load wasm module from buffer
        let wasm_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
            .join("test/api/apiTestData/test.wasm");
        let result = std::fs::read(wasm_path);
        assert!(result.is_ok());
        let buf = result.unwrap();
        let result = vm.load_wasm_from_buffer(&buf);
        assert!(result.is_ok());
    }

    #[test]
    fn test_vm_load_wasm_from_ast_module() {
        // create ast module instance
        let path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
            .join("test/api/apiTestData/test.wasm");
        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::load_from_file(&conf, path);
        assert!(result.is_ok());
        let mut ast_module = result.unwrap();

        // create Vm instance
        let conf = Config::default().enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Store::create();
        assert!(result.is_some(), "Failed to create Store instance");
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
