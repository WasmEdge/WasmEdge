use crate::{
    error::HostFuncError, io::WasmValTypeList, CallingFrame, FuncType, Global, Memory, Table,
    WasmEdgeResult, WasmValue,
};
use wasmedge_sys::{self as sys, AsImport};

/// Creates a normal or wasi [import object](crate::ImportObject).
///
/// # Example
///
/// This example shows how to create a normal import object that contains a host function, a global variable, a memory and a table. The import object is named "extern".
///
/// ```rust
/// // If the version of rust used is less than v1.63, please uncomment the follow attribute.
/// // #![feature(explicit_generic_args_with_impl_trait)]
///
/// use wasmedge_sdk::{
///     types::Val,
///     Global, ImportObjectBuilder, Memory, Table,
///     error::HostFuncError, WasmValue, GlobalType,
///     MemoryType, Mutability, RefType, TableType,
///     ValType, Caller, host_function,
/// };
///
/// fn main() -> Result<(), Box<dyn std::error::Error>> {
///     // a native function to be imported as host function
///     #[host_function]
///     fn real_add(_: Caller, inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
///         if inputs.len() != 2 {
///             return Err(HostFuncError::User(1));
///         }
///
///         let a = if inputs[0].ty() == ValType::I32 {
///             inputs[0].to_i32()
///         } else {
///             return Err(HostFuncError::User(2));
///         };
///
///         let b = if inputs[1].ty() == ValType::I32 {
///             inputs[1].to_i32()
///         } else {
///             return Err(HostFuncError::User(3));
///         };
///
///         let c = a + b;
///
///         Ok(vec![WasmValue::from_i32(c)])
///     }
///
///     // create a Const global instance to be imported
///     let global_const = Global::new(
///         GlobalType::new(ValType::F32, Mutability::Const),
///         Val::F32(3.5),
///     )?;
///
///     // create a memory instance to be imported
///     let memory = Memory::new(MemoryType::new(10, Some(20), false)?)?;
///
///     // create a table instance to be imported
///     let table = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)))?;
///
///     // create an import object
///     let module_name = "extern";
///     let _import = ImportObjectBuilder::new()
///         // add a function
///         .with_func::<(i32, i32), i32>("add", real_add)?
///         // add a global
///         .with_global("global", global_const)?
///         // add a memory
///         .with_memory("memory", memory)?
///         // add a table
///         .with_table("table", table)?
///         .build(module_name)?;
///
///     Ok(())
/// }
/// ```
/// [[Click for more examples]](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust/wasmedge/examples)
///
#[derive(Debug, Default)]
pub struct ImportObjectBuilder {
    funcs: Vec<(String, sys::Function)>,
    globals: Vec<(String, sys::Global)>,
    memories: Vec<(String, sys::Memory)>,
    tables: Vec<(String, sys::Table)>,
}
impl ImportObjectBuilder {
    /// Creates a new [ImportObjectBuilder].
    pub fn new() -> Self {
        Self {
            funcs: Vec::new(),
            globals: Vec::new(),
            memories: Vec::new(),
            tables: Vec::new(),
        }
    }

