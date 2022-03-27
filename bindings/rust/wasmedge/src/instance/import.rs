use crate::{error::Result, sys, types::Val, HostFunc, Signature};
use wasmedge_types::{GlobalType, MemoryType, TableType};

#[derive(Debug, Default)]
pub struct ImportModuleBuilder {
    funcs: Vec<(String, sys::Function)>,
    globals: Vec<(String, sys::Global)>,
    memories: Vec<(String, sys::Memory)>,
    tables: Vec<(String, sys::Table)>,
}
impl ImportModuleBuilder {
    pub fn new() -> Self {
        Self {
            funcs: Vec::new(),
            globals: Vec::new(),
            memories: Vec::new(),
            tables: Vec::new(),
        }
    }

    pub fn with_func(
        mut self,
        name: impl AsRef<str>,
        sig: Signature,
        real_func: HostFunc,
    ) -> Result<Self> {
        let inner_func = sys::Function::create(&sig.into(), real_func, 0)?;
        self.funcs.push((name.as_ref().to_owned(), inner_func));
        Ok(self)
    }

    pub fn with_global(mut self, name: impl AsRef<str>, ty: GlobalType, init: Val) -> Result<Self> {
        let inner_global = sys::Global::create(&ty.into(), init.into())?;
        self.globals.push((name.as_ref().to_owned(), inner_global));
        Ok(self)
    }

    pub fn with_memory(mut self, name: impl AsRef<str>, ty: MemoryType) -> Result<Self> {
        let inner_memory = sys::Memory::create(&ty.into())?;
        self.memories.push((name.as_ref().to_owned(), inner_memory));
        Ok(self)
    }

    pub fn with_table(mut self, name: impl AsRef<str>, ty: TableType) -> Result<Self> {
        let inner_table = sys::Table::create(&ty.into())?;
        self.tables.push((name.as_ref().to_owned(), inner_table));
        Ok(self)
    }

    pub fn build(self, name: impl AsRef<str>) -> Result<ImportModule> {
        let mut inner = sys::ImportObject::create(name.as_ref())?;

        // add func
        for (name, func) in self.funcs.into_iter() {
            inner.add_func(name, func);
        }

        // add global
        for (name, global) in self.globals.into_iter() {
            inner.add_global(name, global);
        }

        // add memory
        for (name, memory) in self.memories.into_iter() {
            inner.add_memory(name, memory);
        }

        // add table
        for (name, table) in self.tables.into_iter() {
            inner.add_table(name, table);
        }

        Ok(ImportModule { inner })
    }

    pub fn build_as_wasi<'a>(
        self,
        args: Option<Vec<&'a str>>,
        envs: Option<Vec<&'a str>>,
        preopens: Option<Vec<&'a str>>,
    ) -> Result<ImportModule> {
        let mut inner = sys::ImportObject::create_wasi(args, envs, preopens)?;

        // add func
        for (name, func) in self.funcs.into_iter() {
            inner.add_func(name, func);
        }

        // add global
        for (name, global) in self.globals.into_iter() {
            inner.add_global(name, global);
        }

        // add memory
        for (name, memory) in self.memories.into_iter() {
            inner.add_memory(name, memory);
        }

        // add table
        for (name, table) in self.tables.into_iter() {
            inner.add_table(name, table);
        }

        Ok(ImportModule { inner })
    }

    pub fn build_as_wasmedge_process(
        self,
        allowed_cmds: Option<Vec<&str>>,
        allowed: bool,
    ) -> Result<ImportModule> {
        let mut inner = sys::ImportObject::create_wasmedge_process(allowed_cmds, allowed)?;

        // add func
        for (name, func) in self.funcs.into_iter() {
            inner.add_func(name, func);
        }

        // add global
        for (name, global) in self.globals.into_iter() {
            inner.add_global(name, global);
        }

        // add memory
        for (name, memory) in self.memories.into_iter() {
            inner.add_memory(name, memory);
        }

        // add table
        for (name, table) in self.tables.into_iter() {
            inner.add_table(name, table);
        }

        Ok(ImportModule { inner })
    }
}

#[derive(Debug)]
pub struct ImportModule {
    pub(crate) inner: sys::ImportObject,
}
impl ImportModule {
    pub fn name(&self) -> String {
        self.inner.name()
    }
}

#[derive(Debug)]
pub struct WasiImportModule<'vm> {
    pub(crate) inner: &'vm mut sys::ImportObject,
}
impl<'vm> WasiImportModule<'vm> {
    pub fn name(&self) -> String {
        self.inner.name()
    }

