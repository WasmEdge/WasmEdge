//! Defines Func, SignatureBuilder, and Signature structs.
use crate::{io::WasmValTypeList, Engine, HostFunc, WasmEdgeResult};
use wasmedge_sys::{self as sys, WasmValue};
use wasmedge_types::{FuncType, ValType};

/// Defines a host function.
///
/// A WasmEdge [Func] represents a host function. A host function is a function defined outside WASM module and passed to it.
///
/// # Example
///
/// The following example shows how to create a host function, access it by its name and its type info.
///
/// ```rust
/// #![feature(explicit_generic_args_with_impl_trait)]
///
/// use wasmedge::{ImportObjectBuilder, config::{ConfigBuilder, CommonConfigOptions}, Statistics, Executor, Store, FuncTypeBuilder};
/// use wasmedge_sys::types::WasmValue;
/// use wasmedge_types::ValType;
///
/// // a function to be exported as host function
/// fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
///     if inputs.len() != 2 {
///         return Err(1);
///     }
///
///     let a = if inputs[0].ty() == ValType::I32 {
///         inputs[0].to_i32()
///     } else {
///         return Err(2);
///     };
///
///     let b = if inputs[1].ty() == ValType::I32 {
///         inputs[1].to_i32()
///     } else {
///         return Err(3);
///     };
///
///     let c = a + b;
///
///     Ok(vec![WasmValue::from_i32(c)])
/// }
///
/// // create an ImportModule which has a host function with an exported name "add"
/// let result = ImportObjectBuilder::new()
/// .with_func::<(i32, i32), i32>(
///     "add",
///     Box::new(real_add),
/// )
/// .expect("failed to add host func")
/// .build("extern");
/// assert!(result.is_ok());
/// let import = result.unwrap();
///
/// // create an executor
/// let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
/// assert!(result.is_ok());
/// let config = result.unwrap();
///
/// let result = Statistics::new();
/// assert!(result.is_ok());
/// let mut stat = result.unwrap();
///
/// let result = Executor::new(Some(&config), Some(&mut stat));
/// assert!(result.is_ok());
/// let mut executor = result.unwrap();
///
/// // create a store
/// let result = Store::new();
/// assert!(result.is_ok());
/// let mut store = result.unwrap();
///
/// // register the import module into the store
/// let result = store.register_import_module(&mut executor, &import);
/// assert!(result.is_ok());
///
/// // get the instance of the ImportObject module
/// let result = store.module_instance("extern");
/// assert!(result.is_some());
/// let instance = result.unwrap();
///
/// // get the exported host function
/// let result = instance.func("add");
/// assert!(result.is_some());
/// let host_func = result.unwrap();
///
/// // check the signature of the host function
/// let result = host_func.ty();
/// assert!(result.is_ok());
/// let signature = result.unwrap();
/// assert!(signature.args().is_some());
/// assert_eq!(signature.args().unwrap(), [ValType::I32; 2]);
/// assert!(signature.returns().is_some());
/// assert_eq!(signature.returns().unwrap(), [ValType::I32]);
///
/// ```
#[derive(Debug)]
pub struct Func {
    pub(crate) inner: sys::Function,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Func {
    /// Creates a host function by wrapping a native function.
    ///
    /// # Arguments
    ///
    /// * `real_func` - The native function to be wrapped.
    ///
    /// # Error
    ///
    /// If fail to create the host function, then an error is returned.
    pub fn wrap<Args, Rets>(real_func: HostFunc) -> WasmEdgeResult<Self>
    where
        Args: WasmValTypeList,
        Rets: WasmValTypeList,
    {
        let args = Args::wasm_types();
        let returns = Rets::wasm_types();
        let ty = FuncType::new(Some(args.to_vec()), Some(returns.to_vec()));
        let inner = sys::Function::create(&ty.into(), real_func, 0)?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Returns the exported name of this function.
    ///
    /// Notice that this field is meaningful only if this host function is used as an exported instance.
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this function exports.
    ///
    /// Notice that this field is meaningful only if this host function is used as an exported instance.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of the host function.
    ///
    /// If fail to get the signature, then an error is returned.
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }

    /// Returns a reference to this function instance.
    pub fn as_ref(&self) -> FuncRef {
        let inner = self.inner.as_ref();
        FuncRef { inner }
    }

    /// Runs this host function and returns the result.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implements Engine trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    /// # Example
    ///
    /// ```rust
    /// use wasmedge_sys::{FuncType, Function, WasmValue, Executor};
    /// use wasmedge_types::ValType;
    ///
    /// fn real_add(input: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
    ///     println!("Rust: Entering Rust function real_add");
    ///
    ///     if input.len() != 2 {
    ///         return Err(1);
    ///     }
    ///
    ///     let a = if input[0].ty() == ValType::I32 {
    ///         input[0].to_i32()
    ///     } else {
    ///         return Err(2);
    ///     };
    ///
    ///     let b = if input[1].ty() == ValType::I32 {
    ///         input[1].to_i32()
    ///     } else {
    ///         return Err(3);
    ///     };
    ///
    ///     let c = a + b;
    ///     println!("Rust: calcuating in real_add c: {:?}", c);
    ///
    ///     println!("Rust: Leaving Rust function real_add");
    ///     Ok(vec![WasmValue::from_i32(c)])
    /// }
    ///
    /// // create a FuncType
    /// let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
    /// assert!(result.is_ok());
    /// let func_ty = result.unwrap();
    /// // create a host function
    /// let result = Function::create(&func_ty, Box::new(real_add), 0);
    /// assert!(result.is_ok());
    /// let host_func = result.unwrap();
    ///
    /// // create an Executor instance
    /// let result = Executor::create(None, None);
    /// assert!(result.is_ok());
    /// let mut executor = result.unwrap();
    ///
    /// // run this function
    /// let result = host_func.call(
    ///     &mut executor,
    ///     vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
    /// );
    /// assert!(result.is_ok());
    /// let returns = result.unwrap();
    /// assert_eq!(returns[0].to_i32(), 3);
    /// ```
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func(self, args)
    }
}

