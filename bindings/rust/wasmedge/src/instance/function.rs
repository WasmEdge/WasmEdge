//! Defines Func, SignatureBuilder, and Signature structs.
use crate::{error::Result, sys, Instance, WasmValueType};

/// Struct of WasmEdge Func.
///
/// A WasmEdge [Func] represents a host function. A host function is a function defined outside WASM module and passed to it.
///
/// # Example
///
/// The following example shows how to create a host function, access it by its name and its type info.
///
/// ```rust
/// use wasmedge::{ImportModuleBuilder, config::{ConfigBuilder, CommonConfigOptions}, Statistics, Executor, Store, WasmValue, WasmValueType, SignatureBuilder};
///
/// // a function to be exported as host function
/// fn real_add(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
///     if inputs.len() != 2 {
///         return Err(1);
///     }
///
///     let a = if inputs[0].ty() == WasmValueType::I32 {
///         inputs[0].to_i32()
///     } else {
///         return Err(2);
///     };
///
///     let b = if inputs[1].ty() == WasmValueType::I32 {
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
/// let result = ImportModuleBuilder::new()
/// .with_func(
///     "add",
///     SignatureBuilder::new()
///         .with_args(vec![WasmValueType::I32; 2])
///         .with_returns(vec![WasmValueType::I32])
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
/// let result = store.named_instance("extern");
/// assert!(result.is_some());
/// let instance = result.unwrap();
///
/// // get the exported host function
/// let result = instance.func("add");
/// assert!(result.is_some());
/// let host_func = result.unwrap();
///
/// // check the signature of the host function
/// let result = host_func.signature();
/// assert!(result.is_ok());
/// let signature = result.unwrap();
/// assert!(signature.args().is_some());
/// assert_eq!(signature.args().unwrap(), [WasmValueType::I32; 2]);
/// assert!(signature.returns().is_some());
/// assert_eq!(signature.returns().unwrap(), [WasmValueType::I32]);
///
/// ```
#[derive(Debug)]
pub struct Func<'instance> {
    pub(crate) inner: sys::Function,
    pub(crate) _marker: std::marker::PhantomData<&'instance Instance<'instance>>,
}
impl<'instance> Func<'instance> {
    /// Returns the name of the host function.
    pub fn name(&self) -> Option<&str> {
        self.inner.name()
    }

    /// Returns the name of the module instance which hosts the host function.
    pub fn mod_name(&self) -> Option<&str> {
        self.inner.mod_name()
    }

    /// Returns the signature of the host function.
    ///
    /// If fail to get the signature, then an error is returned.
    pub fn signature(&self) -> Result<Signature> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }
}

/// Struct of WasmEdge SignatureBuilder.
///
/// [SignatureBuilder] is used to build a [Signature].
#[derive(Debug, Default)]
pub struct SignatureBuilder {
    args: Option<Vec<WasmValueType>>,
    returns: Option<Vec<WasmValueType>>,
}
impl SignatureBuilder {
    /// Creates a new [SignatureBuilder].
    pub fn new() -> Self {
        Self {
            args: None,
            returns: None,
        }
    }

    /// Adds arguments to the signature.
    ///
    /// # Argument
    ///
    /// `args` specifies the arguments to be added to the signature.
    pub fn with_args(self, args: impl IntoIterator<Item = WasmValueType>) -> Self {
        Self {
            args: Some(args.into_iter().collect::<Vec<_>>()),
            returns: self.returns,
        }
    }

    /// Adds a single argument to the signature.
    ///
    /// # Argument
    ///
    /// `arg` specifies the argument to be added to the signature.
    pub fn with_arg(self, arg: WasmValueType) -> Self {
        self.with_args(std::iter::once(arg))
    }

    /// Adds returns to the signature.
    ///
    /// # Argument
    ///
    /// `returns` specifies the returns to be added to the signature.
    pub fn with_returns(self, returns: impl IntoIterator<Item = WasmValueType>) -> Self {
        Self {
            args: self.args,
            returns: Some(returns.into_iter().collect::<Vec<_>>()),
        }
    }

