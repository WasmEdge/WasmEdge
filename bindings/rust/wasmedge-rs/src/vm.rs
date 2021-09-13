use super::wasmedge;

use crate::{error::VmError, module::Module};

/// # Example
///
///
///
/// ```rust
///     let module_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
///         .join("tools/wasmedge/examples/fibonacci.wasm");
///
///     let config = wedge::Config::default();
///     let module = wedge::Module::new(&config, &module_path)?;
///
///     let vm = wedge::Vm::load(&module)?.with_config(&config)?.create()?;
///
///     let results = vm.run("fib", &[5.into()])?;
///
///     assert_eq!(results.len(), 1);
///     let result = results[0].as_i32().unwrap();
///
///     assert_eq!(result, 8);
/// ```
///
#[derive(Debug)]
pub struct Vm<'a> {
    config: Option<&'a wasmedge::Config>,
    module: &'a Module,
    inner: Option<wasmedge::Vm>,
}

impl<'a> Vm<'a> {
    pub fn load(module: &'a Module) -> Result<VmBuilder<'a>, anyhow::Error> {
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
    pub fn new(module: &'a Module) -> Result<Self, anyhow::Error> {
        let vm = Vm {
            config: None,
            module,
            inner: None,
        };
        Ok(Self { inner: vm })
    }

    pub fn with_config(self, config: &'a wasmedge::Config) -> Result<Self, anyhow::Error> {
        let mut vm = self.inner;
        vm.config = Some(config);
        Ok(Self { inner: vm })
    }

    pub fn create(self) -> Result<Vm<'a>, anyhow::Error> {
        let vm = self.inner;
        if let Some(cfg) = vm.config {
            let vm_instance = wasmedge::Vm::create(cfg).map_err(VmError::Create)?;
            let vm_instance = vm_instance
                .load_wasm_from_ast_module(&vm.module.inner)
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
