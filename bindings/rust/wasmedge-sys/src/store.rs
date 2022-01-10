//! Defines WasmEdge Store struct.

use crate::{
    instance::{Function, Global, Memory, Table},
    types::WasmEdgeString,
    wasmedge, StoreError, WasmEdgeError, WasmEdgeResult,
};

/// Struct of Wasmedge Store.
///
/// The [`Store`] represents all global state that can be manipulated by WebAssembly programs. It
/// consists of the runtime representation of all instances of [functions](crate::Function), [tables](crate::Table),
/// [memories](crate::Memory), and [globals](crate::Global) that have been allocated during the
/// life time of the [Vm](crate::Vm).
#[derive(Debug)]
pub struct Store {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StoreContext,
    pub(crate) registered: bool,
}
impl Store {
    /// Creates a new [`Store`].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_StoreCreate() };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::Create)),
            false => Ok(Store {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the exported [function](crate::Function) instance in the anonymous [module](crate::Module)
    /// by the given function name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [`Store`] as an anonymous module.
    ///
    /// # Argument
    ///
    /// - `name` specifies the target exported [function](crate::Function) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [function](crate::Function), then an error is returned.
    pub fn find_func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Function> {
        let func_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindFunction(self.ctx, func_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundFunc(
                name.as_ref().to_string(),
            ))),
            false => Ok(Function {
                ctx,
                registered: true,
                ty: None,
            }),
        }
    }

    /// Returns the exported [function](crate::Function) instance in the registered [module](crate::Module)
    /// by the given function name and module name.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the registered [module](crate::Module).
    ///
    /// - `func_name` specifies the name of the exported [function](crate::Function) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target registered [function](crate::Function), then an error is returned.
    pub fn find_func_registered(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Function> {
        let ctx = unsafe {
            let mod_name: WasmEdgeString = mod_name.as_ref().into();
            let func_name: WasmEdgeString = func_name.as_ref().into();
            wasmedge::WasmEdge_StoreFindFunctionRegistered(
                self.ctx,
                mod_name.as_raw(),
                func_name.as_raw(),
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundFuncRegistered {
                func_name: func_name.as_ref().to_string(),
                mod_name: mod_name.as_ref().to_string(),
            })),
            false => Ok(Function {
                ctx,
                registered: true,
                ty: None,
            }),
        }
    }

