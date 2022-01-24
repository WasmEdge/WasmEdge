use crate::{error::WasmEdgeResult, wasmedge, Mutability, ValType, Value};

#[derive(Debug)]
pub struct Global {
    pub(crate) inner: wasmedge::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Global {
    pub fn new(val: Value, val_ty: ValType, mutable: Mutability) -> WasmEdgeResult<Self> {
        unimplemented!()
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

    pub fn value_type(&self) -> ValType {
        unimplemented!()
    }

    pub fn mutability(&self) -> Mutability {
        unimplemented!()
    }

    pub fn get_value(&self) -> Value {
        unimplemented!()
    }

    pub fn set_value(&self, val: Value) -> WasmEdgeResult<()> {
        unimplemented!()
    }
}
