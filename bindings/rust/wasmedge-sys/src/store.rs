use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    types::WasmEdgeString,
};

#[derive(Debug)]
pub struct Store {
    pub(crate) ctx: *mut wasmedge::WasmEdge_StoreContext,
}
impl Store {
    pub fn create() -> Option<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_StoreCreate() };
        match ctx.is_null() {
            true => None,
            false => Some(Store { ctx }),
        }
    }

    pub fn find_func(&self, func_name: &str) -> Option<Function> {
        let func_name = WasmEdgeString::from_str(func_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", func_name).as_str());
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindFunction(self.ctx, func_name.ctx) };
        match ctx.is_null() {
            true => None,
            false => Some(Function {
                ctx,
                registered: true,
                ty: None,
            }),
        }
    }

    pub fn find_func_registered(&self, mod_name: &str, func_name: &str) -> Option<Function> {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        let func_name = WasmEdgeString::from_str(func_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", func_name).as_str());
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindFunctionRegistered(self.ctx, mod_name.ctx, func_name.ctx)
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

    pub fn find_table(&self, table_name: &str) -> Option<Table> {
        let table_name = WasmEdgeString::from_str(table_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", table_name).as_str());
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindTable(self.ctx, table_name.ctx) };
        match ctx.is_null() {
            true => None,
            false => Some(Table {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_table_registered(&self, mod_name: &str, table_name: &str) -> Option<Table> {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        let table_name = WasmEdgeString::from_str(table_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", table_name).as_str());
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindTableRegistered(self.ctx, mod_name.ctx, table_name.ctx)
        };
        match ctx.is_null() {
            true => None,
            false => Some(Table {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_memory(&self, mem_name: &str) -> Option<Memory> {
        let mem_name = WasmEdgeString::from_str(mem_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mem_name).as_str());
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindMemory(self.ctx, mem_name.ctx) };
        match ctx.is_null() {
            true => None,
            false => Some(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_memory_registered(&self, mod_name: &str, mem_name: &str) -> Option<Memory> {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        let mem_name = WasmEdgeString::from_str(mem_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mem_name).as_str());
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindMemoryRegistered(self.ctx, mod_name.ctx, mem_name.ctx)
        };
        match ctx.is_null() {
            true => None,
            false => Some(Memory {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_global(&self, global_name: &str) -> Option<Global> {
        let global_name = WasmEdgeString::from_str(global_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", global_name).as_str());
        let ctx = unsafe { wasmedge::WasmEdge_StoreFindGlobal(self.ctx, global_name.ctx) };
        match ctx.is_null() {
            true => None,
            false => Some(Global {
                ctx,
                registered: true,
            }),
        }
    }

    pub fn find_global_registered(&self, mod_name: &str, global_name: &str) -> Option<Global> {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        let global_name = WasmEdgeString::from_str(global_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", global_name).as_str());
        let ctx = unsafe {
            wasmedge::WasmEdge_StoreFindGlobalRegistered(self.ctx, mod_name.ctx, global_name.ctx)
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

    pub fn list_func_registered_len(&self, mod_name: &str) -> u32 {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        unsafe { wasmedge::WasmEdge_StoreListFunctionRegisteredLength(self.ctx, mod_name.ctx) }
    }

    pub fn list_func_registered(&self, mod_name: &str) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_func_names = self.list_func_registered_len(mod_name);
        if len_func_names > 0 {
            let mut func_names = Vec::with_capacity(len_func_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListFunctionRegistered(
                    self.ctx,
                    WasmEdgeString::from(mod_name).ctx,
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

    pub fn list_table_registered_len(&self, mod_name: &str) -> u32 {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        unsafe { wasmedge::WasmEdge_StoreListTableRegisteredLength(self.ctx, mod_name.ctx) }
    }

    pub fn list_table_registered(&self, mod_name: &str) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_table_names = self.list_table_registered_len(mod_name);
        if len_table_names > 0 {
            let mut table_names = Vec::with_capacity(len_table_names as usize);
            let mod_name = WasmEdgeString::from_str(mod_name)
                .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
            unsafe {
                wasmedge::WasmEdge_StoreListTableRegistered(
                    self.ctx,
                    WasmEdgeString::from(mod_name).ctx,
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

    pub fn list_global_registered_len(&self, mod_name: &str) -> u32 {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        unsafe { wasmedge::WasmEdge_StoreListGlobalRegisteredLength(self.ctx, mod_name.ctx) }
    }

    pub fn list_global_registered(&self, mod_name: &str) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_global_names = self.list_global_registered_len(mod_name);
        if len_global_names > 0 {
            let mut global_names = Vec::with_capacity(len_global_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListGlobalRegistered(
                    self.ctx,
                    WasmEdgeString::from(mod_name).ctx,
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

    pub fn list_memory_registered_len(&self, mod_name: &str) -> u32 {
        let mod_name = WasmEdgeString::from_str(mod_name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", mod_name).as_str());
        unsafe { wasmedge::WasmEdge_StoreListMemoryRegisteredLength(self.ctx, mod_name.ctx) }
    }

    pub fn list_memory_registered(&self, mod_name: &str) -> Vec<String> {
        let mut names: Vec<String> = vec![];
        let len_mem_names = self.list_memory_registered_len(mod_name);
        if len_mem_names > 0 {
            let mut mem_names = Vec::with_capacity(len_mem_names as usize);
            unsafe {
                wasmedge::WasmEdge_StoreListMemoryRegistered(
                    self.ctx,
                    WasmEdgeString::from(mod_name).ctx,
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
    fn test_store_create() {
        let result = Store::create();
        assert!(result.is_some());
        let store = result.unwrap();

        // check the length of registered module list in store before instatiation
        assert_eq!(store.list_module_len(), 0);
    }

    #[test]
    fn test_store_list_module() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_some());
        let mut store = result.unwrap();

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

        // add table
        let ref_ty = WasmEdgeRefType::FuncRef;
        let limit = Limit::create(0, None);
        let result = Table::create(ref_ty, limit);
        assert!(result.is_some());
        let mut table = result.unwrap();
        import_obj.add_table("table", &mut table);

        // add memory
        let limit = Limit::create(0, None);
        let result = Memory::create(limit);
        assert!(result.is_some());
        let mut memory = result.unwrap();
        import_obj.add_memory("mem", &mut memory);

        // add globals
        let result = GlobalType::create(ValType::F32, Mutability::Const);
        assert!(result.is_some());
        let mut ty = result.unwrap();
        let result = Global::create(&mut ty, Value::F32(3.14));
        assert!(result.is_some());
        let mut global = result.unwrap();
        import_obj.add_global("global", &mut global);

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
