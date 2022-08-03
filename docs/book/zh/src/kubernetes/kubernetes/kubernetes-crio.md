# Kubernetes + CRI-O

## 快速开始

[这个 GitHub 仓库](https://github.com/second-state/wasmedge-containers-examples/) 中包含了使用 Kubernetes + CRI-O 运行示例程序所需的脚本和 Github Actions 配置文件。

* 简单示例程序 [快速开始](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/README.md) | [查看 Github Actions 配置文件](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio.yml) | [查看运行结果](https://github.com/second-state/wasmedge-containers-examples/runs/4328930134?check_suite_focus=true#step:6:3007)

* 基于 WebAssembly 的 HTTP 服务 [快速开始](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/http_server/README.md) | [查看 Github Actions 配置文件](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio-server.yml) | [查看运行结果](https://github.com/second-state/wasmedge-containers-examples/runs/4577789182?check_suite_focus=true#step:6:3030)

本节接下来的部分将详细地解释其中的细节。你需要先 [安装并配置好 CRI-O](../cri/crio.md) 才能使用 WasmEdge 容器镜像。

## 安装和启动 Kubernetes

在终端窗口运行下面的脚本，就可以在本地开发环境中安装和启动一个 Kubernetes。

```bash
# 安装 go
$ wget https://golang.org/dl/go1.17.1.linux-amd64.tar.gz
$ sudo rm -rf /usr/local/go
sudo tar -C /usr/local -xzf go1.17.1.linux-amd64.tar.gz
source /home/${USER}/.profile

# 克隆 k8s
git clone https://github.com/kubernetes/kubernetes.git
cd kubernetes
git checkout v1.22.2

# 用 k8s 中的安装脚本安装 etcd
sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/install-etcd.sh
export PATH="/home/${USER}/kubernetes/third_party/etcd:${PATH}"
sudo cp third_party/etcd/etcd* /usr/local/bin/

# 以上命令运行完毕以后，系统中就可以找到以下文件：/usr/local/bin/etcd  /usr/local/bin/etcdctl  /usr/local/bin/etcdutl 

# 用 CRI-O 构建运行 k8s
sudo apt-get install -y build-essential
sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/local-up-cluster.sh

... ...
Local Kubernetes cluster is running. Press Ctrl-C to shut it down.
```

此时**不要**关闭终端，因为 Kubernetes 正在运行中！

## 在 Kubernetes 中运行 WebAssembly 容器镜像

本节的内容介绍了如何在 Kubernetes 中，以 pod 中的容器的形式来运行 WebAssembly 程序。我们需要打开另一个终端。

```bash
export KUBERNETES_PROVIDER=local

sudo cluster/kubectl.sh config set-cluster local --server=https://localhost:6443 --certificate-authority=/var/run/kubernetes/server-ca.crt
sudo cluster/kubectl.sh config set-credentials myself --client-key=/var/run/kubernetes/client-admin.key --client-certificate=/var/run/kubernetes/client-admin.crt
sudo cluster/kubectl.sh config set-context local --cluster=local --user=myself
sudo cluster/kubectl.sh config use-context local
sudo cluster/kubectl.sh
```

检查一下集群状态，看看集群有没有正常运行。

```bash
$ sudo cluster/kubectl.sh cluster-info

# 运行命令后预计会有以下输出
Cluster "local" set.
User "myself" set.
Context "local" created.
Switched to context "local".
Kubernetes control plane is running at https://localhost:6443
CoreDNS is running at https://localhost:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.
```

### 简单的 WebAssembly 应用

[这篇文章](../demo/wasi.md) 描述了如何编译、打包一个简单的 WebAssembly WASI 程序，以及将它以容器镜像的形式发布到 Docker hub 的完整过程。

```bash
sudo cluster/kubectl.sh run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```

容器化后的应用程序的输出会被打印到控制台上。

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

### 基于 WebAssembly 的 HTTP 服务

[这篇文章](../demo/server.md) 描述了如何编译、打包一个基于 WebAssembly 的 HTTP 服务应用程序，以及将它以容器镜像的形式发布到 Docker hub 的完整过程。由于运行 HTTP 服务的容器需要 Kubernetes 提供网络支持，我们需要用 [k8s-http_server.yaml](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/http_server/k8s-http_server.yaml) 文件设定具体的配置。

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: http-server
  namespace: default
  annotations:
    module.wasm.image/variant: compat-smart
spec:
  hostNetwork: true
  containers:
  - name: http-server
    image: avengermojo/http_server:with-wasm-annotation
    command: [ "/http_server.wasm" ]
    ports:
    - containerPort: 1234
      protocol: TCP
    livenessProbe:
      tcpSocket:
        port: 1234
      initialDelaySeconds: 3
      periodSeconds: 30
```

输入下面的命令，可以使用上面的 `k8s-http_server.yaml` 文件，从 Docker Hub 拉取并运行基于 WebAssembly 的镜像。

```bash
sudo ./kubernetes/cluster/kubectl.sh apply -f k8s-http_server.yaml
```

然后运行以下命令，查看运行中的容器应用程序和他们的 IP 地址。我们在 yaml 配置中用的是 `hostNetwork` 网络，所以 HTTP 服务器镜像会在 IP 为 `127.0.0.1` 的本地网络中运行。

```bash
$ sudo cluster/kubectl.sh get pod --all-namespaces -o wide

NAMESPACE     NAME                       READY   STATUS             RESTARTS      AGE   IP          NODE        NOMINATED NODE   READINESS GATES
default       http-server                1/1     Running            1 (26s ago)     60s     127.0.0.1   127.0.0.1   <none>           <none>
```

现在，我们可以用 `curl` 命令来访问 HTTP 服务了。

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```

搞定！
