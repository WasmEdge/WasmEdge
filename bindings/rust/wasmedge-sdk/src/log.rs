//! Defines WasmEdge LogManager struct

use wasmedge_sys as sys;

/// Manipulates the runtime logger.
#[derive(Debug)]
pub struct LogManager {}
impl LogManager {
    /// Logs the debug information.
    pub fn log_debug_info() {
        sys::utils::log_debug_info()
    }

    /// Logs the error information.
    pub fn log_error_info() {
        sys::utils::log_error_info()
    }

    /// Sets the logging system off.
    pub fn log_off() {
        sys::utils::log_off()
    }
}
