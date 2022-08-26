# Example for using WASI in WasmEdge C API

This example is for passing the environment variables and command line arguments into the WASM environment through WASI.

## Introduction

The rust library to compile into WASM is as following:

```rust
use std::env;

#[no_mangle]
pub fn print_env() {
  println!("The env vars are as follows.");
  for (key, value) in env::vars() {
    println!("{}: {}", key, value);
  }

  println!("The args are as follows.");
  for argument in env::args() {
    println!("{}", argument);
  }
}
```

The `print_env` is the function to print the environment variables and the command line argument in the WASM virtual environtment.

The example C program use the WasmEdge C API to initialize the WASI environment as following:

```c
  /* The envs. */
  const char EnvStrs[] = {
      'E', 'N', 'V', '1', '=', 'V', 'A', 'L', '1', '\0',
      // ENV1=VAL1
      'E', 'N', 'V', '2', '=', 'V', 'A', 'L', '2', '\0',
      // ENV2=VAL2
      'E', 'N', 'V', '3', '=', 'V', 'A', 'L', '3', '\0'
      // ENV3=VAL3
  };
  const char *const Envs[] = {&EnvStrs[0], &EnvStrs[10], &EnvStrs[20]};

  /* Set the envs and args. */
  WasmEdge_ModuleInstanceContext *WasiCxt =
      WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
  WasmEdge_ModuleInstanceInitWASI(WasiCxt, argv, argc, Envs, 3, NULL, 0);
```

The command line arguments are set as the same as the args of the C program, and the enviroment variables are set as the list `ENV1=VAL1, ENV2=VAL2, ENV3=VAL3`.

## Installation

Before trying this example, the [WasmEdge installation](https://wasmedge.org/book/en/start/install.html) after the version 0.10.0 is required.

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all -v 0.10.0
```

Then you can build this example with linking the WasmEdge shared library.

```bash
# In the current directory.
clang set_wasi_env.c -o set_wasm_env -lwasmedge
```

## (Optional) Build the example WASM from rust

The pre-built WASM from rust is provided as "wasi_get_env.wasm".

For building the WASM from the rust source, the following steps are required:

* Install the [rust and cargo](https://www.rust-lang.org/tools/install).
* Install the `wasm32-wasi` target: `$ rustup target add wasm32-wasi`

```bash
cd rust
cargo build --release --target=wasm32-wasi
# The output wasm will be at `target/wasm32-wasi/release/wasi_get_env.wasm`.
```

## Run

```bash
# Run in interpreter mode
./set_wasi_env
```

The standard output of this example will be the following:

```bash
The env vars are as follows.
ENV1: VAL1
ENV2: VAL2
ENV3: VAL3
The args are as follows.
./set_wasi_env
```
