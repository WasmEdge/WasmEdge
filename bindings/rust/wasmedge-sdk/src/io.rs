use crate::{types::ExternRef, FuncRef, ValType, WasmValue};

/// Describes the mapping of Rust type to Wasm type.
///
/// ```rust
/// use wasmedge_sdk::{WasmValType, ValType};
///
/// assert_eq!(i32::WASM_TYPE, ValType::I32);
/// ```
///
pub trait WasmValType {
    /// The Wasm type.
    const WASM_TYPE: ValType;
}

/// The `impl_wasm_val_type` macro is used to generate the following struct
///
/// ```ignore
/// impl WasmValType for i32 {
///     const WASM_TYPE: ValType = ValType::I32;
/// }
/// ```
macro_rules! impl_wasm_val_type {
    ($t:ty, $w:expr) => {
        impl WasmValType for $t {
            const WASM_TYPE: ValType = $w;
        }
    };
}

impl_wasm_val_type!(i8, ValType::I32);
impl_wasm_val_type!(u8, ValType::I32);
impl_wasm_val_type!(i16, ValType::I32);
impl_wasm_val_type!(u16, ValType::I32);
impl_wasm_val_type!(i32, ValType::I32);
impl_wasm_val_type!(u32, ValType::I64);
impl_wasm_val_type!(i64, ValType::I64);
impl_wasm_val_type!(f32, ValType::F32);
impl_wasm_val_type!(f64, ValType::F64);
impl_wasm_val_type!(i128, ValType::V128);
impl_wasm_val_type!(FuncRef, ValType::FuncRef);
impl_wasm_val_type!(ExternRef, ValType::ExternRef);

#[cfg(test)]
mod test_wasm_val_type {
    use super::*;

    #[test]
    fn test_wasm_types() {
        assert_eq!(i8::WASM_TYPE, ValType::I32);
        assert_eq!(u8::WASM_TYPE, ValType::I32);
        assert_eq!(i16::WASM_TYPE, ValType::I32);
        assert_eq!(u16::WASM_TYPE, ValType::I32);
        assert_eq!(i32::WASM_TYPE, ValType::I32);
        assert_eq!(u32::WASM_TYPE, ValType::I64);
        assert_eq!(i64::WASM_TYPE, ValType::I64);
        assert_eq!(f32::WASM_TYPE, ValType::F32);
        assert_eq!(f64::WASM_TYPE, ValType::F64);
        assert_eq!(i128::WASM_TYPE, ValType::V128);
        assert_eq!(FuncRef::WASM_TYPE, ValType::FuncRef);
        assert_eq!(ExternRef::WASM_TYPE, ValType::ExternRef);
    }
}

/// Describes the mapping of a tuple of Rust types to Wasm types.
///
/// ```rust
/// use wasmedge_sdk::{FuncRef, types::ExternRef, ValType, WasmValTypeList};
///
/// assert_eq!(
///      <(i32, i64, f32, f64, FuncRef, ExternRef)>::wasm_types(),
///      [ValType::I32, ValType::I64, ValType::F32, ValType::F64, ValType::FuncRef, ValType::ExternRef]
/// );
/// ```
pub trait WasmValTypeList
where
    Self: Sized,
{
    /// The array type that can hold all the represented values.
    ///
    /// Note that all values are stored in their binary form.
    type Array: AsMut<[i128]>;

    /// Get the Wasm types for the tuple (list) of currently
    /// represented values.
    fn wasm_types() -> &'static [ValType];
}

macro_rules! impl_wasm_val_type_list {
    ( $($o:ident),* ) => {
        #[allow(unused_parens, dead_code)]
        impl< $( $o ),* >
            WasmValTypeList
        for ( $( $o ),* )
        where
            $( $o: WasmValType ),*
        {
            type Array = [i128; count_idents!( $( $o ),* )];

            fn wasm_types() -> &'static [ValType] {
                &[
                    $(
                        $o::WASM_TYPE
                    ),*
                ]
            }
        }
    };
}

