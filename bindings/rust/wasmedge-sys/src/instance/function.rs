use crate::{wasmedge, Value};

use core::ffi::c_void;

extern "C" fn wraper_fn(
    _real_fn: *mut c_void,
    _data: *mut c_void,
    _mem_cxt: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    _params: *const wasmedge::WasmEdge_Value,
    _param_len: u32,
    _returns: *mut wasmedge::WasmEdge_Value,
    _return_len: u32,
) -> wasmedge::WasmEdge_Result {
    wasmedge::WasmEdge_Result {
        // TODO: binding error code and replace this
        Code: 0,
    }
}

pub trait WasmFnIO {
    fn parameters() -> Vec<Value> {
        Vec::new()
    }
}

#[derive(Debug)]
pub struct Function {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionInstanceContext,
}

impl Function {
    /// wasmedge::WasmEdge_FunctionInstanceCreate take C functions
    /// This may not be implement
    pub fn create<I, O>(_f: Box<dyn std::ops::FnOnce(I) -> O>) -> Self
    where
        I: WasmFnIO,
        O: WasmFnIO,
    {
        unimplemented!()
    }

    // TODO:
    // binding errors and restict the error types
    /// Binding Rust function (HostFunction) to WasmEdgeFunction
    pub fn create_bindings<I, O>(real_fn: Box<dyn std::ops::Fn(I) -> Result<O, usize>>) -> Self
    where
        I: WasmFnIO,
        O: WasmFnIO,
    {
        let real_fn_ptr =
            &*real_fn as *const dyn std::ops::Fn(I) -> Result<O, usize> as *mut c_void;
        let t = Type::create(I::parameters(), O::parameters());
        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionInstanceCreateBinding(
                t.ctx,
                Some(wraper_fn),
                real_fn_ptr,
                std::ptr::null_mut(),
                0,
            )
        };
        Self { ctx }
    }
}

impl Drop for Function {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_FunctionInstanceDelete(self.ctx) };
    }
}

#[derive(Debug)]
pub struct Type {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionTypeContext,
}

impl Type {
    pub(crate) fn create(input: Vec<Value>, output: Vec<Value>) -> Self {
        let raw_input = input
            .iter()
            .map(|v| wasmedge::WasmEdge_Value::from(*v).Type)
            .collect::<Vec<wasmedge::WasmEdge_ValType>>();
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
