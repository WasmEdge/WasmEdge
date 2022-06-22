# Universal Wasm Binary Format

WasmEdge could wrap the AOT-compiled native binary into a custom section in the origin wasm file. We call this the universal wasm binary format.

The AOT-compiled wasm file is compatible with any wasm runtime. However, when this wasm file is executed by the WasmEdge runtime, WasmEdge will extract the native binary from the custom section and execute it.

Of course, the user still has the option to generate the native binary file with the `wasmedgec` AOT compiler.
WasmEdge uses the output file extension to determine generated file format. For example, if you set the `wasmedgec` output file extension to `.so`, it will generate native binary in Linux shared library format. Otherwise, it will generate a universal wasm binary by default.
