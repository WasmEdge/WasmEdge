use crate::{error::Result, wasmedge, Instance, ValType, Value};

pub type HostFunc = dyn Fn(Vec<Value>) -> std::result::Result<Vec<Value>, u8>;

#[derive(Debug)]
pub struct Func<'instance> {
    pub(crate) inner: wasmedge::Function,
    pub(crate) _marker: std::marker::PhantomData<&'instance Instance<'instance>>,
}
impl<'instance> Func<'instance> {
    pub fn name(&self) -> Option<&str> {
        self.inner.name()
    }

    pub fn mod_name(&self) -> Option<&str> {
        self.inner.mod_name()
    }

    pub fn registered(&self) -> bool {
        self.inner.mod_name().is_some()
    }

    pub fn signature(&self) -> Result<Signature> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }
}

#[derive(Debug, Default)]
pub struct SignatureBuilder {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl SignatureBuilder {
    pub fn new() -> Self {
        Self {
            args: None,
            returns: None,
        }
    }

    pub fn with_args(self, args: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: Some(args.into_iter().collect::<Vec<_>>()),
            returns: self.returns,
        }
    }

    pub fn with_arg(self, arg: ValType) -> Self {
        self.with_args(std::iter::once(arg))
    }

    pub fn with_returns(self, returns: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: self.args,
            returns: Some(returns.into_iter().collect::<Vec<_>>()),
        }
    }

    pub fn with_return(self, ret: ValType) -> Self {
        self.with_returns(std::iter::once(ret))
    }

    pub fn build(self) -> Signature {
        Signature {
            args: self.args,
            returns: self.returns,
        }
    }
}

#[derive(Debug, PartialEq)]
pub struct Signature {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl Signature {
    pub fn args(&self) -> Option<&[ValType]> {
        match &self.args {
            Some(args) => Some(args.as_ref()),
            None => None,
        }
    }

    pub fn returns(&self) -> Option<&[ValType]> {
        match &self.returns {
            Some(returns) => Some(returns.as_ref()),
            None => None,
        }
    }
}
impl From<wasmedge::FuncType> for Signature {
    fn from(ty: wasmedge::FuncType) -> Self {
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
impl From<Signature> for wasmedge::FuncType {
    fn from(sig: Signature) -> Self {
        let args = match sig.args() {
            Some(args) => args.to_owned(),
            None => Vec::new(),
        };
        let returns = match sig.returns() {
            Some(returns) => returns.to_owned(),
            None => Vec::new(),
        };
        wasmedge::FuncType::create(args, returns).expect("fail to convert Signature into FuncType")
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        CommonConfigOptions, ConfigBuilder, Executor, ImportModuleBuilder, Statistics, Store,
        ValType,
    };

    #[test]
    fn test_func_signature() {
        // test signature with args and returns
        {
            let sig = SignatureBuilder::new()
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

        let result = executor.run_func(
            &mut store,
            Some("extern"),
            "add",
            [Value::from_i32(2), Value::from_i32(3)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 5);
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
