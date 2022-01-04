//! Defines WasmEdge Memory and MemType structs.
//!
//! A WasmEdge `Memory` defines a linear memory as described by `MemType`.
//! `MemType` specifies the limits on the size of a memory by a range. The start of
//! the limit range specifies min size (initial size) of that memory, while the end
//! restricts the size to which the memory can grow later.

use crate::{
    error::{check, MemError},
    wasmedge, WasmEdgeError, WasmEdgeResult,
};
use std::ops::RangeInclusive;

/// Struct of WasmEdge Memory.
///
/// A WasmEdge [`Memory`] defines a linear memory as described by [`MemType`].
#[derive(Debug)]
pub struct Memory {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryInstanceContext,
    pub(crate) registered: bool,
}
impl Memory {
    /// Create a new [`Memory`] to be associated with the given capacity limit.
    ///
    /// # Arguments
    ///
    /// - `limit` specifies a inclusive range of the page count in the [`Memory`]. For example,
    /// (10..=20) means the lower bound (inclusive) of the page count is 10 ,while the upper bound (inclusive) 20.
    ///
    /// # Errors
    ///
    /// If fail to create a [`Memory`], then an error is returned.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let memory = Memory::create(10..=20);
    ///
    /// ```
    ///
    ///
    pub fn create(limit: RangeInclusive<u32>) -> WasmEdgeResult<Self> {
        let mut mem_ty = MemType::create(limit)?;
        let ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceCreate(mem_ty.ctx) };
        mem_ty.ctx = std::ptr::null_mut();
        match ctx.is_null() {
            true => Err(WasmEdgeError::Mem(MemError::Create)),
            false => Ok(Memory {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the type of the [`Memory`].
    ///
    /// # Errors
    ///
    /// If fail to get the type from the [`Memory`], then an error is returned.
    ///
    pub fn ty(&self) -> WasmEdgeResult<MemType> {
        let ty_ctx = unsafe { wasmedge::WasmEdge_MemoryInstanceGetMemoryType(self.ctx) };
        match ty_ctx.is_null() {
            true => Err(WasmEdgeError::Mem(MemError::Type)),
            false => Ok(MemType {
                ctx: ty_ctx as *mut _,
                registered: true,
            }),
        }
    }

    /// Copies the data from the [`Memory`] to the output buffer.
    ///
    /// # Arguments
    ///
    /// - `offset` specifies the data start offset in the [`Memory`].
    ///
    /// - `len` specifies the requested data length.
    ///
    /// # Errors
    ///
    /// If the `offset` + `len` is larger than the data size in the [`Memory`], then an error is returned.
    ///
    pub fn get_data(&self, offset: u32, len: u32) -> WasmEdgeResult<impl Iterator<Item = u8>> {
        let mut data = Vec::with_capacity(len as usize);
        unsafe {
            check(wasmedge::WasmEdge_MemoryInstanceGetData(
                self.ctx,
                data.as_mut_ptr(),
                offset,
                len,
            ))?;
            data.set_len(len as usize);
        }

        Ok(data.into_iter())
    }

    /// Copies the data from the given input buffer into the [`Memory`].
    ///
    /// # Arguments
    ///
    /// - `data` specifies the data buffer to copy.
    ///
    /// - `offset` specifies the data start offset in the [`Memory`].
    ///
    /// # Errors
    ///
    /// If the sum of the `offset` and the data length is larger than the size of the [`Memory`],
    /// then an error is returned.
    ///
    /// ```
    /// use wasmedge_sys::{error::{CoreError, CoreExecutionError}, WasmEdgeError, Memory};
    ///
    /// // create a Memory: the min size 1 and the max size 2
    /// let mut mem = Memory::create(1..=2).expect("fail to create a Memory");
    ///
    /// // set data and the data length is larger than the data size in the memory
    /// let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
    /// assert!(result.is_err());
    /// assert_eq!(result.unwrap_err(), WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::MemoryOutOfBounds)));
    /// ```
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::Memory;
    ///
    /// // create a Memory: the min size 1 and the max size 2
    /// let mut mem = Memory::create(1..=2).expect("fail to create a Memory");
    /// // page count
    /// let count = mem.page_count();
    /// assert_eq!(count, 1);
    ///
    /// // set data
    /// mem.set_data(vec![1; 10], 10).expect("fail to set data");
    ///
    /// // get data
    /// let data = mem.get_data(10, 10).expect("fail to get data");
    /// let data: Vec<_> = data.collect();
    /// assert_eq!(data, vec![1; 10]);
    /// ```
    ///
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

    /// Returns the const data pointer to the [`Memory`].
    ///
    /// # Arguments
    ///
    /// - `offset` specifies the data start offset in the [`Memory`].
    ///
    /// - `len` specifies the requested data length. If the size of `offset` + `len` is larger
    /// than the data size in the [`Memory`]
    ///   
    ///
    /// # Errors
    ///
    /// If fail to get the data pointer, then an error is returned.
    ///
    pub fn data_pointer(&self, offset: u32, len: u32) -> WasmEdgeResult<&u8> {
        let ptr =
            unsafe { wasmedge::WasmEdge_MemoryInstanceGetPointerConst(self.ctx, offset, len) };
        match ptr.is_null() {
            true => Err(WasmEdgeError::Mem(MemError::ConstPtr)),
            false => {
                let result = unsafe { ptr.as_ref() };
                match result {
                    Some(ptr) => Ok(ptr),
                    None => Err(WasmEdgeError::Mem(MemError::Ptr2Ref)),
                }
            }
        }
    }

    /// Returns the data pointer to the [`Memory`].
    ///
    /// # Arguments
    ///
    /// - `offset` specifies the data start offset in the [`Memory`].
    ///
    /// - `len` specifies the requested data length. If the size of `offset` + `len` is larger
    /// than the data size in the [`Memory`]
    ///
    /// # Errors
    ///
    /// If fail to get the data pointer, then an error is returned.
    ///
    pub fn data_pointer_mut(&mut self, offset: u32, len: u32) -> WasmEdgeResult<&mut u8> {
        let ptr = unsafe { wasmedge::WasmEdge_MemoryInstanceGetPointer(self.ctx, offset, len) };
        match ptr.is_null() {
            true => Err(WasmEdgeError::Mem(MemError::MutPtr)),
            false => {
                let result = unsafe { ptr.as_mut() };
                match result {
                    Some(ptr) => Ok(ptr),
                    None => Err(WasmEdgeError::Mem(MemError::Ptr2Ref)),
                }
            }
        }
    }

    /// Returns the page count (64 KiB of each page).
    pub fn page_count(&self) -> u32 {
        unsafe { wasmedge::WasmEdge_MemoryInstanceGetPageSize(self.ctx) as u32 }
    }

    /// Grows the page count of the [`Memory`].
    ///
    /// # Arguments
    ///
    /// - `count` specifies the page counts to be extended to the [`Memory`].
    ///
    /// # Errors
    ///
    /// If fail to grow the page count, then an error is returned.
    ///
    /// ```
    /// use wasmedge_sys::Memory;
    ///
    /// // create a Memory with a limit range [10, 20]
    /// let mut mem = Memory::create(1..=1).expect("fail to create a Memory");
    /// // check page count
    /// let count = mem.page_count();
    /// assert_eq!(count, 1);
    ///
    /// // grow one more page, which causes a failure
    /// let result = mem.grow(1);
    /// assert!(result.is_err());
    /// ```
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::Memory;
    ///
    /// // create a Memory with a limit range [10, 20]
    /// let mut mem = Memory::create(10..=20).expect("fail to create a Memory");
    /// // check page count
    /// let count = mem.page_count();
    /// assert_eq!(count, 10);
    ///
    /// // grow 5 pages
    /// mem.grow(10).expect("fail to grow the page count");
    /// assert_eq!(mem.page_count(), 20);
    /// ```
    ///
    pub fn grow(&mut self, count: u32) -> WasmEdgeResult<()> {
        unsafe { check(wasmedge::WasmEdge_MemoryInstanceGrowPage(self.ctx, count)) }
    }
}
impl Drop for Memory {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryInstanceDelete(self.ctx) };
        }
    }
}

