//! Defines data structure for WasmEdge async mechanism.

use fiber_for_wasmedge::{Fiber, FiberStack, Suspend};
use std::{
    future::Future,
    pin::Pin,
    ptr,
    task::{Context, Poll},
};

/// Defines a FiberFuture.
pub(crate) struct FiberFuture<'a> {
    fiber: Fiber<'a, Result<(), ()>, (), Result<(), ()>>,
    current_suspend: *mut *const Suspend<Result<(), ()>, (), Result<(), ()>>,
    current_poll_cx: *mut *mut Context<'static>,
}
impl<'a> FiberFuture<'a> {
    /// Create a fiber to execute the given function.
    ///
    /// # Arguments
    ///
    /// * `func` - The function to execute.
    ///
    /// # Error
    ///
    /// If fail to create the fiber stack or the fiber fail to resume, then an error is returned.
    pub(crate) async fn on_fiber<R>(
        async_state: &AsyncState,
        func: impl FnOnce() -> R + Send,
    ) -> Result<R, ()> {
        let mut slot = None;
        let future = {
            let current_poll_cx = async_state.current_poll_cx.get();
            let current_suspend = async_state.current_suspend.get();

            let stack = FiberStack::new(2 << 20).map_err(|_e| ())?;
            let slot = &mut slot;
            let fiber = Fiber::new(stack, move |keep_going, suspend| {
                keep_going?;

                unsafe {
                    let _reset = Reset(current_suspend, *current_suspend);
                    *current_suspend = suspend;
                    *slot = Some(func());
                    Ok(())
                }
            })
            .map_err(|_e| ())?;

            FiberFuture {
                fiber,
                current_suspend,
                current_poll_cx,
            }
        };
        future.await?;

        Ok(slot.unwrap())
    }
}
impl<'a> Future for FiberFuture<'a> {
    type Output = Result<(), ()>;
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        unsafe {
            let _reset = Reset(self.current_poll_cx, *self.current_poll_cx);
            *self.current_poll_cx =
                std::mem::transmute::<&mut Context<'_>, *mut Context<'static>>(cx);

            let async_cx = AsyncCx {
                current_suspend: self.current_suspend,
                current_poll_cx: self.current_poll_cx,
            };
            ASYNC_CX.set(&async_cx, || match self.as_ref().fiber.resume(Ok(())) {
                Ok(ret) => Poll::Ready(ret),
                Err(_) => Poll::Pending,
            })
        }
    }
}
unsafe impl Send for FiberFuture<'_> {}

type FiberSuspend = Suspend<Result<(), ()>, (), Result<(), ()>>;

scoped_tls::scoped_thread_local!(static ASYNC_CX: AsyncCx);

/// Defines a async state that contains the pointer to current poll context and current suspend.
#[derive(Debug)]
pub struct AsyncState {
    current_suspend: std::cell::UnsafeCell<*const FiberSuspend>,
    current_poll_cx: std::cell::UnsafeCell<*mut Context<'static>>,
}
impl Default for AsyncState {
    fn default() -> Self {
        Self::new()
    }
}
impl AsyncState {
    /// Creates a new async state.
    pub fn new() -> Self {
        AsyncState {
            current_suspend: std::cell::UnsafeCell::new(std::ptr::null()),
            current_poll_cx: std::cell::UnsafeCell::new(std::ptr::null_mut()),
        }
    }

    /// Returns an async execution context.
    ///
    /// If the pointer of poll context is null, then None is returned.
    pub fn async_cx(&self) -> Option<AsyncCx> {
        let poll_cx_box_ptr = self.current_poll_cx.get();
        if poll_cx_box_ptr.is_null() {
            return None;
        }
        let poll_cx_inner_ptr = unsafe { *poll_cx_box_ptr };
        if poll_cx_inner_ptr.is_null() {
            return None;
        }

        Some(AsyncCx {
            current_suspend: self.current_suspend.get(),
            current_poll_cx: poll_cx_box_ptr,
        })
    }
}
unsafe impl Send for AsyncState {}
unsafe impl Sync for AsyncState {}

/// Defines an async execution context.
#[derive(Debug, Clone, Copy)]
pub struct AsyncCx {
    current_suspend: *mut *const Suspend<Result<(), ()>, (), Result<(), ()>>,
    current_poll_cx: *mut *mut Context<'static>,
}
impl Default for AsyncCx {
    fn default() -> Self {
        Self::new()
    }
}
impl AsyncCx {
    /// Creates a new async execution context.
    pub fn new() -> Self {
        ASYNC_CX.with(|async_cx| *async_cx)
    }

    /// Runs a future to completion.
    ///
    /// # Arguments
    ///
    /// * `future` - The future to run.
    ///
    /// # Error
    ///
    /// If fail to run, then an error is returned.
    pub(crate) unsafe fn block_on<U>(
        &self,
        mut future: Pin<&mut (dyn Future<Output = U> + Send)>,
    ) -> Result<U, ()> {
        let suspend = *self.current_suspend;
        let _reset = Reset(self.current_suspend, suspend);
        *self.current_suspend = ptr::null();
        assert!(!suspend.is_null());

        loop {
            let future_result = {
                let poll_cx = *self.current_poll_cx;
                let _reset = Reset(self.current_poll_cx, poll_cx);
                *self.current_poll_cx = ptr::null_mut();
                assert!(!poll_cx.is_null());
                future.as_mut().poll(&mut *poll_cx)
            };

            match future_result {
                Poll::Ready(t) => break Ok(t),
                Poll::Pending => {}
            }
            let res = (*suspend).suspend(());
            res?;
        }
    }
}

struct Reset<T: Copy>(*mut T, T);
impl<T: Copy> Drop for Reset<T> {
    fn drop(&mut self) {
        unsafe {
            *self.0 = self.1;
        }
    }
}
