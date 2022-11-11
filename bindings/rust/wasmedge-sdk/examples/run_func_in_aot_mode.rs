use wasmedge_sdk::{
    config::{
        CommonConfigOptions, CompilerConfigOptions, ConfigBuilder, HostRegistrationConfigOptions,
    },
    params, Compiler, CompilerOutputFormat, Vm, WasmVal,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let config = ConfigBuilder::new(CommonConfigOptions::new().bulk_memory_operations(true))
        .with_compiler_config(
            CompilerConfigOptions::new()
                .interruptible(true)
                .out_format(CompilerOutputFormat::Native),
        )
        .with_host_registration_config(HostRegistrationConfigOptions::default().wasi(true))
        .build()?;

    let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sys/tests/data/fibonacci.wasm");
    let aot_file = std::path::PathBuf::from("fibonacci.wasm.so");

    // compile wasm to so for runing in the `aot` mode
    let compiler = Compiler::new(Some(&config))?;
    compiler.compile_from_file(wasm_file, &aot_file)?;
    assert!(&aot_file.exists());

    let vm = Vm::new(Some(config))?;

    let res = vm.run_func_from_file(&aot_file, "fib", params!(5))?;
    println!("fib(5): {}", res[0].to_i32());

    // remove the generated aot file
    assert!(std::fs::remove_file(&aot_file).is_ok());
    Ok(())
}
