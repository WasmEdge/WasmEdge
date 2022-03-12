use crate::{error::Result, wasmedge, Instance, ValType, Value, Vm};

pub type HostFunc = dyn Fn(Vec<Value>) -> std::result::Result<Vec<Value>, u8>;

#[derive(Debug)]
pub struct Func<'instance> {
    pub(crate) inner: wasmedge::Function,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance Instance<'instance>>,
}
impl<'instance> Func<'instance> {
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    pub fn registered(&self) -> bool {
        self.mod_name.is_some()
    }

    pub fn ty(&self) -> Result<Signature> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }

    pub fn call(&self, vm: &mut Vm, args: impl IntoIterator<Item = Value>) -> Result<Vec<Value>> {
        let returns = vm.run_func(self.mod_name(), self.name().unwrap(), args)?;
        Ok(returns)
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

    pub fn with_returns(self, returns: impl IntoIterator<Item = ValType>) -> Self {
        Self {
            args: self.args,
            returns: Some(returns.into_iter().collect::<Vec<_>>()),
        }
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
