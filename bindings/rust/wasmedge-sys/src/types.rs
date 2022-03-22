//! Defines the WebAssembly primitive types.

use crate::{instance::function::InnerFunc, wasmedge, Function};
use core::ffi::c_void;
use std::{ffi::CString, fmt, str::FromStr};

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
            _ => panic!("fail to convert u32 to WasmEdgeRefType: {:#X}", value),
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
    /// Unknown.
    None,
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
            ValType::None => wasmedge::WasmEdge_ValType_None,
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
            wasmedge::WasmEdge_ValType_None => ValType::None,
            _ => panic!("unknown WasmEdge_ValType `{:#X}`", ty),
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
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
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

#[derive(Debug)]
pub(crate) struct WasmEdgeString {
    inner: InnerWasmEdgeString,
}
impl Drop for WasmEdgeString {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_StringDelete(self.inner.0) }
    }
}
impl WasmEdgeString {
    pub(crate) fn as_raw(&self) -> wasmedge::WasmEdge_String {
        self.inner.0
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

        Self {
            inner: InnerWasmEdgeString(ctx),
        }
    }
}
impl PartialEq for WasmEdgeString {
    fn eq(&self, other: &Self) -> bool {
        unsafe { wasmedge::WasmEdge_StringIsEqual(self.inner.0, other.inner.0) }
    }
}
impl Eq for WasmEdgeString {}
impl From<WasmEdgeString> for String {
    fn from(s: WasmEdgeString) -> Self {
        let bytes: &[u8] = unsafe {
            std::slice::from_raw_parts(s.as_raw().Buf as *const u8, s.as_raw().Length as usize)
        };
        let x =
            std::str::from_utf8(bytes).expect("Fail to generate string slice: invalid utf8 bytes");
        String::from_str(x).expect("Ill-formatted string")
    }
}
impl From<wasmedge::WasmEdge_String> for String {
    fn from(s: wasmedge::WasmEdge_String) -> Self {
        let cstr = unsafe { std::ffi::CStr::from_ptr(s.Buf as *const _) };
        cstr.to_string_lossy().into_owned()
    }
}

#[derive(Debug)]
pub(crate) struct InnerWasmEdgeString(pub(crate) wasmedge::WasmEdge_String);
unsafe impl Send for InnerWasmEdgeString {}
unsafe impl Sync for InnerWasmEdgeString {}

