//! Defines WasmEdge AST Module, Export, and Import structs.

use super::ffi;
use crate::{
    error::{ExportError, ImportError, WasmEdgeError},
    types::WasmEdgeLimit,
    WasmEdgeResult,
};
use std::{borrow::Cow, ffi::CStr, sync::Arc};
use wasmedge_types::{
    ExternalInstanceType, FuncType, GlobalType, MemoryType, Mutability, RefType, TableType, ValType,
};

/// Defines compiled in-memory representation of an input WASM binary.
///
/// [Module] is also called *AST Module* in WasmEdge terminology. A [Module] is a compiled in-memory
/// representation of an input WebAssembly binary. In the instantiation process, a [Module] is used to create a
/// [module stance](crate::instance), from which the exported [functions](crate::Function), [tables](crate::Table),
/// [memories](crate::Memory), and [globals](crate::Global) can be fetched.
#[derive(Debug, Clone)]
pub struct Module {
    pub(crate) inner: Arc<InnerModule>,
}
impl Drop for Module {
    fn drop(&mut self) {
        if Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_ASTModuleDelete(self.inner.0) };
        }
    }
}
impl Module {
    /// Returns the number of wasm imports in the [Module].
    pub fn count_of_imports(&self) -> u32 {
        unsafe { ffi::WasmEdge_ASTModuleListImportsLength(self.inner.0) }
    }

    /// Returns the types of wasm imports in the [Module].
    pub fn imports(&self) -> Vec<ImportType<'_>> {
        let size = self.count_of_imports();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            ffi::WasmEdge_ASTModuleListImports(self.inner.0, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        returns
            .into_iter()
            .map(|ctx| ImportType {
                inner: InnerImportType(ctx),
                module: self,
            })
            .collect()
    }

    /// Returns the count of wasm exports in the [Module].
    pub fn count_of_exports(&self) -> u32 {
        unsafe { ffi::WasmEdge_ASTModuleListExportsLength(self.inner.0) }
    }

    /// Returns the types of wasm exports in the [Module].
    pub fn exports(&self) -> Vec<ExportType<'_>> {
        let size = self.count_of_exports();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            ffi::WasmEdge_ASTModuleListExports(self.inner.0, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        returns
            .into_iter()
            .map(|ctx| ExportType {
                inner: InnerExportType(ctx),
                module: self,
            })
            .collect()
    }

    /// Provides a raw pointer to the inner ASTModule context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_FunctionTypeContext {
        self.inner.0 as *const _
    }
}

#[derive(Debug)]
pub(crate) struct InnerModule(pub(crate) *mut ffi::WasmEdge_ASTModuleContext);
unsafe impl Send for InnerModule {}
unsafe impl Sync for InnerModule {}

