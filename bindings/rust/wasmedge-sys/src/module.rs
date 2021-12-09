use super::wasmedge;
use crate::{
    error::{check, Error, WasmEdgeResult},
    instance::function::FuncType,
    instance::global::GlobalType,
    instance::memory::MemType,
    types::ExternalType,
    utils, Config,
};
use ::core::mem::MaybeUninit as MU;
use std::{borrow::Cow, ffi::CStr, path::Path};

#[derive(Debug)]
pub struct Module {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ASTModuleContext,
    pub(crate) registered: bool,
}
impl Drop for Module {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ASTModuleDelete(self.ctx) };
        }
    }
}
impl Module {
    pub fn load_from_file<P: AsRef<Path>>(config: &Config, path: P) -> WasmEdgeResult<Self> {
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        let path = utils::path_to_cstring(path.as_ref())?;

        unsafe {
            check(wasmedge::WasmEdge_LoaderParseFromFile(
                loader_ctx,
                &mut ctx as *mut _,
                path.as_ptr(),
            ))?;
        }

        Ok(Self {
            ctx,
            registered: false,
        })
    }

    pub fn load_from_buffer(config: &Config, buffer: &[u8]) -> WasmEdgeResult<Self> {
        if buffer.is_empty() {
            return Err(Error::OperationError(String::from(
                "WasmEdge fail to load an empty buffer",
            )));
        }
        let loader_ctx = unsafe { wasmedge::WasmEdge_LoaderCreate(config.ctx) };
        let mut ctx: *mut wasmedge::WasmEdge_ASTModuleContext = std::ptr::null_mut();

        let ptr = unsafe {
            let ptr = libc::malloc(buffer.len());
            let dst = ::core::slice::from_raw_parts_mut(ptr.cast::<MU<u8>>(), buffer.len());
            let src = ::core::slice::from_raw_parts(buffer.as_ptr().cast::<MU<u8>>(), buffer.len());
            dst.copy_from_slice(src);
            ptr
        };

        let res = unsafe {
            wasmedge::WasmEdge_LoaderParseFromBuffer(
                loader_ctx,
                &mut ctx as *mut _,
                ptr as *const u8,
                buffer.len() as u32,
            )
        };

        unsafe {
            libc::free(ptr as *mut libc::c_void);
        }

        check(res)?;

        if ctx.is_null() {
            return Err(Error::OperationError(String::from(
                "WasmEdge failed to load from buffer!",
            )));
        }

        Ok(Self {
            ctx,
            registered: false,
        })
    }

    /// Get the length of imports list
    pub fn imports_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ASTModuleListImportsLength(self.ctx) }
    }

    /// Get the imports
    pub fn imports(&self) -> WasmEdgeResult<impl Iterator<Item = ImportType>> {
        let size = self.imports_len();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            wasmedge::WasmEdge_ASTModuleListImports(self.ctx, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        Ok(returns.into_iter().map(|ctx| ImportType { ctx }))
    }

    /// Get the length of exports list
    pub fn exports_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ASTModuleListExportsLength(self.ctx) }
    }

    /// Get the exports
    pub fn exports(&self) -> WasmEdgeResult<impl Iterator<Item = ExportType>> {
        let size = self.exports_len();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            wasmedge::WasmEdge_ASTModuleListExports(self.ctx, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        Ok(returns.into_iter().map(|ctx| ExportType { ctx }))
    }
}

#[derive(Debug)]
pub struct ImportType {
    pub(crate) ctx: *const wasmedge::WasmEdge_ImportTypeContext,
}
impl Drop for ImportType {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            self.ctx = std::ptr::null();
        }
    }
}
impl ImportType {
    /// Get the external type
    pub fn external_type(&self) -> ExternalType {
        let ty = unsafe { wasmedge::WasmEdge_ImportTypeGetExternalType(self.ctx) };
        ty.into()
    }