    pub fn init(
        &mut self,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) {
        self.inner.init_wasi(args, envs, preopens)
    }

    pub fn exit_code(&self) -> u32 {
        self.inner.exit_code()
    }
}

#[derive(Debug)]
pub struct WasmEdgeProcessImportModule<'vm> {
    pub(crate) inner: &'vm mut sys::ImportObject,
}
impl<'vm> WasmEdgeProcessImportModule<'vm> {
    pub fn name(&self) -> String {
        self.inner.name()
    }

    pub fn init(&mut self, allowed_cmds: Option<Vec<&str>>, allowed: bool) {
        self.inner.init_wasmedge_process(allowed_cmds, allowed)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::WasmEdgeError,
        sys,
        types::{FuncRef, Val},
        Executor, SignatureBuilder, Statistics, Store, WasmValue, WasmValueType,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{Mutability, RefType, ValType};

    #[test]
    fn test_import_new() {
        {
            let result = ImportModuleBuilder::default().build("extern");
            assert!(result.is_ok());
            let import = result.unwrap();
            assert_eq!(import.name(), "extern");
        }
        {
            let result = ImportModuleBuilder::default().build_as_wasi(None, None, None);
            assert!(result.is_ok());
            let import = result.unwrap();
            assert_eq!(import.name(), "wasi_snapshot_preview1");
        }
        {
            let result = ImportModuleBuilder::default().build_as_wasmedge_process(None, false);
            assert!(result.is_ok());
            let import = result.unwrap();
            assert_eq!(import.name(), "wasmedge_process");
        }
    }

    #[test]
    fn test_import_new_wasmedgeprocess() {
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host func")
            .build_as_wasmedge_process(None, false);
        assert!(result.is_ok());
        let process_import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &process_import);
        assert!(result.is_ok());

        // check registered modules
        assert_eq!(store.named_instance_count(), 1);
        let result = store.instance_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["wasmedge_process"]);
        let result = store.named_instance("wasmedge_process");
        assert!(result.is_some());
        let instance = result.unwrap();

        // find "add" host function
        let result = instance.func("add");
        assert!(result.is_some());
        let host_func = result.unwrap();
        assert!(host_func.name().is_some());
        assert_eq!(host_func.name().unwrap(), "add");
        assert!(host_func.mod_name().is_some());
        assert_eq!(host_func.mod_name().unwrap(), "wasmedge_process");

        // * try to add another WasmEdgeProcess module, that causes error

        // create a WasmEdgeProcess module
        let result = ImportModuleBuilder::default().build_as_wasmedge_process(None, false);
        assert!(result.is_ok());
        let import_process = result.unwrap();

