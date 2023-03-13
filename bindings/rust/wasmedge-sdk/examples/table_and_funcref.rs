// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    error::HostFuncError,
    host_function, params,
    types::Val,
    Caller, Executor, Func, ImportObjectBuilder, RefType, Store, Table, TableType, ValType,
    WasmVal, WasmValue,
};

#[host_function]
fn real_add(_: Caller, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
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
    // create an executor
    let config = ConfigBuilder::new(CommonConfigOptions::default()).build()?;
    let mut executor = Executor::new(Some(&config), None)?;

    // create a store
    let mut store = Store::new()?;

    // create a table instance
    let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
    assert!(result.is_ok());
    let table = result.unwrap();

    // create an import object
    let import = ImportObjectBuilder::new()
        .with_table("my-table", table)?
        .build("extern")?;

    // register the import object into the store
    store.register_import_module(&mut executor, &import)?;

    // get the imported module instance
    let instance = store
        .named_instance("extern")
        .expect("Not found module instance named 'extern'");

    // get the exported table instance
    let mut table = instance
        .table("my-table")
        .expect("Not found table instance named 'my-table'");

    // create a host function
    let host_func = Func::wrap::<(i32, i32), i32>(Box::new(real_add))?;

    // store the reference to host_func at the given index of the table instance
    table.set(3, Val::FuncRef(Some(host_func.as_ref())))?;

    // retrieve the function reference at the given index of the table instance
    let value = table.get(3)?;
    if let Val::FuncRef(Some(func_ref)) = value {
        // get the function type by func_ref
        let func_ty = func_ref.ty();

        // arguments
        assert_eq!(func_ty.args_len(), 2);
        let param_tys = func_ty.args().unwrap();
        assert_eq!(param_tys, [ValType::I32, ValType::I32]);

        // returns
        assert_eq!(func_ty.returns_len(), 1);
        let return_tys = func_ty.returns().unwrap();
        assert_eq!(return_tys, [ValType::I32]);

        // call the function by func_ref
        let returns = func_ref.run(&executor, params!(1, 2))?;
        assert_eq!(returns.len(), 1);
        assert_eq!(returns[0].to_i32(), 3);
    }

    Ok(())
}
