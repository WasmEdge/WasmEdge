//! Defines WasmEdge Global and GlobalType structs.
//!
//! A WasmEdge `Global` defines a global variable, which stores a single value of the given `GlobalType`.
//! `GlobalType` specifies whether a global variable is immutable or mutable.

use crate::{
    error::{GlobalError, WasmEdgeError},
    ffi, WasmEdgeResult, WasmValue,
};
use std::sync::Arc;
use wasmedge_types::{Mutability, ValType};

/// Defines a WebAssembly global variable, which stores a single value of the given [type](crate::GlobalType) and a flag indicating whether it is mutable or not.
#[derive(Debug)]
pub struct Global {
    pub(crate) inner: Arc<InnerGlobal>,
    pub(crate) registered: bool,
}
impl Global {
    /// Creates a new [Global] instance to be associated with the given [GlobalType] and [WasmValue](crate::WasmValue).
    ///
    /// The type of the given [WasmValue](crate::WasmValue) must be matched with [GlobalType]; otherwise, it causes a failure. For example, `WasmValue::I32(520)` conflicts with a [GlobalType] with a value type defined as `ValType::F32`.
    ///
    /// # Errors
    ///
    /// * If fail to create the Global instance, then WasmEdgeError::Global(GlobalError::Create)(crate::error::GlobalError) is returned.
    ///
    pub fn create(ty: &GlobalType, val: WasmValue) -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_GlobalInstanceCreate(ty.inner.0, val.as_raw()) };

        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Global(GlobalError::Create))),
            false => Ok(Self {
                inner: Arc::new(InnerGlobal(ctx)),
                registered: false,
            }),
        }
    }

    /// Returns the underlying wasm type of a [Global] instance.
    ///
    /// # Errors
    ///
    /// If fail to get the type, then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<GlobalType> {
        let ty_ctx = unsafe { ffi::WasmEdge_GlobalInstanceGetGlobalType(self.inner.0) };
        match ty_ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::Global(GlobalError::Type))),
            false => Ok(GlobalType {
                inner: InnerGlobalType(ty_ctx as *mut _),
                registered: true,
            }),
        }
    }

    /// Returns the value of the [Global] instance.
    pub fn get_value(&self) -> WasmValue {
        let val = unsafe { ffi::WasmEdge_GlobalInstanceGetValue(self.inner.0) };
        val.into()
    }

    /// Sets the value of the [Global] instance.
    ///
    /// Notice that only the [Global] instance of [Mutability::Var](wasmedge_types::Mutability::Var) type can be set a new value. Setting a new value for a [Global] of [Mutability::Const](wasmedge_types::Mutability::Const) causes a failure.
    ///
    /// # Argument
    ///
    /// * `val` - The new wasm value to be set.
    ///
    /// # Errors
    ///
    /// If fail to set value, then an error is returned.
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::{Global, GlobalType, WasmValue};
    /// use wasmedge_types::{ValType, Mutability};
    ///
    /// // create a GlobalType instance
    /// let ty = GlobalType::create(ValType::F32, Mutability::Var).expect("fail to create a GlobalType");
    /// // create a Global instance
    /// let mut global = Global::create(&ty, WasmValue::from_f32(3.1415)).expect("fail to create a Global");
    ///
    /// global.set_value(WasmValue::from_f32(314.15)).expect("fail to set a new value for a Global");
    /// assert_eq!(global.get_value().to_f32(), 314.15);
    /// ```
    ///
    ///
    pub fn set_value(&mut self, val: WasmValue) -> WasmEdgeResult<()> {
        let ty = self.ty()?;
        if ty.mutability() == Mutability::Const {
            return Err(Box::new(WasmEdgeError::Global(GlobalError::ModifyConst)));
        }
        if ty.value_type() != val.ty() {
            return Err(Box::new(WasmEdgeError::Global(
                GlobalError::UnmatchedValType,
            )));
        }
        unsafe { ffi::WasmEdge_GlobalInstanceSetValue(self.inner.0, val.as_raw()) }
        Ok(())
    }

    /// Provides a raw pointer to the inner global context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_GlobalInstanceContext {
        self.inner.0 as *const _
    }
}
impl Drop for Global {
    fn drop(&mut self) {
        if !self.registered && Arc::strong_count(&self.inner) == 1 && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_GlobalInstanceDelete(self.inner.0) };
        }
    }
}
impl Clone for Global {
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
            registered: false,
        }
    }
}

