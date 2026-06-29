<div align="right">

  [Readme in English](README.md) | [中文文档](README-zh.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">

![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

WasmEdge （前名為 SSVM ） 是為邊緣運算最佳化的輕量級、高性能、可擴充的 WebAssembly (Wasm) 虛擬機器，可運用在雲端原生、邊緣運算、去中心化的應用程式。 WasmEdge 現今是目前市面上 [最快的 Wasm 虛擬機器之一](https://ieeexplore.ieee.org/document/9214403)。 WasmEdge 是由 [CNCF](https://www.cncf.io/) （Cloud Native Computing Foundation ，雲端原生運算基金會）託管的官方沙箱項目之一。其[使用情境](https://wasmedge.org/docs/zh-tw/start/usage/use-cases)包含 serverless apps 、嵌入式函數、微型服務、智慧合約和 IoT 裝置。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# 快速開始指引

🚀 [安装](https://wasmedge.org/docs/zh-tw/start/install) WasmEdge\
🤖 [編譯](https://wasmedge.org/docs/zh-tw/category/build-wasmedge-from-source) 並[貢獻](https://wasmedge.org/docs/zh-tw/contribute/) WasmEdge\
⌨️ [從 CLI 執行](https://wasmedge.org/docs/zh-tw/category/running-with-wasmedge)一個獨立的 Wasm 程式或 [JavaScript 程式](https://wasmedge.org/docs/zh-tw/category/develop-wasm-apps-in-javascript) \
🔌 在 [Node.js](https://github.com/second-state/wasm-learning/tree/master/ssvm/file-example) 、 [Go 語言](https://wasmedge.org/docs/zh-tw/category/go-sdk-for-embedding-wasmedge)、或是 [Rust 應用程式](https://www.secondstate.io/articles/getting-started-with-rust-function/)嵌入 Wasm 函式 \
🛠 使用 [Docker 工具](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/)、[即時資料流框架](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/), 和 [區塊鏈](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a) 管理和編排 Wasm runtimes

# 簡介

WasmEdge 為其執行的 Wasm bytecode 程式提供了一個有良好定義的沙箱。這個專案的 Runtime 為作業系統資源（例如：檔案系統、 Sockets 、環境變數、程序）和記憶體空間提供獨立性與保護。 WasmEdge 的最重要應用領域是在軟體產品（例如：SaaS 、汽車作業系統、邊緣節點，甚至區塊鏈節點）中安全地執行使用者自定義或者經由社群貢獻的程式碼。它使第三方開發者、軟體供應商和社群成員能夠擴充和客製化應用軟體。

<div align="center">

**檢視 WasmEdge 的[使用情境](https://wasmedge.org/docs/zh-tw/contribute/users)。**

</div>

## 效能

* 論文：[高效能 Serverless 計算的輕量級設計](https://arxiv.org/abs/2010.07115)，發表於 IEEE Software ，2021年1月。 [https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* 文章：[Performance Analysis for Arm vs. x86 CPUs in the Cloud](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)，發表於 infoQ.com ，2021年1月。 [https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)

## 特性

WasmEdge 可以執行從 C/C++ 、 Rust 、 Swift 、 AssemblyScript 或 Kotlin 原始碼編譯的標準 WebAssembly bytecode 應用程式，也可以透過嵌入式 [QuickJS 引擎](https://github.com/second-state/wasmedge-quickjs)[執行 JavaScript](https://wasmedge.org/docs/zh-tw/category/develop-wasm-apps-in-javascript)。 WasmEdge 支援[所有標準的 WebAssembly 特性和許多擴充提案](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)，還支援許多為了原生雲端運算與邊緣運算量身定制的擴充（例如： [WasmEdge 網路套接字](https://wasmedge.org/docs/category/socket-networking)、[Postgres 和 MySQL 資料庫驅動程式](https://wasmedge.org/docs/category/database-drivers)，以及 [WasmEdge AI 擴充](https://wasmedge.org/docs/category/ai-inference)）。

WebAssembly 的 WasmEdge 擴充通常作為 Rust SDK 或 [JavaScript APIs](https://wasmedge.org/docs/zh-tw/category/develop-wasm-apps-in-javascript) 提供給開發者。

## 集成

WasmEdge 及其執行的 Wasm 應用程式可以作為新應用程序或以現有的程序從 CLI 啟動。如果從現有程序啟動（例如，從正在執行的 [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) 、 [Golang](https://github.com/second-state/wasmedge-go) 或 [Rust](https://github.com/WasmEdge/WasmEdge/tree/master/bindings/rust) 程序）， WasmEdge 將簡單地作為一個函式在程序內運行。目前， WasmEdge 還不是執行緒安全的。如您想在自己的應用程式或者原生雲端框架中使用 WasmEdge ，請參考以下指南。

* [在應用程式中嵌入 WasmEdge](https://wasmedge.org/docs/zh-tw/embed/overview)
* [使用容器工具管理和編排 WasmEdge 實例](https://wasmedge.org/docs/zh-tw/category/deploy-wasmedge-apps-in-kubernetes)
* [從 WasmEdge 呼叫原生 host 函式](docs/integrations.md#call-native-host-functions-from-wasmedge)

## 社群

### 貢獻

如果您想為 WasmEdge 專案做出貢獻，請參閱我們的 [CONTRIBUTING](https://wasmedge.org/docs/contribute/overview/) 文件瞭解詳情。 想要獲得靈感，可查看 [需求清單](https://github.com/WasmEdge/WasmEdge/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)。

### 聯繫

如有任何疑問，請隨時在相關項目上提出 GitHub issue ，或：

* 電子郵件：發送郵件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Slack ：加入 #WasmEdge 群組： [CNCF Slack](https://slack.cncf.io/)
* 推特：在 [Twitter](https://twitter.com/realwasmedge) 跟隨 @realwasmedge

## License

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
