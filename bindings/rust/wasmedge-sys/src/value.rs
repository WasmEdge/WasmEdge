use super::wasmedge;

// TODO: FuncRef and ExternRef Support
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

// #[derive(Clone, Copy, Debug)]
// pub struct FuncRef{
//     inner: wasmedge::WasmEdge_Value
// }

// impl FuncRef {
//     fn new(func_index: u32) -> Self{
//         FuncRef {
//             inner: unsafe{ wasmedge::WasmEdge_ValueGenFuncRef(func_index)},
//         }
//     }
// }
// #[derive(Clone, Copy, Debug)]
// pub struct ExternRef{
//     inner: wasmedge::WasmEdge_Value
// }

// impl ExternRef {
//     fn new(ptr: *mut ::std::os::raw::c_void ) -> Self{
//         unsafe{
//             ExternRef{
//                 inner: wasmedge::WasmEdge_ValueGenExternRef(ptr)
//             }
//         }
//     }
// }

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
