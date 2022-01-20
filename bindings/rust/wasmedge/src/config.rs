use crate::{error::WasmEdgeResult, wasmedge};

#[derive(Debug)]
pub struct ConfigBuilder {
    inner: wasmedge::Config,
}
impl ConfigBuilder {
    pub fn new() -> WasmEdgeResult<Self> {
        let inner = wasmedge::Config::create()?;
        Ok(Self { inner })
    }

    pub fn with_wasi(self) -> Self {
        let inner = self.inner.wasi(true);
        Self { inner }
    }

    pub fn with_wasmedge_process(self) -> Self {
        let inner = self.inner.wasmedge_process(true);
        Self { inner }
    }

    pub fn build(self) -> Config {
        Config { inner: self.inner }
    }
}

#[derive(Debug)]
pub struct Config {
    pub(crate) inner: wasmedge::Config,
}
impl Config {
    pub fn wasi_enabled(&self) -> bool {
        self.inner.wasi_enabled()
    }

    pub fn wasmedge_process_enabled(&self) -> bool {
        self.inner.wasmedge_process_enabled()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_config_create() {
        {
            let config = ConfigBuilder::new()
                .expect("fail to create ConfigBuilder")
                .build();
            dbg!(config);
        }

        {
            let config = ConfigBuilder::new()
                .expect("fail to create a ConfigBuilder")
                .with_wasi()
                .with_wasmedge_process()
                .build();

            assert!(config.wasi_enabled());
            assert!(config.wasmedge_process_enabled());
        }
    }
}
