//! Defines WasmEdge CallingFrame.

use crate::{
    executor::InnerExecutor,
    ffi,
    instance::{memory::InnerMemory, module::InnerInstance},
    Executor, Instance, Memory,
};

/// Represents a calling frame on top of stack.
#[derive(Debug)]
pub struct CallingFrame {
    pub(crate) inner: InnerCallingFrame,
}
impl Drop for CallingFrame {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            self.inner.0 = std::ptr::null();
        }
    }
}
impl CallingFrame {
    /// Creates a CallingFrame instance.
    pub(crate) fn create(ctx: *const ffi::WasmEdge_CallingFrameContext) -> Self {
        Self {
            inner: InnerCallingFrame(ctx),
        }
    }

    /// Returns the [executor instance](crate::Executor) from this calling frame.
    pub fn executor_mut(&self) -> Option<Executor> {
        let ctx = unsafe { ffi::WasmEdge_CallingFrameGetExecutor(self.inner.0) };

        match ctx.is_null() {
            false => Some(Executor {
                inner: InnerExecutor(ctx),
                registered: true,
            }),
            true => None,
        }
    }

    /// Returns the [module instance](crate::Instance) in this calling frame.
    ///
    /// If the executing function instance is a host function and not added into any module instance, then returns `None`.
    ///
    /// When a wasm function is executing and tring to call a host function inside, a frame with the module
    /// instance the wasm function belongs to will be pushed onto the stack. And therefore the calling frame
    /// context will record that module instance.
    ///
    pub fn module_instance(&self) -> Option<Instance> {
        let ctx = unsafe { ffi::WasmEdge_CallingFrameGetModuleInstance(self.inner.0) };

        match ctx.is_null() {
            false => Some(Instance {
                inner: std::sync::Arc::new(InnerInstance(ctx as *mut _)),
                registered: true,
            }),
            true => None,
        }
    }

    /// Returns the [memory instance](crate::Memory) by the given index from the module instance of the current
    /// calling frame. If the memory instance is not found, returns `None`.
    ///
    /// By default, a WASM module has only one memory instance after instantiation. Therefore, users can pass in `0` as
    /// the index to get the memory instance in host function body. When the [MultiMemories](crate::Config::multi_memories)
    /// config option is enabled, there would be more than one memory instances in the wasm module. Users can retrieve
    /// the target memory instance by specifying the index of the memory instance in the wasm module instance.
    ///
    /// # Arguments
    ///
    /// * idx - The index of the memory instance.
    ///
    pub fn memory_mut(&self, idx: u32) -> Option<Memory> {
        let ctx = unsafe { ffi::WasmEdge_CallingFrameGetMemoryInstance(self.inner.0, idx) };

        match ctx.is_null() {
            false => Some(Memory {
                inner: InnerMemory(ctx),
                registered: true,
            }),
            true => None,
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerCallingFrame(pub(crate) *const ffi::WasmEdge_CallingFrameContext);
unsafe impl Send for InnerCallingFrame {}
unsafe impl Sync for InnerCallingFrame {}
