use crate::{wasmedge, Instance};

#[derive(Debug)]
pub struct Store<'vm> {
    pub(crate) inner: &'vm wasmedge::Store,
}
impl<'vm> Store<'vm> {
    pub fn instance_count(&self) -> u32 {
        self.inner.reg_module_len()
    }

    /// Returns the names of all registered [modules](crate::Module)
    pub fn instance_names(&self) -> Option<Vec<String>> {
        self.inner.reg_module_names()
    }

    /// Returns the [instance](crate::Instance) with the specified name.
    pub fn named_instance(&self, name: impl AsRef<str>) -> Option<Instance> {
        let inner_instance = self.inner.named_module(name.as_ref()).ok();
        if let Some(inner_instance) = inner_instance {
            return Some(Instance {
                inner: inner_instance,
            });
        }

        None
    }

    /// Returns the active [instance](crate::Instance).
    pub fn active_instance(&self) -> Option<Instance> {
        let inner_instance = self.inner.active_module().ok();
        if let Some(inner_instance) = inner_instance {
            return Some(Instance {
                inner: inner_instance,
            });
        }

        None
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        wasmedge::{Mutability, RefType},
        Func, GlobalType, ImportMod, MemoryType, Module, SignatureBuilder, TableType, ValType,
        Value, Vm,
    };

    #[test]
    fn test_store_basic() {
        // create a Vm context
        let result = Vm::new(None);
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get store
        let store = vm.store_mut();

        assert_eq!(store.instance_count(), 0);
        assert!(store.instance_names().is_none());

        // create an ImportMod instance
        let result = ImportMod::new("extern-module");
        assert!(result.is_ok());
        let mut import = result.unwrap();

        // add host function
        let sig = SignatureBuilder::new()
            .with_args(vec![ValType::I32; 2])
            .with_returns(vec![ValType::I32])
            .build();
        let result = Func::new(sig, Box::new(real_add), 0);
        assert!(result.is_ok());
        let host_func = result.unwrap();
        import.add_func("add", host_func);

        // add table
        let ty = TableType::new(RefType::FuncRef, 5, None);
        let result = import.add_table("table", ty);
        assert!(result.is_ok());

        // add memory
        let ty = MemoryType::new(10, None);
        let result = import.add_memory("mem", ty);
        assert!(result.is_ok());

        // add globals
        let ty = GlobalType::new(ValType::F32, Mutability::Const);
        let result = import.add_global("global", ty, Value::from_f32(3.5));
        assert!(result.is_ok());

        // add the import module into vm
        let result = vm.add_import(&import);
        assert!(result.is_ok());
        let vm = result.unwrap();

        // add a wasm module from a file
        let file = std::path::PathBuf::from(env!("WASMEDGE_DIR"))
            .join("tools/wasmedge/examples/fibonacci.wasm");
        let result = Module::from_file(&vm, file);
        assert!(result.is_ok());
        let module = result.unwrap();
        let result = vm.add_module(module, Some("fib-module"));
        assert!(result.is_ok());
        let mut vm = result.unwrap();

        // get store
        let store = vm.store_mut();

        // check the exported instances
        assert_eq!(store.instance_count(), 2);
        assert!(store.instance_names().is_some());
        let mod_names = store.instance_names().unwrap();
        assert_eq!(mod_names[0], "extern-module");
        assert_eq!(mod_names[1], "fib-module");

        assert_eq!(mod_names[0], "extern-module");
        let result = store.named_instance(mod_names[0].as_str());
        assert!(result.is_some());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[0]);

        assert_eq!(mod_names[1], "fib-module");
        let result = store.named_instance(mod_names[1].as_str());
        assert!(result.is_some());
        let instance = result.unwrap();
        assert!(instance.name().is_some());
        assert_eq!(instance.name().unwrap(), mod_names[1]);
    }

    fn real_add(inputs: Vec<Value>) -> std::result::Result<Vec<Value>, u8> {
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
