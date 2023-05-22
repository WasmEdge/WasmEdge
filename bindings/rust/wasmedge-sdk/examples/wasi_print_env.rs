//! This example uses `wasi_print_env.wasm`, which is generated by `wasi_print_env` project in the `bindings/rust/wasmedge-sys/examples` directory.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! // navigate into the directory bindings/rust/wasmedge-sdk/examples/wasi_print_env
//! cargo build --target wasm32-wasi --release
//!
//! // navigate back to the directory bindings/rust/
//! cargo run -p wasmedge-sdk --example wasi_print_env
//! ```

#[cfg(not(feature = "async"))]
use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder, HostRegistrationConfigOptions},
    params, VmBuilder,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(not(feature = "async"))]
    {
        // create a config with `wasi` option enabled
        let config = ConfigBuilder::new(CommonConfigOptions::default())
            .with_host_registration_config(HostRegistrationConfigOptions::default().wasi(true))
            .build()?;

        // create a vm
        let mut vm = VmBuilder::new().with_config(config).build()?;

        // set the envs and args for the wasi module
        let args = vec!["arg1", "arg2"];
        let envs = vec!["ENV1=VAL1", "ENV2=VAL2", "ENV3=VAL3"];
        let wasi_module = vm.wasi_module_mut().ok_or("Not found wasi module")?;
        wasi_module.initialize(Some(args), Some(envs), None);

        assert_eq!(wasi_module.exit_code(), 0);

        // load wasm module
        let wasm_file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
        .join("bindings/rust/wasmedge-sdk/examples/wasi_print_env/target/wasm32-wasi/release/wasi_print_env.wasm");

        vm.run_func_from_file(wasm_file, "print_env", params!())?;
    }

    Ok(())
}
