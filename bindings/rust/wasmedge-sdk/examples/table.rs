// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

use wasmedge_sdk::{
    error::HostFuncError, host_function, params, types::Val, wat2wasm, Caller, Executor, Func,
    Module, RefType, Store, TableType, ValType, WasmVal, WasmValue,
};

#[cfg_attr(test, test)]
fn main() -> anyhow::Result<()> {
    let wasm_bytes = wat2wasm(
        r#"
(module
  ;; All our callbacks will take 2 i32s and return an i32.
  ;; Wasm tables are not limited to 1 type of function, but the code using the
  ;; table must have code to handle the type it finds.
  (type $callback_t (func (param i32 i32) (result i32)))

  ;; We'll call a callback by passing a table index as an i32 and then the two
  ;; arguments that the function expects.
  (type $call_callback_t (func (param i32 i32 i32) (result i32)))

  ;; Our table of functions that's exactly size 3 (min 3, max 3).
  (table $t1 3 6 funcref)

  ;; Call the function at the given index with the two supplied arguments.
  (func $call_callback (type $call_callback_t) (param $idx i32)
                                               (param $arg1 i32) (param $arg2 i32)
                                               (result i32)
    (call_indirect (type $callback_t) 
                   (local.get $arg1) (local.get $arg2)
                   (local.get $idx)))

  ;; A default function that we'll pad the table with.
  ;; This function doubles both its inputs and then sums them.
  (func $default_fn (type $callback_t) (param $a i32) (param $b i32) (result i32)
     (i32.add 
       (i32.mul (local.get $a) (i32.const 2))
       (i32.mul (local.get $b) (i32.const 2))))

  ;; Fill our table with the default function.
  (elem $t1 (i32.const 0) $default_fn $default_fn $default_fn)

  ;; Export things for the host to call.
  (export "call_callback" (func $call_callback))
  (export "__indirect_function_table" (table $t1)))
"#
        .as_bytes(),
    )?;

    // loads a wasm module from the given in-memory bytes
    let module = Module::from_bytes(None, wasm_bytes)?;

    // create an executor
    let mut executor = Executor::new(None, None)?;

    // create a store
    let mut store = Store::new()?;

    // register the module into the store
    let extern_instance = store.register_named_module(&mut executor, "extern", &module)?;

    // get the exported function "call_callback"
    let call_via_table = extern_instance
        .func("call_callback")
        .ok_or_else(|| anyhow::Error::msg("Not found exported function named `call_callback`."))?;

    // call the exported function named "call_callback"
    // the first argument is the table index, while the other two arguments are passed to the function found in the table.
    let returns = call_via_table.call(&mut executor, params!(1, 2, 7))?;
    assert_eq!(returns[0].to_i32(), 18);

    // get module instance
    let instance = store
        .module_instance("extern")
        .ok_or_else(|| anyhow::anyhow!("failed to get module instance named 'extern'"))?;

    // get the exported table instance named "__indirect_function_table"
    let mut guest_table = instance.table("__indirect_function_table").ok_or_else(|| {
        anyhow::anyhow!("failed to get table instance named '__indirect_function_table'")
    })?;
    assert_eq!(guest_table.size(), 3);
    assert_eq!(
        guest_table.ty()?,
        TableType::new(RefType::FuncRef, 3, Some(6))
    );

    // * setting elements in a table

    /// A function we'll call through a table.
    #[host_function]
    fn host_callback(
        _: Caller,
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

    // create a host function over host_callback
    let func = Func::wrap::<(i32, i32), i32>(Box::new(host_callback))?;

    // set the function at index 1 in the table
    guest_table.set(1, Val::FuncRef(Some(func.as_ref())))?;

    // We then repeat the call from before but this time it will find the host function
    // that we put at table index 1.
    let returns = call_via_table.call(&mut executor, params!(1, 2, 7))?;
    assert_eq!(returns[0].to_i32(), 9);

    // * growing a table

    // We again construct a `Function` over our host_callback.
    let func = Func::wrap::<(i32, i32), i32>(Box::new(host_callback))?;

    // And grow the table by 3 elements, filling in our host_callback in all the
    // new elements of the table.
    let previous_size = guest_table.grow(3, Some(Val::FuncRef(Some(func.as_ref()))))?;
    assert_eq!(previous_size, 3);
    assert_eq!(guest_table.size(), 6);

    // Now demonstrate that the function we grew the table with is actually in the table.
    for idx in 3..6 {
        if let Val::FuncRef(Some(func_ref)) = guest_table.get(idx)? {
            let returns = func_ref.call(&mut executor, params!(1, 9))?;
            assert_eq!(returns[0].to_i32(), 10);
        } else {
            panic!("expected to find funcref in table!");
        }
    }

    // Call function at index 0 to show that it's still the same.
    let returns = call_via_table.call(&mut executor, params!(0, 2, 7))?;
    assert_eq!(returns[0].to_i32(), 18);

    // Now overwrite index 0 with our host_callback.
    let func = Func::wrap::<(i32, i32), i32>(Box::new(host_callback))?;
    guest_table.set(0, Val::FuncRef(Some(func.as_ref())))?;
    // And verify that it does what we expect.
    let returns = call_via_table.call(&mut executor, params!(0, 2, 7))?;
    assert_eq!(returns[0].to_i32(), 9);

    // Now demonstrate that the host and guest see the same table and that both
    // get the same result.
    for idx in 3..6 {
        if let Val::FuncRef(Some(func_ref)) = guest_table.get(idx)? {
            let returns = func_ref.call(&mut executor, params!(1, 9))?;
            assert_eq!(returns[0].to_i32(), 10);
        } else {
            panic!("expected to find funcref in table!");
        }
        let returns = call_via_table.call(&mut executor, params!(idx as i32, 1, 9))?;
        assert_eq!(returns[0].to_i32(), 10);
    }

    Ok(())
}
