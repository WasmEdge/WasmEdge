//! Defines WasmEdge Executor.

use super::ffi;
#[cfg(all(feature = "async", target_os = "linux"))]
use crate::r#async::{AsyncState, FiberFuture};
use crate::{
    error::WasmEdgeError, instance::module::InnerInstance, types::WasmEdgeString, utils::check,
    Config, Engine, FuncRef, Function, ImportObject, Instance, Module, Statistics, Store,
    WasmEdgeResult, WasmValue,
};
use std::sync::Arc;

/// Defines an execution environment for both pure WASM and compiled WASM.
#[derive(Debug, Clone)]
pub struct Executor {
    pub(crate) inner: Arc<InnerExecutor>,
    pub(crate) registered: bool,
}
impl Executor {
    /// Creates a new [executor](crate::Executor) to be associated with the given [config](crate::Config) and [statistics](crate::Statistics).
    ///
    /// # Arguments
    ///
    /// * `config` - The configuration of the new [executor](crate::Executor).
    ///
    /// * `stat` - The [statistics](crate::Statistics) needed by the new [executor](crate::Executor).
    ///
    /// # Error
    ///
    /// If fail to create a [executor](crate::Executor), then an error is returned.
    pub fn create(config: Option<&Config>, stat: Option<&mut Statistics>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(config) => match stat {
                Some(stat) => unsafe { ffi::WasmEdge_ExecutorCreate(config.inner.0, stat.inner.0) },
                None => unsafe {
                    ffi::WasmEdge_ExecutorCreate(config.inner.0, std::ptr::null_mut())
                },
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
            true => Err(Box::new(WasmEdgeError::ExecutorCreate)),
            false => Ok(Executor {
                inner: Arc::new(InnerExecutor(ctx)),
                registered: false,
            }),
        }
    }

    /// Registers and instantiates a WasmEdge [import object](crate::ImportObject) into a [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// * `store` - The target [store](crate::Store), into which the given [import object](crate::ImportObject) is registered.
    ///
    /// * `import` - The WasmEdge [import object](crate::ImportObject) to be registered.
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
            #[cfg(not(feature = "async"))]
            ImportObject::Wasi(import) => unsafe {
                check(ffi::WasmEdge_ExecutorRegisterImport(
                    self.inner.0,
                    store.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
            #[cfg(all(feature = "async", target_os = "linux"))]
            ImportObject::AsyncWasi(import) => unsafe {
                check(ffi::WasmEdge_ExecutorRegisterImport(
                    self.inner.0,
                    store.inner.0,
                    import.inner.0 as *const _,
                ))?;
            },
        }

        Ok(())
    }

    /// Registers and instantiates a WasmEdge [module](crate::Module) into a store.
    ///
    /// Instantiates the given WasmEdge [module](crate::Module), including the [functions](crate::Function), [memories](crate::Memory), [tables](crate::Table), and [globals](crate::Global) it hosts; and then, registers the module [instance](crate::Instance) into the [store](crate::Store) with the given name.
    ///
    /// # Arguments
    ///
    /// * `store` - The target [store](crate::Store), into which the given [module](crate::Module) is registered.
    ///
    /// * `module` - A validated [module](crate::Module) to be registered.
    ///
    /// * `name` - The exported name of the registered [module](crate::Module).
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
        let mut instance_ctx = std::ptr::null_mut();
        let mod_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            check(ffi::WasmEdge_ExecutorRegister(
                self.inner.0,
                &mut instance_ctx,
                store.inner.0,
                module.inner.0 as *const _,
                mod_name.as_raw(),
            ))?;
        }

