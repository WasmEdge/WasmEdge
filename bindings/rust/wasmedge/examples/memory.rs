use wasmedge::{wat2wasm, Executor, Module, Store, WasmValue};

fn main() -> anyhow::Result<()> {
    let wasm_bytes = wat2wasm(
        r#"
(module
  (type $mem_size_t (func (result i32)))
  (type $get_at_t (func (param i32) (result i32)))
  (type $set_at_t (func (param i32) (param i32)))

  (memory $mem 1)

  (func $get_at (type $get_at_t) (param $idx i32) (result i32)
    (i32.load (local.get $idx)))

  (func $set_at (type $set_at_t) (param $idx i32) (param $val i32)
    (i32.store (local.get $idx) (local.get $val)))

  (func $mem_size (type $mem_size_t) (result i32)
    (memory.size))

  (export "get_at" (func $get_at))
  (export "set_at" (func $set_at))
  (export "mem_size" (func $mem_size))
  (export "memory" (memory $mem)))
"#
        .as_bytes(),
    )?;

    // loads a wasm module from the given in-memory bytes
    let module = Module::from_bytes(None, &wasm_bytes)?;

    // create an executor
    let mut executor = Executor::new(None, None)?;

    // create a store
    let mut store = Store::new()?;

    // register the module into the store
    store.register_named_module(&mut executor, "extern", &module)?;

    // get module instance
    let instance = store.module_instance("extern").ok_or(anyhow::anyhow!(
        "failed to get module instance named 'extern'"
    ))?;

    // get the exported memory instance
    let mut memory = instance.memory("memory").ok_or(anyhow::anyhow!(
        "failed to get memory instance named 'memory'"
    ))?;

    // check memory size
    assert_eq!(memory.size(), 1);
    assert_eq!(memory.data_size(), 65536);

    // grow memory size
    memory.grow(2)?;
    assert_eq!(memory.size(), 3);
    assert_eq!(memory.data_size(), 3 * 65536);

    // call the exported function named "set_at"
    let mem_addr = 0x2220;
    let val = 0xFEFEFFE;
    executor.run_func(
        &mut store,
        Some("extern"),
        "set_at",
        [WasmValue::from_i32(mem_addr), WasmValue::from_i32(val)],
    )?;

    // call the exported function named "get_at"
    let returns = executor.run_func(
        &mut store,
        Some("extern"),
        "get_at",
        [WasmValue::from_i32(mem_addr)],
    )?;
    assert_eq!(returns[0].to_i32(), val);

    let page_size = 0x1_0000;
    let mem_addr = (page_size * 2) - std::mem::size_of_val(&val) as i32;
    let val = 0xFEA09;
    executor.run_func(
        &mut store,
        Some("extern"),
        "set_at",
        [WasmValue::from_i32(mem_addr), WasmValue::from_i32(val)],
    )?;

    let returns = executor.run_func(
        &mut store,
        Some("extern"),
        "get_at",
        [WasmValue::from_i32(mem_addr)],
    )?;
    assert_eq!(returns[0].to_i32(), val);

    Ok(())
}
