# GGML and llama.cpp

[GGML][] and [llama.cpp][] are open-source projects in the machine learning domain. GGML is a tensor library for machine learning, developed in C. On the other hand, llama.cpp serves as a LLaMA model inference engine and is implemented in C/C++.

This directory contains the source code from both llama.cpp and GGML. The code in this directory is licensed under the MIT License. For more details, please refer to the [LICENSE](./LICENSE) file.

WasmEdge includes support for GGML and llama.cpp through its WASI-NN plugin, enabling the execution of machine learning models in WebAssembly. Within the WasmEdge WASI-NN plugin, we have added functionality for GGML model loading and LLaMA model inference.

[GGML]: http://ggml.ai
[llama.cpp]: https://github.com/ggerganov/ggml
