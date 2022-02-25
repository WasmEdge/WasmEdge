//! Defines WasmEdge AST Module, Export, and Import structs.

use super::wasmedge;
use crate::{
    error::{ExportError, ImportError, WasmEdgeError, WasmEdgeResult},
    instance::{
        function::{FuncType, InnerFuncType},
        global::{GlobalType, InnerGlobalType},
        memory::{InnerMemType, MemType},
        table::{InnerTableType, TableType},
    },
    types::ExternalType,
};
use std::{borrow::Cow, ffi::CStr, marker::PhantomData};

/// Struct of WasmEdge AST (short for abstract syntax tree) Module.
///
/// [`Module`] is the representation of WasmEdge AST Module concept, but not equivalent to
/// *[W3C Module](https://www.w3.org/TR/wasm-core-1/#concepts%E2%91%A0)*.
/// The initial state of a [`Module`] loaded from a file or buffer is a AST module; After the instantiation step,
/// it "transform"s a module which is equivalent to W3C Module in semantics. The state transformation can be summarized
/// as below:
///
/// `a WASM file ---<load>--> AST Module ---<instantiate>--> Module`
///
#[derive(Debug)]
pub struct Module {
    pub(crate) inner: InnerModule,
}
impl Drop for Module {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            unsafe { wasmedge::WasmEdge_ASTModuleDelete(self.inner.0) };
        }
    }
}
impl Module {
    /// Returns the number of the imports of the [`Module`].
    pub fn count_of_imports(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ASTModuleListImportsLength(self.inner.0) }
    }

    /// Returns the imports of the [`Module`].
    pub fn imports_iter(&self) -> impl Iterator<Item = Import<'_>> {
        let size = self.count_of_imports();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            wasmedge::WasmEdge_ASTModuleListImports(self.inner.0, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        returns.into_iter().map(|ctx| Import {
            inner: InnerImport(ctx),
            _marker: PhantomData,
        })
    }

    /// Returns the count of the exports of the [`Module`].
    pub fn count_of_exports(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_ASTModuleListExportsLength(self.inner.0) }
    }

    /// Returns the exports of the [`Module`].
    pub fn exports_iter(&self) -> impl Iterator<Item = Export<'_>> {
        let size = self.count_of_exports();
        let mut returns = Vec::with_capacity(size as usize);
        unsafe {
            wasmedge::WasmEdge_ASTModuleListExports(self.inner.0, returns.as_mut_ptr(), size);
            returns.set_len(size as usize);
        }

        returns.into_iter().map(|ctx| Export {
            inner: InnerExport(ctx),
            _marker: PhantomData,
        })
    }
}

#[derive(Debug)]
pub(crate) struct InnerModule(pub(crate) *mut wasmedge::WasmEdge_ASTModuleContext);
unsafe impl Send for InnerModule {}
unsafe impl Sync for InnerModule {}

/// Struct of WasmEdge Import.
///
/// The [`Import`] is used for getting the information of the imports from a WasmEdge AST [`Module`].
#[derive(Debug)]
pub struct Import<'module> {
    pub(crate) inner: InnerImport,
    pub(crate) _marker: PhantomData<&'module Module>,
}
impl<'module> Drop for Import<'module> {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            self.inner.0 = std::ptr::null();
        }
    }
}
impl<'module> Import<'module> {
    /// Returns the external type of the [`Import`].
    pub fn ty(&self) -> ExternalType {
        let ty = unsafe { wasmedge::WasmEdge_ImportTypeGetExternalType(self.inner.0) };
        ty.into()
    }