    /// Adds a [host function](crate::Func) to the [ImportObject] to create.
    ///
    /// N.B. that this function can be used in thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [host function](crate::Func) to add.
    ///
    /// * `real_func` - The native function.
    ///
    /// # error
    ///
    /// If fail to create or add the [host function](crate::Func), then an error is returned.
    pub fn with_func<Args, Rets>(
        mut self,
        name: impl AsRef<str>,
        real_func: impl Fn(CallingFrame, Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError>
            + Send
            + Sync
            + 'static,
    ) -> WasmEdgeResult<Self>
    where
        Args: WasmValTypeList,
        Rets: WasmValTypeList,
    {
        let boxed_func = Box::new(real_func);
        let args = Args::wasm_types();
        let returns = Rets::wasm_types();
        let ty = FuncType::new(Some(args.to_vec()), Some(returns.to_vec()));
        let inner_func = sys::Function::create(&ty.into(), boxed_func, 0)?;
        self.funcs.push((name.as_ref().to_owned(), inner_func));
        Ok(self)
    }

    /// Adds a [host function](crate::Func) to the [ImportObject] to create.
    ///
    /// N.B. that this function can be used in thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [host function](crate::Func) to add.
    ///
    /// * `ty` - The function type.
    ///
    /// * `real_func` - The native function.
    ///
    /// # error
    ///
    /// If fail to create or add the [host function](crate::Func), then an error is returned.
    pub fn with_func_by_type(
        mut self,
        name: impl AsRef<str>,
        ty: FuncType,
        real_func: impl Fn(CallingFrame, Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError>
            + Send
            + Sync
            + 'static,
    ) -> WasmEdgeResult<Self> {
        let boxed_func = Box::new(real_func);
        let inner_func = sys::Function::create(&ty.into(), boxed_func, 0)?;
        self.funcs.push((name.as_ref().to_owned(), inner_func));
        Ok(self)
    }

    /// Adds an [async host function](crate::Func) to the [ImportObject] to create.
    ///
    /// N.B. that this function can be used in thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [host function](crate::Func) to add.
    ///
    /// * `real_func` - The native function.
    ///
    /// # error
    ///
    /// If fail to create or add the [host function](crate::Func), then an error is returned.
    #[cfg(feature = "async")]
    pub fn with_func_async<Args, Rets>(
        mut self,
        name: impl AsRef<str>,
        real_func: impl Fn(
                CallingFrame,
                Vec<WasmValue>,
            ) -> Box<
                dyn std::future::Future<
                        Output = Result<Vec<WasmValue>, crate::error::HostFuncError>,
                    > + Send,
            > + Send
            + Sync
            + 'static,
    ) -> WasmEdgeResult<Self>
    where
        Args: WasmValTypeList,
        Rets: WasmValTypeList,
    {
        let boxed_func = Box::new(real_func);
        let args = Args::wasm_types();
        let returns = Rets::wasm_types();
        let ty = FuncType::new(Some(args.to_vec()), Some(returns.to_vec()));
        let inner_func = sys::Function::create_async(&ty.into(), boxed_func, 0)?;
        self.funcs.push((name.as_ref().to_owned(), inner_func));
        Ok(self)
    }

    /// Adds a [global](crate::Global) to the [ImportObject] to create.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [global](crate::Global) to add.
    ///
    /// * `global` - The wasm [global instance](crate::Global) to add.
    ///
    /// # Error
    ///
    /// If fail to create or add the [global](crate::Global), then an error is returned.
    pub fn with_global(mut self, name: impl AsRef<str>, global: Global) -> WasmEdgeResult<Self> {
        self.globals.push((name.as_ref().to_owned(), global.inner));
        Ok(self)
    }

    /// Adds a [memory](crate::Memory) to the [ImportObject] to create.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [memory](crate::Memory) to add.
    ///
    /// * `memory` - The wasm [memory instance](crate::Memory) to add.
    ///
    /// # Error
    ///
    /// If fail to create or add the [memory](crate::Memory), then an error is returned.
    pub fn with_memory(mut self, name: impl AsRef<str>, memory: Memory) -> WasmEdgeResult<Self> {
        // let inner_memory = sys::Memory::create(&ty.into())?;
        self.memories.push((name.as_ref().to_owned(), memory.inner));
        Ok(self)
    }

    /// Adds a [table](crate::Table) to the [ImportObject] to create.
    ///
    /// # Arguments
    ///
    /// * `name` - The exported name of the [table](crate::Table) to add.
    ///
    /// * `table` - The wasm [table instance](crate::Table) to add.
    ///
    /// # Error
    ///
    /// If fail to create or add the [table](crate::Table), then an error is returned.
    pub fn with_table(mut self, name: impl AsRef<str>, table: Table) -> WasmEdgeResult<Self> {
        // let inner_table = sys::Table::create(&ty.into())?;
        self.tables.push((name.as_ref().to_owned(), table.inner));
        Ok(self)
    }

    /// Creates a new [ImportObject].
    ///
    /// # Argument
    ///
    /// * `name` - The name of the [ImportObject] to create.
    ///
    /// # Error
    ///
    /// If fail to create the [ImportObject], then an error is returned.
    pub fn build(self, name: impl AsRef<str>) -> WasmEdgeResult<ImportObject> {
        let mut inner = sys::ImportModule::create(name.as_ref())?;

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

        Ok(ImportObject(sys::ImportObject::Import(inner)))
    }
}

/// Defines an import object that contains the required import data used when instantiating a [module](crate::Module).
///
/// An [ImportObject] instance is created with [ImportObjectBuilder](crate::ImportObjectBuilder).
#[derive(Debug, Clone)]
pub struct ImportObject(pub(crate) sys::ImportObject);
impl ImportObject {
    /// Returns the name of the import object.
    pub fn name(&self) -> &str {
        match &self.0 {
            sys::ImportObject::Import(import) => import.name(),
            sys::ImportObject::Wasi(wasi) => wasi.name(),
        }
    }

    pub(crate) fn inner_ref(&self) -> &sys::ImportObject {
        &self.0
    }

    /// Returns the raw pointer to the inner `WasmEdge_ModuleInstanceContext`.
    #[cfg(feature = "ffi")]
    pub fn as_raw_ptr(&self) -> *const sys::ffi::WasmEdge_ModuleInstanceContext {
        self.0.as_raw_ptr()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::{GlobalError, WasmEdgeError},
        params,
        types::Val,
        Executor, Global, GlobalType, Memory, MemoryType, Mutability, RefType, Statistics, Store,
        Table, TableType, ValType, WasmVal, WasmValue,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_builder() {
        let result = ImportObjectBuilder::default().build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();
        assert_eq!(import.name(), "extern");
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_add_func() {
        fn real_add(
            _frame: CallingFrame,
            inputs: Vec<WasmValue>,
        ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
            if inputs.len() != 2 {
                return Err(HostFuncError::User(1));
            }

            let a = if inputs[0].ty() == ValType::I32 {
                inputs[0].to_i32()
            } else {
                return Err(HostFuncError::User(2));
            };

            let b = if inputs[1].ty() == ValType::I32 {
                inputs[1].to_i32()
            } else {
                return Err(HostFuncError::User(3));
            };

            let c = a + b;

            Ok(vec![WasmValue::from_i32(c)])
        }

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
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
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_ok());
        let host_func = result.unwrap();

        // check the signature of the host function
        let func_ty = host_func.ty();
        assert!(func_ty.args().is_some());
        assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
        assert!(func_ty.returns().is_some());
        assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);

        let returns = host_func.run(&mut executor, params![1, 2]).unwrap();
        assert_eq!(returns[0].to_i32(), 3);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_add_memory() {
        // create a memory
        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_memory("memory", memory)
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
        assert!(result.is_err());

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.named_instance("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the exported memory
        let result = instance.memory("memory");
        assert!(result.is_ok());
        let mut memory = result.unwrap();

        // check memory
        assert!(memory.name().is_some());
        assert_eq!(memory.name().unwrap(), "memory");
        assert_eq!(memory.mod_name(), Some("extern"));
        assert_eq!(memory.page(), 10);
        let ty = memory.ty();
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), Some(20));

        // grow memory
        let result = memory.grow(5);
        assert!(result.is_ok());
        assert_eq!(memory.page(), 15);

        // get memory from instance again
        let result = instance.memory("memory");
        assert!(result.is_ok());
        let memory = result.unwrap();
        assert_eq!(memory.page(), 15);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_add_global() {
        // create a Const global variable
        let result = Global::new(
            GlobalType::new(ValType::I32, Mutability::Const),
            Val::I32(1314),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a Var global variable
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Var),
            Val::F32(13.14),
        );
        assert!(result.is_ok());
        let global_var = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_global("const-global", global_const)
            .expect("failed to add const-global")
            .with_global("var-global", global_var)
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
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the Const global from the store of vm
        let result = instance.global("const-global");
        assert!(result.is_ok());
        let mut const_global = result.unwrap();

        // check global
        assert!(const_global.name().is_some());
        assert_eq!(const_global.name().unwrap(), "const-global");
        assert!(const_global.mod_name().is_some());
        assert_eq!(const_global.mod_name().unwrap(), "extern");
        let ty = const_global.ty();
        assert_eq!(ty.value_ty(), ValType::I32);
        assert_eq!(ty.mutability(), Mutability::Const);

        // get value of global
        if let Val::I32(value) = const_global.get_value() {
            assert_eq!(value, 1314);
        }

        // set a new value
        let result = const_global.set_value(Val::I32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Global(GlobalError::ModifyConst))
        );

        // get the Var global from the store of vm
        let result = store.named_instance("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the Var global from the store of vm
        let result = instance.global("var-global");
        assert!(result.is_ok());
        let mut var_global = result.unwrap();

        // check global
        assert!(var_global.name().is_some());
        assert_eq!(var_global.name().unwrap(), "var-global");
        assert!(var_global.mod_name().is_some());
        assert_eq!(var_global.mod_name().unwrap(), "extern");
        let ty = var_global.ty();
        assert_eq!(ty.value_ty(), ValType::F32);
        assert_eq!(ty.mutability(), Mutability::Var);

        // get the value of var_global
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 13.14);
        }

        // set a new value
        let result = var_global.set_value(Val::F32(1.314));
        assert!(result.is_ok());

        // get the value of var_global again
        let result = instance.global("var-global");
        assert!(result.is_ok());
        let var_global = result.unwrap();
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 1.314);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_add_table() {
        // create a wasm table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host func")
            .with_table("table", table)
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
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_ok());
        let host_func = result.unwrap();

