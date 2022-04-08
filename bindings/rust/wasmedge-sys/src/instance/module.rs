//! Defines WasmEdge Instancestruct.

use crate::{
    error::{InstanceError, WasmEdgeError},
    ffi,
    instance::{function::InnerFunc, global::InnerGlobal, memory::InnerMemory, table::InnerTable},
    types::WasmEdgeString,
    utils::string_to_c_char,
    Function, Global, Memory, Table, WasmEdgeResult,
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
        let name = WasmEdgeString::from(name.as_ref());
        let ctx = unsafe { ffi::WasmEdge_ModuleInstanceCreate(name.as_raw()) };

        match ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::Create)),
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
    pub fn get_func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Function> {
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
    pub fn get_table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
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
    pub fn get_memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
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
    pub fn get_global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
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
pub(crate) struct InnerInstance(pub(crate) *mut ffi::WasmEdge_ModuleInstanceContext);
unsafe impl Send for InnerInstance {}
unsafe impl Sync for InnerInstance {}

#[derive(Debug)]
pub struct ImportModule {
    pub(crate) inner: InnerInstance,
    pub(crate) registered: bool,
    name: String,
}
impl Drop for ImportModule {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe {
                ffi::WasmEdge_ModuleInstanceDelete(self.inner.0);
            }
        }
    }
}
impl ImportModule {
    pub fn create(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let raw_name = WasmEdgeString::from(name.as_ref());
        let ctx = unsafe { ffi::WasmEdge_ModuleInstanceCreate(raw_name.as_raw()) };

        match ctx.is_null() {
            true => Err(WasmEdgeError::Instance(InstanceError::CreateImportModule)),
            false => Ok(Self {
                inner: InnerInstance(ctx),
                registered: false,
                name: name.as_ref().to_string(),
            }),
        }
    }

    /// Returns the name of this exported [module instance](crate::Instance).
    ///
    /// If this module [instance](crate::Instance) is an active [instance](crate::Instance), return None.
    pub fn name(&self) -> String {
        self.name.to_owned()
    }
}
impl AddImportInstance for ImportModule {
    /// Adds a [host function](crate::Function) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the exported host function instance to add.
    fn add_func(&mut self, name: impl AsRef<str>, mut func: Function) {
        let func_name: WasmEdgeString = name.into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddFunction(self.inner.0, func_name.as_raw(), func.inner.0);
        }
        func.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [table](crate::Table) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the exported table instance to add.
    fn add_table(&mut self, name: impl AsRef<str>, mut table: Table) {
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddTable(self.inner.0, table_name.as_raw(), table.inner.0);
        }
        table.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [memory](crate::Memory) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the exported memory instance to add.
    fn add_memory(&mut self, name: impl AsRef<str>, mut memory: Memory) {
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddMemory(self.inner.0, mem_name.as_raw(), memory.inner.0);
        }
        memory.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [global](crate::Global) into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the exported global instance to add.
    fn add_global(&mut self, name: impl AsRef<str>, mut global: Global) {
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddGlobal(
                self.inner.0,
                global_name.as_raw(),
                global.inner.0,
            );
        }
        global.inner.0 = std::ptr::null_mut();
    }
}

