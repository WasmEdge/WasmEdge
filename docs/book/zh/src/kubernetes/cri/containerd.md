# containerd

## 快速开始

[GitHub repo](https://github.com/second-state/wasmedge-containers-examples/)包含 containerd 相关示例的脚本和 Github Actions，以下是相关链接。

* WebAssembly 简单示例 [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/containerd.yml) | [Successful run](https://github.com/second-state/wasmedge-containers-examples/runs/4328930139?check_suite_focus=true#step:4:25)
* HTTP 服务端示例 [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/containerd-server.yml) | [Successful run](https://github.com/second-state/wasmedge-containers-examples/runs/4328930141?check_suite_focus=true#step:4:44)

在接下来的部分中，我们会展示如何使用这些脚本。

* [安装 containerd](#安装-containerd)
* [示例一： WebAssembly 简单示例](#运行简单-WebAssembly-应用)
* [示例二： WebAssembly 中的 HTTP 服务端示例](#运行-HTTP-服务端应用)

## 安装 containerd

使用以下命令在您的系统上安装 containerd。

```bash
export VERSION="1.5.7"
echo -e "Version: $VERSION"
echo -e "Installing libseccomp2 ..."
sudo apt install -y libseccomp2
echo -e "Installing wget"
sudo apt install -y wget

wget https://github.com/containerd/containerd/releases/download/v${VERSION}/cri-containerd-cni-${VERSION}-linux-amd64.tar.gz
wget https://github.com/containerd/containerd/releases/download/v${VERSION}/cri-containerd-cni-${VERSION}-linux-amd64.tar.gz.sha256sum
sha256sum --check cri-containerd-cni-${VERSION}-linux-amd64.tar.gz.sha256sum

sudo tar --no-overwrite-dir -C / -xzf cri-containerd-cni-${VERSION}-linux-amd64.tar.gz
sudo systemctl daemon-reload
```

将 containerd 配置为使用 `crun` 作为底层 OCI runtime。
此处需要修改 `/etc/containerd/config.toml` 文件。

```bash
sudo mkdir -p /etc/containerd/
sudo bash -c "containerd config default > /etc/containerd/config.toml"
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/containerd_config.diff
sudo patch -d/ -p0 < containerd_config.diff
```

启动 containerd 服务。

```bash
sudo systemctl start containerd
```

在运行下一步之前，请确保你已经构建并安装好了[支持 `WasmEdge`的 `crun`二进制文件](../container/crun.md)。

## 运行简单 WebAssembly 应用

现在，我们可以使用 containerd 运行一个简单的 WebAssembly 程序。
[另一篇文章](../demo/wasi.md) 解释了如何编译、打包和将 WebAssembly 程序作为容器镜像发布至 Docker hub 。
在本节中，我们需要先使用 containerd 工具将这个基于 WebAssembly 的容器镜像从 Docker hub 中拉取下来。

```bash
sudo ctr i pull docker.io/hydai/wasm-wasi-example:with-wasm-annotation
```

现在，您可以使用 ctr（containerd cli 工具）运行此示例。

```bash
sudo ctr run --rm --runc-binary crun --runtime io.containerd.runc.v2 --label module.wasm.image/variant=compat-smart docker.io/hydai/wasm-wasi-example:with-wasm-annotation wasm-example /wasi_example_main.wasm 50000000
```

启动容器会执行 WebAssembly 程序， 您可以在控制台中看到输出。

```bash
Creating POD ...
Random number: -1678124602
Random bytes: [12, 222, 246, 184, 139, 182, 97, 3, 74, 155, 107, 243, 20, 164, 175, 250, 60, 9, 98, 25, 244, 92, 224, 233, 221, 196, 112, 97, 151, 155, 19, 204, 54, 136, 171, 93, 204, 129, 177, 163, 187, 52, 33, 32, 63, 104, 128, 20, 204, 60, 40, 183, 236, 220, 130, 41, 74, 181, 103, 178, 43, 231, 92, 211, 219, 47, 223, 137, 70, 70, 132, 96, 208, 126, 142, 0, 133, 166, 112, 63, 126, 164, 122, 49, 94, 80, 26, 110, 124, 114, 108, 90, 62, 250, 195, 19, 189, 203, 175, 189, 236, 112, 203, 230, 104, 130, 150, 39, 113, 240, 17, 252, 115, 42, 12, 185, 62, 145, 161, 3, 37, 161, 195, 138, 232, 39, 235, 222]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
/wasi_example_main.wasm
50000000
File content is This is in a file
```

接下来，你可尝试在 [Kubernetes](../../kubernetes/kubernetes.md)中运行这个应用!

## 运行 HTTP 服务端应用

最后，我们可以在 containerd 中运行一个简单的基于 WebAssembly 的 HTTP 微服务。
[另一篇文章](../demo/server.md) 解释了如何编译、打包和将 WebAssembly 程序作为容器镜像发布至 Docker hub 。
在本节中，我们需要先使用 containerd 工具将这个基于 WebAssembly 的容器镜像从 Docker hub 中拉取下来。

```bash
sudo ctr i pull docker.io/avengermojo/http_server:with-wasm-annotation
```

现在，您可以使用 ctr（containerd cli 工具）运行该示例。（请注意，我们需要加上 `--net-host` 参数来运行容器，以便可以从外部访问 WasmEdge 容器内的 HTTP server。）

```bash
sudo ctr run --rm --net-host --runc-binary crun --runtime io.containerd.runc.v2 --label module.wasm.image/variant=compat-smart docker.io/avengermojo/http_server:with-wasm-annotation http-server-example /http_server.wasm
```

启动容器会执行 WebAssembly 程序， 您可以在控制台中看到输出。

```bash
new connection at 1234

# Test the HTTP service at that IP address
curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```

接下来，你可尝试在 [Kubernetes](../../kubernetes/kubernetes.md) 中运行这个应用!