/// Defines a type builder for creating a [FuncType](wasmedge_types::FuncType).
#[derive(Debug, Default)]
pub struct FuncTypeBuilder {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl FuncTypeBuilder {
    /// Creates a new [FuncTypeBuilder].
    pub fn new() -> Self {
        Self {
            args: None,
            returns: None,
        }
    }

    /// Adds arguments to the function type.
    ///
    /// # Argument
    ///
    /// `args` specifies the arguments to be added to the function type.
    pub fn with_args(self, args: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: Some(args.into_iter().collect::<Vec<_>>()),
            returns: self.returns,
        }
    }

    /// Adds a single argument to the function type.
    ///
    /// # Argument
    ///
    /// `arg` specifies the argument to be added to the function type.
    pub fn with_arg(self, arg: ValType) -> Self {
        self.with_args(std::iter::once(arg))
    }

    /// Adds returns to the function type.
    ///
    /// # Argument
    ///
    /// `returns` specifies the returns to be added to the function type.
    pub fn with_returns(self, returns: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: self.args,
            returns: Some(returns.into_iter().collect::<Vec<_>>()),
        }
    }

    /// Adds a single return to the function type.
    ///
    /// # Argument
    ///
    /// `ret` specifies the return to be added to the function type.
    pub fn with_return(self, ret: ValType) -> Self {
        self.with_returns(std::iter::once(ret))
    }

    /// Returns a function type.
    pub fn build(self) -> FuncType {
        FuncType::new(self.args, self.returns)
    }
}

/// Defines a reference to a [host function](crate::Func).
#[derive(Debug, Clone)]
pub struct FuncRef {
    pub(crate) inner: sys::FuncRef,
}
impl FuncRef {
    /// Returns the underlying wasm type of the host function this [FuncRef] points to.
    ///
    /// # Errors
    ///
    /// If fail to get the function type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let ty = self.inner.ty()?;
        Ok(ty.into())
    }

    /// Runs this host function the reference refers to.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implements Engine trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func_ref(self, args)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        Executor, ImportObjectBuilder, Statistics, Store,
    };
    use wasmedge_sys::WasmValue;

    #[test]
    fn test_func_signature() {
        // test signature with args and returns
        {
            let sig = FuncTypeBuilder::new()
                .with_args(vec![
                    ValType::I32,
                    ValType::I64,
                    ValType::F32,
                    ValType::F64,
                    ValType::V128,
                    ValType::FuncRef,
                    ValType::ExternRef,
                ])
                .with_returns(vec![ValType::FuncRef, ValType::ExternRef, ValType::V128])
                .build();

            // check the arguments
            let result = sig.args();
            assert!(result.is_some());
            let args = result.unwrap();
            assert_eq!(
                args,
                &[
                    ValType::I32,
                    ValType::I64,
                    ValType::F32,
                    ValType::F64,
                    ValType::V128,
                    ValType::FuncRef,
                    ValType::ExternRef,
                ]
            );

            // check the returns
            let result = sig.returns();
            assert!(result.is_some());
            let returns = result.unwrap();
            assert_eq!(
                returns,
                &[ValType::FuncRef, ValType::ExternRef, ValType::V128]
            );
        }

        // test signature without args and returns
        {
            let sig = FuncTypeBuilder::new().build();
            assert_eq!(sig.args(), None);
            assert_eq!(sig.returns(), None);
        }
    }

    #[test]
    fn test_func_basic() {
        // create an ImportModule
        let result = ImportObjectBuilder::new()
            .with_func::<(i32, i32), i32>("add", Box::new(real_add))
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
        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported host function
        let result = instance.func("add");
        assert!(result.is_some());
        let host_func = result.unwrap();

        // check the signature of the host function
        let result = host_func.ty();
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert!(func_ty.args().is_some());
        assert_eq!(func_ty.args().unwrap(), [ValType::I32; 2]);
        assert!(func_ty.returns().is_some());
        assert_eq!(func_ty.returns().unwrap(), [ValType::I32]);

        // run the host function
        let result = host_func.call(
            &mut executor,
            [WasmValue::from_i32(2), WasmValue::from_i32(3)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 5);
    }

    #[test]
    fn test_func_wrap() {
        let result = Func::wrap::<(i32, i32), i32>(Box::new(real_add));
        assert!(result.is_ok());
        let func = result.unwrap();

        let mut executor = crate::Executor::new(None, None).unwrap();
        let result = func.call(
            &mut executor,
            [WasmValue::from_i32(2), WasmValue::from_i32(3)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 5);
    }

    fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
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
