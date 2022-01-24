use crate::{error::WasmEdgeResult, wasmedge};
use std::ops::RangeInclusive;

#[derive(Debug)]
pub struct Memory {
    pub(crate) inner: wasmedge::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Memory {
    pub fn new(minimum: u32, maximum: Option<u32>) -> WasmEdgeResult<Self> {
        let maximum = match maximum {
            Some(max_val) => max_val,
            None => u32::MAX,
        };

        let inner = wasmedge::Memory::create(minimum..=maximum)?;

        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

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

    /// Returns the lower bound of the memory page counts allowed to be used.
    pub fn minimum(&self) -> WasmEdgeResult<u32> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(limit.start().to_owned())
    }

    /// Returns the upper bound of the memory page counts allowed to grow.
    pub fn maximum(&self) -> WasmEdgeResult<u32> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(limit.end().to_owned())
    }

    pub fn page_count(&self) -> u32 {
        self.inner.page_count()
    }

    pub fn get_data(&self, offset: u32, len: u32) -> WasmEdgeResult<Vec<u8>> {
        unimplemented!()
    }

    pub fn set_data(
        &mut self,
        data: impl IntoIterator<Item = u8>,
        offset: u32,
    ) -> WasmEdgeResult<()> {
        unimplemented!()
    }

    pub fn data_pointer(&self, offset: u32, len: u32) -> WasmEdgeResult<&u8> {
        unimplemented!()
    }

    pub fn data_pointer_mut(&mut self, offset: u32, len: u32) -> WasmEdgeResult<&mut u8> {
        unimplemented!()
    }

    pub fn grow(&mut self, count: u32) -> WasmEdgeResult<()> {
        self.inner.grow(count)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    // use crate::error::{CoreError, CoreExecutionError, WasmEdgeError};

    #[test]
    fn test_memory_create() {
        {
            let result = Memory::new(10, None);
            assert!(result.is_ok());
            let mem = result.unwrap();

            // check limit
            let result = mem.minimum();
            assert!(result.is_ok());
            assert_eq!(result.unwrap(), 10);

            let result = mem.maximum();
            assert!(result.is_ok());
            assert_eq!(result.unwrap(), u32::MAX);

            // check page count
            assert_eq!(mem.page_count(), 10);
        }

        {
            let result = Memory::new(10, Some(20));
            assert!(result.is_ok());
            let mem = result.unwrap();

            // check limit
            let result = mem.minimum();
            assert!(result.is_ok());
            assert_eq!(result.unwrap(), 10);

            let result = mem.maximum();
            assert!(result.is_ok());
            assert_eq!(result.unwrap(), 20);

            // check page count
            assert_eq!(mem.page_count(), 10);
        }
    }

    // #[test]
    // fn test_memory_grow() {
    //     // create a Memory with a limit range [10, 20]
    //     let result = MemType::create(10..=20);
    //     assert!(result.is_ok());
    //     let mut ty = result.unwrap();
    //     let result = Memory::create(&mut ty);
    //     assert!(result.is_ok());
    //     let mut mem = result.unwrap();
    //     assert!(!mem.ctx.is_null());
    //     assert!(!mem.registered);

    //     // get type
    //     let result = mem.ty();
    //     assert!(result.is_ok());
    //     let ty = result.unwrap();
    //     assert!(!ty.ctx.is_null());
    //     assert!(ty.registered);
    //     // check limit
    //     assert_eq!(ty.limit(), 10..=20);

    //     // check page count
    //     let count = mem.page_count();
    //     assert_eq!(count, 10);

    //     // grow 5 pages
    //     let result = mem.grow(10);
    //     assert!(result.is_ok());
    //     assert_eq!(mem.page_count(), 20);

    //     // grow additional  pages, which causes a failure
    //     let result = mem.grow(1);
    //     assert!(result.is_err());
    // }

    // #[test]
    // fn test_memory_data() {
    //     // create a Memory: the min size 1 and the max size 2
    //     let result = MemType::create(1..=2);
    //     assert!(result.is_ok());
    //     let mut ty = result.unwrap();
    //     let result = Memory::create(&mut ty);
    //     assert!(result.is_ok());
    //     let mut mem = result.unwrap();
    //     assert!(!mem.ctx.is_null());
    //     assert!(!mem.registered);

    //     // check page count
    //     let count = mem.page_count();
    //     assert_eq!(count, 1);

    //     // get data before set data
    //     let result = mem.get_data(0, 10);
    //     assert!(result.is_ok());
    //     let data: Vec<_> = result.unwrap().collect();
    //     assert_eq!(data, vec![0; 10]);

    //     // set data
    //     let result = mem.set_data(vec![1; 10], 10);
    //     assert!(result.is_ok());
    //     // get data after set data
    //     let result = mem.get_data(10, 10);
    //     assert!(result.is_ok());
    //     let data: Vec<_> = result.unwrap().collect();
    //     assert_eq!(data, vec![1; 10]);

    //     // set data and the data length is larger than the data size in the memory
    //     let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
    //     assert!(result.is_err());
    //     assert_eq!(
    //         result.unwrap_err(),
    //         WasmEdgeError::Core(CoreError::Execution(CoreExecutionError::MemoryOutOfBounds))
    //     );

    //     // grow the memory size
    //     let result = mem.grow(1);
    //     assert!(result.is_ok());
    //     assert_eq!(mem.page_count(), 2);
    //     let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
    //     assert!(result.is_ok());
    // }
}
