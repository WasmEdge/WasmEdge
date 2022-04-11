use wasmedge_sys::{
    error::{CoreError, CoreInstantiationError, VmError, WasmEdgeError},
    Config, FuncType, Function, ImportInstance, ImportObject, Vm, WasmEdgeProcessModule, WasmValue,
};
use wasmedge_types::ValType;

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // A WasmEdgeProcessModule can be created implicitly inside a Vm by passing the Vm a config argument in which the wasmedge_process option is enabled.
    {
        // create a Config context
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);
        config.wasmedge_process(true);

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config), None)?;

        // get the default WasmEdgeProcess module instance
        let default_wasmedge_process_instance = vm.wasmedge_process_import_module_mut()?;
        assert_eq!(default_wasmedge_process_instance.name(), "wasmedge_process");

        // *** try to add another WasmEdgeProcess module, that causes error.

        // create a WasmEdgeProcess module
        let import_process = WasmEdgeProcessModule::create(None, false)?;

        let result = vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Instantiation(
                CoreInstantiationError::ModuleNameConflict
            ))
        );
    }

    // WasmEdgeProcessModule implements ImportInstance trait, therefore it can be used to register function, table, memory and global instances.
    {
        // create a Config context, not enable wasi and wasmedge_process options.
        let mut config = Config::create()?;
        config.bulk_memory_operations(true);

        // create a Vm context with the given Config and Store
        let mut vm = Vm::create(Some(config), None)?;

        // get the WasmEdgeProcess module
        let result = vm.wasmedge_process_import_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule)
        );

        // *** try to add a WasmEdgeProcess module.

        // create a WasmEdgeProcess module
        let mut import_process = WasmEdgeProcessModule::create(None, false)?;

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
        let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
        let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
        import_process.add_func("add", host_func);

        // register the WasmEdgeProcess module
        vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process))?;

        // get the WasmEdgeProcess module
        let result = vm.wasmedge_process_import_module_mut();
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule)
        );

        // get store from vm
        let mut store = vm.store_mut()?;
        assert_eq!(store.module_len(), 1);
        let result = store.module_names();
        assert!(result.is_some());
        assert_eq!(result.unwrap(), ["wasmedge_process"]);

        // get the wasmedge_process module instance
        let wasmedge_process_instance = store.module("wasmedge_process")?;

        // get "add" function
        let result = wasmedge_process_instance.get_func("add");
        assert!(result.is_ok());
    }

    Ok(())
}
