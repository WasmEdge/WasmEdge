use std::marker::PhantomData;

use wasmedge_sys as ffi;

#[derive(Clone, Debug)]
pub enum WasmEdgeString<'a> {
    Owned(StringBuf),
    Borrowed(StringRef<'a>),
}

impl WasmEdgeString<'_> {
    pub(crate) fn raw(&self) -> ffi::WasmEdge_String {
        match self {
            Self::Owned(StringBuf { inner, .. }) | Self::Borrowed(StringRef { inner, .. }) => {
                *inner
            }
        }
    }
}

impl Drop for WasmEdgeString<'_> {
    fn drop(&mut self) {
        if let Self::Owned(buf) = self {
            unsafe { ffi::WasmEdge_StringDelete(buf.inner) };
        }
    }
}

impl WasmEdgeString<'_> {
    pub fn into_owned(self) -> Self {
        match self {
            Self::Owned(_) => self,
            Self::Borrowed(b) => Self::Owned(b.to_owned()),
        }
    }
}

impl From<String> for WasmEdgeString<'_> {
    fn from(s: String) -> Self {
        Self::Owned(StringBuf {
            inner: unsafe { ffi::WasmEdge_StringWrap(s.as_ptr() as *const i8, s.len() as u32) },
        })
    }
}

impl<'a> From<&'a str> for WasmEdgeString<'a> {
    fn from(s: &'a str) -> Self {
        Self::Borrowed(StringRef {
            inner: unsafe { ffi::WasmEdge_StringWrap(s.as_ptr() as *const i8, s.len() as u32) },
            lifetime: PhantomData,
        })
    }
}

impl Default for WasmEdgeString<'_> {
    fn default() -> Self {
        Self::Owned(Default::default())
    }
}

#[derive(Debug, Default)]
pub struct StringBuf {
    inner: ffi::WasmEdge_String,
}

impl Clone for StringBuf {
    fn clone(&self) -> Self {
        Self {
            inner: unsafe { ffi::WasmEdge_StringCreateByBuffer(self.inner.Buf, self.inner.Length) },
        }
    }
}

impl<'a> StringBuf {
    pub fn as_ref(&'a self) -> StringRef<'a> {
        StringRef {
            inner: unsafe { ffi::WasmEdge_StringWrap(self.inner.Buf, self.inner.Length) },
            lifetime: PhantomData,
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub struct StringRef<'a> {
    inner: ffi::WasmEdge_String,
    lifetime: PhantomData<&'a ()>,
}

impl StringRef<'_> {
    pub fn to_owned(self) -> StringBuf {
        StringBuf {
            inner: unsafe { ffi::WasmEdge_StringCreateByBuffer(self.inner.Buf, self.inner.Length) },
        }
    }
}
