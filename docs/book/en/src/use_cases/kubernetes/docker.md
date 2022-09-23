# Running WasmEdge apps with the Docker CLI

The Docker CLI is a very popular developer tool. However, it is not easy to replace Docker's underlying OCI runtime (`runc`) with the WasmEdge-enabled `crun`. In this section, we will discuss two ways to run WasmEdge applications in Docker.

* [Wrap WasmEdge in a slim Linux container](docker/lxc.md)
* [Use containerd shim](docker/containerd.md)
