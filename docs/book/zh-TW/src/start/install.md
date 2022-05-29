# WasmEdge 的安裝與解除安裝

## 快速安裝

安裝 WasmEdge 最簡單的方式是執行以下指令來執行安裝腳本。
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

**恭喜安裝完成！** 您現在可以透過命令列使用 WasmEdge ，或在應用程式中使用 WasmEdge 函式庫。若要升級 WasmEdge ，您只需要重新執行上述步驟，舊的版本將會被覆寫。

## 為所有使用者安裝 WasmEdge

WasmEdge 預設將安裝在 `$HOME/.wasmedge` 資料夾。您也可以安裝在系統資料夾中，例如 `/usr/local` ，以便所有使用者都能使用 WasmEdge 。若要在安裝時指定路徑，您可以在執行 `install.sh` 腳本時加上 `-p` 參數。由於安裝將進行在系統資料夾，您需要以 `root` 使用者或是 `sudo` 權限執行以下指令。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -p /usr/local
```

您也可以一起安裝全部擴充：

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all -p /usr/local
```

## 安裝 WasmEdge 指定版本

您可以在執行 `install.sh` 腳本時加上 `-v` 參數來指定安裝的版本，包含預覽發行與較舊的發行版本。範例如下：

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all -v 0.9.1
```

如果您對 `master` 分支的 `HEAD` 最新的編譯版本，也就是 WasmEdge 的 nightly 版本感到興趣，您可以直接從 Github Action 的 CI artifact 下載。[請參考此範例](https://github.com/WasmEdge/WasmEdge/actions/runs/1521549504#artifacts)。

## 安裝內容

WasmEdge 將安裝以下內容。假設您將 WasmEdge 安裝到 `$HOME/.wasmedge` 資料夾內。若您想進行系統層級的安裝，您也可以指定變更安裝路徑到 `/usr/local` 下。

* `$HOME/.wasmedge/bin` 資料夾包含 WasmEdge Runtime CLI 執行檔。您可以複製或移動到您的其他路徑。
  * `wasmedge` 工具是標準的 WasmEdge runtime 。您可以在命令列中執行它： `wasmedge --dir .:. app.wasm`
  * `wasmedgec` 工具是 能將 `wasm` 檔案編譯成原生 `so` 檔案的 AOT 編譯器：`wasmedgec app.wasm app.so` 之後 `wasmedge` 可以執行 `so` 檔案：`wasmedge --dir .:. app.so`
  * `wasmedge-tensorflow` 和 `wasmedge-tensorflow-lite` 工具是支援 WasmEdge Tensorflow SDK 的 WasmEdge runtime 。
* `$HOME/.wasmedge/lib` 資料夾包含 WasmEdge 的共享函式庫和相依函式庫。使用到 WasmEdge SDK 的應用程式將會用到這些函式庫。
* `$HOME/.wasmedge/include` 資料夾包含 WasmEdge 的標頭檔。這些檔案用於 WasmEdge SDK 。

## 解除安裝

執行以下指令可以解除安裝 WasmEdge ：

```bash
bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh)
```

如果 `wasmedge` 執行檔位置未在 `$PATH` 環境變數內，而且 WasmEdge 並非安裝在預設的 `$HOME/.wasmedge` 資料夾下，您必須提供安裝路徑。

```bash
bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh) -p /path/to/parent/folder
```

如果您希望以自動模式解除安裝 WasmEdge ，您可以加上 `--quick` 或 `-q` 選項。

```bash
bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh) -q
```

> 若 `wasmedge` 執行檔的上層資料夾中包含 `.wasmedge` ，該資料夾也會一並移除。舉例來說，解除安裝腳本將會移除預設安裝的 `$HOME/.wasmedge` 資料夾。

## 安裝 Node.js 使用的 WasmEdge

WasmEdge 可以用來執行 [內嵌在 Node.js 下的 WebAssembly 函式](https://www.secondstate.io/articles/getting-started-with-rust-function/) 。在 Node.js 環境下安裝 WasmEdge 模組非常簡單，只需要使用 `npm` 工具。

```bash
npm install -g wasmedge-core # Append --unsafe-perm if permission denied
```

您也可以安裝含有 [Tensorflow 與其他擴充的 WasmEdge](https://www.secondstate.io/articles/wasi-tensorflow/) 。

```bash
npm install -g wasmedge-extensions # Append --unsafe-perm if permission denied
```

[Second State Functions](https://www.secondstate.io/faas/) 是一個在 Node.js 上基於 WasmEdge 的 FaaS 服務。
