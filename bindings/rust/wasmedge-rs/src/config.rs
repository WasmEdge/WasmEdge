use super::wasmedge;

#[derive(Debug, Default)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}

impl Config {
    pub fn with_wasi() -> Self {
        let config = Self::default();
        let inner: wasmedge::Config = config.inner.enable_wasi();
        Self{inner}
    }
}
