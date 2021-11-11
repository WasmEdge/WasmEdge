use std::marker::PhantomData;

use crate::Value;

pub trait WasmFnIO {
    fn parameters() -> Vec<Value>
    where
        Self: Sized,
    {
        Vec::new()
    }
}

pub struct I1<A> {
    phantom_a: PhantomData<A>,
}

impl<A> WasmFnIO for I1<A> {
    fn parameters() -> Vec<Value> {
        match std::any::type_name::<A>() {
            "i32" => vec![Value::I32(0)],
            // TODO
            _ => panic!("unsupport type for WasmFnIO"),
        }
    }
}

pub struct I2<A, B> {
    phantom_a: PhantomData<A>,
    phantom_b: PhantomData<B>,
}

impl<A, B> WasmFnIO for I2<A, B> {
    fn parameters() -> Vec<Value> {
        match (std::any::type_name::<A>(), std::any::type_name::<B>()) {
            ("i32", "i32") => vec![Value::I32(0), Value::I32(0)],
            // TODO
            _ => panic!("unsupport type for WasmFnIO"),
        }
    }
}
