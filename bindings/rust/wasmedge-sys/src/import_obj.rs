//! Defines WasmEdge ImportObject struct.

use crate::{
    error::WasmEdgeError,
    ffi,
    instance::{Function, Global, Memory, Table},
    types::WasmEdgeString,
    utils::string_to_c_char,
    WasmEdgeResult,
};

/// Struct of WasmEdge ImportObject.
///
/// An [ImportObject](crate::ImportObject) represents a host module with a name. A host module consists of one or more host [functions](crate::Function), [tables](crate::Table), [memories](crate::Memory), and [globals](crate::Global),  which are defined outside WASM modules and fed into WASM modules as imports.
#[derive(Debug)]
pub struct ImportObject {
    pub(crate) inner: InnerImportObject,
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
        let ctx = unsafe { ffi::WasmEdge_ImportObjectCreate(mod_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
            false => Ok(ImportObject {
                inner: InnerImportObject(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the name of the [ImportObject](crate::ImportObject).
    pub fn name(&self) -> String {
        let raw_name = unsafe { ffi::WasmEdge_ImportObjectGetModuleName(self.inner.0 as *const _) };
        raw_name.into()
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
            ffi::WasmEdge_ImportObjectCreateWASI(
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
                inner: InnerImportObject(ctx),
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
            ffi::WasmEdge_ImportObjectInitWASI(
                self.inner.0,
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
        unsafe { ffi::WasmEdge_ImportObjectWASIGetExitCode(self.inner.0) }
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
            ffi::WasmEdge_ImportObjectCreateWasmEdgeProcess(cmds.as_ptr(), cmds_len as u32, allowed)
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
            false => Ok(Self {
                inner: InnerImportObject(ctx),
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
            ffi::WasmEdge_ImportObjectInitWasmEdgeProcess(
                self.inner.0,
                cmds.as_ptr(),
                cmds_len as u32,
                allowed,
            )
        }
    }

    /// Adds a [host function](crate::Function) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the exported host function instance to add.
    pub fn add_func(&mut self, name: impl AsRef<str>, mut func: Function) {
        let func_name: WasmEdgeString = name.into();
        unsafe {
            ffi::WasmEdge_ImportObjectAddFunction(self.inner.0, func_name.as_raw(), func.inner.0);
        }
        func.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [table](crate::Table) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the exported table instance to add.
    pub fn add_table(&mut self, name: impl AsRef<str>, mut table: Table) {
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ImportObjectAddTable(self.inner.0, table_name.as_raw(), table.inner.0);
        }
        table.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [memory](crate::Memory) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the exported memory instance to add.
    pub fn add_memory(&mut self, name: impl AsRef<str>, mut memory: Memory) {
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ImportObjectAddMemory(self.inner.0, mem_name.as_raw(), memory.inner.0);
        }
        memory.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [global](crate::Global) into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the exported global instance to add.
    pub fn add_global(&mut self, name: impl AsRef<str>, mut global: Global) {
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ImportObjectAddGlobal(self.inner.0, global_name.as_raw(), global.inner.0);
        }
        global.inner.0 = std::ptr::null_mut();
    }
}
impl Drop for ImportObject {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ImportObjectDelete(self.inner.0) };
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerImportObject(pub(crate) *mut ffi::WasmEdge_ImportObjectContext);
unsafe impl Send for InnerImportObject {}
unsafe impl Sync for InnerImportObject {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        Config, Executor, FuncType, GlobalType, MemType, Statistics, Store, TableType, Vm,
        WasmValue, WasmValueType,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{Mutability, RefType};

    #[test]
    fn test_import_object_add_instance() {
        let host_name = "extern";

        // create an ImportObj module
        let result = ImportObject::create(host_name);
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // add host function "func-add": (externref, i32) -> (i32)
        let result = FuncType::create(
            [WasmValueType::ExternRef, WasmValueType::I32],
            [WasmValueType::I32],
        );
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        // add the function into the import_obj module
        import_obj.add_func("func-add", host_func);

        // create a Table instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        let result = Table::create(&table_ty);
        assert!(result.is_ok());
        let host_table = result.unwrap();
        // add the table into the import_obj module
        import_obj.add_table("table", host_table);

        // create a Memory instance
        let result = MemType::create(1..=2);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        let result = Memory::create(&mem_ty);
        assert!(result.is_ok());
        let host_memory = result.unwrap();
        // add the memory into the import_obj module
        import_obj.add_memory("memory", host_memory);

        // create a Global instance
        let result = GlobalType::create(WasmValueType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        let result = Global::create(&global_ty, WasmValue::from_i32(666));
        assert!(result.is_ok());
        let host_global = result.unwrap();
        // add the global into import_obj module
        import_obj.add_global("global_i32", host_global);

        assert_eq!(import_obj.exit_code(), 1);
    }

    #[test]
    fn test_import_add_memory() {
        let module_name = "extern";

        // create an ImportObj module
        let result = ImportObject::create(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // create a Memory instance
        let host_memory = {
            let result = MemType::create(10..=20);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            let result = Memory::create(&mem_ty);
            assert!(result.is_ok());
            let host_memory = result.unwrap();
            drop(mem_ty);
            host_memory
        };

        // add the memory into the import_obj module
        import.add_memory("memory", host_memory);

        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::create(Some(config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = executor.register_import_object(&mut store, &import);
        assert!(result.is_ok());

        let result = store.named_module(module_name);
        assert!(result.is_ok());
        let instance = result.unwrap();

        let result = instance.find_memory("memory");
        assert!(result.is_ok());
        let memory = result.unwrap();

        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        assert_eq!(ty.limit(), 10..=20);
    }

    #[test]
    fn test_import_object_wasi() {
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
            let mut config = result.unwrap();
            config.wasi(true);
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
    fn test_import_object_wasmedge_process() {
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
            let mut config = result.unwrap();
            config.wasmedge_process(true);
            let result = Vm::create(Some(config), None);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            let result = vm.wasmedge_process_import_module_mut();
            assert!(result.is_ok());
            let mut import_wasmedge_process = result.unwrap();
            import_wasmedge_process.init_wasmedge_process(Some(vec!["arg1", "arg2"]), false);
        }
    }

    #[test]
    fn test_import_object_send() {
        let host_name = "extern";

        // create an ImportObj module
        let result = ImportObject::create(host_name);
        assert!(result.is_ok());
        let import = result.unwrap();

        let handle = thread::spawn(move || {
            assert!(!import.inner.0.is_null());
            println!("{:?}", import.inner);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_import_object_sync() {
        let host_name = "extern";

        // create ImportObject instance
        let result = ImportObject::create(host_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![WasmValueType::I32; 2], vec![WasmValueType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // add table
        let result = TableType::create(RefType::FuncRef, 0..=u32::MAX);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Table::create(&ty);
        assert!(result.is_ok());
        let table = result.unwrap();
        import.add_table("table", table);

        // add memory
        let memory = {
            let result = MemType::create(10..=20);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            let result = Memory::create(&mem_ty);
            assert!(result.is_ok());
            result.unwrap()
        };
        import.add_memory("memory", memory);

        // add globals
        let result = GlobalType::create(WasmValueType::F32, Mutability::Const);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Global::create(&ty, WasmValue::from_f32(3.5));
        assert!(result.is_ok());
        let global = result.unwrap();
        import.add_global("global", global);

        let import = Arc::new(Mutex::new(import));
        let import_cloned = Arc::clone(&import);
        let handle = thread::spawn(move || {
            let result = import_cloned.lock();
            assert!(result.is_ok());
            let import = result.unwrap();
            assert!(!import.inner.0.is_null());

            // create a store
            let result = Store::create();
            assert!(result.is_ok());
            let mut store = result.unwrap();
            assert!(!store.inner.0.is_null());
            assert!(!store.registered);

            // create an executor
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let result = Executor::create(Some(config), None);
            assert!(result.is_ok());
            let mut executor = result.unwrap();

            // register import object into store
            let result = executor.register_import_object(&mut store, &import);
            assert!(result.is_ok());

            // get the exported module by name
            let result = store.named_module("extern");
            assert!(result.is_ok());
            let instance = result.unwrap();

            // get the exported function by name
            let result = instance.find_func("add");
            assert!(result.is_ok());

            // get the exported global by name
            let result = instance.find_global("global");
            assert!(result.is_ok());
            let global = result.unwrap();
            assert!(!global.inner.0.is_null() && global.registered);
            let val = global.get_value();
            assert_eq!(val.to_f32(), 3.5);

            // get the exported memory by name
            let result = instance.find_memory("memory");
            assert!(result.is_ok());
            let memory = result.unwrap();
            let result = memory.ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            assert_eq!(ty.limit(), 10..=20);

            // get the exported table by name
            let result = instance.find_table("table");
            assert!(result.is_ok());
        });

        handle.join().unwrap();
    }

    fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == WasmValueType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == WasmValueType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
