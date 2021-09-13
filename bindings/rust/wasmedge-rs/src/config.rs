use super::wasmedge;

#[derive(Debug)]
pub struct Config {
    inner: wasmedge::Config,
}

impl Config {
    pub fn default() -> wasmedge::Config {
        wasmedge::Config::default()
    }
}
