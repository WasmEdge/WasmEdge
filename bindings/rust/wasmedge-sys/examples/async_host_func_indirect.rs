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
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_host_func_indirect
//! ```

#[cfg(feature = "async")]
use wasmedge_macro::sys_async_host_function;
#[cfg(feature = "async")]
use wasmedge_sys::{
    AsImport, CallingFrame, Config, FuncType, Function, ImportModule, ImportObject, Loader, Store,
    Vm, WasmValue,
};
#[cfg(feature = "async")]
use wasmedge_types::{error::HostFuncError, wat2wasm, ValType};

#[cfg(feature = "async")]
#[sys_async_host_function]
async fn real_add(
    _frame: CallingFrame,
    input: Vec<WasmValue>,
) -> Result<Vec<WasmValue>, HostFuncError> {
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
    tokio::time::sleep(std::time::Duration::from_secs(4)).await;

    let c = a + b;
    println!("Rust: calcuating in real_add c: {:?}", c);

    println!("Rust: Leaving Rust function real_add");
    Ok(vec![WasmValue::from_i32(c)])
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "async")]
    {
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

        // load module from file
        let config = Config::create()?;
        let loader = Loader::create(Some(config))?;
        let module = loader.from_bytes(&wasm_bytes)?;

        // create a Vm context
        let config = Config::create()?;
        let mut store = Store::create()?;
        let mut vm = Vm::create(Some(config), Some(&mut store))?;

        let func_ty = FuncType::create(
            vec![ValType::ExternRef, ValType::I32, ValType::I32],
            vec![ValType::I32],
        )?;
        let add_ref = WasmValue::from_extern_ref(&mut real_add);
        let host_func = Function::create_async(&func_ty, Box::new(real_add), 0)?;

        // create an ImportObject module
        let mut import = ImportModule::create("extern_module")?;
        import.add_func("add", host_func);
        vm.load_wasm_from_module(&module)?;
        vm.register_wasm_from_import(ImportObject::Import(import))?;

        tokio::spawn(async move {
            let res = vm
                .run_wasm_from_module_async(
                    module,
                    String::from("call_add"),
                    vec![add_ref, WasmValue::from_i32(5), WasmValue::from_i32(10)],
                )
                .await
                .unwrap();
            assert_eq!(res[0].to_i32(), 15);
        })
        .await?;
    }

    println!("main thread");
    Ok(())
}
