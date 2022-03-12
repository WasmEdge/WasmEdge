use crate::{error::Result, wasmedge, RefType, Value};

#[derive(Debug)]
pub struct Table<'instance> {
    pub(crate) inner: wasmedge::Table,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Table<'instance> {
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

    /// Returns the underlying type of this table, including its element type as well as the maximum/minimum lower
    /// bounds.
    pub fn ty(&self) -> Result<TableType> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(TableType {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        })
    }

    /// Returns the current size of this table.
    pub fn size(&self) -> u32 {
        self.inner.capacity() as u32
    }

    /// Grows the size of this table by `size` more elements
    pub fn grow(&mut self, size: u32) -> Result<()> {
        self.inner.grow(size)?;
        Ok(())
    }

    /// Returns the table element value at `index`.
    pub fn get(&self, index: u32) -> Result<Value> {
        let value = self.inner.get_data(index)?;
        Ok(value)
    }

    /// Writes the `data` provided into `index` within this table.
    pub fn set(&mut self, data: Value, index: u32) -> Result<()> {
        self.inner.set_data(data, index)?;
        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableType {
    elem_ty: RefType,
    min: u32,
    max: Option<u32>,
}
impl TableType {
    pub fn new(elem_ty: RefType, min: u32, max: Option<u32>) -> Self {
        Self { elem_ty, min, max }
    }

    pub fn elem_ty(&self) -> RefType {
        self.elem_ty
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> Option<u32> {
        self.max
    }

    pub fn to_raw(self) -> Result<wasmedge::TableType> {
        let min = self.minimum();
        let max = match self.maximum() {
            Some(max) => max,
            None => u32::MAX,
        };
        let raw = wasmedge::TableType::create(self.elem_ty(), min..=max)?;
        Ok(raw)
    }
}
impl From<wasmedge::TableType> for TableType {
    fn from(ty: wasmedge::TableType) -> Self {
        let limit = ty.limit();
        Self {
            elem_ty: ty.elem_ty(),
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        }
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::{RefType, ValType};

//     #[test]
//     fn test_table_type() {
//         // create a TableType instance
//         let ty = TableType::new(RefType::FuncRef, 10, Some(20));

//         // check element type
//         assert_eq!(ty.elem_ty(), RefType::FuncRef);
//         // check minimum
//         assert_eq!(ty.minimum(), 10);
//         // check maximum
//         assert_eq!(ty.maximum(), Some(20));
//     }

//     #[test]
//     fn test_table() {
//         {
//             // create a TableType instance
//             let ty = TableType::new(RefType::FuncRef, 10, None);

//             // create a Table instance
//             let result = Table::new(ty);
//             assert!(result.is_ok());
//             let mut table = result.unwrap();

//             // check capacity
//             assert_eq!(table.capacity(), 10);

//             // get type
//             let result = table.ty();
//             assert!(result.is_ok());
//             let ty = result.unwrap();

//             // check element type
//             assert_eq!(ty.elem_ty(), RefType::FuncRef);
//             // check minimum
//             assert_eq!(ty.minimum(), 10);
//             // check maximum
//             assert_eq!(ty.maximum(), Some(u32::MAX));

//             // grow the capacity of table
//             let result = table.grow(5);
//             assert!(result.is_ok());
//             // check capacity
//             assert_eq!(table.capacity(), 15);
//         }

//         {
//             // create a TableType instance
//             let ty = TableType::new(RefType::FuncRef, 10, Some(20));

//             // create a Table instance
//             let result = Table::new(ty);
//             assert!(result.is_ok());
//             let mut table = result.unwrap();

//             // check capacity
//             assert_eq!(table.capacity(), 10);

//             // get type
//             let result = table.ty();
//             assert!(result.is_ok());
//             let ty = result.unwrap();

//             // check element type
//             assert_eq!(ty.elem_ty(), RefType::FuncRef);
//             // check minimum
//             assert_eq!(ty.minimum(), 10);
//             // check maximum
//             assert_eq!(ty.maximum(), Some(20));

//             // grow the capacity of table
//             let result = table.grow(5);
//             assert!(result.is_ok());
//             // check capacity
//             assert_eq!(table.capacity(), 15);
//         }
//     }

//     #[test]
//     fn test_table_data() {
//         // create a TableType instance
//         let ty = TableType::new(RefType::FuncRef, 10, Some(20));

//         // create a Table instance
//         let result = Table::new(ty);
//         assert!(result.is_ok());
//         let mut table = result.unwrap();

//         // check capacity
//         assert_eq!(table.capacity(), 10);

//         // get data in the scope of the capacity
//         let result = table.get_data(9);
//         assert!(result.is_ok());
//         let value = result.unwrap();
//         assert!(value.is_null_ref());
//         assert_eq!(value.ty(), ValType::FuncRef);

//         // set data
//         let result = table.set_data(Value::from_func_ref(5), 3);
//         assert!(result.is_ok());
//         // get data
//         let result = table.get_data(3);
//         assert!(result.is_ok());
//         let idx = result.unwrap().func_idx();
//         assert!(idx.is_some());
//         assert_eq!(idx.unwrap(), 5);
//     }
// }
