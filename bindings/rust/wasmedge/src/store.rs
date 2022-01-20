use crate::{error::WasmEdgeResult, wasmedge};

#[derive(Debug)]
pub struct Store {
    pub(crate) inner: wasmedge::Store,
}
impl Store {
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = wasmedge::Store::create()?;
        Ok(Self { inner })
    }
}
