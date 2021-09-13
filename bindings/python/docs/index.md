# Welcome to Py SDK for WasmEdge

!!! note "Work in progress"

## Motivation

Python is a well-known programming language and supports quick development for a variety of domains. 
WasmEdge has a `go` language SDK implemented [here](https://github.com/second-state/WasmEdge-go).
This proposal aims to implement a Python ( for the CPython implementation of Python ) SDK using the above implementation as a reference,

## Details

* Some rationale around performance- 
Python has never been intended for performance-critical applications while on the other hand, WasmEdge has different intentions. It is designed to be a lightweight and fast VM for Wasm. The idea behind python is that developers spend more time reading the code. Python SDK should be way easier to use. And as far as performance is considered, all of the code behind this package can be wrapped around the C-API to make it as closer to the native speed as possible.

* This SDK should be able to provide all the functions present in the C - API of WasmEdge (link given in the appendix).
* Stuff to look out for:
  * Data-types integrations for all the three python-c-wasm.
  * Safety (no memory access of host from `.wasm` modules) 
  * Potentially it is also possible to wrap around `WasmEdge` functionalities to support [Wasm C API](https://github.com/WebAssembly/wasm-c-api) if it doesn't already.

* The end goal should be to use WasmEdge as a backend and provide an Easy to use SDK over this.

## Appendix

* C - API for WasmEdge is already present. - [Docs](https://github.com/WasmEdge/WasmEdge/blob/master/docs/c_api.md#wasmedge-c-api-documentation)
* C - API for the CPython implementation of python is quite mature [without third-party tools](https://docs.python.org/3/extending/index.html#creating-extensions-without-third-party-tools) and `Boost.Python` is also available [Examples](https://github.com/TNG/boost-python-examples)

## Misc:

* I'll probably need help in this to make it as correct as possible if anything seems a bit off.
* Thanks a lot already :-D

## Project layout
```tree
.
├── Containerfile
├── coverage.xml
├── docs
│   └── index.md
├── Makefile
├── MANIFEST.in
├── mkdocks.yml
├── pywasmedge
│   ├── app.py
│   ├── include
│   │   └── WasmEdge.h
│   ├── __init__.py
│   ├── __main__.py
│   └── src
│       └── WasmEdge.c
├── README.md
├── requirements-test.txt
├── requirements.txt
├── setup.py
├── tests
│   ├── conftest.py
│   ├── __init__.py
│   └── test_base.py
├── VERSION
```
