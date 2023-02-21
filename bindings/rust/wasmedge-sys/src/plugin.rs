//! Defines PluginManager and Plugin structs.

use super::ffi;
use crate::{
    instance::module::InnerInstance, types::WasmEdgeString, utils, Instance, WasmEdgeResult,
};

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
}

#[derive(Debug)]
pub(crate) struct InnerPlugin(pub(crate) *mut ffi::WasmEdge_PluginContext);
unsafe impl Send for InnerPlugin {}
unsafe impl Sync for InnerPlugin {}

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(all(
        target_os = "linux",
        feature = "wasmedge_process",
        not(feature = "static")
    ))]
    #[test]
    fn test_plugin_wasmedge_process() {
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
}
