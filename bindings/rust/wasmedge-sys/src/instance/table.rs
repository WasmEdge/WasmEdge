//! Defines WasmEdge Table and TableType structs.
//!
//! A WasmEdge `Table` defines a table described by its `TableType`.
//! `TableType` specifies the limits on the size of a table. The start of
//! the limit range specifies the lower bound (inclusive) of the size, while
//! the end resticts the upper bound (inclusive).

use crate::{
    error::check, types::RefType, wasmedge, TableError, Value, WasmEdgeError, WasmEdgeResult,
};
use std::ops::RangeInclusive;

/// Struct of WasmEdge Table.
///
/// A WasmEdge [`Table`] defines a table described by its [`TableType`].
#[derive(Debug)]
pub struct Table {
    pub(crate) ctx: *mut wasmedge::WasmEdge_TableInstanceContext,
    pub(crate) registered: bool,
}
impl Table {
    /// Creates a new [`Table`] to be associated with the given element type and the size.
    ///
    /// # Arguments
    ///
    /// - `ty` specifies the type of the new [`Table`].
    ///
    /// # Error
    ///
    /// If fail to create a [`Table`], then an error is returned.
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::{RefType, TableType, Table};
    /// // create a TableType instance
    /// let mut ty = TableType::create(RefType::FuncRef, 10..=20).expect("fail to create a TableType");
    ///
    /// // create a Table instance
    /// let table = Table::create(&mut ty).expect("fail to create a Table");
    /// ```
    pub fn create(ty: &mut TableType) -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_TableInstanceCreate(ty.ctx) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::Table(TableError::Create)),
            false => {
                ty.ctx = std::ptr::null_mut();
                ty.registered = true;
                Ok(Table {
                    ctx,
                    registered: false,
                })
            }
        }
    }

    /// Returns the [`TableType`] of the [`Table`].
    ///
    /// # Error
    ///
    /// If fail to get type, then an error is returned.
    pub fn ty(&self) -> WasmEdgeResult<TableType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_TableInstanceGetTableType(self.ctx) };
        match ty_ctx.is_null() {
            true => Err(WasmEdgeError::Table(TableError::Type)),
            false => Ok(TableType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Returns the element value at a specific position in the [`Table`].
    ///
    /// # Arguments
    ///
    /// - `idx` specifies the position in the [`Table`], at which the [`Value`] is returned.
    ///
    /// # Error
    ///
    /// If fail to get the data, then an error is returned.
    pub fn get_data(&self, idx: usize) -> WasmEdgeResult<Value> {
        let raw_val = unsafe {
            let mut data = wasmedge::WasmEdge_ValueGenI32(0);
            check(wasmedge::WasmEdge_TableInstanceGetData(
                self.ctx,
                &mut data as *mut _,
                idx as u32,
            ))?;
            data
        };
        Ok(raw_val.into())
    }

    /// Sets a new element value at a specific position in the [`Table`].
    ///
    /// # Arguments
    ///
    /// - `data` specifies the new data.
    ///
    /// - `idx` specifies the position of the new data to be stored in the [`Table`].
    ///
    /// # Error
    ///
    /// If fail to set data, then an error is returned.
    pub fn set_data(&mut self, data: Value, idx: usize) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_TableInstanceSetData(
                self.ctx,
                data.into(),
                idx as u32,
            ))
        }
    }

    /// Returns the capacity of the [`Table`].
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::{RefType, TableType, Table};
    ///
    /// // create a TableType instance and a Table
    /// let mut ty = TableType::create(RefType::FuncRef, 10..=20).expect("fail to create a TableType");
    /// let table = Table::create(&mut ty).expect("fail to create a Table");
    ///
    /// // check capacity
    /// assert_eq!(table.capacity(), 10);
    /// ```
    ///
    pub fn capacity(&self) -> usize {
        unsafe { wasmedge::WasmEdge_TableInstanceGetSize(self.ctx) as usize }
    }

    /// Increases the capacity of the [`Table`].
    ///
    /// After growing, the new capacity must be in the range defined by `limit` when the table is created.
    ///
    /// # Argument
    ///
    /// - `size` specifies the size to be added to the [`Table`].
    ///
    /// # Error
    ///
    /// If fail to increase the size of the [`Table`], then an error is returned.
    pub fn grow(&mut self, size: u32) -> WasmEdgeResult<()> {
        unsafe { check(wasmedge::WasmEdge_TableInstanceGrow(self.ctx, size)) }
    }
}
impl Drop for Table {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe {
                wasmedge::WasmEdge_TableInstanceDelete(self.ctx);
            }
        }
    }
}

