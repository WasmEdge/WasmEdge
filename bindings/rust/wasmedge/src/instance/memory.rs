use crate::{error::WasmEdgeResult, wasmedge};

#[derive(Debug)]
pub struct Memory {
    pub(crate) inner: wasmedge::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Memory {
    pub fn new(ty: MemoryType) -> WasmEdgeResult<Self> {
        let min = ty.minimum();
        let max = match ty.maximum() {
            Some(max) => max,
            None => u32::MAX,
        };
        let mut ty = wasmedge::MemType::create(min..=max)?;
        let inner = wasmedge::Memory::create(&mut ty)?;

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

    pub fn ty(&self) -> WasmEdgeResult<MemoryType> {
        let ty = self.inner.ty()?;
        let limit = ty.limit();
        Ok(MemoryType {
            min: limit.start().to_owned(),
            max: Some(limit.end().to_owned()),
        })
    }

    pub fn page_count(&self) -> u32 {
        self.inner.page_count()
    }

    pub fn get_data(&self, offset: u32, len: u32) -> WasmEdgeResult<Vec<u8>> {
        let data = self.inner.get_data(offset, len)?;
        Ok(data)
    }

    pub fn set_data(
        &mut self,
        data: impl IntoIterator<Item = u8>,
        offset: u32,
    ) -> WasmEdgeResult<()> {
        self.inner.set_data(data, offset)?;
        Ok(())
    }

    pub fn grow(&mut self, count: u32) -> WasmEdgeResult<()> {
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{error::WasmEdgeError, wasmedge};

    #[test]
    fn test_memory_create() {
        {
            // create a MemoryType instance
            let ty = MemoryType::new(10, None);

            // create a Memory instance
            let result = Memory::new(ty);
            assert!(result.is_ok());
            let mem = result.unwrap();

            let result = mem.ty();
            assert!(result.is_ok());
            let ty = result.unwrap();

            // check memory limit
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum().unwrap(), u32::MAX);

            // check page count
            assert_eq!(mem.page_count(), 10);
        }

        {
            // create a MemoryType instance
            let ty = MemoryType::new(10, Some(20));

            // create a Memory instance
            let result = Memory::new(ty);
            assert!(result.is_ok());
            let mem = result.unwrap();

            let result = mem.ty();
            assert!(result.is_ok());
            let ty = result.unwrap();

            // check memory limit
            assert_eq!(ty.minimum(), 10);
            assert_eq!(ty.maximum().unwrap(), 20);

            // check page count
            assert_eq!(mem.page_count(), 10);
        }
    }

    #[test]
    fn test_memory_grow() {
        // create a MemoryType instance
        let ty = MemoryType::new(10, Some(20));

        // create a Memory instance
        let result = Memory::new(ty);
        assert!(result.is_ok());
        let mut mem = result.unwrap();

        // get type
        let result = mem.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        // check limit
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum().unwrap(), 20);

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
        // create a MemoryType instance
        let ty = MemoryType::new(1, Some(2));

        // create a Memory: the min size 1 and the max size 2
        let result = Memory::new(ty);
        assert!(result.is_ok());
        let mut mem = result.unwrap();

        // check page count
        let count = mem.page_count();
        assert_eq!(count, 1);

        // get data before set data
        let result = mem.get_data(0, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![0; 10]);

        // set data
        let result = mem.set_data(vec![1; 10], 10);
        assert!(result.is_ok());
        // get data after set data
        let result = mem.get_data(10, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![1; 10]);

        // set data and the data length is larger than the data size in the memory
        let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
        assert!(result.is_err());
        assert_eq!(
            result.unwrap_err(),
            WasmEdgeError::Operation(wasmedge::WasmEdgeError::Core(
                wasmedge::error::CoreError::Execution(
                    wasmedge::error::CoreExecutionError::MemoryOutOfBounds
                )
            ))
        );

        // grow the memory size
        let result = mem.grow(1);
        assert!(result.is_ok());
        assert_eq!(mem.page_count(), 2);
        let result = mem.set_data(vec![1; 10], u32::pow(2, 16) - 9);
        assert!(result.is_ok());
    }
}
