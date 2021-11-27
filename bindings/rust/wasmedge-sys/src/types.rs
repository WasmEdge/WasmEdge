use super::wasmedge;
use crate::raw_result::WasmEdgeResult;
use std::ffi::{CStr, CString};

// pub type WasmEdgeValue = wasmedge::WasmEdge_Value;
pub type WasmEdgeProposal = wasmedge::WasmEdge_Proposal;
pub type HostRegistration = wasmedge::WasmEdge_HostRegistration;
pub type CompilerOptimizationLevel = wasmedge::WasmEdge_CompilerOptimizationLevel;
pub type HostFunc = wasmedge::WasmEdge_HostFunc_t;
pub type WrapFunc = wasmedge::WasmEdge_WrapFunc_t;

#[derive(Debug)]
pub struct WasmEdgeString {
    pub(crate) ctx: wasmedge::WasmEdge_String,
}
impl WasmEdgeString {
    pub fn from_str(s: &str) -> WasmEdgeResult<WasmEdgeString> {
        let cstring = CString::new(s)?;
        Ok(WasmEdgeString {
            ctx: unsafe { wasmedge::WasmEdge_StringCreateByCString(cstring.as_ptr()) },
        })
    }

    pub fn from_buffer(buf: &[i8]) -> WasmEdgeString {
        WasmEdgeString {
            ctx: unsafe { wasmedge::WasmEdge_StringCreateByBuffer(buf.as_ptr(), buf.len() as u32) },
        }
    }

    pub fn to_string_lossy(&self) -> std::borrow::Cow<'_, str> {
        let cstr = unsafe { CStr::from_ptr(self.ctx.Buf) };
        cstr.to_string_lossy()
    }

    pub fn into_raw(&self) -> wasmedge::WasmEdge_String {
        self.ctx
    }
}
impl From<&str> for WasmEdgeString {
    fn from(s: &str) -> Self {
        let cstring = CString::new(s).expect("fail to create CString from str");
        WasmEdgeString {
            ctx: unsafe { wasmedge::WasmEdge_StringCreateByCString(cstring.as_ptr()) },
        }
    }
}
impl Drop for WasmEdgeString {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_StringDelete(self.ctx) }
    }
}

#[derive(Debug)]
pub enum WasmEdgeRefType {
    FuncRef,
    ExternRef,
}
impl From<WasmEdgeRefType> for wasmedge::WasmEdge_RefType {
    fn from(ty: WasmEdgeRefType) -> Self {
        match ty {
            WasmEdgeRefType::FuncRef => wasmedge::WasmEdge_RefType_FuncRef,
            WasmEdgeRefType::ExternRef => wasmedge::WasmEdge_RefType_ExternRef,
        }
    }
}

#[derive(Debug)]
pub struct Limit {
    pub(crate) min: u32,
    pub(crate) max: Option<u32>,
}
impl Limit {
    pub fn create(min: u32, max: Option<u32>) -> Self {
        Self { min, max }
    }
}
impl From<Limit> for wasmedge::WasmEdge_Limit {
    fn from(limit: Limit) -> Self {
        match limit.max {
            None => Self {
                Min: limit.min,
                HasMax: false,
                Max: u32::MAX,
            },
            Some(max) => Self {
                Min: limit.min,
                HasMax: true,
                Max: max,
            },
        }
    }
}
impl From<wasmedge::WasmEdge_Limit> for Limit {
    fn from(limit: wasmedge::WasmEdge_Limit) -> Self {
        let max = match limit.HasMax {
            true => Some(limit.Max),
            false => None,
        };
        Self {
            min: limit.Min,
            max,
        }
    }
}

#[derive(Debug)]
pub enum ValType {
    I32,
    I64,
    F32,
    F64,
    FuncRef,
    ExternRef,
}
impl From<ValType> for wasmedge::WasmEdge_ValType {
    fn from(ty: ValType) -> Self {
        match ty {
            ValType::I32 => wasmedge::WasmEdge_ValType_I32,
            ValType::I64 => wasmedge::WasmEdge_ValType_I64,
            ValType::F32 => wasmedge::WasmEdge_ValType_F32,
            ValType::F64 => wasmedge::WasmEdge_ValType_F64,
            ValType::FuncRef => wasmedge::WasmEdge_ValType_FuncRef,
            ValType::ExternRef => wasmedge::WasmEdge_ValType_ExternRef,
        }
    }
}
impl From<wasmedge::WasmEdge_ValType> for ValType {
    fn from(ty: wasmedge::WasmEdge_ValType) -> Self {
        match ty {
            wasmedge::WasmEdge_ValType_I32 => ValType::I32,
            wasmedge::WasmEdge_ValType_I64 => ValType::I64,
            wasmedge::WasmEdge_ValType_F32 => ValType::F32,
            wasmedge::WasmEdge_ValType_F64 => ValType::F64,
            wasmedge::WasmEdge_ValType_FuncRef => ValType::FuncRef,
            wasmedge::WasmEdge_ValType_ExternRef => ValType::ExternRef,
            _ => panic!("unknown WasmEdge_ValType `{}`", ty),
        }
    }
}

#[derive(Debug)]
pub enum Mutability {
    Const,
    Var,
}
impl From<Mutability> for wasmedge::WasmEdge_Mutability {
    fn from(mutable: Mutability) -> Self {
        match mutable {
            Mutability::Const => wasmedge::WasmEdge_Mutability_Const,
            Mutability::Var => wasmedge::WasmEdge_Mutability_Var,
        }
    }
}
impl From<wasmedge::WasmEdge_Mutability> for Mutability {
    fn from(mutable: wasmedge::WasmEdge_Mutability) -> Self {
        match mutable {
            wasmedge::WasmEdge_Mutability_Const => Mutability::Const,
            wasmedge::WasmEdge_Mutability_Var => Mutability::Var,
            _ => panic!("unknown Mutability value `{}`", mutable),
        }
    }
}
