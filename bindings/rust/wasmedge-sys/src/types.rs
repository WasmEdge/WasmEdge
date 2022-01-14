//! Defines the WebAssembly primitive types.

use crate::wasmedge;
use core::ffi::c_void;
use std::{ffi::CString, fmt};

/// Defines reference types.
///
/// `RefType` classifies first-class references to objects in the runtime [store](crate::Store).
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum RefType {
    /// `FuncRef` denotes the infinite union of all references to [functions](crate::Function), regardless of their
    /// [function types](crate::FuncType).
    FuncRef,

    /// `ExternRef` denotes the infinite union of all references to objects owned by the [Vm](crate::Vm) and that can be
    /// passed into WebAssembly under this type.
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
        if start == end {
            Self {
                Min: start,
                Max: end,
                HasMax: false,
            }
        } else {
            Self {
                Min: start,
                Max: end,
                HasMax: true,
            }
        }
    }
}
impl From<wasmedge::WasmEdge_Limit> for std::ops::RangeInclusive<u32> {
    fn from(limit: wasmedge::WasmEdge_Limit) -> Self {
        let start = limit.Min;
        let end = match limit.HasMax {
            true => limit.Max,
            false => limit.Min,
        };
        Self::new(start, end)
    }
}

/// Defines value types.
///
/// `ValType` classifies the individual values that WebAssembly code can compute with and the values that a variable
/// accepts.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum ValType {
    /// 32-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I32,
    /// 64-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I64,
    /// 32-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F32,
    /// 64-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F64,
    /// 128-bit vector of packed integer or floating-point data.
    ///
    /// The packed data can be interpreted as signed or unsigned integers, single or double precision floating-point
    /// values, or a single 128 bit type. The interpretation is determined by individual operations.
    V128,
    /// A reference to [functions](crate::Function).
    FuncRef,
    /// A reference to object owned by the [Vm](crate::Vm).
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

/// Defines WasmEdge mutability values.
///
/// `Mutability` determines a [global](crate::Global) variable is either mutable or immutable.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Mutability {
    /// Identifies an immutable global variable
    Const,
    /// Identifies a mutable global variable
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

/// Defines WasmEdge AOT compiler optimization level.
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

/// Defines WasmEdge AOT compiler output binary format.
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

/// Defines WasmEdge host module registration enum.
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

/// Defines WasmEdge ExternType values.
///
/// `ExternType` classifies [imports](crate::Import) and external values with their respective types.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u32)]
pub enum ExternalType {
    Function = wasmedge::WasmEdge_ExternalType_Function,
    Table = wasmedge::WasmEdge_ExternalType_Table,
    Memory = wasmedge::WasmEdge_ExternalType_Memory,
    Global = wasmedge::WasmEdge_ExternalType_Global,
}
impl From<u32> for ExternalType {
    fn from(val: u32) -> Self {
        match val {
            0x00u32 => ExternalType::Function,
            0x01u32 => ExternalType::Table,
            0x02u32 => ExternalType::Memory,
            0x03u32 => ExternalType::Global,
            _ => panic!("Unknown ExternalType value: {}", val),
        }
    }
}
impl fmt::Display for ExternalType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let message = match self {
            ExternalType::Function => "function",
            ExternalType::Table => "table",
            ExternalType::Memory => "memory",
            ExternalType::Global => "global",
        };
        write!(f, "{}", message)
    }
}

