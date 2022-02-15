# 通用 wasm 二进制格式

WasmEdge 可以将 AOT 编译的本地二进制文件包装到本地 wasm 文件中的自定义部分。我们将其称之为通用 wasm 二进制格式。

AOT 编译的 wasm 文件兼容任何 wasm 运行时。当该 wasm 文件被 WasmEdge 运行时执行时，WasmEdge 将从自定义部分中提取本地二进制文件并执行它。

当然，用户可以选择使用 `wasmedgec` AOT 编译器生成本地二进制文件。WasmEdge 将根据输出文件的扩展名来判断生成的文件格式。例如，如果用户使用 `wasmedgec` 输出 `.so` 文件，则 WasmEdge 将生成自定义 Linux 共享库格式的本地库。否则，它将默认生成一个通用的 wasm 二进制文件。
