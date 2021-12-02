use super::wasmedge;

#[derive(Debug)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}
impl Config {
    pub fn create() -> Option<Self> {
        let inner = wasmedge::Config::create();
        match inner {
            Ok(inner) => Some(Self { inner }),
            Err(_) => None,
        }
    }
}
impl Config {
    pub fn with_wasi() -> Option<Self> {
        let result = Self::create();
        match result {
            Some(config) => {
                let inner: wasmedge::Config = config.inner.enable_wasi();
                Some(Self { inner })
            }
            None => None,
        }
    }
}