// /// Defines WebAssembly primitive types.
// #[derive(Clone, Copy, Debug, PartialEq)]
// pub enum Value {
//     /// 32-bit integer.
//     ///
//     /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
//     I32(i32),
//     /// 64-bit integer.
//     ///
//     /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
//     I64(i64),
//     /// 32-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
//     F32(f32),
//     /// 64-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
//     F64(f64),
//     /// 128-bit vector of packed integer or floating-point data.
//     ///
//     /// The packed data can be interpreted as signed or unsigned integers, single or double precision floating-point
//     /// values, or a single 128 bit type. The interpretation is determined by individual operations.
//     V128(i128),
//     /// A reference to [functions](crate::Function).
//     FuncRef(u32),
//     /// A reference to object owned by the [Vm](crate::Vm).
//     ExternRef(u128),
//     /// A null reference value.
//     NullRef(RefType),
// }
// impl Value {
//     /// Returns a [`Value`] which implies a reference to an external object.
//     ///
//     /// # Argument
//     ///
//     /// - `extern_obj` specifies an external object.
//     pub fn gen_extern_ref<T>(extern_obj: &mut T) -> Value {
//         unsafe {
//             let ptr = extern_obj as *mut T as *mut c_void;
//             wasmedge::WasmEdge_ValueGenExternRef(ptr).into()
//         }
//     }

//     pub fn ty(&self) -> ValType {
//         match &self {
//             Value::I32(_) => ValType::I32,
//             Value::I64(_) => ValType::I64,
//             Value::F32(_) => ValType::F32,
//             Value::F64(_) => ValType::F64,
//             Value::V128(_) => ValType::V128,
//             Value::FuncRef(_) => ValType::FuncRef,
//             Value::ExternRef(_) => ValType::ExternRef,
//             Value::NullRef(v) => match &v {
//                 RefType::FuncRef => ValType::FuncRef,
//                 RefType::ExternRef => ValType::ExternRef,
//             },
//         }
//     }
// }
// impl From<Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>> for Value {
//     // TODO:
//     // may not work, check fat pointer issue with C
//     #[allow(clippy::type_complexity)]
//     fn from(f: Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>>) -> Self {
//         let f_ptr = &f as *const dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8> as *mut c_void;
//         unsafe { wasmedge::WasmEdge_ValueGenExternRef(f_ptr).into() }
//     }
// }
// impl From<Value> for wasmedge::WasmEdge_Value {
//     fn from(value: Value) -> Self {
//         match value {
//             Value::I32(v) => unsafe { wasmedge::WasmEdge_ValueGenI32(v) },
//             Value::I64(v) => unsafe { wasmedge::WasmEdge_ValueGenI64(v) },
//             Value::F32(v) => unsafe { wasmedge::WasmEdge_ValueGenF32(v) },
//             Value::F64(v) => unsafe { wasmedge::WasmEdge_ValueGenF64(v) },
//             Value::V128(v) => unsafe { wasmedge::WasmEdge_ValueGenV128(v) },
//             Value::FuncRef(idx) => unsafe { wasmedge::WasmEdge_ValueGenFuncRef(idx) },
//             Value::ExternRef(v) => Self {
//                 Value: v as u128,
//                 Type: wasmedge::WasmEdge_ValType_ExternRef,
//             },
//             Value::NullRef(v) => unsafe { wasmedge::WasmEdge_ValueGenNullRef(v.into()) },
//         }
//     }
// }
// impl From<wasmedge::WasmEdge_Value> for Value {
//     fn from(value: wasmedge::WasmEdge_Value) -> Self {
//         match value.Type {
//             wasmedge::WasmEdge_ValType_I32 => {
//                 let val = unsafe { wasmedge::WasmEdge_ValueGetI32(value) };
//                 Self::I32(val)
//             }
//             wasmedge::WasmEdge_ValType_I64 => {
//                 let val = unsafe { wasmedge::WasmEdge_ValueGetI64(value) };
//                 Self::I64(val)
//             }
//             wasmedge::WasmEdge_ValType_F32 => {
//                 let val = unsafe { wasmedge::WasmEdge_ValueGetF32(value) };
//                 Self::F32(val)
//             }
//             wasmedge::WasmEdge_ValType_F64 => {
//                 let val = unsafe { wasmedge::WasmEdge_ValueGetF64(value) };
//                 Self::F64(val)
//             }
//             wasmedge::WasmEdge_ValType_V128 => {
//                 let val = unsafe { wasmedge::WasmEdge_ValueGetV128(value) };
//                 Self::V128(val)
//             }
//             wasmedge::WasmEdge_ValType_FuncRef => unsafe {
//                 if wasmedge::WasmEdge_ValueIsNullRef(value) {
//                     Self::NullRef(RefType::FuncRef)
//                 } else {
//                     let idx = wasmedge::WasmEdge_ValueGetFuncIdx(value);
//                     Self::FuncRef(idx)
//                 }
//             },
//             wasmedge::WasmEdge_ValType_ExternRef => unsafe {
//                 if wasmedge::WasmEdge_ValueIsNullRef(value) {
//                     Self::NullRef(RefType::ExternRef)
//                 } else {
//                     let val = wasmedge::WasmEdge_ValueGetExternRef(value);
//                     Self::ExternRef(val as u128)
//                 }
//             },
//             _ => panic!("unknown WasmEdge_ValType `{}`", value.Type),
//         }
//     }
// }

