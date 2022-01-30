//! Defines WasmEdge Executor.

use super::wasmedge;
use crate::{
    error::{check, WasmEdgeError, WasmEdgeResult},
    types::WasmEdgeString,
    Config, ImportObject, Module, Statistics, Store, Value,
};

/// Struct of WasmEdge Executor.
///
/// [`Executor`] defines an execution environment for both WASM and compiled WASM. It works based on the
/// [Store](crate::Store).
#[derive(Debug)]
pub struct Executor {
    ctx: *mut wasmedge::WasmEdge_ExecutorContext,
}
impl Executor {
    /// Creates a new [`Executor`] to be associated with the given [`Config`] and [`Statistics`].
    ///
    /// # Arguments
    ///
    /// - `config` specifies the configuration of the new [`Executor`]. It is optional.
    ///
    /// - `stat` specifies the [`Statistics`] needed by the new [`Executor`]. It is optional.
    ///
    /// # Error
    ///
    /// If fail to create a [`Executor`], then an error is returned.
    pub fn create(config: Option<Config>, stat: Option<Statistics>) -> WasmEdgeResult<Self> {
        let ctx = match config {
            Some(mut config) => match stat {
                Some(mut stat) => {
                    let ctx = unsafe { wasmedge::WasmEdge_ExecutorCreate(config.ctx, stat.ctx) };
                    config.ctx = std::ptr::null_mut();
                    stat.ctx = std::ptr::null_mut();
                    ctx
                }
                None => {
                    let ctx = unsafe {
                        wasmedge::WasmEdge_ExecutorCreate(config.ctx, std::ptr::null_mut())
                    };
                    config.ctx = std::ptr::null_mut();
                    ctx
                }
            },
            None => match stat {
                Some(mut stat) => {
                    let ctx = unsafe {
                        wasmedge::WasmEdge_ExecutorCreate(std::ptr::null_mut(), stat.ctx)
                    };
                    stat.ctx = std::ptr::null_mut();
                    ctx
                }
                None => unsafe {
                    wasmedge::WasmEdge_ExecutorCreate(std::ptr::null_mut(), std::ptr::null_mut())
                },
            },
        };

        match ctx.is_null() {
            true => Err(WasmEdgeError::ExecutorCreate),
            false => Ok(Executor { ctx }),
        }
    }

