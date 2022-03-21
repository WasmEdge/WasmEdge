use crate::{error::Result, wasmedge, Mutability, ValType, Value};

#[derive(Debug)]
pub struct Global<'instance> {
    pub(crate) inner: wasmedge::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Global<'instance> {
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

    pub fn ty(&self) -> Result<GlobalType> {
        let gt = self.inner.ty()?;
        Ok(GlobalType {
            ty: gt.value_type(),
            mutability: gt.mutability(),
        })
    }

    pub fn get_value(&self) -> Value {
        self.inner.get_value()
    }

    pub fn set_value(&mut self, val: Value) -> Result<()> {
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

    pub fn value_ty(&self) -> ValType {
        self.ty
    }

    pub fn mutability(&self) -> Mutability {
        self.mutability
    }

    pub fn to_raw(self) -> Result<wasmedge::GlobalType> {
        let raw = wasmedge::GlobalType::create(self.value_ty(), self.mutability())?;
        Ok(raw)
    }
}
impl From<wasmedge::GlobalType> for GlobalType {
    fn from(ty: wasmedge::GlobalType) -> Self {
        Self {
            ty: ty.value_type(),
            mutability: ty.mutability(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Mutability, ValType};

    #[test]
    fn test_global_type() {
        // create a GlobalType instance
        let global_ty = GlobalType::new(ValType::I32, Mutability::Const);

        // value type
        assert_eq!(global_ty.value_ty(), ValType::I32);
        // Mutability
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }
}
