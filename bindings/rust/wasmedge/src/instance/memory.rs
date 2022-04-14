use crate::{error::Result, wasmedge};
use std::marker::PhantomData;

#[derive(Debug)]
pub struct Memory<'store> {
    pub(crate) inner: wasmedge::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: PhantomData<&'store ()>,
}
impl<'store> Memory<'store> {
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

    /// Returns the underlying type of this memory.
    pub fn ty(&self) -> Result<MemoryType> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(MemoryType {
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        })
    }

    /// Returns the size, in WebAssembly pages, of this wasm memory.
    pub fn size(&self) -> u32 {
        self.inner.size()
    }

    /// Safely reads memory contents at the given offset into a buffer.
    pub fn read(&self, offset: u32, len: u32) -> Result<Vec<u8>> {
        let data = self.inner.get_data(offset, len)?;
        Ok(data)
    }

    /// Safely writes contents of a buffer to this memory at the given offset.
    pub fn write(&mut self, data: impl IntoIterator<Item = u8>, offset: u32) -> Result<()> {
        self.inner.set_data(data, offset)?;
        Ok(())
    }

    /// Grows this WebAssembly memory by `count` pages.
    pub fn grow(&mut self, count: u32) -> Result<()> {
        self.inner.grow(count)?;
        Ok(())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MemoryType {
    min: u32,
    max: Option<u32>,
}
impl MemoryType {
    pub fn new(min: u32, max: Option<u32>) -> Self {
        Self { min, max }
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> Option<u32> {
        self.max
    }

    pub fn to_raw(self) -> Result<wasmedge::MemType> {
        let min = self.minimum();
        let max = match self.maximum() {
            Some(max) => max,
            None => u32::MAX,
        };
        let raw = wasmedge::MemType::create(min..=max)?;
        Ok(raw)
    }
}
impl From<wasmedge::MemType> for MemoryType {
    fn from(ty: wasmedge::MemType) -> Self {
        let limit = ty.limit();
        Self {
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        }
    }
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::{error::WasmEdgeError, wasmedge};

//     #[test]
//     fn test_memory_create() {
//         {
//             // create a MemoryType instance
//             let ty = MemoryType::new(10, None);

//             // create a Memory instance
//             let result = Memory::new(ty);
//             assert!(result.is_ok());
//             let mem = result.unwrap();

//             let result = mem.ty();
//             assert!(result.is_ok());
//             let ty = result.unwrap();

//             // check memory limit
//             assert_eq!(ty.minimum(), 10);
//             assert_eq!(ty.maximum().unwrap(), u32::MAX);

//             // check page count
//             assert_eq!(mem.size(), 10);
//         }

//         {
//             // create a MemoryType instance
//             let ty = MemoryType::new(10, Some(20));

//             // create a Memory instance
//             let result = Memory::new(ty);
//             assert!(result.is_ok());
//             let mem = result.unwrap();

//             let result = mem.ty();
//             assert!(result.is_ok());
//             let ty = result.unwrap();

//             // check memory limit
//             assert_eq!(ty.minimum(), 10);
//             assert_eq!(ty.maximum().unwrap(), 20);

//             // check page count
//             assert_eq!(mem.size(), 10);
//         }
//     }

//     #[test]
//     fn test_memory_grow() {
//         // create a MemoryType instance
//         let ty = MemoryType::new(10, Some(20));

//         // create a Memory instance
//         let result = Memory::new(ty);
//         assert!(result.is_ok());
//         let mut mem = result.unwrap();

//         // get type
//         let result = mem.ty();
//         assert!(result.is_ok());
//         let ty = result.unwrap();
//         // check limit
//         assert_eq!(ty.minimum(), 10);
//         assert_eq!(ty.maximum().unwrap(), 20);

//         // check page count
//         let count = mem.size();
//         assert_eq!(count, 10);

//         // grow 5 pages
//         let result = mem.grow(10);
//         assert!(result.is_ok());
//         assert_eq!(mem.size(), 20);

//         // grow additional  pages, which causes a failure
//         let result = mem.grow(1);
//         assert!(result.is_err());
//     }

//     #[test]
//     fn test_memory_data() {
//         // create a MemoryType instance
//         let ty = MemoryType::new(1, Some(2));

//         // create a Memory: the min size 1 and the max size 2
//         let result = Memory::new(ty);
//         assert!(result.is_ok());
//         let mut mem = result.unwrap();

//         // check page count
//         let count = mem.size();
//         assert_eq!(count, 1);

//         // get data before set data
//         let result = mem.read(0, 10);
//         assert!(result.is_ok());
//         let data = result.unwrap();
//         assert_eq!(data, vec![0; 10]);

//         // set data
//         let result = mem.write(vec![1; 10], 10);
//         assert!(result.is_ok());
//         // get data after set data
//         let result = mem.read(10, 10);
//         assert!(result.is_ok());
//         let data = result.unwrap();
//         assert_eq!(data, vec![1; 10]);

//         // set data and the data length is larger than the data size in the memory
//         let result = mem.write(vec![1; 10], u32::pow(2, 16) - 9);
//         assert!(result.is_err());
//         assert_eq!(
//             result.unwrap_err(),
//             WasmEdgeError::Operation(wasmedge::WasmEdgeError::Core(
//                 wasmedge::error::CoreError::Execution(
//                     wasmedge::error::CoreExecutionError::MemoryOutOfBounds
//                 )
//             ))
//         );

//         // grow the memory size
//         let result = mem.grow(1);
//         assert!(result.is_ok());
//         assert_eq!(mem.size(), 2);
//         let result = mem.write(vec![1; 10], u32::pow(2, 16) - 9);
//         assert!(result.is_ok());
//     }
// }
