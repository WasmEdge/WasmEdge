use crate::{wasmedge, Func, Global, Memory, Table};

pub struct ImportObj {
    pub(crate) inner: wasmedge::ImportObj,
}
impl ImportObj {
    pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Func) {
        unimplemented!()
    }
    pub fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
        unimplemented!()
    }
    pub fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
        unimplemented!()
    }
    pub fn add_table(&mut self, name: impl AsRef<str>, global: &mut Table) {
        unimplemented!()
    }
}