#[derive(Debug)]
pub(crate) struct InnerGlobal(pub(crate) *mut ffi::WasmEdge_GlobalInstanceContext);
unsafe impl Send for InnerGlobal {}
unsafe impl Sync for InnerGlobal {}

/// Defines the type of a wasm global variable.
///
/// A [GlobalType] classifies a global variable that hold a value and can either be mutable or immutable.
#[derive(Debug)]
pub struct GlobalType {
    pub(crate) inner: InnerGlobalType,
    pub(crate) registered: bool,
}
impl GlobalType {
    /// Create a new [GlobalType] to be associated with the given [ValType](wasmedge_types::ValType) and [Mutability](wasmedge_types::Mutability).
    ///
    /// # Arguments
    ///
    /// * `val_type` - The value type of the global variable.
    ///
    /// * `mutability` - The mutability of the global variable.
    ///
    /// # Errors
    ///
    /// If fail to create a new [GlobalType], then an error is returned.
    pub fn create(val_ty: ValType, mutable: Mutability) -> WasmEdgeResult<Self> {
        let ctx = unsafe { ffi::WasmEdge_GlobalTypeCreate(val_ty.into(), mutable.into()) };
        match ctx.is_null() {
            true => Err(Box::new(WasmEdgeError::GlobalTypeCreate)),
            false => Ok(Self {
                inner: InnerGlobalType(ctx),
                registered: false,
            }),
        }
    }

    /// Returns the value type of the [GlobalType].
    pub fn value_type(&self) -> ValType {
        let val = unsafe { ffi::WasmEdge_GlobalTypeGetValType(self.inner.0 as *const _) };
        val.into()
    }

    /// Returns the [Mutability](wasmedge_types::Mutability) value of the [GlobalType].
    pub fn mutability(&self) -> Mutability {
        let val = unsafe { ffi::WasmEdge_GlobalTypeGetMutability(self.inner.0) };
        val.into()
    }

    /// Provides a raw pointer to the inner global type context.
    #[cfg(feature = "ffi")]
    pub fn as_ptr(&self) -> *const ffi::WasmEdge_GlobalTypeContext {
        self.inner.0 as *const _
    }
}
impl Drop for GlobalType {
    fn drop(&mut self) {
        if !self.registered && !self.inner.0.is_null() {
            unsafe { ffi::WasmEdge_GlobalTypeDelete(self.inner.0) };
        }
    }
}
impl From<wasmedge_types::GlobalType> for GlobalType {
    fn from(ty: wasmedge_types::GlobalType) -> Self {
        GlobalType::create(ty.value_ty(), ty.mutability()).expect(
            "[wasmedge-sys] Failed to convert wasmedge_types::GlobalType into wasmedge_sys::GlobalType.",
        )
    }
}
impl From<GlobalType> for wasmedge_types::GlobalType {
    fn from(ty: GlobalType) -> Self {
        wasmedge_types::GlobalType::new(ty.value_type(), ty.mutability())
    }
}

