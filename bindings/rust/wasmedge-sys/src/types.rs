use crate::wasmedge;
use core::ffi::c_void;
use std::fmt;

pub type WasmEdgeProposal = wasmedge::WasmEdge_Proposal;
pub type HostFunc = wasmedge::WasmEdge_HostFunc_t;
pub type WrapFunc = wasmedge::WasmEdge_WrapFunc_t;

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum RefType {
    FuncRef,
    ExternRef,
}
impl From<u32> for RefType {
    fn from(value: u32) -> Self {
        match value {
            0x70u32 => RefType::FuncRef,
            0x6Fu32 => RefType::ExternRef,
            _ => panic!("fail to convert u32 to WasmEdgeRefType: {}", value),
        }
    }
}
impl From<RefType> for wasmedge::WasmEdge_RefType {
    fn from(ty: RefType) -> Self {
        match ty {
            RefType::FuncRef => wasmedge::WasmEdge_RefType_FuncRef,
            RefType::ExternRef => wasmedge::WasmEdge_RefType_ExternRef,
        }
    }
}

impl From<std::ops::RangeInclusive<u32>> for wasmedge::WasmEdge_Limit {
    fn from(range: std::ops::RangeInclusive<u32>) -> Self {
        let (start, end) = range.into_inner();
        Self {
            Min: start,
            Max: end,
            HasMax: true,
        }
    }
}
impl From<wasmedge::WasmEdge_Limit> for std::ops::RangeInclusive<u32> {
    fn from(limit: wasmedge::WasmEdge_Limit) -> Self {
        let start = limit.Min;
        let end = match limit.HasMax {
            true => limit.Max,
            false => u32::MAX,
        };
        Self::new(start, end)
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
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

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
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

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u32)]
pub enum ExternType {
    Function = wasmedge::WasmEdge_ExternalType_Function,
    Table = wasmedge::WasmEdge_ExternalType_Table,
    Memory = wasmedge::WasmEdge_ExternalType_Memory,
    Global = wasmedge::WasmEdge_ExternalType_Global,
}
impl From<u32> for ExternType {
    fn from(val: u32) -> Self {
        match val {
            0x00u32 => ExternType::Function,
            0x01u32 => ExternType::Table,
            0x02u32 => ExternType::Memory,
            0x03u32 => ExternType::Global,
            _ => panic!("Unknown ExternalType value: {}", val),
        }
    }
}
impl fmt::Display for ExternType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let message = match self {
            ExternType::Function => "function",
            ExternType::Table => "table",
            ExternType::Memory => "memory",
            ExternType::Global => "global",
        };
        write!(f, "{}", message)
    }
}

/// A polymorphic Wasm primitive type.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Value {
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
    V128(u128),
    /// A reference to a Wasm function.
    FuncRef(u128),
    /// A reference to opaque data in the Wasm instance.
    ExternRef(u128),
}
impl Value {
    pub fn gen_extern_ref(&mut self) -> Value {
        unsafe {
            let self_ptr: *mut c_void = self as *mut _ as *mut c_void;
            wasmedge::WasmEdge_ValueGenExternRef(self_ptr).into()
        }
    }

    pub fn ty(&self) -> ValType {
        match &self {
            Value::I32(_) => ValType::I32,
            Value::I64(_) => ValType::I64,
            Value::F32(_) => ValType::F32,
            Value::F64(_) => ValType::F64,
            Value::V128(_) => ValType::V128,
            Value::FuncRef(_) => ValType::FuncRef,
            Value::ExternRef(_) => ValType::ExternRef,
        }
    }
}
impl From<Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>> for Value {
    // TODO:
    // may not work, check fat pointer issue with C
    #[allow(clippy::type_complexity)]
    fn from(f: Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>) -> Self {
        let f_ptr = &f as *const dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8> as *mut c_void;
        unsafe { wasmedge::WasmEdge_ValueGenExternRef(f_ptr).into() }
    }
}
impl From<Value> for wasmedge::WasmEdge_Value {
    fn from(value: Value) -> Self {
        match value {
            Value::I32(v) => Self {
                Value: v as u128,
                Type: wasmedge::WasmEdge_ValType_I32,
            },
            Value::I64(v) => Self {
                Value: v as u128,
                Type: wasmedge::WasmEdge_ValType_I64,
            },
            Value::F32(v) => Self {
                Value: v.to_bits() as u128,
                Type: wasmedge::WasmEdge_ValType_F32,
            },
            Value::F64(v) => Self {
                Value: v.to_bits() as u128,
                Type: wasmedge::WasmEdge_ValType_F64,
            },
            Value::V128(v) => Self {
                Value: v as u128,
                Type: wasmedge::WasmEdge_ValType_V128,
            },
            Value::FuncRef(v) => Self {
                Value: v as u128,
                Type: wasmedge::WasmEdge_ValType_FuncRef,
            },
            Value::ExternRef(v) => Self {
                Value: v as u128,
                Type: wasmedge::WasmEdge_ValType_ExternRef,
            },
        }
    }
}
impl From<wasmedge::WasmEdge_Value> for Value {
    fn from(value: wasmedge::WasmEdge_Value) -> Self {
        match value.Type {
            wasmedge::WasmEdge_ValType_I32 => Self::I32(value.Value as i32),
            wasmedge::WasmEdge_ValType_I64 => Self::I64(value.Value as i64),
            wasmedge::WasmEdge_ValType_F32 => Self::F32(f32::from_bits(value.Value as u32)),
            wasmedge::WasmEdge_ValType_F64 => Self::F64(f64::from_bits(value.Value as u64)),
            wasmedge::WasmEdge_ValType_V128 => Self::V128(value.Value as u128),
            wasmedge::WasmEdge_ValType_FuncRef => Self::FuncRef(value.Value as u128),
            wasmedge::WasmEdge_ValType_ExternRef => Self::ExternRef(value.Value as u128),
            _ => panic!("unknown WasmEdge_ValType `{}`", value.Type),
        }
    }
}

macro_rules! impl_from_prim_conversions {
    ($( [$($ty:ty),+] => $name:ident),+ $(,)?) => {
        $(
            $(
                impl From<$ty> for Value {
                    fn from(value: $ty) -> Self {
                        Self::$name(value as _)
                    }
                }
            )+
        )+
    }
}

impl_from_prim_conversions! {
    [i8, u8, i16, u16, i32] => I32,
    [u32, i64] => I64,
    [f32] => F32,
    [f64] => F64,
    [u128] => V128,
}

macro_rules! impl_to_prim_conversions {
    ($( [$($name:ident),+] => $ty:ty),+ $(,)?) => {
        impl Value {
            paste::paste! {
                $(
                    #[doc = "Returns a `" $ty "`, if it can be converted from the value type."]
                    pub fn [<as_ $ty>](&self) -> Option<$ty> {
                        #[allow(unreachable_patterns)]
                        match self {
                            $(
                                Self::$name(value) => Some(*value as _),
                            )+
                            _ => None
                        }
                    }
                )+
            }
        }
    }
}

impl_to_prim_conversions! {
    [I32, F32] => i32,
    [I32, F64] => u32,
    [I32, I64, F32, F64] => i64,
    [F32] => f32,
    [F32, F64] => f64,
    [V128] => u128,
}
