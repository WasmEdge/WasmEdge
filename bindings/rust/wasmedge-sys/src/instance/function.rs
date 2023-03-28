//! Defines WasmEdge Function and FuncType structs.

use crate::{
    error::{FuncError, HostFuncError, WasmEdgeError},
    ffi, BoxedFn, CallingFrame, Engine, WasmEdgeResult, WasmValue, HOST_FUNCS,
};
#[cfg(feature = "async")]
use crate::{r#async::FiberFuture, ASYNC_STATE};
use core::ffi::c_void;
use parking_lot::Mutex;
use rand::Rng;
#[cfg(feature = "async")]
use std::pin::Pin;
use std::{convert::TryInto, sync::Arc};
use wasmedge_types::ValType;

// Wrapper function for thread-safe scenarios.
extern "C" fn wrap_fn(
    key_ptr: *mut c_void,
    _data: *mut std::os::raw::c_void,
    call_frame_ctx: *const ffi::WasmEdge_CallingFrameContext,
    params: *const ffi::WasmEdge_Value,
    param_len: u32,
    returns: *mut ffi::WasmEdge_Value,
    return_len: u32,
) -> ffi::WasmEdge_Result {
    let frame = CallingFrame::create(call_frame_ctx);

    let key = key_ptr as *const usize as usize;

    let input = {
        let raw_input = unsafe {
            std::slice::from_raw_parts(
                params,
                param_len
                    .try_into()
                    .expect("len of params should not greater than usize"),
            )
        };
        raw_input.iter().map(|r| (*r).into()).collect::<Vec<_>>()
    };

    let return_len = return_len
        .try_into()
        .expect("len of returns should not greater than usize");
    let raw_returns = unsafe { std::slice::from_raw_parts_mut(returns, return_len) };
    let map_host_func = HOST_FUNCS.read();
    match map_host_func.get(&key) {
        None => unsafe { ffi::WasmEdge_ResultGen(ffi::WasmEdge_ErrCategory_WASM, 5) },
        Some(host_func) => {
            let real_fn = Arc::clone(host_func);
            let real_fn_locked = real_fn.lock();
            drop(map_host_func);

            match real_fn_locked(frame, input) {
                Ok(returns) => {
                    assert!(returns.len() == return_len, "[wasmedge-sys] check the number of returns of host function. Expected: {}, actual: {}", return_len, returns.len());
                    for (idx, wasm_value) in returns.into_iter().enumerate() {
                        raw_returns[idx] = wasm_value.as_raw();
                    }
                    ffi::WasmEdge_Result { Code: 0 }
                }
                Err(err) => match err {
                    HostFuncError::User(code) => unsafe {
                        ffi::WasmEdge_ResultGen(ffi::WasmEdge_ErrCategory_UserLevelError, code)
                    },
                    HostFuncError::Runtime(code) => unsafe {
                        ffi::WasmEdge_ResultGen(ffi::WasmEdge_ErrCategory_WASM, code)
                    },
                },
            }
        }
    }
}

pub type CustomFnWrapper = unsafe extern "C" fn(
    key_ptr: *mut c_void,
    data_ptr: *mut c_void,
    calling_frame_ctx: *const ffi::WasmEdge_CallingFrameContext,
    params: *const ffi::WasmEdge_Value,
    param_len: u32,
    returns: *mut ffi::WasmEdge_Value,
    return_len: u32,
) -> ffi::WasmEdge_Result;

/// Defines a host function.
///
/// A WasmEdge [Function] defines a WebAssembly host function described by its [type](crate::FuncType). A host function is a closure of the original function defined in either the host or the WebAssembly module.
#[derive(Debug)]
pub struct Function {
    pub(crate) inner: Arc<InnerFunc>,
    pub(crate) registered: bool,
}
impl Function {
    /// Creates a [host function](crate::Function) with the given function type.
    ///
    /// N.B. that this function is used for thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `ty` - The types of the arguments and returns of the target function.
    ///
    /// * `real_fn` - The pointer to the target function.
    ///
    /// * `cost` - The function cost in the [Statistics](crate::Statistics). Pass 0 if the calculation is not needed.
    ///
    /// # Error
    ///
    /// * If fail to create a [Function], then [WasmEdgeError::Func(FuncError::Create)](crate::error::FuncError) is returned.
    ///
    /// # Example
    ///
    /// The example defines a host function `real_add`, and creates a [Function] binding to it by calling
    /// the `create_binding` method.
    ///
    /// ```rust
    /// use wasmedge_macro::sys_host_function;
    /// use wasmedge_sys::{FuncType, Function, WasmValue, CallingFrame};
    /// use wasmedge_types::{error::HostFuncError, ValType, WasmEdgeResult};
    ///
    /// #[sys_host_function]
    /// fn real_add(_frame: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    ///     if inputs.len() != 2 {
    ///         return Err(HostFuncError::User(1));
    ///     }
    ///
    ///     let a = if inputs[0].ty() == ValType::I32 {
    ///         inputs[0].to_i32()
    ///     } else {
    ///         return Err(HostFuncError::User(2));
    ///     };
    ///
    ///     let b = if inputs[1].ty() == ValType::I32 {
    ///         inputs[1].to_i32()
    ///     } else {
    ///         return Err(HostFuncError::User(3));
    ///     };
    ///
    ///     let c = a + b;
    ///
    ///     Ok(vec![WasmValue::from_i32(c)])
    /// }
    ///
    /// // create a FuncType
    /// let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]).expect("fail to create a FuncType");
    ///
    /// // create a Function instance
    /// let func = Function::create(&func_ty, Box::new(real_add), 0).expect("fail to create a Function instance");
    /// ```
    pub fn create(ty: &FuncType, real_fn: BoxedFn, cost: u64) -> WasmEdgeResult<Self> {
        unsafe { Self::create_with_data(ty, real_fn, std::ptr::null_mut(), cost) }
    }

    /// Creates a [host function](crate::Function) with the given function type.
    ///
    /// N.B. that this function is used for thread-safe scenarios.
    ///
    /// # Arguments
    ///
    /// * `ty` - The types of the arguments and returns of the target function.
    ///
    /// * `real_fn` - The pointer to the target function.
    ///
    /// * `data` - The pointer to the data.
    ///
    /// * `cost` - The function cost in the [Statistics](crate::Statistics). Pass 0 if the calculation is not needed.
    ///
    /// # Error
    ///
    /// * If fail to create a [Function], then [WasmEdgeError::Func(FuncError::Create)](crate::error::FuncError) is returned.
    ///
    pub unsafe fn create_with_data(
        ty: &FuncType,
        real_fn: BoxedFn,
        data: *mut c_void,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        let mut map_host_func = HOST_FUNCS.write();
        if map_host_func.len() >= map_host_func.capacity() {
            return Err(Box::new(WasmEdgeError::Func(FuncError::CreateBinding(
                format!(
                    "The number of the host functions reaches the upper bound: {}",
                    map_host_func.capacity()
                ),
            ))));
        }

        // generate key for the coming host function
        let mut rng = rand::thread_rng();
        let mut key: usize = rng.gen();
        while map_host_func.contains_key(&key) {
            key = rng.gen();
        }
        map_host_func.insert(key, Arc::new(Mutex::new(real_fn)));
        drop(map_host_func);

        let ctx = ffi::WasmEdge_FunctionInstanceCreateBinding(
            ty.inner.0,
            Some(wrap_fn),
            key as *const usize as *mut c_void,
            data,
            cost,
        );

        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Func(FuncError::Create))),
            false => Ok(Self {
                inner: Arc::new(InnerFunc(ctx)),
                registered: false,
            }),
        }
    }

    /// Creates an async [host function](crate::Function) with the given function type.
    ///
    /// # Arguments
    ///
    /// * `ty` - The types of the arguments and returns of the target function.
    ///
    /// * `real_fn` - The pointer to the target function.
    ///
    /// * `cost` - The function cost in the [Statistics](crate::Statistics). Pass 0 if the calculation is not needed.
    ///
    /// # Error
    ///
    /// * If fail to create a [Function], then [WasmEdgeError::Func(FuncError::Create)](crate::error::FuncError) is returned.
    ///
    /// # Example
    ///
    /// The example defines an async host function `real_add`.
    ///
    /// ```rust
    /// use wasmedge_sys::{FuncType, Function, WasmValue, CallingFrame};
    /// use wasmedge_types::{error::HostFuncError, ValType, WasmEdgeResult};
    /// use std::future::Future;
    /// use std::os::raw::c_void;
    ///
    /// fn real_add(
    ///     _frame: CallingFrame,
    ///     input: Vec<WasmValue>,
    /// ) -> Box<(dyn Future<Output = Result<Vec<WasmValue>, HostFuncError>> + Send + 'static)> {
    ///     Box::new(async move {
    ///         if input.len() != 3 {
    ///             return Err(HostFuncError::User(1));
    ///         }
    ///
    ///         let a = if input[1].ty() == ValType::I32 {
    ///             input[1].to_i32()
    ///         } else {
    ///             1
    ///         };
    ///
    ///         let b = if input[2].ty() == ValType::I32 {
    ///             input[2].to_i32()
    ///         } else {
    ///             2
    ///         };
    ///         tokio::time::sleep(std::time::Duration::from_secs(4)).await;
    ///
    ///         let c = a + b;
    ///         Ok(vec![WasmValue::from_i32(c)])
    ///     })
    /// }
    ///
    /// // create a FuncType
    /// let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]).expect("fail to create a FuncType");
    ///
    /// // create a Function instance
    /// let func = Function::create_async(&func_ty, Box::new(real_add), 0).expect("fail to create a Function instance");
    /// ```
    #[cfg(feature = "async")]
    pub fn create_async(
        ty: &FuncType,
        real_fn: impl Fn(
                CallingFrame,
                Vec<WasmValue>,
            ) -> Box<
                dyn std::future::Future<
                        Output = Result<Vec<WasmValue>, crate::error::HostFuncError>,
                    > + Send,
            > + Send
            + Sync
            + 'static,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        Self::create(
            ty,
            Box::new(move |frame, args| {
                let async_state = ASYNC_STATE.read();
                let async_cx = async_state.async_cx().unwrap();
                drop(async_state);
                let mut future = Pin::from(real_fn(frame, args));
                match unsafe { async_cx.block_on(future.as_mut()) } {
                    Ok(Ok(ret)) => Ok(ret),
                    Ok(Err(err)) => Err(err),
                    Err(_err) => Err(HostFuncError::User(0x87)),
                }
            }),
            cost,
        )
    }

    /// Creates a [host function](crate::Function) with the given function type and the custom function wrapper.
    ///
    /// # Arguments
    ///
    /// * `ty` - The types of the arguments and returns of the target function.
    ///
    /// * `fn_wrapper` - The custom function wrapper.
    ///
    /// * `real_fn` - The pointer to the target function.
    ///
    /// * `data` - The pointer to the data.
    ///
    /// * `cost` - The function cost in the [Statistics](crate::Statistics). Pass 0 if the calculation is not needed.
    ///
    /// # Error
    ///
    /// * If fail to create a [Function], then [WasmEdgeError::Func(FuncError::Create)](crate::error::FuncError) is returned.
    ///
    pub unsafe fn create_with_custom_wrapper(
        ty: &FuncType,
        fn_wrapper: CustomFnWrapper,
        real_fn: *mut c_void,
        data: *mut c_void,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        let ctx = ffi::WasmEdge_FunctionInstanceCreateBinding(
            ty.inner.0,
            Some(fn_wrapper),
            real_fn,
            data,
            cost,
        );

        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Func(FuncError::Create))),
            false => Ok(Self {
                inner: Arc::new(InnerFunc(ctx)),
                registered: false,
            }),
        }
    }

    /// Creates a [host function](crate::Function) with the given function type and the default function wrapper.
    ///
    /// # Arguments
    ///
    /// * `ty` - The types of the arguments and returns of the target function.
    ///
    /// * `real_fn` - The pointer to the target function.
    ///
    /// * `data` - The pointer to the data.
    ///
    /// * `cost` - The function cost in the [Statistics](crate::Statistics). Pass 0 if the calculation is not needed.
    ///
    /// # Error
    ///
    /// * If fail to create a [Function], then [WasmEdgeError::Func(FuncError::Create)](crate::error::FuncError) is returned.
    ///
    pub unsafe fn create_with_default_wrapper(
        ty: &FuncType,
        real_fn: *mut c_void,
        data: *mut c_void,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        let ctx = ffi::WasmEdge_FunctionInstanceCreateBinding(
            ty.inner.0,
            Some(wrap_fn),
            real_fn,
            data,
            cost,
        );

        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Func(FuncError::Create))),
            false => Ok(Self {
                inner: Arc::new(InnerFunc(ctx)),
                registered: false,
            }),
        }
    }

    /// Returns the underlying wasm type of this [Function].
    ///
    /// # Errors
    ///
    /// If fail to get the function type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let ty = unsafe { ffi::WasmEdge_FunctionInstanceGetFunctionType(self.inner.0) };
        match ty.is_null() {
            true => Err(Box::new(WasmEdgeError::Func(FuncError::Type))),
            false => Ok(FuncType {
                inner: InnerFuncType(ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Runs this host function and returns the result.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implementing the [Engine](crate::Engine) trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    /// # Example
    ///
    /// ```rust
    /// use wasmedge_sys::{FuncType, Function, WasmValue, Executor, CallingFrame};
    /// use wasmedge_types::{error::HostFuncError, ValType};
    ///
    /// fn real_add(_: CallingFrame, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    ///     println!("Rust: Entering Rust function real_add");
    ///
    ///     if input.len() != 2 {
    ///         return Err(HostFuncError::User(1));
    ///     }
    ///
    ///     let a = if input[0].ty() == ValType::I32 {
    ///         input[0].to_i32()
    ///     } else {
    ///         return Err(HostFuncError::User(2));
    ///     };
    ///
    ///     let b = if input[1].ty() == ValType::I32 {
    ///         input[1].to_i32()
    ///     } else {
    ///         return Err(HostFuncError::User(3));
    ///     };
    ///
    ///     let c = a + b;
    ///     println!("Rust: calcuating in real_add c: {:?}", c);
    ///
    ///     println!("Rust: Leaving Rust function real_add");
    ///     Ok(vec![WasmValue::from_i32(c)])
    /// }
    ///
    /// // create a FuncType
    /// let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
    /// assert!(result.is_ok());
    /// let func_ty = result.unwrap();
    /// // create a host function
    /// let result = Function::create(&func_ty, Box::new(real_add), 0);
    /// assert!(result.is_ok());
    /// let host_func = result.unwrap();
    ///
    /// // create an Executor instance
    /// let result = Executor::create(None, None);
    /// assert!(result.is_ok());
    /// let mut executor = result.unwrap();
    ///
    /// // run this function
    /// let result = host_func.call(
    ///     &mut executor,
    ///     vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
    /// );
    /// assert!(result.is_ok());
    /// let returns = result.unwrap();
    /// assert_eq!(returns[0].to_i32(), 3);
    /// ```
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func(self, args)
    }

    /// Runs this host function asynchronously and returns the result.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implementing the [Engine](crate::Engine) trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    #[cfg(feature = "async")]
    pub async fn call_async<E: Engine + Send + Sync>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue> + Send,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        FiberFuture::on_fiber(|| engine.run_func(self, args))
            .await
            .unwrap()
    }

    /// Returns a reference to this [Function] instance.
    pub fn as_ref(&self) -> FuncRef {
        FuncRef {
            inner: InnerFuncRef(self.inner.0 as *const _),
        }
    }

    /// Provides a raw pointer to the inner function context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_FunctionInstanceContext {
        self.inner.0 as *const _
    }
}
impl Drop for Function {
    fn drop(&mut self) {
        if !self.registered && Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_FunctionInstanceDelete(self.inner.0) };
        }
    }
}
impl Clone for Function {
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
            registered: false,
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerFunc(pub(crate) *mut ffi::WasmEdge_FunctionInstanceContext);
unsafe impl Send for InnerFunc {}
unsafe impl Sync for InnerFunc {}

/// Defines the type of a [host function](crate::Function).
///
/// A WasmEdge [FuncType] classifies the signature of a [Function], including the type information of both the arguments and the returns.
#[derive(Debug)]
pub struct FuncType {
    pub(crate) inner: InnerFuncType,
    pub(crate) registered: bool,
}
impl FuncType {
    /// Create a new [FuncType] to be associated with the given arguments and returns.
    ///
    /// # Arguments
    ///
    /// * `args` - The argument types of a [Function].
    ///
    /// * `returns` - The types of the returns of a [Function].
    ///
    /// # Error
    ///
    /// If fail to create a [FuncType], then an error is returned.
    ///
    /// # Example
    ///
    /// ```rust
    /// use wasmedge_sys::FuncType;
    /// use wasmedge_types::ValType;
    ///
    /// let func_ty = FuncType::create(vec![ValType::I32;2], vec![ValType::I32]).expect("fail to create a FuncType");
    /// ```
    pub fn create<I: IntoIterator<Item = ValType>, R: IntoIterator<Item = ValType>>(
        args: I,
        returns: R,
    ) -> WasmEdgeResult<Self> {
        let param_tys = args
            .into_iter()
            .map(|x| x.into())
            .collect::<Vec<ffi::WasmEdge_ValType>>();
        let ret_tys = returns
            .into_iter()
            .map(|x| x.into())
            .collect::<Vec<ffi::WasmEdge_ValType>>();

        let ctx = unsafe {
            ffi::WasmEdge_FunctionTypeCreate(
                param_tys.as_ptr() as *const _,
                param_tys.len() as u32,
                ret_tys.as_ptr() as *const _,
                ret_tys.len() as u32,
            )
        };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::FuncTypeCreate)),
            false => Ok(Self {
                inner: InnerFuncType(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the number of the arguments of a [Function].
    pub fn params_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_FunctionTypeGetParametersLength(self.inner.0) }
    }

    /// Returns an Iterator of the arguments of a [Function].
    pub fn params_type_iter(&self) -> impl Iterator<Item = ValType> {
        let len = self.params_len();
        let mut types = Vec::with_capacity(len as usize);
        unsafe {
            ffi::WasmEdge_FunctionTypeGetParameters(self.inner.0, types.as_mut_ptr(), len);
            types.set_len(len as usize);
        }

        types.into_iter().map(Into::into)
    }

    ///Returns the number of the returns of a [Function].
    pub fn returns_len(&self) -> u32 {
        unsafe { ffi::WasmEdge_FunctionTypeGetReturnsLength(self.inner.0) }
    }

    /// Returns an Iterator of the return types of a [Function].
    pub fn returns_type_iter(&self) -> impl Iterator<Item = ValType> {
        let len = self.returns_len();
        let mut types = Vec::with_capacity(len as usize);
        unsafe {
            ffi::WasmEdge_FunctionTypeGetReturns(self.inner.0, types.as_mut_ptr(), len);
            types.set_len(len as usize);
        }

        types.into_iter().map(Into::into)
    }

    /// Provides a raw pointer to the inner function type context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_FunctionTypeContext {
        self.inner.0 as *const _
    }
}
impl Drop for FuncType {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_FunctionTypeDelete(self.inner.0) };
        }
    }
}
impl From<wasmedge_types::FuncType> for FuncType {
    fn from(ty: wasmedge_types::FuncType) -> Self {
        let param_tys: Vec<_> = match ty.args() {
            Some(args) => args.to_vec(),
            None => Vec::new(),
        };
        let ret_tys: Vec<_> = match ty.returns() {
            Some(returns) => returns.to_vec(),
            None => Vec::new(),
        };

        FuncType::create(param_tys, ret_tys).expect("[wasmedge-sys] Failed to convert wasmedge_types::FuncType into wasmedge_sys::FuncType.")
    }
}
impl From<FuncType> for wasmedge_types::FuncType {
    fn from(ty: FuncType) -> Self {
        let args = if ty.params_len() > 0 {
            let mut args = Vec::with_capacity(ty.params_len() as usize);
            for ty in ty.params_type_iter() {
                args.push(ty);
            }
            Some(args)
        } else {
            None
        };

        let returns = if ty.returns_len() > 0 {
            let mut returns = Vec::with_capacity(ty.returns_len() as usize);
            for ty in ty.returns_type_iter() {
                returns.push(ty);
            }
            Some(returns)
        } else {
            None
        };

        wasmedge_types::FuncType::new(args, returns)
    }
}

