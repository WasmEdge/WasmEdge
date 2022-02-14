use crate::{error::Result, wasmedge, Func, GlobalType, MemoryType, TableType, Value, Vm};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct ImportMod {
    pub(crate) inner: wasmedge::ImportObject,
}
impl ImportMod {
    pub fn new(name: impl AsRef<str>) -> Result<Self> {
        let inner = wasmedge::ImportObject::create(name.as_ref())?;
        Ok(Self { inner })
    }

    pub fn new_wasi<'a>(
        args: Option<Vec<&'a str>>,
        envs: Option<Vec<&'a str>>,
        preopens: Option<Vec<&'a str>>,
    ) -> Result<Self> {
        let inner = wasmedge::ImportObject::create_wasi(args, envs, preopens)?;
        Ok(Self { inner })
    }

    pub fn new_wasmedge_process<'a>(
        allowed_cmds: Option<Vec<&'a str>>,
        allowed: bool,
    ) -> Result<Self> {
        let inner = wasmedge::ImportObject::create_wasmedge_process(allowed_cmds, allowed)?;
        Ok(Self { inner })
    }

    pub fn name(&self) -> String {
        self.inner.name()
    }

    pub fn add_func(&mut self, name: impl AsRef<str>, func: Func) {
        self.inner.add_func(name.as_ref(), func.inner)
    }

    /// Given the type and the value, creates a new [Global](crate::Global) instance and adds it to this import module.
    pub fn add_global(
        &mut self,
        name: impl AsRef<str>,
        global_ty: GlobalType,
        value: Value,
    ) -> Result<()> {
        let ty = global_ty.to_raw()?;
        let global = wasmedge::Global::create(ty, value)?;
        self.inner.add_global(name.as_ref(), global);
        Ok(())
    }

    /// Given the type and the value, creates a new [Memory](crate::Memory) instance and adds it to this import module.
    pub fn add_memory(&mut self, name: impl AsRef<str>, memory_ty: MemoryType) -> Result<()> {
        let ty = memory_ty.to_raw()?;
        let memory = wasmedge::Memory::create(ty)?;
        self.inner.add_memory(name.as_ref(), memory);
        Ok(())
    }

    /// Given the type and the value, creates a new [Table](crate::Table) instance and adds it to this import module.
    pub fn add_table(&mut self, name: impl AsRef<str>, table_ty: TableType) -> Result<()> {
        let ty = table_ty.to_raw()?;
        let table = wasmedge::Table::create(ty)?;
        self.inner.add_table(name.as_ref(), table);
        Ok(())
    }
}

