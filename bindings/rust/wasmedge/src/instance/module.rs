use crate::{wasmedge, Func, Global, Memory, Table};
use std::marker::PhantomData;

pub struct Instance<'store> {
    pub(crate) inner: wasmedge::Instance<'store>,
}
impl<'store> Instance<'store> {
    pub fn name(&self) -> Option<String> {
        self.inner.name()
    }

    pub fn func_len(&self) -> usize {
        self.inner.func_len() as usize
    }

    pub fn func_names(&self) -> Option<Vec<String>> {
        self.inner.func_names()
    }

    pub fn func(&self, name: impl AsRef<str>) -> Option<Func> {
        let inner_func = self.inner.find_func(name.as_ref()).ok();
        if let Some(inner_func) = inner_func {
            return Some(Func {
                inner: inner_func,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
            });
        }

        None
    }

    pub fn global(&self, name: impl AsRef<str>) -> Option<Global> {
        let inner_global = self.inner.find_global(name.as_ref()).ok();
        if let Some(inner_global) = inner_global {
            return Some(Global {
                inner: inner_global,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn global_len(&self) -> usize {
        self.inner.global_len() as usize
    }

    pub fn global_names(&self) -> Option<Vec<String>> {
        self.inner.global_names()
    }

    pub fn memory(&self, name: impl AsRef<str>) -> Option<Memory> {
        let inner_memory = self.inner.find_memory(name.as_ref()).ok();
        if let Some(inner_memory) = inner_memory {
            return Some(Memory {
                inner: inner_memory,
                name: Some(name.as_ref().into()),
                mod_name: self.inner.name(),
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn memory_len(&self) -> usize {
        self.inner.mem_len() as usize
    }

    pub fn memory_names(&self) -> Option<Vec<String>> {
        self.inner.mem_names()
    }

    pub fn table(&self, name: impl AsRef<str>) -> Option<Table> {
        let inner_table = self.inner.find_table(name.as_ref()).ok();
        if let Some(inner_table) = inner_table {
            return Some(Table {
                inner: inner_table,
                name: Some(name.as_ref().into()),
                mod_name: None,
                _marker: PhantomData,
            });
        }

        None
    }

    pub fn table_len(&self) -> usize {
        self.inner.table_len() as usize
    }

    pub fn table_names(&self) -> Option<Vec<String>> {
        self.inner.table_names()
    }
}