    /// Adds a single return to the signature.
    ///
    /// # Argument
    ///
    /// `return` specifies the return to be added to the signature.
    pub fn with_return(self, ret: WasmValueType) -> Self {
        self.with_returns(std::iter::once(ret))
    }

    /// Returns a [Signature].
    pub fn build(self) -> Signature {
        Signature {
            args: self.args,
            returns: self.returns,
        }
    }
}

/// Struct of WasmEdge Signature.
///
/// [Signature] is used to represent the types of the arguments and the returns of a host function.
///
/// # Example
///
/// The following example shows how to create the signature of a host function, which has two arguments of `ValType::I32` type and a single return of `ValType::I32` type.
///
/// ```rust
/// use wasmedge::{SignatureBuilder, Signature, WasmValueType};
///
/// let _: Signature = SignatureBuilder::new()
///         .with_args(vec![WasmValueType::I32; 2])
///         .with_returns(vec![WasmValueType::I32])
///         .build();
/// ```
///
#[derive(Debug, PartialEq)]
pub struct Signature {
    args: Option<Vec<WasmValueType>>,
    returns: Option<Vec<WasmValueType>>,
}
impl Signature {
    /// Returns the types of the arguments of a host function.
    pub fn args(&self) -> Option<&[WasmValueType]> {
        match &self.args {
            Some(args) => Some(args.as_ref()),
            None => None,
        }
    }

    /// Returns the types of the returns of a host function.
    pub fn returns(&self) -> Option<&[WasmValueType]> {
        match &self.returns {
            Some(returns) => Some(returns.as_ref()),
            None => None,
        }
    }
}
impl From<sys::FuncType> for Signature {
    fn from(ty: sys::FuncType) -> Self {
        let args = if ty.params_len() > 0 {
            Some(ty.params_type_iter().collect::<Vec<_>>())
        } else {
            None
        };

        let returns = if ty.returns_len() > 0 {
            Some(ty.returns_type_iter().collect::<Vec<_>>())
        } else {
            None
        };

        Self { args, returns }
    }
}
impl From<Signature> for sys::FuncType {
    fn from(sig: Signature) -> Self {
        let args = match sig.args() {
            Some(args) => args.to_owned(),
            None => Vec::new(),
        };
        let returns = match sig.returns() {
            Some(returns) => returns.to_owned(),
            None => Vec::new(),
        };
        sys::FuncType::create(args, returns).expect("fail to convert Signature into FuncType")
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        sys::WasmValue,
        Executor, ImportModuleBuilder, Statistics, Store, WasmValueType,
    };

    #[test]
    fn test_func_signature() {
        // test signature with args and returns
        {
            let sig = SignatureBuilder::new()
                .with_args(vec![
                    WasmValueType::I32,
                    WasmValueType::I64,
                    WasmValueType::F32,
                    WasmValueType::F64,
                    WasmValueType::V128,
                    WasmValueType::FuncRef,
                    WasmValueType::ExternRef,
                ])
                .with_returns(vec![
                    WasmValueType::FuncRef,
                    WasmValueType::ExternRef,
                    WasmValueType::V128,
                ])
                .build();

            // check the arguments
            let result = sig.args();
            assert!(result.is_some());
            let args = result.unwrap();
            assert_eq!(
                args,
                &[
                    WasmValueType::I32,
                    WasmValueType::I64,
                    WasmValueType::F32,
                    WasmValueType::F64,
                    WasmValueType::V128,
                    WasmValueType::FuncRef,
                    WasmValueType::ExternRef,
                ]
            );

            // check the returns
            let result = sig.returns();
            assert!(result.is_some());
            let returns = result.unwrap();
            assert_eq!(
                returns,
                &[
                    WasmValueType::FuncRef,
                    WasmValueType::ExternRef,
                    WasmValueType::V128
                ]
            );
        }

        // test signature without args and returns
        {
            let sig = SignatureBuilder::new().build();
            assert_eq!(sig.args(), None);
            assert_eq!(sig.returns(), None);
        }
    }

    #[test]
    fn test_func() {
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

        let result = executor.run_func(
            &mut store,
            Some("extern"),
            "add",
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
