use crate::{Func, Global, Memory, Store, Table};
use std::marker::PhantomData;

pub struct Instance {
    pub(crate) name: Option<String>,
}
impl Instance {
    pub fn func(&self, store: &Store, name: impl AsRef<str>) -> Option<Func> {
        match self.name {
            Some(ref mod_name) => store
                .inner
                .find_func_registered(mod_name, name.as_ref())
                .ok()
                .map(|inner| Func {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                }),
            None => store.inner.find_func(name.as_ref()).ok().map(|inner| Func {
                inner,
                name: Some(name.as_ref().into()),
                mod_name: None,
            }),
        }
    }

    pub fn global(&self, store: &Store, name: impl AsRef<str>) -> Option<Global> {
        match self.name {
            Some(ref mod_name) => store
                .inner
                .find_global_registered(mod_name, name.as_ref())
                .ok()
                .map(|inner| Global {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                }),
            None => store
                .inner
                .find_global(name.as_ref())
                .ok()
                .map(|inner| Global {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                }),
        }
    }

    pub fn memory(&self, store: &Store, name: impl AsRef<str>) -> Option<Memory> {
        match self.name {
            Some(ref mod_name) => store
                .inner
                .find_memory_registered(mod_name, name.as_ref())
                .ok()
                .map(|inner| Memory {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                }),
            None => store
                .inner
                .find_memory(name.as_ref())
                .ok()
                .map(|inner| Memory {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                }),
        }
    }

    pub fn table(&self, store: &Store, name: impl AsRef<str>) -> Option<Table> {
        match self.name {
            Some(ref mod_name) => store
                .inner
                .find_table_registered(mod_name, name.as_ref())
                .ok()
                .map(|inner| Table {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: Some(mod_name.into()),
                    _marker: PhantomData,
                }),
            None => store
                .inner
                .find_table(name.as_ref())
                .ok()
                .map(|inner| Table {
                    inner,
                    name: Some(name.as_ref().into()),
                    mod_name: None,
                    _marker: PhantomData,
                }),
        }
    }
}
