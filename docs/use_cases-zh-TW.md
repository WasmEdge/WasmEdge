# WasmEdge 使用情境

WasmEdge 是由 CNCF 託管的雲端原生 WebAssembly runtime ，廣泛應用在邊緣運算、汽車、 Jamstack 、 Serverless 、 SaaS 、 服務網格，甚至是區塊鏈應用程式。 WasmEdge 可以進行 AOT （預先編譯）編譯器最佳化，是目前市場上最快的 WebAssembly runtime。

## 目錄

* [雲端原生 runtime](#雲端原生-runtime-（作為-Docker-的輕量級替代方案）)
  * [Dapr](#Dapr-（分散式應用程式-Runtime）)
* [JavaScript 或 DSL runtime](#JavaScript-或-DSL-runtime)
  * [JavaScript](#javascript)
  * [用於影像辨識的 DSL](#用於影像辨識的-DSL)
* [公開雲端中的 Serverless 函式即服務](#公開雲端中的-Serverless-函式即服務)
  * [AWS Lambda](#aws-lambda)
  * [騰訊 Serverless 函式](#騰訊-Serverless-函式)
  * [Vercel Serverless 函式](#Vercel-Serverless-函式)
  * [Netlify 函式](#Netlify-函式)
  * [Second State 函式](#Second-State-函式)
* [支援軟體控制的汽車和 AIoT](#支援軟體控制的汽車和-AIoT)
  * [YoMo Flow](#yomo-flow)
* [用於 SaaS 的互動函式](#用於-SaaS-的互動函式)
  * [Slack](#slack)
  * [飛書](#飛書)

## 雲端原生 runtime （作為 Docker 的輕量級替代方案）

WasmEdge 可以透過其 [C](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md) 、 [Go](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) 、 [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/wasmedge-rs) 和[JavaScript](https://www.secondstate.io/articles/getting-started-with-rust-function/)的 SDK 嵌入到雲端原生基礎架構中。它也是一個符合 OCI 的 runtime ，可以由 [CRI-O 和 Docker 工具直接管理](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) ，作為 Docker 的輕量級和高效能替代方案。

### Dapr （分散式應用程式 Runtime）

* 教學 (待發佈)
* [程式碼教學](https://github.com/second-state/dapr-wasm)

### Service mesh （開發進行中）：

* Linkerd
* MOSN
* Envoy

### 編排和管理 （開發進行中）：

* Kubernetes
* KubeEdge
* SuperEdge

## JavaScript 或 DSL runtime

為了使 WebAssembly runtime 被開發者廣泛採用，它必須支援像 JavaScript 這樣的「簡單」程式語言。或者，更棒的是，透過其高階編譯器工具鏈， WasmEdge 可以支援高效能 DSL （特定領域語言），這是專為特定任務設計的低程式碼解決方案。

### JavaScript

WasmEdge 可以透過嵌入 JS 執行引擎或直譯器來作為雲端原生 JavaScript runtime ，比起在 Docker 中運行 JS 引擎還更快更輕量。 WasmEdge 支援 JS API 呼叫原生擴充函式庫，例如網路 sockets 、 TensorFlow 和使用者定義的共享函式庫。 WasmEdge 還允許將 JS 嵌入其他高效能程式語言（例如 Rust）或使用 Rust/C 來實作 JS 函式。

* [教學](https://www.secondstate.io/articles/run-javascript-in-webassembly-with-wasmedge/)
* [範例](https://github.com/WasmEdge/WasmEdge/blob/master/tools/wasmedge/examples/js/README.md)
* [WasmEdge 的内嵌 QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)

### 用於影像辨識的 DSL

影像辨識 DSL 是一種允許使用者指定 TensorFlow 模型與其參數的 YAML 格式。 WasmEdge 將圖片作為 DSL 的輸入，並輸出偵測到的物件名稱/標籤。

* 範例： [執行 YMAL 以辨識圖片中的食物](https://github.com/second-state/wasm-learning/blob/master/cli/classify_yml/config/food.yml) 

### 用於聊天機器人 DSL

聊天機器人 DSL 函式接受輸入字串並輸出字串進行回覆。 DSL 指定了聊天機器人的內部狀態轉變，以及用於語言理解的 AI 模型。這個項目正在開發中。

## 公開雲端中的 Serverless 函式即服務

WasmEdge 與現有的 Serverless 或 Jamstack 平台配合使用，為函式提供高效能、可移植和安全的 runtime 。即使在這些平台的 Docker 或 microVM 中執行，也能提供顯著的優勢。

### AWS Lambda

* [教學](https://www.cncf.io/blog/2021/08/25/webassembly-serverless-functions-in-aws-lambda/)
* [程式碼範本](https://github.com/second-state/aws-lambda-wasm-runtime)

### 騰訊 Serverless 函式

* [中文教學](https://my.oschina.net/u/4532842/blog/5172639)
* [程式碼範本](https://github.com/second-state/tencent-scf-wasm-runtime)

### Vercel Serverless 函式

* [教學](https://www.secondstate.io/articles/vercel-wasmedge-webassembly-rust/)
* [程式碼範本](https://github.com/second-state/vercel-wasm-runtime)

### Netlify 函式

* [教學](https://www.secondstate.io/articles/netlify-wasmedge-webassembly-rust-serverless/)
* [程式碼範本](https://github.com/second-state/netlify-wasm-runtime)

### Second State 函式

* [教學](https://www.secondstate.io/faas/)

## 支援軟體控制的汽車和 AIoT

WasmEdge 非常適合在需要執行關鍵任務的終端裝置或邊緣網路上執行。

### YoMo Flow

YoMo 是一種用於遠端邊緣（far edge）網路的高效能資料流框架。 WasmEdge 集成到 YoMo 中以執行使用者定義的工作負載，例如在工廠組裝線上的影像辨識。

* [教學](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/)
* [程式碼範本](https://github.com/yomorun/yomo-wasmedge-tensorflow)

### seL4 微型核心作業系統

seL4 是一個高度安全的即時作業系統。 WasmEdge 是唯一可以在 seL4 上以原生速度運行的 WebAssembly runtime 。我們還提供了一個管理工具來支援 Wasm module 的 OTA 部署。這個項目正在開發中。

## 用於 SaaS 的互動函式

WasmEdge 可以使用 Serverless 函式而不是傳統的網路 API 來支援客製化的 SaaS 擴充或應用程式。這大大地提高 SaaS 使用者和開發者的生產力。

### Slack

* [為 Slack 建立 serverless 聊天機器人](http://reactor.secondstate.info/en/docs/user_guideline.html)

### 飛書

飛書為字節跳動，即抖音母公司，旗下的聊天軟體。

* [為非書建立 serverless 聊天機器人](http://reactor.secondstate.info/zh/docs/user_guideline.html)

如果對於 WasmEdge 有任何建議，歡迎提出 [GitHub issue](https://github.com/WasmEdge/WasmEdge/issues) 來討論。
