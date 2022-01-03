//! Defines WasmEdge Store struct.

use crate::{
    instance::{Function, Global, Memory, Table},
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
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindFunction(self.ctx, name.as_ref().into()) };
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
            wasmedge::WasmEdge_StoreFindFunctionRegistered(
                self.ctx,
                mod_name.as_ref().into(),
                func_name.as_ref().into(),
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
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindTable(self.ctx, name.as_ref().into()) };
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
            wasmedge::WasmEdge_StoreFindTableRegistered(
                self.ctx,
                mod_name.as_ref().into(),
                table_name.as_ref().into(),
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
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindMemory(self.ctx, name.as_ref().into()) };
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
            wasmedge::WasmEdge_StoreFindMemoryRegistered(
                self.ctx,
                mod_name.as_ref().into(),
                mem_name.as_ref().into(),
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
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindGlobal(self.ctx, name.as_ref().into()) };
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
            wasmedge::WasmEdge_StoreFindGlobalRegistered(
                self.ctx,
                mod_name.as_ref().into(),
                global_name.as_ref().into(),
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

    /// Returns an iterator of names of the exported [functions](crate::Function) in the anonymous module.
    pub fn func_names_iter(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_func_names = self.func_len();
        if len_func_names > 0 {
            let mut func_names = Vec::with_capacity(len_func_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListFunction(
                    self.ctx,
                    func_names.as_mut_ptr(),
                    len_func_names,
                );
                func_names.set_len(len_func_names as usize);
            }

            for func_name in func_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(func_name.Buf as *const _) };
                let name = slice.to_string_lossy().into_owned();
                names.push(name);
            }
        }
        names
    }

    /// Returns the length of the exported [functions](crate::Function) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_func_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListFunctionRegisteredLength(self.ctx, mod_name.into()) }
    }

    /// Returns an iterator of names of the exported [functions](crate::Function) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_func_names_iter(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_func_names = self.reg_func_len(mod_name.as_ref());
        if len_func_names > 0 {
            let mut func_names = Vec::with_capacity(len_func_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListFunctionRegistered(
                    self.ctx,
                    mod_name.into(),
                    func_names.as_mut_ptr(),
                    len_func_names,
                );
                func_names.set_len(len_func_names as usize);
            };

            for func_name in func_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(func_name.Buf as *const _) };
                let name = slice.to_string_lossy().into_owned();
                names.push(name);
            }
        }

        names
    }

    /// Returns the length of the exported [tables](crate::Table) in the anonymous module.
    pub fn table_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListTableLength(self.ctx) }
    }

    /// Returns an iterator of names of the exported [tables](crate::Table) in the anonynous module.
    pub fn table_names_iter(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_table_names = self.table_len();
        if len_table_names > 0 {
            let mut table_names = Vec::with_capacity(len_table_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListTable(
                    self.ctx,
                    table_names.as_mut_ptr(),
                    len_table_names,
                );
                table_names.set_len(len_table_names as usize);
            }

            for table_name in table_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(table_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the exported [tables](crate::Table) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_table_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListTableRegisteredLength(self.ctx, mod_name.into()) }
    }

    /// Returns an iterator of names of the exported [tables](crate::Table) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_table_names_iter(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_table_names = self.reg_table_len(mod_name.as_ref());
        if len_table_names > 0 {
            let mut table_names = Vec::with_capacity(len_table_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListTableRegistered(
                    self.ctx,
                    mod_name.into(),
                    table_names.as_mut_ptr(),
                    len_table_names,
                );
                table_names.set_len(len_table_names as usize);
            };

            for table_name in table_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(table_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the exported [globals](crate::Global) in the anonymous module.
    pub fn global_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListGlobalLength(self.ctx) }
    }

    /// Returns an iterator of names of the exported [globals](crate::Global) in the anonymous module.
    pub fn global_names_iter(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_global_names = self.global_len();
        if len_global_names > 0 {
            let mut global_names = Vec::with_capacity(len_global_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListGlobal(
                    self.ctx,
                    global_names.as_mut_ptr(),
                    len_global_names,
                );
                global_names.set_len(len_global_names as usize);
            }

            for global_name in global_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(global_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the exported [globals](crate::Global) in the reigstered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_global_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListGlobalRegisteredLength(self.ctx, mod_name.into()) }
    }

    /// Returns an iterator of names of all exported [globals](crate::Global) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_global_names_iter(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_global_names = self.reg_global_len(mod_name.as_ref());
        if len_global_names > 0 {
            let mut global_names = Vec::with_capacity(len_global_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListGlobalRegistered(
                    self.ctx,
                    mod_name.into(),
                    global_names.as_mut_ptr(),
                    len_global_names,
                );
                global_names.set_len(len_global_names as usize);
            }

            for global_name in global_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(global_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the exported [memories](crate::Memory) in the anonymous module.
    pub fn mem_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListMemoryLength(self.ctx) }
    }

    /// Returns an iterator of names of all exported [memories](crate::Memory) in the anonymous module.
    pub fn mem_names_iter(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mem_names = self.mem_len();
        if len_mem_names > 0 {
            let mut mem_names = Vec::with_capacity(len_mem_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListMemory(self.ctx, mem_names.as_mut_ptr(), len_mem_names);
                mem_names.set_len(len_mem_names as usize);
            }

            for mem_name in mem_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(mem_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the exported [memories](crate::Memory) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_mem_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListMemoryRegisteredLength(self.ctx, mod_name.into()) }
    }

    /// Returns an iterator of names of all exported [memories](crate::Memory) in the registered module.
    pub fn reg_mem_names_iter(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mem_names = self.reg_mem_len(mod_name.as_ref());
        if len_mem_names > 0 {
            let mut mem_names = Vec::with_capacity(len_mem_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListMemoryRegistered(
                    self.ctx,
                    mod_name.into(),
                    mem_names.as_mut_ptr(),
                    len_mem_names,
                );
                mem_names.set_len(len_mem_names as usize);
            }

            for mem_name in mem_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(mem_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
    }

    /// Returns the length of the registered [modules](crate::Module).
    pub fn reg_module_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListModuleLength(self.ctx) }
    }

    /// Returns the names of all registered [modules](crate::Module).
    pub fn reg_module_name_iter(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mod_names = self.reg_module_len();
        if len_mod_names > 0 {
            let mut mod_names = Vec::with_capacity(len_mod_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListModule(self.ctx, mod_names.as_mut_ptr(), len_mod_names);
                mod_names.set_len(len_mod_names as usize);
            };

            for mod_name in mod_names {
                let slice = unsafe { std::ffi::CStr::from_ptr(mod_name.Buf as *const _) };
                names.push(slice.to_string_lossy().into_owned());
            }
        }
        names
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
        instance::{Function, Global, GlobalType, Memory, Table, TableType},
        io::{I1, I2},
        types::{Mutability, RefType, ValType},
        Config, Executor, ImportObj, Value,
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
        assert_eq!(store.reg_module_name_iter().len(), 0);

        // create ImportObject instance
        let result = ImportObj::create(module_name);
        assert!(result.is_ok());
        let mut import_obj = result.unwrap();

        // add host function
        let result = Function::create_bindings::<I2<i32, i32>, I1<i32>>(Box::new(real_add));
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
        let result = Memory::create(0..=u32::MAX);
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

        // check the module list after instantiation
        assert_eq!(store.reg_module_len(), 1);
        assert_eq!(store.reg_module_name_iter()[0], module_name);
        assert_eq!(store.func_len(), 0);
        assert_eq!(store.reg_func_len(module_name), 1);
        assert_eq!(store.reg_func_names_iter(module_name)[0], "add");
        assert_eq!(store.table_len(), 0);
        assert_eq!(store.reg_table_len(module_name), 1);
        assert_eq!(store.reg_table_names_iter(module_name)[0], "table");
        assert_eq!(store.global_len(), 0);
        assert_eq!(store.reg_global_len(module_name), 1);
        assert_eq!(store.reg_global_names_iter(module_name)[0], "global");
        assert_eq!(store.mem_len(), 0);
        assert_eq!(store.reg_mem_len(module_name), 1);
        assert_eq!(store.reg_mem_names_iter(module_name)[0], "mem");

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
        let val = val.as_f32();
        assert!(val.is_some());
        assert!((val.unwrap() - 3.5).abs() < f32::EPSILON);
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
