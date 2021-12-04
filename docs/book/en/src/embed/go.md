# WasmEdge Go SDK

The followings are the guides to working with the WasmEdge Go API. You can embed the WasmEdge into your go application through the WasmEdge Go API.

## Getting Started

The WasmEdge-go requires golang version >= 1.15. Please check your golang version before installation. Developers can [download golang here](https://golang.org/dl/).

```bash
$ go version
go version go1.16.5 linux/amd64
```

Developers must [install the WasmEdge shared library](https://github.com/WasmEdge/WasmEdge/blob/master/docs/install.md) with the same `WasmEdge-go` release version.

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0-rc.4
```

For more details, please refer to the [Installation Guide](../start/install.md) for the WasmEdge installation.

For the developers need the `TensorFlow` or `Image` extension for `WasmEdge-go`, please install the `WasmEdge` with extensions:

```bash
wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -v 0.9.0-rc.4
```

Noticed that the `TensorFlow` and `Image` extensions are only for the `Linux` platforms.

Install the `WasmEdge-go` package and build in your Go project directory:

```bash
$ go get github.com/second-state/WasmEdge-go/wasmedge@v0.9.0-rc3
$ go build
```

## WasmEdge-go Extensions

By default, the `WasmEdge-go` only turns on the basic runtime.

`WasmEdge-go` has the following extensions:

 - Tensorflow
    * This extension supports the host functions in [WasmEdge-tensorflow](https://github.com/second-state/WasmEdge-tensorflow).
    * The `TensorFlow` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e tensorflow` command.
    * For using this extension, the tag `tensorflow` when building is required:
        ```bash
        $ go build -tags tensorflow
        ```
 - Image
    * This extension supports the host functions in [WasmEdge-image](https://github.com/second-state/WasmEdge-image).
    * The `Image` extension when installing `WasmEdge` is required. Please install `WasmEdge` with the `-e image` command.
    * For using this extension, the tag `image` when building is required:
        ```bash
        $ go build -tags image
        ```

Users can also turn on the multiple extensions when building:

```bash
$ go build -tags image,tensorflow
```

For examples, please refer to the [example repository](https://github.com/second-state/WasmEdge-go-examples/).

### WasmEdge AOT Compiler in Go

The [go_WasmAOT example](https://github.com/second-state/WasmEdge-go-examples/tree/master/go_WasmAOT) provide a tool for compiling a WASM file into compiled-WASM for AOT mode.

## Go-API Examples

* [Embed a standalone WASM app](go/app.md)
* [Pass complex parameters to Wasm functions](go/memory.md)
* [Embed a bindgen function](go/bindgen.md)

