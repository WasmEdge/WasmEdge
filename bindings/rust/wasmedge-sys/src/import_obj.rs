use std::{ffi::CString, os::raw::c_char};

use super::wasmedge;
use crate::{instance::Function, string::StringRef};

#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
}

impl ImportObj {
    pub fn create(module_name: impl AsRef<str>) -> Self {
        let raw_module_name: wasmedge::WasmEdge_String =
            StringRef::from(module_name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        ImportObj { ctx }
    }

    pub fn add_func(&mut self, func_name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registed = true;
    }

    pub fn create_wasi(
        args: Vec<impl AsRef<str>>,
        envs: Vec<impl AsRef<str>>,
        dirs: Vec<impl AsRef<str>>,
        preopens: Vec<impl AsRef<str>>,
    ) {
        let cstr_args: Vec<_> = args
            .iter()
            .map(|arg| CString::new(arg.as_ref()).unwrap())
            .collect();

        let cstr_envs: Vec<_> = envs
            .iter()
            .map(|env| CString::new(env.as_ref()).unwrap())
            .collect();

        let cstr_dirs: Vec<_> = dirs
            .iter()
            .map(|dir| CString::new(dir.as_ref()).unwrap())
            .collect();

        let cstr_preopens: Vec<_> = preopens
            .iter()
            .map(|preopen| CString::new(preopen.as_ref()).unwrap())
            .collect();

        unsafe {
            wasmedge::WasmEdge_ImportObjectCreateWASI(
                cstr_args.as_ptr() as *const *const c_char,
                args.len() as u32,
                cstr_envs.as_ptr() as *const *const c_char,
                envs.len() as u32,
                cstr_dirs.as_ptr() as *const *const c_char,
                dirs.len() as u32,
                cstr_preopens.as_ptr() as *const *const c_char,
                preopens.len() as u32,
            );
        }
    }
}

impl Drop for ImportObj {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
    }
}
