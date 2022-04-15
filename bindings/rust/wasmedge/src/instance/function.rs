//! Defines Func, SignatureBuilder, and Signature structs.
use crate::{error::Result, Engine, HostFunc};
use wasmedge_sys::{self as sys, WasmValue};
use wasmedge_types::{FuncType, ValType};

/// Struct of WasmEdge Func.
///
/// A WasmEdge [Func] represents a host function. A host function is a function defined outside WASM module and passed to it.
///
/// # Example
///
/// The following example shows how to create a host function, access it by its name and its type info.
///
/// ```rust
/// use wasmedge::{ImportObjectBuilder, config::{ConfigBuilder, CommonConfigOptions}, Statistics, Executor, Store, WasmValue, FuncTypeBuilder};
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
/// .with_func(
///     "add",
///     FuncTypeBuilder::new()
///         .with_args(vec![ValType::I32; 2])
///         .with_returns(vec![ValType::I32])
///         .build(),
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
}
impl Func {
    /// Creates a new host function with the given [FuncType](wasmedge_types::FuncType).
    ///
    /// Notice that if intend to add a host function as an import object, then use the `with_func` function of [ImportObjectBuilder](crate::ImportObjectBuilder) instead. This function is only used to create a host function which is not an import object, for example, generate a funcref and store it in a table.
    ///
    /// # Arguments
    ///
    /// * `ty` - The type of the arguments and returns of the [host function](crate::Func).
    ///
    /// * `real_func` - The host function.
    ///
    /// # Error
    ///
    /// If fail to create the host function, then an error is returned.
    pub fn new(ty: FuncType, real_func: HostFunc) -> Result<Self> {
        let inner = sys::Function::create(&ty.into(), real_func, 0)?;
        Ok(Self { inner })
    }

    /// Returns the type of the host function.
    ///
    /// If fail to get the signature, then an error is returned.
    pub fn ty(&self) -> Result<FuncType> {
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
    ) -> Result<Vec<WasmValue>> {
        engine.run_func(self, args)
    }
}

/// Struct of WasmEdge FuncTypeBuilder.
///
/// [FuncTypeBuilder] is used to build a [FuncType](wasmedge_types::FuncType).
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

/// Struct of WasmEdge FuncRef.
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
    pub fn ty(&self) -> Result<FuncType> {
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
    ) -> Result<Vec<WasmValue>> {
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
            .with_func(
                "add",
                FuncTypeBuilder::new()
                    .with_args(vec![ValType::I32; 2])
                    .with_returns(vec![ValType::I32])
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
