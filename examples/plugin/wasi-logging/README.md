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

## Clone WASI-Logging submodules
```
git submodule update --init
```
Then we can see .wit files in `examples/plugin/wasi-logging/wasi-logging/wit` directory.

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

This example should run successfully and print out the log as follows.