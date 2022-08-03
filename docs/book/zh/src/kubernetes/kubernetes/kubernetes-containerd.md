# Kubernetes + containerd

## 快速开始

[GitHub 仓库](https://github.com/second-state/wasmedge-containers-examples/) 包含在 Kubernetes + containerd 上运行我们的示例应用程序的脚本和 Github Actions。

* 简单的 WebAssembly 示例 [快速开始](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-containerd.yml) | [成功运行](https://github.com/second-state/wasmedge-containers-examples/runs/4577789181?check_suite_focus=true#step:6:3010)
* 基于 WebAssembly 的 HTTP 服务 [快速开始](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_containerd/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-containerd-server.yml) | [成功运行](https://github.com/second-state/wasmedge-containers-examples/runs/4577789184?check_suite_focus=true#step:6:3016)

在本节的其余部分，我们将详细解释这些步骤。我们将假设你已经 [安装并配置了 containerd](../cri/containerd.md)，以便与 WasmEdge 容器镜像一起工作。

## 安装和启动 Kubernetes

从终端窗口运行以下命令，为本地开发设置了 Kubernetes。

```bash
# 安装 go
$ wget https://golang.org/dl/go1.17.1.linux-amd64.tar.gz
$ sudo rm -rf /usr/local/go
$ sudo tar -C /usr/local -xzf go1.17.1.linux-amd64.tar.gz
$ source /home/${USER}/.profile

# 克隆 k8s 源码
$ git clone https://github.com/kubernetes/kubernetes.git
$ cd kubernetes
$ git checkout v1.22.2

# 在 k8s 中用 hack 脚本安装 etcd
$ sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/install-etcd.sh
$ export PATH="/home/${USER}/kubernetes/third_party/etcd:${PATH}"
$ sudo cp third_party/etcd/etcd* /usr/local/bin/

# 运行上述命令后，你可以找到以下文件：/usr/local/bin/etcd  /usr/local/bin/etcdctl  /usr/local/bin/etcdutl

# 用 containerd 构建和运行 k8s
$ sudo apt-get install -y build-essential
$ sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/local-up-cluster.sh

... ...
Local Kubernetes cluster is running. Press Ctrl-C to shut it down.
```
  
**不要关闭**你的终端窗口。Kubernetes 正在运行!

## 在 Kubernetes 中运行 WebAssembly 容器镜像

最后，我们可以在 Kubernetes 中将 WebAssembly 程序作为容器运行在 pod 中。
在本节中，我们将从**另一个终端窗口**开始，使用集群。

```bash
export KUBERNETES_PROVIDER=local

sudo cluster/kubectl.sh config set-cluster local --server=https://localhost:6443 --certificate-authority=/var/run/kubernetes/server-ca.crt
sudo cluster/kubectl.sh config set-credentials myself --client-key=/var/run/kubernetes/client-admin.key --client-certificate=/var/run/kubernetes/client-admin.crt
sudo cluster/kubectl.sh config set-context local --cluster=local --user=myself
sudo cluster/kubectl.sh config use-context local
sudo cluster/kubectl.sh
```

让我们检查一下状态，确保集群正在运行。

```bash
$ sudo cluster/kubectl.sh cluster-info

# 期望输出
Cluster "local" set.
User "myself" set.
Context "local" created.
Switched to context "local".
Kubernetes control plane is running at https://localhost:6443
CoreDNS is running at https://localhost:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.
```

### 一个简单的 WebAssembly 应用程序

[另一篇文章](../demo/wasi.md)解释了如何编译、打包和发布一个简单的 WebAssembly WASI 程序作为容器镜像到 Docker hub。
在 Kubernetes 集群中运行 Docker Hub 中基于 WebAssembly 的镜像，方法如下。

```bash
sudo cluster/kubectl.sh run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" --overrides='{"kind":"Pod", "apiVersion":"v1", "spec": {"hostNetwork": true}}' /wasi_example_main.wasm 50000000
```

容器化应用程序的输出被打印到控制台。

```bash
Random number: 401583443
Random bytes: [192, 226, 162, 92, 129, 17, 186, 164, 239, 84, 98, 255, 209, 79, 51, 227, 103, 83, 253, 31, 78, 239, 33, 218, 68, 208, 91, 56, 37, 200, 32, 12, 106, 101, 241, 78, 161, 16, 240, 158, 42, 24, 29, 121, 78, 19, 157, 185, 32, 162, 95, 214, 175, 46, 170, 100, 212, 33, 27, 190, 139, 121, 121, 222, 230, 125, 251, 21, 210, 246, 215, 127, 176, 224, 38, 184, 201, 74, 76, 133, 233, 129, 48, 239, 106, 164, 190, 29, 118, 71, 79, 203, 92, 71, 68, 96, 33, 240, 228, 62, 45, 196, 149, 21, 23, 143, 169, 163, 136, 206, 214, 244, 26, 194, 25, 101, 8, 236, 247, 5, 164, 117, 40, 220, 52, 217, 92, 179]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
/wasi_example_main.wasm
50000000
File content is This is in a file
pod "wasi-demo-2" deleted
```

### 一个基于 WebAssembly 的 HTTP 服务

[另一篇文章](../demo/server.md)解释了如何编译、打包和发布一个简单的 WebAssembly HTTP 服务应用程序作为容器镜像到 Docker hub。
在 Kubernetes 集群中运行 Docker Hub 中基于 WebAssembly 的镜像，方法如下。

```bash
sudo cluster/kubectl.sh run --restart=Never http-server --image=avengermojo/http_server:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" --overrides='{"kind":"Pod", "apiVersion":"v1", "spec": {"hostNetwork": true}}'
```

由于我们在 `kubectl run` 命令中使用了 `hostNetwork` ，HTTP 服务器镜像运行在本地网络上，IP 地址是 `127.0.0.1` 。
现在，你可以使用 `curl` 命令来访问 HTTP 服务。

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```

就是这样!
