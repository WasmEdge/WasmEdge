# Python

Python 运行时已经有几种不同的语言实现，其中一些支持 WebAssembly。本文档将描述如何在 WasmEdge 上运行 [RustPython](https://github.com/RustPython/RustPython) 来执行 Python 程序。

## 编译 RustPython

要编译 RustPython，你应该在你的机器上安装 Rust 工具链。并且应该启用 `wasm32-wasi` 平台支持。

```bash
rustup target add wasm32-wasi
```

然后你可以使用下面的命令来克隆和编译 RustPython：

```bash
git clone https://github.com/RustPython/RustPython.git
cd RustPython
cargo build --release --target wasm32-wasi --features="freeze-stdlib"
```

为了能够在二进制文件中包含 Python 标准库，需要启用 `freeze-stdlib` 特性。输出文件可以在 `target/wasm32-wasi/release/rustpython.wasm` 找到。

## AOT 编译

WasmEdge 支持将 WebAssembly 字节码程序编译为机器码以获得更好的性能。强烈建议在运行前将 RustPython 编译为机器码。

```bash
wasmedgec ./target/wasm32-wasi/release/rustpython.wasm ./target/wasm32-wasi/release/rustpython.wasm
```

从 0.9.0 开始，WasmEdge引入了[通用 Wasm 二进制格式](https://wasmedge.org/book/en/start/universal.html)。所以你可以使用 `.wasm` 扩展来生成跨运行时兼容的格式，或者使用 `.so` 来生成 Linux 共享库格式。

## 运行

```bash
wasmedge ./target/wasm32-wasi/release/rustpython.wasm
```

然后你可以在 WebAssembly 中得到一个 Python shell 交互解释器！

## 授予文件系统访问权限

可以预先打开目录，让 WASI 程序有权限读写真机上存储的文件。以下命令将当前工作目录挂载到 WASI 虚拟文件系统。

```bash
wasmedge --dir .:. ./target/wasm32-wasi/release/rustpython.wasm
```
