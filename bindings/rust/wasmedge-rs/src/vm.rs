use super::wasmedge;
use crate::module::Module;

use crate::error::VmError;

#[derive(Debug, Default)]
struct Vm<'a>{
    config: Option<&'a wasmedge::Config>,
    module: Option<Module>,
    inner: Option<wasmedge::Vm>,
    _private: ()
}

impl Vm {
    pub fn builder(module_path: &str) -> VmBuilder {
        VmBuilder::new(module_path)
    }

    pub fn default(module_path: &str) -> Result<Vm, Error> {
        let config = wasmedge::Config::default();
        let module = Module::new(&config, module_path)?;
        let vm_instance = wasmedge::Vm::create(&module.inner, &config)?;
        Ok(Vm { config, module, inner: Some(vm_instance), _private: ()})
    }

    fn create(
        self, 
        module: &wasmedge::Module,
        config: &wasmedge::Config,
    ) -> Result<Self, Error> {
        let vm = wasmedge::Vm::create(module, config).map_err(VmError::Create)?;
        let vm = vm.load_wasm_from_ast_module(module).map_err(VmError::ModuleLoad)?; 
        let vm = vm.validate().map_err(VmError::Validate)?;
        let vm = vm.instantiate().map_err(VmError::Instantiate)?;
        Ok(vm)
    }

    pub fn run(self, func_name: &str, params: &[wasmedge::Value]) -> Result<Vec<Value>, Error> {
        match self.inner {
            Some(vm) => {
                vm.run(func_name, params).map_err(Error::Execute)?;
            }
            None => panic!("WasmEdge Vm can't run!")
        }
    }

}


#[derive(Debug)]
pub struct VmBuilder{
    pub module_path: &'static str,
    pub inner: Vm
}

impl VmBuilder {
    pub fn new(module_path: &str) -> Self {
        let vm  = Vm::default();
        Self {module_path, inner: vm}
    }

    pub fn with_config(self, config: &wasmedge::Config) -> Result<Self, Error> {
        let mut vm = self.inner;
        vm.config = Some(config);
        let module = Module::new(vm.config, self.module_path)?;
        vm.module = Some(module);
        Ok(Self {module_path: self.module_path, inner: vm})
    }

    pub fn build(self) -> Result<Vm, Error> {
        let mut vm  = self.inner;
        // unwrap is safety!
        let vm_instance = vm.create(vm.module.unwrap(), vm.config.unwrap())?;
        vm.inner = Some(vm_instance);
        Ok(vm)
    }


}
