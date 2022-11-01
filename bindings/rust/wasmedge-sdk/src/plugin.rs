//! Defines PluginManager struct

use wasmedge_sys as sys;

/// Defines the API to manage plugins.
#[derive(Debug)]
pub struct PluginManager {}
impl PluginManager {
    /// Loads plugins from default paths. The default paths include:
    ///
    /// * The path specified by the `WASMEDGE_PLUGIN_PATH` environment variable.
    ///
    pub fn load_from_default_paths() {
        sys::utils::load_plugin_from_default_paths();
    }
}
