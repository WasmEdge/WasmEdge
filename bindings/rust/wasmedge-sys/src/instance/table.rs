use crate::{error::check, types::WasmEdgeRefType, wasmedge, Error, Value, WasmEdgeResult};
use std::ops::Range;

#[derive(Debug)]
pub struct Table {
    pub(crate) ctx: *mut wasmedge::WasmEdge_TableInstanceContext,
    pub(crate) registered: bool,
}
impl Table {
    pub fn create(ref_type: WasmEdgeRefType, limit: Range<u32>) -> WasmEdgeResult<Self> {
        let ctx = unsafe {
            let table_ty = wasmedge::WasmEdge_TableTypeCreate(
                wasmedge::WasmEdge_RefType::from(ref_type),
                wasmedge::WasmEdge_Limit::from(limit),
            );
            wasmedge::WasmEdge_TableInstanceCreate(table_ty)
        };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Table instance",
            ))),
            false => Ok(Table {
                ctx,
                registered: false,
            }),
        }
    }

    /// Get the table type from a table instance.
    pub fn get_type(&self) -> WasmEdgeResult<TableType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_TableInstanceGetTableType(self.ctx) };
        match ty_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get type info from the Table instance",
            ))),
            false => Ok(TableType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Get the reference value in a table instance.
    pub fn get_data(&self, offset: usize) -> WasmEdgeResult<Value> {
        let ptr_raw_val = std::ptr::null_mut();
        unsafe {
            check(wasmedge::WasmEdge_TableInstanceGetData(
                self.ctx,
                ptr_raw_val,
                offset as u32,
            ))?;
        }
        let ref_raw_val = unsafe { ptr_raw_val.as_ref().unwrap() };
        let raw_val = ref_raw_val.to_owned();
        Ok(raw_val.into())
    }

    /// Set the reference value into a table instance.
    pub fn set_data(&mut self, data: Value, offset: usize) -> WasmEdgeResult<()> {
        unsafe {
            check(wasmedge::WasmEdge_TableInstanceSetData(
                self.ctx,
                data.into(),
                offset as u32,
            ))
        }
    }

    /// Get the capacity of a table instance.
    pub fn capacity(&self) -> usize {
        unsafe { wasmedge::WasmEdge_TableInstanceGetSize(self.ctx) as usize }
    }

    /// Grow a table instance with a size.
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
    pub fn create(ref_type: WasmEdgeRefType, limit: Range<u32>) -> WasmEdgeResult<Self> {
        let ctx = unsafe {
            wasmedge::WasmEdge_TableTypeCreate(
                wasmedge::WasmEdge_RefType::from(ref_type),
                wasmedge::WasmEdge_Limit::from(limit),
            )
        };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create a TableType instance",
            ))),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Get the reference type from a table type.
    pub fn ref_type(&self) -> WasmEdgeRefType {
        let ty = unsafe { wasmedge::WasmEdge_TableTypeGetRefType(self.ctx) };
        ty.into()
    }

    /// Get the limit from a table type.
    pub fn limit(&self) -> Range<u32> {
        let limit = unsafe { wasmedge::WasmEdge_TableTypeGetLimit(self.ctx) };
        limit.into()
    }
}
