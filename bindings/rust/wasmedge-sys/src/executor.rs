//! Defines WasmEdge Executor.

use super::ffi;
use crate::{
    error::{check, WasmEdgeError},
    instance::module::InnerInstance,
    types::WasmEdgeString,
    Config, Function, ImportObject, Instance, Module, Statistics, Store, WasmEdgeResult, WasmValue,
};

/// Struct of WasmEdge Executor.
///
/// [Executor] defines an execution environment for both WASM and compiled WASM. It works with the
/// [Store](crate::Store).
#[derive(Debug)]
pub struct Executor {
    pub(crate) inner: InnerExecutor,
    pub(crate) registered: bool,
}
impl Executor {
    /// Creates a new [executor](crate::Executor) to be associated with the given [config](crate::Config) and [statistics](crate::Statistics).
    ///
    /// # Arguments
    ///
    /// - `config` specifies the configuration of the new [executor](crate::Executor).
    ///
    /// - `stat` specifies the [statistics](crate::Statistics) needed by the new [executor](crate::Executor).
    ///
    /// # Error
    ///
    /// If fail to create a [executor](crate::Executor), then an error is returned.
    pub fn create(config: Option<Config>, stat: Option<&mut Statistics>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => match stat {
                Some(stat) => {
                    let ctx = unsafe { ffi::WasmEdge_ExecutorCreate(config.inner.0, stat.inner.0) };
                    config.inner.0 = std::ptr::null_mut();
                    ctx
                }
                None => {
                    let ctx = unsafe {
                        ffi::WasmEdge_ExecutorCreate(config.inner.0, std::ptr::null_mut())
                    };
                    config.inner.0 = std::ptr::null_mut();
                    ctx
                }
            },
            None => match stat {
                Some(stat) => unsafe {
                    ffi::WasmEdge_ExecutorCreate(std::ptr::null_mut(), stat.inner.0)
                },
                None => unsafe {
                    ffi::WasmEdge_ExecutorCreate(std::ptr::null_mut(), std::ptr::null_mut())
                },
            },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::ExecutorCreate),
            false => Ok(Executor {
                inner: InnerExecutor(ctx),
                registered: false,
            }),
        }
    }

    /// Registers and instantiates a WasmEdge [import object](crate::ImportObject) into a [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// - `store` specifies the target [store](crate::Store), into which the given [import object](crate::ImportObject) is registered.
    ///
    /// - `import` specifies the WasmEdge [import object](crate::ImportObject) to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [import object](crate::ImportObject), then an error is returned.
    pub fn register_import_object(
        &mut self,
        store: &mut Store,
        import: &ImportObject,
    ) -> WasmEdgeResult<()> {
        match import {
            ImportObject::Import(import) => unsafe {
                check(ffi::WasmEdge_ExecutorRegisterImport(
                    self.inner.0,
                    store.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            ImportObject::Wasi(import) => unsafe {
                check(ffi::WasmEdge_ExecutorRegisterImport(
                    self.inner.0,
                    store.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            ImportObject::WasmEdgeProcess(import) => unsafe {
                check(ffi::WasmEdge_ExecutorRegisterImport(
                    self.inner.0,
                    store.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
        }
        // unsafe {
        //     check(ffi::WasmEdge_ExecutorRegisterImport(
        //         self.inner.0,
        //         store.inner.0,
        //         import.inner.0 as *const _,
        //     ))?;
        // }
        Ok(())
    }

    /// Registers and instantiates a WasmEdge [module](crate::Module) into a store.
    ///
    /// Instantiates the given WasmEdge [module](crate::Module), including the [functions](crate::Function), [memories](crate::Memory), [tables](crate::Table), and [globals](crate::Global) it hosts; and then, registers the module [instance](crate::Instance) into the [store](crate::Store) with the given name.
    ///
    /// # Arguments
    ///
    /// - `store` - The target [store](crate::Store), into which the given [module](crate::Module) is registered.
    ///
    /// - `module` - A validated [module](crate::Module) to be registered.
    ///
    /// - `name` - The exported name of the registered [module](crate::Module).
    ///
    /// # Error
    ///
    /// If fail to register the given [module](crate::Module), then an error is returned.
    pub fn register_named_module(
        &mut self,
        store: &mut Store,
        module: &Module,
        name: impl AsRef<str>,
    ) -> WasmEdgeResult<Instance> {
        let mut instance_ctx = Vec::with_capacity(1);
        let mod_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_ExecutorRegister(
                self.inner.0,
                instance_ctx.as_mut_ptr(),
                store.inner.0,
                module.inner.0 as *const _,
                mod_name.as_raw(),
            ))?;
        }
        Ok(Instance {
            inner: InnerInstance(instance_ctx.remove(0)),
            registered: false,
        })
    }

    /// Registers and instantiates a WasmEdge [module](crate::Module) into a [store](crate::Store) as an anonymous module.
    ///
    /// Notice that when a new module is instantiated into the [store](crate::Store), the old instantiated module is removed; in addition, ensure that the [imports](crate::ImportObject) the module depends are already registered into the [store](crate::Store).
    ///
    ///
    /// # Arguments
    ///
    /// - `store` specifies the [store](crate::Store), in which the [module](crate::Module) to be instantiated
    /// is stored.
    ///
    /// - `ast_mod` specifies the target [module](crate::Module) to be instantiated.
    ///
    /// # Error
    ///
    /// If fail to instantiate the given [module](crate::Module), then an error is returned.
    pub fn register_active_module(
        &mut self,
        store: &mut Store,
        module: &Module,
    ) -> WasmEdgeResult<Instance> {
        let mut instance_ctx = Vec::with_capacity(1);
        unsafe {
            check(ffi::WasmEdge_ExecutorInstantiate(
                self.inner.0,
                instance_ctx.as_mut_ptr(),
                store.inner.0,
                module.inner.0 as *const _,
            ))?;
        }
        Ok(Instance {
            inner: InnerInstance(instance_ctx.remove(0)),
            registered: false,
        })
    }

    /// Invokes a WASM function and returns the results.
    ///
    /// # Arguments
    ///
    /// - `func` - The name of the target function.
    ///
    /// - `params` specifies the argument values for the target function.
    ///
    /// # Error
    ///
    /// If fail to invoke the function, then an error is returned.
    pub fn run(
        &mut self,
        func: &Function,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let func_ty = func.ty()?;
        let returns_len = func_ty.returns_len();
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(ffi::WasmEdge_ExecutorInvoke(
                self.inner.0,
                func.inner.0 as *const _,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }
}
impl Drop for Executor {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ExecutorDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerExecutor(pub(crate) *mut ffi::WasmEdge_ExecutorContext);
unsafe impl Send for InnerExecutor {}
unsafe impl Sync for InnerExecutor {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        AddImportInstance, Config, FuncType, Function, Global, GlobalType, ImportModule, MemType,
        Memory, Statistics, Table, TableType,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{Mutability, RefType, ValType};

    #[test]
    fn test_executor_create() {
        {
            // create an Executor context without configuration and statistics
            let result = Executor::create(None, None);
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.inner.0.is_null());
        }

        {
            // create an Executor context with a given configuration
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let result = Executor::create(Some(config), None);
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.inner.0.is_null());
        }

        {
            // create an Executor context with a given statistics
            let result = Statistics::create();
            assert!(result.is_ok());
            let mut stat = result.unwrap();
            let result = Executor::create(None, Some(&mut stat));
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.inner.0.is_null());
        }

        {
            // create an Executor context with the given configuration and statistics.
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Statistics::create();
            assert!(result.is_ok());
            let mut stat = result.unwrap();

            let result = Executor::create(Some(config), Some(&mut stat));
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.inner.0.is_null());
        }
    }

    #[test]
    fn test_executor_register_import() {
        // create an Executor
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        assert!(!executor.inner.0.is_null());

        // create a Store
        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let host_name = "extern";

        // create an ImportObj module
        let result = ImportModule::create(host_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function "func-add": (externref, i32) -> (i32)
        let result = FuncType::create([ValType::ExternRef, ValType::I32], [ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        // add the function into the import_obj module
        import.add_func("func-add", host_func);

        // create a Table instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        let result = Table::create(&table_ty);
        assert!(result.is_ok());
        let host_table = result.unwrap();
        // add the table into the import_obj module
        import.add_table("table", host_table);

        // create a Memory instance
        let result = MemType::create(1..=2);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        let result = Memory::create(&mem_ty);
        assert!(result.is_ok());
        let host_memory = result.unwrap();
        // add the memory into the import_obj module
        import.add_memory("memory", host_memory);

        // create a Global instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        let result = Global::create(&global_ty, WasmValue::from_i32(666));
        assert!(result.is_ok());
        let host_global = result.unwrap();
        // add the global into import_obj module
        import.add_global("global_i32", host_global);

        let import = ImportObject::Import(import);
        let result = executor.register_import_object(&mut store, &import);
        assert!(result.is_ok());

        {
            let result = store.module("extern");
            assert!(result.is_ok());
            let instance = result.unwrap();

            let result = instance.get_global("global_i32");
            assert!(result.is_ok());
            let global = result.unwrap();
            assert_eq!(global.get_value().to_i32(), 666);
        }

        let handle = thread::spawn(move || {
            let result = store.module("extern");
            assert!(result.is_ok());
            let instance = result.unwrap();

            let result = instance.get_global("global_i32");
            assert!(result.is_ok());
            let global = result.unwrap();
            assert_eq!(global.get_value().to_i32(), 666);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_executor_send() {
        // create an Executor context with the given configuration and statistics.
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::create(Some(config), Some(&mut stat));
        assert!(result.is_ok());
        let executor = result.unwrap();
        assert!(!executor.inner.0.is_null());

        let handle = thread::spawn(move || {
            assert!(!executor.inner.0.is_null());
            println!("{:?}", executor.inner);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_executor_sync() {
        // create an Executor context with the given configuration and statistics.
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::create(Some(config), Some(&mut stat));
        assert!(result.is_ok());
        let executor = Arc::new(Mutex::new(result.unwrap()));

        let executor_cloned = Arc::clone(&executor);
        let handle = thread::spawn(move || {
            let result = executor_cloned.lock();
            assert!(result.is_ok());
            let executor = result.unwrap();

            assert!(!executor.inner.0.is_null());
        });

        handle.join().unwrap();
    }

    fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
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

        Ok(vec![WasmValue::from_i32(c)])
    }
}
