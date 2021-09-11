use super::wasmedge;
use crate::module::Module;

use crate::error::VmError;
use std::sync::Arc;

use std::path::PathBuf;

#[derive(Debug)]
pub struct Vm {
    config: Option<Arc<wasmedge::Config>>,
    module: Option<Arc<Module>>,
    inner: Option<wasmedge::Vm>,
}

impl Vm {
    pub fn new(module_path: PathBuf) -> Result<VmBuilder, anyhow::Error> {
        VmBuilder::new(module_path)
    }

    fn default() -> Self {
        // Panic if faild
        let config = Arc::new(wasmedge::Config::default());
        Self {
            config: Some(config),
            module: None,
            inner: None,
        }
    }

    pub fn run(
        self,
        func_name: &str,
        params: &[wasmedge::Value],
    ) -> Result<Vec<wasmedge::Value>, anyhow::Error> {
        match self.inner {
            Some(mut vm) => {
                let returns = vm.run(func_name, params).map_err(VmError::Execute)?;
                Ok(returns)
            }
            None => panic!("WasmEdge Vm can't run!"),
        }
    }
}

#[derive(Debug)]
pub struct VmBuilder {
    pub module_path: PathBuf,
    pub inner: Vm,
}

impl VmBuilder {
    pub fn new(module_path: PathBuf) -> Result<Self, anyhow::Error> {
        let mut vm = Vm::default();
        // unwrap is safety.
        let module = Module::new(vm.config.clone().unwrap().as_ref(), &module_path)?;
        vm.module = Some(Arc::new(module));
        Ok(Self {
            module_path,
            inner: vm,
        })
    }

    pub fn with_config(self, config: wasmedge::Config) -> Result<Self, anyhow::Error> {
        let mut vm = self.inner;
        vm.config = Some(Arc::new(config));
        Ok(Self { inner: vm, ..self })
    }

    pub fn build(self) -> Result<Vm, anyhow::Error> {
        let vm = self.inner;
        if vm.module.is_some() && vm.config.is_some() {
            let (module, config) = (vm.module.clone().unwrap(), vm.config.clone().unwrap());
            let vm_instance =
                wasmedge::Vm::create(&module.inner, &config).map_err(VmError::Create)?;
            let vm_instance = vm_instance
                .load_wasm_from_ast_module(&module.inner)
                .map_err(VmError::ModuleLoad)?;
            let vm_instance = vm_instance.validate().map_err(VmError::Validate)?;
            let vm_instance = vm_instance.instantiate().map_err(VmError::Instantiate)?;
            Ok(Vm {
                config: vm.config,
                module: vm.module,
                inner: Some(vm_instance),
            })
        } else {
            panic!("Failed! Please specify a reasonable module path and configuration");
        }
    }
}
