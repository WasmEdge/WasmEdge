//! Defines WasmEdge Function and FuncType structs.

use crate::{
    types::Value, wasmedge, FuncError, ValType, WasmEdgeError, WasmEdgeResult, HOST_FUNCS,
};
use core::ffi::c_void;
use rand::Rng;
use std::convert::TryInto;

extern "C" fn wraper_fn(
    key_ptr: *mut c_void,
    _data: *mut c_void,
    _mem_cxt: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    params: *const wasmedge::WasmEdge_Value,
    param_len: u32,
    returns: *mut wasmedge::WasmEdge_Value,
    return_len: u32,
) -> wasmedge::WasmEdge_Result {
    let key = key_ptr as *const usize as usize;
    let mut result = Err(0);

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

    HOST_FUNCS.with(|f| {
        let host_functions = f.borrow();
        let real_fn = host_functions
            .get(&key)
            .expect("host function should be there");
        result = real_fn(input);
    });

    match result {
        Ok(v) => {
            assert!(v.len() == return_len);
            for (idx, item) in v.into_iter().enumerate() {
                raw_returns[idx] = item.as_raw();
            }
            wasmedge::WasmEdge_Result { Code: 0 }
        }
        Err(c) => wasmedge::WasmEdge_Result { Code: c as u8 },
    }
}

/// Struct of WasmEdge Function.
///
/// A WasmEdge [`Function`] defines a host function described by its [`FuncType`]. A host function is a
/// function defined outside WebAssembly and passed to WASM module.
///
/// In WasmEdge, developers can create the [`Function`]s and other WasmEdge instances, such as [Memory](crate::Memory),
/// and add them into a WasmEdge [ImportObject](crate::ImportObject) for registering into a WasmEdge [Vm](crate::Vm) or
/// [Store](crate::Store).
#[derive(Debug)]
pub struct Function {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionInstanceContext,
    pub(crate) registered: bool,
}
impl Function {
    #[allow(clippy::type_complexity)]
    /// Creates a [`Function`] for host functions.
    ///
    /// # Arguments
    ///
    /// - `ty` specifies the types of the arguments and returns of the target function.
    ///
    /// - `real_fn` specifies the pointer to the target function.
    ///
    /// - `cost` specifies the function cost in the [Statistics](crate::Statistics).
    ///
    /// # Error
    ///
    /// If fail to create a [`Function`], then an error is returned.
    ///
    /// # Example
    ///
    /// The example defines a host function `real_add`, and creates a [`Function`] binding to it by calling
    /// the `create_binding` method.
    ///
    /// ```rust
    /// use wasmedge_sys::{FuncType, Function, ValType, Value, WasmEdgeResult};
    ///
    /// fn real_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    ///     if inputs.len() != 2 {
    ///         return Err(1);
    ///     }
    ///
    ///     let a = if inputs[0].ty() == ValType::I32 {
    ///         inputs[0].to_i32()
    ///     } else {
    ///         return Err(2);
    ///     };
    ///
    ///     let b = if inputs[1].ty() == ValType::I32 {
    ///         inputs[1].to_i32()
    ///     } else {
    ///         return Err(3);
    ///     };
    ///
    ///     let c = a + b;
    ///
    ///     Ok(vec![Value::from_i32(c)])
    /// }
    ///
    /// // create a FuncType
    /// let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]).expect("fail to create a FuncType");
    ///
    /// // create a Function instance
    /// let func = Function::create(func_ty, Box::new(real_add), 0).expect("fail to create a Function instance");
    /// ```
    pub fn create(
        mut ty: FuncType,
        real_fn: Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>,
        cost: u64,
    ) -> WasmEdgeResult<Self> {
        let mut key = 0usize;

        HOST_FUNCS.with(|f| {
            let mut host_functions = f.borrow_mut();
            if host_functions.len() >= host_functions.capacity() {
                return Err(WasmEdgeError::Func(FuncError::CreateBinding(format!(
                    "The number of the host functions reaches the upper bound: {}",
                    host_functions.capacity()
                ))));
            }

            // generate key for the coming host function
            let mut rng = rand::thread_rng();
            key = rng.gen::<usize>();
            while host_functions.contains_key(&key) {
                key = rng.gen::<usize>();
            }
            host_functions.insert(key, real_fn);

            Ok(())
        })?;

        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionInstanceCreateBinding(
                ty.ctx,
                Some(wraper_fn),
                key as *const usize as *mut c_void,
                std::ptr::null_mut(),
                cost,
            )
        };
        ty.ctx = std::ptr::null_mut();

