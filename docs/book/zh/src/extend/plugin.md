# WasmEdge 插件 API

WasmEdge 提供了一套基于 C++ 的 API 用来注册自定义扩展和主机函数。虽然 WasmEdge 各种语言语言SDK允许将主机应用注册为主机功能函数，但插件 API 方式允许这种扩展被纳入WasmEdge自己的构建和发布过程。

事实上，WasmEdge 对 Tensorflow 、图像处理、键值存储等的扩展都是通过插件 API 实现的。插件API是你可以为 WasmEdge Runtime 本身贡献新功能的方式。