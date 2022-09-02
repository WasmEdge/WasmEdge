# Quick start

We have created Ubuntu-based scripts for you to quickly get started with the following combination of runtimes in a standard Kubernetes setup.

| CRI (high level) runtime | OCI (low level) runtime |                                                                                                                              |
| ---                      | ---                     | ---                                                                                                                          |
| CRI-O                    | crun + WasmEdge         | [Script](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio.yml)       |
| containerd               | crun + WasmEdge         | [Script](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-containerd.yml) |

## CRI-O and crun

You can use the CRI-O [install.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/install.sh) script to install CRI-O and `crun` on Ubuntu 20.04.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/install.sh | bash
```

Next, install Kubernetes using the [following script](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/install.sh).

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_crio/install.sh | bash
```

The [simple_wasi_application.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/simple_wasi_application.sh) script shows how to pull [a WebAssembly application](demo/wasi.md) from Docker Hub, and then run it as a containerized application in Kubernetes.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_crio/simple_wasi_application.sh | bash
```

You should see results from the WebAssembly program printed in the console log. [Here is an example](https://github.com/second-state/wasmedge-containers-examples/runs/4186005677?check_suite_focus=true#step:6:3007).

## containerd and crun

You can use the containerd [install.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/install.sh) script to install `containerd` and `crun` on Ubuntu 20.04.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/install.sh | bash
```

Next, install Kubernetes using the [following script](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/install.sh).

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_containerd/install.sh | bash
```

The [simple_wasi_application.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/simple_wasi_application.sh) script shows how to pull [a WebAssembly application](demo/wasi.md) from Docker Hub, and then run it as a containerized application in Kubernetes.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_containerd/simple_wasi_application.sh | bash
```

You should see results from the WebAssembly program printed in the console log. [Here is an example](https://github.com/second-state/wasmedge-containers-examples/runs/4577789181?check_suite_focus=true#step:6:3010).

Read on to the rest of this chapter to learn how exactly those runtimes are configured.
