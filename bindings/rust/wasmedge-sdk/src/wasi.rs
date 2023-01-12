//! Defines wasi module instance types, including WasiInstance, WasiNnInstance, wasi-crypto instances.

use crate::{AsInstance, Func, Global, Memory, Table, WasmEdgeResult};
use wasmedge_sys::{self as sys, AsImport, AsInstance as sys_as_instance_trait};

/// Represents a wasi module instance.
#[derive(Debug)]
pub struct WasiInstance {
    pub(crate) inner: sys::WasiModule,
}
impl WasiInstance {
    /// Initializes the WASI host module with the given parameters.
    ///
    /// # Arguments
    ///
    /// * `args` - The commandline arguments. The first argument is the program name.
    ///
    /// * `envs` - The environment variables in the format `ENV_VAR_NAME=VALUE`.
    ///
    /// * `preopens` - The directories to pre-open. The required format is `DIR1:DIR2`.
    pub fn initialize(
        &mut self,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) {
        self.inner.init_wasi(args, envs, preopens);
    }

    /// Returns the WASI exit code.
    ///
    /// The WASI exit code can be accessed after running the "_start" function of a `wasm32-wasi` program.
    pub fn exit_code(&self) -> u32 {
        self.inner.exit_code()
    }

    /// Returns the native handler from the mapped FD/Handler.
    ///
    /// # Argument
    ///
    /// * `fd` - The WASI mapped Fd.
    ///
    /// # Error
    ///
    /// If fail to get the native handler, then an error is returned.
    pub fn native_handler(&self, fd: i32) -> WasmEdgeResult<u64> {
        self.inner.get_native_handler(fd)
    }
}
impl AsInstance for WasiInstance {
    /// Returns the name of this exported [module instance](crate::Instance).
    ///
    /// If this [module instance](crate::Instance) is an active [instance](crate::Instance), return None.
    fn name(&self) -> &str {
        self.inner.name()
    }

    /// Returns the count of the exported [function instances](crate::Func) in this [module instance](crate::Instance).
    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    /// Returns the names of the exported [function instances](crate::Func) in this [module instance](crate::Instance).
    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    /// Returns the exported [function instance](crate::Func) in this [module instance](crate::Instance) by the given function name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [function instance](crate::Func).
    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;

        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: None,
        })
    }

    /// Returns the count of the exported [global instances](crate::Global) in this [module instance](crate::Instance).
    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    /// Returns the names of the exported [global instances](crate::Global) in this [module instance](crate::Instance).
    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    /// Returns the exported [global instance](crate::Global) in this [module instance](crate::Instance) by the given global name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [global instance](crate::Global).
    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;

        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: None,
        })
    }

    /// Returns the count of the exported [memory instances](crate::Memory) in this [module instance](crate::Instance).
    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    /// Returns the names of the exported [memory instances](crate::Memory) in this [module instance](crate::Instance).
    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    /// Returns the exported [memory instance](crate::Memory) in this [module instance](crate::Instance) by the given memory name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [memory instance](crate::Memory).
    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;

        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: None,
        })
    }

    /// Returns the count of the exported [table instances](crate::Table) in this [module instance](crate::Instance).
    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    /// Returns the names of the exported [table instances](crate::Table) in this [module instance](crate::Instance).
    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    /// Returns the exported [table instance](crate::Table) in this [module instance](crate::Instance) by the given table name.
    ///
    /// # Argument
    ///
    /// * `name` - the name of the target exported [table instance](crate::Table).
    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;

        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: None,
        })
    }
}

/// A [WasiNnInstance] is a module instance for the WASI-NN specification.
#[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
#[derive(Debug)]
pub struct WasiNnInstance {
    pub(crate) inner: sys::WasiNnModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_nn", target_arch = "x86_64"))]
impl AsInstance for WasiNnInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}

/// A [WasiCryptoCommonInstance] is a module instance for the WASI-Crypto specification, covering common types and functions for symmetric operations.
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
#[derive(Debug)]
pub struct WasiCryptoCommonInstance {
    pub(crate) inner: sys::WasiCryptoCommonModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
impl AsInstance for WasiCryptoCommonInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}

/// A [WasiCryptoAsymmetricCommonInstance] is a module instance for the WASI-Crypto specification, covering common types and functions for asymmetric operations.
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
#[derive(Debug)]
pub struct WasiCryptoAsymmetricCommonInstance {
    pub(crate) inner: sys::WasiCryptoAsymmetricCommonModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
impl AsInstance for WasiCryptoAsymmetricCommonInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}

/// A [WasiCryptoSymmetricInstance] is a module instance for the WASI-Crypto specification, covering symmetric operations.
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
#[derive(Debug)]
pub struct WasiCryptoSymmetricInstance {
    pub(crate) inner: sys::WasiCryptoSymmetricModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
impl AsInstance for WasiCryptoSymmetricInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}

/// A [WasiCryptoKxInstance] is a module instance for the WASI-Crypto specification, covering key exchange interfaces.
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
#[derive(Debug)]
pub struct WasiCryptoKxInstance {
    pub(crate) inner: sys::WasiCryptoKxModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
impl AsInstance for WasiCryptoKxInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}

/// A [WasiCryptoSignaturesInstance] is a module instance for the WASI-Crypto specification, covering signatures interfaces.
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
#[derive(Debug)]
pub struct WasiCryptoSignaturesInstance {
    pub(crate) inner: sys::WasiCryptoSignaturesModule,
}
#[cfg(all(target_os = "linux", feature = "wasi_crypto"))]
impl AsInstance for WasiCryptoSignaturesInstance {
    fn name(&self) -> &str {
        self.inner.name()
    }

    fn func_count(&self) -> usize {
        self.inner.func_len() as usize
    }

    fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    fn func(&self, name: impl AsRef<str>) -> WasmEdgeResult<Func> {
        let inner_func = self.inner.get_func(name.as_ref())?;
        Ok(Func {
            inner: inner_func,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn global_count(&self) -> usize {
        self.inner.global_len() as usize
    }

    fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    fn global(&self, name: impl AsRef<str>) -> WasmEdgeResult<Global> {
        let inner_global = self.inner.get_global(name.as_ref())?;
        Ok(Global {
            inner: inner_global,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn memory_count(&self) -> usize {
        self.inner.mem_len() as usize
    }

    fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    fn memory(&self, name: impl AsRef<str>) -> WasmEdgeResult<Memory> {
        let inner_memory = self.inner.get_memory(name.as_ref())?;
        Ok(Memory {
            inner: inner_memory,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }

    fn table_count(&self) -> usize {
        self.inner.table_len() as usize
    }

    fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }

    fn table(&self, name: impl AsRef<str>) -> WasmEdgeResult<Table> {
        let inner_table = self.inner.get_table(name.as_ref())?;
        Ok(Table {
            inner: inner_table,
            name: Some(name.as_ref().into()),
            mod_name: Some(self.inner.name().into()),
        })
    }
}
