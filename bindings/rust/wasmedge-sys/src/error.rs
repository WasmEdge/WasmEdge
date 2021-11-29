use super::wasmedge::{
    WasmEdge_Result, WasmEdge_ResultGetCode, WasmEdge_ResultGetMessage, WasmEdge_ResultOK,
};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct WasmEdgeError {
    /// 0x00: Success
    /// 0x01: Terminated -> Success
    /// 0x02: Failed
    /// 0x03: NullError
    /// 0x20: File not found
    pub code: u32,
    pub message: String,
}
impl From<std::ffi::NulError> for WasmEdgeError {
    fn from(e: std::ffi::NulError) -> WasmEdgeError {
        WasmEdgeError {
            code: 3,
            message: e.to_string(),
        }
    }
}
impl From<WasmEdge_Result> for WasmEdgeError {
    fn from(result: WasmEdge_Result) -> Self {
        let code = unsafe { WasmEdge_ResultGetCode(result) };
        let message = unsafe {
            let c_str = std::ffi::CStr::from_ptr(WasmEdge_ResultGetMessage(result));
            c_str.to_string_lossy().into_owned()
        };
        WasmEdgeError { code, message }
    }
}

pub type WasmEdgeResult<T> = Result<T, WasmEdgeError>;

pub fn check(result: WasmEdge_Result) -> WasmEdgeResult<()> {
    unsafe {
        if !WasmEdge_ResultOK(result) {
            let code = WasmEdge_ResultGetCode(result);
            let message = std::ffi::CStr::from_ptr(WasmEdge_ResultGetMessage(result))
                .to_string_lossy()
                .into_owned();
            return Err(WasmEdgeError { code, message });
        }
    }
    Ok(())
}
