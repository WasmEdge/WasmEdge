use crate::{wasmedge, Value};

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
    pub fn create<I, O>(_f: Box<dyn std::ops::FnOnce(I) -> O>) -> Self
    where
        I: WasmFnIO,
        O: WasmFnIO,
    {
        let t = Type::create(I::parameters(), O::parameters());
        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionInstanceCreate(t.ctx, None, std::ptr::null_mut(), 0)
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
