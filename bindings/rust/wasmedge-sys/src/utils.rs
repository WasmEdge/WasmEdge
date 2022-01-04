//! Defines the versioning and logging functions.

use crate::{wasmedge, WasmEdgeResult};
use std::{
    ffi::{CStr, CString},
    path::Path,
};

#[cfg(unix)]
pub(crate) fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    use std::os::unix::ffi::OsStrExt;
    Ok(CString::new(path.as_os_str().as_bytes())?)
}

#[cfg(windows)]
pub(crate) fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    match path.to_str() {
        Some(s) => Ok(CString::new(s)?),
        None => {
            let message = format!("Couldn't convert path '{}' to UTF-8", path.display());
            Err(message.into())
        }
    }
}

pub(crate) fn string_to_c_char(arg: impl AsRef<str>) -> *const std::os::raw::c_char {
    let s = CString::new(arg.as_ref()).unwrap();
    s.as_ptr()
}

/// Full version.
///
/// ## Example
///
/// ```rust
/// // in `wasmedge.rs`
/// pub const WASMEDGE_VERSION: &'static [u8; 22usize] = b"0.8.2-rc.5-1-g809c746\0";
/// pub const WASMEDGE_VERSION_MAJOR: u32 = 0;
/// pub const WASMEDGE_VERSION_MINOR: u32 = 8;
/// pub const WASMEDGE_VERSION_PATCH: u32 = 2;
/// ```
pub fn full_version() -> WasmEdgeResult<&'static str> {
    Ok(CStr::from_bytes_with_nul(wasmedge::WASMEDGE_VERSION)?.to_str()?)
}

/// Semantic Version.
pub fn semv_version() -> String {
    format!(
        "{}.{}.{}",
        wasmedge::WASMEDGE_VERSION_MAJOR,
        wasmedge::WASMEDGE_VERSION_MINOR,
        wasmedge::WASMEDGE_VERSION_PATCH
    )
}

/// Logs the debug information.
pub fn log_debug_info() {
    unsafe { wasmedge::WasmEdge_LogSetDebugLevel() }
}

/// Logs the error information.
pub fn log_error_info() {
    unsafe { wasmedge::WasmEdge_LogSetErrorLevel() }
}
