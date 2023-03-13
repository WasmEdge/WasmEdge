//!
//! ```bash
//! // go into the directory: bindings/rust
//! cargo run -p wasmedge-sys --example table_and_funcref -- --nocapture
//! ```

use wasmedge_macro::sys_host_function;
use wasmedge_sys::{
    AsImport, CallingFrame, Config, Executor, FuncType, Function, ImportModule, ImportObject,
    Store, Table, TableType, WasmValue,
};
use wasmedge_types::{error::HostFuncError, RefType, ValType};

#[sys_host_function]
fn real_add(_frame: CallingFrame, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Rust: Entering Rust function real_add");

    if input.len() != 2 {
        return Err(HostFuncError::User(1));
    }

    let a = if input[0].ty() == ValType::I32 {
        input[0].to_i32()
    } else {
        return Err(HostFuncError::User(2));
    };

    let b = if input[1].ty() == ValType::I32 {
        input[1].to_i32()
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
    // create a FuncType
    let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
    // create a host function
    let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;

    // create a TableType instance
    let ty = TableType::create(RefType::FuncRef, 10, Some(20))?;
    // create a Table instance
    let mut table = Table::create(&ty)?;
    // call set_data to store a function reference at the given index of the table instance
    table.set_data(WasmValue::from_func_ref(host_func.as_ref()), 3)?;

    // add the table instance to the import object
    let mut import = ImportModule::create("extern")?;
    import.add_table("my-table", table);

    // create a config
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);

    // create an executor
    let mut executor = Executor::create(Some(&config), None)?;

    // create a store
    let mut store = Store::create()?;

    // register the import object
    let import_obj = ImportObject::Import(import);
    executor.register_import_object(&mut store, &import_obj)?;

    //get the module instance named "extern"
    let instance = store.module("extern")?;

    // get the exported table named "my-table"
    let table = instance.get_table("my-table")?;
    // call get_data to recover the function reference from the value at the given index of the table instance
    let value = table.get_data(3)?;
    let result = value.func_ref();
    assert!(result.is_some());
    let func_ref = result.unwrap();

    // get the function type by func_ref
    let func_ty = func_ref.ty()?;
    assert_eq!(func_ty.params_len(), 2);
    let param_tys = func_ty.params_type_iter().collect::<Vec<_>>();
    assert_eq!(param_tys, [ValType::I32, ValType::I32]);
    assert_eq!(func_ty.returns_len(), 1);
    let return_tys = func_ty.returns_type_iter().collect::<Vec<_>>();
    assert_eq!(return_tys, [ValType::I32]);

    Ok(())
}
