use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    string::StringRef,
    utils::string_to_c_char,
    Error, WasmEdgeResult,
};
#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
}

impl ImportObj {
    pub fn create(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let raw_module_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create ImportObj instance",
            ))),
            false => Ok(ImportObj { ctx }),
        }
    }

    pub fn create_wasi<T: Iterator<Item = E>, E: AsRef<str>>(
        args: Option<T>,
        envs: Option<T>,
        preopens: Option<T>,
    ) -> WasmEdgeResult<Self> {
        let (args_len, args) = match args {
            Some(args) => {
                let args = args.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (args.len() as u32, args.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (envs_len, envs) = match envs {
            Some(envs) => {
                let envs = envs.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (envs.len() as u32, envs.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (preopens_len, preopens) = match preopens {
            Some(preopens) => {
                let preopens = preopens
                    .into_iter()
                    .map(string_to_c_char)
                    .collect::<Vec<_>>();
                (preopens.len() as u32, preopens.as_ptr())
            }
            None => (0, std::ptr::null()),
        };

        let ctx = unsafe {
            wasmedge::WasmEdge_ImportObjectCreateWASI(
                args,
                args_len,
                envs,
                envs_len,
                preopens,
                preopens_len,
            )
        };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create ImportObj for the WASI Specification",
            ))),
            false => Ok(ImportObj { ctx }),
        }
    }

    pub fn init_wasi<T: Iterator<Item = E>, E: AsRef<str>>(
        &self,
        args: Option<T>,
        envs: Option<T>,
        preopens: Option<T>,
    ) {
        let (args_len, args) = match args {
            Some(args) => {
                let args = args.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (args.len() as u32, args.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (envs_len, envs) = match envs {
            Some(envs) => {
                let envs = envs.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (envs.len() as u32, envs.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (preopens_len, preopens) = match preopens {
            Some(preopens) => {
                let preopens = preopens
                    .into_iter()
                    .map(string_to_c_char)
                    .collect::<Vec<_>>();
                (preopens.len() as u32, preopens.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWASI(
                self.ctx,
                args,
                args_len,
                envs,
                envs_len,
                preopens,
                preopens_len,
            )
        };
    }

    pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registered = true;
        func.ctx = std::ptr::null_mut();
    }

    pub fn add_table(&mut self, name: impl AsRef<str>, table: &mut Table) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                table.ctx,
            );
        }
        table.registered = true;
        table.ctx = std::ptr::null_mut();
    }

    pub fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                memory.ctx,
            );
        }
        memory.registered = true;
        memory.ctx = std::ptr::null_mut();
    }

    pub fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                global.ctx,
            );
        }
        global.registered = true;
        global.ctx = std::ptr::null_mut();
    }
}

impl Drop for ImportObj {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
    }
}
