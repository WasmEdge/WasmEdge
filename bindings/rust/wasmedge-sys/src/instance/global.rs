//! Defines WasmEdge Global and GlobalType structs.
//!
//! A WasmEdge `Global` defines a global variable, which stores a single value of the given `GlobalType`.
//! `GlobalType` specifies whether a global variable is immutable or mutable.

use crate::{
    types::{Mutability, ValType},
    wasmedge, Error, Value, WasmEdgeResult,
};

/// Struct of WasmEdge Global.
///
/// A WasmEdge `Global` defines a global variable, which stores a single value of the given `GlobalType`.
#[derive(Debug)]
pub struct Global {
    pub(crate) ctx: *mut wasmedge::WasmEdge_GlobalInstanceContext,
    pub(crate) registered: bool,
}
impl Global {
    /// Creates a new `Global` to be associated with the given `GlobalType` and `Value`.
    ///
    /// If the creation succeeds, then the given `GlobalType` is consumed. The type of the given `Value`
    /// must be matched with `GlobalType`;otherwise, it causes a failure. For example, `Value::I32(520)` conflicts
    /// with a `GlobalType` with a value type defined as `ValType::F32`.
    ///
    /// # Errors
    ///
    /// If fail to create a `Global`, then an error is returned.
    ///
    pub fn create(ty: &mut GlobalType, val: Value) -> WasmEdgeResult<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_GlobalInstanceCreate(ty.ctx, wasmedge::WasmEdge_Value::from(val))
        };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Global instance",
            ))),
            false => {
                ty.ctx = std::ptr::null_mut();
                ty.registered = true;
                Ok(Self {
                    ctx,
                    registered: false,
                })
            }
        }
    }

    /// Returns the underlying wasm type of a `Global`.
    ///
    /// # Errors
    ///
    /// If fail to get the type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<GlobalType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_GlobalInstanceGetGlobalType(self.ctx) };
        match ty_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get type info from the Global instance",
            ))),
            false => Ok(GlobalType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Returns the value of a `Global`.
    pub fn get_value(&self) -> Value {
        let val = unsafe { wasmedge::WasmEdge_GlobalInstanceGetValue(self.ctx) };
        Value::from(val)
    }

    /// Sets the value of a `Global`.
    ///
    /// Notice that only a `Global` of Mutability::Var can be set a new value. Setting a new value for a
    /// `Global` of `Mutability::Const` causes a failure.
    ///
    /// # Errors
    ///
    /// If fail to set value, then an error is returned.
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::{Global, GlobalType, ValType, Mutability, Value};
    ///
    /// // create a GlobalType instance
    /// let mut ty = GlobalType::create(ValType::F32, Mutability::Var).expect("fail to create a GlobalType");
    /// // create a Global instance
    /// let mut global = Global::create(&mut ty, Value::F32(3.1415)).expect("fail to create a Global");
    ///
    /// global.set_value(Value::F32(314.15)).expect("fail to set a new value for a Global");
    /// assert_eq!(global.get_value(), Value::F32(314.15));
    /// ```
    ///
    ///
    pub fn set_value(&mut self, val: Value) -> WasmEdgeResult<()> {
        let ty = self.ty()?;
        if ty.mutability() == Mutability::Const {
            return Err(Error::OperationError(String::from(
                "Cannot set value for a const `Global`.",
            )));
        }
        if ty.value_type() != val.ty() {
            return Err(Error::OperationError(format!(
                "found a type conflict, expected:{:?}, fould:{:?}",
                ty.value_type(),
                val.ty()
            )));
        }
        unsafe {
            wasmedge::WasmEdge_GlobalInstanceSetValue(self.ctx, wasmedge::WasmEdge_Value::from(val))
        }
        Ok(())
    }
}
impl Drop for Global {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_GlobalInstanceDelete(self.ctx) };
        }
    }
}

