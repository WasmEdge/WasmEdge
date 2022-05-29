# WasmEdge 命令列工具

在 [安裝 WasmEdge](install.md) 後或啟動 [WasmEdge 應用程式開發 Docker 容器](docker.md) ，有幾種方式可以執行已經編譯好的 WebAssembly 應用程式。

## wasmedge

`wasmedge` 執行檔是一個用來執行 WebAssembly 應用程式的命令行工具（ CLI ）。

- 若 WebAssembly 檔案包含一個 `main()` 函式， `wasmedge` 將在 Command 模式以獨立應用程式的方式執行它。
- 若 WebAssembly 檔案是由一個或多個函式組成的公用函式庫， `wasmedge` 將可以在 Reactor 模式呼叫指定的個別函式。

### 命令列選項

`wasmedge` 命令列工具的選項如下。

1. （選用）統計資訊：
   - 使用 `--enable-time-measuring` 顯示執行時間。
   - 使用 `--enable-gas-measuring` 顯示使用的 gas 數量。
   - 使用 `--enable-instruction-count` 顯示執行的 WebAssembly 指令集數量。
   - 或者使用 `--enable-all-statistics` 啟用所有統計資訊選項。
2. （選用）限制使用資源：
   - 使用 `--gas-limit` 限制執行花費的 gas 。
   - 使用 `--memory-page-limit` 設定每一個 Memory instance 的 pages 限制（每一個 page 為 64 KiB ）。
3. （選用） Reactor 模式：使用 `--reactor` 啟用 reactor 模式。在 reactor 模式下， `wasmedge` 可以單獨執行 WebAssembly 檔案中的特定函式。
   - WasmEdge 將執行命令列參數中的第一個參數 `ARG[0]` 為名的函式。
   - 若 WebAssembly 檔案內存在名為 `_initialize` 的函式，這個函式將不帶任何參數最先被執行。
4. （選用）繫結資料夾進 WASI 虛擬檔案系統。
   - 每個路徑繫結可以被指定為 `--dir guest_path:host_path` 。
5. （選用）環境變數。
   - 每個環境變數可以被指定為 `--env NAME=VALUE` 。
6. Wasm 檔案 (`/path/to/wasm/file`) 。
7. （選用）參數。
   - 在 Reactor 模式下，第一個參數 `ARG[0]` 作為指定執行的函數名稱，後續的參數將作為該函式的參數傳入。
   - 在 Command 模式下，這些參數也將成為 `_start` 函式的參數，就如同一般命令列應用程式的命令列參數。

WasmEdge 安裝完畢後您可以 [參考並執行我們的範例](../index.md)。

## wasmedgec

`wasmedgec` 執行檔可以將 WebAssembly 檔案編譯成原生機械碼（即 AOT 編譯器）。
編譯好的機械碼也可以被[存放在原本的 `wasm` 檔案中](universal.md)， `wasmedge` CLI 在執行的時候若偵測到 `wasm` 檔案含有原生機械碼，將自動選擇它來執行。

`wasmedgec` 命令列工具的選項如下。

1. 輸入的 Wasm 檔案路徑（ `/path/to/input/wasm/file` ）。
2. 輸出的檔案名稱（ `/path/to/output/file` ）。
   - 預設下將產生 [通用 Wasm binary 格式](universal.md)。
   - 使用者也可以透過指定 `.so` ， `.dylib` 或者 `.dll` 這些副檔名來產生原生執行檔格式。

```bash
# This is slow
wasmedge app.wasm

# AOT compile
wasmedgec app.wasm app.wasm

# This is now MUCH faster
wasmedge app.wasm
```

在 Linux 作業系統，它也可以生成一個 `so` 共享函式庫格式檔案並被 `wasmedge` CLI 執行。

```bash
wasmedgec app.wasm app.so
wasmedge app.so
```
