# 通用 wasm 二进制格式

WasmEdge 可以将 AOT 编译的原生二进制包装到原始 wasm 文件中的自定义部分。我们将其称之为通用 wasm 二进制格式。

AOT 编译的 wasm 文件与任何 wasm 运行时兼容。然而，当 WasmEdge 运行时执行此 wasm 文件时，WasmEdge 将从自定义部分中提取原生二进制并执行它。

当然，用户仍然可以选择使用 `wasmedgec` AOT 编译器生成原生二进制文件。
WasmEdge 将以输出文件扩展名来确定生成的文件格式。举例来说，如果你将 `wasmedgec` 的输出文件扩展名设置为 `.so`，它将生成 Linux 共享库格式的原生二进制文件；否则，它将默认生成一个通用的 wasm 二进制文件。
