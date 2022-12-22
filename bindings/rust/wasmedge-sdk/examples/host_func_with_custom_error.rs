//! In this example, we'll present how to define a custom error type and use it in our host function.
//! To run the example, just follow the commands below
//!
//! ```bash
//! cd wasmedge-sdk/
//! cargo run --example host_func_with_custom_error
//! ```
//!
//! After running the example, you should see the following output:
//!
//! ```bash
//! User error: OutOfUpperBound
//! ```
//! which means that the one or two input numbers of i32 type of the host function are out of upper bound (here is 100).

use thiserror::Error;
use wasmedge_sdk::{
    config::ConfigBuilder,
    error::{HostFuncError, WasmEdgeError},
    host_function, params,
    types::ExternRef,
    Caller, ImportObjectBuilder, ValType, Vm, WasmVal, WasmValue,
};

// Define custom error type
#[derive(Error, Clone, Debug, PartialEq, Eq)]
enum MyError {
    #[error("The number of arguments should be 3")]
    InvalidArguments,
    #[error("The type of the first argument should be i32")]
    InvalidFirstArgumentType,
    #[error("The type of the second argument should be i32")]
    InvalidSecondArgumentType,
    #[error("The value of argument is out of upper bound.")]
    OutOfUpperBound,
}
impl From<MyError> for u32 {
    fn from(err: MyError) -> Self {
        match err {
            MyError::InvalidArguments => 1,
            MyError::InvalidFirstArgumentType => 2,
            MyError::InvalidSecondArgumentType => 3,
            MyError::OutOfUpperBound => 4,
        }
    }
}
impl From<u32> for MyError {
    fn from(err: u32) -> Self {
        match err {
            1 => MyError::InvalidArguments,
            2 => MyError::InvalidFirstArgumentType,
            3 => MyError::InvalidSecondArgumentType,
            4 => MyError::OutOfUpperBound,
            x => panic!("Unknown error code: {x}"),
        }
    }
}

// compute the sum of two numbers, which are less than 100
#[host_function]
fn real_add(_caller: Caller, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    if input.len() != 3 {
        return Err(HostFuncError::User(MyError::InvalidArguments.into()));
    }

    let a = if input[1].ty() == ValType::I32 {
        let a = input[1].to_i32();
        match a >= 100 {
            true => return Err(HostFuncError::User(MyError::OutOfUpperBound.into())),
            false => a,
        }
    } else {
        return Err(HostFuncError::User(
            MyError::InvalidFirstArgumentType.into(),
        ));
    };

    let b = if input[2].ty() == ValType::I32 {
        let b = input[2].to_i32();
        match b >= 100 {
            true => return Err(HostFuncError::User(MyError::OutOfUpperBound.into())),
            false => b,
        }
    } else {
        return Err(HostFuncError::User(
            MyError::InvalidSecondArgumentType.into(),
        ));
    };

    let c = a + b;

    Ok(vec![WasmValue::from_i32(c)])
}

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    // create import module
    let import = ImportObjectBuilder::new()
        .with_func::<(ExternRef, i32, i32), i32>("add", real_add)?
        .build("extern_module")?;

    // create a vm instance
    let config = ConfigBuilder::default().build()?;
    let vm = Vm::new(Some(config))?.register_import_module(import)?;

    // run the export wasm function named "call_add" from func.wasm
    let wasm_file = std::env::current_dir()?.join("examples/data/funcs.wasm");
    let add_ref = ExternRef::new(&mut real_add);
    let a: i32 = 1234;
    let b: i32 = 5678;
    match vm.run_func_from_file(wasm_file, "call_add", params!(add_ref, a, b)) {
        Ok(returns) => {
            let ret = returns[0].to_i32();
            assert_eq!(ret, 1234 + 5678);
            println!("result from call_add: {ret}")
        }
        Err(e) => match *e {
            WasmEdgeError::User(code) => println!("User error: {:?}", MyError::from(code)),
            err => println!("Runtime error: {err:?}"),
        },
    };

    Ok(())
}
