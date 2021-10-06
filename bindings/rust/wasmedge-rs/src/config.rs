use super::wasmedge;

#[derive(Debug)]
pub struct Config {
    inner: wasmedge::Config,
}

impl Config {
    pub fn init() -> wasmedge::Config {
        wasmedge::Config::default()
    }

    pub fn with_wasi() -> wasmedge::Config {
        let config = Self::init();
        config.enable_wasi()
    }
}
