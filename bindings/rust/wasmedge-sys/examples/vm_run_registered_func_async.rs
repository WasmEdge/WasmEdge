//! This example presents a host function runs timeout, then cancel the task.

use wasmedge_sys::{
    AsImport, CallingFrame, Config, FuncType, Function, ImportModule, ImportObject, Vm, WasmValue,
};
use wasmedge_types::{error::HostFuncError, ValType};

// A native function
fn real_add(_: &CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    if inputs.len() != 2 {
        return Err(HostFuncError::User(1));
    }

    let a = if inputs[0].ty() == ValType::I32 {
        inputs[0].to_i32()
    } else {
        return Err(HostFuncError::User(2));
    };

    let b = if inputs[1].ty() == ValType::I32 {
        inputs[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    // simulate a long running operation
    std::thread::sleep(std::time::Duration::from_millis(1_000));

    let c = a + b;

    Ok(vec![WasmValue::from_i32(c)])
}

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());
    // enable Interruptible option
    config.interruptible(true);
    assert!(config.interruptible_enabled());

    // create a Vm context with the given Config and Store
    let mut vm = Vm::create(Some(config), None)?;

    // create import module
    let mut import = ImportModule::create("extern")?;

    // add host function
    let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
    let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
    import.add_func("add", host_func);

    // register the import_obj module
    vm.register_wasm_from_import(ImportObject::Import(import))?;

    // async run the host function
    let async_result = vm.run_registered_function_async(
        "extern",
        "add",
        [WasmValue::from_i32(2), WasmValue::from_i32(3)],
    )?;

    // set the timeout
    let ok = async_result.wait_for(900);
    assert!(!ok);

    // get the result returned by the host function
    let returns = async_result.get_async()?;
    assert_eq!(returns.len(), 1);
    assert_eq!(returns[0].to_i32(), 5);

    Ok(())
}