        match ctx.is_null() {
            true => Err(WasmEdgeError::Func(FuncError::Create)),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the underlying wasm type of a [`Function`].
    ///
    /// # Errors
    ///
    /// If fail to get the function type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<FuncType> {
        let ty = unsafe { wasmedge::WasmEdge_FunctionInstanceGetFunctionType(self.ctx) };
        match ty.is_null() {
            true => Err(WasmEdgeError::Func(FuncError::Type)),
            false => Ok(FuncType {
                ctx: ty as *mut _,
                registered: true,
            }),
        }
    }
}
impl Drop for Function {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_FunctionInstanceDelete(self.ctx) };
        }
    }
}

/// Struct of WasmEdge FuncType.
///
/// A WasmEdge [`FuncType`] classifies the signature of a [`Function`], including the type information
/// of both the arguments and the returns.
#[derive(Debug, Clone)]
pub struct FuncType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionTypeContext,
    pub(crate) registered: bool,
}
impl FuncType {
    /// Create a new [`FuncType`] to be associated with the given arguments and returns.
    ///
    /// # Arguments
    ///
    /// - `args` specifies the agrgument types of a [`Function`].
    ///
    /// - `returns` specifies the types of the returns of a [`Function`].
    ///
    /// # Error
    ///
    /// If fail to create a [`FuncType`], then a [WasmEdgeError::FuncTypeCreate](crate::error::WasmEdgeError::FuncTypeCreate) error is returned.
    ///
    /// # Example
    ///
    /// ```rust
    /// use wasmedge_sys::{FuncType, ValType};
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
            .collect::<Vec<wasmedge::WasmEdge_ValType>>();
        let ret_tys = returns
            .into_iter()
            .map(|x| x.into())
            .collect::<Vec<wasmedge::WasmEdge_ValType>>();

        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionTypeCreate(
                param_tys.as_ptr() as *const _,
                param_tys.len() as u32,
                ret_tys.as_ptr() as *const u32,
                ret_tys.len() as u32,
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::FuncTypeCreate),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the number of the arguments of a [`Function`].
    pub fn params_len(&self) -> usize {
        unsafe { wasmedge::WasmEdge_FunctionTypeGetParametersLength(self.ctx) as usize }
    }

    /// Returns an Iterator of the arguments of a [`Function`].
    pub fn params_type_iter(&self) -> impl Iterator<Item = ValType> {
        let len = self.params_len();
        let mut types = Vec::with_capacity(len);
        unsafe {
            wasmedge::WasmEdge_FunctionTypeGetParameters(self.ctx, types.as_mut_ptr(), len as u32);
            types.set_len(len);
        }

        types.into_iter().map(Into::into)
    }

    ///Returns the number of the returns of a [`Function`].
    pub fn returns_len(&self) -> usize {
        unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(self.ctx) as usize }
    }

    /// Returns an Iterator of the return types of a [`Function`].
    pub fn returns_type_iter(&self) -> impl Iterator<Item = ValType> {
        let len = self.returns_len();
        let mut types = Vec::with_capacity(len);
        unsafe {
            wasmedge::WasmEdge_FunctionTypeGetReturns(self.ctx, types.as_mut_ptr(), len as u32);
            types.set_len(len);
        }

        types.into_iter().map(Into::into)
    }
}
impl Drop for FuncType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_FunctionTypeDelete(self.ctx) };
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ValType;

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
            assert_eq!(func_ty.params_len(), param_len);
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
            assert_eq!(func_ty.returns_len(), ret_len);
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
    fn test_func() {
        // create a FuncType
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        // create a host function
        let result = Function::create(func_ty, Box::new(real_add), 0);
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
    }

    fn real_add(input: Vec<Value>) -> Result<Vec<Value>, u8> {
        println!("Rust: Entering Rust function real_add");

        if input.len() != 2 {
            return Err(1);
        }

        let a = if input[0].ty() == ValType::I32 {
            input[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if input[1].ty() == ValType::I32 {
            input[0].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;
        println!("Rust: calcuating in real_add c: {:?}", c);

        println!("Rust: Leaving Rust function real_add");
        Ok(vec![Value::from_i32(c)])
    }
}
