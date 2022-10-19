use std::{
    cell::UnsafeCell,
    future::Future,
    pin::Pin,
    ptr,
    task::{Context, Poll},
};
use wasmtime_fiber::Fiber;
pub(crate) struct Reset<T: Copy>(pub(crate) *mut T, pub(crate) T);

impl<T: Copy> Drop for Reset<T> {
    fn drop(&mut self) {
        unsafe {
            *self.0 = self.1;
        }
    }
}

pub(crate) struct FiberFuture<'a> {
    fiber: Fiber<'a, Result<(), ()>, (), Result<(), ()>>,
    current_poll_cx: *mut *mut Context<'static>,
    // engine: Engine,
}

impl<'a> FiberFuture<'a> {
    pub(crate) fn new(
        fiber: Fiber<'a, Result<(), ()>, (), Result<(), ()>>,
        current_poll_cx: *mut *mut Context<'static>,
    ) -> Self {
        FiberFuture {
            fiber,
            current_poll_cx,
        }
    }
}

unsafe impl Send for FiberFuture<'_> {}

impl Future for FiberFuture<'_> {
    type Output = Result<(), ()>;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        unsafe {
            let _reset = Reset(self.current_poll_cx, *self.current_poll_cx);
            *self.current_poll_cx =
                std::mem::transmute::<&mut Context<'_>, *mut Context<'static>>(cx);

            match self.fiber.resume(Ok(())) {
                Ok(result) => Poll::Ready(result),
                Err(()) => Poll::Pending,
            }
        }
    }
}

impl Drop for FiberFuture<'_> {
    fn drop(&mut self) {
        if !self.fiber.done() {
            let result = self.fiber.resume(Err(()));
            // This resumption with an error should always complete the
            // fiber. While it's technically possible for host code to catch
            // the trap and re-resume, we'd ideally like to signal that to
            // callers that they shouldn't be doing that.
            debug_assert!(result.is_ok());
        }
    }
}

#[derive(Debug)]
pub struct AsyncState {
    pub current_suspend:
        UnsafeCell<*const wasmtime_fiber::Suspend<Result<(), ()>, (), Result<(), ()>>>,
    pub current_poll_cx: UnsafeCell<*mut Context<'static>>,
}

unsafe impl Send for AsyncState {}
unsafe impl Sync for AsyncState {}

#[derive(Debug)]
pub(crate) struct AsyncCx {
    pub(crate) current_suspend:
        *mut *const wasmtime_fiber::Suspend<Result<(), ()>, (), Result<(), ()>>,
    pub(crate) current_poll_cx: *mut *mut Context<'static>,
}

impl AsyncCx {
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
