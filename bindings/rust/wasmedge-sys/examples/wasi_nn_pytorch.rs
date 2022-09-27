use wasmedge_sys::{utils, AsInstance, Config, Vm};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    utils::load_plugin_from_default_paths();

    // create a Config context
    let mut config = Config::create()?;
    config.bulk_memory_operations(true);
    config.wasi(true);
    assert!(config.wasi_enabled());

    let mut vm = Vm::create(Some(config), None)?;

    // println!("len: {}", vm.function_list_len());

    // vm.function_iter()
    //     .for_each(|(name, _)| println!("{:?}", name));

    // let wasi_module = vm.wasi_module_mut()?;

    // println!("len: {}", wasi_module.func_len());

    // wasi_module.func_names().unwrap().iter().for_each(|name| {
    //     println!("{:?}", name);
    // });
    //

    let store = vm.store_mut()?;
    println!("module len: {}", store.module_len());

    store.module_names().unwrap().iter().for_each(|name| {
        println!("{:?}", name);
    });

    Ok(())
}