/// Struct of WasmEdge MemType.
///
/// A [`MemType`] classifies a [`Memory`] and its size range.
#[derive(Debug)]
pub struct MemType {
    pub(crate) ctx: *mut wasmedge::WasmEdge_MemoryTypeContext,
    pub(crate) registered: bool,
}
impl MemType {
    /// Create a new [`MemType`] to be associated with the given limit range for the capacity.
    ///
    /// # Arguments
    ///
    /// - `limit` specifies the linear memory size. The start value of the limit range
    /// specifies the min size (also, initial size) of the memory, while the end value specifies
    /// the max size allowed to grow.
    ///
    /// # Errors
    ///
    /// If fail to create a [`MemType`], then an error is returned.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let ty = MemType::create(0..=u32::MAX);
    /// ```
    ///
    pub fn create(limit: RangeInclusive<u32>) -> WasmEdgeResult<Self> {
        let ctx =
            unsafe { wasmedge::WasmEdge_MemoryTypeCreate(wasmedge::WasmEdge_Limit::from(limit)) };
        match ctx.is_null() {
            true => Err(WasmEdgeError::MemTypeCreate),
            false => Ok(Self {
                ctx,
                registered: false,
            }),
        }
    }

    /// Returns the limit range of a [`MemType`].
    ///
    /// # Example
    ///
    /// ```
    /// use wasmedge_sys::MemType;
    ///
    /// let ty = MemType::create(0..=u32::MAX).expect("fail to create a MemType");
    /// assert_eq!(ty.limit(), 0..=u32::MAX);
    /// ```
    ///
    pub fn limit(&self) -> RangeInclusive<u32> {
        let limit = unsafe { wasmedge::WasmEdge_MemoryTypeGetLimit(self.ctx) };
        RangeInclusive::from(limit)
    }
}
impl Drop for MemType {
    fn drop(&mut self) {
        if !self.registered && !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_MemoryTypeDelete(self.ctx) }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::error::{CoreError, CoreExecutionError, WasmEdgeError};

