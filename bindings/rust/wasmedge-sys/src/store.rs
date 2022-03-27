//! Defines WasmEdge Store struct.

use crate::{
    error::{StoreError, WasmEdgeError},
    ffi,
    instance::{
        function::{Function, InnerFunc},
        global::{Global, InnerGlobal},
        memory::{InnerMemory, Memory},
        module::{InnerInstance, Instance},
        table::{InnerTable, Table},
    },
    types::WasmEdgeString,
    WasmEdgeResult,
};

/// Struct of Wasmedge Store.
///
/// The [Store] represents all global state that can be manipulated by WebAssembly programs. It consists of the runtime representation of all instances of [functions](crate::Function), [tables](crate::Table), [memories](crate::Memory), and [globals](crate::Global) that have been allocated during the life time of the [Vm](crate::Vm).
#[derive(Debug)]
pub struct Store {
    pub(crate) inner: InnerStore,
    pub(crate) registered: bool,
}
impl Store {
    /// Creates a new [Store].
    ///
    /// # Error
    ///
    /// If fail to create, then an error is returned.
    pub fn create() -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_StoreCreate() };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::Create)),
            false => Ok(Store {
                inner: InnerStore(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the exported [function](crate::Function) instance in the anonymous [module](crate::Module)
    /// by the given function name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [Store] as an anonymous module.
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
        let ctx = unsafe { ffi::WasmEdge_StoreFindFunction(self.inner.0, func_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundFunc(
                name.as_ref().to_string(),
            ))),
            false => Ok(Function {
                inner: InnerFunc(ctx),
                registered: true,
                name: Some(name.as_ref().to_string()),
                mod_name: None,
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
            ffi::WasmEdge_StoreFindFunctionRegistered(
                self.inner.0,
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
                inner: InnerFunc(ctx),
                registered: true,
                name: Some(func_name.as_ref().to_string()),
                mod_name: Some(mod_name.as_ref().to_string()),
            }),
        }
    }

    /// Returns the exported [table](crate::Table) instance in the anonymous [module](crate::Module)
    /// by the given table name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [Store] as an anonymous module.
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
        let ctx = unsafe { ffi::WasmEdge_StoreFindTable(self.inner.0, table_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundTable(
                name.as_ref().to_string(),
            ))),
            false => Ok(Table {
                inner: InnerTable(ctx),
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
            ffi::WasmEdge_StoreFindTableRegistered(
                self.inner.0,
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
                inner: InnerTable(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the exported [memory](crate::Memory) instance in the anonymous [module](crate::Module)
    /// by the given memory name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [Store] as an anonymous module.
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
        let ctx = unsafe { ffi::WasmEdge_StoreFindMemory(self.inner.0, mem_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundMem(
                name.as_ref().to_string(),
            ))),
            false => Ok(Memory {
                inner: InnerMemory(ctx),
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
            ffi::WasmEdge_StoreFindMemoryRegistered(
                self.inner.0,
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
                inner: InnerMemory(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the exported [global](crate::Global) instance in the anonymous [module](crate::Module)
    /// by the given global name.
    ///
    /// After instantiating a WASM module, the WASM module is registered into the [Store] as an anonymous module.
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
        let ctx = unsafe { ffi::WasmEdge_StoreFindGlobal(self.inner.0, global_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundGlobal(
                name.as_ref().to_string(),
            ))),
            false => Ok(Global {
                inner: InnerGlobal(ctx),
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
            ffi::WasmEdge_StoreFindGlobalRegistered(
                self.inner.0,
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
                inner: InnerGlobal(ctx),
                registered: true,
            }),
        }
    }

    /// Returns the length of the exported [functions](crate::Function) in the anonymous module.
    pub fn func_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_StoreListFunctionLength(self.inner.0) }
    }

    /// Returns the names of the exported [functions](crate::Function) in the anonymous module.
    pub fn func_names(&self) -> Option<Vec<String>> {
        let len_func_names = self.func_len();
        match len_func_names > 0 {
            true => {
                let mut func_names = Vec::with_capacity(len_func_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListFunction(
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

    /// Returns the length of the exported [functions](crate::Function) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_func_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { ffi::WasmEdge_StoreListFunctionRegisteredLength(self.inner.0, mod_name.as_raw()) }
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
                    ffi::WasmEdge_StoreListFunctionRegistered(
                        self.inner.0,
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
        unsafe { ffi::WasmEdge_StoreListTableLength(self.inner.0) }
    }

    /// Returns the names of the exported [tables](crate::Table) in the anonynous module.
    pub fn table_names(&self) -> Option<Vec<String>> {
        let len_table_names = self.table_len();
        match len_table_names > 0 {
            true => {
                let mut table_names = Vec::with_capacity(len_table_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListTable(
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

    /// Returns the length of the exported [tables](crate::Table) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_table_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { ffi::WasmEdge_StoreListTableRegisteredLength(self.inner.0, mod_name.as_raw()) }
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
                    ffi::WasmEdge_StoreListTableRegistered(
                        self.inner.0,
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
        unsafe { ffi::WasmEdge_StoreListGlobalLength(self.inner.0) }
    }

    /// Returns the names of the exported [globals](crate::Global) in the anonymous module.
    pub fn global_names(&self) -> Option<Vec<String>> {
        let len_global_names = self.global_len();
        match len_global_names > 0 {
            true => {
                let mut global_names = Vec::with_capacity(len_global_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListGlobal(
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

    /// Returns the length of the exported [globals](crate::Global) in the reigstered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_global_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { ffi::WasmEdge_StoreListGlobalRegisteredLength(self.inner.0, mod_name.as_raw()) }
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
                    ffi::WasmEdge_StoreListGlobalRegistered(
                        self.inner.0,
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
        unsafe { ffi::WasmEdge_StoreListMemoryLength(self.inner.0) }
    }

    /// Returns the names of all exported [memories](crate::Memory) in the anonymous module.
    pub fn mem_names(&self) -> Option<Vec<String>> {
        let len_mem_names = self.mem_len();
        match len_mem_names > 0 {
            true => {
                let mut mem_names = Vec::with_capacity(len_mem_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListMemory(
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

    /// Returns the length of the exported [memories](crate::Memory) in the registered module.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module.
    pub fn reg_mem_len(&self, mod_name: impl AsRef<str>) -> u32 {
        let mod_name: WasmEdgeString = mod_name.as_ref().into();
        unsafe { ffi::WasmEdge_StoreListMemoryRegisteredLength(self.inner.0, mod_name.as_raw()) }
    }

    /// Returns the names of all exported [memories](crate::Memory) in the registered module.
    pub fn reg_mem_names(&self, mod_name: impl AsRef<str>) -> Option<Vec<String>> {
        let len_mem_names = self.reg_mem_len(mod_name.as_ref());
        match len_mem_names > 0 {
            true => {
                let mut mem_names = Vec::with_capacity(len_mem_names as usize);
                let mod_name: WasmEdgeString = mod_name.as_ref().into();
                unsafe {
                    ffi::WasmEdge_StoreListMemoryRegistered(
                        self.inner.0,
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
        unsafe { ffi::WasmEdge_StoreListModuleLength(self.inner.0) }
    }

    /// Returns the names of all registered [modules](crate::Module).
    pub fn reg_module_names(&self) -> Option<Vec<String>> {
        let len_mod_names = self.reg_module_len();
        match len_mod_names > 0 {
            true => {
                let mut mod_names = Vec::with_capacity(len_mod_names as usize);
                unsafe {
                    ffi::WasmEdge_StoreListModule(
                        self.inner.0,
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

    /// Returns the active anonymous module instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [module instance](crate::Instance), then an error is returned.
    pub fn active_module(&mut self) -> WasmEdgeResult<Instance<'_>> {
        let ctx = unsafe { ffi::WasmEdge_StoreGetActiveModule(self.inner.0) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundActiveModule)),
            false => Ok(Instance {
                inner: InnerInstance(ctx),
                store: self,
            }),
        }
    }

    /// Returns the registered module instance by the module name.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the name of the registered module instance.
    ///
    /// # Error
    ///
    /// If fail to find the target [module instance](crate::Instance), then an error is returned.
    pub fn named_module(&mut self, name: impl AsRef<str>) -> WasmEdgeResult<Instance<'_>> {
        let mod_name: WasmEdgeString = name.as_ref().into();
        let ctx = unsafe { ffi::WasmEdge_StoreFindModule(self.inner.0, mod_name.as_raw()) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Store(StoreError::NotFoundModule(
                name.as_ref().to_string(),
            ))),
            false => Ok(Instance {
                inner: InnerInstance(ctx),
                store: self,
            }),
        }
    }

    /// Checks if the [Store] contains a function of which the name matches the given
    /// `func_name`.
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the function's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [Store], then an error is returned.
    pub fn contains_func(&self, func_name: impl AsRef<str>) -> WasmEdgeResult<()> {
        // check if the anonymous module contains functions
        if self.func_len() == 0 {
            return Err(WasmEdgeError::Store(StoreError::NotFoundFunc(
                func_name.as_ref().into(),
            )));
        }

        // get the names of all functions in the anonymous module
        let result = self.func_names().ok_or_else(|| {
            WasmEdgeError::Store(StoreError::NotFoundFunc(func_name.as_ref().into()))
        });
        let names = result.unwrap();

        // check if the specified function name is in the names or not
        if names.iter().all(|x| x != func_name.as_ref()) {
            return Err(WasmEdgeError::Store(StoreError::NotFoundFunc(
                func_name.as_ref().into(),
            )));
        }
        Ok(())
    }

    /// Checks if the [Store] contains a registered function of which the name matches the
    /// given `func_name`.
    ///
    /// # Argument
    ///
    /// - `func_name` specifies the registered function's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [Store], then an error is returned.
    pub fn contains_reg_func(
        &self,
        mod_name: impl AsRef<str>,
        func_name: impl AsRef<str>,
    ) -> WasmEdgeResult<()> {
        // check if the module exists or not in the store
        self.contains_mod_name(mod_name.as_ref())?;

        // check if the specified module contains registered functions
        if self.reg_func_len(mod_name.as_ref()) == 0 {
            return Err(WasmEdgeError::Store(StoreError::NotFoundModule(
                mod_name.as_ref().into(),
            )));
        }

        // get the names of all registered functions in the specified module
        let result = self.reg_func_names(mod_name.as_ref()).ok_or_else(|| {
            WasmEdgeError::Store(StoreError::NotFoundFuncRegistered {
                func_name: func_name.as_ref().into(),
                mod_name: mod_name.as_ref().into(),
            })
        });
        let names = result.unwrap();

        // check if the specified function name is in the names or not
        if names.iter().all(|x| x != func_name.as_ref()) {
            return Err(WasmEdgeError::Store(StoreError::NotFoundFuncRegistered {
                func_name: func_name.as_ref().into(),
                mod_name: mod_name.as_ref().into(),
            }));
        }
        Ok(())
    }

    /// Checks if the [Store] contains a registered module of which the name matches the given
    /// `mod_name`.
    ///
    /// # Argument
    ///
    /// - `mod_name` specifies the registered module's name to check.
    ///
    /// # Error
    ///
    /// If fail to find the name in the [Store], then an error is returned.
    pub fn contains_mod_name(&self, mod_name: impl AsRef<str>) -> WasmEdgeResult<()> {
        if self.reg_module_len() == 0 {
            return Err(WasmEdgeError::Store(StoreError::NotFoundModule(
                mod_name.as_ref().into(),
            )));
        }

        let result = self.reg_module_names().ok_or_else(|| {
            WasmEdgeError::Store(StoreError::NotFoundModule(mod_name.as_ref().into()))
        });

        let names = result.unwrap();
        if names.iter().all(|x| x != mod_name.as_ref()) {
            return Err(WasmEdgeError::Store(StoreError::NotFoundModule(
                mod_name.as_ref().into(),
            )));
        }
        Ok(())
    }
}
impl Drop for Store {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_StoreDelete(self.inner.0) }
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerStore(pub(crate) *mut ffi::WasmEdge_StoreContext);
unsafe impl Send for InnerStore {}
unsafe impl Sync for InnerStore {}

#[cfg(test)]
mod tests {
    use super::Store;
    use crate::{
        instance::{Function, Global, GlobalType, MemType, Memory, Table, TableType},
        types::WasmValue,
        Config, Executor, FuncType, ImportObject, Vm, WasmValueType,
    };
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{Mutability, RefType};

    #[test]
    fn test_store_basic() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();
        assert!(!store.inner.0.is_null());
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
        let result = ImportObject::create(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![WasmValueType::I32; 2], vec![WasmValueType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // add table
        let result = TableType::create(RefType::FuncRef, 0..=u32::MAX);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Table::create(&ty);
        assert!(result.is_ok());
        let table = result.unwrap();
        import.add_table("table", table);

        // add memory
        let memory = {
            let result = MemType::create(10..=20);
            assert!(result.is_ok());
            let mem_ty = result.unwrap();
            let result = Memory::create(&mem_ty);
            assert!(result.is_ok());
            result.unwrap()
        };
        import.add_memory("mem", memory);

        // add globals
        let result = GlobalType::create(WasmValueType::F32, Mutability::Const);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Global::create(&ty, WasmValue::from_f32(3.5));
        assert!(result.is_ok());
        let global = result.unwrap();
        import.add_global("global", global);

        let result = Config::create();
        assert!(result.is_ok());
        let config = result.unwrap();
        let result = Executor::create(Some(config), None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        let result = executor.register_import_object(&mut store, &import);
        assert!(result.is_ok());

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
        let memory = result.unwrap();
        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.limit(), 10..=20);

        // check the global list after instantiation
        let result = store.find_global("global");
        assert!(result.is_err());
        let result = store.find_global_registered("extern_module", "global");
        assert!(result.is_ok());
        let global = result.unwrap();
        assert!(!global.inner.0.is_null() && global.registered);
        let val = global.get_value();
        assert_eq!(val.to_f32(), 3.5);

        // run the registered function
        let result = executor.run_func_registered(
            &mut store,
            "extern_module",
            "add",
            vec![WasmValue::from_i32(12), WasmValue::from_i32(21)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 33);

        let second_run = executor.run_func_registered(
            &mut store,
            "extern_module",
            "add",
            vec![WasmValue::from_i32(12), WasmValue::from_i32(21)],
        );
        assert!(second_run.is_ok());
    }

    #[test]
    fn test_store_send() {
        let result = Store::create();
        assert!(result.is_ok());
        let store = result.unwrap();
        assert!(!store.inner.0.is_null());
        assert!(!store.registered);

        let handle = thread::spawn(move || {
            let s = store;
            assert!(!s.inner.0.is_null());
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_store_sync() {
        let result = Store::create();
        assert!(result.is_ok());
        let store = Arc::new(Mutex::new(result.unwrap()));

        let store_cloned = Arc::clone(&store);
        let handle = thread::spawn(move || {
            // create ImportObject instance
            let result = ImportObject::create("extern_module");
            assert!(result.is_ok());
            let mut import = result.unwrap();

            // add host function
            let result = FuncType::create(vec![WasmValueType::I32; 2], vec![WasmValueType::I32]);
            assert!(result.is_ok());
            let func_ty = result.unwrap();
            let result = Function::create(&func_ty, Box::new(real_add), 0);
            assert!(result.is_ok());
            let host_func = result.unwrap();
            import.add_func("add", host_func);

            // create an Executor
            let result = Config::create();
            assert!(result.is_ok());
            let config = result.unwrap();
            let result = Executor::create(Some(config), None);
            assert!(result.is_ok());
            let mut executor = result.unwrap();

            let result = store_cloned.lock();
            assert!(result.is_ok());
            let mut store = result.unwrap();

            let result = executor.register_import_object(&mut store, &import);
            assert!(result.is_ok());

            // run the registered function
            let result = executor.run_func_registered(
                &mut store,
                "extern_module",
                "add",
                vec![WasmValue::from_i32(12), WasmValue::from_i32(21)],
            );
            assert!(result.is_ok());
            let returns = result.unwrap();
            assert_eq!(returns[0].to_i32(), 33);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_store_active_module() {
        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // load wasm module from a specified file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.load_wasm_from_file(path);
        assert!(result.is_ok());

        // validate
        let result = vm.validate();
        assert!(result.is_ok());

        // instantiate
        let result = vm.instantiate();
        assert!(result.is_ok());

        // get the store in vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // get the active module
        let result = store.active_module();
        assert!(result.is_ok());
        let instance = result.unwrap();

        // check the name of the module instance
        assert!(instance.name().is_none());

        // get the exported function named "fib"
        let result = instance.find_func("fib");
        assert!(result.is_ok());
        let func = result.unwrap();

        // check the type of the function
        let result = func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check the parameter types
        let param_types = ty.params_type_iter().collect::<Vec<WasmValueType>>();
        assert_eq!(param_types, [WasmValueType::I32]);

        // check the return types
        let return_types = ty.returns_type_iter().collect::<Vec<WasmValueType>>();
        assert_eq!(return_types, [WasmValueType::I32]);
    }

    #[test]
    fn test_store_named_module() {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Store context
        let result = Store::create();
        assert!(result.is_ok(), "Failed to create Store instance");
        let mut store = result.unwrap();

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), Some(&mut store));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // register a wasm module from a wasm file
        let path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
        let result = vm.register_wasm_from_file("extern", path);
        assert!(result.is_ok());

        // get the store in vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // get the module named "extern"
        let result = store.named_module("extern");
        assert!(result.is_ok());
        let instance = result.unwrap();

        // check the name of the module
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern");

        // get the exported function named "fib"
        let result = instance.find_func("fib");
        assert!(result.is_ok());
        let func = result.unwrap();

        // check the type of the function
        let result = func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check the parameter types
        let param_types = ty.params_type_iter().collect::<Vec<WasmValueType>>();
        assert_eq!(param_types, [WasmValueType::I32]);

        // check the return types
        let return_types = ty.returns_type_iter().collect::<Vec<WasmValueType>>();
        assert_eq!(return_types, [WasmValueType::I32]);
    }

    fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == WasmValueType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == WasmValueType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
