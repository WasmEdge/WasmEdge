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

use wasmedge_sys::{Config, FuncType, Function, ImportObj, Loader, ValType, Value, Vm};

fn real_add(input: Vec<Value>) -> Result<Vec<Value>, u8> {
    println!("Rust: Entering Rust function real_add");

    if input.len() != 2 {
        return Err(1);
    }

    let a = if input[0].ty() == ValType::I32 {
        input[0].to_i32()
    } else {
        return Err(2);
    };

    let b = if input[1].ty() == ValType::I32 {
        input[0].to_i32()
    } else {
        return Err(3);
    };

    let c = a + b;
    println!("Rust: calcuating in real_add c: {:?}", c);

    println!("Rust: Leaving Rust function real_add");
    Ok(vec![Value::from_i32(c)])
}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut hostfunc_path = std::env::current_dir()?.join("funcs.wasm");

    if !hostfunc_path.exists() {
        // modify path for cargo test
        hostfunc_path = std::env::current_dir()?.join("examples/funcs.wasm");
    }

    let config = Config::create().expect("fail to create Config instance");
    let mut import_obj =
        ImportObj::create("extern_module").expect("fail to create ImportObj instance");

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

    // load module from file
    let loader = Loader::create(Some(&config))?;
    let mut module = loader.from_file(hostfunc_path)?;

    let mut vm = Vm::create(Some(&config), None)?;
    vm.register_wasm_from_import(&mut import_obj)?;

    let add_ref = Value::from_extern_ref(&mut real_add);
    match vm.run_wasm_from_module(
        &mut module,
        "call_add",
        [add_ref, Value::from_i32(1234), Value::from_i32(5678)],
    ) {
        Ok(v) => println!("result from call_add: {:?}", v),
        Err(r) => println!("error from call_add{:?}", r),
    };

    Ok(())
}
