use wasmedge_macro::sys_host_function;
use wasmedge_sys::{CallingFrame, Executor, FuncType, Function, WasmValue};
use wasmedge_types::{error::HostFuncError, ValType};

#[derive(Debug)]
struct Data<T, S> {
    _x: i32,
    _y: String,
    _v: Vec<T>,
    _s: Vec<S>,
}

#[sys_host_function]
fn real_add<T: std::fmt::Debug>(
    _frame: CallingFrame,
    input: Vec<WasmValue>,
    data: Option<&mut T>,
) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Rust: Entering Rust function real_add");

    println!("data: {data:?}");

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

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut data: Data<i32, &str> = Data {
        _x: 12,
        _y: "hello".to_string(),
        _v: vec![1, 2, 3],
        _s: vec!["macos", "linux", "windows"],
    };

    // create a FuncType
    let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
    assert!(result.is_ok());
    let func_ty = result.unwrap();
    // create a host function
    let result = Function::create(&func_ty, real_add, Some(&mut data), 0);
    assert!(result.is_ok());
    let host_func = result.unwrap();

    // get func type
    let result = host_func.ty();
    assert!(result.is_ok());
    let ty = result.unwrap();

    // check parameters
    assert_eq!(ty.params_len(), 2);
    let param_tys = ty.params_type_iter().collect::<Vec<_>>();
    assert_eq!(param_tys, vec![ValType::I32; 2]);

    // check returns
    assert_eq!(ty.returns_len(), 1);
    let return_tys = ty.returns_type_iter().collect::<Vec<_>>();
    assert_eq!(return_tys, vec![ValType::I32]);

    // run this function
    let result = Executor::create(None, None);
    assert!(result.is_ok());
    let mut executor = result.unwrap();
    let result = host_func.call(
        &mut executor,
        vec![WasmValue::from_i32(1), WasmValue::from_i32(2)],
    );
    assert!(result.is_ok());
    let returns = result.unwrap();
    assert_eq!(returns[0].to_i32(), 3);

    Ok(())
}