/// Defines the types of the imported wasm value.
#[derive(Debug)]
pub struct ImportType<'module> {
    pub(crate) inner: InnerImportType,
    pub(crate) module: &'module Module,
}
impl<'module> Drop for ImportType<'module> {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            self.inner.0 = std::ptr::null();
        }
    }
}
impl<'module> ImportType<'module> {
    /// Returns the type of this import.
    pub fn ty(&self) -> WasmEdgeResult<ExternalInstanceType> {
        let ty = unsafe { ffi::WasmEdge_ImportTypeGetExternalType(self.inner.0) };
        let ty: ExternalInstanceType = ty.into();
        match ty {
            ExternalInstanceType::Func(_) => {
                let ctx_func_ty = unsafe {
                    ffi::WasmEdge_ImportTypeGetFunctionType(
                        self.module.inner.0 as *const _,
                        self.inner.0 as *const _,
                    )
                };
                match ctx_func_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Import(ImportError::FuncType(
                        "Fail to get the function type".into(),
                    )))),
                    false => {
                        // get types of the arguments
                        let args_len = unsafe {
                            ffi::WasmEdge_FunctionTypeGetParametersLength(ctx_func_ty) as usize
                        };
                        let mut args = Vec::with_capacity(args_len);
                        unsafe {
                            ffi::WasmEdge_FunctionTypeGetParameters(
                                ctx_func_ty,
                                args.as_mut_ptr(),
                                args_len as u32,
                            );
                            args.set_len(args_len);
                        }
                        let args: Vec<ValType> = args.into_iter().map(Into::into).collect();

                        // get types of the returns
                        let returns_len = unsafe {
                            ffi::WasmEdge_FunctionTypeGetReturnsLength(ctx_func_ty) as usize
                        };
                        let mut returns = Vec::with_capacity(returns_len);
                        unsafe {
                            ffi::WasmEdge_FunctionTypeGetReturns(
                                ctx_func_ty,
                                returns.as_mut_ptr(),
                                returns_len as u32,
                            );
                            returns.set_len(returns_len);
                        }
                        let returns: Vec<ValType> = returns.into_iter().map(Into::into).collect();

                        Ok(ExternalInstanceType::Func(FuncType::new(
                            Some(args),
                            Some(returns),
                        )))
                    }
                }
            }
            ExternalInstanceType::Global(_) => {
                let ctx_global_ty = unsafe {
                    ffi::WasmEdge_ImportTypeGetGlobalType(self.module.inner.0, self.inner.0)
                };
                match ctx_global_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Import(ImportError::MemType(
                        "Fail to get the global type".into(),
                    )))),
                    false => {
                        // get the value type
                        let val = unsafe { ffi::WasmEdge_GlobalTypeGetValType(ctx_global_ty) };
                        let val_ty: ValType = val.into();

                        // get mutability
                        let val = unsafe { ffi::WasmEdge_GlobalTypeGetMutability(ctx_global_ty) };
                        let mutability: Mutability = val.into();

                        Ok(ExternalInstanceType::Global(GlobalType::new(
                            val_ty, mutability,
                        )))
                    }
                }
            }
            ExternalInstanceType::Memory(_) => {
                let ctx_mem_ty = unsafe {
                    ffi::WasmEdge_ImportTypeGetMemoryType(self.module.inner.0, self.inner.0)
                };
                match ctx_mem_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Import(ImportError::MemType(
                        "Fail to get the memory type".into(),
                    )))),
                    false => {
                        let limit = unsafe { ffi::WasmEdge_MemoryTypeGetLimit(ctx_mem_ty) };
                        let limit: WasmEdgeLimit = limit.into();

                        Ok(ExternalInstanceType::Memory(MemoryType::new(
                            limit.min(),
                            limit.max(),
                            limit.shared(),
                        )?))
                    }
                }
            }
            ExternalInstanceType::Table(_) => {
                let ctx_tab_ty = unsafe {
                    ffi::WasmEdge_ImportTypeGetTableType(self.module.inner.0, self.inner.0)
                };
                match ctx_tab_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Import(ImportError::TableType(
                        "Fail to get the table type".into(),
                    )))),
                    false => {
                        // get the element type
                        let elem_ty = unsafe { ffi::WasmEdge_TableTypeGetRefType(ctx_tab_ty) };
                        let elem_ty: RefType = elem_ty.into();

                        // get the limit
                        let limit = unsafe { ffi::WasmEdge_TableTypeGetLimit(ctx_tab_ty) };
                        let limit: WasmEdgeLimit = limit.into();

                        Ok(ExternalInstanceType::Table(TableType::new(
                            elem_ty,
                            limit.min(),
                            limit.max(),
                        )))
                    }
                }
            }
        }
    }

    /// Returns the field name of the module that this import is expected to come from.
    pub fn name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = ffi::WasmEdge_ImportTypeGetExternalName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Returns the module name that this import is expected to come from.
    pub fn module_name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = ffi::WasmEdge_ImportTypeGetModuleName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Provides a raw pointer to the inner ImportType context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_ImportTypeContext {
        self.inner.0 as *const _
    }
}

#[derive(Debug)]
pub(crate) struct InnerImportType(pub(crate) *const ffi::WasmEdge_ImportTypeContext);
unsafe impl Send for InnerImportType {}
unsafe impl Sync for InnerImportType {}

