//! Defines WasmEdge ImportObj struct.

use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    utils::string_to_c_char,
    Error, WasmEdgeResult,
};

/// Struct of WasmEdge ImportObj.
///
/// A [`ImportObj`] represents a host module with a name. A host module consists of one or more
/// host functions which are defined outside WebAssembly and passed to WASM modules as imports.
#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
    pub(crate) registered: bool,
}
impl ImportObj {
    /// Creates a new host module with the given name.
    ///
    /// # Argument
    ///
    /// `name` specifies the name of the new host module.
    ///
    /// # Error
    ///
    /// If fail to create a host module, then an error is returned.
    pub fn create(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let raw_module_name = name.into();
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

    /// Creates a WASI host module which contains the WASI host functions, and initializes it with the given parameters.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the commandline arguments. The first argument is the program name.
    ///
    /// - `envs` specifies the environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// - `preopens` specifies the directories to pre-open. The required format is `DIR1:DIR2`.
    ///
    /// # Error
    ///
    /// If fail to create a host module, then an error is returned.
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

    /// Initializes the WASI host module with the given parameters.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the commandline arguments. The first argument is the program name.
    ///
    /// - `envs` specifies the environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// - `preopens` specifies the directories to pre-open. The required format is `DIR1:DIR2`.
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

    /// Returns the WASI exit code.
    ///
    /// The WASI exit code can be accessed after running the "_start" function of a `wasm32-wasi` program.
    pub fn exit_code(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ImportObjectWASIGetExitCode(self.ctx) }
    }

    /// Creates a wasmedge_process host module that contains the wasmedge_process host functions and
    /// initialize it with the parameters.
    ///
    /// # Arguments
    ///
    /// - `cmds` specifies a white list of commands.
    ///
    /// - `allow` determines if wasmedge_process is allowed to execute all commands on the white list.
    ///
    /// # Error
    ///
    /// If fail to create a wasmedge_process host module, then an error is returned.
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

    /// Initializes the wasmedge_process host module with the parameters.
    ///
    /// # Arguments
    ///
    /// - `cmds` specifies a white list of commands.
    ///
    /// - `allow` determines if wasmedge_process is allowed to execute all commands on the white list.
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

    /// Adds a [`Function`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the host function instance to add.
    pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name = name.into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registered = true;
        func.ctx = std::ptr::null_mut();
    }

    /// Adds a [`Table`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the export table instance to add.
    pub fn add_table(&mut self, name: impl AsRef<str>, table: &mut Table) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(self.ctx, name.into(), table.ctx);
        }
        table.registered = true;
        table.ctx = std::ptr::null_mut();
    }

    /// Adds a [`Memory`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the export memory instance to add.
    pub fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(self.ctx, name.into(), memory.ctx);
        }
        memory.registered = true;
        memory.ctx = std::ptr::null_mut();
    }

    /// Adds a [`Global`] into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the export global instance to add.
    pub fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(self.ctx, name.into(), global.ctx);
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
