//! Defines the general types.

use crate::{wasmedge, Func, GlobalType, MemoryType, Signature, TableType};
use std::marker::PhantomData;

/// External types.
#[derive(Debug)]
pub enum ExternalType {
    /// The [signature](crate::Signature) of a [host function](crate::Func).
    Func(Signature),
    /// The [table type](crate::TableType) of a [table](crate::Table).
    Table(TableType),
    /// The [memory type](crate::MemoryType) of a [memory](crate::Memory).
    Memory(MemoryType),
    /// The [global type](crate::GlobalType) of a [global](crate::Global).
    Global(GlobalType),
}

/// Runtime values that a WebAssembly module can either consume or produce.
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
    /// An `externref` value which can hold opaque data to the Wasm instance
    /// itself.
    ///
    /// `ExternRef(None)` is the null external reference, created by `ref.null
    /// extern` in Wasm.
    ExternRef(Option<ExternRef>),
}
impl From<Val> for wasmedge::Value {
    fn from(val: Val) -> Self {
        match val {
            Val::I32(i) => wasmedge::Value::from_i32(i),
            Val::I64(i) => wasmedge::Value::from_i64(i),
            Val::F32(i) => wasmedge::Value::from_f32(i),
            Val::F64(i) => wasmedge::Value::from_f64(i),
            Val::V128(i) => wasmedge::Value::from_v128(i),
            Val::FuncRef(Some(func_ref)) => func_ref.inner,
            Val::FuncRef(None) => wasmedge::Value::from_null_ref(wasmedge::RefType::FuncRef),
            Val::ExternRef(Some(extern_ref)) => extern_ref.inner,
            Val::ExternRef(None) => wasmedge::Value::from_null_ref(wasmedge::RefType::ExternRef),
        }
    }
}
impl From<wasmedge::Value> for Val {
    fn from(value: wasmedge::Value) -> Self {
        match value.ty() {
            wasmedge::ValType::I32 => Val::I32(value.to_i32()),
            wasmedge::ValType::I64 => Val::I64(value.to_i64()),
            wasmedge::ValType::F32 => Val::F32(value.to_f32()),
            wasmedge::ValType::F64 => Val::F64(value.to_f64()),
            wasmedge::ValType::V128 => Val::V128(value.to_v128()),
            wasmedge::ValType::FuncRef => {
                if value.is_null_ref() {
                    Val::FuncRef(None)
                } else {
                    Val::FuncRef(Some(FuncRef { inner: value }))
                }
            }
            wasmedge::ValType::ExternRef => {
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
    pub(crate) inner: wasmedge::Value,
}
impl FuncRef {
    pub fn new(func_ref: &mut Func) -> Self {
        let inner = wasmedge::Value::from_func_ref(&mut func_ref.inner);
        Self { inner }
    }

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
    pub(crate) inner: wasmedge::Value,
}
impl ExternRef {
    /// Creates a new instance of `ExternRef` wrapping the given value.
    pub fn new<T>(extern_obj: &mut T) -> Self
    where
        T: 'static + Send + Sync,
    {
        let inner = wasmedge::Value::from_extern_ref(extern_obj);
        Self { inner }
    }
}
