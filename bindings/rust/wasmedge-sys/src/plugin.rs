//! Defines PluginManager and Plugin structs.

use super::ffi;
use crate::{
    error::WasmEdgeError, instance::module::InnerInstance, types::WasmEdgeString, utils, Instance,
    WasmEdgeResult,
};
use std::ffi::CString;

/// Defines the APIs for loading plugins and check the basic information of the loaded plugins.
#[derive(Debug)]
pub struct PluginManager {}
impl PluginManager {
    /// Load plugins from the default path. The default plugin path could be one of the following:
    ///
    /// * The environment variable "WASMEDGE_PLUGIN_PATH".
    ///   
    /// * The `../plugin/` directory related to the WasmEdge installation path.
    ///
    /// * The `wasmedge/` directory under the library path if the WasmEdge is installed under the "/usr".
    pub fn load_plugins_from_default_paths() {
        unsafe { ffi::WasmEdge_PluginLoadWithDefaultPaths() }
    }

    /// Load a single or multiple plugins from a given path.
    ///
    /// * If the path is pointing at a file , then it indicates that a single plugin will be loaded from the file.
    ///
    /// * If the path is pointing at a directory, then the method will load plugins from the files in the directory.
    ///
    /// # Argument
    ///
    /// * `param` - A path to a plugin file or a directory holding plugin files.
    ///
    /// # Error
    ///
    /// * If the path contains invalid characters, then an [WasmEdgeError::FoundNulByte](crate::error::WasmEdgeError::FoundNulByte) error is returned.
    pub fn load_plugins(path: impl AsRef<std::path::Path>) -> WasmEdgeResult<()> {
        let c_path = utils::path_to_cstring(path.as_ref())?;
        unsafe { ffi::WasmEdge_PluginLoadFromPath(c_path.as_ptr()) }

        Ok(())
    }

    /// Returns the count of loaded plugins.
    pub fn count() -> u32 {
        unsafe { ffi::WasmEdge_PluginListPluginsLength() }
    }

    /// Returns the names of all loaded plugins.
    pub fn names() -> Vec<String> {
        let count = Self::count();
        let mut names = Vec::with_capacity(count as usize);

        unsafe {
            ffi::WasmEdge_PluginListPlugins(names.as_mut_ptr(), count);
            names.set_len(count as usize);
        };

        names.into_iter().map(|x| x.into()).collect::<Vec<String>>()
    }

    /// Returns the target plugin by its name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target plugin.
    pub fn find(name: impl AsRef<str>) -> Option<Plugin> {
        let plugin_name: WasmEdgeString = name.as_ref().into();

        let ctx = unsafe { ffi::WasmEdge_PluginFind(plugin_name.as_raw()) };

        match ctx.is_null() {
            true => None,
            false => Some(Plugin {
                inner: InnerPlugin(ctx as *mut _),
            }),
        }
    }

    /// Initializes the `wasmedge_process` plugin module instance with the parameters.
    ///
    /// # Arguments
    ///
    /// * `allowed_cmds` - A white list of commands.
    ///
    /// * `allowed` - Determines if wasmedge_process is allowed to execute all commands on the white list.
    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    pub fn init_wasmedge_process(allowed_cmds: Option<Vec<&str>>, allowed: bool) {
        // parse cmds
        let cstr_cmds: Vec<_> = match allowed_cmds {
            Some(cmds) => cmds
                .iter()
                .map(|&x| std::ffi::CString::new(x).unwrap())
                .collect(),
            None => vec![],
        };
        let mut p_cmds: Vec<_> = cstr_cmds.iter().map(|x| x.as_ptr()).collect();
        let p_cmds_len = p_cmds.len();
        p_cmds.push(std::ptr::null());

        unsafe {
            ffi::WasmEdge_ModuleInstanceInitWasmEdgeProcess(
                p_cmds.as_ptr(),
                p_cmds_len as u32,
                allowed,
            )
        }
    }
}

/// Represents a loaded plugin. It provides the APIs for accessing the plugin.
#[derive(Debug)]
pub struct Plugin {
    pub(crate) inner: InnerPlugin,
}
impl Plugin {
    /// Returns the name of this plugin.
    pub fn name(&self) -> String {
        let name = unsafe { ffi::WasmEdge_PluginGetPluginName(self.inner.0) };
        name.into()
    }

    /// Returns the count of the module instances in this plugin.
    pub fn mod_count(&self) -> u32 {
        unsafe { ffi::WasmEdge_PluginListModuleLength(self.inner.0) }
    }

