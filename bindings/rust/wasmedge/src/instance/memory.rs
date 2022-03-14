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
