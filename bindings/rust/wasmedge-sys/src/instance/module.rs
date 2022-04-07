//! Defines WasmEdge Instancestruct.

use crate::{
    error::{InstanceError, WasmEdgeError},
    ffi,
    instance::{function::InnerFunc, global::InnerGlobal, memory::InnerMemory, table::InnerTable},
    types::WasmEdgeString,
    Function, Global, Memory, Store, Table, WasmEdgeResult,
};

/// Struct of WasmEdge Instance.
///
/// An [Instance] represents an instantiated module. In the instantiation process, An [Instance] is created from al[Module](crate::Module). From an [Instance] the exported [functions](crate::Function), [tables](crate::Table), [memories](crate::Memory), and [globals](crate::Global) can be fetched.
#[derive(Debug)]
pub struct Instance {
    pub(crate) inner: InnerInstance,
    pub(crate) registered: bool,
}
impl Drop for Instance {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe {
                ffi::WasmEdge_ModuleInstanceDelete(self.inner.0);
            }
        }
    }
}
impl Instance {
    pub fn create(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_ModuleInstanceCreate(name.as_ref().into()) };

        match ctx.is_null() {
            true => Err(WasmEdgeError::InstanceError(InstanceError::Create)),
            false => Ok(Instance {
                inner: InnerInstance(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the name of this exported [module instance](crate::Instance).
    ///
    /// If this module [instance](crate::Instance) is an active [instance](crate::Instance), return None.
    pub fn name(&self) -> Option<String> {
        let name = unsafe { ffi::WasmEdge_ModuleInstanceGetModuleName(self.inner.0 as *const _) };

        let name: String = name.into();
        if name.is_empty() {
            return None;
        }

        Some(name)
    }

    /// Returns the exported [function](crate::Function) instance in the this [module instance](crate::Instance) by the given function name.
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
        let func_ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceFindFunction(self.inner.0 as *const _, func_name.as_raw())
        };
        match func_ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::NotFoundFunc(
                name.as_ref().to_string(),
            ))),
            false => Ok(Function {
                inner: InnerFunc(func_ctx),
                registered: true,
                name: Some(name.as_ref().to_string()),
                mod_name: self.name(),
            }),
        }
    }

    /// Returns the exported [table](crate::Table) instance in this [module instance](crate::Instance)
    /// by the given table name.
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
        let ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceFindTable(self.inner.0 as *const _, table_name.as_raw())
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::NotFoundTable(
                name.as_ref().to_string(),
            ))),
            false => Ok(Table {
                inner: InnerTable(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the exported [memory](crate::Memory) instance in the [module instance](crate::Instance)
    /// by the given memory name.
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
        let ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceFindMemory(self.inner.0 as *const _, mem_name.as_raw())
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::NotFoundMem(
                name.as_ref().to_string(),
            ))),
            false => Ok(Memory {
                inner: InnerMemory(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the exported [global](crate::Global) instance in the [module instance](crate::Instance)
    /// by the given global name.
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
        let ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceFindGlobal(self.inner.0 as *const _, global_name.as_raw())
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::NotFoundGlobal(
                name.as_ref().to_string(),
            ))),
            false => Ok(Global {
                inner: InnerGlobal(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the length of the exported [functions](crate::Function) in this module.
    pub fn func_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_ModuleInstanceListFunctionLength(self.inner.0) }
    }

    /// Returns the names of the exported [functions](crate::Function) in this module.
    pub fn func_names(&self) -> Option<Vec<String>> {
        let len_func_names = self.func_len();
        match len_func_names > 0 {
            true => {
                let mut func_names = Vec::with_capacity(len_func_names as usize);
                unsafe {
                    ffi::WasmEdge_ModuleInstanceListFunction(
                        self.inner.0,
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

    /// Returns the length of the exported [tables](crate::Table) in this module.
    pub fn table_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_ModuleInstanceListTableLength(self.inner.0) }
    }

    /// Returns the names of the exported [tables](crate::Table) in this module.
    pub fn table_names(&self) -> Option<Vec<String>> {
        let len_table_names = self.table_len();
        match len_table_names > 0 {
            true => {
                let mut table_names = Vec::with_capacity(len_table_names as usize);
                unsafe {
                    ffi::WasmEdge_ModuleInstanceListTable(
                        self.inner.0,
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

    /// Returns the length of the exported [memories](crate::Memory) in this module.
    pub fn mem_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_ModuleInstanceListMemoryLength(self.inner.0) }
    }

    /// Returns the names of all exported [memories](crate::Memory) in this module.
    pub fn mem_names(&self) -> Option<Vec<String>> {
        let len_mem_names = self.mem_len();
        match len_mem_names > 0 {
            true => {
                let mut mem_names = Vec::with_capacity(len_mem_names as usize);
                unsafe {
                    ffi::WasmEdge_ModuleInstanceListMemory(
                        self.inner.0,
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

    /// Returns the length of the exported [globals](crate::Global) in this module.
    pub fn global_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_ModuleInstanceListGlobalLength(self.inner.0) }
    }

    /// Returns the names of the exported [globals](crate::Global) in this module.
    pub fn global_names(&self) -> Option<Vec<String>> {
        let len_global_names = self.global_len();
        match len_global_names > 0 {
            true => {
                let mut global_names = Vec::with_capacity(len_global_names as usize);
                unsafe {
                    ffi::WasmEdge_ModuleInstanceListGlobal(
                        self.inner.0,
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
}

#[derive(Debug)]
pub(crate) struct InnerInstance(pub(crate) *const ffi::WasmEdge_ModuleInstanceContext);
unsafe impl Send for InnerInstance {}
unsafe impl Sync for InnerInstance {}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::{Config, Executor, FuncType, GlobalType, MemType, TableType, Vm, WasmValue};
//     use wasmedge_types::{Mutability, RefType, ValType};

//     #[test]
//     fn test_instance_find_xxx() {
//         let vm = create_vm();
//         let result = vm.store_mut();
//         assert!(result.is_ok());
//         let mut store = result.unwrap();

//         // get the module named "extern"
//         let result = store.named_module("extern_module");
//         assert!(result.is_ok());
//         let instance = result.unwrap();

//         // check the name of the module
//         assert!(instance.name().is_some());
//         assert_eq!(instance.name().unwrap(), "extern_module");

//         // get the exported function named "fib"
//         let result = instance.find_func("add");
//         assert!(result.is_ok());
//         let func = result.unwrap();

//         // check the type of the function
//         let result = func.ty();
//         assert!(result.is_ok());
//         let ty = result.unwrap();

//         // check the parameter types
//         let param_types = ty.params_type_iter().collect::<Vec<ValType>>();
//         assert_eq!(param_types, [ValType::I32, ValType::I32]);

//         // check the return types
//         let return_types = ty.returns_type_iter().collect::<Vec<ValType>>();
//         assert_eq!(return_types, [ValType::I32]);

//         // get the exported table named "table"
//         let result = instance.find_table("table");
//         assert!(result.is_ok());
//         let table = result.unwrap();

//         // check the type of the table
//         let result = table.ty();
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         assert_eq!(ty.elem_ty(), RefType::FuncRef);
//         assert_eq!(ty.limit(), 0..=u32::MAX);

//         // get the exported memory named "mem"
//         let result = instance.find_memory("mem");
//         assert!(result.is_ok());
//         let memory = result.unwrap();

//         // check the type of the memory
//         let result = memory.ty();
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         assert_eq!(ty.limit(), 0..=u32::MAX);

//         // get the exported global named "global"
//         let result = instance.find_global("global");
//         assert!(result.is_ok());
//         let global = result.unwrap();

//         // check the type of the global
//         let result = global.ty();
//         assert!(result.is_ok());
//         let global = result.unwrap();
//         assert_eq!(global.value_type(), ValType::F32);
//         assert_eq!(global.mutability(), Mutability::Const);
//     }

//     #[test]
//     fn test_instance_find_names() {
//         let vm = create_vm();
//         let result = vm.store_mut();
//         assert!(result.is_ok());
//         let mut store = result.unwrap();

//         // get the module named "extern"
//         let result = store.named_module("extern_module");
//         assert!(result.is_ok());
//         let instance = result.unwrap();

//         // check the name of the module
//         assert!(instance.name().is_some());
//         assert_eq!(instance.name().unwrap(), "extern_module");

//         assert_eq!(instance.func_len(), 1);
//         let result = instance.func_names();
//         assert!(result.is_some());
//         assert_eq!(result.unwrap(), ["add"]);

//         assert_eq!(instance.table_len(), 1);
//         let result = instance.table_names();
//         assert!(result.is_some());
//         assert_eq!(result.unwrap(), ["table"]);

//         assert_eq!(instance.mem_len(), 1);
//         let result = instance.mem_names();
//         assert!(result.is_some());
//         assert_eq!(result.unwrap(), ["mem"]);

//         assert_eq!(instance.global_len(), 1);
//         let result = instance.global_names();
//         assert!(result.is_some());
//         assert_eq!(result.unwrap(), ["global"]);
//     }

//     #[test]
//     fn test_instance_get() {
//         let module_name = "extern_module";

//         let result = Store::create();
//         assert!(result.is_ok());
//         let mut store = result.unwrap();
//         assert!(!store.inner.0.is_null());
//         assert!(!store.registered);

//         // check the length of registered module list in store before instatiation
//         assert_eq!(store.func_len(), 0);
//         assert_eq!(store.reg_func_len(module_name), 0);
//         assert_eq!(store.table_len(), 0);
//         assert_eq!(store.reg_table_len(module_name), 0);
//         assert_eq!(store.global_len(), 0);
//         assert_eq!(store.reg_global_len(module_name), 0);
//         assert_eq!(store.mem_len(), 0);
//         assert_eq!(store.reg_mem_len(module_name), 0);
//         assert_eq!(store.reg_module_len(), 0);
//         assert!(store.reg_module_names().is_none());

//         // create ImportObject instance
//         let result = ImportObject::create(module_name);
//         assert!(result.is_ok());
//         let mut import = result.unwrap();

//         // add host function
//         let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
//         assert!(result.is_ok());
//         let func_ty = result.unwrap();
//         let result = Function::create(&func_ty, Box::new(real_add), 0);
//         assert!(result.is_ok());
//         let host_func = result.unwrap();
//         import.add_func("add", host_func);

//         // add table
//         let result = TableType::create(RefType::FuncRef, 0..=u32::MAX);
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         let result = Table::create(&ty);
//         assert!(result.is_ok());
//         let table = result.unwrap();
//         import.add_table("table", table);

//         // add memory
//         let memory = {
//             let result = MemType::create(10..=20);
//             assert!(result.is_ok());
//             let mem_ty = result.unwrap();
//             let result = Memory::create(&mem_ty);
//             assert!(result.is_ok());
//             result.unwrap()
//         };
//         import.add_memory("mem", memory);

//         // add globals
//         let result = GlobalType::create(ValType::F32, Mutability::Const);
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         let result = Global::create(&ty, WasmValue::from_f32(3.5));
//         assert!(result.is_ok());
//         let global = result.unwrap();
//         import.add_global("global", global);

//         let result = Config::create();
//         assert!(result.is_ok());
//         let config = result.unwrap();
//         let result = Executor::create(Some(config), None);
//         assert!(result.is_ok());
//         let mut executor = result.unwrap();
//         let result = executor.register_import_object(&mut store, &import);
//         assert!(result.is_ok());

//         let result = store.named_module(module_name);
//         assert!(result.is_ok());
//         let instance = result.unwrap();

//         // get the exported memory
//         let result = instance.find_memory("mem");
//         assert!(result.is_ok());
//         let memory = result.unwrap();
//         let result = memory.ty();
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         assert_eq!(ty.limit(), 10..=20);
//     }

//     fn create_vm() -> Vm {
//         let module_name = "extern_module";

//         // create ImportObject instance
//         let result = ImportObject::create(module_name);
//         assert!(result.is_ok());
//         let mut import = result.unwrap();

//         // add host function
//         let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
//         assert!(result.is_ok());
//         let func_ty = result.unwrap();
//         let result = Function::create(&func_ty, Box::new(real_add), 0);
//         assert!(result.is_ok());
//         let host_func = result.unwrap();
//         import.add_func("add", host_func);

//         // add table
//         let result = TableType::create(RefType::FuncRef, 0..=u32::MAX);
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         let result = Table::create(&ty);
//         assert!(result.is_ok());
//         let table = result.unwrap();
//         import.add_table("table", table);

//         // add memory
//         let result = MemType::create(0..=u32::MAX);
//         assert!(result.is_ok());
//         let mem_ty = result.unwrap();
//         let result = Memory::create(&mem_ty);
//         assert!(result.is_ok());
//         let memory = result.unwrap();
//         import.add_memory("mem", memory);

//         // add global
//         let result = GlobalType::create(ValType::F32, Mutability::Const);
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         let result = Global::create(&ty, WasmValue::from_f32(3.5));
//         assert!(result.is_ok());
//         let global = result.unwrap();
//         import.add_global("global", global);

//         let result = Vm::create(None, None);
//         assert!(result.is_ok());
//         let mut vm = result.unwrap();

//         let result = vm.register_wasm_from_import(import);
//         assert!(result.is_ok());

//         vm
//     }

//     fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
//         if inputs.len() != 2 {
//             return Err(1);
//         }

//         let a = if inputs[0].ty() == ValType::I32 {
//             inputs[0].to_i32()
//         } else {
//             return Err(2);
//         };

//         let b = if inputs[1].ty() == ValType::I32 {
//             inputs[1].to_i32()
//         } else {
//             return Err(3);
//         };

//         let c = a + b;

//         Ok(vec![WasmValue::from_i32(c)])
//     }
// }