        let result = store.register_import_module(&mut executor, &import_process);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Core(
                sys::error::CoreError::Instantiation(
                    sys::error::CoreInstantiationError::ModuleNameConflict
                )
            ))
        );
    }

    #[test]
    fn test_import_new_wasi() {
        // create a wasi module
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host func")
            .build_as_wasi(None, None, None);
        assert!(result.is_ok());
        let wasi_import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &wasi_import);
        assert!(result.is_ok());

        // check registered modules
        assert_eq!(store.named_instance_count(), 1);
        let result = store.instance_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

        // * try to add another Wasi module, that causes error

        // create a Wasi module
        let result = ImportModuleBuilder::default().build_as_wasi(None, None, None);
        assert!(result.is_ok());
        let wasi_import = result.unwrap();

        let result = store.register_import_module(&mut executor, &wasi_import);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Core(
                sys::error::CoreError::Instantiation(
                    sys::error::CoreInstantiationError::ModuleNameConflict
                )
            ))
        );
    }

    #[test]
    fn test_import_add_func() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host func")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        // get the instance of the ImportObject module
        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let host_func = result.unwrap();

        // check the signature of the host function
        let result = host_func.signature();
        assert!(result.is_ok());
        let signature = result.unwrap();
        assert!(signature.args().is_some());
        assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);
    }

    #[test]
    fn test_import_add_memory() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_memory("memory", MemoryType::new(10, Some(20)))
            .expect("failed to add memory")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.named_instance("extern");
        assert!(result.is_none());

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported memory
        let result = instance.memory("memory");
        assert!(result.is_some());
        let mut memory = result.unwrap();

        // check memory
        assert!(memory.name().is_some());
        assert_eq!(memory.name().unwrap(), "memory");
        assert_eq!(memory.mod_name(), Some("extern"));
        assert_eq!(memory.size(), 10);
        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), 20);

        // grow memory
        let result = memory.grow(5);
        assert!(result.is_ok());
        assert_eq!(memory.size(), 15);

        // get memory from instance agains
        let result = instance.memory("memory");
        assert!(result.is_some());
        let memory = result.unwrap();
        assert_eq!(memory.size(), 15);
    }

    #[test]
    fn test_import_add_global() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_global(
                "const-global",
                GlobalType::new(ValType::I32, Mutability::Const),
                Val::I32(1314),
            )
            .expect("failed to add const-global")
            .with_global(
                "var-global",
                GlobalType::new(ValType::F32, Mutability::Var),
                Val::F32(13.14),
            )
            .expect("failed to add var-global")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the Const global from the store of vm
        let result = instance.global("const-global");
        assert!(result.is_some());
        let mut const_global = result.unwrap();

        // check global
        assert!(const_global.name().is_some());
        assert_eq!(const_global.name().unwrap(), "const-global");
        assert!(const_global.mod_name().is_some());
        assert_eq!(const_global.mod_name().unwrap(), "extern");
        let result = const_global.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.value_ty(), ValType::I32);
        assert_eq!(ty.mutability(), Mutability::Const);

        // get value of global
        if let Val::I32(value) = const_global.get_value() {
            assert_eq!(value, 1314);
        } else {
            assert!(false);
        }

        // set a new value
        let result = const_global.set_value(Val::I32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Global(
                sys::error::GlobalError::ModifyConst
            ))
        );

        // get the Var global from the store of vm
        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the Var global from the store of vm
        let result = instance.global("var-global");
        assert!(result.is_some());
        let mut var_global = result.unwrap();

        // check global
        assert!(var_global.name().is_some());
        assert_eq!(var_global.name().unwrap(), "var-global");
        assert!(var_global.mod_name().is_some());
        assert_eq!(var_global.mod_name().unwrap(), "extern");
        let result = var_global.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.value_ty(), ValType::F32);
        assert_eq!(ty.mutability(), Mutability::Var);

        // get the value of var_global
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 13.14);
        } else {
            assert!(false);
        }

        // set a new value
        let result = var_global.set_value(Val::F32(1.314));
        assert!(result.is_ok());

        // get the value of var_global again
        let result = instance.global("var-global");
        assert!(result.is_some());
        let var_global = result.unwrap();
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 1.314);
        } else {
            assert!(false);
        }
    }

    #[test]
    fn test_import_add_table() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host func")
            .with_table("table", TableType::new(RefType::FuncRef, 10, Some(20)))
            .expect("failed to add table")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let mut host_func = result.unwrap();

        // get the exported table
        let result = instance.table("table");
        assert!(result.is_some());
        let mut table = result.unwrap();

        // check table
        assert!(table.name().is_some());
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.mod_name().is_some());
        assert_eq!(table.mod_name().unwrap(), "extern");
        assert_eq!(table.capacity(), 10);
        let result = table.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), 20);

        // get value from table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_none());
        } else {
            assert!(false);
        }

        // set value to table[0]
        let func_ref = FuncRef::new(&mut host_func);
        let result = table.set(Val::FuncRef(Some(func_ref)), 0);
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            let result = func_ref.as_func();
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);
        } else {
            assert!(false)
        }

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        let result = instance.table("table");
        assert!(result.is_some());
        let table = result.unwrap();

        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            let result = func_ref.as_func();
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);
        } else {
            assert!(false);
        }
    }

    #[test]
    fn test_import_send() {
        // create an ImportModule instance
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host function")
            .with_global(
                "global",
                GlobalType::new(ValType::F32, Mutability::Const),
                Val::F32(3.5),
            )
            .expect("failed to add const global")
            .with_memory("memory", MemoryType::new(10, Some(20)))
            .expect("failed to add memory")
            .with_table("table", TableType::new(RefType::FuncRef, 10, Some(20)))
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = result.unwrap();

        let handle = thread::spawn(move || {
            // create an executor
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Statistics::new();
            assert!(result.is_ok());
            let mut stat = result.unwrap();

            let result = Executor::new(Some(&config), Some(&mut stat));
            assert!(result.is_ok());
            let mut executor = result.unwrap();

            // create a store
            let result = Store::new();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            // register an import module into store
            let result = store.register_import_module(&mut executor, &import);
            assert!(result.is_ok());

            // get active module instance
            let result = store.named_instance("extern-module");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), "extern-module");

            // check the exported global
            let result = instance.global("global");
            assert!(result.is_some());
            let global = result.unwrap();
            let result = global.ty();
            assert!(result.is_ok());
            if let Val::F32(value) = global.get_value() {
                assert_eq!(value, 3.5);
            } else {
                assert!(false);
            }

            // get the exported memory
            let result = instance.memory("memory");
            assert!(result.is_some());
            let mut memory = result.unwrap();
            // write data
            let result = memory.write(vec![1; 10], 10);
            assert!(result.is_ok());
            // read data after write data
            let result = memory.read(10, 10);
            assert!(result.is_ok());
            let data = result.unwrap();
            assert_eq!(data, vec![1; 10]);

            // get the exported table by name
            let result = instance.table("table");
            assert!(result.is_some());
            let table = result.unwrap();
            // check table
            assert!(table.name().is_some());
            assert_eq!(table.name().unwrap(), "table");
            assert!(table.mod_name().is_some());
            assert_eq!(table.mod_name().unwrap(), "extern-module");
            assert_eq!(table.capacity(), 10);
            let result = table.ty();
            assert!(result.is_ok());
            // check table type
            let ty = result.unwrap();
            assert_eq!(ty.elem_ty(), RefType::FuncRef);
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum(), 20);

            // get the exported host function
            let result = instance.func("add");
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_import_sync() {
        // create an ImportModule instance
        let result = ImportModuleBuilder::new()
            .with_func(
                "add",
                SignatureBuilder::new()
                    .with_args(vec![WasmValueType::I32; 2])
                    .with_returns(vec![WasmValueType::I32])
                    .build(),
                Box::new(real_add),
            )
            .expect("failed to add host function")
            .with_global(
                "global",
                GlobalType::new(ValType::F32, Mutability::Const),
                Val::F32(3.5),
            )
            .expect("failed to add const global")
            .with_memory("memory", MemoryType::new(10, Some(20)))
            .expect("failed to add memory")
            .with_table("table", TableType::new(RefType::FuncRef, 10, Some(20)))
            .expect("failed to add table")
            .build("extern-module");
        assert!(result.is_ok());
        let import = Arc::new(Mutex::new(result.unwrap()));

        let import_cloned = Arc::clone(&import);
        let handle = thread::spawn(move || {
            let result = import_cloned.lock();
            assert!(result.is_ok());
            let import = result.unwrap();

            // create an executor
            let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Statistics::new();
            assert!(result.is_ok());
            let mut stat = result.unwrap();

            let result = Executor::new(Some(&config), Some(&mut stat));
            assert!(result.is_ok());
            let mut executor = result.unwrap();

            // create a store
            let result = Store::new();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            // register an import module into store
            let result = store.register_import_module(&mut executor, &import);
            assert!(result.is_ok());

            // get active module instance
            let result = store.named_instance("extern-module");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), "extern-module");

            // check the exported global
            let result = instance.global("global");
            assert!(result.is_some());
            let global = result.unwrap();
            let result = global.ty();
            assert!(result.is_ok());
            if let Val::F32(v) = global.get_value() {
                assert_eq!(v, 3.5);
            } else {
                assert!(false);
            }

            // get the exported memory
            let result = instance.memory("memory");
            assert!(result.is_some());
            let mut memory = result.unwrap();
            // write data
            let result = memory.write(vec![1; 10], 10);
            assert!(result.is_ok());
            // read data after write data
            let result = memory.read(10, 10);
            assert!(result.is_ok());
            let data = result.unwrap();
            assert_eq!(data, vec![1; 10]);

            // get the exported table by name
            let result = instance.table("table");
            assert!(result.is_some());
            let table = result.unwrap();
            // check table
            assert!(table.name().is_some());
            assert_eq!(table.name().unwrap(), "table");
            assert!(table.mod_name().is_some());
            assert_eq!(table.mod_name().unwrap(), "extern-module");
            assert_eq!(table.capacity(), 10);
            let result = table.ty();
            assert!(result.is_ok());
            // check table type
            let ty = result.unwrap();
            assert_eq!(ty.elem_ty(), RefType::FuncRef);
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum(), 20);

            // get the exported host function
            let result = instance.func("add");
            assert!(result.is_some());
            let host_func = result.unwrap();
            // check the signature of the host function
            let result = host_func.signature();
            assert!(result.is_ok());
            let signature = result.unwrap();
            assert!(signature.args().is_some());
            assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
            assert!(signature.returns().is_some());
            assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);

            // run host func
            let result = executor.run_func(
                &mut store,
                Some("extern-module"),
                "add",
                [WasmValue::from_i32(2), WasmValue::from_i32(3)],
            );
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 5);
        });

        handle.join().unwrap();
    }

    fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
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
