# wasm_bpf Plugin tests

This file contains BPF programs that will be used during testing.

- `bootstrap` and `runqlat`: examples copied from `wasm-bpf`. See [here](https://github.com/eunomia-bpf/wasm-bpf/tree/main/examples) for build instructions.

- `simple_ringbuf`: A simple eBPF program that writes fixed data to a ring buffer
- `simple_map`: A simple eBPF program that stores fixed data to a BPF map

The sources of `simple_ringbuf` and `simple_map` are listed under
`bpf-sources`. Run `make` under that directory to build them.

`libbpf` and `clang` are required to build them.
