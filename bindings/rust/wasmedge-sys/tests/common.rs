use wasmedge_sys::{AsImport, CallingFrame, FuncType, Function, ImportModule, WasmValue};
use wasmedge_types::{error::HostFuncError, ValType};

pub fn create_extern_module(name: impl AsRef<str>) -> ImportModule {
    // create an import module
    let result = ImportModule::create(name);
    assert!(result.is_ok());
    let mut import = result.unwrap();

    // add host function: "func-add"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_add), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    import.add_func("func-add", host_func);

    // add host function: "func-sub"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_sub), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    import.add_func("func-sub", host_func);

    // add host function: "func-mul"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_mul), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    import.add_func("func-mul", host_func);

    // add host function: "func-div"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_div), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();
    import.add_func("func-div", host_func);

    // add host function: "func-term"
    let result = FuncType::create([], [ValType::I32]);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_term), 0);
    let host_func = result.unwrap();
    import.add_func("func-term", host_func);

    // add host function: "func-fail"
    let result = FuncType::create([], [ValType::I32]);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(&func_ty, Box::new(extern_fail), 0);
    let host_func = result.unwrap();
    import.add_func("func-fail", host_func);

    import
}

fn extern_add(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    let val1 = if inputs[0].ty() == ValType::ExternRef {
        inputs[0]
    } else {
        return Err(HostFuncError::User(2));
    };
    let val1 = val1
        .extern_ref::<i32>()
        .expect("fail to get i32 from an ExternRef");

    let val2 = if inputs[1].ty() == ValType::I32 {
        inputs[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    Ok(vec![WasmValue::from_i32(val1 + val2)])
}

fn extern_sub(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    let val1 = if inputs[0].ty() == ValType::ExternRef {
        inputs[0]
    } else {
        return Err(HostFuncError::User(2));
    };

    let val1 = val1
        .extern_ref::<i32>()
        .expect("fail to get i32 from an ExternRef");

    let val2 = if inputs[1].ty() == ValType::I32 {
        inputs[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    Ok(vec![WasmValue::from_i32(val1 - val2)])
}

fn extern_mul(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    let val1 = if inputs[0].ty() == ValType::ExternRef {
        inputs[0]
    } else {
        return Err(HostFuncError::User(2));
    };
    let val1 = val1
        .extern_ref::<i32>()
        .expect("fail to get i32 from an ExternRef");

    let val2 = if inputs[1].ty() == ValType::I32 {
        inputs[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    Ok(vec![WasmValue::from_i32(val1 * val2)])
}

fn extern_div(_: CallingFrame, inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    let val1 = if inputs[0].ty() == ValType::ExternRef {
        inputs[0]
    } else {
        return Err(HostFuncError::User(2));
    };
    let val1 = val1
        .extern_ref::<i32>()
        .expect("fail to get i32 from an ExternRef");

    let val2 = if inputs[1].ty() == ValType::I32 {
        inputs[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    Ok(vec![WasmValue::from_i32(val1 / val2)])
}

fn extern_term(_: CallingFrame, _inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(1234)])
}

fn extern_fail(_: CallingFrame, _inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    Err(HostFuncError::User(2))
}