// macro_rules! impl_from_prim_conversions {
//     ($( [$($ty:ty),+] => $name:ident),+ $(,)?) => {
//         $(
//             $(
//                 impl From<$ty> for Value {
//                     fn from(value: $ty) -> Self {
//                         Self::$name(value as _)
//                     }
//                 }
//             )+
//         )+
//     }
// }

// impl_from_prim_conversions! {
//     [i8, u8, i16, u16, i32] => I32,
//     [u32, i64] => I64,
//     [f32] => F32,
//     [f64] => F64,
//     [u128] => V128,
// }

// macro_rules! impl_to_prim_conversions {
//     ($( [$($name:ident),+] => $ty:ty),+ $(,)?) => {
//         impl Value {
//             paste::paste! {
//                 $(
//                     #[doc = "Returns a `" $ty "`, if it can be converted from the value type."]
//                     pub fn [<as_ $ty>](&self) -> Option<$ty> {
//                         #[allow(unreachable_patterns)]
//                         match self {
//                             $(
//                                 Self::$name(value) => Some(*value as _),
//                             )+
//                             _ => None
//                         }
//                     }
//                 )+
//             }
//         }
//     }
// }

// impl_to_prim_conversions! {
//     [I32, F32] => i32,
//     [I32, F64] => u32,
//     [I32, I64, F32, F64] => i64,
//     [F32] => f32,
//     [F32, F64] => f64,
//     [V128] => u128,
// }

#[derive(Debug, Clone)]
pub(crate) struct WasmEdgeString {
    ctx: wasmedge::WasmEdge_String,
}
impl Drop for WasmEdgeString {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_StringDelete(self.ctx) }
    }
}
impl WasmEdgeString {
    pub(crate) fn as_raw(&self) -> wasmedge::WasmEdge_String {
        self.ctx
    }
}
impl<T: AsRef<str>> From<T> for WasmEdgeString {
    fn from(s: T) -> Self {
        let ctx = if s.as_ref().contains('\0') {
            let buffer = s.as_ref().as_bytes();
            unsafe {
                wasmedge::WasmEdge_StringCreateByBuffer(
                    buffer.as_ptr() as *const _,
                    buffer.len() as u32,
                )
            }
        } else {
            let cs = CString::new(s.as_ref()).expect(
                "Failed to create a CString: the supplied bytes contain an internal 0 byte",
            );
            unsafe { wasmedge::WasmEdge_StringCreateByCString(cs.as_ptr()) }
        };

        Self { ctx }
    }
}
impl PartialEq for WasmEdgeString {
    fn eq(&self, other: &Self) -> bool {
        unsafe { wasmedge::WasmEdge_StringIsEqual(self.ctx, other.ctx) }
    }
}
impl Eq for WasmEdgeString {}
impl From<wasmedge::WasmEdge_String> for String {
    fn from(s: wasmedge::WasmEdge_String) -> Self {
        let cstr = unsafe { std::ffi::CStr::from_ptr(s.Buf as *const _) };
        cstr.to_string_lossy().into_owned()
    }
}

