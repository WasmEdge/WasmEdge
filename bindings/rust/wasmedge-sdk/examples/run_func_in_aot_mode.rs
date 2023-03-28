//!
//! To run this example, use the following command:
//! ```bash
//! cd bindings/rust
//! cargo run -p wasmedge-sdk --example run_func_in_aot_mode -- --nocapture
//! ```

#[cfg(feature = "aot")]
use wasmedge_sdk::{
    config::{
        CommonConfigOptions, CompilerConfigOptions, ConfigBuilder, HostRegistrationConfigOptions,
    },
    params, Compiler, CompilerOutputFormat, VmBuilder, WasmVal,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "aot")]
    {
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
            .join("bindings/rust/wasmedge-sdk/examples/data/fibonacci.wat");
        let out_dir = std::env::current_dir()?;
        let aot_filename = "example_aot_fibonacci";

        // compile wasm to so for running in the `aot` mode
        let compiler = Compiler::new(Some(&config))?;
        let aot_file_path = compiler.compile_from_file(wasm_file, aot_filename, out_dir)?;
        assert!(&aot_file_path.exists());
        #[cfg(target_os = "macos")]
        assert!(aot_file_path.ends_with("example_aot_fibonacci.dylib"));
        #[cfg(target_os = "linux")]
        assert!(aot_file_path.ends_with("example_aot_fibonacci.so"));
        #[cfg(target_os = "windows")]
        assert!(aot_file_path.ends_with("example_aot_fibonacci.dll"));

        let mut vm = VmBuilder::new().with_config(config).build()?;

        let res = vm.run_func_from_file(&aot_file_path, "fib", params!(5))?;
        println!("fib(5): {}", res[0].to_i32());

        // remove the generated aot file
        let metadata = aot_file_path.metadata()?;
        if metadata.permissions().readonly() {
            let mut permissions = metadata.permissions();
            permissions.set_readonly(false);
            std::fs::set_permissions(&aot_file_path, permissions)?;
        }
        let result = std::fs::remove_file(&aot_file_path);
        println!("remove aot file: {result:?}");
    }

    Ok(())
}
