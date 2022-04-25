use crate::{types::ExternRef, FuncRef};
use wasmedge_types::ValType;

/// Describes the mapping of Rust type to Wasm type.
///
/// ```ignore
/// use wasmedge::{WasmValType, ValType};
///
/// assert_eq!(i32::WASM_TYPE, ValType::I32);
/// ```
///
pub trait WasmValType {
    /// The Wasm type.
    const WASM_TYPE: ValType;
}

/// The `impl_native_val_type` macro is used to generate the following struct
///
/// ```ignore
/// impl NativeWasmType for i32 {
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
impl_wasm_val_type!(u128, ValType::V128);
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
        assert_eq!(u128::WASM_TYPE, ValType::V128);
        assert_eq!(FuncRef::WASM_TYPE, ValType::FuncRef);
        assert_eq!(ExternRef::WASM_TYPE, ValType::ExternRef);
    }
}

/// Describes the mapping of a tuple of Rust types to Wasm types.
///
/// ```ignore
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

// ====================

/// The `IntoResult` trait turns a `WasmTypeList` into a
/// `Result<WasmTypeList, Self::Error>`.
///
/// It is mostly used to turn result values of a Wasm function
/// call into a `Result`.
pub trait IntoResult<T>
where
    T: WasmValTypeList,
{
    /// The error type for this trait.
    type Error: std::error::Error + Sync + Send + 'static;

    /// Transforms `Self` into a `Result`.
    fn into_result(self) -> Result<T, Self::Error>;
}

impl<T> IntoResult<T> for T
where
    T: WasmValTypeList,
{
    // `T` is not a `Result`, it's already a value, so no error
    // can be built.
    type Error = core::convert::Infallible;

    fn into_result(self) -> Result<Self, core::convert::Infallible> {
        Ok(self)
    }
}

impl<T, E> IntoResult<T> for Result<T, E>
where
    T: WasmValTypeList,
    E: std::error::Error + Sync + Send + 'static,
{
    type Error = E;

    fn into_result(self) -> Self {
        self
    }
}

// #[cfg(test)]
// mod test_into_result {
//     use super::*;
//     use std::convert::Infallible;

//     #[test]
//     fn test_into_result_over_t() {
//         let x: i32 = 42;
//         let result_of_x: Result<i32, std::convert::Infallible> = x.into_result();

//         assert_eq!(result_of_x.unwrap(), x);
//     }

//     #[test]
//     fn test_into_result_over_result() {
//         {
//             let x: Result<i32, Infallible> = Ok(42);
//             let result_of_x: Result<i32, Infallible> = x.into_result();

//             assert_eq!(result_of_x, x);
//         }

//         {
//             use std::{error, fmt};

//             #[derive(Debug, PartialEq)]
//             struct E;

//             impl fmt::Display for E {
//                 fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
//                     write!(formatter, "E")
//                 }
//             }

//             impl error::Error for E {}

//             let x: Result<Infallible, E> = Err(E);
//             let result_of_x: Result<Infallible, E> = x.into_result();

//             assert_eq!(result_of_x.unwrap_err(), E);
//         }
//     }
// }

// /// The `HostFunction` trait represents the set of functions that
// /// can be used as host function. To uphold this statement, it is
// /// necessary for a function to be transformed into a pointer to
// /// `VMFunctionBody`.
// pub trait HostFunction<Args, Rets>
// where
//     Args: WasmValTypeList,
//     Rets: WasmValTypeList,
//     Self: Sized,
// {
//     // /// Get the pointer to the function body.
//     // fn function_body_ptr(self) -> *const VMFunctionBody;
// }

// macro_rules! impl_host_function {
//     ( [$c_struct_representation:ident]
//        $c_struct_name:ident,
//        $( $x:ident ),* ) => {

//         /// A structure with a C-compatible representation that can hold a set of Wasm values.
//         /// This type is used by `WasmTypeList::CStruct`.
//         #[repr($c_struct_representation)]
//         pub struct $c_struct_name< $( $x ),* > ( $( <$x as FromToNativeWasmType>::Native ),* )
//         where
//             $( $x: FromToNativeWasmType ),*;

//         // Implement `HostFunction` for a function that has the same arity than the tuple.
//         // This specific function has no environment.
//         #[allow(unused_parens)]
//         impl< $( $x, )* Rets, RetsAsResult, Func>
//             HostFunction<( $( $x ),* ), Rets>
//         for
//             Func
//         where
//             $( $x: WasmValType, )*
//             Rets: WasmTypeList,
//             RetsAsResult: IntoResult<Rets>,
//             Func: Fn($( $x , )*) -> RetsAsResult + 'static + Send,
//         {
//             // #[allow(non_snake_case)]
//             // fn function_body_ptr(self) -> *const VMFunctionBody {
//             //     /// This is a function that wraps the real host
//             //     /// function. Its address will be used inside the
//             //     /// runtime.
//             //     extern fn func_wrapper<$( $x, )* Rets, RetsAsResult, Func>( _: usize, $( $x: $x::Native, )* ) -> Rets::CStruct
//             //     where
//             //         $( $x: FromToNativeWasmType, )*
//             //         Rets: WasmTypeList,
//             //         RetsAsResult: IntoResult<Rets>,
//             //         Func: Fn( $( $x ),* ) -> RetsAsResult + 'static
//             //     {
//             //         let func: &Func = unsafe { &*(&() as *const () as *const Func) };
//             //         let result = on_host_stack(|| {
//             //             panic::catch_unwind(AssertUnwindSafe(|| {
//             //                 func( $( FromToNativeWasmType::from_native($x) ),* ).into_result()
//             //             }))
//             //         });

//             //         match result {
//             //             Ok(Ok(result)) => return result.into_c_struct(),
//             //             Ok(Err(trap)) => unsafe { raise_user_trap(Box::new(trap)) },
//             //             Err(panic) => unsafe { resume_panic(panic) },
//             //         }
//             //     }

//             //     func_wrapper::< $( $x, )* Rets, RetsAsResult, Self > as *const VMFunctionBody
//             // }
//         }
//     };
// }