    /// Returns the names of all module instances in this plugin.
    pub fn mod_names(&self) -> Vec<String> {
        let count = self.mod_count();
        let mut names = Vec::with_capacity(count as usize);

        unsafe {
            ffi::WasmEdge_PluginListModule(self.inner.0, names.as_mut_ptr(), count);
            names.set_len(count as usize);
        }

        names.into_iter().map(|x| x.into()).collect::<Vec<String>>()
    }

    /// Returns a module instance that is generated from the module with the given name in this plugin.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target module.
    pub fn mod_instance(&self, name: impl AsRef<str>) -> Option<Instance> {
        let mod_name: WasmEdgeString = name.as_ref().into();

        let ctx = unsafe { ffi::WasmEdge_PluginCreateModule(self.inner.0, mod_name.as_raw()) };

        match ctx.is_null() {
            true => None,
            false => Some(Instance {
                inner: std::sync::Arc::new(InnerInstance(ctx)),
                registered: false,
            }),
        }
    }

    /// Provides a raw pointer to the inner Plugin context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_PluginContext {
        self.inner.0 as *const _
    }
}

#[derive(Debug)]
pub(crate) struct InnerPlugin(pub(crate) *mut ffi::WasmEdge_PluginContext);
unsafe impl Send for InnerPlugin {}
unsafe impl Sync for InnerPlugin {}

/// Defines the type of the program options.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProgramOptionType {
    None = 0,
    Toggle = 1,
    I8 = 2,
    I16 = 3,
    I32 = 4,
    I64 = 5,
    U8 = 6,
    U16 = 7,
    U32 = 8,
    U64 = 9,
    F32 = 10,
    F64 = 11,
    String = 12,
}
impl From<ffi::WasmEdge_ProgramOptionType> for ProgramOptionType {
    fn from(ty: ffi::WasmEdge_ProgramOptionType) -> Self {
        match ty {
            ffi::WasmEdge_ProgramOptionType_None => ProgramOptionType::None,
            ffi::WasmEdge_ProgramOptionType_Toggle => ProgramOptionType::Toggle,
            ffi::WasmEdge_ProgramOptionType_Int8 => ProgramOptionType::I8,
            ffi::WasmEdge_ProgramOptionType_Int16 => ProgramOptionType::I16,
            ffi::WasmEdge_ProgramOptionType_Int32 => ProgramOptionType::I32,
            ffi::WasmEdge_ProgramOptionType_Int64 => ProgramOptionType::I64,
            ffi::WasmEdge_ProgramOptionType_UInt8 => ProgramOptionType::U8,
            ffi::WasmEdge_ProgramOptionType_UInt16 => ProgramOptionType::U16,
            ffi::WasmEdge_ProgramOptionType_UInt32 => ProgramOptionType::U32,
            ffi::WasmEdge_ProgramOptionType_UInt64 => ProgramOptionType::U64,
            ffi::WasmEdge_ProgramOptionType_Float => ProgramOptionType::F32,
            ffi::WasmEdge_ProgramOptionType_Double => ProgramOptionType::F64,
            ffi::WasmEdge_ProgramOptionType_String => ProgramOptionType::String,
            _ => {
                panic!("[wasmedge-sys] Unsupported ffi::WasmEdge_ProgramOptionType value: {ty}");
            }
        }
    }
}
impl From<ProgramOptionType> for ffi::WasmEdge_ProgramOptionType {
    fn from(value: ProgramOptionType) -> Self {
        match value {
            ProgramOptionType::None => ffi::WasmEdge_ProgramOptionType_None,
            ProgramOptionType::Toggle => ffi::WasmEdge_ProgramOptionType_Toggle,
            ProgramOptionType::I8 => ffi::WasmEdge_ProgramOptionType_Int8,
            ProgramOptionType::I16 => ffi::WasmEdge_ProgramOptionType_Int16,
            ProgramOptionType::I32 => ffi::WasmEdge_ProgramOptionType_Int32,
            ProgramOptionType::I64 => ffi::WasmEdge_ProgramOptionType_Int64,
            ProgramOptionType::U8 => ffi::WasmEdge_ProgramOptionType_UInt8,
            ProgramOptionType::U16 => ffi::WasmEdge_ProgramOptionType_UInt16,
            ProgramOptionType::U32 => ffi::WasmEdge_ProgramOptionType_UInt32,
            ProgramOptionType::U64 => ffi::WasmEdge_ProgramOptionType_UInt64,
            ProgramOptionType::F32 => ffi::WasmEdge_ProgramOptionType_Float,
            ProgramOptionType::F64 => ffi::WasmEdge_ProgramOptionType_Double,
            ProgramOptionType::String => ffi::WasmEdge_ProgramOptionType_String,
        }
    }
}

