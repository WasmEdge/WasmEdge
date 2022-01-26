# Edge computing

WasmEdge is ideally suited to run on mission-critical edge devices or edge networks.

## YoMo Flow

YoMo is a high-performance data streaming framework for far edge networks. WasmEdge is integrated into YoMo to run user-defined workloads, such as image classification along a factory assembly line.

* [Tutorial](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [Code template](https://github.com/yomorun/yomo-wasmedge-tensorflow)

## seL4 microkernel OS

seL4 is a highly secure real-time operating system. WasmEdge is the only WebAssembly runtime that can run on seL4, and it runs at native speed. We also provide a management tool to support the OTA deployment of wasm modules.

* [Demo](https://github.com/second-state/wasmedge-seL4)
