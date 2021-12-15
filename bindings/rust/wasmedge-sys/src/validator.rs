use crate::{error::check, wasmedge, Config, Error, Module, WasmEdgeResult};

#[derive(Debug)]
pub struct Validator {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ValidatorContext,
}
impl Validator {
    /// Create a Validator instance.
    pub fn create(config: &Config) -> WasmEdgeResult<Self> {
        let ctx = unsafe { wasmedge::WasmEdge_ValidatorCreate(config.ctx) };
        match ctx.is_null() {
            true => Err(Error::OperationError(String::from(
                "fail to create Validator instance",
            ))),
            false => Ok(Self { ctx }),
        }
    }

    /// Validate the WasmEdge Module instance.
    pub fn validate(&self, module: &Module) -> WasmEdgeResult<()> {
        unsafe { check(wasmedge::WasmEdge_ValidatorValidate(self.ctx, module.ctx)) }
    }
}
impl Drop for Validator {
    fn drop(&mut self) {
        if !self.ctx.is_null() {
            unsafe { wasmedge::WasmEdge_ValidatorDelete(self.ctx) }
        }
    }
}
