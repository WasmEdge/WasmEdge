use crate::error::Result;
use wasmedge_sys as sys;
use wasmedge_types::MemoryType;

/// Struct of WasmEdge Memory.
///
/// A WasmEdge [Memory] defines a linear memory.
#[derive(Debug)]
pub struct Memory<'instance> {
    pub(crate) inner: sys::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
    pub(crate) _marker: std::marker::PhantomData<&'instance ()>,
}
impl<'instance> Memory<'instance> {
    /// Returns the exported name of this [Memory].
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this [Memory] exports.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of this memory.
    pub fn ty(&self) -> Result<MemoryType> {
        let ty = self.inner.ty()?;
        Ok(ty.into())
    }

    /// Returns the size, in WebAssembly pages, of this memory.
    pub fn size(&self) -> u32 {
        self.inner.size()
    }

    /// Returns the size, in bytes, of this memory.
    pub fn data_size(&self) -> u64 {
        self.size() as u64 * 65536 as u64
    }

    /// Safely reads memory contents at the given offset into a buffer.
    ///
    /// # Arguments
    ///
    /// * `offset` - The offset from which to read.
    ///
    /// * `len` - the length of bytes to read.
    ///
    /// # Error
    ///
    /// If fail to read the memory, then an error is returned.
    pub fn read(&self, offset: u32, len: u32) -> Result<Vec<u8>> {
        let data = self.inner.get_data(offset, len)?;
        Ok(data)
    }

    /// Safely writes contents of a buffer to this memory at the given offset.
    ///
    /// # Arguments
    ///
    /// * `data` - The bytes to write to this memory..
    ///
    /// * `offset` - The offset at which to write.
    ///
    /// # Error
    ///
    /// If fail to write to the memory, then an error is returned.
    pub fn write(&mut self, data: impl IntoIterator<Item = u8>, offset: u32) -> Result<()> {
        self.inner.set_data(data, offset)?;
        Ok(())
    }

    /// Grows this WebAssembly memory by `count` pages.
    ///
    /// # Argument
    ///
    /// * `count` - The number of pages to grow the memory by.
    ///
    /// # Error
    ///
    /// If fail to grow the memory, then an error is returned.
    pub fn grow(&mut self, count: u32) -> Result<()> {
        self.inner.grow(count)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        config::{CommonConfigOptions, ConfigBuilder},
        Executor, ImportObjectBuilder, Statistics, Store,
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
        let result = ImportObjectBuilder::new()
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

        let result = store.module_instance("extern");
        assert!(result.is_none());

        let result = store.register_import_module(&mut executor, &import);
        assert!(result.is_ok());

        let result = store.module_instance("extern");
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
