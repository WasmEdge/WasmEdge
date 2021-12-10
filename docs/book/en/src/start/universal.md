# Universal Wasm Binary Format

After 0.9.0 is released, WasmEdge will wrap the native binary into a custom section in the origin wasm file. So you can create a universal wasm binary format.

With this feature, you can still run the wasm with other wasm runtimes just like a normal wasm file. However, when this wasm file is handled by the WasmEdge runtime, we will extract the native binary from the custom section and execute it.

In case that users want to generate the native binary only, WasmEdge uses the name of the extension to create different output formats. For example, if you set the extension as `.so`, WasmEdge will generate native binary only. Otherwise, it will generate a universal wasm binary by default.
