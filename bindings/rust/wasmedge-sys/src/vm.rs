use super::wasmedge;
use crate::value::Value;
use crate::raw_result::{ErrReport, decode_result};
use crate::string::StringRef;


/// A WasmEdge VM instance.
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    _private: ()
}

impl Vm {
    pub fn create(
        config: &crate::config::Config,
    ) -> Result<Self, ErrReport> {  
        let ctx = unsafe {
            wasmedge::WasmEdge_VMCreate(config.ctx, std::ptr::null_mut() /* store */)
        };
        if ctx.is_null() {
            return Err(ErrReport::default());
        }
        Ok(Self { ctx , _private: ()})
    }

    pub fn load_wasm_from_ast_module(self, module: &crate::module::Module) -> Result<Self, ErrReport> {
        unsafe{
            decode_result(wasmedge::WasmEdge_VMLoadWasmFromASTModule(self.ctx, module.ctx))?;
        }
        Ok(self)
    }

    pub fn validate(self) -> Result<Self, ErrReport> {
        unsafe{
            decode_result(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(self)
    }

    pub fn instantiate(self) -> Result<Self, ErrReport> {
        unsafe{
            decode_result(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(self)
    }

    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<Vec<Value>, ErrReport> {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();

        let raw_params: Vec<_> = params
            .as_ref()
            .iter()
            .copied()
            .map(wasmedge::WasmEdge_Value::from)
            .collect();
        
        let func_type = unsafe { wasmedge::WasmEdge_VMGetFunctionType(self.ctx, raw_func_name) };
        if func_type.is_null() {
            return Err(ErrReport::default());
        }

        let returns_len = unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);
        
        unsafe {
            decode_result(wasmedge::WasmEdge_VMExecute(
                self.ctx,
                raw_func_name,
                raw_params.as_ptr(),
                raw_params.len() as u32,
                returns.as_mut_ptr(),
                returns_len,
            ))?;
            returns.set_len(returns_len as usize);
        }
        Ok(returns.into_iter().map(Into::into).collect())
    }

}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
    }
}

