# WasmEdge 命令行

在[安装 WasmEdge](install.md) 或启动 [WasmEdge appdev Docker 容器](docker.md)后，我们可以通过多种方法运行已经编译好的 WebAssembly 程序。

## wasmedge

`wasmedge` 二进制文件是一个用来运行 WebAssembly 程序应用的命令行工具（CLI）。

- 假如 WebAssembly 程序包含一个 `main()` 函数，`wasmedge` 将在命令模式下将其作为独立程序执行。
- 假如 WebAssembly 程序包含一个或者多个公共函数，`wasmedge` 可以在 反应器模式下单独执行每个函数。

### 命令行选项

`wasmedge` 的选项和标志如下所示：

1. （可选）统计信息：
   - 用 `--enable-time-measuring` 来展示执行时间；
   - 用 `--enable-gas-measuring` 来展示 gas 的消耗量；
   - 用 `--enable-instruction-count` 来展示执行的指令数量；
   - 或者用 `--enable-all-statistics` 来启用所有的统计选项。
2. （可选）资源限制：
   - 用 `--gas-limit` 来限制执行所花费的 gas；
   - 用 `--memory-page-limit` 来设置每一个内存实例的页限制（大小为 64 KiB）。
3. (可选) 反应器模式：使用 `--reactor` 来启用反应器模式；在该模式下，`wasmedge` 将运行 WebAssembly 程序中指定的函数：
   - WasmEdge 将执行第一个参数（`ARG[0]`）所指定的函数名；
   - 假如存在名为 `_initialize` 的导出函数，这个函数将最先以空参的方式执行。
4. （可选）将目录绑定到 WASI 虚拟文件系统：
   - 每一个目录都以 `--dir guest_path:host_path` 的方式指定。
5. （可选）环境变量：
   - 每一个变量都以 `--env NAME=VALUE` 的方式指定。
6. Wasm 文件（`/path/to/wasm/file`）；
7. （可选）参数：
   - 在反应器模式下，第一个参数将作为函数的名称，`ARG[0]` 后的其余参数将作为 `ARG[0]` 函数的参数；
   - 在命令模式下，命令行参数将作为 `_start` 函数的参数；它们也被称为独立程序的命令行参数。

安装完毕后，你可以[查看并运行我们的示例](../index.md)。

## wasmedgec

`wasmedgec` 是一个将 WebAssembly 字节码编译成原生机器码的程序（即 AOT 编译器）。
编译好的机器码可以被[储存在原本的 `wasm` 文件中](universal.md)，`wasmedge` 命令行将自动地选择可用的原生机器码。

`wasmedgec` 的选项和标志如下所示：

1. Wasm 输入文件（`/path/to/input/wasm/file`）；
2. 输出文件的名称（`/path/to/output/file`）：
   - 默认情况下，它将生成[通用 wasm 二进制格式](universal.md)；
   - 用户也可以通过指定 `.so`、`.dylib` 或者 `.dll` 扩展名来生成原生二进制文件。

```bash
# 这很慢
wasmedge app.wasm

# AOT 编译
wasmedgec app.wasm app.wasm

# 现在就快多了
wasmedge app.wasm
```

在 Linux 系统上，它将会生成一个 `so` 共享库文件，然后由 `wasmedge` 运行时执行。

```bash
wasmedgec app.wasm app.so
wasmedge app.so
```
