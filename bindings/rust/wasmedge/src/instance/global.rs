//! Defines Global and GlobalType.

use crate::{error::Result, sys, types::Val, Mutability, ValType};

/// Struct of WasmEdge Global.
#[derive(Debug)]
pub struct Global<'instance> {
    pub(crate) inner: sys::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Global<'instance> {
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    pub fn registered(&self) -> bool {
        self.mod_name.is_some()
    }

    pub fn ty(&self) -> Result<GlobalType> {
        let gt = self.inner.ty()?;
        Ok(GlobalType {
            ty: gt.value_type(),
            mutability: gt.mutability(),
        })
    }

    pub fn get_value(&self) -> Val {
        self.inner.get_value().into()
    }

    pub fn set_value(&mut self, val: Val) -> Result<()> {
        self.inner.set_value(val.into())?;
        Ok(())
    }
}

/// Struct of WasmEdge GlobalType.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GlobalType {
    ty: ValType,
    mutability: Mutability,
}
impl GlobalType {
    pub fn new(ty: ValType, mutability: Mutability) -> Self {
        Self { ty, mutability }
    }

    pub fn value_ty(&self) -> ValType {
        self.ty
    }

    pub fn mutability(&self) -> Mutability {
        self.mutability
    }

    pub fn to_raw(self) -> Result<sys::GlobalType> {
        let raw = sys::GlobalType::create(self.value_ty(), self.mutability())?;
        Ok(raw)
    }
}
impl From<sys::GlobalType> for GlobalType {
    fn from(ty: sys::GlobalType) -> Self {
        Self {
            ty: ty.value_type(),
            mutability: ty.mutability(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        error::WasmEdgeError,
        sys, Executor, ImportModuleBuilder, Mutability, Statistics, Store, ValType,
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
    fn test_global_basic() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_global(
                "const-global",
                GlobalType::new(ValType::I32, Mutability::Const),
                Val::I32(1314),
            )
            .expect("failed to add const-global")
            .with_global(
                "var-global",
                GlobalType::new(ValType::F32, Mutability::Var),
                Val::F32(13.14),
            )
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

        let result = store.named_instance("extern");
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
        } else {
            assert!(false);
        }

        // set a new value
        let result = const_global.set_value(Val::I32(314));
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(sys::error::WasmEdgeError::Global(
                sys::error::GlobalError::ModifyConst
            ))
        );

        // get the Var global from the store of vm
        let result = store.named_instance("extern");
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
        } else {
            assert!(false);
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
        } else {
            assert!(false);
        }
    }
}
