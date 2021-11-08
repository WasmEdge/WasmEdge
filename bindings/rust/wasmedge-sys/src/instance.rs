use super::wasmedge;

use crate::hostfunc::FuncType;

pub struct Function {
    ctx: *mut wasmedge::WasmEdge_FunctionInstanceContext,
    index: usize,
}

impl Function {
    pub fn get_func_type(&self) -> FuncType {
        let ctx = unsafe {
            wasmedge::WasmEdge_FunctionInstanceGetFunctionType(self.ctx)
                as *mut wasmedge::WasmEdge_FunctionTypeContext
        };

        FuncType { ctx }
    }
}

pub struct Table {
    ctx: *mut wasmedge::WasmEdge_TableInstanceContext,
    index: usize,
}

pub struct Memory {
    ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    index: usize,
}

pub struct Global {
    ctx: *mut wasmedge::WasmEdge_GlobalInstanceContext,
    index: usize,
}

impl Drop for Function {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_FunctionInstanceDelete(self.ctx) };
    }
}

impl Drop for Table {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_TableInstanceDelete(self.ctx) };
    }
}

impl Drop for Memory {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_MemoryInstanceDelete(self.ctx) };
    }
}

impl Drop for Global {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_GlobalInstanceDelete(self.ctx) };
    }
}
