//! Defines WasmEdge Store struct.

use crate::{
    error::{StoreError, WasmEdgeError},
    ffi,
    instance::module::{InnerInstance, Instance},
    types::WasmEdgeString,
    WasmEdgeResult,
};

/// A [Store] represents all global state that can be manipulated by WebAssembly programs. It consists of the runtime representation of all instances of [functions](crate::Function), [tables](crate::Table), [memories](crate::Memory), and [globals](crate::Global).
#[derive(Debug)]
pub struct Store {
    pub(crate) inner: InnerStore,
    pub(crate) registered: bool,
}
impl Store {
    /// Creates a new [Store].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_StoreCreate() };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Store(StoreError::Create))),
            false => Ok(Store {
                inner: InnerStore(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the length of the registered [modules](crate::Module).
    pub fn module_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_StoreListModuleLength(self.inner.0 as *const _) }
    }

    /// Returns the names of all registered [modules](crate::Module).
    pub fn module_names(&self) -> Option<Vec<String>> {
        let len_mod_names = self.module_len();
        match len_mod_names > 0 {
            true => {
                let mut mod_names = Vec::with_capacity(len_mod_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListModule(
                        self.inner.0,
                        mod_names.as_mut_ptr(),
                        len_mod_names,
                    );
                    mod_names.set_len(len_mod_names as usize);
                };

                let names = mod_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the module instance by the module name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the module instance to get.
    ///
    /// # Error
    ///
    /// If fail to find the target [module instance](crate::Instance), then an error is returned.
    pub fn module(&self, name: impl AsRef<str>) -> WasmEdgeResult<Instance> {
        let mod_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { ffi::WasmEdge_StoreFindModule(self.inner.0, mod_name.as_raw()) };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Store(StoreError::NotFoundModule(
                name.as_ref().to_string(),
            )))),
            false => Ok(Instance {
                inner: std::sync::Arc::new(InnerInstance(ctx as *mut _)),
                registered: true,
            }),
        }
    }

    /// Checks if the [Store] contains a module of which the name matches the given name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the module to search.
    ///
    pub fn contains(&self, name: impl AsRef<str>) -> bool {
        if self.module_len() == 0 {
            return false;
        }

        match self.module_names() {
            Some(names) => names.iter().any(|x| x == name.as_ref()),
            None => false,
        }
    }

    /// Provides a raw pointer to the inner Store context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_StoreContext {
        self.inner.0 as *const _
    }
}
impl Drop for Store {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_StoreDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerStore(pub(crate) *mut ffi::WasmEdge_StoreContext);
unsafe impl Send for InnerStore {}
unsafe impl Sync for InnerStore {}

#[cfg(test)]
mod tests {
    use super::Store;
    use crate::{
        instance::{Function, Global, GlobalType, MemType, Memory, Table, TableType},
        types::WasmValue,
        AsImport, CallingFrame, Config, Engine, Executor, FuncType, ImportModule, ImportObject,
        Loader, Validator,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{error::HostFuncError, Mutability, RefType, ValType};

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_basic() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();
        assert!(!store.inner.0.is_null());
        assert!(!store.registered);

        // check the length of registered module list in store before instantiation
        assert_eq!(store.module_len(), 0);
        assert!(store.module_names().is_none());

        // create ImportObject instance
        let result = ImportModule::create(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // add table
        let result = TableType::create(RefType::FuncRef, 0, Some(u32::MAX));
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Table::create(&ty);
        assert!(result.is_ok());
        let table = result.unwrap();
        import.add_table("table", table);

        // add memory
        let memory = {
            let result = MemType::create(10, Some(20), false);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            let result = Memory::create(&mem_ty);
            assert!(result.is_ok());
            result.unwrap()
        };
        import.add_memory("mem", memory);

        // add globals
        let result = GlobalType::create(ValType::F32, Mutability::Const);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Global::create(&ty, WasmValue::from_f32(3.5));
        assert!(result.is_ok());
        let global = result.unwrap();
        import.add_global("global", global);

        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let result = Executor::create(Some(&config), None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        let import = ImportObject::Import(import);
        let result = executor.register_import_object(&mut store, &import);
        assert!(result.is_ok());

        // check the module list after instantiation
        assert_eq!(store.module_len(), 1);
        assert!(store.module_names().is_some());
        assert_eq!(store.module_names().unwrap()[0], module_name);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_send() {
        let result = Store::create();
        assert!(result.is_ok());
        let store = result.unwrap();
        assert!(!store.inner.0.is_null());
        assert!(!store.registered);

        let handle = thread::spawn(move || {
            let s = store;
            assert!(!s.inner.0.is_null());
        });

        handle.join().unwrap();
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_sync() {
        let result = Store::create();
        assert!(result.is_ok());
        let store = Arc::new(Mutex::new(result.unwrap()));

        let store_cloned = Arc::clone(&store);
        let handle = thread::spawn(move || {
            // create ImportObject instance
            let result = ImportModule::create("extern_module");
            assert!(result.is_ok());
            let mut import = result.unwrap();

            // add host function
            let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            let result = Function::create(&func_ty, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import.add_func("add", host_func);

            // create an Executor
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let result = Executor::create(Some(&config), None);
            assert!(result.is_ok());
            let mut executor = result.unwrap();

            let result = store_cloned.lock();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            let import = ImportObject::Import(import);
            let result = executor.register_import_object(&mut store, &import);
            assert!(result.is_ok());

            // get module instance
            let result = store.module("extern_module");
            assert!(result.is_ok());
            let instance = result.unwrap();

            // get function instance
            let result = instance.get_func("add");
            assert!(result.is_ok());
            let add = result.unwrap();

            // run the function
            let result =
                executor.run_func(&add, vec![WasmValue::from_i32(12), WasmValue::from_i32(21)]);
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 33);
        });

        handle.join().unwrap();
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_store_named_module() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create an executor with the given config
        let mut executor = Executor::create(Some(&config), None)?;

        // create a store
        let mut store = Store::create()?;

        // register a wasm module from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/fibonacci.wat");
        let module = Loader::create(Some(&config))?.from_file(path)?;
        Validator::create(Some(&config))?.validate(&module)?;
        let instance = executor.register_named_module(&mut store, &module, "extern")?;

        // check the name of the module
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern");

        // get the exported function named "fib"
        let result = instance.get_func("fib");
        assert!(result.is_ok());
        let func = result.unwrap();

        // check the type of the function
        let result = func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check the parameter types
        let param_types = ty.params_type_iter().collect::<Vec<ValType>>();
        assert_eq!(param_types, [ValType::I32]);

        // check the return types
        let return_types = ty.returns_type_iter().collect::<Vec<ValType>>();
        assert_eq!(return_types, [ValType::I32]);

        Ok(())
    }

    fn real_add(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
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
