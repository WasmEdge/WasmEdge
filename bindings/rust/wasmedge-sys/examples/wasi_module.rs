//!
//! To run this example, follow the commands below:
//!
//! ```bash
//! // go into the directory: bindings/rust
//! cargo run -p wasmedge-sys --example wasi_module -- --nocapture
//! ```

#[cfg(not(feature = "custom_wasi"))]
use wasmedge_sys::{
    AsImport, CallingFrame, Config, FuncType, Function, ImportObject, Vm, WasiModule, WasmValue,
};
#[cfg(not(feature = "custom_wasi"))]
use wasmedge_types::{
    error::{HostFuncError, VmError, WasmEdgeError},
    ValType,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(not(feature = "custom_wasi"))]
    {
        // create a Config context, not enable wasi and wasmedge_process options.
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config
        let mut vm = Vm::create(Some(config))?;

        // get the Wasi module
        let result = vm.wasi_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Vm(VmError::NotFoundWasiModule))
        );

        // *** add a Wasi module.

        // create a Wasi module
        let mut import_wasi = WasiModule::create(None, None, None)?;

        // a function to import
        fn real_add(
            _: CallingFrame,
            inputs: Vec<WasmValue>,
        ) -> Result<Vec<WasmValue>, HostFuncError> {
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

            let c = a + b;

            Ok(vec![WasmValue::from_i32(c)])
        }

        // add host function
        let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
        let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
        import_wasi.add_func("add", host_func);

        // register the Wasi module
        vm.register_instance_from_import(ImportObject::Wasi(import_wasi))?;

        // get the Wasi module
        let result = vm.wasi_module();
        assert!(result.is_ok());
        let wasi_instance = result.unwrap();
        assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
    }

    #[cfg(not(feature = "custom_wasi"))]
    {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config))?;

        {
            // get the Wasi module
            let wasi_instance = vm.wasi_module()?;
            assert_eq!(wasi_instance.name(), "wasi_snapshot_preview1");
        }

        // *** try to add another Wasi module, that causes error.

        // create a Wasi module
        let import_wasi = WasiModule::create(None, None, None)?;

        let result = vm.register_instance_from_import(ImportObject::Wasi(import_wasi));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Vm(VmError::DuplicateImportModule(
                "wasi_snapshot_preview1".into()
            )))
        );
    }

    Ok(())
}
