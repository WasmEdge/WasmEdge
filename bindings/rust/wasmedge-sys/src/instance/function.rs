use crate::{wasmedge, Value, WasmFnIO, HOST_FUNCS};

use core::ffi::c_void;
use std::convert::TryInto;

use rand::Rng;

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
    let mut result: Result<Vec<Value>, u8> = Err(0);

    let mut input: Vec<Value> = {
        let raw_input = unsafe {
            std::slice::from_raw_parts(
                params,
                param_len
                    .try_into()
                    .expect("len of params should not greater than usize"),
            )
        };
        raw_input.iter().map(|r| (*r).into()).collect()
    };
    input.remove(0);

    let return_len = return_len
        .try_into()
        .expect("len of returns should not greater than usize");
    let raw_returns = unsafe { std::slice::from_raw_parts_mut(returns, return_len) };

    HOST_FUNCS.with(|f| {
        let mut host_functions = f.borrow_mut();
        let real_fn = host_functions
            .remove(&key)
            .expect("host function should there");
        result = real_fn(input);
    });

    match result {
        Ok(v) => {
            assert!(v.len() == return_len);
            for (idx, item) in v.into_iter().enumerate() {
                raw_returns[idx] = item.into();
            }
            wasmedge::WasmEdge_Result { Code: 0 }
        }
        Err(c) => wasmedge::WasmEdge_Result { Code: c as u8 },
    }
}

#[derive(Debug)]
pub struct Function {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionInstanceContext,
    pub(crate) registed: bool,
    ty: Option<Type>,
}

impl Function {
    /// wasmedge::WasmEdge_FunctionInstanceCreate take C functions
    /// This may not be implement
    pub fn create<I: WasmFnIO, O: WasmFnIO>(
        _f: Box<dyn std::ops::Fn(Vec<Value>) -> Vec<Value>>,
    ) -> Self {
        unimplemented!()
    }

    // TODO:
    // binding errors and restict the error types
    #[allow(clippy::type_complexity)]
    /// Binding Rust function (HostFunction) to WasmEdgeFunction
    pub fn create_bindings<I: WasmFnIO, O: WasmFnIO>(
        real_fn: Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>,
    ) -> Self {
        let mut key = 0usize;
        HOST_FUNCS.with(|f| {
            let mut rng = rand::thread_rng();
            let mut host_functions = f.borrow_mut();
            assert!(
                host_functions.len() <= host_functions.capacity(),
                "Too many host functions"
            );
            key = rng.gen::<usize>();
            while host_functions.contains_key(&key) {
                key = rng.gen::<usize>();
            }
            host_functions.insert(key, real_fn);
        });

        let key_ptr = key as *const usize as *mut c_void;
        let ty = Type::create(I::parameters(), O::parameters());

        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionInstanceCreateBinding(
                ty.ctx,
                Some(wraper_fn),
                key_ptr,
                std::ptr::null_mut(),
                0,
            )
        };
        Self {
            ctx,
            ty: Some(ty),
            registed: false,
        }
    }
}

impl Drop for Function {
    fn drop(&mut self) {
        self.ty = None;
        if !self.registed {
            unsafe { wasmedge::WasmEdge_FunctionInstanceDelete(self.ctx) };
        }
    }
}

#[derive(Debug)]
pub struct Type {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionTypeContext,
}

impl Type {
    pub(crate) fn create(input: Vec<Value>, output: Vec<Value>) -> Self {
        let raw_input = {
            let mut head = vec![wasmedge::WasmEdge_ValType_ExternRef];
            let mut tail = input
                .iter()
                .map(|v| wasmedge::WasmEdge_Value::from(*v).Type)
                .collect::<Vec<wasmedge::WasmEdge_ValType>>();
            head.append(&mut tail);
            head
        };
        let raw_output = output
            .iter()
            .map(|v| wasmedge::WasmEdge_Value::from(*v).Type)
            .collect::<Vec<wasmedge::WasmEdge_ValType>>();

        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionTypeCreate(
                raw_input.as_ptr() as *const u32,
                raw_input.len() as u32,
                raw_output.as_ptr() as *const u32,
                raw_output.len() as u32,
            )
        };
        Self { ctx }
    }
}

impl Drop for Type {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_FunctionTypeDelete(self.ctx) };
    }
}