/// Struct of WasmEdge TableType
///
/// A WasmEdge [`TableType`] classify a [`Table`] over elements of element types within a size range.
#[derive(Debug)]
pub struct TableType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_TableTypeContext,
    pub(crate) registered: bool,
}
impl Drop for TableType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe {
                wasmedge::WasmEdge_TableTypeDelete(self.ctx);
            }
        }
    }
}
impl TableType {
    /// Creates a new [`TableType`] to be associated with the given limit range of the size and the reference type.
    ///
    /// # Arguments
    ///
    /// - `ref_type` specifies the element type.
    ///
    /// - `limit` specifies a range of the table size.
    ///
    /// # Error
    ///
    /// If fail to create a [`TableType`], then an error is returned.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let ty = TableType::create(RefType::FuncRef, 10..=20).expect("fail to create a TableType");
    /// ```
    ///
    pub fn create(elem_ty: RefType, limit: RangeInclusive<u32>) -> WasmEdgeResult<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_TableTypeCreate(
                wasmedge::WasmEdge_RefType::from(elem_ty),
                wasmedge::WasmEdge_Limit::from(limit),
            )
        };
        match ctx.is_null() {
            true => Err(WasmEdgeError::TableTypeCreate),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the element type.
    pub fn elem_ty(&self) -> RefType {
        let ty = unsafe { wasmedge::WasmEdge_TableTypeGetRefType(self.ctx) };
        ty.into()
    }

    /// Returns a range of the limit size of a [`Table`].
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::{RefType, TableType};
    ///
    /// // create a TableType instance
    /// let ty = TableType::create(RefType::FuncRef, 10..=20).expect("fail to create a TableType");
    ///
    /// // check limit
    /// assert_eq!(ty.limit(), 10..=20);
    /// ```
    pub fn limit(&self) -> RangeInclusive<u32> {
        let limit = unsafe { wasmedge::WasmEdge_TableTypeGetLimit(self.ctx) };
        limit.into()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::RefType;

    #[test]
    fn test_tabletype() {
        // create a TableType instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        // check element type
        assert_eq!(ty.elem_ty(), RefType::FuncRef);
        // check limit
        assert_eq!(ty.limit(), 10..=20);
    }

    #[test]
    fn test_table() {
        // create a TableType instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        // create a Table instance
        let result = Table::create(&mut ty);
        assert!(result.is_ok());
        assert!(ty.ctx.is_null());
        assert!(ty.registered);
        let mut table = result.unwrap();
        assert!(!table.ctx.is_null());
        assert!(!table.registered);

        // check capacity
        assert_eq!(table.capacity(), 10);

        // get type
        let result = table.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(ty.registered);

        // check limit and element type
        assert_eq!(ty.limit(), 10..=20);
        assert_eq!(ty.elem_ty(), RefType::FuncRef);

        // grow the capacity of table
        let result = table.grow(5);
        assert!(result.is_ok());
        // check capacity
        assert_eq!(table.capacity(), 15);
    }

    #[test]
    fn test_table_data() {
        // create a TableType instance
        let result = TableType::create(RefType::FuncRef, 10..=20);
        assert!(result.is_ok());
        let mut ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        // create a Table instance
        let result = Table::create(&mut ty);
        assert!(result.is_ok());
        assert!(ty.ctx.is_null());
        assert!(ty.registered);
        let mut table = result.unwrap();
        assert!(!table.ctx.is_null());
        assert!(!table.registered);

        // check capacity
        assert_eq!(table.capacity(), 10);

        // get data in the scope of the capacity
        let result = table.get_data(9);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert_eq!(value, Value::FuncRef(0));

        // set data
        let result = table.set_data(Value::FuncRef(5), 3);
        assert!(result.is_ok());
        // get data
        let result = table.get_data(3);
        assert!(result.is_ok());
        let value = result.unwrap();
        assert_eq!(value, Value::FuncRef(5));
    }
}
