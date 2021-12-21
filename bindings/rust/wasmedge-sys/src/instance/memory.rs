use crate::{error::check, wasmedge, Error, WasmEdgeResult};
use std::ops::Range;

#[derive(Debug)]
pub struct Memory {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    pub(crate) registered: bool,
}
impl Memory {
    /// Create a Memory instance
    pub fn create(limit: Range<u32>) -> WasmEdgeResult<Self> {
        let mut mem_ty = MemType::create(limit)?;
        let ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceCreate(mem_ty.ctx) };
        mem_ty.ctx = std::ptr::null_mut();
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Memory instance",
            ))),
            false => Ok(Memory {
                ctx,
                registered: false,
            }),
        }
    }

    /// Get the memory type from a memory instance.
    pub fn get_type(&self) -> WasmEdgeResult<MemType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceGetMemoryType(self.ctx) };
        match ty_ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get type info from a Memory instance",
            ))),
            false => Ok(MemType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Copy the data to the output buffer from a memory instance.
    pub fn get_data(&self, offset: u32, size: u32) -> WasmEdgeResult<impl Iterator<Item = u8>> {
        let mut data = Vec::with_capacity(size as usize);
        unsafe {
            check(wasmedge::WasmEdge_MemoryInstanceGetData(
                self.ctx,
                data.as_mut_ptr(),
                offset,
                size,
            ))?;
            data.set_len(size as usize);
        }

        Ok(data.into_iter())
    }

    /// Copy the data into a memory instance from the input buffer.
    pub fn set_data(
        &mut self,
        data: impl IntoIterator<Item = u8>,
        offset: u32,
    ) -> WasmEdgeResult<()> {
        let data = data.into_iter().collect::<Vec<u8>>();
        unsafe {
            check(wasmedge::WasmEdge_MemoryInstanceSetData(
                self.ctx,
                data.as_ptr() as *mut _,
                offset,
                data.len() as u32,
            ))
        }
    }

    /// Get the const data pointer in a const memory instance.
    pub fn data_pointer(&self, offset: u32, size: u32) -> WasmEdgeResult<&u8> {
        let ptr =
            unsafe { wasmedge::WasmEdge_MemoryInstanceGetPointerConst(self.ctx, offset, size) };
        match ptr.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the const data pointer in the memory instance",
            ))),
            false => {
                let result = unsafe { ptr.as_ref() };
                match result {
                    Some(ptr) => Ok(ptr),
                    None => Err(Error::OperationError(String::from(
                        "fail to convert the raw const data pointer into a reference",
                    ))),
                }
            }
        }
    }

    /// Get the data pointer in a memory instance.
    pub fn data_pointer_mut(&mut self, offset: u32, size: u32) -> WasmEdgeResult<&mut u8> {
        let ptr = unsafe { wasmedge::WasmEdge_MemoryInstanceGetPointer(self.ctx, offset, size) };
        match ptr.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to get the data pointer in the memory instance",
            ))),
            false => {
                let result = unsafe { ptr.as_mut() };
                match result {
                    Some(ptr) => Ok(ptr),
                    None => Err(Error::OperationError(String::from(
                        "fail to convert the raw data pointer into a reference",
                    ))),
                }
            }
        }
    }

    /// Get the current page size (64 KiB of each page) of a memory instance.
    pub fn page_size(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_MemoryInstanceGetPageSize(self.ctx) as u32 }
    }

    /// Grow the page size.
    pub fn grow_page(&mut self, page: u32) -> WasmEdgeResult<()> {
        unsafe { check(wasmedge::WasmEdge_MemoryInstanceGrowPage(self.ctx, page)) }
    }
}
impl Drop for Memory {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryInstanceDelete(self.ctx) };
        }
    }
}

#[derive(Debug)]
pub struct MemType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryTypeContext,
    pub(crate) registered: bool,
}
impl MemType {
    pub fn create(limit: Range<u32>) -> WasmEdgeResult<Self> {
        let ctx =
            unsafe { wasmedge::WasmEdge_MemoryTypeCreate(wasmedge::WasmEdge_Limit::from(limit)) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create MemType instance",
            ))),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    pub fn limit(&self) -> Range<u32> {
        let limit = unsafe { wasmedge::WasmEdge_MemoryTypeGetLimit(self.ctx) };
        Range::from(limit)
    }
}
impl Drop for MemType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryTypeDelete(self.ctx) }
        }
    }
}
