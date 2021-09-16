use super::wasmedge;

#[derive(Debug)]
pub struct Config {
    inner: wasmedge::Config,
}

impl Config {
    pub fn default() -> wasmedge::Config {
        wasmedge::Config::default()
    }

    pub fn with_wasi() -> wasmedge::Config {
        let config = Self::default();
        config.reg_wasi()
    }
}
