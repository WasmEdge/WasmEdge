//! Defines Global and GlobalType.

use crate::{types::Val, GlobalType, WasmEdgeResult};
use wasmedge_sys as sys;

/// Defines a WebAssembly global variable, which stores a single value of the given [GlobalType](https://wasmedge.github.io/WasmEdge/wasmedge_types/struct.GlobalType.html) and a flag indicating whether it is mutable or not.
#[derive(Debug)]
pub struct Global {
    pub(crate) inner: sys::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Global {
    /// Creates a new wasm global variable with the given type and initial value.
    ///
    /// # Arguments
    ///
    /// * `ty` - The type of the global variable to be created.
    ///
    /// * `init` - The initial value of the global variable.
    ///
    /// # Error
    ///
    /// If fail to create the global variable, then an error is returned.
    pub fn new(ty: GlobalType, init: Val) -> WasmEdgeResult<Self> {
        let inner = sys::Global::create(&ty.into(), init.into())?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Returns the exported name of this [Global].
    ///
    /// Notice that this field is meaningful only if this global variable is used as an exported instance.
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this [Global] exports.
    ///
    /// Notice that this field is meaningful only if this global variable is used as an exported instance.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of this [Global].
    pub fn ty(&self) -> WasmEdgeResult<GlobalType> {
        let gt = self.inner.ty()?;
        Ok(gt.into())
    }

    /// Returns the current value of this [Global].
    pub fn get_value(&self) -> Val {
        self.inner.get_value().into()
    }

    /// Sets a new value of this [Global].
    ///
    /// Notice that only global variables of [Var](wasmedge_types::Mutability) type are allowed to perform this function.
    ///
    /// # Argument
    ///
    /// * `value` - The new value of the [Global].
    ///
    /// # Error
    ///
    /// If fail to update the value of the global variable, then an error is returned.
    pub fn set_value(&mut self, val: Val) -> WasmEdgeResult<()> {
        self.inner.set_value(val.into())?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::{GlobalError, WasmEdgeError},
        Executor, ImportObjectBuilder, Mutability, Statistics, Store, ValType,
    };

    #[test]
    fn test_global_type() {
        // create a GlobalType instance
        let global_ty = GlobalType::new(ValType::I32, Mutability::Const);

        // value type
        assert_eq!(global_ty.value_ty(), ValType::I32);
        // Mutability
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_global_basic() {
        // create a Const global instance
        let result = Global::new(
            GlobalType::new(ValType::I32, Mutability::Const),
            Val::I32(1314),
        );
        assert!(result.is_ok());
        let global_const = result.unwrap();

        // create a Var global instance
        let result = Global::new(
            GlobalType::new(ValType::F32, Mutability::Var),
            Val::F32(13.14),
        );
        assert!(result.is_ok());
        let global_var = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_global("const-global", global_const)
            .expect("failed to add const-global")
            .with_global("var-global", global_var)
            .expect("failed to add var-global")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the Const global from the store of vm
        let result = instance.global("const-global");
        assert!(result.is_some());
        let mut const_global = result.unwrap();

        // check global
        assert!(const_global.name().is_some());
        assert_eq!(const_global.name().unwrap(), "const-global");
        assert!(const_global.mod_name().is_some());
        assert_eq!(const_global.mod_name().unwrap(), "extern");
        let result = const_global.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.value_ty(), ValType::I32);
        assert_eq!(ty.mutability(), Mutability::Const);

        // get value of global
        if let Val::I32(value) = const_global.get_value() {
            assert_eq!(value, 1314);
        }

        // set a new value
        let result = const_global.set_value(Val::I32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            Box::new(WasmEdgeError::Global(GlobalError::ModifyConst))
        );

        // get the Var global from the store of vm
        let result = store.module_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the Var global from the store of vm
        let result = instance.global("var-global");
        assert!(result.is_some());
        let mut var_global = result.unwrap();

        // check global
        assert!(var_global.name().is_some());
        assert_eq!(var_global.name().unwrap(), "var-global");
        assert!(var_global.mod_name().is_some());
        assert_eq!(var_global.mod_name().unwrap(), "extern");
        let result = var_global.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.value_ty(), ValType::F32);
        assert_eq!(ty.mutability(), Mutability::Var);

        // get the value of var_global
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 13.14);
        }

        // set a new value
        let result = var_global.set_value(Val::F32(1.314));
        assert!(result.is_ok());

        // get the value of var_global again
        let result = instance.global("var-global");
        assert!(result.is_some());
        let var_global = result.unwrap();
        if let Val::F32(value) = var_global.get_value() {
            assert_eq!(value, 1.314);
        }
    }
}