#[derive(Debug)]
pub(crate) struct InnerFuncType(pub(crate) *mut ffi::WasmEdge_FunctionTypeContext);
unsafe impl Send for InnerFuncType {}
unsafe impl Sync for InnerFuncType {}

/// Defines a reference to a [host function](crate::Function).
#[derive(Debug, Clone)]
pub struct FuncRef {
    pub(crate) inner: InnerFuncRef,
}
impl Drop for FuncRef {
    fn drop(&mut self) {
        self.inner.0 = std::ptr::null();
    }
}
impl FuncRef {
    /// Returns the underlying wasm type of the host function this [FuncRef] points to.
    ///
    /// # Errors
    ///
    /// If fail to get the function type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let ty = unsafe { ffi::WasmEdge_FunctionInstanceGetFunctionType(self.inner.0 as *mut _) };
        match ty.is_null() {
            true => Err(Box::new(WasmEdgeError::Func(FuncError::Type))),
            false => Ok(FuncType {
                inner: InnerFuncType(ty as *mut _),
                registered: true,
            }),
        }
    }

    /// Runs this host function the reference refers to.
    ///
    /// # Arguments
    ///
    /// * `engine` - The object implementing the [Engine](crate::Engine) trait.
    ///
    /// * `args` - The arguments passed to the host function.
    ///
    /// # Error
    ///
    /// If fail to run the host function, then an error is returned.
    ///
    pub fn call<E: Engine>(
        &self,
        engine: &mut E,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        engine.run_func_ref(self, args)
    }
}

