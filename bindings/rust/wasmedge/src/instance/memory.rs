use crate::{error::Result, wasmedge};

#[derive(Debug)]
pub struct Memory<'instance> {
    pub(crate) inner: wasmedge::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Memory<'instance> {
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
        Ok(MemoryType {
            min: ty.limit().start().to_owned(),
            max: ty.limit().end().to_owned(),
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
    max: u32,
}
impl MemoryType {
    pub fn new(min: u32, max: Option<u32>) -> Self {
        let max = match max {
            Some(max) => max,
            None => u32::MAX,
        };
        Self { min, max }
    }

    pub fn minimum(&self) -> u32 {
        self.min
    }

    pub fn maximum(&self) -> u32 {
        self.max
    }

    pub fn to_raw(self) -> Result<wasmedge::MemType> {
        let raw = wasmedge::MemType::create(self.min..=self.max)?;
        Ok(raw)
    }
}
impl From<wasmedge::MemType> for MemoryType {
    fn from(ty: wasmedge::MemType) -> Self {
        let limit = ty.limit();
        Self {
            min: limit.start().to_owned(),
            max: limit.end().to_owned(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        CommonConfigOptions, ConfigBuilder, Executor, ImportModuleBuilder, Statistics, Store,
    };

    #[test]
    fn test_memory_type() {
        let ty = MemoryType::new(0, None);
        assert_eq!(ty.minimum(), 0);
        assert_eq!(ty.maximum(), u32::MAX);

        let ty = MemoryType::new(10, Some(20));
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), 20);
    }

    #[test]
    fn test_memory() {
        // create an ImportModule
        let result = ImportModuleBuilder::new()
            .with_memory("memory", MemoryType::new(10, Some(20)))
            .expect("failed to add memory")
            .build("extern");
        assert!(result.is_ok());
        let import = result.unwrap();

        // create an executor
        let result = ConfigBuilder::new(CommonConfigOptions::default()).build();
        assert!(result.is_ok());
        let config = result.unwrap();

        let result = Statistics::new();
        assert!(result.is_ok());
        let mut stat = result.unwrap();

        let result = Executor::new(Some(&config), Some(&mut stat));
        assert!(result.is_ok());
        let mut executor = result.unwrap();

        // create a store
        let result = Store::new();
        assert!(result.is_ok());
        let mut store = result.unwrap();

        let result = store.named_instance("extern");
        assert!(result.is_none());

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.named_instance("extern");
        assert!(result.is_some());
        let instance = result.unwrap();

        // get the exported memory
        let result = instance.memory("memory");
        assert!(result.is_some());
        let mut memory = result.unwrap();

        // check memory
        assert!(memory.name().is_some());
        assert_eq!(memory.name().unwrap(), "memory");
        assert_eq!(memory.mod_name(), Some("extern"));
        assert_eq!(memory.size(), 10);

        // check memory type
        let result = memory.ty();
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), 20);

        // read data before write data
        let result = memory.read(0, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![0; 10]);

        // write data
        let result = memory.write(vec![1; 10], 10);
        assert!(result.is_ok());
        // read data after write data
        let result = memory.read(10, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![1; 10]);

        // grow memory
        let result = memory.grow(5);
        assert!(result.is_ok());
        assert_eq!(memory.size(), 15);

        // get memory from instance agains
        let result = instance.memory("memory");
        assert!(result.is_some());
        let memory = result.unwrap();
        assert_eq!(memory.size(), 15);
    }
}
