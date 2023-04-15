This file contains bpf programs that will be used during testing.


- `bootstrap` and `runqlat`: examples copied from `wasm-bpf`. See [here](https://github.com/eunomia-bpf/wasm-bpf/tree/main/examples) for build instructions.

- `simple_ringbuf`: A simple ebpf program which writes fixed data to a ring buffer
- `simple_map`: A simple ebpf program which stores fixed data to a bpf map

The source of `simple_ringbuf` and `simple_map` are listed under `bpf-sources`. Run `make` under that directory to build them.

`libbpf` and `clang` are required to build them.
