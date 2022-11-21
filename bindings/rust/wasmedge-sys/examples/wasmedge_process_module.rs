//! Please set the environment variable `WASMEDGE_PLUGIN_PATH` to the directory containing the plugins to enable the wasmedge-process plugin.

#[cfg(target_os = "linux")]
use wasmedge_macro::sys_host_function;
use wasmedge_sys::utils;
#[cfg(target_os = "linux")]
use wasmedge_sys::{
    AsImport, CallingFrame, Config, FuncType, Function, ImportObject, Vm, WasmEdgeProcessModule,
    WasmValue,
};
#[cfg(target_os = "linux")]
use wasmedge_types::{
    error::{CoreError, CoreInstantiationError, HostFuncError, VmError, WasmEdgeError},
    ValType,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // load wasmedge_process plugins
    utils::load_plugin_from_default_paths();

    // A WasmEdgeProcessModule can be created implicitly inside a Vm by passing the Vm a config argument in which the wasmedge_process option is enabled.
    #[cfg(target_os = "linux")]
    create_wasmedge_process_module_implicitly()?;

    // WasmEdgeProcessModule implements ImportInstance trait, therefore it can be used to register function, table, memory and global instances.
    #[cfg(target_os = "linux")]
    create_wasmedge_process_module_explicitly()?;

    Ok(())
}

#[cfg(target_os = "linux")]
#[allow(clippy::assertions_on_result_states)]
fn create_wasmedge_process_module_implicitly() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);
    assert!(config.bulk_memory_operations_enabled());
    config.wasmedge_process(true);
    assert!(config.wasmedge_process_enabled());

    // create a Vm context with the given Config and Store
    let mut vm = Vm::create(Some(config), None)?;

    // get the default WasmEdgeProcess module instance
    let wasmedge_process_instance = vm.wasmedge_process_module_mut()?;
    assert_eq!(wasmedge_process_instance.name(), "wasmedge_process");

    // *** try to add another WasmEdgeProcess module, that causes error.

    // create a WasmEdgeProcess module
    let import_process = WasmEdgeProcessModule::create(None, false)?;

    let result = vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process));
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Core(CoreError::Instantiation(
            CoreInstantiationError::ModuleNameConflict
        )))
    );

    Ok(())
}

#[cfg(target_os = "linux")]
#[allow(clippy::assertions_on_result_states)]
fn create_wasmedge_process_module_explicitly() -> Result<(), Box<dyn std::error::Error>> {
    // create a Config context, not enable wasi and wasmedge_process options.
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);

    // create a Vm context with the given Config and Store
    let mut vm = Vm::create(Some(config), None)?;

    // get the WasmEdgeProcess module
    let result = vm.wasmedge_process_module_mut();
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule))
    );

    // *** try to add a WasmEdgeProcess module.

    // create a WasmEdgeProcess module
    let mut import_process = WasmEdgeProcessModule::create(None, false)?;

    // a function to import
    #[sys_host_function]
    fn real_add(
        _frame: CallingFrame,
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
    import_process.add_func("add", host_func);

    // register the WasmEdgeProcess module
    vm.register_wasm_from_import(ImportObject::WasmEdgeProcess(import_process))?;

    // get the WasmEdgeProcess module
    let result = vm.wasmedge_process_module_mut();
    assert!(result.is_err());
    assert_eq!(
        result.unwrap_err(),
        Box::new(WasmEdgeError::Vm(VmError::NotFoundWasmEdgeProcessModule))
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

    Ok(())
}
