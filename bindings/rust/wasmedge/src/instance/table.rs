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

    pub fn ty(&self) -> WasmEdgeResult<TableType> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(TableType {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        })
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

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableType {
    elem_ty: RefType,
    min: u32,
    max: Option<u32>,
}
impl TableType {
    pub fn new(elem_ty: RefType, min: u32, max: Option<u32>) -> Self {
        Self { elem_ty, min, max }
    }

    pub fn elem_ty(&self) -> RefType {
        self.elem_ty
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> Option<u32> {
        self.max
    }
}
impl From<wasmedge::TableType> for TableType {
    fn from(ty: wasmedge::TableType) -> Self {
        let limit = ty.limit();
        Self {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        }
    }
}
