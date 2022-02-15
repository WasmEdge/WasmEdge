# Python

已经有多个语言支持 Python 运行时，其中一些还支持 WebAssembly。本文档将描述怎样在 WasmEdge 上运行 [RustPython](https://github.com/RustPython/RustPython) 来执行 Python 程序。

## 编译 RustPython

为了编译 RustPython, 你需要在你的机器上安装 Rust 工具链。并且启用 `wasm32-wasi` 平台支持。

```
$ rustup target add wasm32-wasi
```

然后你可以使用下面的命令去下载并编译 RustPython。

```
$ git clone https://github.com/RustPython/RustPython.git
$ cd RustPython
$ cargo build --release --target wasm32-wasi --features="freeze-stdlib"
```

启用 `freeze-stdlib` 功能可以在二进制文件中使用 Python 标准库。输出文件路径为 `target/wasm32-wasi/release/rustpython.wasm`。

## AOT 编译

为了更好的性能，WasmEdge 支持编译 WebAssembly 字节码程序编译为本地代码。强烈建议在运行之前编译 RustPython 到本地代码。


```
$ wasmedgec ./target/wasm32-wasi/release/rustpython.wasm ./target/wasm32-wasi/release/rustpython.wasm
```

WasmEdge 从 0.9.0 版本开始，引入了 [通用 Wasm 二进制格式](https://wasmedge.org/book/en/start/universal.html) 。因此你可以使用 `.wasm` 扩展来生成跨运行时兼容的格式，或者使用 `.so` 生成 Linux 共享库格式。

## 运行

```
$ wasmedge ./target/wasm32-wasi/release/rustpython.wasm
```

之后你就可以在 WebAssembly 中使用 Python shell。

## 授予文件系统权限

你可以预先打开目录，让 WASI 程序有权限在实机读写文件。接下来的命令可以将当前工作目录挂载到 WASI 虚拟文件系统上。

```
$ wasmedge --dir .:. ./target/wasm32-wasi/release/rustpython.wasm
```