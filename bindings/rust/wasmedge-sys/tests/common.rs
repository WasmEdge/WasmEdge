use wasmedge_sys::{FuncType, Function, ImportObj, ValType, Value};

pub fn create_extern_module(name: impl AsRef<str>) -> ImportObj {
    // create an ImportObj module
    let result = ImportObj::create(name);
    assert!(result.is_ok());
    let mut import_obj = result.unwrap();

    // add host function: "func-add"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_add), 0);
    assert!(result.is_ok());
    let mut host_func = result.unwrap();
    import_obj.add_func("func-add", &mut host_func);

    // add host function: "func-sub"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_sub), 0);
    assert!(result.is_ok());
    let mut host_func = result.unwrap();
    import_obj.add_func("func-sub", &mut host_func);

    // add host function: "func-mul"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_mul), 0);
    assert!(result.is_ok());
    let mut host_func = result.unwrap();
    import_obj.add_func("func-mul", &mut host_func);

    // add host function: "func-div"
    let result = FuncType::create(vec![ValType::ExternRef, ValType::I32], vec![ValType::I32]);
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_div), 0);
    assert!(result.is_ok());
    let mut host_func = result.unwrap();
    import_obj.add_func("func-div", &mut host_func);

    // add host function: "func-term"
    let result = FuncType::create([], [ValType::I32]);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_term), 0);
    let mut host_func = result.unwrap();
    import_obj.add_func("func-term", &mut host_func);

    // add host function: "func-fail"
    let result = FuncType::create([], [ValType::I32]);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    let result = Function::create(func_ty, Box::new(extern_fail), 0);
    let mut host_func = result.unwrap();
    import_obj.add_func("func-fail", &mut host_func);

    import_obj
}

fn _real_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    if inputs.len() != 2 {
        return Err(1);
    }

    let a = if let Value::I32(i) = inputs[0] {
        i
    } else {
        return Err(2);
    };

    let b = if let Value::I32(i) = inputs[1] {
        i
    } else {
        return Err(3);
    };

    let c = a + b;
    Ok(vec![Value::I32(c)])
}

fn extern_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    if inputs.len() != 2 {
        return Err(1);
    }

    let a = if let Value::I32(i) = inputs[0] {
        i
    } else {
        return Err(2);
    };

    let b = if let Value::I32(i) = inputs[1] {
        i
    } else {
        return Err(3);
    };

    let c = a + b;
    Ok(vec![Value::I32(c)])
}

fn extern_sub(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    todo!()
}

fn extern_mul(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    todo!()
}

fn extern_div(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    todo!()
}

fn extern_term(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    Ok(vec![Value::I32(1234)])
}

fn extern_fail(_inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
    Err(0x02)
}