#[derive(Debug, Clone)]
pub(crate) struct InnerFuncRef(pub(crate) *const ffi::WasmEdge_FunctionInstanceContext);
unsafe impl Send for InnerFuncRef {}
unsafe impl Sync for InnerFuncRef {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{types::WasmValue, Executor};
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_macro::sys_host_function;
    use wasmedge_types::ValType;

    #[test]
    fn test_func_type() {
        // test FuncType with args and returns
        {
            let param_tys = vec![
                ValType::I32,
                ValType::I64,
                ValType::F32,
                ValType::F64,
                ValType::V128,
                ValType::ExternRef,
            ];
            let param_len = param_tys.len();
            let ret_tys = vec![ValType::FuncRef, ValType::ExternRef, ValType::V128];
            let ret_len = ret_tys.len();

            // create FuncType
            let result = FuncType::create(param_tys, ret_tys);
            assert!(result.is_ok());
            let func_ty = result.unwrap();

            // check parameters
            assert_eq!(func_ty.params_len(), param_len as u32);
            let param_tys = func_ty.params_type_iter().collect::<Vec<_>>();
            assert_eq!(
                param_tys,
                vec![
                    ValType::I32,
                    ValType::I64,
                    ValType::F32,
                    ValType::F64,
                    ValType::V128,
                    ValType::ExternRef,
                ]
            );

            // check returns
            assert_eq!(func_ty.returns_len(), ret_len as u32);
            let return_tys = func_ty.returns_type_iter().collect::<Vec<_>>();
            assert_eq!(
                return_tys,
                vec![ValType::FuncRef, ValType::ExternRef, ValType::V128]
            );
        }

        // test FuncType without args and returns
        {
            // create FuncType
            let result = FuncType::create([], []);
            assert!(result.is_ok());
            let func_ty = result.unwrap();

            assert_eq!(func_ty.params_len(), 0);
            assert_eq!(func_ty.returns_len(), 0);
        }
    }

