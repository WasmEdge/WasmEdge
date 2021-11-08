#![deny(rust_2018_idioms, unreachable_pub)]

#[allow(warnings)]
pub mod wasmedge {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}

pub mod config;
pub mod function;
pub mod import_obj;
pub mod instance;
pub mod module;
pub mod raw_result;
pub mod string;
pub mod value;
pub mod version;
pub mod vm;
pub mod wasi;

pub use config::{Config, OptLevel};
pub use module::Module;
pub use raw_result::ErrReport;
pub use string::{StringBuf, StringRef, WasmEdgeString};
pub use value::Value;
pub use version::{full_version, semv_version};
pub use vm::Vm;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn links() {
        unsafe {
            assert!(
                wasmedge::WasmEdge_VersionGetMajor()
                    + wasmedge::WasmEdge_VersionGetMinor()
                    + wasmedge::WasmEdge_VersionGetPatch()
                    != 0
            );
        }
    }
}