    /// Returns the exported [table](crate::Table) instance in the anonymous [module](crate::Module)
    /// by the given table name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [`Store`] as an anonymous module.
    ///
    /// # Argument
    ///
    /// - `name` specifies the target exported [table](crate::Table) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [table](crate::Table), then an error is returned.
    pub fn find_table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let table_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindTable(self.ctx, table_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundGlobal(
                name.as_ref().to_string(),
            ))),
            false => Ok(Table {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the exported [table](crate::Table) instance in the registered [module](crate::Module)
    /// by the given table name and module name.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the registered [module](crate::Module).
    ///
    /// - `table_name` specifies the name of the exported [table](crate::Table) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target registered [table](crate::Table), then an error is returned.
    pub fn find_table_registered(
        &self,
        mod_name: impl AsRef<str>,
        table_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Table> {
        let ctx = unsafe {
            let mod_name: WasmEdgeString = mod_name.as_ref().into();
            let table_name: WasmEdgeString = table_name.as_ref().into();
            wasmedge::WasmEdge_StoreFindTableRegistered(
                self.ctx,
                mod_name.as_raw(),
                table_name.as_raw(),
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundTableRegistered {
                table_name: table_name.as_ref().to_string(),
                mod_name: mod_name.as_ref().to_string(),
            })),
            false => Ok(Table {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the exported [memory](crate::Memory) instance in the anonymous [module](crate::Module)
    /// by the given memory name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [`Store`] as an anonymous module.
    ///
    /// # Argument
    ///
    /// - `name` specifies the target exported [memory](crate::Memory) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [memory](crate::Memory), then an error is returned.
    pub fn find_memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let mem_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindMemory(self.ctx, mem_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundMem(
                name.as_ref().to_string(),
            ))),
            false => Ok(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the exported [memory](crate::Memory) instance in the registered [module](crate::Module)
    /// by the given memory name and module name.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the registered [module](crate::Module).
    ///
    /// - `mem_name` specifies the name of the exported [memory](crate::Memory) instance.
    ///
    /// # Error
    ///
    /// If fail to get the target registered [memory](crate::Memory), then an error is returned.
    pub fn find_memory_registered(
        &self,
        mod_name: impl AsRef<str>,
        mem_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Memory> {
        let ctx = unsafe {
            let mod_name: WasmEdgeString = mod_name.as_ref().into();
            let mem_name: WasmEdgeString = mem_name.as_ref().into();
            wasmedge::WasmEdge_StoreFindMemoryRegistered(
                self.ctx,
                mod_name.as_raw(),
                mem_name.as_raw(),
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundMemRegistered {
                mem_name: mem_name.as_ref().to_string(),
                mod_name: mod_name.as_ref().to_string(),
            })),
            false => Ok(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the exported [global](crate::Global) instance in the anonymous [module](crate::Module)
    /// by the given global name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [`Store`] as an anonymous module.
    ///
    /// # Argument
    ///
    /// - `name` specifies the target exported [global](crate::Global) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [global](crate::Global), then an error is returned.
    pub fn find_global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let global_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindGlobal(self.ctx, global_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundGlobal(
                name.as_ref().to_string(),
            ))),
            false => Ok(Global {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the exported [global](crate::Global) instance in the registered [module](crate::Module)
    /// by the given global name and module name.
    ///
    /// # Arguments
    ///
    /// - `mod_name` specifies the name of the registered [module](crate::Module).
    ///
    /// - `global_name` specifies the name of the exported [global](crate::Global) instance.
    ///
    /// # Error
    ///
    /// If fail to find the target registered [global](crate::Global), then an error is returned.
    pub fn find_global_registered(
        &self,
        mod_name: impl AsRef<str>,
        global_name: impl AsRef<str>,
    ) -> WasmEdgeResult<Global> {
        let ctx = unsafe {
            let mod_name: WasmEdgeString = mod_name.as_ref().into();
            let global_name: WasmEdgeString = global_name.as_ref().into();
            wasmedge::WasmEdge_StoreFindGlobalRegistered(
                self.ctx,
                mod_name.as_raw(),
                global_name.as_raw(),
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundGlobalRegistered {
                global_name: global_name.as_ref().to_string(),
                mod_name: mod_name.as_ref().to_string(),
            })),
            false => Ok(Global {
                ctx,
                registered: true,
            }),
        }
    }

    /// Returns the length of the exported [functions](crate::Function) in the anonymous module.
    pub fn func_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListFunctionLength(self.ctx) }
    }

    /// Returns the names of the exported [functions](crate::Function) in the anonymous module.
    pub fn func_names(&self) -> Option<Vec<String>> {
        let len_func_names = self.func_len();
        match len_func_names > 0 {
            true => {
                let mut func_names = Vec::with_capacity(len_func_names as usize);
                unsafe {
                    wasmedge::WasmEdge_StoreListFunction(
                        self.ctx,
                        func_names.as_mut_ptr(),
                        len_func_names,
                    );
                    func_names.set_len(len_func_names as usize);
                }

                let names = func_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [functions](crate::Function) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_func_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { wasmedge::WasmEdge_StoreListFunctionRegisteredLength(self.ctx, mod_name.as_raw()) }
    }

    /// Returns the names of the exported [functions](crate::Function) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_func_names(&self, mod_name: impl AsRef<str>) -> Option<Vec<String>> {
        let len_func_names = self.reg_func_len(mod_name.as_ref());
        match len_func_names > 0 {
            true => {
                let mut func_names = Vec::with_capacity(len_func_names as usize);
                let mod_name: WasmEdgeString = mod_name.as_ref().into();
                unsafe {
                    wasmedge::WasmEdge_StoreListFunctionRegistered(
                        self.ctx,
                        mod_name.as_raw(),
                        func_names.as_mut_ptr(),
                        len_func_names,
                    );
                    func_names.set_len(len_func_names as usize);
                };

                let names = func_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [tables](crate::Table) in the anonymous module.
    pub fn table_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListTableLength(self.ctx) }
    }

    /// Returns the names of the exported [tables](crate::Table) in the anonynous module.
    pub fn table_names(&self) -> Option<Vec<String>> {
        let len_table_names = self.table_len();
        match len_table_names > 0 {
            true => {
                let mut table_names = Vec::with_capacity(len_table_names as usize);
                unsafe {
                    wasmedge::WasmEdge_StoreListTable(
                        self.ctx,
                        table_names.as_mut_ptr(),
                        len_table_names,
                    );
                    table_names.set_len(len_table_names as usize);
                }

                let names = table_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [tables](crate::Table) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_table_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { wasmedge::WasmEdge_StoreListTableRegisteredLength(self.ctx, mod_name.as_raw()) }
    }

    /// Returns the names of the exported [tables](crate::Table) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_table_names(&self, mod_name: impl AsRef<str>) -> Option<Vec<String>> {
        let len_table_names = self.reg_table_len(mod_name.as_ref());
        match len_table_names > 0 {
            true => {
                let mut table_names = Vec::with_capacity(len_table_names as usize);
                let mod_name: WasmEdgeString = mod_name.as_ref().into();
                unsafe {
                    wasmedge::WasmEdge_StoreListTableRegistered(
                        self.ctx,
                        mod_name.as_raw(),
                        table_names.as_mut_ptr(),
                        len_table_names,
                    );
                    table_names.set_len(len_table_names as usize);
                };

                let names = table_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [globals](crate::Global) in the anonymous module.
    pub fn global_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListGlobalLength(self.ctx) }
    }

    /// Returns the names of the exported [globals](crate::Global) in the anonymous module.
    pub fn global_names(&self) -> Option<Vec<String>> {
        let len_global_names = self.global_len();
        match len_global_names > 0 {
            true => {
                let mut global_names = Vec::with_capacity(len_global_names as usize);
                unsafe {
                    wasmedge::WasmEdge_StoreListGlobal(
                        self.ctx,
                        global_names.as_mut_ptr(),
                        len_global_names,
                    );
                    global_names.set_len(len_global_names as usize);
                }

                let names = global_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [globals](crate::Global) in the reigstered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_global_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { wasmedge::WasmEdge_StoreListGlobalRegisteredLength(self.ctx, mod_name.as_raw()) }
    }

    /// Returns the names of all exported [globals](crate::Global) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_global_names(&self, mod_name: impl AsRef<str>) -> Option<Vec<String>> {
        let len_global_names = self.reg_global_len(mod_name.as_ref());
        match len_global_names > 0 {
            true => {
                let mut global_names = Vec::with_capacity(len_global_names as usize);
                let mod_name: WasmEdgeString = mod_name.as_ref().into();
                unsafe {
                    wasmedge::WasmEdge_StoreListGlobalRegistered(
                        self.ctx,
                        mod_name.as_raw(),
                        global_names.as_mut_ptr(),
                        len_global_names,
                    );
                    global_names.set_len(len_global_names as usize);
                }

                let names = global_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [memories](crate::Memory) in the anonymous module.
    pub fn mem_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListMemoryLength(self.ctx) }
    }

    /// Returns the names of all exported [memories](crate::Memory) in the anonymous module.
    pub fn mem_names(&self) -> Option<Vec<String>> {
        let len_mem_names = self.mem_len();
        match len_mem_names > 0 {
            true => {
                let mut mem_names = Vec::with_capacity(len_mem_names as usize);
                unsafe {
                    wasmedge::WasmEdge_StoreListMemory(
                        self.ctx,
                        mem_names.as_mut_ptr(),
                        len_mem_names,
                    );
                    mem_names.set_len(len_mem_names as usize);
                }

                let names = mem_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the exported [memories](crate::Memory) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_mem_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { wasmedge::WasmEdge_StoreListMemoryRegisteredLength(self.ctx, mod_name.as_raw()) }
    }

    /// Returns the names of all exported [memories](crate::Memory) in the registered module.
    pub fn reg_mem_names(&self, mod_name: impl AsRef<str>) -> Option<Vec<String>> {
        let len_mem_names = self.reg_mem_len(mod_name.as_ref());
        match len_mem_names > 0 {
            true => {
                let mut mem_names = Vec::with_capacity(len_mem_names as usize);
                let mod_name: WasmEdgeString = mod_name.as_ref().into();
                unsafe {
                    wasmedge::WasmEdge_StoreListMemoryRegistered(
                        self.ctx,
                        mod_name.as_raw(),
                        mem_names.as_mut_ptr(),
                        len_mem_names,
                    );
                    mem_names.set_len(len_mem_names as usize);
                }

                let names = mem_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }

    /// Returns the length of the registered [modules](crate::Module).
    pub fn reg_module_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListModuleLength(self.ctx) }
    }

    /// Returns the names of all registered [modules](crate::Module).
    pub fn reg_module_names(&self) -> Option<Vec<String>> {
        let len_mod_names = self.reg_module_len();
        match len_mod_names > 0 {
            true => {
                let mut mod_names = Vec::with_capacity(len_mod_names as usize);
                unsafe {
                    wasmedge::WasmEdge_StoreListModule(
                        self.ctx,
                        mod_names.as_mut_ptr(),
                        len_mod_names,
                    );
                    mod_names.set_len(len_mod_names as usize);
                };

                let names = mod_names
                    .into_iter()
                    .map(|x| x.into())
                    .collect::<Vec<String>>();
                Some(names)
            }
            false => None,
        }
    }
}
impl Drop for Store {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_StoreDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Store;
    use crate::{
        instance::{Function, Global, GlobalType, MemType, Memory, Table, TableType},
        Config, Executor, FuncType, ImportObj, Mutability, RefType, ValType, Value,
    };

    #[test]
    fn test_store_basic() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();
        assert!(!store.ctx.is_null());
        assert!(!store.registered);

        // check the length of registered module list in store before instatiation
        assert_eq!(store.func_len(), 0);
        assert_eq!(store.reg_func_len(module_name), 0);
        assert_eq!(store.table_len(), 0);
        assert_eq!(store.reg_table_len(module_name), 0);
        assert_eq!(store.global_len(), 0);
        assert_eq!(store.reg_global_len(module_name), 0);
        assert_eq!(store.mem_len(), 0);
        assert_eq!(store.reg_mem_len(module_name), 0);
        assert_eq!(store.reg_module_len(), 0);
        assert!(store.reg_module_names().is_none());

        // create ImportObject instance
        let result = ImportObj::create(module_name);
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let mut host_func = result.unwrap();
        import_obj.add_func("add", &mut host_func);
        assert!(host_func.ctx.is_null() && host_func.registered);

        // add table
        let result = TableType::create(RefType::FuncRef, 0..=u32::MAX);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);
        let result = Table::create(&mut ty);
        assert!(result.is_ok());
        let mut table = result.unwrap();
        import_obj.add_table("table", &mut table);
        assert!(table.ctx.is_null() && table.registered);

        // add memory
        let result = MemType::create(0..=u32::MAX);
        assert!(result.is_ok());
        let mut mem_ty = result.unwrap();
        let result = Memory::create(&mut mem_ty);
        assert!(result.is_ok());
        let mut memory = result.unwrap();
        import_obj.add_memory("mem", &mut memory);
        assert!(memory.ctx.is_null() && memory.registered);

        // add globals
        let result = GlobalType::create(ValType::F32, Mutability::Const);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        let result = Global::create(&mut ty, Value::F32(3.5));
        assert!(result.is_ok());
        let mut global = result.unwrap();
        import_obj.add_global("global", &mut global);
        assert!(global.ctx.is_null() && global.registered);

        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let result = Executor::create(Some(&config), None);
        assert!(result.is_ok());
        let executor = result.unwrap();
        let result = executor.register_import_object(&mut store, &import_obj);
        assert!(result.is_ok());
        let executor = result.unwrap();

        // check the module list after instantiation
        assert_eq!(store.reg_module_len(), 1);
        assert!(store.reg_module_names().is_some());
        assert_eq!(store.reg_module_names().unwrap()[0], module_name);
        assert_eq!(store.func_len(), 0);
        assert_eq!(store.reg_func_len(module_name), 1);
        assert!(store.reg_func_names(module_name).is_some());
        assert_eq!(store.reg_func_names(module_name).unwrap()[0], "add");
        assert_eq!(store.table_len(), 0);
        assert_eq!(store.reg_table_len(module_name), 1);
        assert!(store.reg_table_names(module_name).is_some());
        assert_eq!(store.reg_table_names(module_name).unwrap()[0], "table");
        assert_eq!(store.global_len(), 0);
        assert_eq!(store.reg_global_len(module_name), 1);
        assert!(store.reg_global_names(module_name).is_some());
        assert_eq!(store.reg_global_names(module_name).unwrap()[0], "global");
        assert_eq!(store.mem_len(), 0);
        assert_eq!(store.reg_mem_len(module_name), 1);
        assert!(store.reg_mem_names(module_name).is_some());
        assert_eq!(store.reg_mem_names(module_name).unwrap()[0], "mem");

        // check the function list after instantiation
        let result = store.find_func("add");
        assert!(result.is_err());
        let result = store.find_func_registered("extern_module", "add");
        assert!(result.is_ok());

        // check the table list after instantiation
        let result = store.find_table("table");
        assert!(result.is_err());
        let result = store.find_table_registered("extern_module", "table");
        assert!(result.is_ok());

        // check the memory list after instantiation
        let result = store.find_memory("mem");
        assert!(result.is_err());
        let result = store.find_memory_registered("extern_module", "mem");
        assert!(result.is_ok());

        // check the global list after instantiation
        let result = store.find_global("global");
        assert!(result.is_err());
        let result = store.find_global_registered("extern_module", "global");
        assert!(result.is_ok());
        let global = result.unwrap();
        assert!(!global.ctx.is_null() && global.registered);
        let val = global.get_value();
        assert_eq!(val, Value::F32(3.5));

        // run the registered function
        let result = executor.run_func_registered(
            &store,
            "extern_module",
            "add",
            vec![Value::I32(12), Value::I32(21)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns, vec![Value::I32(33)]);

        let second_run = executor.run_func_registered(
            &store,
            "extern_module",
            "add",
            vec![Value::I32(12), Value::I32(21)],
        );
        assert!(second_run.is_ok());
    }

    fn real_add(input: Vec<Value>) -> Result<Vec<Value>, u8> {
        println!("Rust: Entering Rust function real_add");

        if input.len() != 2 {
            return Err(1);
        }

        let a = if let Value::I32(i) = input[0] {
            i
        } else {
            return Err(2);
        };

        let b = if let Value::I32(i) = input[1] {
            i
        } else {
            return Err(3);
        };

        let c = a + b;
        println!("Rust: calcuating in real_add c: {:?}", c);

        println!("Rust: Leaving Rust function real_add");
        Ok(vec![Value::I32(c)])
    }
}
