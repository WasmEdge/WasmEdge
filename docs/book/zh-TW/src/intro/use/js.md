# JavaScript 或 DSL runtime

為了使 WebAssembly runtime 被開發者廣泛採用，它必須支援像 JavaScript 這樣的「簡單」程式語言。或者，更棒的是，透過其高階編譯器工具鏈， WasmEdge 可以支援高效能 DSL （特定領域語言），這是專為特定任務設計的低程式碼解決方案。

## JavaScript

WasmEdge 可以透過嵌入 JS 執行引擎或直譯器來作為雲端原生 JavaScript runtime ，比起在 Docker 中運行 JS 引擎還更快更輕量。 WasmEdge 支援 JS API 呼叫原生擴充函式庫，例如網路 sockets 、 TensorFlow 和使用者定義的共享函式庫。 WasmEdge 還允許將 JS 嵌入其他高效能程式語言（例如 Rust）或使用 Rust/C 來實作 JS 函式。

* 教學
  * [執行 JavaScript](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
  * [在 Rust 中嵌入 JavaScript](https://www.secondstate.io/articles/embed-javascript-in-rust/)
  * [用 Rust 函式建立 JavaScript API](https://www.secondstate.io/articles/embed-rust-in-javascript/)
  * [從 JavaScript 呼叫共享函式庫中的 C 原生函數](https://www.secondstate.io/articles/call-native-functions-from-javascript/)
* [範例](https://github.com/WasmEdge/WasmEdge/blob/master/examples/js/README.md)
* [WasmEdge 的内嵌 QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)

## 用於影像辨識的 DSL

影像辨識 DSL 是一種允許使用者指定 TensorFlow 模型與其參數的 YAML 格式。 WasmEdge 將圖片作為 DSL 的輸入，並輸出偵測到的物件名稱/標籤。

* 範例： [執行 YMAL 以辨識圖片中的食物](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml)

## 用於聊天機器人 DSL

聊天機器人 DSL 函式接受輸入字串並輸出字串進行回覆。 DSL 指定了聊天機器人的內部狀態轉變，以及用於語言理解的 AI 模型。這個項目正在開發中。
