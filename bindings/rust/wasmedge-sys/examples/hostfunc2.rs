//! An example help you using host function for WasmEdge
//!
//! In this example, the `real_add` is a function that run on host, and the WasmEdge VM step up in
//! main function, and the `real_add` registry as an `add` function in an `extern_module`, then
//! the main function call `call_add`, which do nothing just passing the extern reference and the
//! parameters of add function to the `real_add` function.
//!
//! The inputs and outputs of real host function are the `Vec<Value>`, which are the primitive type
//! for WasmEdge, and the host function for registration should be the return value from the
//! generics of `Function::create_bindings::<I, O>`, wherein the I and O are the `WasmFnIO` traits
//! base on the inputs and outputs of the real host function.
//!

use std::{
    fs::{self, File},
    io::Read,
};
use wasmedge_sys::{Config, FuncType, Function, ImportObj, Loader, ValType, Value, Vm};

fn real_add(input: Vec<Value>) -> Result<Vec<Value>, u8> {
    println!("Rust: Entering Rust function real_add");

    if input.len() != 2 {
        return Err(1);
    }

    let a = if let Value::I32(i) = input[0] {
        i
    } else {
        return Err(2);
    };

    let b = if let Value::I32(i) = input[1] {
        i
    } else {
        return Err(3);
    };

    let c = a + b;
    println!("Rust: calcuating in real_add c: {:?}", c);

    println!("Rust: Leaving Rust function real_add");
    Ok(vec![Value::I32(c)])
}

fn load_file_as_byte_vec(filename: &str) -> Vec<u8> {
    let mut f = File::open(&filename).expect("no file found");
    let metadata = fs::metadata(&filename).expect("unable to read metadata");
    let mut buffer = vec![0; metadata.len() as usize];
    f.read_exact(&mut buffer)
        .expect("buffer should be the same size");
    buffer
}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut hostfunc_path = std::env::current_dir()?.join("funcs.wasm");

    if !hostfunc_path.exists() {
        // modify path for cargo test
        hostfunc_path = std::env::current_dir()?.join("examples/funcs.wasm");
    }
    let wasm_binary = load_file_as_byte_vec(&hostfunc_path.as_path().display().to_string());

    let config = Config::create().expect("fail to create Config instance");
    let mut import_obj = ImportObj::create("extern_module").unwrap();

    let result = FuncType::create(
        vec![ValType::ExternRef, ValType::I32, ValType::I32],
        vec![ValType::I32],
    );
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(real_add), 0);
    assert!(result.is_ok());
    let mut host_func = result.unwrap();
    import_obj.add_func("add", &mut host_func);

    // load wasm from binary
    let loader = Loader::create(Some(&config))?;
    let mut module = loader.from_buffer(&wasm_binary)?;

    let mut vm = Vm::create(Some(&config), None)?;
    vm.register_wasm_from_import(&mut import_obj)?;

    #[allow(clippy::type_complexity)]
    fn boxed_fn() -> Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>> {
        Box::new(real_add)
    }

    let add_ref = Value::from(boxed_fn());

    match vm.run_wasm_from_module(
        &mut module,
        "call_add",
        [add_ref, 1234i32.into(), 5678i32.into()],
    ) {
        Ok(v) => println!("result from call_add: {:?}", v),
        Err(r) => println!("error from call_add{:?}", r),
    };
    Ok(())
}