/// Struct of WasmEdge Value.
#[derive(Debug, Clone, Copy)]
pub struct Value {
    ctx: wasmedge::WasmEdge_Value,
    ty: ValType,
}
impl Value {
    /// Returns the raw `WasmEdge_Value`.
    pub fn as_raw(&self) -> wasmedge::WasmEdge_Value {
        self.ctx
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
            ctx: unsafe { wasmedge::WasmEdge_ValueGenI32(val) },
            ty: ValType::I32,
        }
    }

    /// Generates a `i32` value from a [`Value`].
    pub fn to_i32(&self) -> i32 {
        unsafe { wasmedge::WasmEdge_ValueGetI32(self.ctx) }
    }

    /// Creates a [`Value`] from a `i64` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `i64` value.
    pub fn from_i64(val: i64) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenI64(val) },
            ty: ValType::I64,
        }
    }

    /// Generates a `i64` value from a [`Value`].
    pub fn to_i64(&self) -> i64 {
        unsafe { wasmedge::WasmEdge_ValueGetI64(self.ctx) }
    }

    /// Creates a [`Value`] from a `f32` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `f32` value.
    pub fn from_f32(val: f32) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenF32(val) },
            ty: ValType::F32,
        }
    }

    /// Generates a `f32` value from a [`Value`].
    pub fn to_f32(&self) -> f32 {
        unsafe { wasmedge::WasmEdge_ValueGetF32(self.ctx) }
    }

    /// Creates a [`Value`] from a `f64` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `f64` value.
    pub fn from_f64(val: f64) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenF64(val) },
            ty: ValType::F64,
        }
    }

    /// Generates a `f64` value from a [`Value`].
    pub fn to_f64(&self) -> f64 {
        unsafe { wasmedge::WasmEdge_ValueGetF64(self.ctx) }
    }

    /// Creates a [`Value`] from a `i128` value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the source `i128` value.
    pub fn from_v128(val: i128) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenV128(val) },
            ty: ValType::V128,
        }
    }

    /// Generates a `v128` value from a [`Value`].
    pub fn to_v128(&self) -> i128 {
        unsafe { wasmedge::WasmEdge_ValueGetV128(self.ctx) }
    }

    /// Creates a [`Value`] from a [`RefType`] value.
    ///
    /// # Argument
    ///
    /// - `val` specifies the `[`RefType`] value.
    pub fn from_null_ref(ref_ty: RefType) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenNullRef(ref_ty.into()) },
            ty: match ref_ty {
                RefType::FuncRef => ValType::FuncRef,
                RefType::ExternRef => ValType::ExternRef,
            },
        }
    }

    /// Checks if a [`Value`] is NullRef or not.
    pub fn is_null_ref(&self) -> bool {
        unsafe { wasmedge::WasmEdge_ValueIsNullRef(self.ctx) }
    }

    /// Creates a [`Value`] from a the function reference.
    ///
    /// The [`Value`]s generated by this function are only meaningful when the `bulk_memory_operations` option or the
    /// `reference_types` option is enabled in the [Config](crate::Config).
    ///
    /// # Argument
    ///
    /// - `idx` specifies the function index.
    pub fn from_func_ref(func: &mut Function) -> Self {
        Self {
            ctx: unsafe { wasmedge::WasmEdge_ValueGenFuncRef(func.inner.0) },
            ty: ValType::FuncRef,
        }
    }

    /// Returns the function index.
    ///
    /// If the [`Value`] is a `NullRef`, then `None` is returned.
    pub fn func_ref(&self) -> Option<Function> {
        unsafe {
            match wasmedge::WasmEdge_ValueIsNullRef(self.ctx) {
                true => None,
                false => {
                    let ctx = wasmedge::WasmEdge_ValueGetFuncRef(self.ctx);
                    // Some(idx)
                    Some(Function {
                        inner: InnerFunc(ctx),
                        registered: true,
                    })
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
            ctx: unsafe { wasmedge::WasmEdge_ValueGenExternRef(ptr) },
            ty: ValType::ExternRef,
        }
    }

    /// Returns the reference to an external object.
    ///
    /// If the [`Value`] is a `NullRef`, then `None` is returned.
    pub fn extern_ref<T>(&self) -> Option<&T> {
        unsafe {
            match wasmedge::WasmEdge_ValueIsNullRef(self.ctx) {
                true => None,
                false => {
                    let ptr = wasmedge::WasmEdge_ValueGetExternRef(self.ctx);
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
                ctx: raw_val,
                ty: ValType::I32,
            },
            wasmedge::WasmEdge_ValType_I64 => Self {
                ctx: raw_val,
                ty: ValType::I64,
            },
            wasmedge::WasmEdge_ValType_F32 => Self {
                ctx: raw_val,
                ty: ValType::F32,
            },
            wasmedge::WasmEdge_ValType_F64 => Self {
                ctx: raw_val,
                ty: ValType::F64,
            },
            wasmedge::WasmEdge_ValType_V128 => Self {
                ctx: raw_val,
                ty: ValType::V128,
            },
            wasmedge::WasmEdge_ValType_FuncRef => Self {
                ctx: raw_val,
                ty: ValType::FuncRef,
            },
            wasmedge::WasmEdge_ValType_ExternRef => Self {
                ctx: raw_val,
                ty: ValType::ExternRef,
            },
            _ => panic!("unknown WasmEdge_ValType `{}`", raw_val.Type),
        }
    }
}
impl PartialEq for Value {
    fn eq(&self, other: &Self) -> bool {
        self.ctx.Value == other.ctx.Value && self.ctx.Type == other.ctx.Type
    }
}
impl Eq for Value {}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{RefType, Table, TableType};
    use std::{
        sync::{Arc, Mutex},
        thread,
    };

    #[test]
    fn test_types_value() {
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
        let ty = result.unwrap();
        let result = Table::create(&ty);
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

        let val1 = Value::from_i32(1314);
        let val2 = Value::from_i32(1314);
        assert_eq!(val1, val2);

        let val1 = Value::from_i32(1314);
        let val2 = Value::from_i64(1314);
        assert_ne!(val1, val2);

        let val1 = Value::from_i32(1314);
        let val2 = Value::from_f32(13.14);
        assert_ne!(val1, val2);
    }

    #[test]
    fn test_types_value_send() {
        // I32
        let val_i32 = Value::from_i32(1314);
        // I64
        let val_i64 = Value::from_i64(1314);
        // F32
        let val_f32 = Value::from_f32(13.14);
        // F64
        let val_f64 = Value::from_f64(13.14);
        // V128
        let val_v128 = Value::from_v128(1314);

        // ExternRef
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Table::create(&ty);
        assert!(result.is_ok());
        let mut table = result.unwrap();
        let val_extern_ref = Value::from_extern_ref(&mut table);

        // NullRef(FuncRef)
        let val_null_func_ref = Value::from_null_ref(RefType::FuncRef);

        // NullRef(ExternRef)
        let val_null_extern_ref = Value::from_null_ref(RefType::ExternRef);

        let handle = thread::spawn(move || {
            let val_i32_c = val_i32;
            assert_eq!(val_i32_c.ty(), ValType::I32);

            let val_i64_c = val_i64;
            assert_eq!(val_i64_c.ty(), ValType::I64);

            let val_f32_c = val_f32;
            assert_eq!(val_f32_c.ty(), ValType::F32);

            let val_f64_c = val_f64;
            assert_eq!(val_f64_c.ty(), ValType::F64);

            let val_v128_c = val_v128;
            assert_eq!(val_v128_c.ty(), ValType::V128);

            let val_extern_ref_c = val_extern_ref;
            assert_eq!(val_extern_ref_c.ty(), ValType::ExternRef);
            assert!(val_extern_ref_c.extern_ref::<Table>().is_some());

            let val_null_func_ref_c = val_null_func_ref;
            assert_eq!(val_null_func_ref_c.ty(), ValType::FuncRef);
            assert!(val_null_func_ref_c.is_null_ref());
            assert_eq!(val_null_func_ref_c.ty(), ValType::FuncRef);

            let val_null_extern_ref_c = val_null_extern_ref;
            assert_eq!(val_null_extern_ref_c.ty(), ValType::ExternRef);
            assert!(val_null_extern_ref_c.is_null_ref());
            assert_eq!(val_null_extern_ref_c.ty(), ValType::ExternRef);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_types_value_sync() {
        // I32
        let val_i32 = Arc::new(Mutex::new(Value::from_i32(1314)));
        let val_i32_cloned = Arc::clone(&val_i32);

        // I64
        let val_i64 = Arc::new(Mutex::new(Value::from_i64(1314)));
        let val_i64_cloned = Arc::clone(&val_i64);

        // F32
        let val_f32 = Arc::new(Mutex::new(Value::from_f32(13.14)));
        let val_f32_cloned = Arc::clone(&val_f32);

        // F64
        let val_f64 = Arc::new(Mutex::new(Value::from_f64(13.14)));
        let val_f64_cloned = Arc::clone(&val_f64);

        // V128
        let val_v128 = Arc::new(Mutex::new(Value::from_v128(1314)));
        let val_v128_cloned = Arc::clone(&val_v128);

        // ExternRef
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let ty = result.unwrap();
        let result = Table::create(&ty);
        assert!(result.is_ok());
        let mut table = result.unwrap();
        let val_extern_ref = Arc::new(Mutex::new(Value::from_extern_ref(&mut table)));
        let val_extern_ref_cloned = Arc::clone(&val_extern_ref);

        // NullRef(FuncRef)
        let val_null_func_ref = Arc::new(Mutex::new(Value::from_null_ref(RefType::FuncRef)));
        let val_null_func_ref_cloned = Arc::clone(&val_null_func_ref);

        // NullRef(ExternRef)
        let val_null_extern_ref = Arc::new(Mutex::new(Value::from_null_ref(RefType::ExternRef)));
        let val_null_extern_ref_cloned = Arc::clone(&val_null_extern_ref);

        let handle = thread::spawn(move || {
            let result = val_i32_cloned.lock();
            assert!(result.is_ok());
            let val_i32_c = result.unwrap();
            assert_eq!(val_i32_c.ty(), ValType::I32);

            let result = val_i64_cloned.lock();
            assert!(result.is_ok());
            let val_i64_c = result.unwrap();
            assert_eq!(val_i64_c.ty(), ValType::I64);

            let result = val_f32_cloned.lock();
            assert!(result.is_ok());
            let val_f32_c = result.unwrap();
            assert_eq!(val_f32_c.ty(), ValType::F32);

            let result = val_f64_cloned.lock();
            assert!(result.is_ok());
            let val_f64_c = result.unwrap();
            assert_eq!(val_f64_c.ty(), ValType::F64);

            let result = val_v128_cloned.lock();
            assert!(result.is_ok());
            let val_v128_c = result.unwrap();
            assert_eq!(val_v128_c.ty(), ValType::V128);

            let result = val_extern_ref_cloned.lock();
            assert!(result.is_ok());
            let val_extern_ref_c = result.unwrap();
            assert_eq!(val_extern_ref_c.ty(), ValType::ExternRef);
            assert!(val_extern_ref_c.extern_ref::<Table>().is_some());

            let result = val_null_func_ref_cloned.lock();
            assert!(result.is_ok());
            let val_null_func_ref_c = result.unwrap();
            assert_eq!(val_null_func_ref_c.ty(), ValType::FuncRef);
            assert!(val_null_func_ref_c.is_null_ref());
            assert_eq!(val_null_func_ref_c.ty(), ValType::FuncRef);

            let result = val_null_extern_ref_cloned.lock();
            assert!(result.is_ok());
            let val_null_extern_ref_c = result.unwrap();
            assert_eq!(val_null_extern_ref_c.ty(), ValType::ExternRef);
            assert!(val_null_extern_ref_c.is_null_ref());
            assert_eq!(val_null_extern_ref_c.ty(), ValType::ExternRef);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_types_string() {
        let s: WasmEdgeString = "hello".into();
        let t: WasmEdgeString = "hello".into();
        assert_eq!(s, t);

        let s: WasmEdgeString = "hello".into();
        let t = String::from(s);
        assert_eq!(t, "hello");

        let s: WasmEdgeString = "hello".into();
        let t: WasmEdgeString = "hello\0".into();
        assert_ne!(s, t);

        let s: WasmEdgeString = "hello\0".into();
        let t = String::from(s);
        assert_eq!(t, "hello\0");

        let s: WasmEdgeString = "he\0llo\0".into();
        let t = String::from(s);
        assert_eq!(t, "he\0llo\0");
    }

    #[test]
    fn test_types_string_send() {
        let s: WasmEdgeString = "hello".into();

        let handle = thread::spawn(move || {
            let t: WasmEdgeString = "hello".into();
            assert_eq!(s, t);
        });

        handle.join().unwrap();
    }

    #[test]
    fn test_types_string_sync() {
        let s: WasmEdgeString = "hello".into();
        let p = Arc::new(Mutex::new(s));

        let s_cloned = Arc::clone(&p);
        let handle = thread::spawn(move || {
            let result = s_cloned.lock();
            assert!(result.is_ok());
            let s = result.unwrap();

            let t: WasmEdgeString = "hello".into();
            assert_eq!(*s, t);
        });

        handle.join().unwrap();
    }
}
