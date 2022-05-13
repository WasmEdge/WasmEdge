# WasmEdge Go SDK

The followings are the guides to working with the WasmEdge Go API. You can embed the WasmEdge into your go application through the WasmEdge Go API.

## Getting Started

The WasmEdge-go requires golang version >= 1.15. Please check your golang version before installation. You can [download golang here](https://golang.org/dl/).

```bash
$ go version
go version go1.16.5 linux/amd64
```

Meantime, please make sure you have installed [the WasmEdge shared library](../start/install.md) with the same `WasmEdge-go` release version.

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.1
```

If you need the `TensorFlow` or `Image` extension for `WasmEdge-go`, please install the `WasmEdge` with extensions:

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.1 -e tensorflow,image
```

Noticed that the `TensorFlow` and `Image` extensions are only for the `Linux` platforms.

Install the `WasmEdge-go` package and build in your Go project directory:

```bash
go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.1
go build
```

## WasmEdge-go Extensions

By default, the `WasmEdge-go` only turns on the basic runtime.

`WasmEdge-go` has the following extensions:

- TensorFlow
  - This extension supports the host functions in [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow).
  - To install the `tensorflow` extension, please use the `-e tensorflow` flag in the WasmEdge installer command.
  - For using this extension, the tag `tensorflow` when building is required:

      ```bash
      go build -tags tensorflow
      ```

- Image
  - This extension supports the host functions in [WasmEdge-image](https://github.com/second-state/WasmEdge-image).
  - To install the `image` extension, please use the `-e image` flag in the WasmEdge installer command.
  - For using this extension, the tag `image` when building is required:

      ```bash
      go build -tags image
      ```

You can also turn on the multiple extensions when building:

```bash
go build -tags image,tensorflow
```

For examples, please refer to the [example repository](https://github.com/second-state/WasmEdge-go-examples/).

### WasmEdge AOT Compiler in Go

The [go_WasmAOT example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) demonstrates how to compile a WASM file into a native binary (AOT compile) from within a Go application.

## Examples

- [Embed a standalone Wasm app](go/app.md)
- [Embed a Wasm function](go/function.md)
- [Pass complex parameters to Wasm functions](go/memory.md)
- [Embed a Tensorflow inference function](go/tensorflow.md)
- [Embed a bindgen function](go/bindgen.md)
