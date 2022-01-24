use crate::{error::WasmEdgeResult, wasmedge, Mutability, ValType, Value};

#[derive(Debug)]
pub struct Global {
    pub(crate) inner: wasmedge::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Global {
    pub fn new(ty: GlobalType, val: Value) -> WasmEdgeResult<Self> {
        let mut ty = wasmedge::GlobalType::create(ty.ty(), ty.mutability())?;
        let inner = wasmedge::Global::create(&mut ty, val)?;
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

    pub fn ty(&self) -> WasmEdgeResult<GlobalType> {
        let gt = self.inner.ty()?;
        Ok(GlobalType {
            ty: gt.value_type(),
            mutability: gt.mutability(),
        })
    }

    pub fn get_value(&self) -> Value {
        self.inner.get_value()
    }

    pub fn set_value(&mut self, val: Value) -> WasmEdgeResult<()> {
        self.inner.set_value(val)?;
        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GlobalType {
    ty: ValType,
    mutability: Mutability,
}
impl GlobalType {
    pub fn new(ty: ValType, mutability: Mutability) -> Self {
        Self { ty, mutability }
    }

    pub fn ty(&self) -> ValType {
        self.ty
    }

    pub fn mutability(&self) -> Mutability {
        self.mutability
    }
}
