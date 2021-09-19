use super::wasmedge;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ErrReport {
    pub code: u32,
    pub message: &'static str,
}

pub fn is_ok(res: wasmedge::WasmEdge_Result) -> bool {
    unsafe { wasmedge::WasmEdge_ResultOK(res) }
}

pub fn get_code(res: wasmedge::WasmEdge_Result) -> u32 {
    unsafe { wasmedge::WasmEdge_ResultGetCode(res) }
}

pub fn get_message<'a>(res: wasmedge::WasmEdge_Result) -> &'a str {
    unsafe {
        std::ffi::CStr::from_ptr(wasmedge::WasmEdge_ResultGetMessage(res))
            .to_str()
            .unwrap_or("Utf8 Error")
    }
}

impl From<wasmedge::WasmEdge_Result> for ErrReport {
    fn from(raw_result: wasmedge::WasmEdge_Result) -> Self {
        ErrReport {
            code: get_code(raw_result),
            message: get_message(raw_result),
        }
    }
}

// Since WasmEdge_ErrCode is subject to change on the wasmedge side
// it does not correspond to enum here
pub fn decode_result(raw_result: wasmedge::WasmEdge_Result) -> Result<(), ErrReport> {
    if is_ok(raw_result) {
        Ok(())
    } else {
        Err(raw_result.into())
    }
}
