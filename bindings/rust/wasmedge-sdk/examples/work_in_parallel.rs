use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    ImportObjectBuilder, Memory, MemoryType, Vm,
};

#[cfg_attr(test, test)]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a shared memory instance
    let mem_ty = MemoryType::new(60, Some(60), true)?;
    let mem = Memory::new(mem_ty)?;

    // create an import object containing the shared memory
    let import = ImportObjectBuilder::new()
        .with_memory("memory", mem)?
        .build("env")?;

    // create a Vm instance
    let config = ConfigBuilder::new(
        CommonConfigOptions::new()
            .multi_memories(true)
            .reference_types(false),
    )
    .build()?;
    let _vm = Vm::new(Some(config))?
        .register_import_module(import)?
        .register_module_from_file("mandelbrot", "wasmedge-sdk/examples/data/mandelbrot.so")?;

    // load and instantiate wasm module from the AOT-compiled wasm file named "mandelbrot.so"

    Ok(())
}
