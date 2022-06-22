# Embed WasmEdge functions

A common use case for WasmEdge is to embed it in your own applications (called a host application). It allows you to support 3rd party plug-ins and extensions for your applications. Those plug-ins and extensions could be written in any of the languages WasmEdge supports, and by anyone as they are safely and securely executed in the WasmEdge sandbox.

In this chapter, we will discuss how to use WasmEdge SDKs to embed WasmEdge programs into C, Rust, Go, and Python host applications.
