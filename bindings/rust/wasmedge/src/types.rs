//! Defines the general types.

use crate::{
    sys::{WasmValue, WasmValueType},
    Func,
};
use std::marker::PhantomData;
use wasmedge_types::{self, RefType};

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
    /// Type of [FuncRef] value which is a reference to a [host function](crate::Func).
    FuncRef,
    /// Type of [ExternRef] value which can hold opaque data to the Wasm instance itself.
    ExternRef,
    /// Unknown.
    None,
}
impl From<WasmValueType> for ValType {
    fn from(wasm_value_type: WasmValueType) -> Self {
        match wasm_value_type {
            WasmValueType::I32 => ValType::I32,
            WasmValueType::I64 => ValType::I64,
            WasmValueType::F32 => ValType::F32,
            WasmValueType::F64 => ValType::F64,
            WasmValueType::V128 => ValType::V128,
            WasmValueType::FuncRef => ValType::FuncRef,
            WasmValueType::ExternRef => ValType::ExternRef,
            WasmValueType::None => ValType::None,
        }
    }
}
impl From<ValType> for WasmValueType {
    fn from(val_type: ValType) -> Self {
        match val_type {
            ValType::I32 => WasmValueType::I32,
            ValType::I64 => WasmValueType::I64,
            ValType::F32 => WasmValueType::F32,
            ValType::F64 => WasmValueType::F64,
            ValType::V128 => WasmValueType::V128,
            ValType::FuncRef => WasmValueType::FuncRef,
            ValType::ExternRef => WasmValueType::ExternRef,
            ValType::None => WasmValueType::None,
        }
    }
}

/// Defines runtime values that a WebAssembly module can either consume or produce.
#[derive(Debug)]
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
    /// An `ExternRef` value which can hold opaque data to the Wasm instance itself.
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
            Val::FuncRef(Some(func_ref)) => func_ref.inner,
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
                    Val::FuncRef(Some(FuncRef { inner: value }))
                }
            }
            wasmedge_types::ValType::ExternRef => {
                if value.is_null_ref() {
                    Val::ExternRef(None)
                } else {
                    Val::ExternRef(Some(ExternRef { inner: value }))
                }
            }
            _ => panic!("unsupported value type"),
        }
    }
}

/// Struct of WasmEdge FuncRef.
#[derive(Debug)]
pub struct FuncRef {
    pub(crate) inner: WasmValue,
}
impl FuncRef {
    /// Creates a new instance of [FuncRef] from the given [host function](crate::Func).
    ///
    /// # Argument
    ///
    /// - `func` - The [host function](crate::Func) to create the [FuncRef] from.
    pub fn new(func_ref: &mut Func) -> Self {
        let inner = WasmValue::from_func_ref(&mut func_ref.inner);
        Self { inner }
    }

    /// Generates a [host function](crate::Func) from this [FuncRef].
    pub fn as_func(&self) -> Option<Func> {
        let result = self.inner.func_ref();
        if let Some(inner) = result {
            return Some(Func {
                inner,
                _marker: PhantomData,
            });
        }
        None
    }
}

/// Struct of WasmEdge ExternRef.
#[derive(Debug)]
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
