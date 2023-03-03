use crate::{Executor, Instance, Memory};
use wasmedge_sys::CallingFrame;
use wasmedge_types::MemoryType;

/// Represents the calling frame on top of stack.
///
/// The [Caller] object is only used as the first argument when defining a host function. With this argument,
/// developers can have access to the [Executor] instance, the [Instance] instance, and the [Memory] instance inside the host functions they defined.
#[derive(Debug)]
pub struct Caller {
    inner: Option<CallingFrame>,
    executor: Option<Executor>,
    instance: Option<Instance>,
}
impl Caller {
    /// Creates a Caller instance with the given [CallingFrame](wasmedge_sys::CallingFrame) instance.
    ///
    /// # Caller vs. CallingFrame
    ///
    /// `CallingFrame` is a low-level type defined in `wasmedge-sys` crate, while `Caller` is a high-level type. For developers using the APIs in `wasmedge-sdk`, they should create a `Caller` instance with the given `CallingFrame` instance, as `Caller` provides APIs to access high-level instances, such as executor and memory, related to the current calling frame.
    ///
    pub fn new(frame: CallingFrame) -> Self {
        let executor = frame.executor_mut().map(|inner| Executor { inner });
        let instance = frame.module_instance().map(|inner| Instance { inner });

        Self {
            inner: Some(frame),
            executor,
            instance,
        }
    }

    /// Returns the [executor instance](crate::Executor) from this caller.
    pub fn executor(&self) -> Option<&Executor> {
        self.executor.as_ref()
    }

    /// Returns the mutable [executor instance](crate::Executor) from this caller.
    pub fn executor_mut(&mut self) -> Option<&mut Executor> {
        self.executor.as_mut()
    }

    /// Returns the [module instance](crate::Instance) in this caller.
    pub fn instance(&self) -> Option<&Instance> {
        self.instance.as_ref()
    }

    /// Returns the mutable [module instance](crate::Instance) in this caller.
    pub fn instance_mut(&mut self) -> Option<&mut Instance> {
        self.instance.as_mut()
    }

    /// Returns the [memory instance](crate::Memory) by the given index from the module instance of this caller. If
    /// the memory instance is not found, then return `None`.
    ///
    /// # Arguments
    ///
    /// * `idx` - The index of the memory instance. By default, a WASM module has only one memory instance after instantiation. Therefore, users can pass in `0` as the index to get the memory instance in host function body. When the [MultiMemories](crate::config::CommonConfigOptions::multi_memories) config option is enabled, there would be more than one memory instances in the wasm module. Users can retrieve the target memory instance by specifying the index of the memory instance in the wasm module instance.
    ///
    pub fn memory(&self, idx: u32) -> Option<Memory> {
        match self.inner.as_ref() {
            Some(frame) => frame.memory_mut(idx).map(|inner| {
                let ty: MemoryType = inner.ty().unwrap().into();
                Memory {
                    inner,
                    name: None,
                    mod_name: None,
                    ty,
                }
            }),
            None => None,
        }
    }
}
impl From<CallingFrame> for Caller {
    fn from(frame: CallingFrame) -> Self {
        Self::new(frame)
    }
}
