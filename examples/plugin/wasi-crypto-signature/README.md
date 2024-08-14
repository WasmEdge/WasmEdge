# WasmEdge WASI-Crypto example

This is a example for demonstrate how to use wasi-crypto plugin of WasmEdge in Rust which is adopted from wasi-crypto tests.

## Prerequisites

### Install Rust

Follow the instructions below to install rust and wasm32-wasi target.

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add wasm32-wasi
```

### Install WasmEdge and WASI-crypto plugin

Note that if you install WasmEdge using install script, you need to download `wasi-crypto` plugin from [release page](https://github.com/WasmEdge/WasmEdge/releases/) and put it into `$HOME/.wasmedge/plugin/`.

Or you can build wasmedge from scratch with wasi-crypto plugin enabled.

```bash
git clone https://github.com/WasmEdge/WasmEdge.git --depth 1
cd WasmEdge
mkdir build; cd build
cmake .. -DWASMEDGE_PLUGIN_WASI_CRYPTO=ON
make
# to tell wasmedge where to find wasi-crypto plugin.
export WASMEDGE_PLUGIN_PATH=$PWD/plugins/wasi_crypto
# compiled wasmedge is located in: ./tools/wasmedge/wasmedge
```

## Build the example

```bash
cargo b --target wasm32-wasi
```

Then we get `target/wasm32-wasi/debug/wasi-crypto-example.wasm`.

## Run the example

We can run this example with `wasmedge` like

```bash
wasmedge target/wasm32-wasi/debug/wasi-crypto-example.wasm
```

This example should run successfully and print out the signatures as follows.

```bash
[src/main.rs:20] decode(encoded) = "9D92E9FDCA3DDF2E1DDCA1E3B7A79A25B6E4AFFCABF5F9FF4D960B152AB830E9EB978BD3DA89C42BBFE5A2C2AEB0AF1DD178FB4BCD833B587D118F59BBB4D"
[src/main.rs:21] decode(export_sig) = "9D92E9FDCA3DDF2E1DDCA1E3B7A79A25B6E4AFFCABF5F9FF4D960B152AB830E9EB978BD3DA89C42BBFE5A2C2AEB0AF1DD178FB4BCD833B587D118F59BBB4D"
```
