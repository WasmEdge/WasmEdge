//! WasmEdge Runtime supports WebAssembly Threads proposal. This example demonstrates how to use multiple threads to
//! render the [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set) in parallel.
//!
//! Use the following command to run the example:
//!
//! ```bash
//! // cd bindings/rust
//! cargo run -p wasmedge-sdk --example work_in_parallel -- --nocapture
//! ```
//!
//! The file "mandelbrot.so" used in the code below is generated by `wasmedgec` command. For details, please refer to
//! [Enable AOT Mode](https://github.com/WasmEdge/WasmEdge/blob/master/examples/capi/mandelbrot-set-in-threads/README.md#enable-aot-mode).
//!
//! In addition, to visualize the generated binary image file "output-wasmedge.bin", you can use the following Python code:
//! ```python
//! import matplotlib.pyplot as plt
//! import numpy as np
//!
//! data = np.fromfile("output-wasmedge.bin", dtype=np.int32)
//! print(f"data size: {data.size}")
//! resized_data = np.resize(data, (800, 1200))
//! print(f"resized_data: {resized_data.shape}, dtype: {resized_data.dtype}")
//! plt.imshow(resized_data)
//! ```

use std::{
    fs,
    sync::{Arc, Mutex},
    thread,
    time::Instant,
};
use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    params, ImportObjectBuilder, Memory, MemoryType, Vm, WasmEdgeResult, WasmVal,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // number of theads to spawn
    let num_threads: i32 = 4;

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
    let vm = Vm::new(Some(config))?
        .register_import_module(import)?
        .register_module_from_file("mandelbrot", "wasmedge-sdk/examples/data/mandelbrot.so")?;

    // parallelly renders the image
    let x: f64 = -0.743644786;
    let y: f64 = 0.1318252536;
    let d: f64 = 0.00029336;
    let max_iter: i32 = 10_000;
    let mut handles = vec![];
    let p_vm = Arc::new(Mutex::new(vm));
    for rank in 0..num_threads {
        let vm_cloned = Arc::clone(&p_vm);
        let handle = thread::spawn(move || -> WasmEdgeResult<()> {
            let vm = vm_cloned.lock().unwrap();
            vm.run_func(
                Some("mandelbrot"),
                "mandelbrotThread",
                params!(max_iter, num_threads, rank, x, y, d),
            )?;

            Ok(())
        });

        handles.push(handle);
    }
    let start = Instant::now();
    for handle in handles {
        handle.join().unwrap()?;
    }
    println!("Time elapsed: {:.2?}", start.elapsed());

    // get the final image
    let vm = p_vm.lock().unwrap();
    let returns = vm.run_func(Some("mandelbrot"), "getImage", params!())?;
    let offset = returns[0].to_i32();
    let env_instance = vm.named_module("env")?;
    let memory = env_instance.memory("memory").unwrap();
    let width = 1200;
    let height = 800;
    let buffer = memory.read(offset as u32, width * height * 4)?;
    // dump the image to a binary file
    fs::write("output-wasmedge.bin", buffer)?;

    Ok(())
}
