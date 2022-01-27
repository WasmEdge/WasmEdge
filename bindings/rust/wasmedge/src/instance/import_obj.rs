use crate::{error::WasmEdgeResult, wasmedge, Func, Global, Memory, Table, Vm};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct ImportObject {
    pub(crate) inner: wasmedge::ImportObj,
}
impl ImportObject {
    pub fn new(name: impl AsRef<str>) -> WasmEdgeResult<Self> {
        let inner = wasmedge::ImportObj::create(name.as_ref())?;
        Ok(Self { inner })
    }

    pub fn new_as_wasi<'a>(
        args: Option<Vec<&'a str>>,
        envs: Option<Vec<&'a str>>,
        preopens: Option<Vec<&'a str>>,
    ) -> WasmEdgeResult<Self> {
        let inner = wasmedge::ImportObj::create_wasi(args, envs, preopens)?;
        Ok(Self { inner })
    }

    pub fn new_as_wasmedge_process<'a>(
        allowed_cmds: Option<Vec<&'a str>>,
        allowed: bool,
    ) -> WasmEdgeResult<Self> {
        let inner = wasmedge::ImportObj::create_wasmedge_process(allowed_cmds, allowed)?;
        Ok(Self { inner })
    }

    fn add_func(&mut self, name: impl AsRef<str>, func: &mut Func) {
        self.inner.add_func(name.as_ref(), &mut func.inner)
    }

    fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
        self.inner.add_global(name.as_ref(), &mut global.inner)
    }

    fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
        self.inner.add_memory(name.as_ref(), &mut memory.inner)
    }

    fn add_table(&mut self, name: impl AsRef<str>, table: &mut Table) {
        self.inner.add_table(name.as_ref(), &mut table.inner)
    }
}

#[derive(Debug)]
pub struct ImportObjectWasi<'vm> {
    pub(crate) inner: wasmedge::ImportObj,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> ImportObjectWasi<'vm> {
    pub fn reinit(
        &mut self,
        args: Option<Vec<&str>>,
        envs: Option<Vec<&str>>,
        preopens: Option<Vec<&str>>,
    ) {
        self.inner.init_wasi(args, envs, preopens)
    }

    pub fn exit_code(&self) -> u32 {
        self.inner.exit_code()
    }
}

#[derive(Debug)]
pub struct ImportObjectWasmEdgeProcess<'vm> {
    pub(crate) inner: wasmedge::ImportObj,
    pub(crate) _marker: PhantomData<&'vm Vm>,
}
impl<'vm> ImportObjectWasmEdgeProcess<'vm> {
    pub fn reinit(&mut self, allowed_cmds: Option<Vec<&str>>, allowed: bool) {
        self.inner.init_wasmedge_process(allowed_cmds, allowed)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        error::WasmEdgeError, wasmedge, ConfigBuilder, SignatureBuilder, ValType, Value, VmBuilder,
    };

