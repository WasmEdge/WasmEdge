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
use std::{future::Future, os::raw::c_void};
use wasmedge_sys::{
    AsImport, AsyncBoxedFn, CallingFrame, Config, FuncType, Function, ImportModule, ImportObject,
    Loader, Store, Vm, WasmValue,
};
use wasmedge_types::{error::HostFuncError, ValType};

fn real_add(
    _store: &mut Store,
    _frame: &CallingFrame,
    input: Vec<WasmValue>,
    _data: *mut c_void,
) -> Box<(dyn Future<Output = Result<Vec<WasmValue>, HostFuncError>> + Send + 'static)> {
    Box::new(async move {
        println!("Rust: Entering Rust function real_add");

        if input.len() != 3 {
            return Err(HostFuncError::User(1));
        }

        let a = if input[1].ty() == ValType::I32 {
            input[1].to_i32()
        } else {
            1
        };

        let b = if input[2].ty() == ValType::I32 {
            input[2].to_i32()
        } else {
            2
        };
        tokio::time::sleep(std::time::Duration::from_secs(1)).await;

        let c = a + b;
        println!("Rust: calcuating in real_add c: {:?}", c);

        println!("Rust: Leaving Rust function real_add");
        Ok(vec![WasmValue::from_i32(c)])
    })
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut hostfunc_path = std::env::current_dir()?.join("funcs.wasm");

    if !hostfunc_path.exists() {
        // modify path for cargo test
        hostfunc_path = std::env::current_dir()?.join("examples/data/funcs.wasm");
    }

    // load module from file
    let config = Config::create()?;
    let loader = Loader::create(Some(config))?;
    let module = loader.from_file(hostfunc_path)?;

    // create a Vm context
    let config = Config::create()?;
    let mut vm = Vm::create(Some(config), None)?;
    let mut store = vm.store_mut().expect("no store");
    // let mut store = Store::create().expect("123");
    dbg!(&store);
    dbg!(&store.async_state.current_poll_cx.get());

    let result = FuncType::create(
        vec![ValType::ExternRef, ValType::I32, ValType::I32],
        vec![ValType::I32],
    );
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let add_ref = WasmValue::from_extern_ref(&mut real_add);

    let result = Function::create_async(&func_ty, Box::new(real_add), Some(&mut store), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();

    // create an ImportObject module
    let mut import = ImportModule::create("extern_module")?;
    import.add_func("add", host_func);
    vm.load_wasm_from_module(&module);
    for func in vm.function_iter() {
        dbg!(func);
    }
    println!("functions");
    vm.register_wasm_from_import(ImportObject::Import(import))?;

    let res = vm
        .run_wasm_from_module_async2(
            module,
            String::from("call_add"),
            vec![add_ref, WasmValue::from_i32(5), WasmValue::from_i32(10)],
        )
        .await?;

    dbg!(res);
    Ok(())
}
