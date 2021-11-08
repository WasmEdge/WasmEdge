use super::wasmedge;
use crate::{
    import_obj::ImportObj,
    raw_result::{decode_result, ErrReport},
    string::StringRef,
    value::Value,
    wasi,
};
use std::os::raw::c_char;

// If there is a third-party sdk based on wasmedge-sys
// the private encapsulation  here can force the third-party sdk to use the api
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
}

impl Vm {
    pub fn create(config: &crate::config::Config) -> Self {
        let ctx = unsafe {
            wasmedge::WasmEdge_VMCreate(config.ctx, std::ptr::null_mut() /* store */)
        };
        assert!(!ctx.is_null(), "WasmEdge VM create failed");
        Self { ctx }
    }

    pub fn load_wasm_from_ast_module(
        self,
        module: &crate::module::Module,
    ) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMLoadWasmFromASTModule(
                self.ctx, module.ctx,
            ))?;
        }
        Ok(self)
    }

    pub fn validate(self) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(self)
    }

    pub fn instantiate(self) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
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

    pub fn register_module_from_import(self, import_obj: ImportObj) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMRegisterModuleFromImport(
                self.ctx,
                import_obj.ctx,
            ))?;
        }
        Ok(self)
    }

    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<Vec<Value>, ErrReport> {
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
            decode_result(wasmedge::WasmEdge_VMExecute(
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
}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
    }
}
