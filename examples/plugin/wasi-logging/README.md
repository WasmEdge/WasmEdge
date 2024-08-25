# WasmEdge WASI-Logging example

This is an example of using the WASI-Logging plugin of WasmEdge in Rust.

## Prerequisites

### Install Rust

Follow the instructions below to install Rust and wasm32-wasi target.

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add wasm32-wasi
```

### Install WasmEdge and WASI-Logging plugin

Build wasmedge from scratch with the WASI-Logging plugin enabled.

```sh
git clone https://github.com/WasmEdge/WasmEdge.git --depth 1
cd WasmEdge
mkdir build; cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j
# WASI-Logging plug-in is a built-in plug-in in WasmEdge library.
```

### (Optional) Download WASI-Logging WIT files

In the example, we already prepare the WIT files in `wit` directory. The WIT files are from [wasi-logging](https://github.com/WebAssembly/wasi-logging) repo. You can get the same WIT files by doing the following steps.

```sh
git clone https://github.com/WebAssembly/wasi-logging.git
cd wasi-logging
git checkout 3293e84de91a1ead98a1b4362f95ac8af5a16ddd
cp -r wit /path/to/wasmedge/examples/plugin/wasi-logging
```

## Build the example

```sh
cargo build --target wasm32-wasi
```

Then we get `target/wasm32-wasi/debug/wasi-logging-example.wasm`.

## Logging context

For the logging context of the `log` function in Rust, developers can use the `""` or `"stdout"` to log to the console, use the `"stderr"` to log to standard error output, or use the file name to log into the target file.

## Run the example

We can run this example with `wasmedge` like

```sh
wasmedge target/wasm32-wasi/debug/wasi-logging-example.wasm
```

This example should run successfully and print out the log as follow.

```sh
[2024-05-21 13:43:53.240] [info] ===================================
[2024-05-21 13:43:53.240] [info] Stdout Message Demo
[2024-05-21 13:43:53.240] [info] -----------------------------------
[2024-05-21 13:43:53.240] [trace] Trace Level Message
[2024-05-21 13:43:53.240] [debug] Debug Level Message
[2024-05-21 13:43:53.240] [info] Info Level Message
[2024-05-21 13:43:53.240] [warning] Warn Level Message
[2024-05-21 13:43:53.240] [error] Error Level Message
[2024-05-21 13:43:53.240] [critical] Critical Level Message
[2024-05-21 13:43:53.240] [info] ===================================
[2024-05-21 13:43:53.240] [info] Stderr Message Demo
[2024-05-21 13:43:53.240] [info] -----------------------------------
[2024-05-21 13:43:53.240] [trace] Trace Level Message
[2024-05-21 13:43:53.240] [debug] Debug Level Message
[2024-05-21 13:43:53.240] [info] Info Level Message
[2024-05-21 13:43:53.240] [warning] Warn Level Message
[2024-05-21 13:43:53.240] [error] Error Level Message
[2024-05-21 13:43:53.240] [critical] Critical Level Message
[2024-05-21 13:43:53.240] [info] ===================================
[2024-05-21 13:43:53.240] [info] File Message Demo: log/output.log
[2024-05-21 13:43:53.240] [info] -----------------------------------
[2024-05-21 13:43:53.240] [info] ===================================
[2024-05-21 13:43:53.240] [info] File Message Demo: log/output2.log
[2024-05-21 13:43:53.240] [info] -----------------------------------
[2024-05-21 13:43:53.240] [info] ===================================
[2024-05-21 13:43:53.240] [info] File Message Demo: continue to log/output.log
[2024-05-21 13:43:53.240] [info] -----------------------------------
```

The log file `log/output.log` will be generated:

```text
[2024-05-21 13:44:50.966] [trace] Trace Level Message
[2024-05-21 13:44:50.966] [debug] Debug Level Message
[2024-05-21 13:44:50.966] [info] Info Level Message
[2024-05-21 13:44:50.966] [warning] Warn Level Message
[2024-05-21 13:44:50.966] [error] Error Level Message
[2024-05-21 13:44:50.966] [critical] Critical Level Message
[2024-05-21 13:44:50.966] [trace] Trace Level Message
[2024-05-21 13:44:50.966] [debug] Debug Level Message
[2024-05-21 13:44:50.966] [info] Info Level Message
[2024-05-21 13:44:50.966] [warning] Warn Level Message
[2024-05-21 13:44:50.966] [error] Error Level Message
[2024-05-21 13:44:50.966] [critical] Critical Level Message
```

The log file `log/output2.log` will be generated:

```text
[2024-05-21 13:44:50.966] [trace] Trace Level Message
[2024-05-21 13:44:50.966] [debug] Debug Level Message
[2024-05-21 13:44:50.966] [info] Info Level Message
[2024-05-21 13:44:50.966] [warning] Warn Level Message
[2024-05-21 13:44:50.966] [error] Error Level Message
[2024-05-21 13:44:50.966] [critical] Critical Level Message
```
