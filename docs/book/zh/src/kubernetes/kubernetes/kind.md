# KinD

KinD 是在 Docker 内部运行的 Kubernetes 发行版，非常适合本地开发或集成测试。

## 快速开始

作为先决条件，我们第一步需要安装 KinD。为此，请参阅[快速入门指南](https://kind.sigs.k8s.io/docs/user/quick-start/#installing-from-release-binaries)和[稳定版发布页面](https://kind.sigs.k8s.io/docs/user/quick-start/#installing-from-release-binaries)来安装最新版本的 KinD CLI。

如果已经安装了 KinD，我们可以直接从[这里](https://github.com/Liquid-Reply/kind-crun-wasm)中的示例开始：

```bash
# Create a "WASM in KinD" Cluster
kind create cluster --image ghcr.io/liquid-reply/kind-crun-wasm:v1.23.0
# Run the example
kubectl run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```

在这个章节的剩余部分，我们会介绍如何去创建一个带有 wasmedge 的支持 KinD 的节点镜像。

## 编译 crun

KinD 使用 kindest/node 这个镜像作为控制平面和工作节点。该镜像包括作为 CRI 的 containerd 和作为 OCI 运行时的 runc。为了启用 WasmEdge 支持，我们将 runc 替换为 crun。

```Dockerfile
FROM ubuntu:21.10 AS builder
WORKDIR /data
RUN DEBIAN_FRONTEND=noninteractive apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y curl make git gcc build-essential pkgconf libtool libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev go-md2man libtool autoconf python3 automake \
    && curl https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -p /usr/local \
    && git clone --single-branch --branch feat/handler_per_container https://github.com/liquid-reply/crun \
    && cd crun \
    && ./autogen.sh \
    && ./configure --with-wasmedge --enable-embedded-yajl\
    && make 

...
```

现在我们在 `/data/crun/crun` 目录下有了一个新的可以支持 wasmedge 的 `crun` 二进制文件，我们可以在下一步中从这个容器中复制它。

## 替代 crun 和配置 containerd

runc 和 crun 都实现了 OCI 运行时规范，并且它们具有相同的 CLI 参数。因此我们可以用之前创建的 crun-wasmedge 二进制文件替换 runc 二进制文件。

由于 crun 使用了一些共享库，我们需要安装 libyajl、wasmedge 和 criu 来使 crun 工作。

现在我们已经有了一个使用 crun 而不是 runc 的 KinD。现在我们只需要将两个配置更改即可。第一个是在 `/etc/containerd/config.toml` 中，我们添加了可以传递给运行时的 `pod_annotations`：

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
  pod_annotations = ["*.wasm.*", "wasm.*", "module.wasm.image/*", "*.module.wasm.image", "module.wasm.image/variant.*"]
```

第二个是在 `/etc/containerd/cri-base.json` 中，我们删除了一个导致一些问题的 hook。

生成的 dockerfile 如下所示：

```Dockerfile
...

FROM kindest/node:v1.23.0

COPY config.toml /etc/containerd/config.toml
COPY --from=builder /data/crun/crun /usr/local/sbin/runc
COPY --from=builder /usr/local/lib/libwasmedge.so /usr/local/lib/libwasmedge.so

RUN echo "Installing Packages ..." \
    && bash -c 'cat <<< $(jq "del(.hooks.createContainer)" /etc/containerd/cri-base.json) > /etc/containerd/cri-base.json' \
    && ldconfig
```

## 编译和测试

最终我们构建了一个新的 `node-wasmedge` 镜像。为了测试它，我们从该图像创建一个 kind 集群并运行简单的应用程序示例。

```bash
$ docker build -t node-wasmedge .
$ kind create cluster --image node-wasmedge
# Now you can run the example to validate your cluster
$ kubectl run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```