        Ok(Instance {
            inner: std::sync::Arc::new(InnerInstance(instance_ctx)),
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
    /// * `store` - The [store](crate::Store), in which the [module](crate::Module) to be instantiated
    /// is stored.
    ///
    /// * `ast_mod` - The target [module](crate::Module) to be instantiated.
    ///
    /// # Error
    ///
    /// If fail to instantiate the given [module](crate::Module), then an error is returned.
    pub fn register_active_module(
        &mut self,
        store: &mut Store,
        module: &Module,
    ) -> WasmEdgeResult<Instance> {
        let mut instance_ctx = std::ptr::null_mut();
        unsafe {
            check(ffi::WasmEdge_ExecutorInstantiate(
                self.inner.0,
                &mut instance_ctx,
                store.inner.0,
                module.inner.0 as *const _,
            ))?;
        }
        Ok(Instance {
            inner: std::sync::Arc::new(InnerInstance(instance_ctx)),
            registered: false,
        })
    }

    /// Registers plugin module instance into a [store](crate::Store).
    ///
    /// # Arguments
    ///
    /// * `store` - The [store](crate::Store), in which the [module](crate::Module) to be instantiated
    /// is stored.
    ///
    /// * `instance` - The plugin module instance to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given plugin module instance, then an error is returned.
    pub fn register_plugin_instance(
        &mut self,
        store: &mut Store,
        instance: &Instance,
    ) -> WasmEdgeResult<()> {
        unsafe {
            check(ffi::WasmEdge_ExecutorRegisterImport(
                self.inner.0,
                store.inner.0,
                instance.inner.0 as *const _,
            ))?;
        }

        Ok(())
    }

    /// Runs a host function instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func` - The function instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    pub fn call_func(
        &self,
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

    /// Asynchronously runs a host function instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func` - The function instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Errors
    ///
    /// If fail to run the host function, then an error is returned.
    #[cfg(all(feature = "async", target_os = "linux"))]
    pub async fn call_func_async(
        &self,
        async_state: &AsyncState,
        func: &Function,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(async_state, || self.call_func(func, params))
            .await
            .unwrap()
    }

    /// Runs a host function reference instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func_ref` - The function reference instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Errors
    ///
    /// If fail to run the host function reference instance, then an error is returned.
    pub fn call_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let func_ty = func_ref.ty()?;
        let returns_len = func_ty.returns_len();
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(ffi::WasmEdge_ExecutorInvoke(
                self.inner.0,
                func_ref.inner.0 as *const _,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }

    /// Asynchronously runs a host function reference instance and returns the results.
    ///
    /// # Arguments
    ///
    /// * `func_ref` - The function reference instance to run.
    ///
    /// * `params` - The arguments to pass to the function.
    ///
    /// # Errors
    ///
    /// If fail to run the host function reference instance, then an error is returned.
    #[cfg(all(feature = "async", target_os = "linux"))]
    pub async fn call_func_ref_async(
        &self,
        async_state: &AsyncState,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(async_state, || self.call_func_ref(func_ref, params))
            .await
            .unwrap()
    }

    /// Provides a raw pointer to the inner Executor context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_ExecutorContext {
        self.inner.0 as *const _
    }
}
impl Drop for Executor {
    fn drop(&mut self) {
        if !self.registered && Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ExecutorDelete(self.inner.0) }
        }
    }
}
impl Engine for Executor {
    fn run_func(
        &self,
        func: &Function,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        self.call_func(func, params)
    }

    fn run_func_ref(
        &self,
        func_ref: &FuncRef,
        params: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        self.call_func_ref(func_ref, params)
    }
}