/// Defines the program option for plugins.
#[derive(Debug)]
pub struct ProgramOption {
    name: CString,
    desc: CString,
    pub inner: ffi::WasmEdge_ProgramOption,
}
impl ProgramOption {
    /// Creates a new program option.
    pub fn create(
        name: impl AsRef<str>,
        desc: impl AsRef<str>,
        ty: ProgramOptionType,
    ) -> WasmEdgeResult<Self> {
        let name = std::ffi::CString::new(name.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        let desc = std::ffi::CString::new(desc.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        let mut po = Self {
            name,
            desc,
            inner: ffi::WasmEdge_ProgramOption {
                Name: std::ptr::null(),
                Description: std::ptr::null(),
                Type: ty.into(),
                Storage: std::ptr::null_mut(),
                DefaultValue: std::ptr::null(),
            },
        };
        po.inner.Name = po.name.as_ptr();
        po.inner.Description = po.desc.as_ptr();

        Ok(po)
    }
}
unsafe impl Send for ProgramOption {}
unsafe impl Sync for ProgramOption {}

/// Defines the module descriptor for plugins.
#[derive(Debug)]
pub struct ModuleDescriptor {
    name: CString,
    desc: CString,
    create: Option<ModuleInstanceCreateFn>,
    pub inner: ffi::WasmEdge_ModuleDescriptor,
}
impl ModuleDescriptor {
    /// Creates a new module descriptor.
    pub fn create(
        name: impl AsRef<str>,
        desc: impl AsRef<str>,
        f: Option<ModuleInstanceCreateFn>,
    ) -> WasmEdgeResult<Self> {
        // module name
        let name = std::ffi::CString::new(name.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        // module description
        let desc = std::ffi::CString::new(desc.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        let mut md = Self {
            name,
            desc,
            create: f,
            inner: ffi::WasmEdge_ModuleDescriptor {
                Name: std::ptr::null(),
                Description: std::ptr::null(),
                Create: None,
            },
        };
        md.inner.Name = md.name.as_ptr();
        md.inner.Description = md.desc.as_ptr();
        md.inner.Create = md.create;

        Ok(md)
    }
}

/// Defines the type of the function that creates a module instance for a plugin.
pub type ModuleInstanceCreateFn = unsafe extern "C" fn(
    arg1: *const ffi::WasmEdge_ModuleDescriptor,
) -> *mut ffi::WasmEdge_ModuleInstanceContext;

/// Defines the version of a plugin.
#[derive(Debug)]
pub struct PluginVersion {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub build: u32,
}
impl PluginVersion {
    /// Creates a new plugin version.
    pub fn create(major: u32, minor: u32, patch: u32, build: u32) -> Self {
        Self {
            major,
            minor,
            patch,
            build,
        }
    }
}
impl From<PluginVersion> for ffi::WasmEdge_PluginVersionData {
    fn from(value: PluginVersion) -> Self {
        Self {
            Major: value.major,
            Minor: value.minor,
            Patch: value.patch,
            Build: value.build,
        }
    }
}

/// Represents Plugin descriptor for plugins.
#[derive(Debug)]
pub struct PluginDescriptor {
    name: CString,
    desc: CString,
    module_descriptors_name_desc: Vec<(CString, CString)>,
    module_descriptors: Vec<ffi::WasmEdge_ModuleDescriptor>,
    program_options_name_desc: Vec<(CString, CString)>,
    program_options: Vec<ffi::WasmEdge_ProgramOption>,
    pub inner: ffi::WasmEdge_PluginDescriptor,
}
impl PluginDescriptor {
    pub fn create(
        name: impl AsRef<str>,
        desc: impl AsRef<str>,
        version: PluginVersion,
    ) -> WasmEdgeResult<Self> {
        // plugin name
        let name = std::ffi::CString::new(name.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        // plugin description
        let desc = std::ffi::CString::new(desc.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        let mut pd = Self {
            name,
            desc,
            module_descriptors_name_desc: Vec::new(),
            module_descriptors: Vec::new(),
            program_options_name_desc: Vec::new(),
            program_options: Vec::new(),
            inner: ffi::WasmEdge_PluginDescriptor {
                Name: std::ptr::null(),
                Description: std::ptr::null(),
                APIVersion: ffi::WasmEdge_Plugin_CurrentAPIVersion,
                Version: version.into(),
                ModuleCount: 0,
                ModuleDescriptions: std::ptr::null_mut(),
                ProgramOptionCount: 0,
                ProgramOptions: std::ptr::null_mut(),
            },
        };
        pd.inner.Name = pd.name.as_ptr();
        pd.inner.Description = pd.desc.as_ptr();

        Ok(pd)
    }

    pub fn add_module_descriptor(
        mut self,
        name: impl AsRef<str>,
        desc: impl AsRef<str>,
        f: Option<ModuleInstanceCreateFn>,
    ) -> WasmEdgeResult<Self> {
        // module name
        let name = std::ffi::CString::new(name.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        // module description
        let desc = std::ffi::CString::new(desc.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        self.module_descriptors
            .push(ffi::WasmEdge_ModuleDescriptor {
                Name: name.as_ptr(),
                Description: desc.as_ptr(),
                Create: f,
            });
        self.module_descriptors_name_desc.push((name, desc));

        self.inner.ModuleCount = self.module_descriptors.len() as u32;
        self.inner.ModuleDescriptions = self.module_descriptors.as_mut_ptr();

        Ok(self)
    }

    pub fn add_program_option(
        mut self,
        name: impl AsRef<str>,
        desc: impl AsRef<str>,
        ty: ProgramOptionType,
    ) -> WasmEdgeResult<Self> {
        let name = std::ffi::CString::new(name.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        let desc = std::ffi::CString::new(desc.as_ref()).map_err(WasmEdgeError::FoundNulByte)?;

        self.program_options.push(ffi::WasmEdge_ProgramOption {
            Name: name.as_ptr(),
            Description: desc.as_ptr(),
            Type: ty.into(),
            Storage: std::ptr::null_mut(),
            DefaultValue: std::ptr::null(),
        });
        self.program_options_name_desc.push((name, desc));

        self.inner.ProgramOptionCount = self.program_options.len() as u32;
        self.inner.ProgramOptions = self.program_options.as_mut_ptr();

        Ok(self)
    }

    /// Returns the raw pointer to the inner `WasmEdge_PluginDescriptor`.
    #[cfg(feature = "ffi")]
    pub fn as_raw_ptr(&self) -> *const ffi::WasmEdge_PluginDescriptor {
        &self.inner
    }
}

#[cfg(test)]
mod tests {

    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    #[test]
    fn test_plugin_wasmedge_process() {
        use super::*;

        PluginManager::load_plugins_from_default_paths();
        assert!(PluginManager::count() >= 1);
        assert!(PluginManager::names()
            .iter()
            .any(|x| x == "wasmedge_process"));

        // get `wasmedge_process` plugin
        let result = PluginManager::find("wasmedge_process");
        assert!(result.is_some());
        let plugin = result.unwrap();
        assert_eq!(plugin.name(), "wasmedge_process");
        assert_eq!(plugin.mod_count(), 1);
        assert!(plugin.mod_names().iter().any(|x| x == "wasmedge_process"));

        // get module instance from plugin
        let result = plugin.mod_instance("wasmedge_process");
        assert!(result.is_some());
        let instance = result.unwrap();

        assert_eq!(instance.name().unwrap(), "wasmedge_process");
        assert_eq!(instance.func_len(), 11);
        assert_eq!(
            instance.func_names().unwrap(),
            [
                "wasmedge_process_add_arg",
                "wasmedge_process_add_env",
                "wasmedge_process_add_stdin",
                "wasmedge_process_get_exit_code",
                "wasmedge_process_get_stderr",
                "wasmedge_process_get_stderr_len",
                "wasmedge_process_get_stdout",
                "wasmedge_process_get_stdout_len",
                "wasmedge_process_run",
                "wasmedge_process_set_prog_name",
                "wasmedge_process_set_timeout",
            ]
        );
        assert_eq!(instance.mem_len(), 0);
        assert_eq!(instance.table_len(), 0);
        assert_eq!(instance.global_len(), 0);
    }

    #[cfg(all(target_os = "linux", feature = "wasi_crypto", not(feature = "static")))]
    #[test]
    fn test_plugin_wasi_crypto() {
        use super::*;

        PluginManager::load_plugins_from_default_paths();
        assert!(PluginManager::count() >= 1);
        assert!(
            PluginManager::names().iter().any(|x| x == "wasi_crypto"),
            "Not found the `wasi_crypto` plugin"
        );

        // get `wasmedge_process` plugin
        let result = PluginManager::find("wasi_crypto");
        assert!(result.is_some());
        let plugin = result.unwrap();
        assert_eq!(plugin.name(), "wasi_crypto");
        assert_eq!(plugin.mod_count(), 5);
        assert_eq!(
            plugin.mod_names(),
            [
                "wasi_crypto_asymmetric_common",
                "wasi_crypto_common",
                "wasi_crypto_kx",
                "wasi_crypto_signatures",
                "wasi_crypto_symmetric",
            ]
        );

        // get `wasi_crypto_asymmetric_common` module instance from plugin
        {
            let result = plugin.mod_instance("wasi_crypto_asymmetric_common");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert_eq!(
                instance.name().unwrap(),
                "wasi_ephemeral_crypto_asymmetric_common"
            );
            assert_eq!(instance.func_len(), 20);
            assert_eq!(
                instance.func_names().unwrap(),
                [
                    "keypair_close",
                    "keypair_export",
                    "keypair_from_id",
                    "keypair_from_pk_and_sk",
                    "keypair_generate",
                    "keypair_generate_managed",
                    "keypair_id",
                    "keypair_import",
                    "keypair_publickey",
                    "keypair_replace_managed",
                    "keypair_secretkey",
                    "keypair_store_managed",
                    "publickey_close",
                    "publickey_export",
                    "publickey_from_secretkey",
                    "publickey_import",
                    "publickey_verify",
                    "secretkey_close",
                    "secretkey_export",
                    "secretkey_import",
                ],
            );
        }

        // get `wasi_crypto_common` module instance from plugin
        {
            let result = plugin.mod_instance("wasi_crypto_common");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert_eq!(instance.name().unwrap(), "wasi_ephemeral_crypto_common");
            assert_eq!(instance.func_len(), 10);
            assert_eq!(
                instance.func_names().unwrap(),
                [
                    "array_output_len",
                    "array_output_pull",
                    "options_close",
                    "options_open",
                    "options_set",
                    "options_set_guest_buffer",
                    "options_set_u64",
                    "secrets_manager_close",
                    "secrets_manager_invalidate",
                    "secrets_manager_open",
                ],
            );
        }

        // get `wasi_crypto_kx` module instance from plugin
        {
            let result = plugin.mod_instance("wasi_crypto_kx");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert_eq!(instance.name().unwrap(), "wasi_ephemeral_crypto_kx");
            assert_eq!(instance.func_len(), 3);
            assert_eq!(
                instance.func_names().unwrap(),
                ["kx_decapsulate", "kx_dh", "kx_encapsulate",],
            );
        }

        // get `wasi_crypto_signatures` module instance from plugin
        {
            let result = plugin.mod_instance("wasi_crypto_signatures");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert_eq!(instance.name().unwrap(), "wasi_ephemeral_crypto_signatures");
            assert_eq!(instance.func_len(), 11);
            assert_eq!(
                instance.func_names().unwrap(),
                [
                    "signature_close",
                    "signature_export",
                    "signature_import",
                    "signature_state_close",
                    "signature_state_open",
                    "signature_state_sign",
                    "signature_state_update",
                    "signature_verification_state_close",
                    "signature_verification_state_open",
                    "signature_verification_state_update",
                    "signature_verification_state_verify",
                ],
            );
        }

        // get `wasi_crypto_symmetric` module instance from plugin
        {
            let result = plugin.mod_instance("wasi_crypto_symmetric");
            assert!(result.is_some());
            let instance = result.unwrap();
            assert_eq!(instance.name().unwrap(), "wasi_ephemeral_crypto_symmetric");
            assert_eq!(instance.func_len(), 28);
            assert_eq!(
                instance.func_names().unwrap(),
                [
                    "symmetric_key_close",
                    "symmetric_key_export",
                    "symmetric_key_from_id",
                    "symmetric_key_generate",
                    "symmetric_key_generate_managed",
                    "symmetric_key_id",
                    "symmetric_key_import",
                    "symmetric_key_replace_managed",
                    "symmetric_key_store_managed",
                    "symmetric_state_absorb",
                    "symmetric_state_clone",
                    "symmetric_state_close",
                    "symmetric_state_decrypt",
                    "symmetric_state_decrypt_detached",
                    "symmetric_state_encrypt",
                    "symmetric_state_encrypt_detached",
                    "symmetric_state_max_tag_len",
                    "symmetric_state_open",
                    "symmetric_state_options_get",
                    "symmetric_state_options_get_u64",
                    "symmetric_state_ratchet",
                    "symmetric_state_squeeze",
                    "symmetric_state_squeeze_key",
                    "symmetric_state_squeeze_tag",
                    "symmetric_tag_close",
                    "symmetric_tag_len",
                    "symmetric_tag_pull",
                    "symmetric_tag_verify",
                ],
            );
        }
    }
}
