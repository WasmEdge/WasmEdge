//! This example presents how to create an import object and add function, global, memory and table instances in it.

// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{
    error::HostFuncError, host_function, types::Val, Caller, Global, GlobalType,
    ImportObjectBuilder, Memory, MemoryType, Mutability, RefType, Table, TableType, ValType,
    WasmValue,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // a native function to be imported as host function
    #[host_function]
    fn real_add(
        _caller: Caller,
        inputs: Vec<WasmValue>,
    ) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
        if inputs.len() != 2 {
            return Err(HostFuncError::User(1));
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(HostFuncError::User(2));
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(HostFuncError::User(3));
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }

    // create a Const global instance to be imported
    let global_const = Global::new(
        GlobalType::new(ValType::F32, Mutability::Const),
        Val::F32(3.5),
    )?;

    // create a memory instance to be imported
    let memory = Memory::new(MemoryType::new(10, Some(20), false)?)?;

    // create a table instance to be imported
    let table = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)))?;

    // create an import object
    let module_name = "extern";
    let _import = ImportObjectBuilder::new()
        // add a function
        .with_func::<(i32, i32), i32>("add", real_add)?
        // add a global
        .with_global("global", global_const)?
        // add a memory
        .with_memory("memory", memory)?
        // add a table
        .with_table("table", table)?
        .build(module_name)?;

    Ok(())
}
