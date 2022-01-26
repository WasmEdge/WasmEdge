use crate::{error::WasmEdgeResult, wasmedge, Engine, Func, Global, Memory, Table};

pub struct ImportObj {
    pub(crate) inner: wasmedge::ImportObj,
}
impl ImportObj {
    pub fn new(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let inner = wasmedge::ImportObj::create(name.as_ref())?;
        Ok(Self { inner })
    }

    pub fn new_wasi_and_join(
        mut engine: impl Engine,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) -> WasmEdgeResult<()> {
        let inner = wasmedge::ImportObj::create_wasi(args, envs, preopens)?;
        let mut import = Self { inner };
        engine.register_wasm_from_import(&mut import);

        Ok(())
    }

    pub fn init_wasi() {}

    pub fn wasi_exit_code() {}

    pub fn new_wasmedge_process_and_join(
        mut engine: impl Engine,
        allowed_cmds: Option<Vec<&str>>,
        allowed: bool,
    ) -> WasmEdgeResult<()> {
        let inner = wasmedge::ImportObj::create_wasmedge_process(allowed_cmds, allowed)?;
        let mut import = Self { inner };
        engine.register_wasm_from_import(&mut import);
        Ok(())
    }

    pub fn init_wasmedge_process() {}

    // pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Func) {
    //     unimplemented!()
    // }
    // pub fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
    //     unimplemented!()
    // }
    // pub fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
    //     unimplemented!()
    // }
    // pub fn add_table(&mut self, name: impl AsRef<str>, global: &mut Table) {
    //     unimplemented!()
    // }
}
