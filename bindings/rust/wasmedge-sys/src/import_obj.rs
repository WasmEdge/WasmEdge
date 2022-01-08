//! Defines WasmEdge ImportObj struct.

use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    types::WasmEdgeString,
    utils::string_to_c_char,
    WasmEdgeError, WasmEdgeResult,
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
        let mod_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(mod_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
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
        T: IntoIterator<Item = E>,
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
            true => Err(WasmEdgeError::ImportObjCreate),
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
    pub fn create_wasmedge_process(cmds: Option<&[String]>, allow: bool) -> WasmEdgeResult<Self> {
        let (cmds_len, cmds) = match cmds {
            Some(cmds) => {
                let cmds = cmds.iter().map(string_to_c_char).collect::<Vec<_>>();
                (cmds.len() as u32, cmds.as_ptr())
            }
            None => (0, std::ptr::null()),
        };

        let ctx =
            unsafe { wasmedge::WasmEdge_ImportObjectCreateWasmEdgeProcess(cmds, cmds_len, allow) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
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
    pub fn init_wasmedge_process(&mut self, cmds: Option<&[String]>, allow: bool) {
        let (cmds_len, cmds) = match cmds {
            Some(cmds) => {
                let cmds = cmds.iter().map(string_to_c_char).collect::<Vec<_>>();
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
        let func_name: WasmEdgeString = name.into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, func_name.as_raw(), (*func).ctx);
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
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(self.ctx, table_name.as_raw(), table.ctx);
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
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(self.ctx, mem_name.as_raw(), memory.ctx);
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
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(self.ctx, global_name.as_raw(), global.ctx);
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{GlobalType, MemType, Mutability, RefType, TableType, ValType, Value};

    #[test]
    fn test_importobj() {
        let host_name = "extern";

        // create an ImportObj module
        let result = ImportObj::create(host_name);
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // create a Table instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let mut table_ty = result.unwrap();
        let result = Table::create(&mut table_ty);
        assert!(result.is_ok());
        let mut host_table = result.unwrap();
        // add the table into the import_obj module
        import_obj.add_table("table", &mut host_table);

        // create a Memory instance
        let result = MemType::create(1..=2);
        assert!(result.is_ok());
        let mut mem_ty = result.unwrap();
        let result = Memory::create(&mut mem_ty);
        assert!(result.is_ok());
        let mut host_memory = result.unwrap();
        // add the memory into the import_obj module
        import_obj.add_memory("memory", &mut host_memory);

        // create a Global instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let mut global_ty = result.unwrap();
        let result = Global::create(&mut global_ty, Value::I32(666));
        assert!(result.is_ok());
        let mut host_global = result.unwrap();
        // add the global into import_obj module
        import_obj.add_global("global_i32", &mut host_global);

        // TODO: create WASI
        // let args = vec!["arg1", "arg2"];
        // let envs = vec!["ENV1=VAL1", "ENV1=VAL2", "ENV3=VAL3"];
        // let preopens = vec![
        //     "apiTestData",
        //     "Makefile",
        //     "CMakeFiles",
        //     "ssvmAPICoreTests",
        //     ".:.",
        // ];
        // let result = ImportObj::create_wasi(Some(args), Some(envs), Some(preopens));
        // assert!(result.is_ok());

        // TODO: initialize WASI in VM

        // create wasmedge_process
        println!("*** create wasmedge_process");
        {
            let args = vec!["arg1".into(), "arg2".into()];
            let result = ImportObj::create_wasmedge_process(Some(&args), true);
            assert!(result.is_ok());
        }
        {
            let result = ImportObj::create_wasmedge_process(None, false);
            assert!(result.is_ok());
        }

        // TODO: invalid memory reference error
        // {
        //     let args = vec!["arg1".into(), "arg2".into()];
        //     let result = ImportObj::create_wasmedge_process(Some(&args), false);
        //     assert!(result.is_ok());
        // }

        // initialize wasmedge_process in VM
        // println!("*** initialize wasmedge_process in VM");
        // let result = Config::create();
        // assert!(result.is_ok());
        // let config = result.unwrap();
        // let config = config.enable_wasmedge_process(true);
        // let result = Vm::create(Some(&config), None);
        // assert!(result.is_ok());
        // let mut vm = result.unwrap();
        // let result = vm.import_obj_mut(HostRegistration::WasmEdgeProcess);
        // assert!(result.is_ok());
        // let mut import_obj = result.unwrap();
        // TODO: invalid memory reference
        // let args = vec!["arg1".into(), "arg2".into()];
        // import_obj.init_wasmedge_process(Some(&args), false);
    }
}
