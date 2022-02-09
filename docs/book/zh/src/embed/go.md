# Go SDK

WasmEdge Go API 的使用指南。 你可以通过 WasmEdge Go API 将 WasmEdge 嵌入到 Go 应用程序中。

## 开始

WasmEdge-go 需要 golang 版本 >= 1.15。 请在安装前检查 golang 版本。 你可以[在这里下载 golang](https://golang.org/dl/)

```bash
$ go version
go version go1.16.5 linux/amd64
```

同时，请确保安装具有相同 `WasmEdge-go` 发行版本的 [WasmEdge 共享库](../start/install.md)。

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0
```

如果你需要 `WasmEdge-go` 的 `TensorFlow` 或 `Image` 扩展，请安装带有扩展的 `WasmEdge`：

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0 -e tensorflow,image
```

请注意，`TensorFlow` 和 `Image` 扩展仅适用于 `Linux` 平台。

安装 `WasmEdge-go` 包并在 Go 项目目录中构建：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.0
go build
```

## WasmEdge-go 扩展

默认情况下，“WasmEdge-go”只开启基本运行时。

`WasmEdge-go` 有以下扩展：

- TensorFlow
   - 此扩展支持 [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow) 中的主机功能。
   - 要安装 `tensorflow` 扩展，请在 WasmEdge 安装程序命令中使用 `-e tensorflow` 标志。
   - 为了使用这个扩展，构建时需要标签`tensorflow`：

      ```bash
      go build -tags tensorflow
      ```

- Image
    - 此扩展支持 [WasmEdge-image](https://github.com/second-state/WasmEdge-image) 中的主机功能。
    - 要安装 `image` 扩展，请在 WasmEdge 安装程序命令中使用 `-e image` 标志。
    - 要使用此扩展程序，构建时需要标记`image`:

      ```bash
      go build -tags image
      ```

你也可以在构建时打开多个扩展：

```bash
go build -tags image,tensorflow
```

案例，请参考【示例仓库】(https://github.com/second-state/WasmEdge-go-examples/)。

### Go 中 WasmEdge AOT 编译器

[go_WasmAOT 示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) 演示如何在 Go 应用程序中将 WASM 文件编译为本机二进制文件（AOT 编译）

## 案例

- [嵌入独立的 Wasm 应用程序](go/app.md)
- [嵌入 Wasm 函数](go/function.md)
- [将复杂参数传递给 Wasm 函数](go/memory.md)
- [嵌入 Tensorflow 推理函数](go/tensorflow.md)
- [嵌入绑定函数](go/bindgen.md)
