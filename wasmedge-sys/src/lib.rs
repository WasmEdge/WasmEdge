#![deny(rust_2018_idioms, unreachable_pub)]

#[allow(warnings)]
pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/wasmedge.rs"));
}
pub use ffi::*;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn links() {
        unsafe {
            assert!(
                WasmEdge_VersionGetMajor()
                    + WasmEdge_VersionGetMinor()
                    + WasmEdge_VersionGetPatch()
                    != 0
            );
        }
    }
}
