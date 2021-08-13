pub use wasmedge_sys as ffi;

use crate::value::Value;

/// A WasmEdge VM instance.
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut ffi::WasmEdge_VMContext,
}

impl Vm {
    pub fn create(
        module: &crate::module::Module,
        config: &crate::config::Config,
    ) -> Result<Self, Error> {
        let ctx = unsafe {
            ffi::WasmEdge_VMCreate(config.ctx, std::ptr::null_mut() /* store */)
        };
        if ctx.is_null() {
            return Err(Error::Create);
        }

        unsafe {
            ffi::decode_result(ffi::WasmEdge_VMLoadWasmFromASTModule(ctx, module.ctx))
                .map_err(Error::ModuleLoad)?;
            // The following two calls could be made lazily if they're expensive.
            ffi::decode_result(ffi::WasmEdge_VMValidate(ctx)).map_err(Error::Validate)?;
            ffi::decode_result(ffi::WasmEdge_VMInstantiate(ctx)).map_err(Error::Instantiate)?;
        }

        Ok(Self { ctx })
    }

    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<Vec<Value>, Error> {
        let raw_func_name: ffi::WasmEdge_String =
            crate::string::StringRef::from(func_name.as_ref()).into();

        let raw_params: Vec<_> = params
            .as_ref()
            .iter()
            .copied()
            .map(ffi::WasmEdge_Value::from)
            .collect();

        let func_type = unsafe { ffi::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(Error::MissingFunction(func_name.as_ref().to_string()));
        }
        let num_returns = unsafe { ffi::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(num_returns as usize);

        unsafe {
            ffi::decode_result(ffi::WasmEdge_VMExecute(
                self.ctx,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                num_returns,
            ))
            .map_err(Error::Execute)?;
            returns.set_len(num_returns as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect())
    }
}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { ffi::WasmEdge_VMDelete(self.ctx) };
    }
}

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("failed to create WasmEdge VM")]
    Create,

    #[error("module loading failed: {}", _0.message)]
    ModuleLoad(ffi::Error),

    #[error("module validation failed: {}", _0.message)]
    Validate(ffi::Error),

    #[error("module instantiation failed: {}", _0.message)]
    Instantiate(ffi::Error),

    #[error("could not find function `{0}` in module")]
    MissingFunction(String),

    #[error("module execution failed: {}", _0.message)]
    Execute(ffi::Error),
}