/// Defines the types of the exported wasm values.
#[derive(Debug)]
pub struct ExportType<'module> {
    pub(crate) inner: InnerExportType,
    pub(crate) module: &'module Module,
}
impl<'module> Drop for ExportType<'module> {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            self.inner.0 = std::ptr::null();
        }
    }
}
impl<'module> ExportType<'module> {
    /// Returns the type of this export.
    pub fn ty(&self) -> WasmEdgeResult<ExternalInstanceType> {
        let ty = unsafe { ffi::WasmEdge_ExportTypeGetExternalType(self.inner.0) };
        let ty: ExternalInstanceType = ty.into();
        match ty {
            ExternalInstanceType::Func(_) => {
                let ctx_func_ty = unsafe {
                    ffi::WasmEdge_ExportTypeGetFunctionType(self.module.inner.0, self.inner.0)
                };
                match ctx_func_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Export(ExportError::FuncType(
                        "Fail to get the function type".into(),
                    )))),
                    false => {
                        // get types of the arguments
                        let args_len = unsafe {
                            ffi::WasmEdge_FunctionTypeGetParametersLength(ctx_func_ty) as usize
                        };
                        let mut args = Vec::with_capacity(args_len);
                        unsafe {
                            ffi::WasmEdge_FunctionTypeGetParameters(
                                ctx_func_ty,
                                args.as_mut_ptr(),
                                args_len as u32,
                            );
                            args.set_len(args_len);
                        }
                        let args: Vec<ValType> = args.into_iter().map(Into::into).collect();

                        // get types of the returns
                        let returns_len = unsafe {
                            ffi::WasmEdge_FunctionTypeGetReturnsLength(ctx_func_ty) as usize
                        };
                        let mut returns = Vec::with_capacity(returns_len);
                        unsafe {
                            ffi::WasmEdge_FunctionTypeGetReturns(
                                ctx_func_ty,
                                returns.as_mut_ptr(),
                                returns_len as u32,
                            );
                            returns.set_len(returns_len);
                        }
                        let returns: Vec<ValType> = returns.into_iter().map(Into::into).collect();

                        Ok(ExternalInstanceType::Func(FuncType::new(
                            Some(args),
                            Some(returns),
                        )))
                    }
                }
            }
            ExternalInstanceType::Table(_) => {
                let ctx_tab_ty = unsafe {
                    ffi::WasmEdge_ExportTypeGetTableType(self.module.inner.0, self.inner.0)
                };
                match ctx_tab_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Export(ExportError::TableType(
                        "Fail to get the function type".into(),
                    )))),
                    false => {
                        // get the element type
                        let elem_ty = unsafe { ffi::WasmEdge_TableTypeGetRefType(ctx_tab_ty) };
                        let elem_ty: RefType = elem_ty.into();

                        // get the limit
                        let limit = unsafe { ffi::WasmEdge_TableTypeGetLimit(ctx_tab_ty) };
                        let limit: WasmEdgeLimit = limit.into();

                        Ok(ExternalInstanceType::Table(TableType::new(
                            elem_ty,
                            limit.min(),
                            limit.max(),
                        )))
                    }
                }
            }
            ExternalInstanceType::Memory(_) => {
                let ctx_mem_ty = unsafe {
                    ffi::WasmEdge_ExportTypeGetMemoryType(self.module.inner.0, self.inner.0)
                };
                match ctx_mem_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Export(ExportError::MemType(
                        "Fail to get the function type".into(),
                    )))),
                    false => {
                        let limit = unsafe { ffi::WasmEdge_MemoryTypeGetLimit(ctx_mem_ty) };
                        let limit: WasmEdgeLimit = limit.into();

                        Ok(ExternalInstanceType::Memory(MemoryType::new(
                            limit.min(),
                            limit.max(),
                            limit.shared(),
                        )?))
                    }
                }
            }
            ExternalInstanceType::Global(_) => {
                let ctx_global_ty = unsafe {
                    ffi::WasmEdge_ExportTypeGetGlobalType(self.module.inner.0, self.inner.0)
                };
                match ctx_global_ty.is_null() {
                    true => Err(Box::new(WasmEdgeError::Export(ExportError::GlobalType(
                        "Fail to get the function type".into(),
                    )))),
                    false => {
                        // get the value type
                        let val = unsafe { ffi::WasmEdge_GlobalTypeGetValType(ctx_global_ty) };
                        let val_ty: ValType = val.into();

                        // get mutability
                        let val = unsafe { ffi::WasmEdge_GlobalTypeGetMutability(ctx_global_ty) };
                        let mutability: Mutability = val.into();

                        Ok(ExternalInstanceType::Global(GlobalType::new(
                            val_ty, mutability,
                        )))
                    }
                }
            }
        }
    }

    /// Returns the name by which this export is known by.
    pub fn name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = ffi::WasmEdge_ExportTypeGetExternalName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Provides a raw pointer to the inner ExportType context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_ExportTypeContext {
        self.inner.0 as *const _
    }
}