    #[test]
    fn test_import_wasmedgeprocess() {
        {
            // create a Config
            let result = ConfigBuilder::new();
            assert!(result.is_ok());
            let config_builder = result.unwrap();
            let config = config_builder.with_wasmedge_process().build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get WasmEdgeProcess module from vm
            let result = vm.wasmedge_process_module();
            assert!(result.is_ok());

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.count_of_module(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);

            // * try to add another WasmEdgeProcess module, that causes error

            // create a WasmEdgeProcess module
            let result = ImportObject::new_as_wasmedge_process(None, false);
            assert!(result.is_ok());
            let mut import_process = result.unwrap();

            let result = vm.register_wasm_from_import(&mut import_process);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                    wasmedge::error::CoreError::Instantiation(
                        wasmedge::error::CoreInstantiationError::ModuleNameConflict
                    )
                ))
            );
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = ConfigBuilder::new();
            assert!(result.is_ok());
            let config_builder = result.unwrap();
            let config = config_builder.build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get WasmEdgeProcess module from vm
            let result = vm.wasmedge_process_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasmEdgeProcessImportObjectModule
                ))
            );

            // *** try to add a WasmEdgeProcess module.

            // create a WasmEdgeProcess module
            let result = ImportObject::new_as_wasmedge_process(None, false);
            assert!(result.is_ok());
            let mut import_process = result.unwrap();

            // add host function
            let signature = SignatureBuilder::new()
                .with_args(vec![ValType::I32; 2])
                .with_returns(vec![ValType::I32])
                .build();
            let result = Func::new(signature, Box::new(real_add), 0);
            assert!(result.is_ok());
            let mut host_func = result.unwrap();
            import_process.add_func("add", &mut host_func);

            let result = vm.register_wasm_from_import(&mut import_process);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the WasmEdgeProcess module
            let result = vm.wasmedge_process_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasmEdgeProcessImportObjectModule
                ))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.count_of_module(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasmedge_process"]);

            // find "add" host function
            let result = store.function("add", Some("wasmedge_process"));
            assert!(result.is_ok());
            let host_func = result.unwrap();
            assert_eq!(host_func.name().unwrap(), "add");
            assert_eq!(host_func.mod_name().unwrap(), "wasmedge_process");
        }
    }

    #[test]
    fn test_import_wasi() {
        {
            // create a Config
            let result = ConfigBuilder::new();
            assert!(result.is_ok());
            let config_builder = result.unwrap();
            let config = config_builder.with_wasi().build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get Wasi module from vm
            let result = vm.wasi_module();
            assert!(result.is_ok());

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.count_of_module(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

            // * try to add another Wasi module, that causes error

            // create a Wasi module
            let result = ImportObject::new_as_wasi(None, None, None);
            assert!(result.is_ok());
            let mut import_wasi = result.unwrap();

            let result = vm.register_wasm_from_import(&mut import_wasi);
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Core(
                    wasmedge::error::CoreError::Instantiation(
                        wasmedge::error::CoreInstantiationError::ModuleNameConflict
                    )
                ))
            );
        }

        {
            // create a Config context, not enable wasi and wasmedge_process options.
            let result = ConfigBuilder::new();
            assert!(result.is_ok());
            let config_builder = result.unwrap();
            let config = config_builder.build();

            // create a Vm context
            let result = VmBuilder::new().with_config(&config).build();
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get Wasi module from vm
            let result = vm.wasi_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasiImportObjectModule
                ))
            );

            // *** try to add a Wasi module.

            // create a WasmEdgeProcess module
            let result = ImportObject::new_as_wasi(None, None, None);
            assert!(result.is_ok());
            let mut import_wasi = result.unwrap();

            // add host function
            let signature = SignatureBuilder::new()
                .with_args(vec![ValType::I32; 2])
                .with_returns(vec![ValType::I32])
                .build();
            let result = Func::new(signature, Box::new(real_add), 0);
            assert!(result.is_ok());
            let mut host_func = result.unwrap();
            import_wasi.add_func("add", &mut host_func);

            let result = vm.register_wasm_from_import(&mut import_wasi);
            assert!(result.is_ok());
            let mut vm = result.unwrap();

            // get the Wasi module
            let result = vm.wasi_module();
            assert!(result.is_err());
            assert_eq!(
                result.unwrap_err(),
                WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Vm(
                    wasmedge::error::VmError::NotFoundWasiImportObjectModule
                ))
            );

            // get store from vm
            let result = vm.store_mut();
            assert!(result.is_ok());
            let store = result.unwrap();

            // check registered modules
            assert_eq!(store.count_of_module(), 1);
            let result = store.module_names();
            assert!(result.is_some());
            assert_eq!(result.unwrap(), ["wasi_snapshot_preview1"]);

            // find "add" host function
            let result = store.function("add", Some("wasi_snapshot_preview1"));
            assert!(result.is_ok());
            let host_func = result.unwrap();
            assert_eq!(host_func.name().unwrap(), "add");
            assert_eq!(host_func.mod_name().unwrap(), "wasi_snapshot_preview1");
        }
    }

    fn real_add(inputs: Vec<Value>) -> Result<Vec<Value>, u8> {
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

        Ok(vec![Value::from_i32(c)])
    }
}
