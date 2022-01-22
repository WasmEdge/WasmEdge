# 通用二进制

WasmEdge 可以将AOT编译的本地二进制文件包装到原始wasm
文件的自定义部分。我们称其为通用wasm二进制格式。

AOT编译的wasm文件兼容任何wasm运行时。当该wasm文件被WasmEdge
运行时执行，WasmEdge将从自定义部分提取本地二进制文件并执行它。

当然，用户可以选择使用 `wasmedge` AOT编译器去生成本地二进制文件。
WasmEdge将根据输出文件的扩展名去判断生成的文件格式。例如，如果用户
使用 `wasmedge` 输出 `.so` 文件，则WasmEdge将会生成自定义 Linux 共享库格式。
否则，它将默认生成一个统一的wasm二进制文件。