        // get the exported table
        let result = instance.table("table");
        assert!(result.is_ok());
        let mut table = result.unwrap();

        // check table
        assert!(table.name().is_some());
        assert_eq!(table.name().unwrap(), "table");
        assert!(table.mod_name().is_some());
        assert_eq!(table.mod_name().unwrap(), "extern");
        assert_eq!(table.size(), 10);
        let ty = table.ty();
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), Some(20));

        // get value from table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_none());
        }

        // set value to table[0]
        let func_ref = host_func.as_ref();
        let result = table.set(0, Val::FuncRef(Some(func_ref)));
        assert!(result.is_ok());
        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            // check the signature of the host function
            let func_ty = func_ref.ty();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);
        }

        let result = store.named_instance("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        let result = instance.table("table");
        assert!(result.is_ok());
        let table = result.unwrap();

        // get the value in table[0]
        let result = table.get(0);
        assert!(result.is_ok());
        if let Val::FuncRef(func_ref) = result.unwrap() {
            assert!(func_ref.is_some());
            let func_ref = func_ref.unwrap();
            // check the signature of the host function
            let func_ty = func_ref.ty();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_send() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an ImportModule instance
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host function")
            .with_global("global", global_const)
            .expect("failed to add const global")
            .with_memory("memory", memory)
            .expect("failed to add memory")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern-module-send");
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
            let result = store.named_instance("extern-module-send");
            assert!(result.is_ok());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), "extern-module-send");

            // check the exported global
            let result = instance.global("global");
            assert!(result.is_ok());
            let global = result.unwrap();
            let ty = global.ty();
            assert_eq!(*ty, GlobalType::new(ValType::F32, Mutability::Const));
            if let Val::F32(value) = global.get_value() {
                assert_eq!(value, 3.5);
            }

            // get the exported memory
            let result = instance.memory("memory");
            assert!(result.is_ok());
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
            assert!(result.is_ok());
            let table = result.unwrap();
            // check table
            assert!(table.name().is_some());
            assert_eq!(table.name().unwrap(), "table");
            assert!(table.mod_name().is_some());
            assert_eq!(table.mod_name().unwrap(), "extern-module-send");
            assert_eq!(table.size(), 10);
            let ty = table.ty();
            assert_eq!(ty.elem_ty(), RefType::FuncRef);
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum(), Some(20));

            // get the exported host function
            let result = instance.func("add");
            assert!(result.is_ok());
            let host_func = result.unwrap();
            // check the signature of the host function
            let func_ty = host_func.ty();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);
        });

        handle.join().unwrap();
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_import_sync() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Const),
            Val::F32(3.5),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a memory instance
        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create a table instance
        let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
        assert!(result.is_ok());
        let table = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", real_add)
            .expect("failed to add host function")
            .with_global("global", global_const)
            .expect("failed to add const global")
            .with_memory("memory", memory)
            .expect("failed to add memory")
            .with_table("table", table)
            .expect("failed to add table")
            .build("extern-module-sync");
        assert!(result.is_ok());
        let import = result.unwrap();
        let import = Arc::new(Mutex::new(import));

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
            let result = store.named_instance("extern-module-sync");
            assert!(result.is_ok());
            let instance = result.unwrap();
            assert!(instance.name().is_some());
            assert_eq!(instance.name().unwrap(), "extern-module-sync");

            // check the exported global
            let result = instance.global("global");
            assert!(result.is_ok());
            let global = result.unwrap();
            let ty = global.ty();
            assert_eq!(*ty, GlobalType::new(ValType::F32, Mutability::Const));
            if let Val::F32(v) = global.get_value() {
                assert_eq!(v, 3.5);
            }

            // get the exported memory
            let result = instance.memory("memory");
            assert!(result.is_ok());
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
            assert!(result.is_ok());
            let table = result.unwrap();
            // check table
            assert!(table.name().is_some());
            assert_eq!(table.name().unwrap(), "table");
            assert!(table.mod_name().is_some());
            assert_eq!(table.mod_name().unwrap(), "extern-module-sync");
            assert_eq!(table.size(), 10);
            let ty = table.ty();
            assert_eq!(ty.elem_ty(), RefType::FuncRef);
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum(), Some(20));

            // get the exported host function
            let result = instance.func("add");
            assert!(result.is_ok());
            let host_func = result.unwrap();
            // check the signature of the host function
            let func_ty = host_func.ty();
            assert!(func_ty.args().is_some());
            assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
            assert!(func_ty.returns().is_some());
            assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);

            // run host func
            let result = host_func.run(&mut executor, params!(2, 3));
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 5);
        });

        handle.join().unwrap();
    }

    fn real_add(
        _frame: CallingFrame,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