#[derive(Debug)]
pub(crate) struct InnerGlobalType(pub(crate) *mut ffi::WasmEdge_GlobalTypeContext);
unsafe impl Send for InnerGlobalType {}
unsafe impl Sync for InnerGlobalType {}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        sync::{Arc, Mutex},
        thread,
    };
    use wasmedge_types::{Mutability, ValType};

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_global_type() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();
        assert!(!global_ty.inner.0.is_null());
        assert!(!global_ty.registered);

        // value type
        assert_eq!(global_ty.value_type(), ValType::I32);
        // Mutability
        assert_eq!(global_ty.mutability(), Mutability::Const);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_global_const_i32() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.inner.0.is_null());

        // create a const Global instance
        let result = Global::create(&ty, WasmValue::from_i32(99));
        assert!(result.is_ok());
        let mut global_const = result.unwrap();

        // access the value held by global_const
        assert_eq!(global_const.get_value().to_i32(), 99);
        let result = global_const.set_value(WasmValue::from_i32(0));
        assert!(result.is_err());

        // access the global type
        let result = global_const.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.inner.0.is_null());
        assert!(ty.registered);
        assert_eq!(ty.value_type(), ValType::I32);
        assert_eq!(ty.mutability(), Mutability::Const);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_global_var_f32() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::F32, Mutability::Var);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.inner.0.is_null());

        // create a Var Global instance
        let result = Global::create(&ty, WasmValue::from_f32(13.14));
        assert!(result.is_ok());
        let mut global_var = result.unwrap();

        // access the value held by global_var
        assert_eq!(global_var.get_value().to_f32(), 13.14);
        let result = global_var.set_value(WasmValue::from_f32(1.314));
        assert!(result.is_ok());
        assert_eq!(global_var.get_value().to_f32(), 1.314);

        // access the global type
        let result = global_var.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.inner.0.is_null());
        assert!(ty.registered);
        assert_eq!(ty.value_type(), ValType::F32);
        assert_eq!(ty.mutability(), Mutability::Var);
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_global_conflict() {
        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::F32, Mutability::Var);
            assert!(result.is_ok());
            let ty = result.unwrap();
            assert!(!ty.inner.0.is_null());

            // create a Var Global instance with a value of mis-matched Value::I32 type
            let result = Global::create(&ty, WasmValue::from_i32(520));
            assert!(result.is_err());
        }

        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::F32, Mutability::Var);
            assert!(result.is_ok());
            let ty = result.unwrap();
            assert!(!ty.inner.0.is_null());

            // create a Var Global instance with a value of Value::F32 type
            let result = Global::create(&ty, WasmValue::from_f32(13.14));
            assert!(result.is_ok());
            let mut global_var = result.unwrap();

            // set a new value of mis-matched Value::I32 type
            let result = global_var.set_value(WasmValue::from_i32(1314));
            assert!(result.is_err());
            assert_eq!(global_var.get_value().to_f32(), 13.14);

            // set a new value of Value::F32 type
            let result = global_var.set_value(WasmValue::from_f32(1.314));
            assert!(result.is_ok());
            assert_eq!(global_var.get_value().to_f32(), 1.314);
        }
    }

    #[test]
    fn test_global_send() {
        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::I32, Mutability::Const);
            assert!(result.is_ok());
            let global_ty = result.unwrap();

            let handle = thread::spawn(move || {
                assert!(!global_ty.inner.0.is_null());
                assert!(!global_ty.registered);

                // value type
                assert_eq!(global_ty.value_type(), ValType::I32);
                // Mutability
                assert_eq!(global_ty.mutability(), Mutability::Const);
            });

            handle.join().unwrap()
        }

        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::I32, Mutability::Const);
            assert!(result.is_ok());
            let global_ty = result.unwrap();

            // create a Global instance
            let result = Global::create(&global_ty, WasmValue::from_i32(5));
            assert!(result.is_ok());
            let global = result.unwrap();

            let handle = thread::spawn(move || {
                // access the value held by global
                assert_eq!(global.get_value().to_i32(), 5);
            });

            handle.join().unwrap()
        }
    }

    #[test]
    fn test_global_sync() {
        // create a GlobalType instance
        let result = GlobalType::create(ValType::I32, Mutability::Const);
        assert!(result.is_ok());
        let global_ty = result.unwrap();

        // create a Global instance
        let result = Global::create(&global_ty, WasmValue::from_i32(5));
        assert!(result.is_ok());
        let global = Arc::new(Mutex::new(result.unwrap()));

        let global_cloned = Arc::clone(&global);
        let handle = thread::spawn(move || {
            let result = global_cloned.lock();
            assert!(result.is_ok());
            let global = result.unwrap();

            assert_eq!(global.get_value().to_i32(), 5);
        });

        handle.join().unwrap()
    }

    #[test]
    fn test_global_clone() {
        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::I32, Mutability::Const);
            assert!(result.is_ok());
            let global_ty = result.unwrap();

            // create a Global instance
            let result = Global::create(&global_ty, WasmValue::from_i32(5));
            assert!(result.is_ok());
            let global = result.unwrap();

            let global_cloned = global.clone();

            drop(global);

            assert_eq!(global_cloned.get_value().to_i32(), 5);
        }

        {
            // create a GlobalType instance
            let result = GlobalType::create(ValType::F32, Mutability::Var);
            assert!(result.is_ok());
            let ty = result.unwrap();
            assert!(!ty.inner.0.is_null());

            // create a Var Global instance
            let result = Global::create(&ty, WasmValue::from_f32(13.14));
            assert!(result.is_ok());
            let mut global_var = result.unwrap();
            assert_eq!(global_var.get_value().to_f32(), 13.14);

            let global_var_cloned = global_var.clone();
            assert_eq!(
                global_var_cloned.get_value().to_f32(),
                global_var.get_value().to_f32()
            );

            // access the value held by global_var
            let result = global_var.set_value(WasmValue::from_f32(1.314));
            assert!(result.is_ok());
            assert_eq!(global_var.get_value().to_f32(), 1.314);

            drop(global_var);

            assert_eq!(global_var_cloned.get_value().to_f32(), 1.314);
        }
    }
}
