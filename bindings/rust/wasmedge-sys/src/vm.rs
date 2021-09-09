use super::wasmedge;
use crate::value::Value;
use crate::raw_result::{ErrReport, decode_result};
use crate::string::StringRef;


/// A WasmEdge VM instance.
/// Not allowed to be constructed downstream
#[derive(Debug)]
pub struct Vm {
    pub(crate) ctx: *mut wasmedge::WasmEdge_VMContext,
    _private: ()
}

struct VmExecParams{
    vm_ctx: *mut wasmedge::WasmEdge_VMContext,
    raw_func_name: wasmedge::WasmEdge_String,
    raw_params: *const wasmedge::WasmEdge_Value,
    raw_params_len: u32,
    returns_values: *mut wasmedge::WasmEdge_Value,
    returns_len: u32,
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

    fn construct_func(&mut self, func_name: impl AsRef<str>, params: &[Value]) -> Result<VmExecParams, ErrReport> {
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
        
        Ok(VmExecParams {
            vm_ctx: self.ctx,
            raw_func_name: raw_func_name,
            raw_params: raw_params.as_ptr(),
            raw_params_len: raw_params.len() as u32,
            returns_values: returns.as_mut_ptr(),
            returns_len: returns_len,
        })
    }


    pub fn run(
        &mut self,
        func_name: impl AsRef<str>,
        params: &[Value],
    ) -> Result<Vec<Value>, ErrReport> {
        
        let vm_exec_params = self.construct_func(func_name, params)?;
        let returns_len = vm_exec_params.returns_len;
        let returns_p = vm_exec_params.returns_values;

        unsafe {
            decode_result(wasmedge::WasmEdge_VMExecute(
                vm_exec_params.vm_ctx,
                vm_exec_params.raw_func_name,
                vm_exec_params.raw_params,
                vm_exec_params.raw_params_len,
                vm_exec_params.returns_values,
                returns_len,
            ))?;
        }

        let (returns_len, returns_cap) = (returns_len as usize, returns_len as usize);
        let returns = unsafe {
            Vec::from_raw_parts(returns_p, returns_len, returns_cap)
        };

        Ok(returns.into_iter().map(Into::into).collect())
    }
}

impl Drop for Vm {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_VMDelete(self.ctx) };
    }
}

