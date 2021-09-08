#![deny(rust_2018_idioms, unreachable_pub)]

#[allow(warnings)]
pub mod wasmedge {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}

pub mod vm;
pub mod value;
pub mod config;
pub mod module;
pub mod string;
pub mod raw_result;
pub mod version;
pub mod statistics;


pub use version::{full_version, semv_version};
pub use raw_result::ErrReport;
pub use string::{WasmEdgeString, StringBuf, StringRef};
pub use value::Value;
pub use config::{Config, OptLevel};
pub use module::Module;
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
