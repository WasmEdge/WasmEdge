use wasmedge_sys::{
    error::{CoreError, CoreInstantiationError, VmError, WasmEdgeError},
    Config, FuncType, Function, ImportInstance, ImportObject, Vm, WasiModule, WasmValue,
};
use wasmedge_types::ValType;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // [WasiModule] implements [ImportInstance](crate::ImportInstance) trait, therefore it can be used to register function, table, memory and global instances.
    {
        // create a Config context, not enable wasi and wasmedge_process options.
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the Wasi module
        let result = vm.wasi_import_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Vm(VmError::NotFoundWasiModule)
        );

        // *** try to add a Wasi module.

        // create a Wasi module
        let result = WasiModule::create(None, None, None);
        assert!(result.is_ok());
        let mut import_wasi = result.unwrap();

        // a function to import
        fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
            if inputs.len() != 2 {
                return Err(1);
            }

            let a = if inputs[0].ty() == ValType::I32 {
                inputs[0].to_i32()
            } else {
                return Err(2);
            };

            let b = if inputs[1].ty() == ValType::I32 {
                inputs[1].to_i32()
            } else {
                return Err(3);
            };

            let c = a + b;

            Ok(vec![WasmValue::from_i32(c)])
        }

        // add host function
        let result = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32]);
        assert!(result.is_ok());
        let func_ty = result.unwrap();
        let result = Function::create(&func_ty, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import_wasi.add_func("add", host_func);

        let result = vm.register_wasm_from_import(ImportObject::Wasi(import_wasi));
        assert!(result.is_ok());

        // get the Wasi module
        let result = vm.wasi_import_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Vm(VmError::NotFoundWasiModule)
        );

        // get store from vm
        let result = vm.store_mut();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        // check registered modules
        assert_eq!(store.module_len(), 1);
        let result = store.module_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

        // get wasi module instance
        let result = store.module("wasi_snapshot_preview1");
        assert!(result.is_ok());
        let _wasi_instance = result.unwrap();
    }

    // A [WasiModule] can be created implicitly inside a [Vm](crate::Vm) by passing the [Vm](crate::Vm) a [config](crate::Config) argument in which the `wasi` option is enabled.
    {
        // create a Config context
        let result = Config::create();
        assert!(result.is_ok());
        let mut config = result.unwrap();
        config.bulk_memory_operations(true);
        assert!(config.bulk_memory_operations_enabled());
        config.wasi(true);
        assert!(config.wasi_enabled());

        // create a Vm context with the given Config and Store
        let result = Vm::create(Some(config), None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get the Wasi module
        let result = vm.wasi_import_module_mut();
        assert!(result.is_ok());
        let _wasi_instance = result.unwrap();

        // *** try to add another Wasi module, that causes error.

        // create a Wasi module
        let result = WasiModule::create(None, None, None);
        assert!(result.is_ok());
        let import_wasi = result.unwrap();

        let result = vm.register_wasm_from_import(ImportObject::Wasi(import_wasi));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Instantiation(
                CoreInstantiationError::ModuleNameConflict
            ))
        );
    }

    Ok(())
}
