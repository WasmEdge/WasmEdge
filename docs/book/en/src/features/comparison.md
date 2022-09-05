# Comparison

## What's the relationship between WebAssembly and Docker?

Check out our infographic [WebAssembly vs. Docker](https://wasmedge.org/wasm_docker/). WebAssembly runs side by side with Docker in cloud native and edge native applications.

## What's the difference for Native clients (NaCl), Application runtimes, and WebAssembly?

We created a handy table for the comparison.

|                               | NaCl  | Application runtimes (eg Node &amp; Python)   | Docker-like container | WebAssembly   |
| ---                           | ---   | ---                                           | ---                   | ---           |
| Performance                   | Great | Poor                                          | OK                    | Great         |
| Resource footprint            | Great | Poor                                          | Poor                  | Great         |
| Isolation                     | Poor  | OK                                            | OK                    | Great         |
| Safety                        | Poor  | OK                                            | OK                    | Great         |
| Portability                   | Poor  | Great                                         | OK                    | Great         |
| Security                      | Poor  | OK                                            | OK                    | Great         |
| Language and framework choice | N/A   | N/A                                           | Great                 | OK            |
| Ease of use                   | OK    | Great                                         | Great                 | OK            |
| Manageability                 | Poor  | Poor                                          | Great                 | Great         |

## What's the difference between WebAssembly and eBPF?

`eBPF` is the bytecode format for a Linux kernel space VM that is suitable for network or security related tasks. WebAssembly is the bytecode format for a user space VM that is suited for business applications. [See details here](https://medium.com/codex/ebpf-and-webassembly-whose-vm-reigns-supreme-c2861ce08f89).
