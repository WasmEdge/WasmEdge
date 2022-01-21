use crate::{error::WasmEdgeResult, wasmedge, ValType, Value};

#[derive(Debug)]
pub struct Func {
    pub(crate) inner: wasmedge::Function,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Func {
    pub fn new(
        sig: FuncSignature,
        real_fn: Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        let inner = wasmedge::Function::create(sig.into(), real_fn, cost)?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

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

    pub fn signature(&self) -> WasmEdgeResult<FuncSignature> {
        let func_ty = self.inner.ty()?;
        Ok(func_ty.into())
    }
}

#[derive(Debug)]
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

    pub fn build(self) -> FuncSignature {
        FuncSignature {
            args: self.args,
            returns: self.returns,
        }
    }
}

#[derive(Debug)]
pub struct FuncSignature {
    args: Option<Vec<ValType>>,
    returns: Option<Vec<ValType>>,
}
impl FuncSignature {
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
impl From<wasmedge::FuncType> for FuncSignature {
    fn from(ty: wasmedge::FuncType) -> Self {
        let args = if ty.params_len() > 0 {
            Some(ty.params_type_iter().map(|x| x.into()).collect::<Vec<_>>())
        } else {
            None
        };

        let returns = if ty.returns_len() > 0 {
            Some(ty.returns_type_iter().map(|x| x.into()).collect::<Vec<_>>())
        } else {
            None
        };

        Self { args, returns }
    }
}
impl From<FuncSignature> for wasmedge::FuncType {
    fn from(_: FuncSignature) -> Self {
        todo!()
    }
}