// Count the number of identifiers at compile-time.
macro_rules! count_idents {
    ( $($idents:ident),* ) => {
        {
            #[allow(dead_code, non_camel_case_types)]
            enum Idents { $( $idents, )* __CountIdentsLast }
            const COUNT: usize = Idents::__CountIdentsLast as usize;
            COUNT
        }
    };
}

impl_wasm_val_type_list!();
impl_wasm_val_type_list!(A1);
impl_wasm_val_type_list!(A1, A2);
impl_wasm_val_type_list!(A1, A2, A3);
impl_wasm_val_type_list!(A1, A2, A3, A4);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15);
impl_wasm_val_type_list!(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27, A28
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27, A28, A29
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27, A28, A29, A30
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27, A28, A29, A30, A31
);
impl_wasm_val_type_list!(
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20, A21,
    A22, A23, A24, A25, A26, A27, A28, A29, A30, A31, A32
);

#[cfg(test)]
mod test_wasm_val_type_list {
    use super::*;

    #[test]
    fn test_wasm_types_for_multi_values() {
        assert_eq!(<()>::wasm_types(), []);
        assert_eq!(<(i32, i32)>::wasm_types(), [ValType::I32, ValType::I32]);
        assert_eq!(<(i64, i64)>::wasm_types(), [ValType::I64, ValType::I64]);
        assert_eq!(<(f32, f32)>::wasm_types(), [ValType::F32, ValType::F32]);
        assert_eq!(<(f64, f64)>::wasm_types(), [ValType::F64, ValType::F64]);
        assert_eq!(
            <(i32, i64, f32, f64, FuncRef, ExternRef)>::wasm_types(),
            [
                ValType::I32,
                ValType::I64,
                ValType::F32,
                ValType::F64,
                ValType::FuncRef,
                ValType::ExternRef,
            ]
        );
        assert_eq!(
            <(FuncRef, ExternRef)>::wasm_types(),
            [ValType::FuncRef, ValType::ExternRef]
        );
    }
}

/// Defines the function converting a value of Rust type to the one of Wasm type.
pub trait WasmVal {
    fn to_wasm_value(self) -> WasmValue;
}
impl WasmVal for i8 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i32(self as i32)
    }
}
impl WasmVal for u8 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i32(self as i32)
    }
}
impl WasmVal for i16 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i32(self as i32)
    }
}
impl WasmVal for u16 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i32(self as i32)
    }
}
impl WasmVal for i32 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i32(self)
    }
}
impl WasmVal for u32 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i64(self as i64)
    }
}
impl WasmVal for i64 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_i64(self)
    }
}
impl WasmVal for f32 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_f32(self)
    }
}
impl WasmVal for f64 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_f64(self)
    }
}
impl WasmVal for i128 {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_v128(self)
    }
}
impl WasmVal for FuncRef {
    fn to_wasm_value(self) -> WasmValue {
        WasmValue::from_func_ref(self.inner)
    }
}
impl WasmVal for ExternRef {
    fn to_wasm_value(self) -> WasmValue {
        self.inner
    }
}

/// Used to pass arguments to the following methods:
/// - [Func::run](crate::Func::run)
/// - [Func::run_async](crate::Func::run_async)
/// - [FuncRef::run](crate::FuncRef::run)
/// - [FuncRef::run_async](crate::FuncRef::run_async)
/// - [Executor::run_func](crate::Executor::run_func)
/// - [Executor::run_func_async](crate::Executor::run_func_async)
/// - [Executor::run_func_ref](crate::Executor::run_func_ref)
/// - [Executor::run_func_ref_async](crate::Executor::run_func_ref_async)
/// - [Vm::run_func](crate::Vm::run_func)
/// - [Vm::run_func_async](crate::Vm::run_func_async)
///
/// Notice that to use the macro, it is required to use `WasmVal` trait. If the version of rust used is less than v1.63, please place `#![feature(explicit_generic_args_with_impl_trait)]` on the root of the program.
#[macro_export]
macro_rules! params {
    ( $( $x:expr ),* ) => {
        {
            let mut temp_vec = vec![];
            $(
                temp_vec.push($x.to_wasm_value());

            )*
            temp_vec
        }
    };
}
