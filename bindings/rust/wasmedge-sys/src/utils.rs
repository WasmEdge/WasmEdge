use crate::raw_result::WasmEdgeResult;
use std::ffi::CString;
use std::path::Path;

#[cfg(unix)]
pub fn path_to_cstring(path: &Path) -> WasmEdgeResult<CString> {
    use std::os::unix::ffi::OsStrExt;

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

pub fn vec_string_to_c_array<T: Iterator<Item = E>, E: AsRef<str>>(
    args: T,
) -> *const *const std::os::raw::c_char {
    let mut v = vec![];
    for arg in args {
        v.push(string_to_c_char(arg));
    }
    let p = v.as_ptr();
    std::mem::forget(v);
    p
}

pub fn string_to_c_char(arg: impl AsRef<str>) -> *const std::os::raw::c_char {
    let s = CString::new(arg.as_ref()).unwrap();
    s.as_ptr()
}
