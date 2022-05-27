# WasmEdge Go SDK

这是一份如何使用 WasmEdge Go API 的指南。你可以通过 WasmEdge Go API 将 WasmEdge 嵌入在你的 Go 应用程序中。

## 快速开始

WasmEdge-go 要求 Go 语言版本 >= 1.16。在安装之前请检查你的 Go 语言版本。你可以[从这里下载 Go 语言](https://golang.org/dl/)。

```bash
$ go version
go version go1.16.5 linux/amd64
```

同时，请确保你已经安装了和 `WasmEdge-go` 同一发布版本的 [WasmEdge 共享库](../start/install.md)。

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.10.0
```

如果你需要 `WasmEdge-go` 的 `TensorFlow` 或 `Image` 扩展，请安装带有这些扩展的 `WasmEdge` ：

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.10.0 -e tensorflow,image
```

注意 `TensorFlow` 和 `Image` 扩展仅支持 `Linux` 平台。

安装 `WasmEdge-go` 包并在你的 Go 项目目录下构建：

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.10.0
go build
```

## WasmEdge-go 扩展

默认情况下， `WasmEdge-go` 只打开基本运行时。

`WasmEdge-go` 有以下扩展：

- TensorFlow
  - 该扩展支持 [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow) 中的 host 函数。
  - 若要安装 `tensorflow` 扩展。请在 WasmEdge 安装器命令中使用 `-e tensorflow` 标志。
  - 若使用该扩展，在构建时需要打上 `tensorflow` 标签：

      ```bash
      go build -tags tensorflow
      ```

- Image
  - 该扩展支持 [WasmEdge-image](https://github.com/second-state/WasmEdge-image) 中的 host 函数。
  - 若要安装 `image` 扩展。请在 WasmEdge 安装器命令中使用 `-e image` 标志。
  - 若使用该扩展，在构建时需要打上 `image` 标签：

      ```bash
      go build -tags image
      ```

你也可以在构建的时候打开多个扩展：

```bash
go build -tags image,tensorflow
```

有关示例，请参考 [示例仓库](https://github.com/second-state/WasmEdge-go-examples/)。

### Go 中的 WasmEdge AOT 编译器

[go_WasmAOT 示例](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) 演示了如何通过一个 Go 应用程序将一个 WASM 文件编译成本机二进制文件（AOT 编译）。

## 相关示例

- [嵌入一个独立的 Wasm 应用](go/app.md)
- [嵌入一个 Wasm 函数](go/function.md)
- [将复杂的参数传递到 Wasm 函数](go/memory.md)
- [嵌入一个 Tensorflow 推理函数](go/tensorflow.md)
- [嵌入一个 bindgen 函数](go/bindgen.md)
