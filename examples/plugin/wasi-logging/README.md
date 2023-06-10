# WasmEdge WASI-Logging example.

This is an example of using the WASI-Logging plugin of WasmEdge in Rust.

## Prerequisites

### Install Rust.

Follow the instructions below to install Rust and wasm32-wasi target.

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add wasm32-wasi
```

### Install WasmEdge and WASI-Logging plugin.

Build wasmedge from scratch with the WASI-Logging plugin enabled.

```sh
git clone https://github.com/WasmEdge/WasmEdge.git --depth 1
cd WasmEdge
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_PLUGIN_WASI_LOGGING=ON .. 
make -j
# to tell wasmedge where to find the WASI-Logging plugin.
export WASMEDGE_PLUGIN_PATH=$PWD/plugins/wasi_logging
```

If you install WasmEdge using the install script, you can copy library `wasmedge/build/plugins/wasi_logging/libwasmedgePluginWasiLogging.so` to `$HOME/.wasmedge/plugin/`

### (Optional) Download WASI-Logging WIT files 

In the example, we already prepare the WIT files in `wit` directory. The WIT files are from [wasi-logging](https://github.com/WebAssembly/wasi-logging) repo. You can get the same WIT files by doing the following steps.

```
git clone https://github.com/WebAssembly/wasi-logging.git
cd wasi-logging
git checkout 0a6225ba5f3e90cf72fb75a9796e3e92ff006a65
cp -r wit /path/to/wasmedge/examples/plugin/wasi-logging
```

## Build the example

```sh
cargo build --target wasm32-wasi
```

Then we get `target/wasm32-wasi/debug/wasi-logging-example.wasm`.

## Run the example

We can run this example with `wasmedge` like

```sh
wasmedge target/wasm32-wasi/debug/wasi-logging-example.wasm
```

This example should run successfully and print out the log as follow.

```
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] ===================================
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] Demo 1: Stdout Message Demo
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] -----------------------------------
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [trace] Context: Trace Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [debug] Context: Debug Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] Context: Info Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [warning] Context: Warn Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [error] Context: Error Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [critical] Context: Critical Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] ===================================
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] Demo 2: Stdout Message Without Context
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] -----------------------------------
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [trace] Trace Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [debug] Debug Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] Info Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [warning] Warn Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [error] Error Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [critical] Critical Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] ===================================
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] Demo 3: Stderr Message Demo
[2023-06-08 05:33:55.950] [wasi_logging_stdout] [info] -----------------------------------
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [trace] stderr: Trace Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [debug] stderr: Debug Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [info] stderr: Info Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [warning] stderr: Warn Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [error] stderr: Error Level Message
[2023-06-08 05:33:55.950] [wasi_logging_stderr] [critical] stderr: Critical Level Message
```