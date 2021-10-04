#![deny(rust_2018_idioms, unreachable_pub)]

#[allow(warnings)]
pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}
pub use ffi::*;

impl Default for WasmEdge_String {
    fn default() -> Self {
        WasmEdge_String {
            Length: 0,
            Buf: std::ptr::null(),
        }
    }
}

pub fn decode_result(result: WasmEdge_Result) -> Result<(), Error> {
    unsafe {
        if WasmEdge_ResultOK(result) {
            Ok(())
        } else {
            Err(Error {
                code: WasmEdge_ResultGetCode(result),
                message: std::ffi::CStr::from_ptr(WasmEdge_ResultGetMessage(result))
                    .to_str()
                    .unwrap_or("error")
                    .to_string(),
            })
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error {
    pub code: WasmEdge_ErrCode,
    pub message: String,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn links() {
        unsafe {
            assert!(
                WasmEdge_VersionGetMajor()
                    + WasmEdge_VersionGetMinor()
                    + WasmEdge_VersionGetPatch()
                    != 0
            );
        }
    }
}
