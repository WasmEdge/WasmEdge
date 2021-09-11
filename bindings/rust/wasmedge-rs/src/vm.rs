use super::wasmedge;

use crate::error::VmError;

#[derive(Debug)]
pub struct Vm<'a> {
    config: Option<&'a wasmedge::Config>,
    module: &'a wasmedge::Module,
    inner: Option<wasmedge::Vm>,
}

impl<'a> Vm<'a> {
    pub fn new(module: &'a wasmedge::Module) -> Result<VmBuilder<'a>, anyhow::Error> {
        VmBuilder::new(module)
    }

    pub fn run(
        mut self,
        func_name: &str,
        params: &[wasmedge::Value],
    ) -> Result<Vec<wasmedge::Value>, anyhow::Error> {
        match self.inner {
            Some(ref mut vm) => {
                let returns = vm.run(func_name, params).map_err(VmError::Execute)?;
                Ok(returns)
            }
            None => panic!("WasmEdge Vm can't run!"),
        }
    }
}

#[derive(Debug)]
pub struct VmBuilder<'a> {
    pub inner: Vm<'a>,
}

impl<'a> VmBuilder<'a> {
    pub fn new(module: &'a wasmedge::Module) -> Result<Self, anyhow::Error> {
        let vm = Vm {
            config: None,
            module,
            inner: None,
        };
        Ok(Self {
            inner: vm,
        })
    }

    pub fn with_config(self, config: &'a wasmedge::Config) -> Result<Self, anyhow::Error> {
        let mut vm = self.inner;
        vm.config = Some(config);
        Ok(Self { inner: vm })
    }

    pub fn build(self) -> Result<Vm<'a>, anyhow::Error> {
        let vm = self.inner;
        if let Some(cfg) = vm.config {
            let vm_instance =
                wasmedge::Vm::create(&cfg).map_err(VmError::Create)?;
            let vm_instance = vm_instance
                .load_wasm_from_ast_module(&vm.module)
                .map_err(VmError::ModuleLoad)?;
            let vm_instance = vm_instance.validate().map_err(VmError::Validate)?;
            let vm_instance = vm_instance.instantiate().map_err(VmError::Instantiate)?;
            println!("vm_instance {:#?}", vm_instance);
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
