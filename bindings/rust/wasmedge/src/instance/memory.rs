use crate::wasmedge;

pub struct Memory {
    inner: wasmedge::Memory,
}
