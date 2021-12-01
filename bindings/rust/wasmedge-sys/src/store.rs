use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    string::StringRef,
};

#[derive(Debug)]
pub struct Store {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StoreContext,
    pub(crate) registered: bool,
}
impl Store {
    pub fn create() -> Option<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_StoreCreate() };
        match ctx.is_null() {
            true => None,
            false => Some(Store {
                ctx,
                registered: false,
            }),
        }
    }

    pub fn find_func(&self, name: impl AsRef<str>) -> Option<Function> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindFunction(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Function {
                ctx,
                registered: true,
                ty: None,
            }),
        }
    }

    pub fn find_func_registered(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> Option<Function> {
        // let mod_name = WasmEdgeString::from(mod_name.as_ref());
        // let func_name = WasmEdgeString::from(func_name.as_ref());
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindFunctionRegistered(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                wasmedge::WasmEdge_String::from(StringRef::from(func_name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Function {
                ctx,
                registered: true,
                ty: None,
            }),
        }
    }

    pub fn find_table(&self, name: impl AsRef<str>) -> Option<Table> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindTable(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Table {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_table_registered(
        &self,
        mod_name: impl AsRef<str>,
        table_name: impl AsRef<str>,
    ) -> Option<Table> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindTableRegistered(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                wasmedge::WasmEdge_String::from(StringRef::from(table_name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Table {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_memory(&self, name: impl AsRef<str>) -> Option<Memory> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindMemory(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_memory_registered(
        &self,
        mod_name: impl AsRef<str>,
        mem_name: impl AsRef<str>,
    ) -> Option<Memory> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindMemoryRegistered(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                wasmedge::WasmEdge_String::from(StringRef::from(mem_name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_global(&self, name: impl AsRef<str>) -> Option<Global> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindGlobal(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Global {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_global_registered(
        &self,
        mod_name: impl AsRef<str>,
        global_name: impl AsRef<str>,
    ) -> Option<Global> {
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindGlobalRegistered(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
                wasmedge::WasmEdge_String::from(StringRef::from(global_name.as_ref())),
            )
        };
        match ctx.is_null() {
            true => None,
            false => Some(Global {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn list_func_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListFunctionLength(self.ctx) }
    }

    pub fn list_function(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_func_names = self.list_func_len();
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

    pub fn list_func_registered_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe {
            wasmedge::WasmEdge_StoreListFunctionRegisteredLength(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            )
        }
    }

    pub fn list_func_registered(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_func_names = self.list_func_registered_len(mod_name.as_ref());
        if len_func_names > 0 {
            let mut func_names = Vec::with_capacity(len_func_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListFunctionRegistered(
                    self.ctx,
                    wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
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

    pub fn list_table_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListTableLength(self.ctx) }
    }

    pub fn list_table(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_table_names = self.list_table_len();
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

    pub fn list_table_registered_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe {
            wasmedge::WasmEdge_StoreListTableRegisteredLength(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            )
        }
    }

    pub fn list_table_registered(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_table_names = self.list_table_registered_len(mod_name.as_ref());
        if len_table_names > 0 {
            let mut table_names = Vec::with_capacity(len_table_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListTableRegistered(
                    self.ctx,
                    wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
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

    pub fn list_global_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListGlobalLength(self.ctx) }
    }

    pub fn list_global(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_global_names = self.list_global_len();
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

    pub fn list_global_registered_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe {
            wasmedge::WasmEdge_StoreListGlobalRegisteredLength(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            )
        }
    }

    pub fn list_global_registered(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_global_names = self.list_global_registered_len(mod_name.as_ref());
        if len_global_names > 0 {
            let mut global_names = Vec::with_capacity(len_global_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListGlobalRegistered(
                    self.ctx,
                    wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
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

    pub fn list_memory_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListMemoryLength(self.ctx) }
    }

    pub fn list_memory(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mem_names = self.list_memory_len();
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

    pub fn list_memory_registered_len(&self, mod_name: impl AsRef<str>) -> u32 {
        unsafe {
            wasmedge::WasmEdge_StoreListMemoryRegisteredLength(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
            )
        }
    }

    pub fn list_memory_registered(&self, mod_name: impl AsRef<str>) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mem_names = self.list_memory_registered_len(mod_name.as_ref());
        if len_mem_names > 0 {
            let mut mem_names = Vec::with_capacity(len_mem_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListMemoryRegistered(
                    self.ctx,
                    wasmedge::WasmEdge_String::from(StringRef::from(mod_name.as_ref())),
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

    pub fn list_module_len(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_StoreListModuleLength(self.ctx) }
    }

    pub fn list_module(&self) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mod_names = self.list_module_len();
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
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_StoreDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Store;
    use crate::{
        instance::{Function, Global, GlobalType, Memory, Table},
        io::{I1, I2},
        types::{Limit, Mutability, ValType, WasmEdgeRefType},
        Config, Executor, ImportObj, Value,
    };

    #[test]
    fn test_store_basic() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_some());
        let mut store = result.unwrap();
        assert!(!store.ctx.is_null() && !store.registered);

        // check the length of registered module list in store before instatiation
        assert_eq!(store.list_func_len(), 0);
        assert_eq!(store.list_func_registered_len(module_name), 0);
        assert_eq!(store.list_table_len(), 0);
        assert_eq!(store.list_table_registered_len(module_name), 0);
        assert_eq!(store.list_global_len(), 0);
        assert_eq!(store.list_global_registered_len(module_name), 0);
        assert_eq!(store.list_memory_len(), 0);
        assert_eq!(store.list_memory_registered_len(module_name), 0);
        assert_eq!(store.list_module_len(), 0);
        assert_eq!(store.list_module().len(), 0);

        // create ImportObject instance
        let result = ImportObj::create(module_name);
        assert!(result.is_some());
        let mut import_obj = result.unwrap();

        // add host function
        let mut host_func = Function::create_bindings::<I2<i32, i32>, I1<i32>>(Box::new(real_add));
        import_obj.add_func("add", &mut host_func);
        assert!(host_func.ctx.is_null() && host_func.registered);

        // add table
        let ref_ty = WasmEdgeRefType::FuncRef;
        let limit = Limit::create(0, None);
        let result = Table::create(ref_ty, limit);
        assert!(result.is_some());
        let mut table = result.unwrap();
        import_obj.add_table("table", &mut table);
        assert!(table.ctx.is_null() && table.registered);

        // add memory
        let limit = Limit::create(0, None);
        let result = Memory::create(limit);
        assert!(result.is_some());
        let mut memory = result.unwrap();
        import_obj.add_memory("mem", &mut memory);
        assert!(memory.ctx.is_null() && memory.registered);

        // add globals
        let result = GlobalType::create(ValType::F32, Mutability::Const);
        assert!(result.is_some());
        let mut ty = result.unwrap();
        let result = Global::create(&mut ty, Value::F32(3.5));
        assert!(result.is_some());
        let mut global = result.unwrap();
        import_obj.add_global("global", &mut global);
        assert!(global.ctx.is_null() && global.registered);

        let config = Config::default();
        let result = Executor::create(Some(&config), None);
        assert!(result.is_some());
        let executor = result.unwrap();
        let result = executor.register_import_object_module(&mut store, &import_obj);
        assert!(result.is_ok());

        // check the module list after instantiation
        assert_eq!(store.list_module_len(), 1);
        assert_eq!(store.list_module()[0], module_name);
        assert_eq!(store.list_func_len(), 0);
        assert_eq!(store.list_func_registered_len(module_name), 1);
        assert_eq!(store.list_func_registered(module_name)[0], "add");
        assert_eq!(store.list_table_len(), 0);
        assert_eq!(store.list_table_registered_len(module_name), 1);
        assert_eq!(store.list_table_registered(module_name)[0], "table");
        assert_eq!(store.list_global_len(), 0);
        assert_eq!(store.list_global_registered_len(module_name), 1);
        assert_eq!(store.list_global_registered(module_name)[0], "global");
        assert_eq!(store.list_memory_len(), 0);
        assert_eq!(store.list_memory_registered_len(module_name), 1);
        assert_eq!(store.list_memory_registered(module_name)[0], "mem");

        // check the function list after instantiation
        let result = store.find_func("add");
        assert!(result.is_none());
        let result = store.find_func_registered("extern_module", "add");
        assert!(result.is_some());

        // check the table list after instantiation
        let result = store.find_table("table");
        assert!(result.is_none());
        let result = store.find_table_registered("extern_module", "table");
        assert!(result.is_some());

        // check the memory list after instantiation
        let result = store.find_memory("mem");
        assert!(result.is_none());
        let result = store.find_memory_registered("extern_module", "mem");
        assert!(result.is_some());

        // check the global list after instantiation
        let result = store.find_global("global");
        assert!(result.is_none());
        let result = store.find_global_registered("extern_module", "global");
        assert!(result.is_some());
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
