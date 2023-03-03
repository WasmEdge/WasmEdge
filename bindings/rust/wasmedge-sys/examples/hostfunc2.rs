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

use wasmedge_macro::sys_host_function;
use wasmedge_sys::{
    AsImport, CallingFrame, Config, Executor, FuncType, Function, ImportModule, ImportObject,
    Loader, Store, Validator, WasmValue,
};
use wasmedge_types::{error::HostFuncError, wat2wasm, ValType};

#[sys_host_function]
fn real_add(_frame: CallingFrame, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Rust: Entering Rust function real_add");

    if input.len() != 3 {
        return Err(HostFuncError::User(1));
    }

    let a = if input[1].ty() == ValType::I32 {
        input[1].to_i32()
    } else {
        return Err(HostFuncError::User(2));
    };

    let b = if input[2].ty() == ValType::I32 {
        input[2].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    let c = a + b;
    println!("Rust: calcuating in real_add c: {c:?}");

    println!("Rust: Leaving Rust function real_add");
    Ok(vec![WasmValue::from_i32(c)])
}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let config = Config::create()?;
    let mut import = ImportModule::create("extern_module")?;

    let result = FuncType::create(
        vec![ValType::ExternRef, ValType::I32, ValType::I32],
        vec![ValType::I32],
    );
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(real_add), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    import.add_func("add", host_func);

    // create an executor
    let mut executor = Executor::create(Some(&config), None)?;

    // create a store
    let mut store = Store::create()?;

    let import_obj = ImportObject::Import(import);
    executor.register_import_object(&mut store, &import_obj)?;

    // load wasm from binary
    let wasm_bytes = wat2wasm(
        br#"
        (module
            (type (;0;) (func (param externref i32 i32) (result i32)))
            (import "extern_module" "add" (func (;0;) (type 0)))
            (func (;1;) (type 0) (param externref i32 i32) (result i32)
              local.get 0
              local.get 1
              local.get 2
              call 0)
            (memory (;0;) 1)
            (export "call_add" (func 1))
            (export "memory" (memory 0)))
    "#,
    )?;
    let loader = Loader::create(Some(&config))?;
    let module = loader.from_bytes(wasm_bytes)?;
    Validator::create(Some(&config))?.validate(&module)?;
    let active_instance = executor.register_active_module(&mut store, &module)?;
    let call_add = active_instance.get_func("call_add")?;

    let add_ref = WasmValue::from_extern_ref(&mut real_add);
    let returns = executor.call_func(
        &call_add,
        [
            add_ref,
            WasmValue::from_i32(1234),
            WasmValue::from_i32(5678),
        ],
    )?;
    assert_eq!(returns[0].to_i32(), 1234 + 5678);

    Ok(())
}
