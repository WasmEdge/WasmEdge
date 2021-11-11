use super::wasmedge;

use std::{convert::TryInto, ffi::CString, marker::PhantomData};

#[derive(Clone, Debug)]
pub enum WasmEdgeString<'a> {
    Owned(StringBuf),
    Borrowed(StringRef<'a>),
}

impl PartialEq for WasmEdgeString<'_> {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (WasmEdgeString::Owned(b), WasmEdgeString::Borrowed(r))
            | (WasmEdgeString::Borrowed(r), WasmEdgeString::Owned(b)) => unsafe {
                wasmedge::WasmEdge_StringIsEqual(
                    wasmedge::WasmEdge_StringWrap(b.inner.Buf, b.inner.Length),
                    r.inner,
                )
            },
            (WasmEdgeString::Owned(b), WasmEdgeString::Owned(other_b)) => b == other_b,
            (WasmEdgeString::Borrowed(r), WasmEdgeString::Borrowed(other_r)) => r == other_r,
        }
    }
}

impl Drop for WasmEdgeString<'_> {
    fn drop(&mut self) {
        if let Self::Owned(buf) = self {
            unsafe { wasmedge::WasmEdge_StringDelete(buf.inner) };
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

    pub fn into_bytes(self) -> Vec<u8> {
        match self {
            Self::Owned(o) => o.into_bytes(),
            Self::Borrowed(b) => b.into_bytes(),
        }
    }
}

impl From<StringBuf> for WasmEdgeString<'_> {
    fn from(str_buf: StringBuf) -> Self {
        Self::Owned(str_buf)
    }
}

impl From<CString> for WasmEdgeString<'_> {
    fn from(s: CString) -> Self {
        let buf: StringBuf = s.into();
        buf.into()
    }
}

impl<'a, T: Into<StringRef<'a>>> From<T> for WasmEdgeString<'a> {
    fn from(str_ref: T) -> Self {
        Self::Borrowed(str_ref.into())
    }
}

impl Default for WasmEdgeString<'_> {
    fn default() -> Self {
        Self::Owned(Default::default())
    }
}

impl Default for wasmedge::WasmEdge_String {
    fn default() -> Self {
        wasmedge::WasmEdge_String {
            Length: 0,
            Buf: std::ptr::null(),
        }
    }
}

#[derive(Debug, Default, Copy)]
pub struct StringBuf {
    pub(crate) inner: wasmedge::WasmEdge_String,
}

impl PartialEq for StringBuf {
    fn eq(&self, other: &Self) -> bool {
        unsafe {
            wasmedge::WasmEdge_StringIsEqual(
                wasmedge::WasmEdge_StringWrap(self.inner.Buf, self.inner.Length),
                wasmedge::WasmEdge_StringWrap(other.inner.Buf, other.inner.Length),
            )
        }
    }
}

impl<'a> StringBuf {
    pub fn as_ref(&'a self) -> StringRef<'a> {
        StringRef {
            inner: unsafe { wasmedge::WasmEdge_StringWrap(self.inner.Buf, self.inner.Length) },
            lifetime: PhantomData,
        }
    }

    pub fn into_bytes(self) -> Vec<u8> {
        let mut vec = Vec::with_capacity(
            self.inner
                .Length
                .try_into()
                .expect("buffer size should smaller than the platform usize"),
        );
        unsafe {
            wasmedge::WasmEdge_StringCopy(self.inner, vec.as_mut_ptr(), self.inner.Length);
        }
        vec.into_iter().map(|i| i as u8).collect()
    }
}

impl From<String> for StringBuf {
    fn from(s: String) -> Self {
        Self {
            // Safety:  String is valid utf-8 encoding in Rust
            inner: unsafe {
                wasmedge::WasmEdge_StringWrap(s.as_ptr() as *const i8, s.len() as u32)
            },
        }
    }
}

impl From<CString> for StringBuf {
    fn from(s: CString) -> Self {
        Self {
            inner: unsafe { wasmedge::WasmEdge_StringCreateByCString(s.as_ptr()) },
        }
    }
}

impl Clone for StringBuf {
    fn clone(&self) -> Self {
        Self {
            inner: unsafe {
                wasmedge::WasmEdge_StringCreateByBuffer(self.inner.Buf, self.inner.Length)
            },
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub struct StringRef<'a> {
    pub(crate) inner: wasmedge::WasmEdge_String,
    lifetime: PhantomData<&'a ()>,
}

impl PartialEq for StringRef<'_> {
    fn eq(&self, other: &Self) -> bool {
        unsafe { wasmedge::WasmEdge_StringIsEqual(self.inner, other.inner) }
    }
}

impl StringRef<'_> {
    pub fn to_owned(self) -> StringBuf {
        StringBuf {
            inner: unsafe {
                wasmedge::WasmEdge_StringCreateByBuffer(self.inner.Buf, self.inner.Length)
            },
        }
    }

    pub fn into_bytes(self) -> Vec<u8> {
        self.to_owned().into_bytes()
    }
}

impl<'a> From<&'a str> for StringRef<'a> {
    fn from(s: &'a str) -> Self {
        Self {
            inner: unsafe {
                wasmedge::WasmEdge_StringWrap(s.as_ptr() as *const i8, s.len() as u32)
            },
            lifetime: PhantomData,
        }
    }
}

impl From<StringRef<'_>> for wasmedge::WasmEdge_String {
    fn from(s: StringRef<'_>) -> Self {
        s.inner
    }
}