#[derive(Debug)]
pub struct WasiImportMod<'vm> {
    pub(crate) inner: wasmedge::ImportObject,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> WasiImportMod<'vm> {
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
pub struct WasmEdgeProcessImportMod<'vm> {
    pub(crate) inner: wasmedge::ImportObject,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> WasmEdgeProcessImportMod<'vm> {
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
        error::WasmEdgeError, wasmedge, Config, Mutability, RefType, SignatureBuilder, ValType,
        Value,
    };

    #[test]
    fn test_import_new_wasmedgeprocess() {
        {
            // create a Config
            let result = Config::new();
            let config = result.unwrap().wasmedge_process(true);

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get WasmEdgeProcess module from vm
            let result = vm.wasmedge_process_module();
            assert!(result.is_ok());

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.mod_count(), 1);
            let result = store.mod_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);

            // * try to add another WasmEdgeProcess module, that causes error

            // create a WasmEdgeProcess module
            let result = ImportMod::new_wasmedge_process(None, false);
            assert!(result.is_ok());
            let import_process = result.unwrap();

            let result = vm.add_named_import(import_process);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                    wasmedge::error::CoreError::Instantiation(
                        wasmedge::error::CoreInstantiationError::ModuleNameConflict
                    )
                ))
            );
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = Config::new();
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get WasmEdgeProcess module from vm
            let result = vm.wasmedge_process_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasmEdgeProcessImportObjectModule
                ))
            );

            // *** try to add a WasmEdgeProcess module.

            // create a WasmEdgeProcess module
            let result = ImportMod::new_wasmedge_process(None, false);
            assert!(result.is_ok());
            let mut import_process = result.unwrap();

            // add host function
            let signature = SignatureBuilder::new()
                .with_args(vec![ValType::I32; 2])
                .with_returns(vec![ValType::I32])
                .build();
            let result = Func::new(signature, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import_process.add_func("add", host_func);

            let result = vm.add_named_import(import_process);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasmEdgeProcessImportObjectModule
                ))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.mod_count(), 1);
            let result = store.mod_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);

            // find "add" host function
            let result = store.function("add", Some("wasmedge_process"));
            assert!(result.is_ok());
            let host_func = result.unwrap();
            assert_eq!(host_func.name().unwrap(), "add");
            assert_eq!(host_func.mod_name().unwrap(), "wasmedge_process");
        }
    }

    #[test]
    fn test_import_new_wasi() {
        {
            // create a Config
            let result = Config::new();
            assert!(result.is_ok());
            let config = result.unwrap().wasi(true);

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get Wasi module from vm
            let result = vm.wasi_module();
            assert!(result.is_ok());

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.mod_count(), 1);
            let result = store.mod_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

            // * try to add another Wasi module, that causes error

            // create a Wasi module
            let result = ImportMod::new_wasi(None, None, None);
            assert!(result.is_ok());
            let import_wasi = result.unwrap();

            let result = vm.add_named_import(import_wasi);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                    wasmedge::error::CoreError::Instantiation(
                        wasmedge::error::CoreInstantiationError::ModuleNameConflict
                    )
                ))
            );
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = Config::new();
            let config = result.unwrap();

            // create a Vm context
            let result = Vm::new(Some(config));
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get Wasi module from vm
            let result = vm.wasi_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasiImportObjectModule
                ))
            );

            // *** try to add a Wasi module.

            // create a WasmEdgeProcess module
            let result = ImportMod::new_wasi(None, None, None);
            assert!(result.is_ok());
            let mut import_wasi = result.unwrap();

            // add host function
            let signature = SignatureBuilder::new()
                .with_args(vec![ValType::I32; 2])
                .with_returns(vec![ValType::I32])
                .build();
            let result = Func::new(signature, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import_wasi.add_func("add", host_func);

            let result = vm.add_named_import(import_wasi);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the Wasi module
            let result = vm.wasi_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasiImportObjectModule
                ))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.mod_count(), 1);
            let result = store.mod_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

            // find "add" host function
            let result = store.function("add", Some("wasi_snapshot_preview1"));
            assert!(result.is_ok());
            let host_func = result.unwrap();
            assert_eq!(host_func.name().unwrap(), "add");
            assert_eq!(host_func.mod_name().unwrap(), "wasi_snapshot_preview1");
        }
    }

    #[test]
    fn test_import_add_memory() {
        // create an ImportObject module
        let result = ImportMod::new("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add a memory
        let result = import.add_memory("memory", MemoryType::new(10, Some(20)));
        assert!(result.is_ok());

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register the ImportObject module into vm
        let result = vm.add_named_import(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get the memory from vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();
        let result = store.memory("memory", Some("extern"));
        assert!(result.is_ok());
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
        assert!(ty.maximum().is_some());
        assert_eq!(ty.maximum().unwrap(), 20);

        // grow memory
        let result = memory.grow(5);
        assert!(result.is_ok());
        assert_eq!(memory.size(), 15);

        // get memory from store agains
        let result = store.memory("memory", Some("extern"));
        assert!(result.is_ok());
        let memory = result.unwrap();
        assert_eq!(memory.size(), 15);
    }

    #[test]
    fn test_import_add_global() {
        // create an ImportObject module
        let result = ImportMod::new("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add a Const global
        let result = import.add_global(
            "const-global",
            GlobalType::new(ValType::I32, Mutability::Const),
            Value::from_i32(1314),
        );
        assert!(result.is_ok());

        // add a Var global
        let result = import.add_global(
            "var-global",
            GlobalType::new(ValType::F32, Mutability::Var),
            Value::from_f32(13.14),
        );
        assert!(result.is_ok());

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register the ImportObject module into vm
        let result = vm.add_named_import(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();

        // get the Const global from the store of vm
        let result = store.global("const-global", Some("extern"));
        assert!(result.is_ok());
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
        assert_eq!(const_global.get_value().to_i32(), 1314);
        // set a new value
        let result = const_global.set_value(Value::from_i32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(wasmedge::WasmEdgeError::Global(
                wasmedge::GlobalError::ModifyConst
            ))
        );

        // get the Var global from the store of vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();

        // get the Var global from the store of vm
        let result = store.global("var-global", Some("extern"));
        assert!(result.is_ok());
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
        assert_eq!(var_global.get_value().to_f32(), 13.14);
        // set a new value
        let result = var_global.set_value(Value::from_f32(1.314));
        assert!(result.is_ok());

        // get the value of var_global again
        let var_global = store.global("var-global", Some("extern")).unwrap();
        assert_eq!(var_global.get_value().to_f32(), 1.314);
    }

    #[test]
    fn test_import_add_table() {
        // create an ImportObject module
        let result = ImportMod::new("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add a table
        let result = import.add_table("table", TableType::new(RefType::FuncRef, 10, Some(20)));
        assert!(result.is_ok());

        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // register the ImportObject module into vm
        let result = vm.add_named_import(import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // get the table from vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();
        let result = store.table("table", Some("extern"));
        assert!(result.is_ok());
        let mut table = result.unwrap();

        // check table
        assert!(table.name().is_some());
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.mod_name().is_some());
        assert_eq!(table.mod_name().unwrap(), "extern");
        assert_eq!(table.size(), 10);
        let result = table.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), Some(20));

        // get value from table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert!(value.func_idx().is_none());

        // set value to table[0]
        let result = table.set(Value::from_func_ref(5), 0);
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert!(value.func_idx().is_some());
        assert_eq!(value.func_idx().unwrap(), 5);

        // get the table from vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let store = result.unwrap();
        let result = store.table("table", Some("extern"));
        assert!(result.is_ok());
        let table = result.unwrap();

        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert!(value.func_idx().is_some());
        assert_eq!(value.func_idx().unwrap(), 5);
    }

    fn real_add(inputs: Vec<Value>) -> std::result::Result<Vec<Value>, u8> {
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
