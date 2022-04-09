use wasmedge_sys::{
    FuncType, Function, Global, GlobalType, ImportInstance, ImportModule, ImportObject, MemType,
    Memory, Table, TableType, Vm, WasmValue,
};
use wasmedge_types::{Mutability, RefType, ValType};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let module_name = "extern_module";

    // create ImportModule instance
    let mut import = ImportModule::create(module_name)?;

    // a function to import
    fn real_add(inputs: Vec<WasmValue>) -> Result<Vec<WasmValue>, u8> {
        if inputs.len() != 2 {
            return Err(1);
        }

        let a = if inputs[0].ty() == ValType::I32 {
            inputs[0].to_i32()
        } else {
            return Err(2);
        };

        let b = if inputs[1].ty() == ValType::I32 {
            inputs[1].to_i32()
        } else {
            return Err(3);
        };

        let c = a + b;

        Ok(vec![WasmValue::from_i32(c)])
    }

    // add host function
    let func_ty = FuncType::create(vec![ValType::I32; 2], vec![ValType::I32])?;
    let host_func = Function::create(&func_ty, Box::new(real_add), 0)?;
    import.add_func("add", host_func);

    // add table
    let table_ty = TableType::create(RefType::FuncRef, 0..=u32::MAX)?;
    let table = Table::create(&table_ty)?;
    import.add_table("table", table);

    // add memory
    let mem_ty = MemType::create(0..=u32::MAX)?;
    let memory = Memory::create(&mem_ty)?;
    import.add_memory("mem", memory);

    // add global
    let ty = GlobalType::create(ValType::F32, Mutability::Const)?;
    let global = Global::create(&ty, WasmValue::from_f32(3.5))?;
    import.add_global("global", global);

    let mut vm = Vm::create(None, None)?;

    vm.register_wasm_from_import(ImportObject::Import(import))?;

    Ok(())
}
