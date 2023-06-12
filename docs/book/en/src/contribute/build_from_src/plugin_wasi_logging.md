# Build WasmEdge With WASI-Logging Plug-in

## Prerequisites

The prerequisite of the Wasi-Logging plug-in is the same as the WasmEdge building environment on the [Linux](linux.md) and [MacOS](macos.md) platforms.

## Build WasmEdge with WASI-Logging Plug-in

To enable the WASI-Logging Plug-in, developers need to build the WasmEdge from source with the cmake option `-DWASMEDGE_PLUGIN_WASI_LOGGING=ON`.

```bash
cd <path/to/your/wasmedge/source/folder>
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_PLUGIN_WASI_LOGGING=ON .. && make -j
# For the WASI-Logging plug-in, you should install this project.
cmake --install .
```

> If the built `wasmedge` CLI tool cannot find the WASI-Logging plug-in, you can set the `WASMEDGE_PLUGIN_PATH` environment variable to the plug-in installation path (`/usr/local/lib/wasmedge`, or the built plug-in path `build/plugins/wasi_logging`) to try to fix this issue. You should find `libwasmedgePluginWasiLogging.so` in your `WASMEDGE_PLUGIN_PATH`

Then you will have an executable `wasmedge` runtime under `/usr/local/bin` and the WASI-Logging plug-in under `/usr/local/lib/wasmedge/libwasmedgePluginWasiLogging.so` after installation.