#[derive(Debug)]
pub struct WasiModule {
    pub(crate) inner: InnerInstance,
    pub(crate) registered: bool,
}
impl Drop for WasiModule {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe {
                ffi::WasmEdge_ModuleInstanceDelete(self.inner.0);
            }
        }
    }
}
impl WasiModule {
    /// Creates a WASI host module which contains the WASI host functions, and initializes it with the given parameters.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the commandline arguments. The first argument is the program name.
    ///
    /// - `envs` specifies the environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// - `preopens` specifies the directories to pre-open. The required format is `DIR1:DIR2`.
    ///
    /// # Error
    ///
    /// If fail to create a host module, then an error is returned.
    pub fn create(
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) -> WasmEdgeResult<Self> {
        let args = match args {
            Some(args) => args.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let args_len = args.len();

        let envs = match envs {
            Some(envs) => envs.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let envs_len = envs.len();

        let preopens = match preopens {
            Some(preopens) => preopens
                .into_iter()
                .map(string_to_c_char)
                .collect::<Vec<_>>(),
            None => vec![],
        };
        let preopens_len = preopens.len();

        let ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceCreateWASI(
                args.as_ptr(),
                args_len as u32,
                envs.as_ptr(),
                envs_len as u32,
                preopens.as_ptr(),
                preopens_len as u32,
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
            false => Ok(Self {
                inner: InnerInstance(ctx),
                registered: false,
            }),
        }
    }

    pub fn name(&self) -> String {
        String::from("wasi_snapshot_preview1")
    }

    /// Initializes the WASI host module with the given parameters.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the commandline arguments. The first argument is the program name.
    ///
    /// - `envs` specifies the environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// - `preopens` specifies the directories to pre-open. The required format is `DIR1:DIR2`.
    pub fn init_wasi(
        &mut self,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) {
        let args = match args {
            Some(args) => args.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let args_len = args.len();

        let envs = match envs {
            Some(envs) => envs.into_iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let envs_len = envs.len();

        let preopens = match preopens {
            Some(preopens) => preopens
                .into_iter()
                .map(string_to_c_char)
                .collect::<Vec<_>>(),
            None => vec![],
        };
        let preopens_len = preopens.len();

        unsafe {
            ffi::WasmEdge_ModuleInstanceInitWASI(
                self.inner.0,
                args.as_ptr(),
                args_len as u32,
                envs.as_ptr(),
                envs_len as u32,
                preopens.as_ptr(),
                preopens_len as u32,
            )
        };
    }

    /// Returns the WASI exit code.
    ///
    /// The WASI exit code can be accessed after running the "_start" function of a `wasm32-wasi` program.
    pub fn exit_code(&self) -> u32 {
        unsafe { ffi::WasmEdge_ModuleInstanceWASIGetExitCode(self.inner.0 as *const _) }
    }
}
impl AddImportInstance for WasiModule {
    /// Adds a [host function](crate::Function) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the exported host function instance to add.
    fn add_func(&mut self, name: impl AsRef<str>, mut func: Function) {
        let func_name: WasmEdgeString = name.into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddFunction(self.inner.0, func_name.as_raw(), func.inner.0);
        }
        func.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [table](crate::Table) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the exported table instance to add.
    fn add_table(&mut self, name: impl AsRef<str>, mut table: Table) {
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddTable(self.inner.0, table_name.as_raw(), table.inner.0);
        }
        table.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [memory](crate::Memory) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the exported memory instance to add.
    fn add_memory(&mut self, name: impl AsRef<str>, mut memory: Memory) {
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddMemory(self.inner.0, mem_name.as_raw(), memory.inner.0);
        }
        memory.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [global](crate::Global) into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the exported global instance to add.
    fn add_global(&mut self, name: impl AsRef<str>, mut global: Global) {
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddGlobal(
                self.inner.0,
                global_name.as_raw(),
                global.inner.0,
            );
        }
        global.inner.0 = std::ptr::null_mut();
    }
}

#[derive(Debug)]
pub struct WasmEdgeProcessModule {
    pub(crate) inner: InnerInstance,
    pub(crate) registered: bool,
}
impl Drop for WasmEdgeProcessModule {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe {
                ffi::WasmEdge_ModuleInstanceDelete(self.inner.0);
            }
        }
    }
}
impl WasmEdgeProcessModule {
    /// Creates a wasmedge_process host module that contains the wasmedge_process host functions and
    /// initialize it with the parameters.
    ///
    /// # Arguments
    ///
    /// - `allowed_cmds` specifies a white list of commands.
    ///
    /// - `allowed` determines if wasmedge_process is allowed to execute all commands on the white list.
    ///
    /// # Error
    ///
    /// If fail to create a wasmedge_process host module, then an error is returned.
    pub fn create(allowed_cmds: Option<Vec<&str>>, allowed: bool) -> WasmEdgeResult<Self> {
        let cmds = match allowed_cmds {
            Some(cmds) => cmds.iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let cmds_len = cmds.len();

        let ctx = unsafe {
            ffi::WasmEdge_ModuleInstanceCreateWasmEdgeProcess(
                cmds.as_ptr(),
                cmds_len as u32,
                allowed,
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::ImportObjCreate),
            false => Ok(Self {
                inner: InnerInstance(ctx),
                registered: false,
            }),
        }
    }

    pub fn name(&self) -> String {
        String::from("wasmedge_process")
    }

    /// Initializes the wasmedge_process host module with the parameters.
    ///
    /// # Arguments
    ///
    /// - `allowed_cmds` specifies a white list of commands.
    ///
    /// - `allowed` determines if wasmedge_process is allowed to execute all commands on the white list.
    pub fn init_wasmedge_process(&mut self, allowed_cmds: Option<Vec<&str>>, allowed: bool) {
        let cmds = match allowed_cmds {
            Some(cmds) => cmds.iter().map(string_to_c_char).collect::<Vec<_>>(),
            None => vec![],
        };
        let cmds_len = cmds.len();

        unsafe {
            ffi::WasmEdge_ModuleInstanceInitWasmEdgeProcess(
                self.inner.0,
                cmds.as_ptr(),
                cmds_len as u32,
                allowed,
            )
        }
    }
}
impl AddImportInstance for WasmEdgeProcessModule {
    /// Adds a [host function](crate::Function) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the exported host function instance to add.
    fn add_func(&mut self, name: impl AsRef<str>, mut func: Function) {
        let func_name: WasmEdgeString = name.into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddFunction(self.inner.0, func_name.as_raw(), func.inner.0);
        }
        func.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [table](crate::Table) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the exported table instance to add.
    fn add_table(&mut self, name: impl AsRef<str>, mut table: Table) {
        let table_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddTable(self.inner.0, table_name.as_raw(), table.inner.0);
        }
        table.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [memory](crate::Memory) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the exported memory instance to add.
    fn add_memory(&mut self, name: impl AsRef<str>, mut memory: Memory) {
        let mem_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddMemory(self.inner.0, mem_name.as_raw(), memory.inner.0);
        }
        memory.inner.0 = std::ptr::null_mut();
    }

