//! This example demonstrates that the function in interpreter mode calls the functions in AOT mode, and vise versa.

use wasmedge_macro::sys_host_function;
#[cfg(feature = "aot")]
use wasmedge_sys::{
    AsImport, Compiler, Config, FuncType, Function, ImportModule, ImportObject, Vm,
};
use wasmedge_sys::{CallingFrame, WasmValue};
use wasmedge_types::error::HostFuncError;
#[cfg(feature = "aot")]
use wasmedge_types::ValType;

#[sys_host_function]
fn host_print_i32(
    _frame: CallingFrame,
    val: Vec<WasmValue>,
) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("-- Host Function: print I32: {}", val[0].to_i32());

    Ok(vec![])
}

#[sys_host_function]
fn host_print_f64(
    _frame: CallingFrame,
    val: Vec<WasmValue>,
) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("-- Host Function: print F64: {}", val[0].to_f64());

    Ok(vec![])
}

/// The function in interpreter mode (defined in module1) calls the functions in AOT mode (defined in module2)
#[allow(clippy::assertions_on_result_states)]
#[cfg(feature = "aot")]
fn interpreter_call_aot() -> Result<(), Box<dyn std::error::Error>> {
    // create a Vm instance
    let mut vm = Vm::create(Some(Config::create()?), None)?;

    // create an import module
    let mut import = ImportModule::create("host")?;

    // import host_print_i32 as a host function
    let func_ty = FuncType::create([ValType::I32], [])?;
    let host_func_print_i32 = Function::create(&func_ty, Box::new(host_print_i32), 0)?;
    import.add_func("host_printI32", host_func_print_i32);

    // import host_print_f64 as a host function
    let func_ty = FuncType::create([ValType::F64], [])?;
    let host_func_print_f64 = Function::create(&func_ty, Box::new(host_print_f64), 0)?;
    import.add_func("host_printF64", host_func_print_f64);

    // register the import module
    vm.register_wasm_from_import(ImportObject::Import(import))?;

    // compile the "module2" into AOT mode
    let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/examples/data/module2.wat");
    #[cfg(target_os = "linux")]
    let out_path = std::path::PathBuf::from("module2-uni.so");
    #[cfg(target_os = "macos")]
    let out_path = std::path::PathBuf::from("module2-uni.dylib");
    #[cfg(target_os = "windows")]
    let out_path = std::path::PathBuf::from("module2-uni.dll");
    let compiler = Compiler::create(Some(Config::create()?))?;
    compiler.compile_from_file(in_path, &out_path)?;

    // register a named module from "module2-uni.wasm"
    vm.register_wasm_from_file("module", &out_path)?;

    // register an active module from "module1.wasm"
    let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/examples/data/module1.wat");
    vm.load_wasm_from_file(wasm_file)?;
    vm.validate()?;
    vm.instantiate()?;

    // run "printAdd" function exported from "module1.wasm"
    let returns = vm.run_function(
        "printAdd",
        [WasmValue::from_i32(1234), WasmValue::from_i32(5678)],
    )?;
    assert_eq!(returns.len(), 0);

    // run "printDiv" function exported from "module1.wasm"
    let returns = vm.run_function(
        "printDiv",
        [WasmValue::from_f64(9876.0), WasmValue::from_f64(4321.0)],
    )?;
    assert_eq!(returns.len(), 0);

    // run "printI32" function exported from "module1.wasm"
    let returns = vm.run_function("printI32", [WasmValue::from_i32(87654321)])?;
    assert_eq!(returns.len(), 0);

    // run "printF64" function exported from "module1.wasm"
    let returns = vm.run_function("printF64", [WasmValue::from_f64(5566.1122)])?;
    assert_eq!(returns.len(), 0);

    // clean up the generated file at runtime
    if out_path.exists() {
        assert!(std::fs::remove_file(out_path).is_ok());
    }

    Ok(())
}

#[allow(clippy::assertions_on_result_states)]
#[cfg(feature = "aot")]
fn aot_call_interpreter() -> Result<(), Box<dyn std::error::Error>> {
    // create a Vm instance
    let mut vm = Vm::create(Some(Config::create()?), None)?;

    // create an import module
    let mut import = ImportModule::create("host")?;

    // import host_print_i32 as a host function
    let func_ty = FuncType::create([ValType::I32], [])?;
    let host_func_print_i32 = Function::create(&func_ty, Box::new(host_print_i32), 0)?;
    import.add_func("host_printI32", host_func_print_i32);

    // import host_print_f64 as a host function
    let func_ty = FuncType::create([ValType::F64], [])?;
    let host_func_print_f64 = Function::create(&func_ty, Box::new(host_print_f64), 0)?;
    import.add_func("host_printF64", host_func_print_f64);

    // register the import module
    vm.register_wasm_from_import(ImportObject::Import(import))?;

    // register a named module from "module2.wasm"
    let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/examples/data/module2.wat");
    vm.register_wasm_from_file("module", wasm_file)?;

    // compile the "module1" into AOT mode
    let in_path = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/examples/data/module1.wat");
    #[cfg(target_os = "linux")]
    let out_path = std::path::PathBuf::from("module1-uni.so");
    #[cfg(target_os = "macos")]
    let out_path = std::path::PathBuf::from("module1-uni.dylib");
    #[cfg(target_os = "windows")]
    let out_path = std::path::PathBuf::from("module1-uni.dll");
    let compiler = Compiler::create(Some(Config::create()?))?;
    compiler.compile_from_file(in_path, &out_path)?;

    // register an active module from "module1.wasm"
    vm.load_wasm_from_file(&out_path)?;
    vm.validate()?;
    vm.instantiate()?;

    // run "printAdd" function exported from "module1.wasm"
    let returns = vm.run_function(
        "printAdd",
        [WasmValue::from_i32(1234), WasmValue::from_i32(5678)],
    )?;
    assert_eq!(returns.len(), 0);

    // run "printDiv" function exported from "module1.wasm"
    let returns = vm.run_function(
        "printDiv",
        [WasmValue::from_f64(9876.0), WasmValue::from_f64(4321.0)],
    )?;
    assert_eq!(returns.len(), 0);

    // run "printI32" function exported from "module1.wasm"
    let returns = vm.run_function("printI32", [WasmValue::from_i32(87654321)])?;
    assert_eq!(returns.len(), 0);

    // run "printF64" function exported from "module1.wasm"
    let returns = vm.run_function("printF64", [WasmValue::from_f64(5566.1122)])?;
    assert_eq!(returns.len(), 0);

    // clean up the generated file at runtime
    if out_path.exists() {
        assert!(std::fs::remove_file(out_path).is_ok());
    }

    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // The function in interpreter mode calls the functions in AOT mode
    #[cfg(feature = "aot")]
    interpreter_call_aot()?;

    // The function in AOT mode calls the functions in interpreter mode
    #[cfg(feature = "aot")]
    aot_call_interpreter()?;

    Ok(())
}