    #[test]
    fn test_memtype() {
        let result = MemType::create(0..=u32::MAX);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        let limit = ty.limit();
        assert_eq!(limit, 0..=u32::MAX);

        let result = MemType::create(10..=101);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(!ty.registered);

        let limit = ty.limit();
        assert_eq!(limit, 10..=101);
    }

    #[test]
    fn test_memory_grow() {
        // create a Memory with a limit range [10, 20]
        let result = Memory::create(10..=20);
        assert!(result.is_ok());
        let mut mem = result.unwrap();
        assert!(!mem.ctx.is_null());
        assert!(!mem.registered);

        // get type
        let result = mem.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert!(!ty.ctx.is_null());
        assert!(ty.registered);
        // check limit
        assert_eq!(ty.limit(), 10..=20);

        // check page count
        let count = mem.page_count();
        assert_eq!(count, 10);

        // grow 5 pages
        let result = mem.grow(10);
        assert!(result.is_ok());
        assert_eq!(mem.page_count(), 20);

        // grow additional  pages, which causes a failure
        let result = mem.grow(1);
        assert!(result.is_err());
    }

    #[test]
    fn test_memory_data() {
        // create a Memory: the min size 1 and the max size 2
        let result = Memory::create(1..=2);
        assert!(result.is_ok());
        let mut mem = result.unwrap();
        assert!(!mem.ctx.is_null());
        assert!(!mem.registered);

        // check page count
        let count = mem.page_count();
        assert_eq!(count, 1);

        // get data before set data
        let result = mem.get_data(0, 10);
        assert!(result.is_ok());
        let data: Vec<_> = result.unwrap().collect();
        assert_eq!(data, vec![0; 10]);

        // set data
        let result = mem.set_data(vec![1; 10], 10);
        assert!(result.is_ok());
        // get data after set data
        let result = mem.get_data(10, 10);
        assert!(result.is_ok());
        let data: Vec<_> = result.unwrap().collect();
        assert_eq!(data, vec![1; 10]);

        // set data and the data length is larger than the data size in the memory
        let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::MemoryOutOfBounds))
        );

        // grow the memory size
        let result = mem.grow(1);
        assert!(result.is_ok());
        assert_eq!(mem.page_count(), 2);
        let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
        assert!(result.is_ok());
    }
}
