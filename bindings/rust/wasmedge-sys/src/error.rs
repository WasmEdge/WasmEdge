use super::wasmedge::{
    WasmEdge_Result, WasmEdge_ResultGetCode, WasmEdge_ResultGetMessage, WasmEdge_ResultOK,
};

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Error {
    WasmEdgeError(WasmEdgeError),
    /// other errors in the crate, so that we can remove assert
    OperationError(String),
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::OperationError(message) => write!(f, "{}", message),
            Error::WasmEdgeError(e) => write!(f, "{}", e),
        }
    }
}
impl From<WasmEdge_Result> for Error {
    fn from(result: WasmEdge_Result) -> Self {
        Error::WasmEdgeError(WasmEdgeError::from(result))
    }
}
impl From<std::ffi::NulError> for Error {
    fn from(e: std::ffi::NulError) -> Self {
        Error::WasmEdgeError(WasmEdgeError::from(e))
    }
}
impl From<std::ffi::FromBytesWithNulError> for Error {
    fn from(e: std::ffi::FromBytesWithNulError) -> Self {
        Error::WasmEdgeError(WasmEdgeError::from(e))
    }
}
impl From<std::str::Utf8Error> for Error {
    fn from(e: std::str::Utf8Error) -> Self {
        Error::WasmEdgeError(WasmEdgeError::from(e))
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct WasmEdgeError {
    /// 0x00: Success
    /// 0x01: Terminated -> Success
    /// 0x02: Failed
    /// 0x03: NullError
    /// 0x20: File not found
    /// 0x21: A nul byte was not in the expected position
    /// 0x22: Errors which can occur when attempting to interpret a sequence of u8 as a string.
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
impl From<std::ffi::FromBytesWithNulError> for WasmEdgeError {
    fn from(e: std::ffi::FromBytesWithNulError) -> Self {
        WasmEdgeError {
            code: 0x21u32,
            message: e.to_string(),
        }
    }
}
impl From<std::str::Utf8Error> for WasmEdgeError {
    fn from(e: std::str::Utf8Error) -> Self {
        WasmEdgeError {
            code: 0x22u32,
            message: e.to_string(),
        }
    }
}
impl std::fmt::Display for WasmEdgeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "code: {}, message: {}", self.code, self.message)
    }
}

pub type WasmEdgeResult<T> = Result<T, Error>;

pub fn check(result: WasmEdge_Result) -> WasmEdgeResult<()> {
    unsafe {
        if !WasmEdge_ResultOK(result) {
            let code = WasmEdge_ResultGetCode(result);
            let message = std::ffi::CStr::from_ptr(WasmEdge_ResultGetMessage(result))
                .to_string_lossy()
                .into_owned();
            return Err(Error::WasmEdgeError(WasmEdgeError { code, message }));
        }
    }
    Ok(())
}