/// Struct of WasmEdge GlobalType.
///
/// A [`GlobalType`] classifies a global variable, which hold a value and can either be mutable or immutable.
#[derive(Debug)]
pub struct GlobalType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_GlobalTypeContext,
    pub(crate) registered: bool,
}
impl GlobalType {
    /// Create a new `GlobalType` to be associated with the given `ValType` and `Mutability`.
    ///
    /// # Errors
    ///
    /// If fail to create a new `GlobalType`, then an error is returned.
    pub fn create(val_ty: ValType, mutable: Mutability) -> WasmEdgeResult<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_GlobalTypeCreate(
                wasmedge::WasmEdge_ValType::from(val_ty),
                wasmedge::WasmEdge_Mutability::from(mutable),
            )
        };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create GlobalType instance",
            ))),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the value type of a `GlobalType`.
    pub fn value_type(&self) -> ValType {
        let val = unsafe { wasmedge::WasmEdge_GlobalTypeGetValType(self.ctx) };
        val.into()
    }

    /// Returns a `Mutability` value of a `GlobalType`.
    pub fn mutability(&self) -> Mutability {
        let val = unsafe { wasmedge::WasmEdge_GlobalTypeGetMutability(self.ctx) };
        val.into()
    }
}
impl Drop for GlobalType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_GlobalTypeDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Mutability, ValType};

    #[test]
    fn test_globaltype() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        assert!(!global_ty.ctx.is_null());
        assert!(!global_ty.registered);

        // value type
        assert_eq!(global_ty.value_type(), ValType::I32);
        // Mutability
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }

    #[test]
    fn test_global_const_i32() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());

        // create a const Global instance
        let result = Global::create(&mut ty, Value::I32(99));
        assert!(result.is_ok());
        assert!(ty.ctx.is_null());
        assert!(ty.registered);
        let mut global_const = result.unwrap();

        // access the value held by global_const
        assert_eq!(global_const.get_value(), Value::I32(99));
        let result = global_const.set_value(Value::I32(0));
        assert!(result.is_err());

        // access the global type
        let result = global_const.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(ty.registered);
        assert_eq!(ty.value_type(), ValType::I32);
        assert_eq!(ty.mutability(), Mutability::Const);
    }

    #[test]
    fn test_global_var_f32() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::F32, Mutability::Var);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());

        // create a Var Global instance
        let result = Global::create(&mut ty, Value::F32(13.14));
        assert!(result.is_ok());
        assert!(ty.ctx.is_null());
        assert!(ty.registered);
        let mut global_var = result.unwrap();

        // access the value held by global_var
        assert_eq!(global_var.get_value(), Value::F32(13.14));
        let result = global_var.set_value(Value::F32(1.314));
        assert!(result.is_ok());
        assert_eq!(global_var.get_value(), Value::F32(1.314));

        // access the global type
        let result = global_var.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(ty.registered);
        assert_eq!(ty.value_type(), ValType::F32);
        assert_eq!(ty.mutability(), Mutability::Var);
    }

    #[test]
    fn test_global_conflict() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::F32, Mutability::Var);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());

        // create a Var Global instance with a value of mis-matched Value::I32 type
        let result = Global::create(&mut ty, Value::I32(520));
        assert!(result.is_err());
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        // create a Var Global instance with a value of Value::F32 type
        let result = Global::create(&mut ty, Value::F32(13.14));
        assert!(result.is_ok());
        assert!(ty.ctx.is_null());
        assert!(ty.registered);
        let mut global_var = result.unwrap();

        // set a new value of mis-matched Value::I32 type
        let result = global_var.set_value(Value::I32(1314));
        assert!(result.is_err());
        assert_eq!(global_var.get_value(), Value::F32(13.14));

        // set a new value of Value::F32 type
        let result = global_var.set_value(Value::F32(1.314));
        assert!(result.is_ok());
        assert_eq!(global_var.get_value(), Value::F32(1.314));
    }
}