    /// Returns the external name of the [`Import`].
    pub fn name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ImportTypeGetExternalName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Returns the module name from the [`Import`].
    pub fn module_name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ImportTypeGetModuleName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Returns the [function type](crate::FuncType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the function type, then an error is returned.
    pub fn function_type(&self, module: &Module) -> WasmEdgeResult<FuncType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Function {
            return Err(WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Function,
                actual: external_ty,
            }));
        }
        let ctx_func_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetFunctionType(module.inner.0, self.inner.0) };
        match ctx_func_ty.is_null() {
            true => Err(WasmEdgeError::Import(ImportError::FuncType(
                "Fail to get the function type".into(),
            ))),
            false => Ok(FuncType {
                inner: InnerFuncType(ctx_func_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [table type](crate::TableType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the table type, then an error is returned.
    pub fn table_type(&self, module: &Module) -> WasmEdgeResult<TableType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Table {
            return Err(WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Table,
                actual: external_ty,
            }));
        }
        let ctx_tab_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetTableType(module.inner.0, self.inner.0) };
        match ctx_tab_ty.is_null() {
            true => Err(WasmEdgeError::Import(ImportError::TableType(
                "Fail to get the table type".into(),
            ))),
            false => Ok(TableType {
                inner: InnerTableType(ctx_tab_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [memory type](crate::MemType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the memory type, then an error is returned.
    pub fn memory_type(&self, module: &Module) -> WasmEdgeResult<MemType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Memory {
            return Err(WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Memory,
                actual: external_ty,
            }));
        }
        let ctx_mem_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetMemoryType(module.inner.0, self.inner.0) };
        match ctx_mem_ty.is_null() {
            true => Err(WasmEdgeError::Import(ImportError::MemType(
                "Fail to get the memory type".into(),
            ))),
            false => Ok(MemType {
                inner: InnerMemType(ctx_mem_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [global type](crate::GlobalType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the global type, then an error is returned.
    pub fn global_type(&self, module: &Module) -> WasmEdgeResult<GlobalType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Global {
            return Err(WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Global,
                actual: external_ty,
            }));
        }
        let ctx_global_ty =
            unsafe { wasmedge::WasmEdge_ImportTypeGetGlobalType(module.inner.0, self.inner.0) };
        match ctx_global_ty.is_null() {
            true => Err(WasmEdgeError::Import(ImportError::MemType(
                "Fail to get the global type".into(),
            ))),
            false => Ok(GlobalType {
                inner: InnerGlobalType(ctx_global_ty as *mut _),
                registered: true,
            }),
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerImport(pub(crate) *const wasmedge::WasmEdge_ImportTypeContext);
unsafe impl Send for InnerImport {}
unsafe impl Sync for InnerImport {}

/// Struct of WasmEdge Export.
///
/// The [`Export`] is used for getting the information of the exports from a WasmEdge AST [`Module`].
#[derive(Debug)]
pub struct Export<'module> {
    pub(crate) inner: InnerExport,
    pub(crate) _marker: PhantomData<&'module Module>,
}
impl<'module> Drop for Export<'module> {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            self.inner.0 = std::ptr::null();
        }
    }
}
impl<'module> Export<'module> {
    /// Returns the external type of the [`Export`].
    pub fn ty(&self) -> ExternalType {
        let ty = unsafe { wasmedge::WasmEdge_ExportTypeGetExternalType(self.inner.0) };
        ty.into()
    }

    /// Returns the external name of the [`Export`].
    pub fn name(&self) -> Cow<'_, str> {
        let c_name = unsafe {
            let raw_name = wasmedge::WasmEdge_ExportTypeGetExternalName(self.inner.0);
            CStr::from_ptr(raw_name.Buf)
        };
        c_name.to_string_lossy()
    }

    /// Returns the [function type](crate::FuncType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the function type, then an error is returned.
    pub fn function_type(&self, module: &Module) -> WasmEdgeResult<FuncType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Function {
            return Err(WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Function,
                actual: external_ty,
            }));
        }
        let ctx_func_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetFunctionType(module.inner.0, self.inner.0) };
        match ctx_func_ty.is_null() {
            true => Err(WasmEdgeError::Export(ExportError::FuncType(
                "Fail to get the function type".into(),
            ))),
            false => Ok(FuncType {
                inner: InnerFuncType(ctx_func_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [table type](crate::TableType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the table type, then an error is returned.
    pub fn table_type(&self, module: &Module) -> WasmEdgeResult<TableType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Table {
            return Err(WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Table,
                actual: external_ty,
            }));
        }
        let ctx_tab_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetTableType(module.inner.0, self.inner.0) };
        match ctx_tab_ty.is_null() {
            true => Err(WasmEdgeError::Export(ExportError::TableType(
                "Fail to get the function type".into(),
            ))),
            false => Ok(TableType {
                inner: InnerTableType(ctx_tab_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [memory type](crate::MemType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the memory type, then an error is returned.
    pub fn memory_type(&self, module: &Module) -> WasmEdgeResult<MemType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Memory {
            return Err(WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Memory,
                actual: external_ty,
            }));
        }
        let ctx_mem_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetMemoryType(module.inner.0, self.inner.0) };
        match ctx_mem_ty.is_null() {
            true => Err(WasmEdgeError::Export(ExportError::MemType(
                "Fail to get the function type".into(),
            ))),
            false => Ok(MemType {
                inner: InnerMemType(ctx_mem_ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the [global type](crate::GlobalType).
    ///
    /// # Argument
    ///
    /// - `module` specifies the target WasmEdge AST [`Module`].
    ///
    /// # Error
    ///
    /// If fail to get the global type, then an error is returned.
    pub fn global_type(&self, module: &Module) -> WasmEdgeResult<GlobalType> {
        let external_ty = self.ty();
        if external_ty != ExternalType::Global {
            return Err(WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Global,
                actual: external_ty,
            }));
        }
        let ctx_global_ty =
            unsafe { wasmedge::WasmEdge_ExportTypeGetGlobalType(module.inner.0, self.inner.0) };
        match ctx_global_ty.is_null() {
            true => Err(WasmEdgeError::Export(ExportError::GlobalType(
                "Fail to get the function type".into(),
            ))),
            false => Ok(GlobalType {
                inner: InnerGlobalType(ctx_global_ty as *mut _),
                registered: true,
            }),
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerExport(pub(crate) *const wasmedge::WasmEdge_ExportTypeContext);
unsafe impl Send for InnerExport {}
unsafe impl Sync for InnerExport {}

#[cfg(test)]
mod tests {
    use crate::{
        Config, ExportError, ExternalType, ImportError, Loader, Mutability, RefType, ValType,
        WasmEdgeError,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_module_import() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/import.wasm");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // check imports

        assert_eq!(module.count_of_imports(), 14);
        let imports = module.imports_iter().collect::<Vec<_>>();

        // check the ty, name, and module_name functions
        assert_eq!(imports[0].ty(), ExternalType::Function);
        assert_eq!(imports[0].name(), "func-add");
        assert_eq!(imports[0].module_name(), "extern");

        assert_eq!(imports[1].ty(), ExternalType::Function);
        assert_eq!(imports[1].name(), "func-sub");
        assert_eq!(imports[1].module_name(), "extern");

        assert_eq!(imports[2].ty(), ExternalType::Function);
        assert_eq!(imports[2].name(), "func-mul");
        assert_eq!(imports[2].module_name(), "extern");

        assert_eq!(imports[3].ty(), ExternalType::Function);
        assert_eq!(imports[3].name(), "func-div");
        assert_eq!(imports[3].module_name(), "extern");

        assert_eq!(imports[4].ty(), ExternalType::Function);
        assert_eq!(imports[4].name(), "func-term");
        assert_eq!(imports[4].module_name(), "extern");

        assert_eq!(imports[5].ty(), ExternalType::Function);
        assert_eq!(imports[5].name(), "func-fail");
        assert_eq!(imports[5].module_name(), "extern");

        assert_eq!(imports[6].ty(), ExternalType::Global);
        assert_eq!(imports[6].name(), "glob-i32");
        assert_eq!(imports[6].module_name(), "dummy");

        assert_eq!(imports[7].ty(), ExternalType::Global);
        assert_eq!(imports[7].name(), "glob-i64");
        assert_eq!(imports[7].module_name(), "dummy");

        assert_eq!(imports[8].ty(), ExternalType::Global);
        assert_eq!(imports[8].name(), "glob-f32");
        assert_eq!(imports[8].module_name(), "dummy");

        assert_eq!(imports[9].ty(), ExternalType::Global);
        assert_eq!(imports[9].name(), "glob-f64");
        assert_eq!(imports[9].module_name(), "dummy");

        assert_eq!(imports[10].ty(), ExternalType::Table);
        assert_eq!(imports[10].name(), "tab-func");
        assert_eq!(imports[10].module_name(), "dummy");

        assert_eq!(imports[11].ty(), ExternalType::Table);
        assert_eq!(imports[11].name(), "tab-ext");
        assert_eq!(imports[11].module_name(), "dummy");

        assert_eq!(imports[12].ty(), ExternalType::Memory);
        assert_eq!(imports[12].name(), "mem1");
        assert_eq!(imports[12].module_name(), "dummy");

        assert_eq!(imports[13].ty(), ExternalType::Memory);
        assert_eq!(imports[13].name(), "mem2");
        assert_eq!(imports[13].module_name(), "dummy");

        // check the function_type function
        let result = imports[8].function_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Function,
                actual: ExternalType::Global,
            })
        );
        let result = imports[4].function_type(&module);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.returns_len(), 1);

        // check the table_type function
        let result = imports[0].table_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Table,
                actual: ExternalType::Function,
            })
        );
        let result = imports[11].table_type(&module);
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
        assert_eq!(table_ty.limit(), 10..=30);

        // check the memory_type function
        let result = imports[0].memory_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Memory,
                actual: ExternalType::Function,
            })
        );
        let result = imports[13].memory_type(&module);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        assert_eq!(mem_ty.limit(), 2..=2);

        // check the global_type function
        let result = imports[0].global_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Import(ImportError::Type {
                expected: ExternalType::Global,
                actual: ExternalType::Function,
            })
        );
        let result = imports[7].global_type(&module);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        assert_eq!(global_ty.value_type(), ValType::I64);
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }

    #[test]
    fn test_module_export() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/import.wasm");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        // check exports

        assert_eq!(module.count_of_exports(), 16);
        let exports = module.exports_iter().collect::<Vec<_>>();

        // check the ty and name functions
        assert_eq!(exports[0].ty(), ExternalType::Function);
        assert_eq!(exports[0].name(), "func-1");

        assert_eq!(exports[1].ty(), ExternalType::Function);
        assert_eq!(exports[1].name(), "func-2");

        assert_eq!(exports[2].ty(), ExternalType::Function);
        assert_eq!(exports[2].name(), "func-3");

        assert_eq!(exports[3].ty(), ExternalType::Function);
        assert_eq!(exports[3].name(), "func-4");

        assert_eq!(module.count_of_exports(), 16);

        assert_eq!(exports[4].ty(), ExternalType::Function);
        assert_eq!(exports[4].name(), "func-add");

        assert_eq!(exports[5].ty(), ExternalType::Function);
        assert_eq!(exports[5].name(), "func-mul-2");

        assert_eq!(exports[6].ty(), ExternalType::Function);
        assert_eq!(exports[6].name(), "func-call-indirect");

        assert_eq!(exports[7].ty(), ExternalType::Function);
        assert_eq!(exports[7].name(), "func-host-add");

        assert_eq!(exports[8].ty(), ExternalType::Function);
        assert_eq!(exports[8].name(), "func-host-sub");

        assert_eq!(exports[9].ty(), ExternalType::Function);
        assert_eq!(exports[9].name(), "func-host-mul");

        assert_eq!(exports[10].ty(), ExternalType::Function);
        assert_eq!(exports[10].name(), "func-host-div");

        assert_eq!(exports[11].ty(), ExternalType::Table);
        assert_eq!(exports[11].name(), "tab-func");

        assert_eq!(exports[12].ty(), ExternalType::Table);
        assert_eq!(exports[12].name(), "tab-ext");

        assert_eq!(exports[13].ty(), ExternalType::Memory);
        assert_eq!(exports[13].name(), "mem");

        assert_eq!(exports[14].ty(), ExternalType::Global);
        assert_eq!(exports[14].name(), "glob-mut-i32");

        assert_eq!(exports[15].ty(), ExternalType::Global);
        assert_eq!(exports[15].name(), "glob-const-f32");

        // check the function_type function
        let result = exports[15].function_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Function,
                actual: ExternalType::Global,
            })
        );
        let result = exports[4].function_type(&module);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        assert_eq!(func_ty.params_len(), 2);
        assert_eq!(func_ty.returns_len(), 1);

        // check the table_type function
        let result = exports[0].table_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Table,
                actual: ExternalType::Function,
            })
        );
        let result = exports[12].table_type(&module);
        assert!(result.is_ok());
        let table_ty = result.unwrap();
        assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
        assert_eq!(table_ty.limit(), 10..=10);

        // check the memory_type function
        let result = exports[0].memory_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Memory,
                actual: ExternalType::Function,
            })
        );
        let result = exports[13].memory_type(&module);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        assert_eq!(mem_ty.limit(), 1..=3);

        // check the global_type function
        let result = exports[0].global_type(&module);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Export(ExportError::Type {
                expected: ExternalType::Global,
                actual: ExternalType::Function,
            })
        );
        let result = exports[15].global_type(&module);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        assert_eq!(global_ty.value_type(), ValType::F32);
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }

    #[test]
    fn test_module_send() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/import.wasm");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(config));
        assert!(result.is_ok());
        let loader = result.unwrap();
        let result = loader.from_file(path);
        assert!(result.is_ok());
        let module = result.unwrap();
        assert!(!module.inner.0.is_null());

        let handle = thread::spawn(move || {
            // check exports

            assert_eq!(module.count_of_exports(), 16);
            let exports = module.exports_iter().collect::<Vec<_>>();

            // check the ty and name functions
            assert_eq!(exports[0].ty(), ExternalType::Function);
            assert_eq!(exports[0].name(), "func-1");

            assert_eq!(exports[1].ty(), ExternalType::Function);
            assert_eq!(exports[1].name(), "func-2");

            assert_eq!(exports[2].ty(), ExternalType::Function);
            assert_eq!(exports[2].name(), "func-3");

            assert_eq!(exports[3].ty(), ExternalType::Function);
            assert_eq!(exports[3].name(), "func-4");

            assert_eq!(module.count_of_exports(), 16);

            assert_eq!(exports[4].ty(), ExternalType::Function);
            assert_eq!(exports[4].name(), "func-add");

            assert_eq!(exports[5].ty(), ExternalType::Function);
            assert_eq!(exports[5].name(), "func-mul-2");

            assert_eq!(exports[6].ty(), ExternalType::Function);
            assert_eq!(exports[6].name(), "func-call-indirect");

            assert_eq!(exports[7].ty(), ExternalType::Function);
            assert_eq!(exports[7].name(), "func-host-add");

            assert_eq!(exports[8].ty(), ExternalType::Function);
            assert_eq!(exports[8].name(), "func-host-sub");

            assert_eq!(exports[9].ty(), ExternalType::Function);
            assert_eq!(exports[9].name(), "func-host-mul");

            assert_eq!(exports[10].ty(), ExternalType::Function);
            assert_eq!(exports[10].name(), "func-host-div");

            assert_eq!(exports[11].ty(), ExternalType::Table);
            assert_eq!(exports[11].name(), "tab-func");

            assert_eq!(exports[12].ty(), ExternalType::Table);
            assert_eq!(exports[12].name(), "tab-ext");

            assert_eq!(exports[13].ty(), ExternalType::Memory);
            assert_eq!(exports[13].name(), "mem");

            assert_eq!(exports[14].ty(), ExternalType::Global);
            assert_eq!(exports[14].name(), "glob-mut-i32");

            assert_eq!(exports[15].ty(), ExternalType::Global);
            assert_eq!(exports[15].name(), "glob-const-f32");

            // check the function_type function
            let result = exports[15].function_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Function,
                    actual: ExternalType::Global,
                })
            );
            let result = exports[4].function_type(&module);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            assert_eq!(func_ty.params_len(), 2);
            assert_eq!(func_ty.returns_len(), 1);

            // check the table_type function
            let result = exports[0].table_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Table,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[12].table_type(&module);
            assert!(result.is_ok());
            let table_ty = result.unwrap();
            assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
            assert_eq!(table_ty.limit(), 10..=10);

            // check the memory_type function
            let result = exports[0].memory_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Memory,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[13].memory_type(&module);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            assert_eq!(mem_ty.limit(), 1..=3);

            // check the global_type function
            let result = exports[0].global_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Global,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[15].global_type(&module);
            assert!(result.is_ok());
            let global_ty = result.unwrap();
            assert_eq!(global_ty.value_type(), ValType::F32);
            assert_eq!(global_ty.mutability(), Mutability::Const);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_module_sync() {
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/import.wasm");

        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // load module from file
        let result = Loader::create(Some(config));
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
            let exports = module.exports_iter().collect::<Vec<_>>();

            // check the ty and name functions
            assert_eq!(exports[0].ty(), ExternalType::Function);
            assert_eq!(exports[0].name(), "func-1");

            assert_eq!(exports[1].ty(), ExternalType::Function);
            assert_eq!(exports[1].name(), "func-2");

            assert_eq!(exports[2].ty(), ExternalType::Function);
            assert_eq!(exports[2].name(), "func-3");

            assert_eq!(exports[3].ty(), ExternalType::Function);
            assert_eq!(exports[3].name(), "func-4");

            assert_eq!(module.count_of_exports(), 16);

            assert_eq!(exports[4].ty(), ExternalType::Function);
            assert_eq!(exports[4].name(), "func-add");

            assert_eq!(exports[5].ty(), ExternalType::Function);
            assert_eq!(exports[5].name(), "func-mul-2");

            assert_eq!(exports[6].ty(), ExternalType::Function);
            assert_eq!(exports[6].name(), "func-call-indirect");

            assert_eq!(exports[7].ty(), ExternalType::Function);
            assert_eq!(exports[7].name(), "func-host-add");

            assert_eq!(exports[8].ty(), ExternalType::Function);
            assert_eq!(exports[8].name(), "func-host-sub");

            assert_eq!(exports[9].ty(), ExternalType::Function);
            assert_eq!(exports[9].name(), "func-host-mul");

            assert_eq!(exports[10].ty(), ExternalType::Function);
            assert_eq!(exports[10].name(), "func-host-div");

            assert_eq!(exports[11].ty(), ExternalType::Table);
            assert_eq!(exports[11].name(), "tab-func");

            assert_eq!(exports[12].ty(), ExternalType::Table);
            assert_eq!(exports[12].name(), "tab-ext");

            assert_eq!(exports[13].ty(), ExternalType::Memory);
            assert_eq!(exports[13].name(), "mem");

            assert_eq!(exports[14].ty(), ExternalType::Global);
            assert_eq!(exports[14].name(), "glob-mut-i32");

            assert_eq!(exports[15].ty(), ExternalType::Global);
            assert_eq!(exports[15].name(), "glob-const-f32");

            // check the function_type function
            let result = exports[15].function_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Function,
                    actual: ExternalType::Global,
                })
            );
            let result = exports[4].function_type(&module);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            assert_eq!(func_ty.params_len(), 2);
            assert_eq!(func_ty.returns_len(), 1);

            // check the table_type function
            let result = exports[0].table_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Table,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[12].table_type(&module);
            assert!(result.is_ok());
            let table_ty = result.unwrap();
            assert_eq!(table_ty.elem_ty(), RefType::ExternRef);
            assert_eq!(table_ty.limit(), 10..=10);

            // check the memory_type function
            let result = exports[0].memory_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Memory,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[13].memory_type(&module);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            assert_eq!(mem_ty.limit(), 1..=3);

            // check the global_type function
            let result = exports[0].global_type(&module);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Export(ExportError::Type {
                    expected: ExternalType::Global,
                    actual: ExternalType::Function,
                })
            );
            let result = exports[15].global_type(&module);
            assert!(result.is_ok());
            let global_ty = result.unwrap();
            assert_eq!(global_ty.value_type(), ValType::F32);
            assert_eq!(global_ty.mutability(), Mutability::Const);
        });

        handle.join().unwrap();
    }
}
