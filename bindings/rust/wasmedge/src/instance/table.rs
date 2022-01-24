use crate::{error::WasmEdgeResult, wasmedge, RefType, Value};
use std::ops::RangeInclusive;

#[derive(Debug)]
pub struct Table {
    pub(crate) inner: wasmedge::Table,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Table {
    pub fn new(elem_ty: RefType, limit: RangeInclusive<u32>) -> WasmEdgeResult<Self> {
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

    pub fn elem_ty(&self) -> RefType {
        unimplemented!()
    }

    pub fn limit(&self) -> RangeInclusive<u32> {
        unimplemented!()
    }

    pub fn capacity(&self) -> usize {
        unimplemented!()
    }

    pub fn grow(&mut self, size: u32) -> WasmEdgeResult<()> {
        unimplemented!()
    }

    pub fn get_data(&self, idx: usize) -> WasmEdgeResult<Value> {
        unimplemented!()
    }

    pub fn set_data(&mut self, data: Value, idx: usize) -> WasmEdgeResult<()> {
        unimplemented!()
    }
}
