use super::wasmedge;
pub type WasmEdgeProposal = wasmedge::WasmEdge_Proposal;
pub type HostFunc = wasmedge::WasmEdge_HostFunc_t;
pub type WrapFunc = wasmedge::WasmEdge_WrapFunc_t;

#[derive(Debug)]
pub enum WasmEdgeRefType {
    FuncRef,
    ExternRef,
}
impl From<u32> for WasmEdgeRefType {
    fn from(value: u32) -> Self {
        match value {
            0x70u32 => WasmEdgeRefType::FuncRef,
            0x6Fu32 => WasmEdgeRefType::ExternRef,
            _ => panic!("fail to convert u32 to WasmEdgeRefType: {}", value),
        }
    }
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

#[derive(Debug, Copy, Clone)]
pub enum ValType {
    I32,
    I64,
    F32,
    F64,
    V128,
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
            ValType::V128 => wasmedge::WasmEdge_ValType_V128,
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
            wasmedge::WasmEdge_ValType_V128 => ValType::V128,
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

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CompilerOptimizationLevel {
    /// Disable as many optimizations as possible.
    O0 = wasmedge::WasmEdge_CompilerOptimizationLevel_O0,

    /// Optimize quickly without destroying debuggability.
    O1 = wasmedge::WasmEdge_CompilerOptimizationLevel_O1,

    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    O2 = wasmedge::WasmEdge_CompilerOptimizationLevel_O2,

    ///  Optimize for fast execution as much as possible.
    O3 = wasmedge::WasmEdge_CompilerOptimizationLevel_O3,

    ///  Optimize for small code size as much as possible without triggering
    ///  significant incremental compile time or execution time slowdowns.
    Os = wasmedge::WasmEdge_CompilerOptimizationLevel_Os,

    /// Optimize for small code size as much as possible.
    Oz = wasmedge::WasmEdge_CompilerOptimizationLevel_Oz,
}
impl From<u32> for CompilerOptimizationLevel {
    fn from(val: u32) -> CompilerOptimizationLevel {
        match val {
            0 => CompilerOptimizationLevel::O0,
            1 => CompilerOptimizationLevel::O1,
            2 => CompilerOptimizationLevel::O2,
            3 => CompilerOptimizationLevel::O3,
            4 => CompilerOptimizationLevel::Os,
            5 => CompilerOptimizationLevel::Oz,
            _ => panic!("Unknown CompilerOptimizationLevel value: {}", val),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CompilerOutputFormat {
    /// Native dynamic library format.
    Native = wasmedge::WasmEdge_CompilerOutputFormat_Native,

    /// WebAssembly with AOT compiled codes in custom sections.
    Wasm = wasmedge::WasmEdge_CompilerOutputFormat_Wasm,
}
impl From<u32> for CompilerOutputFormat {
    fn from(val: u32) -> CompilerOutputFormat {
        match val {
            0 => CompilerOutputFormat::Native,
            1 => CompilerOutputFormat::Wasm,
            _ => panic!("Unknown CompilerOutputFormat value: {}", val),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HostRegistration {
    Wasi,
    WasmEdgeProcess,
}
impl From<u32> for HostRegistration {
    fn from(val: u32) -> Self {
        match val {
            0 => HostRegistration::Wasi,
            1 => HostRegistration::WasmEdgeProcess,
            _ => panic!("Unknown WasmEdge_HostRegistration value: {}", val),
        }
    }
}
impl From<HostRegistration> for wasmedge::WasmEdge_HostRegistration {
    fn from(val: HostRegistration) -> Self {
        match val {
            HostRegistration::Wasi => wasmedge::WasmEdge_HostRegistration_Wasi,
            HostRegistration::WasmEdgeProcess => {
                wasmedge::WasmEdge_HostRegistration_WasmEdge_Process
            }
        }
    }
}
