# WasmEdge plug-in API

WasmEdge provides a C++ based API for registering extension modules and host functions. While the WasmEdge language SDKs allow registering host functions from a host (wrapping) application, the plugin API allows such extensions to be incorporated into WasmEdge's own building and releasing process.

In fact, the WasmEdge extensions for Tensorflow, image processing, key-value storage etc are all implemented using the plugin API. The plugin API is how you could contribute new functions to the WasmEdge Runtime itself.
