# 快速开始

使用 WasmEdge 最簡單的方式就是使用 WasmEdge 命令列工具（ CLI ）。
開發者能使用 WasmEdge CLI 工具來執行我們的 WebAssembly 與 Javascript 範例程式。
此外，我們也能使用它來建立新的 WasmEdge 程式，並部署到不同的應用程式或框架中執行。

## 安裝

您可以使用我們的安裝腳本來安裝 WasmEdge 。
您的環境必須已有安裝 `git` 和 `curl` 。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

若您想安裝附帶有 [Tensorflow 與 image 擴充](https://www.secondstate.io/articles/wasi-tensorflow/) 的 WasmEdge ，請執行以下指令。
下列指令將嘗試在您的系統安裝 Tensorflow 與 image 相關的相依函式庫。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

執行以下指令可以讓已安裝的 WasmEdge 執行檔與函式庫在當前 session 加入 `PATH` 路徑中。

```bash
source $HOME/.wasmedge/env
```

## 使用 Docker

若您使用 Docker ，您可以使用 WasmEdge 應用程式開發用的映像檔 ([x86](https://hub.docker.com/repository/docker/wasmedge/appdev_x86_64) 和 [arm64](https://hub.docker.com/repository/docker/wasmedge/appdev_aarch64)) 。這些映像檔裡包含了所有開發 WasmEdge 應用程式所需要的工具。

```bash
$ docker pull wasmedge/appdev_x86_64:0.9.0
$ docker run --rm -v $(pwd):/app -it wasmedge/appdev_x86_64:0.9.0
(docker) #
```

## WebAssembly 範例

我們有數個 WebAssembly 應用程式範例，您可以使用您安裝的 WasmEdge CLI 嘗試。

### Hello world

[hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.wasm) 這個 WebAssembly 應用程式包含一個 `main()` 函式。
請參考它的 [Rust 原始碼](https://github.com/second-state/wasm-learning/tree/master/cli/hello)。
它會在標準輸出印出 `hello` 與傳入的命令列參數。

```bash
$ wasmedge hello.wasm second state
hello
second
state
```

### 呼叫一個使用 Rust 編寫的函式

[add.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/add.wasm) 這個 WebAssembly 應用程式包含一個 `add()` 函式。
請參考它的 [Rust 原始碼](https://github.com/second-state/wasm-learning/tree/master/cli/add)。
我們使用 WasmEdge 的 reactor 模式來呼叫 `add()` ，並傳入兩個整數作為參數。

```bash
$ wasmedge --reactor add.wasm add 2 2
4
```

### 呼叫一個使用 WAT 編寫的函式

我們編寫了一個 [fibonacci.wat](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wat) 程式，並使用 [wat2wasm](https://github.com/WebAssembly/wabt) 工具來組譯成 [fibonacci.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/fibonacci.wasm) 這個 WebAssembly 檔案。
它包含一個 `fib()` 函式，並以一個整數作為參數。我們使用 WasmEdge 的 reactor 模式來呼叫這個函式。

```bash
$ wasmedge --reactor fibonacci.wasm fib 10
89
```

### 啟用統計資訊

CLI 工具支援 `--enable-all-statistics` 選項，可以啟用統計和 gas 計算等相關設置。

```bash
$ wasmedge --enable-all-statistics hello.wasm second state
hello
second
state
[2021-12-09 16:03:33.261] [info] ====================  Statistics  ====================
[2021-12-09 16:03:33.261] [info]  Total execution time: 268266 ns
[2021-12-09 16:03:33.261] [info]  Wasm instructions execution time: 251610 ns
[2021-12-09 16:03:33.261] [info]  Host functions execution time: 16656 ns
[2021-12-09 16:03:33.261] [info]  Executed wasm instructions count: 20425
[2021-12-09 16:03:33.261] [info]  Gas costs: 20425
[2021-12-09 16:03:33.261] [info]  Instructions per second: 81177218
[2021-12-09 16:03:33.261] [info] =======================   End   ======================
```

### 啟用 gas-limit 設定

CLI 工具支援 `--gas-limit` 選項，可以限制執行的 gas 限制。

```bash
# cd <path/to/WasmEdge>
$ cd examples/wasm
# With enough gas
$ wasmedge --enable-all-statistics --gas-limit 20425 hello.wasm second state
hello
second
state
[2021-12-09 16:03:33.261] [info] ====================  Statistics  ====================
[2021-12-09 16:03:33.261] [info]  Total execution time: 268266 ns
[2021-12-09 16:03:33.261] [info]  Wasm instructions execution time: 251610 ns
[2021-12-09 16:03:33.261] [info]  Host functions execution time: 16656 ns
[2021-12-09 16:03:33.261] [info]  Executed wasm instructions count: 20425
[2021-12-09 16:03:33.261] [info]  Gas costs: 20425
[2021-12-09 16:03:33.261] [info]  Instructions per second: 81177218
[2021-12-09 16:03:33.261] [info] =======================   End   ======================

# Without enough gas
$ wasmedge --enable-all-statistics --gas-limit 20 hello.wasm second state
[2021-12-23 15:19:06.690] [error] Cost exceeded limit. Force terminate the execution.
[2021-12-23 15:19:06.690] [error]     In instruction: ref.func (0xd2) , Bytecode offset: 0x00000000
[2021-12-23 15:19:06.690] [error]     At AST node: expression
[2021-12-23 15:19:06.690] [error]     At AST node: element segment
[2021-12-23 15:19:06.690] [error]     At AST node: element section
[2021-12-23 15:19:06.690] [error]     At AST node: module
[2021-12-23 15:19:06.690] [info] ====================  Statistics  ====================
[2021-12-23 15:19:06.690] [info]  Total execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Wasm instructions execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Host functions execution time: 0 ns
[2021-12-23 15:19:06.690] [info]  Executed wasm instructions count: 21
[2021-12-23 15:19:06.690] [info]  Gas costs: 20
```

## JavaScript 範例

WasmEdge 也可以作為一個高效能、安全、可擴充、容易部署、以及 [相容 Kubernetes](https://github.com/second-state/wasmedge-containers-examples) 的 JavaScript runtime 來使用。

[qjs.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/qjs.wasm) 是一個編譯成 WebAssembly 的 JavaScript 直譯器。
[hello.js](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.js) 是一個簡單的 JavaScript 應用程式。

```bash
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

[qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/qjs_tf.wasm) 是一個附有 [WasmEdge Tensorflow 擴充](https://www.secondstate.io/articles/wasi-tensorflow/) 的 WebAssembly 版本的 JavaScript 直譯器。
為了可以執行 [qjs_tf.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/qjs_tf.wasm) ，您必須使用 `wasmedge-tensorflow-lite` 這個 CLI 工具，這個工具裡內建了 WasmEdge 的 Tensorflow 擴充。
您可以下載一個完整的 [基於 Tensorflow 的 JavaScript 範例](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/tensorflow_lite_demo) 來嘗試分辨圖片。

```bash
# Download the Tensorflow example
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/aiy_food_V1_labelmap.txt
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/food.jpg
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/lite-model_aiy_vision_classifier_food_V1_1.tflite
$ wget https://raw.githubusercontent.com/second-state/wasmedge-quickjs/main/example_js/tensorflow_lite_demo/main.js

$ wasmedge-tensorflow-lite --dir .:. qjs_tf.wasm main.js
label: Hot dog
confidence: 0.8941176470588236
```

您可以繼續閱讀來學習使用 WasmEdge 。

- [WasmEdge 的安裝與解除安裝](start/install.md)
- [WasmEdge 命令列](start/cli.md)
- [WasmEdge 使用情境](intro/use.md)
- [WasmEdge 的優勢與特色](intro/features.md)
