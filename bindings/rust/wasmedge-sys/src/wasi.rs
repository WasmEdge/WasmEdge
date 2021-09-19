use super::wasmedge;
use crate::string;
use core::default::Default;

#[derive(Debug, Default)]
pub struct ImportObjInitParams {
    pub(crate) args: &[wasmedge::WasmEdge_String],
    pub(crate) args_len: u32,
    pub(crate) envs: &[wasmedge::WasmEdge_String],
    pub(crate) envs_len: u32,
    pub(crate) dirs: &[wasmedge::WasmEdge_String],
    pub(crate) dirs_len: u32,
    pub(crate) preopens: &[wasmedge::WasmEdge_String],
    pub(crate) preopens_len: u32,
}

impl ImportObjInitParams {
    pub(crate) fn new() -> Self{
        Self::default()
    }
}
