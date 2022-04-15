use wasmedge::{types::Val, wat2wasm, Executor, Func, FuncTypeBuilder, Module, Store, WasmValue};
use wasmedge_types::{RefType, TableType, ValType};

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
    let callback = extern_instance
        .func("call_callback")
        .ok_or(anyhow::Error::msg(
            "Not found exported function named `call_callback`.",
        ))?;

    // call the exported function named "call_callback"
    let returns = callback.call(
        &mut executor,
        [
            WasmValue::from_i32(1),
            WasmValue::from_i32(2),
            WasmValue::from_i32(7),
        ],
    )?;
    assert_eq!(returns[0].to_i32(), 18);

    // get module instance
    let instance = store.module_instance("extern").ok_or(anyhow::anyhow!(
        "failed to get module instance named 'extern'"
    ))?;

    // get the exported table instance named "__indirect_function_table"
    let mut table = instance
        .table("__indirect_function_table")
        .ok_or(anyhow::anyhow!(
            "failed to get table instance named '__indirect_function_table'"
        ))?;
    assert_eq!(table.size(), 3);
    assert_eq!(table.ty()?, TableType::new(RefType::FuncRef, 3, Some(6)));

    /// A function we'll call through a table.
    fn host_callback(inputs: Vec<WasmValue>) -> std::result::Result<Vec<WasmValue>, u8> {
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

    // create a host function over host_callback
    let func = Func::new(
        FuncTypeBuilder::new()
            .with_args([ValType::I32; 2])
            .with_return(ValType::I32)
            .build(),
        Box::new(host_callback),
    )?;

    // set elements in the table instance
    let previous_size = table.grow(3, Some(Val::FuncRef(Some(func.as_ref()))))?;
    assert_eq!(previous_size, 3);
    assert_eq!(table.size(), 6);

    for idx in 3..6 {
        if let Val::FuncRef(Some(_func_ref)) = table.get(idx)? {
            todo!()

            // TODO now no way to call func. New WasmEdge C-API will provide a solution to the issue.
        } else {
            panic!("expected to find funcref in table!");
        }
    }

    Ok(())
}
