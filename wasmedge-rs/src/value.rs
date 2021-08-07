use wasmedge_sys as ffi;

/// A polymorphic Wasm primitive type.
#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
}

impl From<Value> for ffi::WasmEdge_Value {
    fn from(value: Value) -> Self {
        match value {
            Value::I32(v) => Self {
                Value: v as u128,
                Type: ffi::WasmEdge_ValType_I32,
            },
            Value::I64(v) => Self {
                Value: v as u128,
                Type: ffi::WasmEdge_ValType_I64,
            },
            Value::F32(v) => Self {
                Value: v.to_bits() as u128,
                Type: ffi::WasmEdge_ValType_F32,
            },
            Value::F64(v) => Self {
                Value: v.to_bits() as u128,
                Type: ffi::WasmEdge_ValType_F64,
            },
        }
    }
}

impl From<ffi::WasmEdge_Value> for Value {
    fn from(value: ffi::WasmEdge_Value) -> Self {
        match value.Type {
            ffi::WasmEdge_ValType_I32 => Self::I32(value.Value as i32),
            ffi::WasmEdge_ValType_I64 => Self::I64(value.Value as i64),
            ffi::WasmEdge_ValType_F32 => Self::F32(f32::from_bits(value.Value as u32)),
            ffi::WasmEdge_ValType_F64 => Self::F64(f64::from_bits(value.Value as u64)),
            _ => panic!("unknown WasmEdge_ValType `{}`", value.Type),
        }
    }
}

macro_rules! impl_value_conversions {
    ($( $name:ident($ty:ty) ),+ $(,)?) => {
        $(
        impl From<$ty> for Value {
            fn from(value: $ty) -> Self {
                Self::$name(value)
            }
        }
        )+

        impl Value {
            paste::paste! {
                $(
                    #[doc = "Returns the `" $ty "` value, if that's the contained type."]
                    pub fn [<as_ $ty>](&self) -> Option<$ty> {
                        match self {
                            Self::$name(value) => Some(*value),
                            _ => None
                        }
                    }
                )+
            }
        }
    }
}

impl_value_conversions! {
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
}
