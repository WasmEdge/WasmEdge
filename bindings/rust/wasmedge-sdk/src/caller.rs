use crate::{Executor, Instance, Memory};
use wasmedge_sys::CallingFrame;

/// Represents the calling frame on top of stack.
///
/// The [Caller] object is only used as the first argument when defining a host function. With this argument,
/// developers can have access to the [Executor] instance, the [Instance] instance, and the [Memory] instance inside the host functions they defined.
#[derive(Debug)]
pub struct Caller {
    inner: Option<CallingFrame>,
}
impl Caller {
    /// Creates a [Caller] instance with the given [CallingFrame](wasmedge_sys::CallingFrame) instance which is
    /// a low-level object defined in `wasmedge-sys` crate.
    ///
    /// Notice that this function is not used by developers to create a [Caller] instance.
    pub fn new(frame: CallingFrame) -> Self {
        Self { inner: Some(frame) }
    }

    /// Returns the [executor instance](crate::Executor) from this caller.
    pub fn executor(&self) -> Option<Executor> {
        match self.inner {
            Some(ref frame) => frame.executor_mut().map(|inner| Executor { inner }),
            None => None,
        }
    }

    /// Returns the [module instance](crate::Instance) in this caller.
    pub fn instance(&self) -> Option<Instance> {
        match self.inner {
            Some(ref frame) => frame.module_instance().map(|inner| Instance { inner }),
            None => None,
        }
    }

    /// Returns the [memory instance](crate::Memory) by the given index from the module instance of this caller. If
    /// the memory instance is not found, then return `None`.
    ///
    /// By default, a WASM module has only one memory instance after instantiation. Therefore, users can pass in `0` as
    /// the index to get the memory instance in host function body. When the [MultiMemories](crate::config::CommonConfigOptions::multi_memories)
    /// config option is enabled, there would be more than one memory instances in the wasm module. Users can retrieve
    /// the target memory instance by specifying the index of the memory instance in the wasm module instance.
    ///
    /// # Arguments
    ///
    /// * idx - The index of the memory instance.
    ///
    pub fn memory(&self, idx: usize) -> Option<Memory> {
        match self.inner {
            Some(ref frame) => frame.memory_mut(idx as u32).map(|inner| Memory {
                inner,
                name: None,
                mod_name: None,
            }),
            None => None,
        }
    }
}
