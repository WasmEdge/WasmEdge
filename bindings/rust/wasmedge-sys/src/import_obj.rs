use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    string::StringRef,
    utils::string_to_c_char,
};
#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
}

impl ImportObj {
    pub fn create(name: impl AsRef<str>) -> Option<Self> {
        let raw_module_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        match ctx.is_null() {
            true => None,
            false => Some(ImportObj { ctx }),
        }
    }

    pub fn add_func(&mut self, name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registered = true;
        func.ctx = std::ptr::null_mut();
    }

    pub fn add_table(&mut self, name: impl AsRef<str>, table: &mut Table) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                table.ctx,
            );
        }
        table.registered = true;
        table.ctx = std::ptr::null_mut();
    }

    pub fn add_memory(&mut self, name: impl AsRef<str>, memory: &mut Memory) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                memory.ctx,
            );
        }
        memory.registered = true;
        memory.ctx = std::ptr::null_mut();
    }

    pub fn add_global(&mut self, name: impl AsRef<str>, global: &mut Global) {
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(
                self.ctx,
                wasmedge::WasmEdge_String::from(StringRef::from(name.as_ref())),
                global.ctx,
            );
        }
        global.registered = true;
        global.ctx = std::ptr::null_mut();
    }

    pub fn init_wasi<T: Iterator<Item = E>, E: AsRef<str>>(
        &self,
        args: Option<T>,
        envs: Option<T>,
        preopens: Option<T>,
    ) {
        let (args_len, args) = match args {
            Some(args) => {
                let args = args
                    .into_iter()
                    .map(|arg| string_to_c_char(arg))
                    .collect::<Vec<_>>();
                (args.len() as u32, args.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (envs_len, envs) = match envs {
            Some(envs) => {
                let envs = envs
                    .into_iter()
                    .map(|env| string_to_c_char(env))
                    .collect::<Vec<_>>();
                (envs.len() as u32, envs.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        let (preopens_len, preopens) = match preopens {
            Some(preopens) => {
                let preopens = preopens
                    .into_iter()
                    .map(|preopen| string_to_c_char(preopen))
                    .collect::<Vec<_>>();
                (preopens.len() as u32, preopens.as_ptr())
            }
            None => (0, std::ptr::null()),
        };
        unsafe {
            wasmedge::WasmEdge_ImportObjectInitWASI(
                self.ctx,
                args,
                args_len,
                envs,
                envs_len,
                preopens,
                preopens_len,
            )
        };
    }
}

impl Drop for ImportObj {
    fn drop(&mut self) {
        unsafe { wasmedge::WasmEdge_ImportObjectDelete(self.ctx) };
    }
}

#[cfg(test)]
mod tests {
    use super::ImportObj;
    use crate::{
        instance::Function,
        io::{I1, I2},
        Value,
    };

    #[test]
    fn test_imports_add_host_function() {
        let result = ImportObj::create("extern_module");
        assert!(result.is_some());
        let mut import_obj = result.unwrap();

        let mut host_func = Function::create_bindings::<I2<i32, i32>, I1<i32>>(Box::new(real_add));
        import_obj.add_func("add", &mut host_func);
        assert!(host_func.ctx.is_null() && host_func.registered);
    }

    fn real_add(input: Vec<Value>) -> Result<Vec<Value>, u8> {
        println!("Rust: Entering Rust function real_add");

        if input.len() != 2 {
            return Err(1);
        }

        let a = if let Value::I32(i) = input[0] {
            i
        } else {
            return Err(2);
        };

        let b = if let Value::I32(i) = input[1] {
            i
        } else {
            return Err(3);
        };

        let c = a + b;
        println!("Rust: calcuating in real_add c: {:?}", c);

        println!("Rust: Leaving Rust function real_add");
        Ok(vec![Value::I32(c)])
    }
}
