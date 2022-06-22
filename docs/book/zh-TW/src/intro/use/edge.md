# 邊緣運算

WasmEdge 非常適合在需要執行關鍵任務的終端裝置或邊緣網路上執行。

## YoMo Flow

YoMo 是一種用於遠端邊緣（far edge）網路的高效能資料流框架。 WasmEdge 集成到 YoMo 中以執行使用者定義的工作負載，例如在工廠組裝線上的影像辨識。

* [教學](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [程式碼範本](https://github.com/yomorun/yomo-wasmedge-tensorflow)

## seL4 微型核心作業系統

seL4 是一個高度安全的即時作業系統。 WasmEdge 是唯一可以在 seL4 上以原生速度運行的 WebAssembly runtime 。我們還提供了一個管理工具來支援 Wasm module 的 OTA 部署。

* [Demo](https://github.com/second-state/wasmedge-seL4)