    #[test]
    fn test_func_basic() {
        #[sys_host_function]
        fn real_add(
            _frame: CallingFrame,
            input: Vec<WasmValue>,
        ) -> Result<Vec<WasmValue>, HostFuncError> {
            println!("Rust: Entering Rust function real_add");

            if input.len() != 2 {
                return Err(HostFuncError::User(1));
            }

            let a = if input[0].ty() == ValType::I32 {
                input[0].to_i32()
            } else {
                return Err(HostFuncError::User(2));
            };

            let b = if input[1].ty() == ValType::I32 {
                input[1].to_i32()
            } else {
                return Err(HostFuncError::User(3));
            };

            let c = a + b;
            println!("Rust: calcuating in real_add c: {c:?}");

            println!("Rust: Leaving Rust function real_add");
            Ok(vec![WasmValue::from_i32(c)])
        }

        // create a FuncType
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();

        // get func type
        let result = host_func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check parameters
        assert_eq!(ty.params_len(), 2);
        let param_tys = ty.params_type_iter().collect::<Vec<_>>();
        assert_eq!(param_tys, vec![ValType::I32; 2]);

        // check returns
        assert_eq!(ty.returns_len(), 1);
        let return_tys = ty.returns_type_iter().collect::<Vec<_>>();
        assert_eq!(return_tys, vec![ValType::I32]);

        // run this function
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        let result = host_func.call(
            &mut executor,
            vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 3);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_func_create_host_func_in_host_func() {
        #[sys_host_function]
        fn func(
            _frame: CallingFrame,
            _input: Vec<WasmValue>,
        ) -> Result<Vec<WasmValue>, HostFuncError> {
            println!("Entering host function: func");

            // spawn a new thread to create a new host function
            let handler = std::thread::spawn(|| {
                #[sys_host_function]
                fn real_add(
                    _frame: CallingFrame,
                    input: Vec<WasmValue>,
                ) -> Result<Vec<WasmValue>, HostFuncError> {
                    println!("Rust: Entering Rust function real_add");

                    if input.len() != 2 {
                        return Err(HostFuncError::User(1));
                    }

                    let a = if input[0].ty() == ValType::I32 {
                        input[0].to_i32()
                    } else {
                        return Err(HostFuncError::User(2));
                    };

                    let b = if input[1].ty() == ValType::I32 {
                        input[1].to_i32()
                    } else {
                        return Err(HostFuncError::User(3));
                    };

                    let c = a + b;

                    println!("Rust: Leaving Rust function real_add");
                    Ok(vec![WasmValue::from_i32(c)])
                }

                // create a FuncType
                let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
                assert!(result.is_ok());
                let func_ty = result.unwrap();
                // create a host function
                let result = Function::create(&func_ty, Box::new(real_add), 0);
                assert!(result.is_ok());
                let host_func = result.unwrap();

                // run this function
                let result = Executor::create(None, None);
                assert!(result.is_ok());
                let mut executor = result.unwrap();
                let result = host_func.call(
                    &mut executor,
                    vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
                );
                assert!(result.is_ok());
                let returns = result.unwrap();
                assert_eq!(returns[0].to_i32(), 3);
            });
            handler.join().unwrap();

            println!("Leaving host function: func");
            Ok(vec![])
        }

        // create a FuncType
        let result = FuncType::create(vec![], vec![]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let result = Function::create(&func_ty, Box::new(func), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();

        // run this function
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        let result = host_func.call(&mut executor, vec![]);
        assert!(result.is_ok());
    }

    #[test]
    fn test_func_send() {
        // create a FuncType
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();

        let handle = thread::spawn(move || {
            // get func type
            let result = host_func.ty();
            assert!(result.is_ok());
            let ty = result.unwrap();

            // check parameters
            assert_eq!(ty.params_len(), 2);
            let param_tys = ty.params_type_iter().collect::<Vec<_>>();
            assert_eq!(param_tys, vec![ValType::I32; 2]);

            // check returns
            assert_eq!(ty.returns_len(), 1);
            let return_tys = ty.returns_type_iter().collect::<Vec<_>>();
            assert_eq!(return_tys, vec![ValType::I32]);
        });

        handle.join().unwrap()
    }

    #[test]
    fn test_func_sync() {
        // create a FuncType
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = Arc::new(Mutex::new(result.unwrap()));

        let host_func_cloned = Arc::clone(&host_func);
        let handle = thread::spawn(move || {
            let result = host_func_cloned.lock();
            assert!(result.is_ok());
            let host_func = result.unwrap();

            // get func type
            let result = host_func.ty();
            assert!(result.is_ok());
            let ty = result.unwrap();

            // check parameters
            assert_eq!(ty.params_len(), 2);
            let param_tys = ty.params_type_iter().collect::<Vec<_>>();
            assert_eq!(param_tys, vec![ValType::I32; 2]);

            // check returns
            assert_eq!(ty.returns_len(), 1);
            let return_tys = ty.returns_type_iter().collect::<Vec<_>>();
            assert_eq!(return_tys, vec![ValType::I32]);
        });

        handle.join().unwrap();
    }

    fn real_add(
        _frame: CallingFrame,
        input: Vec<WasmValue>,
    ) -> Result<Vec<WasmValue>, HostFuncError> {
        println!("Rust: Entering Rust function real_add");

        if input.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if input[0].ty() == ValType::I32 {
            input[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if input[1].ty() == ValType::I32 {
            input[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;
        println!("Rust: calcuating in real_add c: {c:?}");

        println!("Rust: Leaving Rust function real_add");
        Ok(vec![WasmValue::from_i32(c)])
    }

    #[test]
    fn test_func_closure() {
        // create a FuncType
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let real_add =
            |_: CallingFrame, input: Vec<WasmValue>| -> Result<Vec<WasmValue>, HostFuncError> {
                println!("Rust: Entering Rust function real_add");

                if input.len() != 2 {
                    return Err(HostFuncError::User(1));
                }

                let a = if input[0].ty() == ValType::I32 {
                    input[0].to_i32()
                } else {
                    return Err(HostFuncError::User(2));
                };

                let b = if input[1].ty() == ValType::I32 {
                    input[1].to_i32()
                } else {
                    return Err(HostFuncError::User(3));
                };

                let c = a + b;
                println!("Rust: calcuating in real_add c: {c:?}");

                println!("Rust: Leaving Rust function real_add");
                Ok(vec![WasmValue::from_i32(c)])
            };

        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();

        // get func type
        let result = host_func.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();

        // check parameters
        assert_eq!(ty.params_len(), 2);
        let param_tys = ty.params_type_iter().collect::<Vec<_>>();
        assert_eq!(param_tys, vec![ValType::I32; 2]);

        // check returns
        assert_eq!(ty.returns_len(), 1);
        let return_tys = ty.returns_type_iter().collect::<Vec<_>>();
        assert_eq!(return_tys, vec![ValType::I32]);

        // run this function
        let result = Executor::create(None, None);
        assert!(result.is_ok());
        let mut executor = result.unwrap();
        let result = host_func.call(
            &mut executor,
            vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
        );
        assert!(result.is_ok());
        let returns = result.unwrap();
        assert_eq!(returns[0].to_i32(), 3);
    }
}
