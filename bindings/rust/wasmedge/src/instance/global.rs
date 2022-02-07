use crate::{error::Result, wasmedge, Mutability, ValType, Value};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Global<'store> {
    pub(crate) inner: wasmedge::Global,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: PhantomData<&'store ()>,
}
impl<'store> Global<'store> {
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

    pub fn get_value(&self) -> Value {
        self.inner.get_value()
    }

    pub fn set_value(&mut self, val: Value) -> Result<()> {
        self.inner.set_value(val)?;
        Ok(())
    }
}

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

    pub fn to_raw(self) -> Result<wasmedge::GlobalType> {
        let raw = wasmedge::GlobalType::create(self.value_ty(), self.mutability())?;
        Ok(raw)
    }
}
impl From<wasmedge::GlobalType> for GlobalType {
    fn from(ty: wasmedge::GlobalType) -> Self {
        Self {
            ty: ty.value_type(),
            mutability: ty.mutability(),
        }
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::{error::WasmEdgeError, Mutability, ValType};

//     #[test]
//     fn test_global_type() {
//         // create a GlobalType instance
//         let global_ty = GlobalType::new(ValType::I32, Mutability::Const);

//         // value type
//         assert_eq!(global_ty.value_ty(), ValType::I32);
//         // Mutability
//         assert_eq!(global_ty.mutability(), Mutability::Const);
//     }

//     #[test]
//     fn test_global_const_i32() {
//         // create a GlobalType instance
//         let global_ty = GlobalType::new(ValType::I32, Mutability::Const);

//         // create a const Global instance
//         let result = Global::new(global_ty, Value::from_i32(99));
//         assert!(result.is_ok());
//         let mut global_const = result.unwrap();

//         // access the value held by global_const
//         assert_eq!(global_const.get_value().to_i32(), 99);
//         let result = global_const.set_value(Value::from_i32(0));
//         assert!(result.is_err());
//         assert_eq!(
//             result.unwrap_err(),
//             WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Global(
//                 wasmedge::error::GlobalError::ModifyConst
//             ))
//         );

//         // access the global type
//         let result = global_const.ty();
//         assert!(result.is_ok());
//         let global_ty = result.unwrap();
//         assert_eq!(global_ty.value_ty(), ValType::I32);
//         assert_eq!(global_ty.mutability(), Mutability::Const);
//     }

//     #[test]
//     fn test_global_var_f32() {
//         // create a GlobalType instance
//         let global_ty = GlobalType::new(ValType::F32, Mutability::Var);

//         // create a Var Global instance
//         let result = Global::new(global_ty, Value::from_f32(13.14));
//         assert!(result.is_ok());
//         let mut global_var = result.unwrap();

//         // access the value held by global_var
//         assert_eq!(global_var.get_value().to_f32(), 13.14);
//         let result = global_var.set_value(Value::from_f32(1.314));
//         assert!(result.is_ok());
//         assert_eq!(global_var.get_value().to_f32(), 1.314);

//         // access the global type
//         let result = global_var.ty();
//         assert!(result.is_ok());
//         let global_ty = result.unwrap();
//         assert_eq!(global_ty.value_ty(), ValType::F32);
//         assert_eq!(global_ty.mutability(), Mutability::Var);
//     }

//     #[test]
//     fn test_global_conflict() {
//         // create a GlobalType instance
//         let global_ty = GlobalType::new(ValType::F32, Mutability::Var);

//         // create a Var Global instance with a value of mis-matched Value::I32 type
//         let result = Global::new(global_ty, Value::from_i32(520));
//         assert!(result.is_err());
//         assert_eq!(
//             result.unwrap_err(),
//             WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Global(
//                 wasmedge::error::GlobalError::Create
//             ))
//         );

//         // create a GlobalType instance
//         let global_ty = GlobalType::new(ValType::F32, Mutability::Var);

//         // create a Var Global instance with a value of Value::F32 type
//         let result = Global::new(global_ty, Value::from_f32(13.14));
//         assert!(result.is_ok());
//         let mut global_var = result.unwrap();

//         // set a new value of mis-matched Value::I32 type
//         let result = global_var.set_value(Value::from_i32(1314));
//         assert!(result.is_err());
//         assert_eq!(
//             result.unwrap_err(),
//             WasmEdgeError::Operation(wasmedge::error::WasmEdgeError::Global(
//                 wasmedge::error::GlobalError::UnmatchedValType
//             ))
//         );
//         assert_eq!(global_var.get_value().to_f32(), 13.14);

//         // set a new value of Value::F32 type
//         let result = global_var.set_value(Value::from_f32(1.314));
//         assert!(result.is_ok());
//         assert_eq!(global_var.get_value().to_f32(), 1.314);
//     }
// }
