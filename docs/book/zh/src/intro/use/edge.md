# 边缘计算

WasmEdge 非常适合在任务关键的边缘设备或边缘网络上运行。

## YoMo Flow

YoMo 是一种用于远边缘（far edge）网络的高性能数据流框架。 WasmEdge 集成到 YoMo 中以运行用户定义的工作负载，例如在工厂装配线上进行的图像识别。

* [教程](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [代码模板](https://github.com/yomorun/yomo-wasmedge-tensorflow)

## seL4 微内核操作系统

seL4 是一个高度安全的实时操作系统。 WasmEdge 是唯一可以在 seL4 上运行的 WebAssembly runtime，它以本机速度运行。我们还提供了一个管理工具来支持 wasm 模块的 OTA 部署。

* [Demo](https://github.com/second-state/wasmedge-seL4)
