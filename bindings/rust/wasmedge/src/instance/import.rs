use crate::{
    error::Result, wasmedge, GlobalType, HostFunc, MemoryType, Signature, TableType, Value,
};

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

    pub fn new_wasmedge_process(allowed_cmds: Option<Vec<&str>>, allowed: bool) -> Result<Self> {
        let inner = wasmedge::ImportObject::create_wasmedge_process(allowed_cmds, allowed)?;
        Ok(Self { inner })
    }

    pub fn name(&self) -> String {
        self.inner.name()
    }

    pub fn add_func(
        &mut self,
        name: impl AsRef<str>,
        sig: Signature,
        real_func: Box<HostFunc>,
    ) -> Result<()> {
        let inner = wasmedge::Function::create(sig.into(), real_func, 0)?;
        self.inner.add_func(name.as_ref(), inner);
        Ok(())
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
        let inner_ty = memory_ty.to_raw()?;
        let memory = wasmedge::Memory::create(&inner_ty)?;
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
    pub(crate) inner: &'vm mut wasmedge::ImportObject,
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
    pub(crate) inner: &'vm mut wasmedge::ImportObject,
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
        error::WasmEdgeError, wasmedge, Config, Executor, Mutability, RefType, SignatureBuilder,
        Statistics, Store, ValType, Value,
    };

    #[test]
    fn test_import_new_wasmedgeprocess() {
        // create a WasmEdgeProcess module
        let result = ImportMod::new_wasmedge_process(None, false);
        assert!(result.is_ok());
        let mut process_import = result.unwrap();

        // add host function
        let signature = SignatureBuilder::new()
            .with_args(vec![ValType::I32; 2])
            .with_returns(vec![ValType::I32])
            .build();
        let result = process_import.add_func("add", signature, Box::new(real_add));
        assert!(result.is_ok());

        // create an executor
        let result = Config::new();
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
        let result = ImportMod::new_wasmedge_process(None, false);
        assert!(result.is_ok());
        let import_process = result.unwrap();

        let result = store.register_import_module(&mut executor, &import_process);
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

    #[test]
    fn test_import_new_wasi() {
        // create a WasmEdgeProcess module
        let result = ImportMod::new_wasi(None, None, None);
        assert!(result.is_ok());
        let mut wasi_import = result.unwrap();

        // add host function
        let signature = SignatureBuilder::new()
            .with_args(vec![ValType::I32; 2])
            .with_returns(vec![ValType::I32])
            .build();
        let result = wasi_import.add_func("add", signature, Box::new(real_add));
        assert!(result.is_ok());

        // create an executor
        let result = Config::new();
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
        let result = ImportMod::new_wasi(None, None, None);
        assert!(result.is_ok());
        let wasi_import = result.unwrap();

        let result = store.register_import_module(&mut executor, &wasi_import);
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

    #[test]
    fn test_import_add_func() {
        // create an ImportObject module
        let result = ImportMod::new("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let signature = SignatureBuilder::new()
            .with_args(vec![ValType::I32; 2])
            .with_returns(vec![ValType::I32])
            .build();
        let result = import.add_func("add", signature, Box::new(real_add));
        assert!(result.is_ok());

        // create an executor
        let result = Config::new();
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
        assert_eq!(signature.args().unwrap(), [ValType::I32; 2]);
        assert!(signature.returns().is_some());
        assert_eq!(signature.returns().unwrap(), [ValType::I32]);
    }

    #[test]
    fn test_import_add_memory() {
        // create an ImportObject module
        let result = ImportMod::new("extern");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add a memory
        let mem_ty = MemoryType::new(10, Some(20));

        // add a memory to the import module
        let result = import.add_memory("memory", mem_ty);
        assert!(result.is_ok());

        // create an executor
        let result = Config::new();
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

        // create an executor
        let result = Config::new();
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
        assert_eq!(const_global.get_value().to_i32(), 1314);
        // set a new value
        let result = const_global.set_value(Value::from_i32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Global(
                wasmedge::error::GlobalError::ModifyConst
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
        assert_eq!(var_global.get_value().to_f32(), 13.14);
        // set a new value
        let result = var_global.set_value(Value::from_f32(1.314));
        assert!(result.is_ok());

        // get the value of var_global again
        let result = instance.global("var-global");
        assert!(result.is_some());
        let var_global = result.unwrap();
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

        // create an executor
        let result = Config::new();
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

        let result = instance.table("table");
        assert!(result.is_some());
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

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        let result = instance.table("table");
        assert!(result.is_some());
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
