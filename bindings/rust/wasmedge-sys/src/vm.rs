use super::wasmedge;
use crate::{
    raw_result::{decode_result, ErrReport},
    string::StringRef,
    value::Value,
};

// Since `wasmedge-sys` is also a standalone crate
// if someone else wants to rely on it to implement a third-party sdk
// then we expect that the third-party sdk will only use the interface we want it to use.
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    _private: (),
}

#[derive(Debug)]
struct VmParams {
    vm_ctx: *mut wasmedge::WasmEdge_VMContext,
    raw_func_name: wasmedge::WasmEdge_String,
    raw_params: *const wasmedge::WasmEdge_Value,
    raw_params_len: u32,
    func_type: *mut wasmedge::WasmEdge_FunctionTypeContext,
}

impl Vm {
    pub fn create(
        module: &crate::module::Module,
        config: &crate::config::Config,
    ) -> Result<Self, ErrReport> {
        let ctx = unsafe {
            wasmedge::WasmEdge_VMCreate(config.ctx, std::ptr::null_mut() /* store */)
        };
        if ctx.is_null() {
            return Err(ErrReport::default());
        }

        Ok(Self { ctx, _private: () })
    }

    pub fn load_wasm_from_ast_module(
        self,
        module: &crate::module::Module,
    ) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMLoadWasmFromASTModule(
                self.ctx, module.ctx,
            ))?;
        }
        Ok(self)
    }

    pub fn validate(self) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMValidate(self.ctx))?;
        }
        Ok(self)
    }

    pub fn instantiate(self) -> Result<Self, ErrReport> {
        unsafe {
            decode_result(wasmedge::WasmEdge_VMInstantiate(self.ctx))?;
        }
        Ok(self)
    }

    fn construct_params(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<VmParams, ErrReport> {
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

        Ok(VmParams {
            vm_ctx: self.ctx,
            raw_func_name: raw_func_name,
            raw_params: raw_params.as_ptr(),
            raw_params_len: raw_params.len() as u32,
            func_type: func_type,
        })
    }

    fn vm_exec_returns(&mut self, vm_params: VmParams) -> Result<Vec<Value>, ErrReport> {
        let returns_len =
            unsafe { wasmedge::WasmEdge_FunctionTypeGetReturnsLength(vm_params.func_type) };
        let mut returns = Vec::with_capacity(returns_len as usize);

        unsafe {
            decode_result(wasmedge::WasmEdge_VMExecute(
                vm_params.vm_ctx,
                vm_params.raw_func_name,
                vm_params.raw_params,
                vm_params.raw_params_len,
                returns.as_mut_ptr(),
                returns_len,
            ))?;

            returns.set_len(returns_len as usize);
        }

        Ok(returns.into_iter().map(Into::into).collect())
    }

    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<Vec<Value>, ErrReport> {
        let vm_params = self.construct_params(func_name, params)?;
        println!("{:#?}", vm_params);
        let returns = self.vm_exec_returns(vm_params)?;

        println!("---> {:#?}", returns);
        Ok(returns)
    }
}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
    }
}
