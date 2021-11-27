use std::{ffi::CString, os::raw::c_char};

use super::wasmedge;
use crate::{
    instance::{Function, Global, Memory, Table},
    string::StringRef,
    types::WasmEdgeString,
};

#[derive(Debug)]
pub struct ImportObj {
    pub(crate) ctx: *mut wasmedge::WasmEdge_ImportObjectContext,
}

impl ImportObj {
    pub fn create(module_name: impl AsRef<str>) -> Option<Self> {
        let raw_module_name: wasmedge::WasmEdge_String =
            StringRef::from(module_name.as_ref()).into();
        let ctx = unsafe { wasmedge::WasmEdge_ImportObjectCreate(raw_module_name) };
        match ctx.is_null() {
            true => None,
            false => Some(ImportObj { ctx }),
        }
    }

    pub fn add_func(&mut self, func_name: impl AsRef<str>, func: &mut Function) {
        let raw_func_name: wasmedge::WasmEdge_String = StringRef::from(func_name.as_ref()).into();
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddFunction(self.ctx, raw_func_name, (*func).ctx);
        }
        func.registered = true;
    }

    pub fn add_table(&mut self, name: &str, table: &mut Table) {
        let name = WasmEdgeString::from_str(name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", name).as_str());
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddTable(self.ctx, name.ctx, table.ctx);
        }
        table.registered = true
    }

    pub fn add_memory(&mut self, name: &str, memory: &mut Memory) {
        let name = WasmEdgeString::from_str(name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", name).as_str());
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddMemory(self.ctx, name.ctx, memory.ctx);
        }
        memory.registered = true;
    }

    pub fn add_global(&mut self, name: &str, global: &mut Global) {
        let name = WasmEdgeString::from_str(name)
            .expect(format!("Failed to create WasmEdgeString from '{}'", name).as_str());
        unsafe {
            wasmedge::WasmEdge_ImportObjectAddGlobal(self.ctx, name.ctx, global.ctx);
        }
        global.registered = true;
    }

    pub fn create_wasi(
        args: Vec<impl AsRef<str>>,
        envs: Vec<impl AsRef<str>>,
        dirs: Vec<impl AsRef<str>>,
        preopens: Vec<impl AsRef<str>>,
    ) {
        let cstr_args: Vec<_> = args
            .iter()
            .map(|arg| CString::new(arg.as_ref()).unwrap())
            .collect();

        let cstr_envs: Vec<_> = envs
            .iter()
            .map(|env| CString::new(env.as_ref()).unwrap())
            .collect();

        let cstr_dirs: Vec<_> = dirs
            .iter()
            .map(|dir| CString::new(dir.as_ref()).unwrap())
            .collect();

        let cstr_preopens: Vec<_> = preopens
            .iter()
            .map(|preopen| CString::new(preopen.as_ref()).unwrap())
            .collect();

        unsafe {
            wasmedge::WasmEdge_ImportObjectCreateWASI(
                cstr_args.as_ptr() as *const *const c_char,
                args.len() as u32,
                cstr_envs.as_ptr() as *const *const c_char,
                envs.len() as u32,
                cstr_dirs.as_ptr() as *const *const c_char,
                dirs.len() as u32,
                cstr_preopens.as_ptr() as *const *const c_char,
                preopens.len() as u32,
            );
        }
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
        // let hostfunc_path = std::path::PathBuf::from(env!("WASMEDGE_SRC_DIR"))
        //     .join("bindings/rust/wasmedge-sys/examples/funcs.wasm");

        let result = ImportObj::create("extern_module");
        assert!(result.is_some());
        let mut import_obj = result.unwrap();

        let mut host_func = Function::create_bindings::<I2<i32, i32>, I1<i32>>(Box::new(real_add));
        import_obj.add_func("add", &mut host_func);
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
