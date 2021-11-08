use super::wasmedge;

use crate::value::Value;

use once_cell::sync::OnceCell;
use std::{collections::HashMap, os::raw::c_void, sync::Mutex};

pub struct FuncType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_FunctionTypeContext,
}

impl Drop for FuncType {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_FunctionTypeDelete(self.ctx) };
    }
}

pub struct HostFuncManager<'a> {
    idx: usize,
    gc: &'a [usize],
    data: HashMap<usize, *mut c_void>,
    funcs: HashMap<usize, wasmedge::WasmEdge_HostFunc_t>,
}

thread_local! {
    static HOST_FUNC_MANAGER: HostFuncManager<'static> = {
        let hf = HostFuncManager {
            idx: 0,
            gc: &[],
            data: HashMap::new(),
            funcs: HashMap::new(),
        };
        hf
    }
}

impl<'a> HostFuncManager<'a> {
    pub fn add(&mut self, hostfunc: wasmedge::WasmEdge_HostFunc_t, hostdata: *mut c_void) -> usize {
        let gc_len = self.gc.len();
        let realidx = if gc_len > 0 {
            let realidx: usize = self.gc[gc_len - 1];
            self.gc = &self.gc[0..(gc_len - 1)];
            realidx
        } else {
            let realidx = self.idx;
            self.idx += 1;
            realidx
        };
        self.data.insert(realidx, hostdata);
        self.funcs.insert(realidx, hostfunc);
        realidx
    }

    pub fn get_funcs(&self, i: usize) -> Option<&'_ wasmedge::WasmEdge_HostFunc_t> {
        self.funcs.get(&i)
    }

    pub fn get_data(&self, i: usize) -> Option<&'_ *mut c_void> {
        self.data.get(&i)
    }
}

// extern C
pub extern "C" fn hostfunc_invoke_impl(
    this: *mut c_void,
    data: *mut c_void,
    mem_cxt: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    params: *const wasmedge::WasmEdge_Value,
    param_len: u32,
    returns: *mut WasmEdge_Value,
    return_len: u32,
) -> wasmedge::WasmEdge_Result {
    // TODO
}

// pub struct HostFunc{
//     inner: wasmedge::WasmEdge_HostFunc_t,
// }

// pub fn func_type_create(
//     param_list: Value,
//     param_len: u32,
//     return_list: Value,
//     return_len: u32) -> *mut wasmedge::WasmEdge_FunctionTypeContext {

//     let param_list_type = wasmedge::WasmEdge_Value::from(param_list).Type as *const u32;
//     let return_list_type = wasmedge::WasmEdge_Value::from(return_list).Type as *const u32;

//     unsafe{
//         wasmedge::WasmEdge_FunctionTypeCreate(
//             param_list_type,
//             param_len,
//             return_list_type,
//             return_len,
//         )
//     }

// }

// pub fn func_instance_create(
//     func_type: *mut wasmedge::WasmEdge_FunctionTypeContext,
//     hostfunc: wasmedge::WasmEdge_HostFunc_t,
//     data: *mut ::std::os::raw::c_void,
//     cost: u64,
// ) -> *mut wasmedge::WasmEdge_FunctionInstanceContext{

//     unsafe{
//         wasmedge::WasmEdge_FunctionInstanceCreate(
//             func_type,
//             hostfunc,
//             data,
//             cost,
//         )
//     }

// }