#[derive(Debug)]
pub(crate) struct InnerExecutor(pub(crate) *mut ffi::WasmEdge_ExecutorContext);
unsafe impl Send for InnerExecutor {}
unsafe impl Sync for InnerExecutor {}

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(all(feature = "async", target_os = "linux"))]
    use crate::{instance::module::AsyncWasiModule, Loader, Validator};
    use crate::{
        AsImport, CallingFrame, Config, FuncType, Function, Global, GlobalType, ImportModule,
        MemType, Memory, Statistics, Table, TableType,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    #[cfg(all(feature = "async", target_os = "linux"))]
    use wasmedge_async_wasi::snapshots::WasiCtx;
    #[cfg(all(feature = "async", target_os = "linux"))]
    use wasmedge_macro::sys_async_host_function_new;
    use wasmedge_macro::sys_host_function;
    use wasmedge_types::{error::HostFuncError, Mutability, NeverType, RefType, ValType};

    #[test]
    #[allow(clippy::assertions_on_result_states)]
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
            let result = Executor::create(Some(&config), None);
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

            let result = Executor::create(Some(&config), Some(&mut stat));
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.inner.0.is_null());
        }
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
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
        let result = Function::create::<NeverType>(&func_ty, real_add, None, 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        // add the function into the import_obj module
        import.add_func("func-add", host_func);

        // create a Table instance
        let result = TableType::create(RefType::FuncRef, 10, Some(20));
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        let result = Table::create(&table_ty);
        assert!(result.is_ok());
        let host_table = result.unwrap();
        // add the table into the import_obj module
        import.add_table("table", host_table);

        // create a Memory instance
        let result = MemType::create(1, Some(2), false);
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
    #[allow(clippy::assertions_on_result_states)]
    fn test_executor_send() {
        // create an Executor context with the given configuration and statistics.
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::create(Some(&config), Some(&mut stat));
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
    #[allow(clippy::assertions_on_result_states)]
    fn test_executor_sync() {
        // create an Executor context with the given configuration and statistics.
        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::create();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::create(Some(&config), Some(&mut stat));
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

    #[cfg(all(feature = "async", target_os = "linux"))]
    #[tokio::test]
    async fn test_executor_register_async_wasi() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config
        let mut config = Config::create()?;
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create an Executor
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        assert!(!executor.inner.0.is_null());

        // create a Store
        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // create async wasi context
        let mut async_wasi_ctx = WasiCtx::new();
        async_wasi_ctx.push_arg("abc".into());
        async_wasi_ctx.push_env("a=1".into());

        // create an AsyncWasiModule
        let result = AsyncWasiModule::create(Some(&mut async_wasi_ctx));
        assert!(result.is_ok());
        let async_wasi_module = result.unwrap();

        // register async_wasi module into the store
        let wasi_import = ImportObject::AsyncWasi(async_wasi_module);
        let result = executor.register_import_object(&mut store, &wasi_import);
        assert!(result.is_ok());

        let module = Loader::create(None)?.from_file("examples/async_hello.wasm")?;
        Validator::create(None)?.validate(&module)?;
        let instance = executor.register_active_module(&mut store, &module)?;
        let fn_start = instance.get_func("_start")?;

        async fn tick() {
            let mut i = 0;
            loop {
                println!("[tick] i={i}");
                tokio::time::sleep(std::time::Duration::from_millis(500)).await;
                i += 1;
            }
        }
        tokio::spawn(tick());

        let async_state = AsyncState::new();
        let _ = executor
            .call_func_async(&async_state, &fn_start, [])
            .await?;

        Ok(())
    }

    #[cfg(all(feature = "async", target_os = "linux"))]
    #[tokio::test]
    async fn test_executor_run_async_host_func() -> Result<(), Box<dyn std::error::Error>> {
        // create a Config
        let mut config = Config::create()?;
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create an Executor
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        assert!(!executor.inner.0.is_null());

        // create a Store
        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // create async wasi context
        let mut async_wasi_ctx = WasiCtx::new();
        async_wasi_ctx.push_arg("abc".into());
        async_wasi_ctx.push_env("a=1".into());

        // create an AsyncWasiModule
        let result = AsyncWasiModule::create(Some(&mut async_wasi_ctx));
        assert!(result.is_ok());
        let async_wasi_module = result.unwrap();

        // register async_wasi module into the store
        let wasi_import = ImportObject::AsyncWasi(async_wasi_module);
        let result = executor.register_import_object(&mut store, &wasi_import);
        assert!(result.is_ok());

        // todo
        let ty = FuncType::create([], [])?;
        let async_hello_func = Function::create_async::<NeverType>(&ty, async_hello, None, 0)?;
        let mut import = ImportModule::create("extern")?;
        import.add_func("async_hello", async_hello_func);

        let extern_import = ImportObject::Import(import);
        executor.register_import_object(&mut store, &extern_import)?;

        let extern_instance = store.module("extern")?;
        let async_hello = extern_instance.get_func("async_hello")?;

        async fn tick() {
            let mut i = 0;
            loop {
                println!("[tick] i={i}");
                tokio::time::sleep(std::time::Duration::from_millis(500)).await;
                i += 1;
            }
        }
        tokio::spawn(tick());

        let async_state = AsyncState::new();
        let _ = executor
            .call_func_async(&async_state, &async_hello, [])
            .await?;

        Ok(())
    }

    #[sys_host_function]
    fn real_add<T>(
        _frame: CallingFrame,
        inputs: Vec<WasmValue>,
        _: Option<&mut T>,
    ) -> Result<Vec<WasmValue>, HostFuncError> {
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

    #[cfg(all(feature = "async", target_os = "linux"))]
    #[sys_async_host_function_new]
    async fn async_hello<T>(
        _frame: CallingFrame,
        _inputs: Vec<WasmValue>,
        _data: Option<&mut T>,
    ) -> Result<Vec<WasmValue>, HostFuncError> {
        for _ in 0..10 {
            println!("[async hello] say hello");
            tokio::time::sleep(std::time::Duration::from_secs(1)).await;
        }

        println!("[async hello] Done!");

        Ok(vec![])
    }
}
