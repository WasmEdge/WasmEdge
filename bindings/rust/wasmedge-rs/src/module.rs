use super::wasmedge;

use std::{
    ffi::{CString, NulError},
    path::Path,
};

use crate::error::ModuleError;

#[derive(Debug)]
pub struct Module {
    pub inner: wasmedge::Module,
}

impl Module{
    fn new(config: wasmedge::Config, module_path: &str) -> Result<Self, Error>{
        let module_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
        .join(module_path);
        let path = path.as_ref();
        let path_cstr = path_to_cstr(path)
            .map_err(|e| ModuleError::Path(path.to_string_lossy().to_string(), Box::new(e)))?;

        let module = wasmedge::Module::load_from_file(&config, path_cstr).map_err(ModuleError::Load)?;

        Ok(Self{ inner: module})
        
    }
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