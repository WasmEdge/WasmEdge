mod helper;
mod models;

pub enum ErrNo {
    Success = 0,              // No error occurred.
    InvalidArgument = 1,      // Caller module passed an invalid argument.
    InvalidEncoding = 2,      // Invalid encoding.
    MissingMemory = 3,        // Caller module is missing a memory export.
    Busy = 4,                 // Device or resource busy.
    RuntimeError = 5,         // Runtime Error.
    UnsupportedOperation = 6, // Unsupported Operation.
    TooLarge = 7,             // Too Large.
    NotFound = 8,             // Not Found.
}
mod wasi_nn {
    use crate::helper::get_slice;
    #[cfg(feature = "squeezenet")]
    use crate::models::squeezenet::*;
    #[cfg(feature = "whisper")]
    use crate::models::whisper::*;
    use crate::ErrNo;
    use burn::backend::wgpu::{AutoGraphicsApi, Wgpu, WgpuDevice};
    use burn::backend::NdArray;
    use lazy_static::lazy_static;
    use std::collections::HashMap;
    use std::env;
    use std::mem;
    use std::process;
    use std::sync::Mutex;

    use wasmedge_wasi_nn::TensorType;
    use wasmedge_plugin_sdk::{
        error::CoreError,
        memory::Memory,
        module::{PluginModule, SyncInstanceRef},
        types::{ValType, WasmVal},
    };

    type NdArrayBackend = NdArray<f32>;
    type WgpuBackend = Wgpu<AutoGraphicsApi, f32, i32>;

    pub enum Graph {
        /// The model is loaded to the NdArray backend
        WithNdArrayBackend(GraphInner<NdArrayBackend>),
        /// The model is loaded to the Wgpu backend
        WithWgpuBackend(GraphInner<WgpuBackend>),
    }

    enum ExecutionContext {
        /// The model is loaded to the NdArray backend
        WithNdArrayBackend(ContextInner<NdArrayBackend>),
        /// The model is loaded to the Wgpu backend
        WithWgpuBackend(ContextInner<WgpuBackend>),
    }

    lazy_static! {
        static ref GRAPH_HANDLE_MAP: Mutex<HashMap<u32, Graph>> = Mutex::new(HashMap::new());
        static ref GRAPH_NAME_MAP: Mutex<HashMap<String, u32>> = Mutex::new(HashMap::new());
        static ref CONTEXT_HANDLE_MAP: Mutex<HashMap<u32, (u32, ExecutionContext)>> =
            Mutex::new(HashMap::new());
    }

    fn parse_opts() {
        fn process_nn_preload(nn_preload: String) {
            let parts: Vec<&str> = nn_preload.split(':').collect();

            if parts.len() < 4 {
                eprintln!("[WASI_NN] Invalid nn-preload format. {:?} len < 4", parts);
                process::exit(1);
            }

            let graph_encoding = parts[1].to_string();
            if graph_encoding.to_lowercase() != "burn" {
                eprintln!("[WASI_NN] Unsupported graph encoding. {:?}", graph_encoding);
                process::exit(1);
            }

            let name = parts[0].to_string();
            let mut graph_map = GRAPH_HANDLE_MAP.lock().unwrap();
            let graph_handle = graph_map.len() as u32;
            let mut name_map = GRAPH_NAME_MAP.lock().unwrap();
            name_map.insert(name.clone(), graph_handle);
            let target = parts[2].to_string().to_lowercase();
            if target == "gpu" {
                let device = WgpuDevice::BestAvailable;
                graph_map.insert(
                    graph_handle,
                    Graph::WithWgpuBackend(GraphInner::create(parts[3..].to_vec(), &device)),
                );
            } else {
                let device = Default::default();
                graph_map.insert(
                    graph_handle,
                    Graph::WithNdArrayBackend(GraphInner::create(parts[3..].to_vec(), &device)),
                );
            };
        }

        unsafe {
            if let Ok(nn_preload) = (*crate::nn_preload()).to_string() {
                process_nn_preload(nn_preload);
            } else if let Ok(env_nn_preload) = env::var("WASMEDGE_WASINN_PRELOAD") {
                process_nn_preload(env_nn_preload);
            }
        }
    }

    pub fn create_module() -> PluginModule<()> {
        fn load<'a>(
            _inst: &'a mut SyncInstanceRef,
            _memory: &'a mut Memory,
            _data: &'a mut (),
            _args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            Ok(vec![WasmVal::I32(ErrNo::UnsupportedOperation as i32)])
        }

