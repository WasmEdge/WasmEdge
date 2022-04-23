use crate::{types::ExternRef, FuncRef};
use std::marker::PhantomData;
use wasmedge_types::ValType;

pub trait NativeValType {
    const WASM_TYPE: ValType;
}

/// The `impl_native_val_type` macro is used to generate the following struct
///
/// ```ignore
/// impl NativeValType for i32 {
///     const WASM_TYPE: ValType = ValType::I32;
/// }
/// ```
macro_rules! impl_native_val_type {
    ($t:ty, $w:expr) => {
        impl NativeValType for $t {
            const WASM_TYPE: ValType = $w;
        }
    };
}

impl_native_val_type!(i8, ValType::I32);
impl_native_val_type!(u8, ValType::I32);
impl_native_val_type!(i16, ValType::I32);
impl_native_val_type!(u16, ValType::I32);
impl_native_val_type!(i32, ValType::I32);
impl_native_val_type!(u32, ValType::I64);
impl_native_val_type!(i64, ValType::I64);
impl_native_val_type!(f32, ValType::F32);
impl_native_val_type!(f64, ValType::F64);
impl_native_val_type!(u128, ValType::V128);
impl_native_val_type!(FuncRef, ValType::FuncRef);
impl_native_val_type!(ExternRef, ValType::ExternRef);

#[cfg(test)]
mod test_native_val_type {
    use super::*;

    #[test]
    fn test_i8() {
        assert_eq!(i8::WASM_TYPE, ValType::I32);
    }

    #[test]
    fn test_u8() {
        assert_eq!(u8::WASM_TYPE, ValType::I32);
    }

    #[test]
    fn test_i16() {
        assert_eq!(i16::WASM_TYPE, ValType::I32);
    }

    #[test]
    fn test_u16() {
        assert_eq!(u16::WASM_TYPE, ValType::I32);
    }

    #[test]
    fn test_i32() {
        assert_eq!(i32::WASM_TYPE, ValType::I32);
    }

    #[test]
    fn test_u32() {
        assert_eq!(u32::WASM_TYPE, ValType::I64);
    }

    #[test]
    fn test_i64() {
        assert_eq!(i64::WASM_TYPE, ValType::I64);
    }

    #[test]
    fn test_f32() {
        assert_eq!(f32::WASM_TYPE, ValType::F32);
    }

    #[test]
    fn test_f64() {
        assert_eq!(f64::WASM_TYPE, ValType::F64);
    }

    #[test]
    fn test_u128() {
        assert_eq!(u128::WASM_TYPE, ValType::V128);
    }

    #[test]
    fn test_func_ref() {
        assert_eq!(FuncRef::WASM_TYPE, ValType::FuncRef);
    }

    #[test]
    fn test_extern_ref() {
        assert_eq!(ExternRef::WASM_TYPE, ValType::ExternRef);
    }
}

pub trait ValTypeList
where
    Self: Sized,
{
    fn parameters() -> Vec<ValType> {
        Vec::new()
    }
}

/// The `impl_val_type_list` macro is used to generate the following code.
///
/// ```ignore
/// pub struct I1<O1: NativeValType> {
///     phantom_1: PhantomData<O1>,
/// }
///
/// impl<O1: NativeValType> ValTypeList for I1<O1> {
///     fn parameters() -> Vec<ValType> {
///         vec![O1::WASM_TYPE]
///     }
/// }
/// ```
macro_rules! impl_val_type_list {
    ($struct_name:ident, ($($o:ident),*)) => {
       #[allow(non_snake_case)]
        pub struct $struct_name<$($o: NativeValType),*> {
            $($o: PhantomData<$o>),*
        }

        impl<$($o: NativeValType),*> ValTypeList for $struct_name<$($o),*> {
            fn parameters() -> Vec<ValType> {
                vec![$($o::WASM_TYPE),*]
            }
        }
    }
}

impl_val_type_list!(I0, ());
impl_val_type_list!(I1, (O1));
impl_val_type_list!(I2, (O1, O2));
impl_val_type_list!(I3, (O1, O2, O3));
impl_val_type_list!(I4, (O1, O2, O3, O4));
impl_val_type_list!(I5, (O1, O2, O3, O4, O5));
impl_val_type_list!(I6, (O1, O2, O3, O4, O5, O6));
impl_val_type_list!(I7, (O1, O2, O3, O4, O5, O6, O7));
impl_val_type_list!(I8, (O1, O2, O3, O4, O5, O6, O7, O8));
impl_val_type_list!(I9, (O1, O2, O3, O4, O5, O6, O7, O8, O9));
impl_val_type_list!(I10, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10));
impl_val_type_list!(I11, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11));
impl_val_type_list!(I12, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12));
impl_val_type_list!(
    I13,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13)
);
impl_val_type_list!(
    I14,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14)
);
impl_val_type_list!(
    I15,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15)
);
impl_val_type_list!(
    I16,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16)
);
impl_val_type_list!(
    I17,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17)
);
impl_val_type_list!(
    I18,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18)
);
impl_val_type_list!(
    I19,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19)
);
impl_val_type_list!(
    I20,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20)
);
impl_val_type_list!(
    I21,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21
    )
);
impl_val_type_list!(
    I22,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22
    )
);
impl_val_type_list!(
    I23,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23
    )
);
impl_val_type_list!(
    I24,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24
    )
);
impl_val_type_list!(
    I25,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25
    )
);
impl_val_type_list!(
    I26,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26
    )
);
impl_val_type_list!(
    I27,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27
    )
);
impl_val_type_list!(
    I28,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28
    )
);
impl_val_type_list!(
    I29,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29
    )
);
impl_val_type_list!(
    I30,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30
    )
);
impl_val_type_list!(
    I31,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31
    )
);
impl_val_type_list!(
    I32,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31, O32
    )
);

#[cfg(test)]
mod test_val_type_list {
    use super::*;

    #[test]
    fn test_val_type_list() {
        assert_eq!(I0::parameters(), vec![]);
        assert_eq!(I1::<i8>::parameters(), vec![ValType::I32]);
        assert_eq!(I2::<i8, u8>::parameters(), vec![ValType::I32, ValType::I32]);
        assert_eq!(
            I4::<i16, u16, i32, u32>::parameters(),
            vec![ValType::I32, ValType::I32, ValType::I32, ValType::I64]
        );
        assert_eq!(
            I6::<i64, f32, f64, u128, FuncRef, ExternRef>::parameters(),
            vec![
                ValType::I64,
                ValType::F32,
                ValType::F64,
                ValType::V128,
                ValType::FuncRef,
                ValType::ExternRef
            ]
        );
    }
}
