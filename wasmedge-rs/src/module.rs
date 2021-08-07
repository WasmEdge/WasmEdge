use std::{
    ffi::{CString, NulError},
    path::Path,
};

use wasmedge_sys as ffi;

pub struct Module {
    pub(crate) ctx: *mut ffi::WasmEdge_ASTModuleContext,
}

impl Module {
    pub fn load_from_file(
        path: impl AsRef<std::path::Path>,
        config: &crate::config::Config,
    ) -> Result<Self, Error> {
        let path = path.as_ref();
        let path_cstr = path_to_cstr(path)
            .map_err(|e| Error::Path(path.to_string_lossy().to_string(), Box::new(e)))?;
        let ctx = unsafe {
            let loader = ffi::WasmEdge_LoaderCreate(config.ctx);

            let mut ctx: *mut ffi::WasmEdge_ASTModuleContext = std::ptr::null_mut();

            ffi::decode_result(ffi::WasmEdge_LoaderParseFromFile(
                loader,
                &mut ctx as *mut _,
                path_cstr.as_ptr(),
            ))
            .map_err(Error::Load)?;

            if ctx.is_null() {
                Err(Error::Unknown)
            } else {
                Ok(ctx)
            }
        }?;
        Ok(Self { ctx })
    }
}

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("an unknown error occured")]
    Unknown,

    #[error("`{0}` is not a valid path: {1}")]
    Path(String, Box<dyn std::error::Error>),

    #[error("loader error: {}", _0.message)]
    Load(ffi::Error),
}

#[cfg(windows)]
fn path_to_cstr(path: &Path) -> Result<CString, NulError> {
    use std::os::windows::ffi::OsStrExt;
    let path_bytes: Vec<u8> = path
        .as_os_str()
        .encode_wide()
        .flat_map(|c| c.to_be_bytes())
        .collect();
    CString::new(path_bytes)
}

#[cfg(unix)]
fn path_to_cstr(path: &Path) -> Result<CString, NulError> {
    use std::os::unix::ffi::OsStrExt;
    CString::new(path.as_os_str().as_bytes().to_vec())
}