        fn load_by_name<'a>(
            _inst: &'a mut SyncInstanceRef,
            memory: &'a mut Memory,
            _data: &'a mut (),
            args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            if let [WasmVal::I32(data_ptr), WasmVal::I32(data_len), WasmVal::I32(graph_handle_ptr)] =
                &args[..]
            {
                let bytes = memory
                    .data_pointer(*data_ptr as usize, *data_len as usize)
                    .unwrap();
                let name = String::from_utf8_lossy(&bytes);
                let name_map = GRAPH_NAME_MAP.lock().unwrap();
                if let Some(handle) = name_map.get(name.as_ref()) {
                    memory.write_data((*graph_handle_ptr as usize).into(), *handle);
                    Ok(vec![WasmVal::I32(ErrNo::Success as i32)])
                } else {
                    Ok(vec![WasmVal::I32(ErrNo::NotFound as i32)])
                }
            } else {
                Ok(vec![WasmVal::I32(ErrNo::InvalidArgument as i32)])
            }
        }

        fn init_execution_context<'a>(
            _inst: &'a mut SyncInstanceRef,
            memory: &'a mut Memory,
            _data: &'a mut (),
            args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            if let [WasmVal::I32(graph_handle), WasmVal::I32(context_handle_ptr)] = &args[..] {
                let mut context_map = CONTEXT_HANDLE_MAP.lock().unwrap();
                let context_handle = context_map.len() as u32;
                let graph_map = GRAPH_HANDLE_MAP.lock().unwrap();
                let graph = graph_map
                    .get(&(*graph_handle as u32))
                    .unwrap_or_else(|| unreachable!());
                match graph {
                    Graph::WithNdArrayBackend(_) => {
                        context_map.insert(
                            context_handle,
                            (
                                *graph_handle as u32,
                                ExecutionContext::WithNdArrayBackend(ContextInner::new()),
                            ),
                        );
                    }
                    Graph::WithWgpuBackend(_) => {
                        context_map.insert(
                            context_handle,
                            (
                                *graph_handle as u32,
                                ExecutionContext::WithWgpuBackend(ContextInner::new()),
                            ),
                        );
                    }
                }
                memory.write_data((*context_handle_ptr as usize).into(), context_handle);
                Ok(vec![WasmVal::I32(ErrNo::Success as i32)])
            } else {
                Ok(vec![WasmVal::I32(ErrNo::InvalidArgument as i32)])
            }
        }

        fn set_input<'a>(
            _inst: &'a mut SyncInstanceRef,
            memory: &'a mut Memory,
            _data: &'a mut (),
            args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            #[derive(Debug)]
            #[repr(C)]
            struct WasiTensorData {
                dimens_ptr: u32,
                dimens_length: u32,
                tensor_type: TensorType,
                tensor_ptr: u32,
                tensor_length: u32,
            }
            if let [WasmVal::I32(context_handle), WasmVal::I32(input_index), WasmVal::I32(input_tensor_ptr)] =
                &args[..]
            {
                match memory.get_data::<WasiTensorData>((*input_tensor_ptr as usize).into()) {
                    Some(input_tensor) => {
                        let raw_dimens = get_slice!(
                            memory,
                            input_tensor.dimens_ptr,
                            INPUT_DIM * mem::size_of::<u32>(),
                            u32
                        );
                        let dimens: [usize; INPUT_DIM] = raw_dimens
                            .iter()
                            .map(|&x| x as usize)
                            .collect::<Vec<usize>>()
                            .try_into()
                            .unwrap();

                        // FIXME: The type of f32 should be decided at runtime based on input_tensor.tensor_type.
                        let tensor = get_slice!(
                            memory,
                            input_tensor.tensor_ptr,
                            input_tensor.tensor_length,
                            f32
                        );

                        let mut context_map = CONTEXT_HANDLE_MAP.lock().unwrap();
                        let (_, context) = context_map
                            .get_mut(&(*context_handle as u32))
                            .unwrap_or_else(|| unreachable!());

                        match context {
                            ExecutionContext::WithNdArrayBackend(inner) => {
                                inner.set_input(*input_index as u32, tensor, dimens);
                            }
                            ExecutionContext::WithWgpuBackend(inner) => {
                                inner.set_input(*input_index as u32, tensor, dimens);
                            }
                        }
                        Ok(vec![WasmVal::I32(ErrNo::Success as i32)])
                    }
                    None => Ok(vec![WasmVal::I32(ErrNo::MissingMemory as i32)]),
                }
            } else {
                Ok(vec![WasmVal::I32(ErrNo::InvalidArgument as i32)])
            }
        }

        fn compute<'a>(
            _inst: &'a mut SyncInstanceRef,
            _memory: &'a mut Memory,
            _data: &'a mut (),
            args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            if let [WasmVal::I32(context_handle)] = &args[..] {
                let mut context_map = CONTEXT_HANDLE_MAP.lock().unwrap();
                let (graph_handle, context) = context_map
                    .get_mut(&(*context_handle as u32))
                    .unwrap_or_else(|| unreachable!());

                let graph_map = GRAPH_HANDLE_MAP.lock().unwrap();
                let graph = graph_map
                    .get(graph_handle)
                    .unwrap_or_else(|| unreachable!());

                match context {
                    ExecutionContext::WithNdArrayBackend(ctx_inner) => {
                        let Graph::WithNdArrayBackend(graph_inner) = graph else {
                            unreachable!()
                        };
                        let output =
                            graph_inner.compute((*ctx_inner.inputs.get(&0).unwrap()).clone());
                        ctx_inner.outputs.push(output);
                    }
                    ExecutionContext::WithWgpuBackend(ctx_inner) => {
                        let Graph::WithWgpuBackend(graph_inner) = graph else {
                            unreachable!()
                        };
                        let output =
                            graph_inner.compute((*ctx_inner.inputs.get(&0).unwrap()).clone());
                        ctx_inner.outputs.push(output);
                    }
                };
                Ok(vec![WasmVal::I32(ErrNo::Success as i32)])
            } else {
                Ok(vec![WasmVal::I32(ErrNo::InvalidArgument as i32)])
            }
        }

        fn get_output<'a>(
            _inst: &'a mut SyncInstanceRef,
            memory: &'a mut Memory,
            _data: &'a mut (),
            args: Vec<WasmVal>,
        ) -> Result<Vec<WasmVal>, CoreError> {
            if let [WasmVal::I32(context_handle), WasmVal::I32(output_index), WasmVal::I32(output_ptr), WasmVal::I32(output_max_size), WasmVal::I32(written_length)] =
                &args[..]
            {
                let mut context_map = CONTEXT_HANDLE_MAP.lock().unwrap();
                let (_, context) = context_map
                    .get_mut(&(*context_handle as u32))
                    .unwrap_or_else(|| unreachable!());
                let raw_output = match context {
                    ExecutionContext::WithNdArrayBackend(ctx_inner) => {
                        ctx_inner.get_output(*output_index as usize)
                    }
                    ExecutionContext::WithWgpuBackend(ctx_inner) => {
                        ctx_inner.get_output(*output_index as usize)
                    }
                };
                let output: &[u8] = bytemuck::cast_slice(&raw_output);
                if output.len() > *output_max_size as usize {
                    return Ok(vec![WasmVal::I32(ErrNo::TooLarge as i32)]);
                }
                memory.write_bytes(output, *output_ptr as u32).unwrap();
                memory.write_data((*written_length as usize).into(), output.len());
                Ok(vec![WasmVal::I32(ErrNo::Success as i32)])
            } else {
                Ok(vec![WasmVal::I32(ErrNo::InvalidArgument as i32)])
            }
        }

        parse_opts();

        let mut module = PluginModule::create("wasi_ephemeral_nn", ()).unwrap();
        module
            .add_func("load", (vec![ValType::I32; 5], vec![ValType::I32]), load)
            .unwrap();
        module
            .add_func(
                "load_by_name",
                (vec![ValType::I32; 3], vec![ValType::I32]),
                load_by_name,
            )
            .unwrap();
        module
            .add_func(
                "init_execution_context",
                (vec![ValType::I32; 2], vec![ValType::I32]),
                init_execution_context,
            )
            .unwrap();
        module
            .add_func(
                "set_input",
                (vec![ValType::I32; 3], vec![ValType::I32]),
                set_input,
            )
            .unwrap();
        module
            .add_func(
                "compute",
                (vec![ValType::I32; 1], vec![ValType::I32]),
                compute,
            )
            .unwrap();
        module
            .add_func(
                "get_output",
                (vec![ValType::I32; 5], vec![ValType::I32]),
                get_output,
            )
            .unwrap();
        module
    }
}

use wasi_nn::create_module;
use wasmedge_plugin_sdk::plugin::{option_string, register_plugin, OptionString};
register_plugin!(
    plugin_name = "wasi_nn",
    plugin_description = "burn framework adapter as wasi-nn plugin",
    version = (0,0,0,1),
    modules = [
        {"wasi_nn", "wasinn with burn backend module", create_module}
    ],
    options = [
        {
            "nn-preload",
            "Allow preload models from wasinn plugin. Each NN model can be specified as --nn-preload `COMMAND`.",
            OptionString,
            option_string!("none")
        }
    ]
);