    /// Get the module name
    pub fn module_name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ImportTypeGetModuleName(self.ctx);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Get the external name
    pub fn external_name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ImportTypeGetExternalName(self.ctx);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Get the external value (which is function type)
    pub fn function_type(&self, module: &Module) -> WasmEdgeResult<FuncType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Function {
            return Err(Error::OperationError(format!(
                "The external type should be 'function', but found '{}'",
                external_ty
            )));
        }
        let ctx_func_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetFunctionType(module.ctx, self.ctx) };
        match ctx_func_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the function type",
            ))),
            false => Ok(FuncType {
                ctx: ctx_func_ty,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is table type)
    pub fn table_type(&self, module: &Module) -> WasmEdgeResult<TableType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Table {
            return Err(Error::OperationError(format!(
                "The external type should be 'table', but found '{}'",
                external_ty
            )));
        }
        let ctx_tab_ty = unsafe { wasmedge::WasmEdge_ImportTypeGetTableType(module.ctx, self.ctx) };
        match ctx_tab_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the table type",
            ))),
            false => Ok(TableType {
                ctx: ctx_tab_ty,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is memory type)
    pub fn memory_type(&self, module: &Module) -> WasmEdgeResult<MemType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Memory {
            return Err(Error::OperationError(format!(
                "The external type should be 'memory', but found '{}'",
                external_ty
            )));
        }
        let ctx_mem_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetMemoryType(module.ctx, self.ctx) };
        match ctx_mem_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the memory type",
            ))),
            false => Ok(MemType {
                ctx: ctx_mem_ty as *mut _,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is global type)
    pub fn global_type(&self, module: &Module) -> WasmEdgeResult<GlobalType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Global {
            return Err(Error::OperationError(format!(
                "The external type should be 'global', but found '{}'",
                external_ty
            )));
        }
        let ctx_global_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetGlobalType(module.ctx, self.ctx) };
        match ctx_global_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the global type",
            ))),
            false => Ok(GlobalType {
                ctx: ctx_global_ty as *mut _,
                registered: true,
            }),
        }
    }
}

#[derive(Debug)]
pub struct ExportType {
    pub(crate) ctx: *const wasmedge::WasmEdge_ExportTypeContext,
}
impl Drop for ExportType {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            self.ctx = std::ptr::null();
        }
    }
}
impl ExportType {
    /// Get the external type
    pub fn external_type(&self) -> ExternalType {
        let ty = unsafe { wasmedge::WasmEdge_ExportTypeGetExternalType(self.ctx) };
        ty.into()
    }

    /// Get the external name
    pub fn external_name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ExportTypeGetExternalName(self.ctx);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Get the external value (which is function type)
    pub fn function_type(&self, module: &Module) -> WasmEdgeResult<FuncType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Function {
            return Err(Error::OperationError(format!(
                "The external type should be 'function', but found '{}'",
                external_ty
            )));
        }
        let ctx_func_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetFunctionType(module.ctx, self.ctx) };
        match ctx_func_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the function type",
            ))),
            false => Ok(FuncType {
                ctx: ctx_func_ty,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is table type)
    pub fn table_type(&self, module: &Module) -> WasmEdgeResult<TableType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Table {
            return Err(Error::OperationError(format!(
                "The external type should be 'table', but found '{}'",
                external_ty
            )));
        }
        let ctx_tab_ty = unsafe { wasmedge::WasmEdge_ExportTypeGetTableType(module.ctx, self.ctx) };
        match ctx_tab_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the table type",
            ))),
            false => Ok(TableType {
                ctx: ctx_tab_ty,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is memory type)
    pub fn memory_type(&self, module: &Module) -> WasmEdgeResult<MemType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Memory {
            return Err(Error::OperationError(format!(
                "The external type should be 'memory', but found '{}'",
                external_ty
            )));
        }
        let ctx_mem_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetMemoryType(module.ctx, self.ctx) };
        match ctx_mem_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the function type",
            ))),
            false => Ok(MemType {
                ctx: ctx_mem_ty as *mut _,
                registered: true,
            }),
        }
    }

    /// Get the external value (which is global type)
    pub fn global_type(&self, module: &Module) -> WasmEdgeResult<GlobalType> {
        let external_ty = self.external_type();
        if external_ty != ExternalType::Global {
            return Err(Error::OperationError(format!(
                "The external type should be 'global', but found '{}'",
                external_ty
            )));
        }
        let ctx_global_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetGlobalType(module.ctx, self.ctx) };
        match ctx_global_ty.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the function type",
            ))),
            false => Ok(GlobalType {
                ctx: ctx_global_ty as *mut _,
                registered: true,
            }),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Module;
    use crate::Config;

    #[test]
    fn test_module_from_buffer() {
        let wasm_path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");
        let result = std::fs::read(wasm_path);
        assert!(result.is_ok());
        let buf = result.unwrap();

        let result = Config::create();
        assert!(result.is_ok());
        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::load_from_buffer(&conf, &buf);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.ctx.is_null());
    }

    #[test]
    fn test_module_from_file() {
        let path =
            std::path::PathBuf::from(env!("WASMEDGE_DIR")).join("test/api/apiTestData/test.wasm");

        let result = Config::create();
        assert!(result.is_ok());
        let conf = result.unwrap();
        let conf = conf.enable_bulkmemoryoperations(true);
        assert!(conf.has_bulkmemoryoperations());

        let result = Module::load_from_file(&conf, path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.ctx.is_null());
    }
}