/// Struct of WasmEdge Value.
#[derive(Debug, Clone, Copy)]
pub struct Value {
    raw: wasmedge::WasmEdge_Value,
    ty: ValType,
}
impl Value {
    /// Returns the raw `WasmEdge_Value`.
    pub fn as_raw(&self) -> wasmedge::WasmEdge_Value {
        self.raw
    }

    /// Returns the type of a [`Value`].
    pub fn ty(&self) -> ValType {
        self.ty
    }

    /// Creates a [`Value`] from a `i32` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `i32` value.
    pub fn from_i32(val: i32) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenI32(val) },
            ty: ValType::I32,
        }
    }

    /// Generates a `i32` value from a [`Value`].
    pub fn to_i32(&self) -> i32 {
        unsafe { wasmedge::WasmEdge_ValueGetI32(self.raw) }
    }

    /// Creates a [`Value`] from a `i64` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `i64` value.
    pub fn from_i64(val: i64) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenI64(val) },
            ty: ValType::I64,
        }
    }

    /// Generates a `i64` value from a [`Value`].
    pub fn to_i64(&self) -> i64 {
        unsafe { wasmedge::WasmEdge_ValueGetI64(self.raw) }
    }

    /// Creates a [`Value`] from a `f32` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `f32` value.
    pub fn from_f32(val: f32) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenF32(val) },
            ty: ValType::F32,
        }
    }

    /// Generates a `f32` value from a [`Value`].
    pub fn to_f32(&self) -> f32 {
        unsafe { wasmedge::WasmEdge_ValueGetF32(self.raw) }
    }

    /// Creates a [`Value`] from a `f64` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `f64` value.
    pub fn from_f64(val: f64) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenF64(val) },
            ty: ValType::F64,
        }
    }

    /// Generates a `f64` value from a [`Value`].
    pub fn to_f64(&self) -> f64 {
        unsafe { wasmedge::WasmEdge_ValueGetF64(self.raw) }
    }

    /// Creates a [`Value`] from a `i128` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `i128` value.
    pub fn from_v128(val: i128) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenV128(val) },
            ty: ValType::V128,
        }
    }

    /// Generates a `v128` value from a [`Value`].
    pub fn to_v128(&self) -> i128 {
        unsafe { wasmedge::WasmEdge_ValueGetV128(self.raw) }
    }

    /// Creates a [`Value`] from a [`RefType`] value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the `[`RefType`] value.
    pub fn from_null_ref(ref_ty: RefType) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenNullRef(ref_ty.into()) },
            ty: match ref_ty {
                RefType::FuncRef => ValType::FuncRef,
                RefType::ExternRef => ValType::ExternRef,
            },
        }
    }

    /// Checks if a [`Value`] is NullRef or not.
    pub fn is_null_ref(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ValueIsNullRef(self.raw) }
    }

    /// Creates a [`Value`] from a the function reference.
    ///
    /// The [`Value`]s generated by this function are only meaningful when the `bulk_memory_operations` option or the
    /// `reference_types` option is enabled in the [Config](crate::Config).
    ///
    /// # Argument
    ///
    /// - `idx` specifies the function index.
    pub fn from_func_ref(idx: u32) -> Self {
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenFuncRef(idx) },
            ty: ValType::FuncRef,
        }
    }

    /// Returns the function index.
    ///
    /// If the [`Value`] is a `NullRef`, then `None` is returned.
    pub fn func_idx(&self) -> Option<u32> {
        unsafe {
            match wasmedge::WasmEdge_ValueIsNullRef(self.raw) {
                true => None,
                false => {
                    let idx = wasmedge::WasmEdge_ValueGetFuncIdx(self.raw);
                    Some(idx)
                }
            }
        }
    }

    /// Creates a [`Value`] from a reference to an external object.
    ///
    /// The [`Value`]s generated by this function are only meaningful when the `reference_types` option is enabled in
    /// the [Config](crate::Config).
    ///
    /// # Argument
    ///
    /// - `extern_obj` specifies the reference to an external object.
    pub fn from_extern_ref<T>(extern_obj: &mut T) -> Self {
        let ptr = extern_obj as *mut T as *mut c_void;
        Self {
            raw: unsafe { wasmedge::WasmEdge_ValueGenExternRef(ptr) },
            ty: ValType::ExternRef,
        }
    }

    /// Returns the reference to an external object.
    ///
    /// If the [`Value`] is a `NullRef`, then `None` is returned.
    pub fn extern_ref<T>(&self) -> Option<&T> {
        unsafe {
            match wasmedge::WasmEdge_ValueIsNullRef(self.raw) {
                true => None,
                false => {
                    let ptr = wasmedge::WasmEdge_ValueGetExternRef(self.raw);
                    let x = ptr as *mut T;
                    Some(&*x)
                }
            }
        }
    }
}
impl From<wasmedge::WasmEdge_Value> for Value {
    fn from(raw_val: wasmedge::WasmEdge_Value) -> Self {
        match raw_val.Type {
            wasmedge::WasmEdge_ValType_I32 => Self {
                raw: raw_val,
                ty: ValType::I32,
            },
            wasmedge::WasmEdge_ValType_I64 => Self {
                raw: raw_val,
                ty: ValType::I64,
            },
            wasmedge::WasmEdge_ValType_F32 => Self {
                raw: raw_val,
                ty: ValType::F32,
            },
            wasmedge::WasmEdge_ValType_F64 => Self {
                raw: raw_val,
                ty: ValType::F64,
            },
            wasmedge::WasmEdge_ValType_V128 => Self {
                raw: raw_val,
                ty: ValType::V128,
            },
            wasmedge::WasmEdge_ValType_FuncRef => Self {
                raw: raw_val,
                ty: ValType::FuncRef,
            },
            wasmedge::WasmEdge_ValType_ExternRef => Self {
                raw: raw_val,
                ty: ValType::ExternRef,
            },
            _ => panic!("unknown WasmEdge_ValType `{}`", raw_val.Type),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{RefType, Table, TableType};

    #[test]
    fn test_value() {
        // I32
        let val = Value::from_i32(1314);
        assert_eq!(val.ty(), ValType::I32);

        // I64
        let val = Value::from_i64(1314);
        assert_eq!(val.ty(), ValType::I64);

        // F32
        let val = Value::from_f32(13.14);
        assert_eq!(val.ty(), ValType::F32);

        // F64
        let val = Value::from_f64(13.14);
        assert_eq!(val.ty(), ValType::F64);

        // V128
        let val = Value::from_v128(1314);
        assert_eq!(val.ty(), ValType::V128);

        // ExternRef
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        let result = Table::create(&mut ty);
        assert!(result.is_ok());
        let mut table = result.unwrap();
        let value = Value::from_extern_ref(&mut table);
        assert_eq!(value.ty(), ValType::ExternRef);
        assert!(value.extern_ref::<Table>().is_some());

        // NullRef(FuncRef)
        let val = Value::from_null_ref(RefType::FuncRef);
        assert_eq!(val.ty(), ValType::FuncRef);
        assert!(val.is_null_ref());
        assert_eq!(val.ty(), ValType::FuncRef);

        // NullRef(ExternRef)
        let val = Value::from_null_ref(RefType::ExternRef);
        assert_eq!(val.ty(), ValType::ExternRef);
        assert!(val.is_null_ref());
        assert_eq!(val.ty(), ValType::ExternRef);
    }

    #[test]
    fn test_string() {
        let s: WasmEdgeString = "hello".into();
        let t: WasmEdgeString = "hello".into();
        assert_eq!(s, t);

        let s: WasmEdgeString = "hello".into();
        let t: WasmEdgeString = "hello\0".into();
        assert_ne!(s, t);
    }
}
