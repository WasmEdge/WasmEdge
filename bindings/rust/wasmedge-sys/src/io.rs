use crate::WasmValue;
use std::marker::PhantomData;

fn match_value(s: &str) -> WasmValue {
    match s {
        "i8" => WasmValue::from_i32(0),
        "u8" => WasmValue::from_i32(0),
        "i16" => WasmValue::from_i32(0),
        "u16" => WasmValue::from_i32(0),
        "i32" => WasmValue::from_i32(0),
        "u32" => WasmValue::from_i64(0),
        "i64" => WasmValue::from_i64(0),
        "f32" => WasmValue::from_f32(0.),
        "f64" => WasmValue::from_f64(0.),
        "u128" => WasmValue::from_v128(0),
        _ => panic!("unsupported type for WasmFnIO"),
    }
}
pub trait WasmFnIO {
    fn parameters() -> Vec<WasmValue>
    where
        Self: Sized,
    {
        Vec::new()
    }
}

macro_rules! factory_io {
    ($i:ident, ($($o:ident),+)) => {
        #[allow(non_snake_case)]
        pub struct $i<$($o),+> {
            $($o: PhantomData<$o>),+
        }

        impl<$($o),+> WasmFnIO for $i<$($o),+> {
            fn parameters() -> Vec<WasmValue> {
                vec![$(match_value(std::any::type_name::<$o>())),+]
            }
        }
    }
}

factory_io!(I1, (O1));
factory_io!(I2, (O1, O2));
factory_io!(I3, (O1, O2, O3));
factory_io!(I4, (O1, O2, O3, O4));
factory_io!(I5, (O1, O2, O3, O4, O5));
factory_io!(I6, (O1, O2, O3, O4, O5, O6));
factory_io!(I7, (O1, O2, O3, O4, O5, O6, O7));
factory_io!(I8, (O1, O2, O3, O4, O5, O6, O7, O8));
factory_io!(I9, (O1, O2, O3, O4, O5, O6, O7, O8, O9));
factory_io!(I10, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10));
factory_io!(I11, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11));
factory_io!(I12, (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12));
factory_io!(
    I13,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13)
);
factory_io!(
    I14,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14)
);
factory_io!(
    I15,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15)
);
factory_io!(
    I16,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16)
);
factory_io!(
    I17,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17)
);
factory_io!(
    I18,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18)
);
factory_io!(
    I19,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19)
);
factory_io!(
    I20,
    (O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20)
);
factory_io!(
    I21,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21
    )
);
factory_io!(
    I22,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22
    )
);
factory_io!(
    I23,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23
    )
);
factory_io!(
    I24,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24
    )
);
factory_io!(
    I25,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25
    )
);
factory_io!(
    I26,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26
    )
);
factory_io!(
    I27,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27
    )
);
factory_io!(
    I28,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28
    )
);
factory_io!(
    I29,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29
    )
);
factory_io!(
    I30,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30
    )
);
factory_io!(
    I31,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31
    )
);
factory_io!(
    I32,
    (
        O1, O2, O3, O4, O5, O6, O7, O8, O9, O10, O11, O12, O13, O14, O15, O16, O17, O18, O19, O20,
        O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31, O32
    )
);
