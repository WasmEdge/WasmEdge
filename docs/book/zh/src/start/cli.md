# WasmEdge 命令行

在 [安装 WasmEdge](install.md) 之后或启动 [WasmEdge 应用开发容器](docker.md) 时, 有几种方式可以运行已经编译好的 WebAssembly 程序.

## wasmedge

`wasmedge` 是一个用来运行 WebAssembly 程序应用的命令行工具 (CLI).

- 假如 WebAssembly 程序包含一个 `main()` 方法, `wasmedge` 将在命令行以一种独立程序的方式来执行它.
- 假如 WebAssembly 程序包含一个或者多个公开的(`public`)方法, `wasmedge` 将把它们当做独立的方法, 在 Reactor 模式下执行.

### 命令行选项

`wasmedge` 的选项和标志位如下所示:

1. (可选) 统计信息:
   - 用 `--enable-time-measuring` 来展示执行时间.
   - 用 `--enable-gas-measuring` 来展示使用 gas 的数量.
   - 用 `--enable-instruction-count` 来展示执行的指令数量.
   - 或者用 `--enable-all-statistics` 来启用所有的统计选项.
2. (可选) 资源限制:
   - 用 `--gas-limit` 来限制执行花费的 gas.
   - 用 `--memory-page-limit` 来设置每一个内存实例的 pages 限制 (as size of 64 KiB).
3. (可选) Reactor 模式: 用 `--reactor` 来启用 reactor 模式. 在 reactor 模式下, `wasmedge` 运行 WebAssembly 程序中一个特定的方法如下所示:
   - WasmEdge 将从命令行中，提取第一个参数 `ARG[0]` 的值作为函数名称执行.
   - 假如存在名为 `_initialize` 的导出方法, 这个方法将最先执行, 执行的参数为空.
4. (可选) 绑定目录给 WASI 虚拟文件系统.
   - 每一个目录可以被指定为 `--dir guest_path:host_path`.
5. (可选) 环境变量.
   - 每一个变量可以被指定为 `--env NAME=VALUE`.
6. Wasm 文件 (`/path/to/wasm/file`).
7. (可选) 参数.
   - 在 reactor 模式下, 第一个参数将作为函数的名称, 在 `ARG[0]` 后的其余参数, 将作为参数传入 `ARG[0]` 指定的 wasm 方法.
   - 在命令行模式下, 命令行参数也将成为函数 `_start` 的形参. 它们也被称为独立程序的命令行参数.

一旦安装完毕, 你可以 [查看并运行我们的示例](../index.md).

## wasmedgec

`wasmedgec` 是一个把 WebAssembly 字节码程序编译成原生机器码的可执行文件 (即 the AOT compiler).
编译好的机器码会被 [储存在原本的 `wasm` 文件中](universal.md), 
`wasmedge` CLI 一旦发现原生机器码可用，将自动的选择它来执行.

`wasmedgec` 的选项和标志位如下所示:

1. 输入的Wasm文件路径(`/path/to/input/wasm/file`).
2. 输出的文件名称(`/path/to/output/file`).
   - 默认情况, 它将生成 [通用的 Wasm 二进制的格式](universal.md).
   - 用户也可以，通过指定 `.so`, `.dylib`, 或者 `.dll` 这些扩展名，来生成原生二进制文件.

```bash
# This is slow
$ wasmedge app.wasm

# AOT compile
$ wasmedgec app.wasm app.wasm

# This is now MUCH faster
$ wasmedge app.wasm
```

在 Linux 系统, 它会生成一个 `so` 共享库文件来被 `wasmedge` CLI 执行.

```bash
$ wasmedgec app.wasm app.so
$ wasmedge app.so
```
