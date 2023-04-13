# Develop WasmEdge Plug-in

While the WasmEdge language SDKs allow registering host functions from a host (wrapping) application, developers should implement the host functions before compilation.
For the other solutions, WasmEdge provides the plug-in architecture to load the plug-in shared library for easier extending of the host functions.
With developing the plug-ins, WasmEdge SDKs in the supported languages can load and register the host functions from the plug-in shared libraries.

In current, developers can follow the guides to implement the plug-ins in [C API (recommended)](develop_plugin/c.md) or [C++](develop_plugin/cpp.md).

## Loadable Plug-in

Loadable plugin is a standalone `.so`/`.dylib`/`.dll` file that WasmEdge can load during runtime environment, and provide modules to be imported.

Please [refer to the plugin example code](https://github.com/WasmEdge/WasmEdge/tree/master/examples/plugin/get-string).

## WasmEdge Currently Released Plug-ins

There are several plug-in releases with the WasmEdge official releases.
Please check the following table to check the release status and how to build from source with the plug-ins.

> The `WasmEdge-Process` plug-in is attached in the WasmEdge release tarballs.

| Plug-in                                                                                                                     | Rust Crate                     | Released Platforms                                                                          | Build Steps                                                                                                           |
| --------------------------------------------------------------------------------------------------------------------------- | ------------------------------ | ------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| WasmEdge-Process                                                                                                            | [wasmedge_process_interface][] | `manylinux2014 x86_64`, `manylinux2014 aarch64`, and `ubuntu 20.04 x86_64` (since `0.10.0`) | [Build With WasmEdge-Process](contribute/build_from_src/plugin_wasmedge_process.md)                                   |
| [WASI-Crypto](write_wasm/rust/wasicrypto.md#prerequisites)                                                                  | [wasi-crypto][]                | `manylinux2014 x86_64`, `manylinux2014 aarch64`, and `ubuntu 20.04 x86_64` (since `0.10.1`) | [Build With WASI-Crypto](contribute/build_from_src/plugin_wasi_crypto.md)                                             |
| [WASI-NN with OpenVINO backend](write_wasm/rust/wasinn.md#get-wasmedge-with-wasi-nn-plug-in-openvino-backend)               | [wasi-nn][]                    | `ubuntu 20.04 x86_64` (since `0.10.1`)                                                      | [Build With WASI-NN](contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-openvino-backend)        |
| [WASI-NN with PyTorch backend](write_wasm/rust/wasinn.md#get-wasmedge-with-wasi-nn-plug-in-pytorch-backend)                 | [wasi-nn][]                    | `ubuntu 20.04 x86_64` (since `0.11.1`)                                                      | [Build With WASI-NN](contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-pytorch-backend)         |
| [WASI-NN with TensorFlow-Lite backend](write_wasm/rust/wasinn.md#get-wasmedge-with-wasi-nn-plug-in-tensorflow-lite-backend) | [wasi-nn][]                    | `manylinux2014 x86_64`, `manylinux2014 aarch64`, and `ubuntu 20.04 x86_64` (since `0.11.2`) | [Build With WASI-NN](contribute/build_from_src/plugin_wasi_nn.md#build-wasmedge-with-wasi-nn-tensorflow-lite-backend) |
| [WasmEdge-HttpsReq](write_wasm/rust/networking-https.md#prerequisites)                                                      | [wasmedge_http_req][]          | `manylinux2014 x86_64`, and `manylinux2014 aarch64` (since `0.11.1`)                        | [Build With WasmEdge-HttpsReq](contribute/build_from_src/plugin_wasmedge_httpsreq.md)                                 |

> Due to the `OpenVINO` and `PyTorch` dependencies, we only release the WASI-NN plug-in on `Ubuntu 20.04 x86_64` now. We'll work with `manylinux2014` versions in the future.

[wasmedge_process_interface]: https://crates.io/crates/wasmedge_process_interface
[wasi-crypto]: https://crates.io/crates/wasi-crypto
[wasi-nn]: https://crates.io/crates/wasi-nn
[wasmedge_http_req]: https://crates.io/crates/wasmedge_http_req
