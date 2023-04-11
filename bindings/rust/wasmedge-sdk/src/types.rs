//! Defines the general types.

use crate::FuncRef;
use wasmedge_sys::WasmValue;
use wasmedge_types::{self, FuncType, RefType};

/// Defines runtime values that a WebAssembly module can either consume or produce.
#[derive(Debug, Clone)]
pub enum Val {
    /// 32-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I32(i32),
    /// 64-bit integer.
    ///
    /// Integers are not inherently signed or unsigned, their interpretation is determined by individual operations.
    I64(i64),
    /// 32-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F32(f32),
    /// 64-bit floating-point data as defined by the [IEEE 754-2019](https://ieeexplore.ieee.org/document/8766229).
    F64(f64),
    /// 128-bit vector of packed integer or floating-point data.
    ///
    /// The packed data can be interpreted as signed or unsigned integers, single or double precision floating-point
    /// values, or a single 128 bit type. The interpretation is determined by individual operations.
    V128(i128),
    /// A reference to a [host function](crate::Func).
    ///
    /// `FuncRef(None)` is the null function reference, created by `ref.null
    /// func` in Wasm.
    FuncRef(Option<FuncRef>),
    /// A reference to opaque data in the wasm instance.
    ///
    /// `ExternRef(None)` is the null external reference, created by `ref.null
    /// extern` in Wasm.
    ExternRef(Option<ExternRef>),
}
impl From<Val> for WasmValue {
    fn from(val: Val) -> Self {
        match val {
            Val::I32(i) => WasmValue::from_i32(i),
            Val::I64(i) => WasmValue::from_i64(i),
            Val::F32(i) => WasmValue::from_f32(i),
            Val::F64(i) => WasmValue::from_f64(i),
            Val::V128(i) => WasmValue::from_v128(i),
            Val::FuncRef(Some(func_ref)) => WasmValue::from_func_ref(func_ref.inner),
            Val::FuncRef(None) => WasmValue::from_null_ref(RefType::FuncRef),
            Val::ExternRef(Some(extern_ref)) => extern_ref.inner,
            Val::ExternRef(None) => WasmValue::from_null_ref(RefType::ExternRef),
        }
    }
}
impl From<WasmValue> for Val {
    fn from(value: WasmValue) -> Self {
        match value.ty() {
            wasmedge_types::ValType::I32 => Val::I32(value.to_i32()),
            wasmedge_types::ValType::I64 => Val::I64(value.to_i64()),
            wasmedge_types::ValType::F32 => Val::F32(value.to_f32()),
            wasmedge_types::ValType::F64 => Val::F64(value.to_f64()),
            wasmedge_types::ValType::V128 => Val::V128(value.to_v128()),
            wasmedge_types::ValType::FuncRef => {
                if value.is_null_ref() {
                    Val::FuncRef(None)
                } else {
                    let inner = value
                        .func_ref()
                        .expect("[wasmedge] Failed to convert a WasmValue to a FuncRef.");
                    let ty: FuncType = inner.ty().unwrap().into();
                    Val::FuncRef(Some(FuncRef { inner, ty }))
                }
            }
            wasmedge_types::ValType::ExternRef => {
                if value.is_null_ref() {
                    Val::ExternRef(None)
                } else {
                    Val::ExternRef(Some(ExternRef { inner: value }))
                }
            }
        }
    }
}

/// A reference to opaque data in the wasm instance.
#[derive(Debug, Clone, Copy)]
pub struct ExternRef {
    pub(crate) inner: WasmValue,
}
impl ExternRef {
    /// Creates a new instance of [ExternRef] wrapping the given value.
    pub fn new<T>(extern_obj: &mut T) -> Self
    where
        T: 'static + Send + Sync,
    {
        let inner = WasmValue::from_extern_ref(extern_obj);
        Self { inner }
    }
}
