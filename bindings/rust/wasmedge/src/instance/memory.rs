use std::ops::RangeInclusive;

use crate::{error::WasmEdgeResult, wasmedge};

pub struct Memory {
    pub(crate) inner: wasmedge::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Memory {
    pub fn new(limit: RangeInclusive<u32>) -> WasmEdgeResult<Self> {
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

    pub fn limit(&self) -> RangeInclusive<u32> {
        unimplemented!()
    }

    pub fn page_count(&self) -> u32 {
        unimplemented!()
    }

    pub fn get_data(&self, offset: u32, len: u32) -> WasmEdgeResult<Vec<u8>> {
        unimplemented!()
    }

    pub fn set_data(
        &mut self,
        data: impl IntoIterator<Item = u8>,
        offset: u32,
    ) -> WasmEdgeResult<()> {
        unimplemented!()
    }

    pub fn data_pointer(&self, offset: u32, len: u32) -> WasmEdgeResult<&u8> {
        unimplemented!()
    }

    pub fn data_pointer_mut(&mut self, offset: u32, len: u32) -> WasmEdgeResult<&mut u8> {
        unimplemented!()
    }

    pub fn grow(&mut self, count: u32) -> WasmEdgeResult<()> {
        unimplemented!()
    }
}
