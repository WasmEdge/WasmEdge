use crate::error::WasmEdgeResult;
use std::{ffi::CString, os::unix::ffi::OsStrExt, path::Path};

#[cfg(unix)]
pub fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    Ok(CString::new(path.as_os_str().as_bytes())?)
}

#[cfg(windows)]
pub fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
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
