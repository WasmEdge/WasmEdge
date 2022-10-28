use std::{
    future::Future,
    pin::Pin,
    ptr,
    task::{Context, Poll},
};
use wasmtime_fiber::{Fiber, FiberStack, Suspend};

use crate::ASYNC_STATE;
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
}

impl<'a> FiberFuture<'a> {
    pub(crate) async fn on_fiber<R>(
        func: impl FnOnce() -> R + Send,
    ) -> Result<R, ()> {
        let mut slot = None;
        let future = {
            let async_state = ASYNC_STATE.read();
            let current_poll_cx = async_state.current_poll_cx.get();
            let current_suspend = async_state.current_suspend.get();
            drop(async_state);
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

            FiberFuture {fiber, current_poll_cx}
        };
        future.await?;

        return Ok(slot.unwrap());
    }
}
impl<'a> Future for FiberFuture<'a> {
    type Output = Result<(), ()>;
    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        unsafe {
            let _reset = Reset(self.current_poll_cx, *self.current_poll_cx);
            *self.current_poll_cx =
                std::mem::transmute::<&mut Context<'_>, *mut Context<'static>>(cx);
            match self.as_ref().fiber.resume(Ok(())) {
                Ok(ret) => Poll::Ready(ret),
                Err(_) => Poll::Pending,
            }
        }

    }
}

unsafe impl Send for FiberFuture<'_> {}

#[derive(Debug)]
pub(crate) struct AsyncState {
    pub(crate) current_suspend:
        std::cell::UnsafeCell<*const Suspend<Result<(), ()>, (), Result<(), ()>>>,
    pub(crate) current_poll_cx: std::cell::UnsafeCell<*mut Context<'static>>,
}

unsafe impl Send for AsyncState {}
unsafe impl Sync for AsyncState {}

impl AsyncState {
    pub(crate) fn async_cx(&self) -> Option<AsyncCx> {
        let poll_cx_box_ptr = self.current_poll_cx.get();
        if poll_cx_box_ptr.is_null() {
            println!("ptr null");
            return None;
        }
        let poll_cx_inner_ptr = unsafe { *poll_cx_box_ptr };
        if poll_cx_inner_ptr.is_null() {
            println!("inner ptr null");
            return None;
        }

        Some(AsyncCx {
            current_suspend: self.current_suspend.get(),
            current_poll_cx: poll_cx_box_ptr,
        })
    }
}

#[derive(Debug)]
pub(crate) struct AsyncCx {
    pub(crate) current_suspend:
        *mut *const Suspend<Result<(), ()>, (), Result<(), ()>>,
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