    /// Adds a [global](crate::Global) into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the exported global instance to add.
    fn add_global(&mut self, name: impl AsRef<str>, mut global: Global) {
        let global_name: WasmEdgeString = name.as_ref().into();
        unsafe {
            ffi::WasmEdge_ModuleInstanceAddGlobal(
                self.inner.0,
                global_name.as_raw(),
                global.inner.0,
            );
        }
        global.inner.0 = std::ptr::null_mut();
    }
}

pub trait AddImportInstance {
    /// Adds a [host function](crate::Function) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the host function in the host module.
    ///
    /// - `func` specifies the exported host function instance to add.
    fn add_func(&mut self, name: impl AsRef<str>, func: Function);

    /// Adds a [table](crate::Table) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export table in the host module.
    ///
    /// - `table` specifies the exported table instance to add.
    fn add_table(&mut self, name: impl AsRef<str>, table: Table);

    /// Adds a [memory](crate::Memory) into the host module.
    ///
    /// # Arguments
    ///
    /// - `name` specifies the name of the export memory in the host module.
    ///
    /// - `memory` specifies the exported memory instance to add.
    fn add_memory(&mut self, name: impl AsRef<str>, memory: Memory);

    /// Adds a [global](crate::Global) into the host module.
    ///
    /// # Arguments
    ///
    /// `name` specifies the name of the export global in the host module.
    ///
    /// `global` specifies the exported global instance to add.
    fn add_global(&mut self, name: impl AsRef<str>, global: Global);
}

#[derive(Debug)]
pub enum ImportObject {
    Import(ImportModule),
    Wasi(WasiModule),
    WasmEdgeProcess(WasmEdgeProcessModule),
}
impl ImportObject {
    pub fn name(&self) -> String {
        match self {
            ImportObject::Import(import) => import.name(),
            ImportObject::Wasi(wasi) => wasi.name(),
            ImportObject::WasmEdgeProcess(wasmedge_process) => wasmedge_process.name(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        Config, Executor, FuncType, GlobalType, ImportModule, MemType, Store, TableType, Vm,
        WasmValue,
    };
    use wasmedge_types::{Mutability, RefType, ValType};

    #[test]
    fn test_instance_find_xxx() {
        let vm = create_vm();
        let result = vm.store_mut();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // get the module named "extern"
        let result = store.module("extern_module");
        assert!(result.is_ok());
        let instance = result.unwrap();

        // check the name of the module
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern_module");

        // get the exported function named "fib"
        let result = instance.get_func("add");
        assert!(result.is_ok());
        let func = result.unwrap();

        // check the type of the function
        let result = func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check the parameter types
        let param_types = ty.params_type_iter().collect::<Vec<ValType>>();
        assert_eq!(param_types, [ValType::I32, ValType::I32]);

        // check the return types
        let return_types = ty.returns_type_iter().collect::<Vec<ValType>>();
        assert_eq!(return_types, [ValType::I32]);

        // get the exported table named "table"
        let result = instance.get_table("table");
        assert!(result.is_ok());
        let table = result.unwrap();

        // check the type of the table
        let result = table.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        assert_eq!(ty.limit(), 0..=u32::MAX);

        // get the exported memory named "mem"
        let result = instance.get_memory("mem");
        assert!(result.is_ok());
        let memory = result.unwrap();

        // check the type of the memory
        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.limit(), 0..=u32::MAX);

        // get the exported global named "global"
        let result = instance.get_global("global");
        assert!(result.is_ok());
        let global = result.unwrap();

        // check the type of the global
        let result = global.ty();
        assert!(result.is_ok());
        let global = result.unwrap();
        assert_eq!(global.value_type(), ValType::F32);
        assert_eq!(global.mutability(), Mutability::Const);
    }

    #[test]
    fn test_instance_find_names() {
        let vm = create_vm();
        let result = vm.store_mut();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // get the module named "extern"
        let result = store.module("extern_module");
        assert!(result.is_ok());
        let instance = result.unwrap();

        // check the name of the module
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), "extern_module");

