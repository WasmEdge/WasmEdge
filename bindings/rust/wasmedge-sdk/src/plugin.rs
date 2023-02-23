//! Defines PluginManager struct

use crate::{instance::Instance, WasmEdgeResult};
use wasmedge_sys as sys;

/// Defines the API to manage plugins.
#[derive(Debug)]
pub struct PluginManager {}
impl PluginManager {
    /// Load plugins from the given path.
    ///
    /// * If the path is not given, then the default plugin paths will be used. The default plugin paths are
    ///
    ///     * The environment variable "WASMEDGE_PLUGIN_PATH".
    ///   
    ///     * The `../plugin/` directory related to the WasmEdge installation path.
    ///
    ///     * The `wasmedge/` directory under the library path if the WasmEdge is installed under the "/usr".
    ///
    /// * If the path is given, then
    ///
    ///     * If the path is pointing at a file , then it indicates that a single plugin will be loaded from the file.
    ///
    ///     * If the path is pointing at a directory, then the method will load plugins from the files in the directory.
    ///
    /// # Argument
    ///
    /// * `path` - A path to a plugin file or a directory holding plugin files. If `None`, then the default plugin path will be used.
    pub fn load(path: Option<&std::path::Path>) -> WasmEdgeResult<()> {
        match path {
            Some(p) => sys::plugin::PluginManager::load_plugins(p),
            None => {
                sys::plugin::PluginManager::load_plugins_from_default_paths();
                Ok(())
            }
        }
    }

    /// Returns the count of loaded plugins.
    pub fn count() -> u32 {
        sys::plugin::PluginManager::count()
    }

    /// Returns the names of all loaded plugins.
    pub fn names() -> Vec<String> {
        sys::plugin::PluginManager::names()
    }

    /// Returns the target plugin by its name.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target plugin.
    pub fn find(name: impl AsRef<str>) -> Option<Plugin> {
        sys::plugin::PluginManager::find(name.as_ref()).map(|p| Plugin { inner: p })
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
        sys::plugin::PluginManager::init_wasmedge_process(allowed_cmds, allowed);
    }
}

/// Represents a loaded plugin. It provides the APIs for accessing the plugin.
#[derive(Debug)]
pub struct Plugin {
    inner: sys::plugin::Plugin,
}
impl Plugin {
    /// Returns the name of this plugin.
    pub fn name(&self) -> String {
        self.inner.name()
    }

    /// Returns the count of the module instances in this plugin.
    pub fn mod_count(&self) -> u32 {
        self.inner.mod_count()
    }

    /// Returns the names of all module instances in this plugin.
    pub fn mod_names(&self) -> Vec<String> {
        self.inner.mod_names()
    }

    /// Returns a module instance that is generated from the module with the given name in this plugin.
    ///
    /// # Argument
    ///
    /// * `name` - The name of the target module.
    pub fn mod_instance(&self, name: impl AsRef<str>) -> Option<Instance> {
        self.inner
            .mod_instance(name.as_ref())
            .map(|i| Instance { inner: i })
    }
}