    /// Registers and instantiates a WasmEdge [`ImportObject`] into a [`Store`].
    ///
    /// # Arguments
    ///
    /// - `store` specifies the target [`Store`], into which the given [`ImportObject`] is registered.
    ///
    /// - `import` specifies the WasmEdge [`ImportObject`] to be registered.
    ///
    /// # Error
    ///
    /// If fail to register the given [`ImportObject`], then an error is returned.
    pub fn register_import_object(
        self,
        store: &mut Store,
        mut import: ImportObject,
    ) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorRegisterImport(
                self.ctx, store.ctx, import.ctx,
            ))?;
        }
        import.ctx = std::ptr::null_mut();
        Ok(self)
    }

    /// Registers and instantiates a WasmEdge AST [`Module`] into a store.
    ///
    /// Instantiates the instances in a WasmEdge AST [`Module`], and then registers the [`Module`] into
    /// a [`Store`] with their exported names and the given [`Module`] name.
    ///
    /// # Arguments
    ///
    /// - `store` specifies the target [`Store`], into which the given [`Module`] is registered.
    ///
    /// - `ast_mod` specifies the AST [`Module`] to be registered.
    ///
    /// - `mod_name` specifies the [`Module`] name for all exported instances.
    ///
    /// # Error
    ///
    /// If fail to register the given [`Module`], then an error is returned.
    pub fn register_module(
        self,
        store: &mut Store,
        mut module: Module,
        mod_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Self> {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_ExecutorRegisterModule(
                self.ctx,
                store.ctx,
                module.ctx,
                mod_name.as_raw(),
            ))?;
            module.ctx = std::ptr::null_mut();
        }
        Ok(self)
    }

    /// Instantiates a WasmEdge AST [Module](crate::Module) into a [Store](crate::Store).
    ///
    /// Instantiates the WasmEdge AST [Module](crate::Module) as an active anonymous module in the
    /// [Store](crate::Store). Notice that when a new module is instantiated into the [Store](crate::Store), the old
    /// instantiated module is removed; in addition, ensure that the [imports](crate::ImportObject) are registered into
    /// the [Store](crate::Store).
    ///
    ///
    /// # Arguments
    ///
    /// - `store` specifies the [Store](crate::Store), in which the [Module](crate::Module) to be instantiated
    /// is stored.
    ///
    /// - `ast_mod` specifies the target [Module](crate::Module) to be instantiated.
    ///
    /// # Error
    ///
    /// If fail to instantiate the given [Module](crate::Module), then an error is returned.
    pub fn instantiate(self, store: &mut Store, mut module: Module) -> WasmEdgeResult<Self> {
        unsafe {
            check(wasmedge::WasmEdge_ExecutorInstantiate(
                self.ctx, store.ctx, module.ctx,
            ))?;
        }
        module.ctx = std::ptr::null_mut();
        Ok(self)
    }

    /// Invokes a WASM function in the anonymous [`Module`], and returns the results.
    ///
    /// After instantiating a WasmEdge [`Module`], the [`Module`] is registered as an
    /// anonymous module in the [`Store`]; then, you can repeatedly call this function
    /// to invoke exported WASM functions by their names until the [`Store`] is reset or
    /// a new [`Module`] is registered or instantiated.
    ///
    /// For calling the functions in a registered [`Module`], reference `invoke_registered_function`.
    ///
    /// # Arguments
    ///
    /// - `store` specifies the target [`Store`] which owns the target function specified by `func_name`.
    ///
    /// - `func_name` specifies the name of the target function, which is stored in an anonymous module in `store`.
    ///
    /// - `params` specifies the argument values for the target function.
    ///
    /// # Error
    ///
    /// If fail to invoke the function specified by `func_name`, then an error is returned.
    pub fn run_func(
        &self,
        store: &Store,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        store.contains_func(func_name.as_ref())?;

        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let returns_len = store.find_func(func_name.as_ref())?.ty()?.returns_len();
        let mut returns = Vec::with_capacity(returns_len);

        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_ExecutorInvoke(
                self.ctx,
                store.ctx,
                func_name.as_raw(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len as u32,
            ))?;
            returns.set_len(returns_len);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }

    /// Invokes a registered WASM function by its module name and function name, and returns the results.
    ///
    /// # Arguments
    ///
    /// - `store` specifies the target [`Store`] which owns the module and the target function.
    ///
    /// - `mod_name` specifies the name of the registered module.
    ///
    /// - `func_name` specifies the name of the target function.
    ///
    /// - `params` specifies the argument values for the target function.
    ///
    /// # Error
    ///
    /// If fail to invoke the target registered function, then an error is returned.
    ///
    pub fn run_func_registered(
        &self,
        store: &Store,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
        params: impl IntoIterator<Item = Value>,
    ) -> WasmEdgeResult<Vec<Value>> {
        store.contains_reg_func(mod_name.as_ref(), func_name.as_ref())?;

        let raw_params = params.into_iter().map(|x| x.as_raw()).collect::<Vec<_>>();

        // get the length of the function's returns
        let returns_len = store
            .find_func_registered(mod_name.as_ref(), func_name.as_ref())?
            .ty()?
            .returns_len();
        let mut returns = Vec::with_capacity(returns_len);

        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        let func_name: WasmEdgeString = func_name.as_ref().into();
        unsafe {
            check(wasmedge::WasmEdge_ExecutorInvokeRegistered(
                self.ctx,
                store.ctx,
                mod_name.as_raw(),
                func_name.as_raw(),
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len as u32,
            ))?;
            returns.set_len(returns_len);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }
}
impl Drop for Executor {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ExecutorDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Config, Statistics};

    #[test]
    fn test_executor_create() {
        {
            // create an Executor context without configuration and statistics
            let result = Executor::create(None, None);
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.ctx.is_null());
        }

        {
            // create an Executor context with a given configuration
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let result = Executor::create(Some(config), None);
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.ctx.is_null());
        }

        {
            // create an Executor context with a given statistics
            let result = Statistics::create();
            assert!(result.is_ok());
            let stat = result.unwrap();
            let result = Executor::create(None, Some(stat));
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.ctx.is_null());
        }

        {
            // create an Executor context with the given configuration and statistics.
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();

            let result = Statistics::create();
            assert!(result.is_ok());
            let stat = result.unwrap();

            let result = Executor::create(Some(config), Some(stat));
            assert!(result.is_ok());
            let executor = result.unwrap();
            assert!(!executor.ctx.is_null());
        }
    }
}
