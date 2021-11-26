# FAQ abut WebAssembly and WasmEdge

**Q: Will WebAssembly replace with Docker?**

A: We have made a comprehensive comparison page between [WebAssembly and Docker](https://wasmedge.org/wasm_docker/). The Conclusion is that WebAssembly is the future of cloud computing and WebAssembly will run side by side wth Docker now. Details see here: https://wasmedge.org/wasm_docker/

> if you want to add something on this page, go to https://github.com/WasmEdge/www and create an issue or PR.

**Q: What's the differnece among NaCI, apllication runtimes, and WebAssembly?**

A: They all have pros and cons. Hope this table will help you.

|	|NaCl	|Application runtimes (eg Node & Python)	|Docker-like container	|WebAssembly	|
|---	|---	|---	|---	|---	|
|Performance	|Great	|Poor	|OK	|Great	|
|Resource footprint	|Great	|Poor	|Poor	|Great	|
|Isolation	|Poor	|OK	|OK	|Great	|
|Safety	|Poor	|OK	|OK	|Great	|
|Portability	|Poor	|Great	|OK	|Great	|
|Security	|Poor	|OK	|OK	|Great	|
|Language and framework choice	|N/A	|N/A	|Great	|OK	|
|Ease of use	|OK	|Great	|Great	|OK	|
|Manageability	|Poor	|Poor	|Great	|Great	|

**Q: What's the difference between WebAssembly and eBPF**

A: In general, eBPF is suitable for network or security related tasks, while WebAssembly is well suited for business applications. Details see here: https://medium.com/codex/ebpf-and-webassembly-whose-vm-reigns-supreme-c2861ce08f89


