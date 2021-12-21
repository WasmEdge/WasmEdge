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
    pub(crate) registered: bool,
}
impl ImportObj {
    pub fn create(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let raw_module_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create ImportObj instance",
            ))),
            false => Ok(ImportObj {
                ctx,
                registered: false,
            }),
        }
    }

    /// Create an ImportObj instance for the WASI specification.
    pub fn create_wasi<T, E>(
        args: Option<T>,
        envs: Option<T>,
        preopens: Option<T>,
    ) -> WasmEdgeResult<Self>
    where
        T: Iterator<Item = E>,
        E: AsRef<str>,
    {
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
            false => Ok(ImportObj {
                ctx,
                registered: false,
            }),
        }
    }

    /// Initialize the ImportObj instance for the WASI specification.
    pub fn init_wasi<T, E>(&mut self, args: Option<T>, envs: Option<T>, preopens: Option<T>)
    where
        T: Iterator<Item = E>,
        E: AsRef<str>,
    {
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

    /// Get the WASI exit code.
    pub fn exit_code(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ImportObjectWASIGetExitCode(self.ctx) }
    }

    /// Create an ImportObj instance for the wasmedge_process specification.
    pub fn create_wasmedge_process<T, E>(cmds: Option<T>, allow: bool) -> WasmEdgeResult<Self>
    where
        T: Iterator<Item = E>,
        E: AsRef<str>,
    {
        let (cmds_len, cmds) = match cmds {
            Some(cmds) => {
                let cmds = cmds.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (cmds.len() as u32, cmds.as_ptr())
            }
            None => (0, std::ptr::null()),
        };

        let ctx =
            unsafe { wasmedge::WasmEdge_ImportObjectCreateWasmEdgeProcess(cmds, cmds_len, allow) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to an ImportObj instance for the wasmedge_process specification",
            ))),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Initialize an ImportObj instance for the wasmedge_process specification.
    pub fn init_wasmedge_process<T, E>(&mut self, cmds: Option<T>, allow: bool)
    where
        T: Iterator<Item = E>,
        E: AsRef<str>,
    {
        let (cmds_len, cmds) = match cmds {
            Some(cmds) => {
                let cmds = cmds.into_iter().map(string_to_c_char).collect::<Vec<_>>();
                (cmds.len() as u32, cmds.as_ptr())
            }
            None => (0, std::ptr::null()),
        };

        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWasmEdgeProcess(self.ctx, cmds, cmds_len, allow)
        }
    }

    /// Add a Function instance into a ImportObj instance.
    pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registered = true;
        func.ctx = std::ptr::null_mut();
    }

    /// Add a Table instance into a ImportObj instance.
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

    /// Add a Memory instance into a ImportObj instance.
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

    /// Add a Global instance into a ImportObj instance.
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
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
        }
    }
}
