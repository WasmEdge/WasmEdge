use crate::{Executor, Instance, Memory};
use wasmedge_sys::CallingFrame;

#[derive(Debug)]
pub struct Caller<'a> {
    inner: Option<&'a CallingFrame>,
}
impl<'a> Caller<'a> {
    pub fn new(frame: &'a CallingFrame) -> Self {
        Self { inner: Some(frame) }
    }

    pub fn executor(&self) -> Option<Executor> {
        match self.inner {
            Some(frame) => match frame.executor_mut() {
                Some(inner) => Some(Executor { inner }),
                None => None,
            },
            None => None,
        }
    }

    pub fn instance(&self) -> Option<Instance> {
        match self.inner {
            Some(frame) => match frame.module_instance() {
                Some(inner) => Some(Instance { inner }),
                None => None,
            },
            None => None,
        }
    }

    pub fn memory(&self, idx: usize) -> Option<Memory> {
        match self.inner {
            Some(frame) => match frame.memory_mut(idx as u32) {
                Some(inner) => Some(Memory {
                    inner,
                    name: None,
                    mod_name: None,
                }),
                None => None,
            },
            None => None,
        }
    }
}
