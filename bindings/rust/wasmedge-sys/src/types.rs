use super::wasmedge;
pub type WasmEdgeProposal = wasmedge::WasmEdge_Proposal;
pub type HostRegistration = wasmedge::WasmEdge_HostRegistration;
pub type CompilerOptimizationLevel = wasmedge::WasmEdge_CompilerOptimizationLevel;
pub type HostFunc = wasmedge::WasmEdge_HostFunc_t;
pub type WrapFunc = wasmedge::WasmEdge_WrapFunc_t;

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

impl From<std::ops::Range<u32>> for wasmedge::WasmEdge_Limit {
    fn from(range: std::ops::Range<u32>) -> Self {
        Self {
            Min: range.start,
            Max: range.end,
            HasMax: true,
        }
    }
}
impl From<wasmedge::WasmEdge_Limit> for std::ops::Range<u32> {
    fn from(limit: wasmedge::WasmEdge_Limit) -> Self {
        let start = limit.Min;
        let end = match limit.HasMax {
            true => limit.Max,
            false => u32::MAX,
        };
        Self { start, end }
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
