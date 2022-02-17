//! Defines WasmEdge ImportObj struct.

use crate::{
    instance::{Function, Global, Memory, Table},
    types::WasmEdgeString,
    utils::string_to_c_char,
    wasmedge, WasmEdgeError, WasmEdgeResult,
};

/// Struct of WasmEdge ImportObj.
///
/// A [`ImportObject`] represents a host module with a name. A host module consists of one or more
/// host functions which are defined outside WebAssembly and passed to WASM modules as imports.
#[derive(Debug)]
pub struct ImportObject {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
    pub(crate) registered: bool,
}
impl ImportObject {
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
            false => Ok(ImportObject {
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
    pub fn create_wasi(
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) -> WasmEdgeResult<Self> {
        let args = match args {
            Some(args) => args.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let args_len = args.len();

        let envs = match envs {
            Some(envs) => envs.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let envs_len = envs.len();

        let preopens = match preopens {
            Some(preopens) => preopens
                .into_iter()
                .map(string_to_c_char)
                .collect::<Vec<_>>(),
            None => vec![],
        };
        let preopens_len = preopens.len();

        let ctx = unsafe {
            wasmedge::WasmEdge_ImportObjectCreateWASI(
                args.as_ptr(),
                args_len as u32,
                envs.as_ptr(),
                envs_len as u32,
                preopens.as_ptr(),
                preopens_len as u32,
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
            false => Ok(ImportObject {
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
    pub fn init_wasi(
        &mut self,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) {
        let args = match args {
            Some(args) => args.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let args_len = args.len();

        let envs = match envs {
            Some(envs) => envs.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let envs_len = envs.len();

        let preopens = match preopens {
            Some(preopens) => preopens
                .into_iter()
                .map(string_to_c_char)
                .collect::<Vec<_>>(),
            None => vec![],
        };
        let preopens_len = preopens.len();

        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWASI(
                self.ctx,
                args.as_ptr(),
                args_len as u32,
                envs.as_ptr(),
                envs_len as u32,
                preopens.as_ptr(),
                preopens_len as u32,
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
    /// - `allowed_cmds` specifies a white list of commands.
    ///
    /// - `allowed` determines if wasmedge_process is allowed to execute all commands on the white list.
    ///
    /// # Error
    ///
    /// If fail to create a wasmedge_process host module, then an error is returned.
    pub fn create_wasmedge_process(
        allowed_cmds: Option<Vec<&str>>,
        allowed: bool,
    ) -> WasmEdgeResult<Self> {
        let cmds = match allowed_cmds {
            Some(cmds) => cmds.iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let cmds_len = cmds.len();

        let ctx = unsafe {
            wasmedge::WasmEdge_ImportObjectCreateWasmEdgeProcess(
                cmds.as_ptr(),
                cmds_len as u32,
                allowed,
            )
        };
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
    /// - `allowed_cmds` specifies a white list of commands.
    ///
    /// - `allowed` determines if wasmedge_process is allowed to execute all commands on the white list.
    pub fn init_wasmedge_process(&mut self, allowed_cmds: Option<Vec<&str>>, allowed: bool) {
        let cmds = match allowed_cmds {
            Some(cmds) => cmds.iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let cmds_len = cmds.len();

        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWasmEdgeProcess(
                self.ctx,
                cmds.as_ptr(),
                cmds_len as u32,
                allowed,
            )
        }
    }

    /// Adds a [`Function`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the host function instance to add.
    pub fn add_func(&mut self, name: impl AsRef<str>, mut func: Function) {
        let func_name: WasmEdgeString = name.into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, func_name.as_raw(), func.inner.0);
        }
        func.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [`Table`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the export table instance to add.
    pub fn add_table(&mut self, name: impl AsRef<str>, mut table: Table) {
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(self.ctx, table_name.as_raw(), table.inner.0);
        }
        table.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [`Memory`] into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the export memory instance to add.
    pub fn add_memory(&mut self, name: impl AsRef<str>, mut memory: Memory) {
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(self.ctx, mem_name.as_raw(), memory.inner.0);
        }
        memory.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [`Global`] into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the export global instance to add.
    pub fn add_global(&mut self, name: impl AsRef<str>, mut global: Global) {
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(
                self.ctx,
                global_name.as_raw(),
                global.inner.0,
            );
        }
        global.inner.0 = std::ptr::null_mut();
    }
}
impl Drop for ImportObject {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        Config, FuncType, GlobalType, MemType, Mutability, RefType, TableType, ValType, Value, Vm,
    };

    #[test]
    fn test_import_obj_add_instance() {
        let host_name = "extern";

        // create an ImportObj module
        let result = ImportObject::create(host_name);
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // add host function "func-add": (externref, i32) -> (i32)
        let result = FuncType::create([ValType::ExternRef, ValType::I32], [ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        // add the function into the import_obj module
        import_obj.add_func("func-add", host_func);

        // create a Table instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        let result = Table::create(table_ty);
        assert!(result.is_ok());
        let host_table = result.unwrap();
        // add the table into the import_obj module
        import_obj.add_table("table", host_table);

        // create a Memory instance
        let result = MemType::create(1..=2);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        let result = Memory::create(mem_ty);
        assert!(result.is_ok());
        let host_memory = result.unwrap();
        // add the memory into the import_obj module
        import_obj.add_memory("memory", host_memory);

        // create a Global instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        let result = Global::create(global_ty, Value::from_i32(666));
        assert!(result.is_ok());
        let host_global = result.unwrap();
        // add the global into import_obj module
        import_obj.add_global("global_i32", host_global);

        assert_eq!(import_obj.exit_code(), 1);
    }

    #[test]
    fn test_import_obj_wasi() {
        // create WASI
        {
            let result = ImportObject::create_wasi(None, None, None);
            assert!(result.is_ok());
            let result = ImportObject::create_wasi(
                Some(vec!["arg1", "arg2"]),
                Some(vec!["ENV1=VAL1", "ENV1=VAL2", "ENV3=VAL3"]),
                Some(vec![
                    "apiTestData",
                    "Makefile",
                    "CMakeFiles",
                    "ssvmAPICoreTests",
                    ".:.",
                ]),
            );
            assert!(result.is_ok());
            let result = ImportObject::create_wasi(
                None,
                Some(vec!["ENV1=VAL1", "ENV1=VAL2", "ENV3=VAL3"]),
                Some(vec![
                    "apiTestData",
                    "Makefile",
                    "CMakeFiles",
                    "ssvmAPICoreTests",
                    ".:.",
                ]),
            );
            assert!(result.is_ok());
            let import_obj = result.unwrap();

            assert_eq!(import_obj.exit_code(), 0);
        }

        // initialize WASI in VM
        {
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let config = config.wasi(true);
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the ImportObject module from vm
            let result = vm.wasi_import_module_mut();
            assert!(result.is_ok());
            let mut import_wasi = result.unwrap();

            let args = vec!["arg1", "arg2"];
            let envs = vec!["ENV1=VAL1", "ENV1=VAL2", "ENV3=VAL3"];
            let preopens = vec![
                "apiTestData",
                "Makefile",
                "CMakeFiles",
                "ssvmAPICoreTests",
                ".:.",
            ];
            import_wasi.init_wasi(Some(args), Some(envs), Some(preopens));

            assert_eq!(import_wasi.exit_code(), 0);
        }
    }

    #[test]
    fn test_import_obj_wasmedge_process() {
        // create wasmedge_process
        {
            let result = ImportObject::create_wasmedge_process(Some(vec!["arg1", "arg2"]), true);
            assert!(result.is_ok());

            let result = ImportObject::create_wasmedge_process(None, false);
            assert!(result.is_ok());

            let result = ImportObject::create_wasmedge_process(Some(vec!["arg1", "arg2"]), false);
            assert!(result.is_ok());
        }

        // initialize wasmedge_process in VM
        {
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let config = config.wasmedge_process(true);
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            let result = vm.wasmedge_process_import_module_mut();
            assert!(result.is_ok());
            let mut import_wasmedge_process = result.unwrap();
            import_wasmedge_process.init_wasmedge_process(Some(vec!["arg1", "arg2"]), false);
        }
    }

    fn real_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![Value::from_i32(c)])
    }
}
