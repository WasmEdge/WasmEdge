
# 快速開始指引

🚀 [安裝](docs/install.md) WasmEdge\
🤖 [從原始碼編譯 WasmEdge](docs/build.md)\
🤖 [貢獻 WasmEdge](docs/contribution.md)\
⌨️ [從 CLI](docs/run.md)  或 [Node.js](https://github.com/second-state/wasm-learning/tree/master/ssvm/file-example) 或 [Golang](https://github.com/second-state/WasmEdge-go/tree/master/examples/go_mtcnn) 執行一個獨立的 Wasm 程式\
💭 發佈一個 [Wasm 函式即 web 服務 (FaaS)](https://www.secondstate.io/articles/getting-started-with-function-as-a-service-in-rust/)\
🛠 [在 SaaS 平台上嵌入使用者自定義 Wasm 函式](http://reactor.secondstate.info/docs/user-create-a-bot.html)\
🔩 [在 Node.js 網頁應用程式裡嵌入 Wasm 函式](https://www.secondstate.io/articles/getting-started-with-rust-function/)\
🔌 [在 Golang 應用程式嵌入 Wasm 函式](https://github.com/second-state/WasmEdge-go/tree/master/examples/go_PassBytes)\
🔗 [將 Wasm 函式部署為區塊鏈智慧合約](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)\

![build](https://github.com/WasmEdge/WasmEdge/workflows/build/badge.svg)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/WasmEdge/WasmEdge.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/WasmEdge/WasmEdge/context:cpp)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)

# 簡介

WasmEdge (前名為 SSVM) 是為邊緣運算最佳化的高性能 WebAssembly (Wasm) 虛擬機器，應用範圍包含了邊緣雲端和搭載軟體的汽車。在 AOT 模式下, WasmEdge 是目前市面上 [最快的 Wasm 虛擬機器之一](https://ieeexplore.ieee.org/document/9214403)。

WasmEdge 是 [CNCF](https://www.cncf.io/) （ Cloud Native Computing Foundation ，雲端原生運算基金會）轄下的官方沙箱專案項目之一。

WasmEdge 的最重要應用領域是在軟體產品（例如， SaaS 、汽車作業系統、邊緣節點，甚至區塊鏈節點）中安全地執行使用者自定義或者經由社群貢獻的程式碼。它使第三方開發者、軟體供應商和社群成員能夠擴充和客製化應用軟體。意即，甚至可以透過 WasmEdge 打造一個軟體部署平台。

WasmEdge 為其執行的 Wasm bytecode 程式提供了一個有良好定義的沙箱。透過組態設定（也就是基於權限設定的安全性模型）， WasmEdge 可以控制虛擬機器執行時能夠存取的系統資源。如果沒有虛擬機器的明確權限， Wasm bytecode 程式無法存取作業系統資源（例如，檔案系統、網路介面、環境變數、應用程序）。

WasmEdge 還為其執行的 Wasm 應用程式提供記憶體保護。 如果應用程式嘗試存取分配給虛擬機器的區域之外的記憶體，則虛擬機器將終止應用程式並顯示錯誤訊息。

WasmEdge 及其執行的 Wasm 應用程式可以作為新應用程序或以現有的程序從 CLI 啟動。如果從現有程序啟動（例如，從正在執行的 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 或 [Golang](https://github.com/second-state/wasmedge-go) 程序），虛擬機器將簡單地作為一個函式在程序內運行。也可以將 WasmEdge 虛擬機器實例以新的執行緒啟動。 目前， WasmEdge 還不是執行緒安全的，這意味著在同一程序中的不同執行緒中運行的虛擬機器實例可能能夠存取彼此的記憶體。 未來，我們計畫讓 WasmEdge 做到執行緒安全。


# 將 WasmEdge 嵌入 host 應用程式

WasmEdge 的一個主要應用場景是從一個 host 應用程式啟動一個虛擬機器實例，你可以使用 [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge.h.in) 做到這一點。

可是， Wasm 說明文件和 [WasmEdge C API](https://github.com/WasmEdge/WasmEdge/blob/master/include/api/wasmedge.h.in) 僅在 Wasm bytecode 函式的輸入參數和回傳值支援非常有限的資料型態。如果要將複雜的資料型態（例如字串的陣列）作為引數傳遞給 Wasm 函式，應該使用 [rustwasmc](https://github.com/second-state/rustwasmc) 工具鏈提供的 bindgen 解決方案。

我們目前 [支援 Node.js 環境中的 bindgen](https://www.secondstate.io/articles/getting-started-with-rust-function/) 。我們正在致力於在 Golang 和基於 Rust 的 host 應用程式中支援 bindgen。

# 從 WasmEdge 呼叫原生 host 函式

開發者或許會認為如果僅有 Wasm bytecode 本身的功能對有些應用程式來說有太多的限制。 WasmEdge 提供一個 [host 函式 API](https://github.com/WasmEdge/WasmEdge/blob/master/docs/host_function.md) 。這允許 Wasm bytecode 應用程式從底層 host 作業系統載入和呼叫原生函式庫的函式。

>此功能破壞了 Wasm 沙箱。 但是這個沙箱破壞是在系統管理員明確權限下完成的。

事實上，對 WasmEdge 的擴充是使用原生 host 函式實現的。 例如， [Tensorflow 擴充](https://www.secondstate.io/articles/wasi-tensorflow/) 允許 Wasm bytecode 使用原生 Tensorflow 函式庫。

# 管理 WasmEdge 虛擬機器實例

有了 [WasmEdge C API](docs/c_api.md) ，您可以編寫程式來啟動、停止和管理您自己的應用程式中的 WasmEdge 虛擬機器實例。例如：

* 當在 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 或者在 [飛書](http://reactor.secondstate.info/docs/user-create-a-bot.html) 上嵌入了 WasmEdge 函式，可以在當有傳入請求時，由應用程式啟動虛擬機器。
* 當 WasmEdge 函式被外掛到像 [YoMo](https://github.com/yomorun/yomo-flow-ssvm-example) 這樣的資料流引擎中時，虛擬機器可以在一個新的資料點流經系統時啟動。
* 作為一個相容於 OCI 的執行體， WasmEdge 應用程式可以由像是 CRI-O 與 Docker Hub 的 Docker 工具來管理。 [由此觀看](https://github.com/second-state/runw) 我們正在進行的對 Kubernetes 的支援。

您也可以參閱 [WasmEdge C API 快速開始指南](docs/c_api_quick_start.md) 。

# 支援 wasm 標準擴充

WasmEdge 支持可選用的 WebAssembly 新特性和提案。這些提案很可能在未來成為官方的 WebAssembly 標準。 WasmEdge 支援以下提案。

* [WASI (WebAssembly 系統介面) spec](https://github.com/WebAssembly/WASI) 。 WasmEdge 支援 WebAssembly 應用程式的 WASI 標準，以安全地與 host Linux 作業系統互動。
* [Reference Types](https://webassembly.github.io/reference-types/core/) 。它允許 WebAssembly 應用程式來與 host 應用程式和作業系統交換資料。
* [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md) 。 能使 WebAssembly 應用程式的記憶體讀寫更快並在大容量記憶體操作中表現更好。
* [SIMD (Single instruction, multiple data)](https://github.com/second-state/SSVM/blob/master/docs/simd.md) 。對於具有多個 CPU 核心的現代設備， SIMD 允許資料處理應用程式充分利用 CPU 。 SIMD 可以顯著提高資料應用程式的性能。

同時， WasmEdge 團隊正 [探索wasi-socket提案](https://github.com/second-state/w13e_wasi_socket) 以支援 WebAssembly 應用程式中的網路存取。

# WasmEdge 擴充

WasmEdge 與其它的 WebAssembly 虛擬機器的關鍵區別是它對非標準擴充的支援。 WASI 標準為開發者提供了一種有效且安全地擴充 WebAssembly 虛擬機器的機制。 WasmEdge 團隊根據現實中客戶需求創造了以下類似 WASI 的擴充。

* [Tensorflow](https://github.com/second-state/wasmedge-tensorflow) 。開發者可以使用 [簡單的 Rust API](https://crates.io/crates/wasmedge_tensorflow_interface) 編寫 Tensorflow inference 函式，然後在 WasmEdge 內以原生速度安全地執行該函式。
* 其他 AI 框架。除了 Tensorflow ， Second State 團隊還在為 AI 框架（如用於 WasmEdge 的ONNX 和 Tengine ）建構類似 WASI 的擴充。
* [Storage](https://github.com/second-state/wasmedge-storage) 。 WasmEdge [Storage 接口](https://github.com/second-state/rust_native_storage_library) 允許 WebAssembly 應用程式以類似資料庫的方式讀取和寫入鍵值。
* [命令列界面](https://github.com/second-state/wasmedge_process_interface) 。 WasmEdge 讓 Webassembly 得以執行 host 作業系統的本地端指令。它支援傳遞參數、環境變數、標準輸入輸出和 host 存取的安全政策。
* [以太坊](https://github.com/second-state/wasmedge-evmc)。 WasmEdge Ewasm 擴充支援編譯為 WebAssembly 的以太坊智慧合約。這是一個支援以太坊風格的 WebAssembly (Ewasm) 的領導級實作。
* [Substrate](https://github.com/second-state/substrate-ssvm-node)。 [Pallet](https://github.com/second-state/pallet-ssvm) 讓 WasmEdge 能在任何基於 Substrate 的區塊鏈上充當以太坊智慧合約執行引擎。


# 應用場景

* *Jamstack 應用程式* 由帶有 JavaScript 的靜態前端組成，與後端 API 進行互動。這是現在流行的[現代 web 應用程式架構](https://jamstack.org/) 。前端靜態網頁可以透過 CDN 發佈，後端函式可以部署在邊緣節點上。 [基於雲端的 WasmEdge](https://www.secondstate.io/faas/) 為 Jamstack app 部署安全且高性能的後端 Serverless 函式，特別是在邊緣雲端上。
  * 案例： [幫 web app 上的任意圖片增加浮水印](https://second-state.github.io/wasm-learning/faas/watermark/html/index.html) 。
  * 案例： [基於騰訊雲的 serverless Tensorflow 函式](https://github.com/second-state/tencent-tensorflow-scf) 。
* *SaaS 應用程式* 通常需要根據客戶要求「在終端」進行量身定製或客製化。使用 WasmEdge ， SaaS 應用程式可以直接嵌入和執行用戶提交的程式碼，並作為工作流程的一部分（例如作為處理來自 SaaS 應用程序的事件的回呼函式）。
  * 案例： [飛書應用平台](http://reactor.secondstate.info/docs/user-create-a-bot.html) 可以透過 WasmEdge 嵌入用戶提交的 serverless 函式來回覆訊息（例如 [聊天機器人](https://app.feishu.cn/app/cli_a08fe99f8169900d)）。
  * 案例： [WasmEdge 運行自定義程式碼來處理 IoT 資料流框架「 YoMo 」中的事件](https://github.com/yomorun/yomo-flow-ssvm-example) 。
* WasmEdge 被調整為適用*終端設備*的各種嵌入式和即時作業系統。 這讓開發者只需用 Rust 或 C 編寫一次高性能應用程式，就能在許多終端設備平台上安全地運行。
  * 案例： [RIOS Lab 範例： RIOS 實驗室的 RISC-V 堆疊](https://rioslab.org/)。
  * 進行中：將 WasmEdge 移植到 SeL4 即時作業系統。
  * 計劃中：將 WasmEdge 作為自動駕駛汽車中 RTOS 上軟體模組的程式碼執行環境。
* *區塊鏈智慧合約* 是用戶提交程式碼，由網路中的所有節點執行。 WasmEdge 得到領導級的區塊鏈企劃項目採用，作為智慧合約執行引擎。
  * 案例: [Substrate 和 Polkadot 上的 EWASM 智慧合約](https://github.com/ParaState/substrate-ssvm-node)
  
  
## 社群

### 貢獻

如果您想為 WasmEdge 專案做出貢獻，請參閱我們的 [CONTRIBUTING](docs/CONTRIBUTING.md) 文件瞭解詳情。 想要獲得靈感，可查看 [需求清單](docs/wish_list.md)!

### 聯繫

如有任何疑問，請隨時在相關項目上提出 GitHub issue ，或：

* 發送郵件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Slack: 加入 #WasmEdge 群組： [CNCF Slack](https://slack.cncf.io/)

## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
