# CRI-O

## 快速开始

 [GitHub repo](https://github.com/second-state/wasmedge-containers-examples/) 包含 CRI-O 相关示例的脚本和 Github Actions，以下是相关链接。

* WebAssembly 简单示例  [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/crio.yml) | [Successful run](https://github.com/second-state/wasmedge-containers-examples/runs/4317457300?check_suite_focus=true#step:4:37)
* HTTP 服务端示例 [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/crio-server.yml) | [Successful run](https://github.com/second-state/wasmedge-containers-examples/runs/4317457313?check_suite_focus=true#step:4:54)

在接下来的部分中，我们会展示如何使用这些脚本。

* [安装 CRI-O](#安装-cri-o)
* [配置 CRI-O 和 crun](#配置-CRI-O-以使用-crun)
* [示例一 :  WebAssembly 简单示例](#运行简单-WebAssembly-应用)
* [示例二 : WebAssembly 中的 HTTP 服务端示例](#运行-HTTP-服务端应用)

## 安装 CRI-O

使用以下命令在你的系统上安装 CRI-O。

```bash
export OS="xUbuntu_20.04"
export VERSION="1.21"
apt update
apt install -y libseccomp2 || sudo apt update -y libseccomp2
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/ /" > /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/$VERSION/$OS/ /" > /etc/apt/sources.list.d/devel:kubic:libcontainers:stable:cri-o:$VERSION.list

curl -L https://download.opensuse.org/repositories/devel:kubic:libcontainers:stable:cri-o:$VERSION/$OS/Release.key | apt-key add -
curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/Release.key | apt-key add -

apt-get update
apt-get install criu libyajl2
apt-get install cri-o cri-o-runc cri-tools containernetworking-plugins
systemctl start crio
```

## 配置 CRI-O 以使用 crun

CRI-O 默认使用 `runc` 运行时，我们需要修改配置以使用 `crun` 代替。
这需要添加到两个配置文件来完成。

>在运行下一步之前，请确保你已经构建并安装好了[支持 `WasmEdge` 的 `crun` 二进制文件](../container/crun.md)。

首先，创建一个 `/etc/crio/crio.conf` 文件并添加以下内容，它会使 CRI-O 默认使用 `crun` 作为运行时。

```conf
[crio.runtime]
default_runtime = "crun"
```

`crun` 运行时在 `/etc/crio/crio.conf.d/01-crio-runc.conf` 文件中定义，按下面的内容修改此文件。

```conf
[crio.runtime.runtimes.runc]
runtime_path = "/usr/lib/cri-o-runc/sbin/runc"
runtime_type = "oci"
runtime_root = "/run/runc"
# The above is the original content

# Add our crunw runtime here
[crio.runtime.runtimes.crun]
runtime_path = "/usr/bin/crun"
runtime_type = "oci"
runtime_root = "/run/crun"
```

接下来，重新启动 CRI-O 以应用配置更改。

```bash
systemctl restart crio
```

## 运行简单 WebAssembly 应用

现在，我们可以使用 CRI-O 运行一个简单的 WebAssembly 程序。
[另一篇文章](../demo/wasi.md) 解释了如何编译、打包和将 WebAssembly 程序作为容器镜像发布至 Docker hub。
在本节中，我们需要先使用 CRI-O 工具将这个基于 WebAssembly 的容器镜像从 Docker hub 中拉取下来。

```bash
sudo crictl pull docker.io/hydai/wasm-wasi-example:with-wasm-annotation
```

接下来，我们需要创建两个简单的配置文件，指定 CRI-O 应该如何在 sandbox 中运行这个 WebAssembly 镜像。 我们已经有 [container_wasi.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/container_wasi.json) 和 [sandbox_config.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/sandbox_config.json) 这两个文件。你可以使用下面的命令将它们下载到本地目录。

```bash
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/sandbox_config.json
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/container_wasi.json
```

现在你可以用 CRI-O 创建 pod 和容器。只需用本文的配置选项即可。

```bash
# 创建 POD。输出会与示例不同。
$ sudo crictl runp sandbox_config.json
7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4
# 设置一个辅助变量供之后使用。
$ POD_ID=7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4

# 创建容器实例。输出会与示例不同。
$ sudo crictl create $POD_ID container_wasi.json sandbox_config.json
# 设置一个辅助变量供之后使用。
CONTAINER_ID=1d056e4a8a168f0c76af122d42c98510670255b16242e81f8e8bce8bd3a4476f
```

启动容器会执行 WebAssembly 程序， 你可以在控制台中看到输出。

```bash
# 列出容器，容器状态应为 `Created`。
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       hydai/wasm-wasi-example:with-wasm-annotation   About a minute ago   Created             podsandbox1-wasm-wasi   0                   7992e75df00cc

# 启动容器。
$ sudo crictl start $CONTAINER_ID

# 再次检查容器状态。
# 如果容器没有结束作业，你会看到它处于 Running 状态。
# 因为这个示例很小。此时你可能会看到已退出。
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       hydai/wasm-wasi-example:with-wasm-annotation   About a minute ago   Running             podsandbox1-wasm-wasi   0                   7992e75df00cc

# 当容器完成作业时，你可以看到他的状态变为已退出。
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       hydai/wasm-wasi-example:with-wasm-annotation   About a minute ago   Exited              podsandbox1-wasm-wasi   0                   7992e75df00cc

# 检查容器的日志，它应该显示 WebAssembly 程序的输出。
$ sudo crictl logs $CONTAINER_ID

Test 1: Print Random Number
Random number: 960251471

Test 2: Print Random Bytes
Random bytes: [50, 222, 62, 128, 120, 26, 64, 42, 210, 137, 176, 90, 60, 24, 183, 56, 150, 35, 209, 211, 141, 146, 2, 61, 215, 167, 194, 1, 15, 44, 156, 27, 179, 23, 241, 138, 71, 32, 173, 159, 180, 21, 198, 197, 247, 80, 35, 75, 245, 31, 6, 246, 23, 54, 9, 192, 3, 103, 72, 186, 39, 182, 248, 80, 146, 70, 244, 28, 166, 197, 17, 42, 109, 245, 83, 35, 106, 130, 233, 143, 90, 78, 155, 29, 230, 34, 58, 49, 234, 230, 145, 119, 83, 44, 111, 57, 164, 82, 120, 183, 194, 201, 133, 106, 3, 73, 164, 155, 224, 218, 73, 31, 54, 28, 124, 2, 38, 253, 114, 222, 217, 202, 59, 138, 155, 71, 178, 113]

Test 3: Call an echo function
Printed from wasi: This is from a main function
This is from a main function

Test 4: Print Environment Variables
The env vars are as follows.
PATH: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
TERM: xterm
HOSTNAME: crictl_host
PATH: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
The args are as follows.
/var/lib/containers/storage/overlay/006e7cf16e82dc7052994232c436991f429109edea14a8437e74f601b5ee1e83/merged/wasi_example_main.wasm
50000000

Test 5: Create a file `/tmp.txt` with content `This is in a file`

Test 6: Read the content from the previous file
File content is This is in a file

Test 7: Delete the previous file
```

接下来，你可尝试在 [Kubernetes](../../kubernetes/kubernetes.md) 中运行这个应用!

## **运行 HTTP 服务端应用**

最后，我们可以在 CRI-O 中运行一个简单的基于 WebAssembly 的 HTTP 微服务。
[另一篇文章](../demo/server.md) 解释了如何编译、打包和将 WebAssembly 程序作为容器镜像发布至 Docker hub 。
在本节中，我们需要先使用 CRI-O 工具将这个基于 WebAssembly 的容器镜像从 Docker hub 中拉取下来。

```bash
sudo crictl pull docker.io/avengermojo/http_server:with-wasm-annotation
```

接下来，我们需要创建两个简单的配置文件，指定 CRI-O 应该如何在 sandbox 中运行这个 WebAssembly 镜像。 我们已经有 [container_http_server.json](https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/http_server/container_http_server.json) 和 [sandbox_config.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/sandbox_config.json) 这两个文件。你可以使用下面的命令将它们下载到本地目录。

> HTTP 服务端示例和 WASI 简单示例使用的 `sandbox_config.json` 文件是相同的。 但另一个 `container_*.json` 文件是特定的，因为它包含应用程序的 Docker Hub 链接。

```bash
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/sandbox_config.json
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/http_server/container_http_server.json
```

现在你可以用 CRI-O 创建 pod 和容器。只需用本文的配置选项即可。

```bash
# 创建 POD。输出会与示例不同。
$ sudo crictl runp sandbox_config.json
7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4
# 设置一个辅助变量供之后使用。
$ POD_ID=7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4

# 创建容器实例。输出会与示例不同。
$ sudo crictl create $POD_ID container_wasi.json sandbox_config.json
# 设置一个辅助变量供之后使用。
CONTAINER_ID=1d056e4a8a168f0c76af122d42c98510670255b16242e81f8e8bce8bd3a4476f
```

启动容器会执行 WebAssembly 程序， 你可以在控制台中看到输出。

```bash
# 启动容器
$ sudo crictl start $CONTAINER_ID

# 检查容器状态。它应该正在运行。
# 如果没有，请等待几秒钟，然后再次检查。
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED                  STATE               NAME                ATTEMPT             POD ID
4eeddf8613691       avengermojo/http_server:with-wasm-annotation   Less than a second ago   Running             http_server         0                   1d84f30e7012e

# 检查容器的日志以查看 HTTP 服务器正在监听端口为 1234 。
$ sudo crictl logs $CONTAINER_ID
new connection at 1234

# 获取分配给容器的 IP 地址。
$ sudo crictl inspect $CONTAINER_ID | grep IP.0 | cut -d: -f 2 | cut -d'"' -f 2
10.85.0.2

# 在该 IP 地址测试 HTTP 服务状态。
$ curl -d "name=WasmEdge" -X POST http://10.85.0.2:1234
echo: name=WasmEdge
```

接下来，你可尝试在 [Kubernetes](../../kubernetes/kubernetes.md) 中运行这个应用!
