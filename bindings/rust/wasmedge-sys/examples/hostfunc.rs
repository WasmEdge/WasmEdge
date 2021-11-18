use std::{ffi::CString, os::unix::ffi::OsStrExt};

use wasmedge_sys::{instance::Function, Config, ImportObj, Module, Value, Vm, I1, I2};

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

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut hostfunc_path = std::env::current_dir()?.join("funcs.wasm");

    if !hostfunc_path.exists() {
        // modify path for cargo test
        hostfunc_path = std::env::current_dir()?.join("examples/funcs.wasm");
    }

    let config = Config::default();
    let mut import_obj = ImportObj::create("extern_module");

    let mut host_func = Function::create_bindings::<I2<i32, i32>, I1<i32>>(Box::new(real_add));
    import_obj.add_func("add", &mut host_func);

    let path_cstr = CString::new(hostfunc_path.as_os_str().as_bytes().to_vec())?;
    let module = Module::load_from_file(&config, path_cstr).expect("funcs.wasm should be correct");

    let mut vm = Vm::create(&config)
        .register_module_from_import(import_obj)
        .expect("import_obj should be regiestered")
        .load_wasm_from_ast_module(&module)
        .expect("funcs.wasm should be loaded")
        .validate()
        .expect("fail to validate vm")
        .instantiate()
        .expect("fail to instantiate vm");

    #[allow(clippy::type_complexity)]
    fn boxed_fn() -> Box<dyn Fn(Vec<Value>) -> Result<Vec<Value>, u8>> {
        Box::new(real_add)
    }

    let add_ref = Value::from(boxed_fn());

    match vm.run("call_add", &[add_ref, 1234i32.into(), 5678i32.into()]) {
        Ok(v) => println!("result from call_add: {:?}", v),
        Err(r) => println!("error from call_add{:?}", r),
    };
    Ok(())
}
