use crate::{ffi, utils::check, WasmValue};
use wasmedge_types::WasmEdgeResult;

/// Defines the WasmEdge Async result.
#[derive(Debug)]
pub struct AsyncResult {
    pub(crate) inner: InnerAsyncResult,
}
impl Drop for AsyncResult {
    fn drop(&mut self) {
        if !self.inner.0.is_null() {
            unsafe {
                ffi::WasmEdge_AsyncDelete(self.inner.0);
            }
        }
    }
}
impl AsyncResult {
    /// Waits a WasmEdge async execution.
    pub fn wait(&self) {
        unsafe {
            ffi::WasmEdge_AsyncWait(self.inner.0);
        }
    }

    /// Waits a WasmEdge async execution with timeout.
    ///
    /// # Argument
    ///
    /// * `timeout` - timeout in milliseconds.
    ///
    /// # Error
    ///
    /// Returns true if the async execution has been completed, false if the timeout has been reached.
    pub fn wait_for(&self, timeout: u64) -> bool {
        unsafe { ffi::WasmEdge_AsyncWaitFor(self.inner.0, timeout) }
    }

    /// Cancels a WasmEdge async execution.
    pub fn cancel(&self) {
        unsafe {
            ffi::WasmEdge_AsyncCancel(self.inner.0);
        }
    }

    /// Waits and gets the result of WasmEdge async execution.
    pub fn get_async(&self) -> WasmEdgeResult<Vec<WasmValue>> {
        let returns_len = unsafe { ffi::WasmEdge_AsyncGetReturnsLength(self.inner.0) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            check(ffi::WasmEdge_AsyncGet(
                self.inner.0,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect::<Vec<_>>())
    }
}

#[derive(Debug)]
pub(crate) struct InnerAsyncResult(pub(crate) *mut ffi::WasmEdge_Async);
unsafe impl Send for InnerAsyncResult {}
unsafe impl Sync for InnerAsyncResult {}
