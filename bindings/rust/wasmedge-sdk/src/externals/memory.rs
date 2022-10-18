use crate::{error::WasmEdgeError, WasmEdgeResult};
use wasmedge_sys as sys;
use wasmedge_types::MemoryType;

/// Defines a linear memory.
#[derive(Debug)]
pub struct Memory {
    pub(crate) inner: sys::Memory,
    pub(crate) name: Option<String>,
    pub(crate) mod_name: Option<String>,
}
impl Memory {
    /// Creates a new wasm memory instance with the given type.
    ///
    /// # Argument
    ///
    /// * `ty` - The type of the memory instance to be created.
    ///
    /// # Error
    ///
    /// If fail to create the memory instance, then an error is returned.
    pub fn new(ty: MemoryType) -> WasmEdgeResult<Self> {
        let inner = sys::Memory::create(&ty.into())?;
        Ok(Self {
            inner,
            name: None,
            mod_name: None,
        })
    }

    /// Returns the exported name of this [Memory].
    ///
    /// Notice that this field is meaningful only if this memory is used as an exported instance.
    pub fn name(&self) -> Option<&str> {
        match &self.name {
            Some(name) => Some(name.as_ref()),
            None => None,
        }
    }

    /// Returns the name of the [module instance](crate::Instance) from which this [Memory] exports.
    ///
    /// Notice that this field is meaningful only if this memory is used as an exported instance.
    pub fn mod_name(&self) -> Option<&str> {
        match &self.mod_name {
            Some(mod_name) => Some(mod_name.as_ref()),
            None => None,
        }
    }

    /// Returns the type of this memory.
    pub fn ty(&self) -> WasmEdgeResult<MemoryType> {
        let ty = self.inner.ty()?;
        Ok(ty.into())
    }

    /// Returns the size, in WebAssembly pages (64 KiB of each page), of this memory.
    pub fn size(&self) -> u32 {
        self.inner.size()
    }

    /// Returns the size, in bytes, of this memory.
    pub fn data_size(&self) -> u64 {
        self.size() as u64 * 65536_u64
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
    pub fn read(&self, offset: u32, len: u32) -> WasmEdgeResult<Vec<u8>> {
        let data = self.inner.get_data(offset, len)?;
        Ok(data)
    }

    /// Returns a string of byte length `len` from `memory`, starting at `offset`.
    ///
    /// # Arguments
    ///
    /// * `offset` - The offset from which to read.
    ///
    /// * `len` - the length of bytes to read.
    ///
    /// # Error
    ///
    /// If fail to read, then an error is returned.
    pub fn read_string(&self, offset: u32, len: u32) -> WasmEdgeResult<String> {
        let slice = self.read(offset, len)?;
        Ok(std::str::from_utf8(&slice)
            .map_err(WasmEdgeError::Utf8)?
            .to_string())
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
    pub fn write(&mut self, data: impl AsRef<[u8]>, offset: u32) -> WasmEdgeResult<()> {
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
    pub fn grow(&mut self, count: u32) -> WasmEdgeResult<()> {
        self.inner.grow(count)?;
        Ok(())
    }

    /// Returns the const data pointer to the [Memory].
    ///
    /// # Arguments
    ///
    /// * `offset` - The data start offset in the [Memory].
    ///
    /// * `len` - The requested data length. If the size of `offset` + `len` is larger
    /// than the data size in the [Memory]
    ///   
    ///
    /// # Errors
    ///
    /// If fail to get the data pointer, then an error is returned.
    ///
    pub fn data_pointer(&self, offset: u32, len: u32) -> WasmEdgeResult<&u8> {
        self.inner.data_pointer(offset, len)
    }

    /// Returns the data pointer to the [Memory].
    ///
    /// # Arguments
    ///
    /// * `offset` - The data start offset in the [Memory].
    ///
    /// * `len` - The requested data length. If the size of `offset` + `len` is larger than the data size in the [Memory]
    ///
    /// # Errors
    ///
    /// If fail to get the data pointer, then an error is returned.
    ///
    pub fn data_pointer_mut(&mut self, offset: u32, len: u32) -> WasmEdgeResult<&mut u8> {
        self.inner.data_pointer_mut(offset, len)
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
    #[allow(clippy::assertions_on_result_states)]
    fn test_memory_type() {
        let result = MemoryType::new(0, None, false);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.minimum(), 0);
        assert_eq!(ty.maximum(), None);

        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let ty = result.unwrap();
        assert_eq!(ty.minimum(), 10);
        assert_eq!(ty.maximum(), Some(20));
    }

    #[test]
    #[allow(clippy::assertions_on_result_states)]
    fn test_memory() {
        // create a memory instance
        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let memory = result.unwrap();

        // create an import object
        let result = ImportObjectBuilder::new()
            .with_memory("memory", memory)
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
        assert_eq!(ty.maximum(), Some(20));

        // read data before write data
        let result = memory.read(0, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![0; 10]);

        // write data
        // ! debug
        let data = vec![1; 10];
        let result = memory.write(data.as_slice(), 10);
        // let result = memory.write(vec![1; 10], 10);
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

    #[test]
    fn test_memory_read() {
        let result = MemoryType::new(10, Some(20), false);
        assert!(result.is_ok());
        let memory_type = result.unwrap();
        let result = Memory::new(memory_type);
        assert!(result.is_ok());
        let mut memory = result.unwrap();

        let result = memory.read(0, 10);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, vec![0; 10]);

        let s = String::from("hello");
        let bytes = s.as_bytes();
        let len = bytes.len();

        let result = memory.write(bytes, 0);
        assert!(result.is_ok());

        let result = memory.read(0, len as u32);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, bytes);

        let result = memory.read_string(0, len as u32);
        assert!(result.is_ok());
        let data = result.unwrap();
        assert_eq!(data, s);
    }
}
