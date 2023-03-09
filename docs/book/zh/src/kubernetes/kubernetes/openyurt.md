# OpenYurt + containerd + crun

在这篇文章中，我们将介绍如何在 [OpenYurt](https://github.com/openyurtio/openyurt)  边缘计算框架下，使用 containerd 去运行一个简单的 WasmEdge 的 demo

## 搭建一个 OpenYurt 集群

这里，我们介绍两种不同的方式来搭建一个 OpenYurt 集群。
第一个是从零开始搭建，先搭建出一个 kubernetes 集群，然后使用 `yurtctl convert` 将
这个 kubernetes 集群转化为 OpenYurt 集群；
第二个是使用 [OpenYurt 体验中心](https://openyurt.io/docs/installation/openyurt-experience-center/overview)   提供的能力，即只要在 OpenYurt 体验中心注册一个账户，即可拥有一个 OpenYurt 集群，如果需要接入新的节点到 OpenYurt 集群中，只需要使用
 `yurtctl join` 命令加入节点。

### 前置准备

|        | OS/kernel                            | 私网IP/ 公网IP                |
| ------ | ------------------------------------ | ---------------------------- |
| Master | Ubuntu  20.04.3 LTS/5.4.0-91-generic | 192.168.3.169/120.55.126.18  |
| Node   | Ubuntu  20.04.3 LTS/5.4.0-91-generic | 192.168.3.170/121.43.113.152 |  

由于操作系统的不同，下面的步骤可能有一些微小的变动，主要是关于 [OpenYurt](https://github.com/openyurtio/openyurt)  和 [crun](https://github.com/containers/crun)  的安装。

因为我们使用 `yurtctl convert` 命令将 K8s 集群转化成 OpenYurt 集群，因此我们应该首先搭建一个 kubernetes 集群。当然， OpenYurt 也提供了一个简单的方法， `yurtctl init/join` 可以允许我们不搭建 kubernetes 集群，直接创建一个 OpenYurt 集群出来。
更多关于 `yurtctl convert/revert` 的介绍可以参考文章[Conversion between OpenYurt and Kubernetes:`yurtctl convert/revert`](https://openyurt.io/docs/installation/yurtctl-convert-revert) ，
而关于 `yurtctl init/join` 的介绍可以参考文章 [how use `Yurtctl init/join`](https://openyurt.io/docs/installation/yurtctl-init-join)

- 关闭节点 swap 分区

```bash
sudo swapoff -a
//verify    
free -m
```

- 配置节点 IP 与 DNS 映射

```bash
192.168.3.169  oy-master 
120.55.126.18  oy-master
92.168.3.170   oy-node
121.43.113.152 oy-node
```

- 加载内核模块 br_netfilter

```bash
sudo modprobe br_netfilter     
lsmod | grep br_netfilter    //确认加载
```

- 调整内核参数，创建 k8s.conf

```bash
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
EOF
sudo sysctl --system
```

- 设置 rp-filter 值，将文件 /etc/sysctl.d/10-network-security.conf 中两个参数的值由2改为1，且将 /proc/sys/net/ipv4/ip_forward 设置为1

```bash
sudo vi /etc/sysctl.d/10-network-security.conf
echo 1 > /proc/sys/net/ipv4/ip_forward
sudo sysctl --system
```

#### 安装 containerd

- 安装 containerd

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

- 配置 containerd 文件 /etc/containerd/config.toml，修改 plugin 中参数，将其中的 runc 改为 crun，添加 pod_annotation

```bash
sudo mkdir -p /etc/containerd/
sudo bash -c "containerd config default > /etc/containerd/config.toml"
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/containerd_config.diff
sudo patch -d/ -p0 < containerd_config.diff
```

- 重启 containerd 服务：

```bash
systemctl start containerd
```

#### 安装 WasmEdge

使用 WasmEdge 提供的[安装脚本](https://wasmedge.org/book/en/start/install.html) 在你的边缘节点上安装 WasmEdge

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

#### 编译、安装 crun

我们需要一个在边缘节点支持 WasmEdge 的 crun 二进制文件。最直接的方法就是自己从源码编译。首先，需要确保在 ubuntu 系统上安装 crun 依赖。对于其他的 Linux 发行版，请参考[文章](https://github.com/containers/crun#readme)

- 编译需要的依赖

```bash
sudo apt update
sudo apt install -y make git gcc build-essential pkgconf libtool \
  libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
  go-md2man libtool autoconf python3 automake
```

- 配置、编译、安装支持 WasmEdge 的 crun

```bash
it clone https://github.com/containers/crun
cd crun
./autogen.sh
./configure --with-wasmedge
make
sudo make install
```

### 从零搭建一个 OpenYurt 集群

在这个 demo 中，我们使用两台虚拟机来搭建 OpenYurt 集群，一台模拟云端节点 Master，一台模拟边缘节点 Node，由这样两个节点构成最简单的 OpenYurt 集群系统。

#### 搭建一个 kubernetes 集群

```bash
$ sudo apt-get update && sudo apt-get install -y ca-certificates curl software-properties-common apt-transport-https
// add K8s source
$ curl -s https://mirrors.aliyun.com/kubernetes/apt/doc/apt-key.gpg | sudo apt-key add -
$ sudo tee /etc/apt/sources.list.d/kubernetes.list <<EOF
$ deb https://mirrors.aliyun.com/kubernetes/apt/ kubernetes-xenial main
// install K8s components 1.18.9
$ sudo apt-get update && sudo apt-get install -y kubelet=1.18.9-00 kubeadm=1.18.9-00 kubectl=1.18.9-00 
// Initialize the master node
$ sudo kubeadm init --pod-network-cidr 172.16.0.0/16 \
--apiserver-advertise-address=192.168.3.167 \
--image-repository registry.cn-hangzhou.aliyuncs.com/google_containers
// join the work node
$ kubeadm join 192.168.3.167:6443 --token 3zefbt.99e6denc1cxpk9fg \
   --discovery-token-ca-cert-hash sha256:8077d4e7dd6eee64a999d56866ae4336073ed5ffc3f23281d757276b08b9b195
```

#### 安装 yurtctl

使用下面命令来安装 yurtctl，借助 yurtctl 命令行工具，我们可以安装/卸载 OpenYurt 集群，也可以用于在 kubernetes 集群和 OpenYurt 集群转化。

```bash
git clone https://github.com/openyurtio/openyurt.git
cd openyurt
make build WHAT=cmd/yurtctl
```

#### 安装 OpenYurt 组件

OpenYurt 包括多个组件。YurtHub 是节点和 kube-apiserver 之间的流量代理，边缘节点上部署的 YurtHub 能够缓存来自云端的数据信息。
 Yurt controller 是对上游节点控制器的补充，以支持边缘计算需求。TunnelServer 通过反向代理与运行在每个
边缘节点上的 TunnelAgent 守护进程连接，以在云端节点控制平面和连接到内网的边缘节点之间建立安全的网络访问。
更详细的信息可以参考 [OpenYurt Docs](https://github.com/openyurtio/openyurt)

```bash
yurtctl convert --deploy-yurttunnel --cloud-nodes oy-master --provider kubeadm\
--yurt-controller-manager-image="openyurt/yurt-controller-manager:v0.5.0"\
--yurt-tunnel-agent-image="openyurt/yurt-tunnel-agent:v0.5.0"\
--yurt-tunnel-server-image="openyurt/yurt-tunnel-server:v0.5.0"\
--node-servant-image="openyurt/node-servant:latest"\
--yurthub-image="openyurt/yurthub:v0.5.0"
```

实际上，我们更推荐安装 OpenYurt0.6.0 版本，而且已经验证通过。关于如何安装该版本，可以参考[文章](https://github.com/openyurtio/openyurt/releases/tag/v0.6.0)

### 利用 OpenYurt 体验中心快速搭建一个 OpenYurt 集群

下面，我们将介绍一种快速搭建 OpenYurt 集群的方法，你只需要在 OpenYurt 体验中心注册一个账户，就可以拥有一个 OpenYurt 集群。如果你想接入一个边缘节点到你创建的 OpenYurt 集群中，
根据提示使用 `yurtctl join` 命令即可。更多关于 OpenYurt 体验中心的介绍，可以参考[文章](https://github.com/openyurtio/openyurt/releases/tag/v0.6.0)

## 运行一个简单的 WebAssembly 应用

这一部分我们将在 OpenYurt 集群 pod 中的一个容器里运行 WebAssembly 应用。首先我们需要从 Docker Hub 拉取基于 WebAssembly 容器镜像，如果你对如何编译、打包以及发布一个基于
 WebAssembly 的容器镜像感兴趣，可以阅读 [WasmEdge Book](https://wasmedge.org/book/en/kubernetes/demo/wasi.html)

在验证 OpenYurt 上运行 WasmEdge 应用的时候，我们参考了之前关于 [kubernetes 的验证](https://wasmedge.org/book/en/kubernetes/kubernetes.html) ，

需要注意的是由于命令 `kubectl run` 在1.18.9版本上缺少 annotations 参数，我们需要修改命令行的内容。
如果你使用的是 OpenYurt 体验中心，默认情况下搭建的 OpenYurt 的版本是0.6.0，对应的 Kubernetes 版本是1.20.11。

```bash
// kubectl 1.18.9
$ sudo kubectl run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation  --overrides='{"kind":"Pod","metadata":{"annotations":{"module.wasm.image/variant":"compat-smart"}} , "apiVersion":"v1", "spec": {"hostNetwork": true}}' /wasi_example_main.wasm 50000000
// kubectl 1.20.11
$ sudo kubectl run -it --rm --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" --overrides='{"kind":"Pod", "apiVersion":"v1", "spec": {"hostNetwork": true}}' /wasi_example_main.wasm 50000000
```

容器化应用的结果将会在控制台上输出，对应不同 Kubernetes 版本，得到的结果都是相同的。

```bash
Random number: 1123434661
Random bytes: [25, 169, 202, 211, 22, 29, 128, 133, 168, 185, 114, 161, 48, 154, 56, 54, 99, 5, 229, 161, 225, 47, 85, 133, 90, 61, 156, 86, 3, 14, 10, 69, 185, 225, 226, 181, 141, 67, 44, 121, 157, 98, 247, 148, 201, 248, 236, 190, 217, 245, 131, 68, 124, 28, 193, 143, 215, 32, 184, 50, 71, 92, 148, 35, 180, 112, 125, 12, 152, 111, 32, 30, 86, 15, 107, 225, 39, 30, 178, 215, 182, 113, 216, 137, 98, 189, 72, 68, 107, 246, 108, 210, 148, 191, 28, 40, 233, 200, 222, 132, 247, 207, 239, 32, 79, 238, 18, 62, 67, 114, 186, 6, 212, 215, 31, 13, 53, 138, 97, 169, 28, 183, 235, 221, 218, 81, 84, 235]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
/wasi_example_main.wasm
50000000
File content is This is in a file
pod "wasi-demo" deleted
```

我们可以使用如下命令检查 pod 的状态

```bash
crictl ps -a
```

输出结果如下：

```bash
CONTAINER           IMAGE               CREATED             STATE               NAME                 ATTEMPT             POD ID
0c176ed65599a       0423b8eb71e31       8 seconds ago       Exited              wasi-demo  
```

至此，在 OpenYurt 上运行一个基于 WasmEdge 应用的简单 demo 已经完成了。如果你在实验的过程中遇到问题或者有一些建议，非常欢迎来到我们项目的 github 上提交 issue，让我们知道你的想法。

- WasmEdge GitHub repo: <https://github.com/WasmEdge/WasmEdge>
- OpenYurt GitHub repo: <https://github.com/openyurtio/openyurt>
