# 快速开始

我们为你创建了基于 Ubuntu 的脚本，以便你在标准 Kubernetes 设置中快速开始使用以下运行时组合。

| CRI（高级）runtime | OCI（低级）runtime | |
| --- | --- | --- |
| CRI-O | crun + WasmEdge | [脚本](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio.yml) |
| containerd | crun + WasmEdge | [脚本](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-containerd.yml) |

## CRI-O 和 crun

在 Ubuntu 20.04 上你可以使用 CRI-O [install.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/install.sh) 脚本来安装 CRI-O 和 `crun` 。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/install.sh | bash
```

接下来，使用 [以下脚本](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/install.sh) 安装 Kubernetes。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_crio/install.sh | bash
```

[simple_wasi_application.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/simple_wasi_application.sh) 脚本展示了如何从 Docker Hub 拉取 [WebAssembly 应用程序](demo/wasi.md) ，然后在 Kubernetes 中将其作为容器化应用程序运行。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_crio/simple_wasi_application.sh | bash
```

你应该会在控制台日志中看到 WebAssembly 程序打印的结果。[这里是一个例子](https://github.com/second-state/wasmedge-containers-examples/runs/4186005677?check_suite_focus=true#step:6:3007)。

## containerd 和 crun

在 Ubuntu 20.04 上你可以使用 containerd [install.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/install.sh) 脚本来安装 `containerd` 和 `crun`。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/install.sh | bash
```

接下来，使用 [以下脚本](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/install.sh) 安装 Kubernetes。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_containerd/install.sh | bash
```

[simple_wasi_application.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/simple_wasi_application.sh) 脚本展示了如何从 Docker Hub拉取 [WebAssembly 应用程序](demo/wasi.md)，然后在 Kubernetes 中将其作为容器化应用程序运行。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes_containerd/simple_wasi_application.sh | bash
```

你应该会在控制台日志中看到 WebAssembly 程序打印的结果。[这里是一个例子](https://github.com/second-state/wasmedge-containers-examples/runs/4577789181?check_suite_focus=true#step:6:3010)。

继续阅读本章的其余部分，了解这些运行时的具体配置方式。