        assert_eq!(instance.func_len(), 1);
        let result = instance.func_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["add"]);

        assert_eq!(instance.table_len(), 1);
        let result = instance.table_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["table"]);

        assert_eq!(instance.mem_len(), 1);
        let result = instance.mem_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["mem"]);

        assert_eq!(instance.global_len(), 1);
        let result = instance.global_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["global"]);
    }

    #[test]
    fn test_instance_get() {
        let module_name = "extern_module";

        let result = Store::create();
        assert!(result.is_ok());
        let mut store = result.unwrap();
        assert!(!store.inner.0.is_null());
        assert!(!store.registered);

        // check the length of registered module list in store before instatiation
        assert_eq!(store.module_len(), 0);
        assert!(store.module_names().is_none());

        // create ImportObject instance
        let result = ImportModule::create(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
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
        let result = GlobalType::create(ValType::F32, Mutability::Const);
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

        let import = ImportObject::Import(import);
        let result = executor.register_import_object(&mut store, &import);
        assert!(result.is_ok());

        let result = store.module(module_name);
        assert!(result.is_ok());
        let instance = result.unwrap();

        // get the exported memory
        let result = instance.get_memory("mem");
        assert!(result.is_ok());
        let memory = result.unwrap();
        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.limit(), 10..=20);
    }

    fn create_vm() -> Vm {
        let module_name = "extern_module";

        // create ImportObject instance
        let result = ImportModule::create(module_name);
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
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
        let result = MemType::create(0..=u32::MAX);
        assert!(result.is_ok());
        let mem_ty = result.unwrap();
        let result = Memory::create(&mem_ty);
        assert!(result.is_ok());
        let memory = result.unwrap();
        import.add_memory("mem", memory);

        // add global
        let result = GlobalType::create(ValType::F32, Mutability::Const);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Global::create(&ty, WasmValue::from_f32(3.5));
        assert!(result.is_ok());
        let global = result.unwrap();
        import.add_global("global", global);

        let result = Vm::create(None, None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        let result = vm.register_wasm_from_import(ImportObject::Import(import));
        assert!(result.is_ok());

        vm
    }

    fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }
}
