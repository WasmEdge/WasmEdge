<div align="right">

  [中文](README-zh.md) | [正體中文](README-zh-TW.md) | [日本語で読む](README-ja.md)

</div>

<div align="center">
  
![WasmEdge Logo](/docs/wasmedge-runtime-logo.png)

# [🤩 WasmEdge 是在您自己的設備上運行 LLM 的最簡單、最快的方法。🤩](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)

<a href="https://trendshift.io/repositories/2481" target="_blank"><img src="https://trendshift.io/api/badge/repositories/2481" alt="WasmEdge%2FWasmEdge | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>

WasmEdge 是一個輕量級、高效能、可擴展的 WebAssembly 執行階段。它是[速度最快的 Wasm 虛擬機](https://ieeexplore.ieee.org/document/9214403)。WasmEdge 是由 [CNCF](https://www.cncf.io/) 託管的官方沙箱專案。[LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 是一個建立在 WasmEdge 之上的應用程式框架，用於在伺服器、個人電腦和邊緣裝置的 GPU 上運行 GenAI 模型（例如 [LLM](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge)、[語音轉文字](https://llamaedge.com/docs/user-guide/speech-to-text/quick-start-whisper)、[文字轉圖像](https://llamaedge.com/docs/user-guide/text-to-image/quick-start-sd) 和 [TTS](https://github.com/LlamaEdge/whisper-api-server)）。其他[用例](https://wasmedge.org/docs/start/usage/use-cases/)包括邊緣雲上的微服務、無伺服器 SaaS API、嵌入式函數、智慧合約和智慧裝置。

[![build](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/build.yml?query=event%3Apush++branch%3Amaster)
[![codecov](https://codecov.io/gh/WasmEdge/WasmEdge/branch/master/graph/badge.svg)](https://codecov.io/gh/WasmEdge/WasmEdge)
[![CodeQL](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/WasmEdge/WasmEdge/actions/workflows/codeql-analysis.yml?query=event%3Apush++branch%3Amaster)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_shield)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059/badge)](https://bestpractices.coreinfrastructure.org/projects/5059)

</div>

# 快速入門指南

🚀 [安裝](https://wasmedge.org/docs/start/install) WasmEdge \ 
👷🏻‍♂️ [建置](https://wasmedge.org/docs/category/build-wasmedge-from-source)並[為 WasmEdge 做出貢獻](https://wasmedge.org/docs/contribute/) \ 
⌨️ 從 CLI 或 [Docker](https://wasmedge.org/docs/start/getting-started/quick_start_docker) [執行](https://wasmedge.org/docs/category/running-with-wasmedge)一個獨立的 Wasm 程式或一個 [JavaScript 程式](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript) \ 
🤖 透過 [LlamaEdge](https://github.com/LlamaEdge/LlamaEdge) 與一個開源 LLM [聊天](https://llamaedge.com/docs/user-guide/llm/get-started-with-llamaedge) \ 
🔌 在您的 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge)、[Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) 或 [C](https://wasmedge.org/docs/category/c-sdk-for-embedding-wasmedge) 應用程式中嵌入一個 Wasm 函數 \ 
🛠 使用 [Kubernetes](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)、[資料流框架](https://wasmedge.org/docs/embed/use-case/yomo)和[區塊鏈](https://medium.com/ethereum-on-steroids/running-ethereum-smart-contracts-in-a-substrate-blockchain-56fbc27fc95a)來管理和編排 Wasm 執行階段 \ 
📚 **[查看我們的官方文件](https://wasmedge.org/docs/)**

# 簡介

WasmEdge 執行階段為其包含的 WebAssembly 位元組碼程式提供了一個定義明確的執行沙箱。該執行階段為作業系統資源（例如，檔案系統、通訊端、環境變數、進程）和記憶體空間提供隔離和保護。WasmEdge 最重要的用例是在軟體產品（例如，SaaS、軟體定義汽車、邊緣節點，甚至區塊鏈節點）中作為外掛程式安全地執行使用者定義或社群貢獻的程式碼。它使第三方開發人員、供應商、供應商和社群成員能夠擴展和自訂軟體產品。**[在此處了解更多資訊](https://wasmedge.org/docs/contribute/users)**

## 效能

* [一種用於高效能無伺服器計算的輕量級設計](https://arxiv.org/abs/2010.07115)，發表於 IEEE Software，2021 年 1 月。[https://arxiv.org/abs/2010.07115](https://arxiv.org/abs/2010.07115)
* [雲端中 Arm 與 x86 CPU 的效能分析](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)，發表於 infoQ.com，2021 年 1 月。[https://www.infoq.com/articles/arm-vs-x86-cloud-performance/](https://www.infoq.com/articles/arm-vs-x86-cloud-performance/)
* [WasmEdge 是 Suborbital Reactr 測試套件中最快的 WebAssembly 執行階段](https://blog.suborbital.dev/suborbital-wasmedge)，2021 年 12 月

## 特性

WasmEdge 可以執行從 C/C++、Rust、Swift、AssemblyScript 或 Kotlin 原始碼編譯的標準 WebAssembly 位元組碼程式。它在一個安全、快速、輕量級、可移植和容器化的沙箱中[執行 JavaScript](https://wasmedge.org/docs/category/develop-wasm-apps-in-javascript)，包括第三方 ES6、CJS 和 NPM 模組。它還支援混合這些語言（例如，[使用 Rust 實現 JavaScript API](https://wasmedge.org/docs/develop/javascript/rust)）、[Fetch API](https://wasmedge.org/docs/develop/javascript/networking#fetch-client) 和邊緣伺服器上的[伺服器端渲染 (SSR)](https://wasmedge.org/docs/develop/javascript/ssr) 功能。

WasmEdge 支援[所有標準的 WebAssembly 功能和許多提議的擴展](https://wasmedge.org/docs/start/wasmedge/extensions/proposals)。它還支援許多為雲端原生和邊緣運算用途量身定制的擴展（例如，[WasmEdge 網路通訊端](https://wasmedge.org/docs/category/socket-networking)、[基於 Postgres 和 MySQL 的資料庫驅動程式](https://wasmedge.org/docs/category/database-drivers)和 [WasmEdge AI 擴展](https://wasmedge.org/docs/category/ai-inference)）。

**了解有關 [WasmEdge 的技術亮點](https://wasmedge.org/docs/start/wasmedge/features)的更多資訊。**

## 整合與管理

WasmEdge 及其包含的 wasm 程式可以從 [CLI](https://wasmedge.org/docs/category/running-with-wasmedge) 作為一個新進程啟動，也可以從現有進程啟動。如果從現有進程（例如，從正在運行的 [Go](https://wasmedge.org/docs/category/go-sdk-for-embedding-wasmedge) 或 [Rust](https://wasmedge.org/docs/category/rust-sdk-for-embedding-wasmedge) 程式）啟動，WasmEdge 將簡單地作為函數在進程內運行。目前，WasmEdge 還不是線程安全的。為了在您自己的應用程式或雲端原生框架中使用 WasmEdge，請參閱以下指南。

* [將 WasmEdge 嵌入到宿主應用程式中](https://wasmedge.org/docs/embed/overview)
* [使用容器工具編程和管理 WasmEdge 實例](https://wasmedge.org/docs/category/deploy-wasmedge-apps-in-kubernetes)
* [將 WasmEdge 應用程式作為 Dapr 微服務運行](https://wasmedge.org/docs/develop/rust/dapr)

# 社群

## 貢獻

我們歡迎社群的貢獻！請查看我們的：
- [貢獻指南](./docs/CONTRIBUTING.md) 以了解如何開始
- [治理文件](./docs/GOVERNANCE.md) 以了解專案決策過程
- [行為準則](./docs/CODE_OF_CONDUCT.md) 以了解社群標準

想成為維護者嗎？請參閱我們的[貢獻者階梯](./CONTRIBUTION_LADDER.md)。

## 路線圖

查看我們的[專案路線圖](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md)以了解 WasmEdge 即將推出的功能和計劃。

## 聯繫

如果您有任何問題，請隨時在相關專案上提出 GitHub 問題或加入以下管道：

* 郵件列表：發送電子郵件至 [WasmEdge@googlegroups.com](https://groups.google.com/g/wasmedge/)
* Discord：加入 [WasmEdge Discord 伺服器](https://discord.gg/h4KDyB8XTt)！
* Slack：在 [CNCF Slack](https://slack.cncf.io/) 上加入 #WasmEdge 頻道
* X (前 Twitter)：在 [X](https://x.com/realwasmedge) 上關注 @realwasmedge

## 採用者

查看我們的[採用者列表](https://wasmedge.org/docs/contribute/users/)，他們在自己的專案中使用 WasmEdge。

## 社群會議

我們每月舉辦一次社群會議，展示新功能、演示新用例，並設有問答環節。歡迎大家參加！

時間：每月第一個星期二，香港時間晚上 11 點/太平洋標準時間早上 7 點。

[公開會議議程/記錄](https://docs.google.com/document/d/1iFlVl7R97Lze4RDykzElJGDjjWYDlkI8Rhf8g4dQ5Rk/edit#) | [Zoom 連結](https://us06web.zoom.us/j/82221747919?pwd=3MORhaxDk15rACk7mNDvyz9KtaEbWy.1)

# 授權

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FWasmEdge%2FWasmEdge?ref=badge_large)