#[derive(Debug)]
pub(crate) struct InnerExportType(pub(crate) *const ffi::WasmEdge_ExportTypeContext);
unsafe impl Send for InnerExportType {}
unsafe impl Sync for InnerExportType {}

#[cfg(test)]
mod tests {
    use crate::{Config, Loader};
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{ExternalInstanceType, Mutability, RefType, ValType};

    #[test]
    fn test_module_clone() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/import.wat");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // clone module
        let module_clone = module.clone();

        drop(module);
        assert_eq!(std::sync::Arc::strong_count(&module_clone.inner), 1);
        drop(module_clone);
    }

    #[test]
    fn test_module_import() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/import.wat");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // check imports

        assert_eq!(module.count_of_imports(), 14);
        let imports = module.imports();

        // check the ty, name, and module_name functions
        let result = imports[0].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Func(_));
        assert_eq!(imports[0].name(), "func-add");
        assert_eq!(imports[0].module_name(), "extern");

        let result = imports[1].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(imports[1].name(), "func-sub");
        assert_eq!(imports[1].module_name(), "extern");

        let result = imports[2].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(imports[2].name(), "func-mul");
        assert_eq!(imports[2].module_name(), "extern");

        let result = imports[3].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(imports[3].name(), "func-div");
        assert_eq!(imports[3].module_name(), "extern");

        let result = imports[4].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(imports[4].name(), "func-term");
        assert_eq!(imports[4].module_name(), "extern");

        let result = imports[5].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(imports[5].name(), "func-fail");
        assert_eq!(imports[5].module_name(), "extern");

        let result = imports[6].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(imports[6].name(), "glob-i32");
        assert_eq!(imports[6].module_name(), "dummy");

        let result = imports[7].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(imports[7].name(), "glob-i64");
        assert_eq!(imports[7].module_name(), "dummy");

        let result = imports[8].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(imports[8].name(), "glob-f32");
        assert_eq!(imports[8].module_name(), "dummy");

        let result = imports[9].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(imports[9].name(), "glob-f64");
        assert_eq!(imports[9].module_name(), "dummy");

        let result = imports[10].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Table(_));
        assert_eq!(imports[10].name(), "tab-func");
        assert_eq!(imports[10].module_name(), "dummy");

        let result = imports[11].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Table(_));
        assert_eq!(imports[11].name(), "tab-ext");
        assert_eq!(imports[11].module_name(), "dummy");

        let result = imports[12].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Memory(_));
        assert_eq!(imports[12].name(), "mem1");
        assert_eq!(imports[12].module_name(), "dummy");

        let result = imports[13].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Memory(_));
        assert_eq!(imports[13].name(), "mem2");
        assert_eq!(imports[13].module_name(), "dummy");

        // check the function_type function
        let result = imports[4].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Func(_));
        if let ExternalInstanceType::Func(func_ty) = ty {
            assert_eq!(func_ty.returns_len(), 1);
        }

        // check the table_type function
        let result = imports[11].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Table(_));
        if let ExternalInstanceType::Table(table_ty) = ty {
            assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
            assert_eq!(table_ty.minimum(), 10);
            assert_eq!(table_ty.maximum(), Some(30));
        }

        // check the memory_type function
        let result = imports[13].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Memory(_));
        if let ExternalInstanceType::Memory(mem_ty) = ty {
            assert_eq!(mem_ty.minimum(), 2);
            assert_eq!(mem_ty.maximum(), None);
        }

        // check the global_type function
        let result = imports[7].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Global(_));
        if let ExternalInstanceType::Global(global_ty) = ty {
            assert_eq!(global_ty.value_ty(), ValType::I64);
            assert_eq!(global_ty.mutability(), Mutability::Const);
        }
    }

    #[test]
    fn test_module_export() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/import.wat");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // check exports

        assert_eq!(module.count_of_exports(), 16);
        let exports = module.exports();

        // check the ty and name functions
        let result = exports[0].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[0].name(), "func-1");

        let result = exports[1].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[1].name(), "func-2");

        let result = exports[2].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[2].name(), "func-3");

        let result = exports[3].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[3].name(), "func-4");

        assert_eq!(module.count_of_exports(), 16);

        let result = exports[4].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[4].name(), "func-add");

        let result = exports[5].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[5].name(), "func-mul-2");

        let result = exports[6].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[6].name(), "func-call-indirect");

        let result = exports[7].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[7].name(), "func-host-add");

        let result = exports[8].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[8].name(), "func-host-sub");

        let result = exports[9].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[9].name(), "func-host-mul");

        let result = exports[10].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Func(_));
        assert_eq!(exports[10].name(), "func-host-div");

        let result = exports[11].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Table(_));
        assert_eq!(exports[11].name(), "tab-func");

        let result = exports[12].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Table(_));
        assert_eq!(exports[12].name(), "tab-ext");

        let result = exports[13].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Memory(_));
        assert_eq!(exports[13].name(), "mem");

        let result = exports[14].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(exports[14].name(), "glob-mut-i32");

        let result = exports[15].ty();
        assert!(result.is_ok());
        matches!(result.unwrap(), ExternalInstanceType::Global(_));
        assert_eq!(exports[15].name(), "glob-const-f32");

        // check the function_type function
        let result = exports[4].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Func(_));
        if let ExternalInstanceType::Func(func_ty) = ty {
            assert_eq!(func_ty.args_len(), 2);
            assert_eq!(func_ty.returns_len(), 1);
        }

        // check the table_type function
        let result = exports[12].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Table(_));
        if let ExternalInstanceType::Table(table_ty) = ty {
            assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
            assert_eq!(table_ty.minimum(), 10);
            assert_eq!(table_ty.maximum(), None);
        }

        // check the memory_type function
        let result = exports[13].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Memory(_));
        if let ExternalInstanceType::Memory(mem_ty) = ty {
            assert_eq!(mem_ty.minimum(), 1);
            assert_eq!(mem_ty.maximum(), Some(3));
        }

        // check the global_type function
        let result = exports[15].ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        matches!(ty, ExternalInstanceType::Global(_));
        if let ExternalInstanceType::Global(global_ty) = ty {
            assert_eq!(global_ty.value_ty(), ValType::F32);
            assert_eq!(global_ty.mutability(), Mutability::Const);
        }
    }

    #[test]
    fn test_module_send() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/import.wat");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        let handle = thread::spawn(move || {
            // check exports

            assert_eq!(module.count_of_exports(), 16);
            let exports = module.exports();

            // check the ty and name functions
            let result = exports[0].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[0].name(), "func-1");

            let result = exports[1].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[1].name(), "func-2");

            let result = exports[2].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[2].name(), "func-3");

            let result = exports[3].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[3].name(), "func-4");

            assert_eq!(module.count_of_exports(), 16);

            let result = exports[4].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[4].name(), "func-add");

            let result = exports[5].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[5].name(), "func-mul-2");

            let result = exports[6].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[6].name(), "func-call-indirect");

            let result = exports[7].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[7].name(), "func-host-add");

            let result = exports[8].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[8].name(), "func-host-sub");

            let result = exports[9].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[9].name(), "func-host-mul");

            let result = exports[10].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[10].name(), "func-host-div");

            let result = exports[11].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Table(_));
            assert_eq!(exports[11].name(), "tab-func");

            let result = exports[12].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Table(_));
            assert_eq!(exports[12].name(), "tab-ext");

            let result = exports[13].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Memory(_));
            assert_eq!(exports[13].name(), "mem");

            let result = exports[14].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Global(_));
            assert_eq!(exports[14].name(), "glob-mut-i32");

            let result = exports[15].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Global(_));
            assert_eq!(exports[15].name(), "glob-const-f32");

            // check the function_type function
            let result = exports[4].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Func(_));
            if let ExternalInstanceType::Func(func_ty) = ty {
                assert_eq!(func_ty.args_len(), 2);
                assert_eq!(func_ty.returns_len(), 1);
            }

            // check the table_type function
            let result = exports[12].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Table(_));
            if let ExternalInstanceType::Table(table_ty) = ty {
                assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
                assert_eq!(table_ty.minimum(), 10);
                assert_eq!(table_ty.maximum(), None);
            }

            // check the memory_type function
            let result = exports[13].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Memory(_));
            if let ExternalInstanceType::Memory(mem_ty) = ty {
                assert_eq!(mem_ty.minimum(), 1);
                assert_eq!(mem_ty.maximum(), Some(3));
            }

            // check the global_type function
            let result = exports[15].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Global(_));
            if let ExternalInstanceType::Global(global_ty) = ty {
                assert_eq!(global_ty.value_ty(), ValType::F32);
                assert_eq!(global_ty.mutability(), Mutability::Const);
            }
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_module_sync() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/examples/data/import.wat");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(&config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = Arc::new(Mutex::new(result.unwrap()));

        let module_cloned = Arc::clone(&module);
        let handle = thread::spawn(move || {
            let result = module_cloned.lock();
            assert!(result.is_ok());
            let module = result.unwrap();

            // check exports

            assert_eq!(module.count_of_exports(), 16);
            let exports = module.exports();

            // check the ty and name functions
            let result = exports[0].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[0].name(), "func-1");

            let result = exports[1].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[1].name(), "func-2");

            let result = exports[2].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[2].name(), "func-3");

            let result = exports[3].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[3].name(), "func-4");

            assert_eq!(module.count_of_exports(), 16);

            let result = exports[4].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[4].name(), "func-add");

            let result = exports[5].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[5].name(), "func-mul-2");

            let result = exports[6].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[6].name(), "func-call-indirect");

            let result = exports[7].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[7].name(), "func-host-add");

            let result = exports[8].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[8].name(), "func-host-sub");

            let result = exports[9].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[9].name(), "func-host-mul");

            let result = exports[10].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Func(_));
            assert_eq!(exports[10].name(), "func-host-div");

            let result = exports[11].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Table(_));
            assert_eq!(exports[11].name(), "tab-func");

            let result = exports[12].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Table(_));
            assert_eq!(exports[12].name(), "tab-ext");

            let result = exports[13].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Memory(_));
            assert_eq!(exports[13].name(), "mem");

            let result = exports[14].ty();
            assert!(result.is_ok());
            matches!(result.unwrap(), ExternalInstanceType::Global(_));
            assert_eq!(exports[14].name(), "glob-mut-i32");

            let result = exports[15].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Global(_));
            assert_eq!(exports[15].name(), "glob-const-f32");

            // check the function_type function
            let result = exports[4].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Func(_));
            if let ExternalInstanceType::Func(func_ty) = ty {
                assert_eq!(func_ty.args_len(), 2);
                assert_eq!(func_ty.returns_len(), 1);
            }

            // check the table_type function
            let result = exports[12].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Table(_));
            if let ExternalInstanceType::Table(table_ty) = ty {
                assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
                assert_eq!(table_ty.minimum(), 10);
                assert_eq!(table_ty.maximum(), None);
            }

            // check the memory_type function
            let result = exports[13].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Memory(_));
            if let ExternalInstanceType::Memory(mem_ty) = ty {
                assert_eq!(mem_ty.minimum(), 1);
                assert_eq!(mem_ty.maximum(), Some(3));
            }

            // check the global_type function
            let result = exports[15].ty();
            assert!(result.is_ok());
            let ty = result.unwrap();
            matches!(ty, ExternalInstanceType::Global(_));
            if let ExternalInstanceType::Global(global_ty) = ty {
                assert_eq!(global_ty.value_ty(), ValType::F32);
                assert_eq!(global_ty.mutability(), Mutability::Const);
            }
        });

        handle.join().unwrap();
    }
}
