use super::wasmedge;
use crate::error::ConfigError;

#[derive(Debug)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}
impl Config {
    pub fn create() -> Result<Self, ConfigError> {
        let inner = wasmedge::Config::create();
        match inner {
            Ok(inner) => Ok(Self { inner }),
            Err(e) => Err(ConfigError::Creation(e)),
        }
    }
}
impl Config {
    pub fn with_wasi() -> Result<Self, ConfigError> {
        let result = Self::create();
        match result {
            Ok(config) => {
                let inner: wasmedge::Config = config.inner.enable_wasi();
                Ok(Self { inner })
            }
            Err(e) => Err(e),
        }
    }
}
