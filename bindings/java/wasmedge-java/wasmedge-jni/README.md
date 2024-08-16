# Build and install

- Clone this project
- Go to `WasmEdge/bindings/java/wasmedge-jni`
- Run `mkdir build && cd build`
- Run `cmake .. && make && make install` 

## Environment variables

To run this build directly, one has to setup a proper `WasmEdge_ROOT`, which contains headers and libraries.

```
WasmEdge_ROOT=/path/to/WasmEdge/build cmake ..
```
