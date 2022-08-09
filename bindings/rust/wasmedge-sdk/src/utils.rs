use wasmedge_sys::utils;

/// Defines WasmEdge Driver functions
#[derive(Debug)]
pub struct Driver {}
impl Driver {
    /// Triggers the WasmEdge AOT compiler tool
    pub fn aot_compiler<I, V>(args: I) -> i32
    where
        I: IntoIterator<Item = V>,
        V: AsRef<str>,
    {
        utils::driver_aot_compiler(args)
    }

    /// Triggers the WasmEdge runtime tool
    pub fn runtime_tool<I, V>(args: I) -> i32
    where
        I: IntoIterator<Item = V>,
        V: AsRef<str>,
    {
        utils::driver_runtime_tool(args)
    }
}
