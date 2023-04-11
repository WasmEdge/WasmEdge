# 为 KubeEdge 创建一个 `crun` 示例

## 1. 云端设置（KubeEdge 主节点）

### 安装 Go

```bash
$ wget https://golang.org/dl/go1.17.3.linux-amd64.tar.gz
$ tar xzvf go1.17.3.linux-amd64.tar.gz

$ export PATH=/home/${user}/go/bin:$PATH
$ go version
go version go1.17.3 linux/amd64
```

### 安装 CRI-O

请参阅 [CRI-O Installation Instructions](https://github.com/cri-o/cri-o/blob/main/install.md#install-packaged-versions-of-cri-o).

```bash
# 创建 .conf 文件以在启动时加载模块
$ cat <<EOF | sudo tee /etc/modules-load.d/crio.conf
overlay
br_netfilter
EOF

$ sudo modprobe overlay
$ sudo modprobe br_netfilter

# 设置所需的 sysctl 参数，这些参数在重启后仍然存在。
$ cat <<EOF | sudo tee /etc/sysctl.d/99-kubernetes-cri.conf
net.bridge.bridge-nf-call-iptables  = 1
net.ipv4.ip_forward                 = 1
net.bridge.bridge-nf-call-ip6tables = 1
EOF

$ sudo sysctl --system
$ export OS="xUbuntu_20.04"
$ export VERSION="1.21"
$ cat <<EOF | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/ /
EOF
$ cat <<EOF | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable:cri-o:$VERSION.list
deb http://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/$VERSION/$OS/ /
EOF

$ curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/Release.key | sudo apt-key --keyring /etc/apt/trusted.gpg.d/libcontainers.gpg add -
$ curl -L https://download.opensuse.org/repositories/devel:kubic:libcontainers:stable:cri-o:$VERSION/$OS/Release.key | sudo apt-key --keyring /etc/apt/trusted.gpg.d/libcontainers-cri-o.gpg add -

$ sudo apt-get update
$ sudo apt-get install cri-o cri-o-runc

$ sudo systemctl daemon-reload
$ sudo systemctl enable crio --now
$ sudo systemctl status cri-o
```

输出：

```bash
$ sudo systemctl status cri-o
● crio.service - Container Runtime Interface for OCI (CRI-O)
     Loaded: loaded (/lib/systemd/system/crio.service; enabled; vendor preset: enabled)
     Active: active (running) since Mon 2021-12-06 13:46:29 UTC; 16h ago
       Docs: https://github.com/cri-o/cri-o
   Main PID: 6868 (crio)
      Tasks: 14
     Memory: 133.2M
     CGroup: /system.slice/crio.service
             └─6868 /usr/bin/crio

Dec 07 06:04:13 master crio[6868]: time="2021-12-07 06:04:13.694226800Z" level=info msg="Checking image status: registry.k8s.io/pause:3.4.1" id=1dbb722e-f031-410c-9f45-5d4b5760163e name=/runtime.v1alpha2.ImageServic>
Dec 07 06:04:13 master crio[6868]: time="2021-12-07 06:04:13.695739507Z" level=info msg="Image status: &{0xc00047fdc0 map[]}" id=1dbb722e-f031-410c-9f45-5d4b5760163e name=/runtime.v1alpha2.ImageService/ImageSta>
Dec 07 06:09:13 master crio[6868]: time="2021-12-07 06:09:13.698823984Z" level=info msg="Checking image status: registry.k8s.io/pause:3.4.1" id=661b754b-48a4-401b-a03f-7f7a553c7eb6 name=/runtime.v1alpha2.ImageServic>
Dec 07 06:09:13 master crio[6868]: time="2021-12-07 06:09:13.703259157Z" level=info msg="Image status: &{0xc0004d98f0 map[]}" id=661b754b-48a4-401b-a03f-7f7a553c7eb6 name=/runtime.v1alpha2.ImageService/ImageSta>
Dec 07 06:14:13 master crio[6868]: time="2021-12-07 06:14:13.707778419Z" level=info msg="Checking image status: registry.k8s.io/pause:3.4.1" id=8c7e4d36-871a-452e-ab55-707053604077 name=/runtime.v1alpha2.ImageServic>
Dec 07 06:14:13 master crio[6868]: time="2021-12-07 06:14:13.709379469Z" level=info msg="Image status: &{0xc000035030 map[]}" id=8c7e4d36-871a-452e-ab55-707053604077 name=/runtime.v1alpha2.ImageService/ImageSta>
Dec 07 06:19:13 master crio[6868]: time="2021-12-07 06:19:13.713158978Z" level=info msg="Checking image status: registry.k8s.io/pause:3.4.1" id=827b6315-f145-4f76-b8da-31653d5892a2 name=/runtime.v1alpha2.ImageServic>
Dec 07 06:19:13 master crio[6868]: time="2021-12-07 06:19:13.714030148Z" level=info msg="Image status: &{0xc000162bd0 map[]}" id=827b6315-f145-4f76-b8da-31653d5892a2 name=/runtime.v1alpha2.ImageService/ImageSta>
Dec 07 06:24:13 master crio[6868]: time="2021-12-07 06:24:13.716746612Z" level=info msg="Checking image status: registry.k8s.io/pause:3.4.1" id=1d53a917-4d98-4723-9ea8-a2951a472cff name=/runtime.v1alpha2.ImageServic>
Dec 07 06:24:13 master crio[6868]: time="2021-12-07 06:24:13.717381882Z" level=info msg="Image status: &{0xc00042ce00 map[]}" id=1d53a917-4d98-4723-9ea8-a2951a472cff name=/runtime.v1alpha2.ImageService/ImageSta>
```

### 使用 K8s Kubeadm 安装和创建集群

请参阅 [使用 kubeadm 创建集群](https://kubernetes.io/zh/docs/setup/production-environment/tools/kubeadm/create-cluster-kubeadm/).

#### 安装 K8s

```bash
sudo apt-get update
sudo apt-get install -y apt-transport-https curl
echo "deb [signed-by=/usr/share/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list

sudo apt update
K_VER="1.21.0-00"
sudo apt install -y kubelet=${K_VER} kubectl=${K_VER} kubeadm=${K_VER}
sudo apt-mark hold kubelet kubeadm kubectl
```

#### 使用 kubeadm 创建集群

```bash
# kubernetes 调度程序需要进行此设置。
$ sudo swapoff -a
$ sudo vim /etc/fstab
mark contain swapfile of row

$ cat /etc/cni/net.d/100-crio-bridge.conf
{
    "cniVersion": "0.3.1",
    "name": "crio",
    "type": "bridge",
    "bridge": "cni0",
    "isGateway": true,
    "ipMasq": true,
    "hairpinMode": true,
    "ipam": {
        "type": "host-local",
        "routes": [
            { "dst": "0.0.0.0/0" },
            { "dst": "1100:200::1/24" }
        ],
        "ranges": [
            [{ "subnet": "10.85.0.0/16" }],
            [{ "subnet": "1100:200::/24" }]
        ]
    }
}
$ export CIDR=10.85.0.0/16
$ sudo kubeadm init --apiserver-advertise-address=192.168.122.160 --pod-network-cidr=$CIDR --cri-socket=/var/run/crio/crio.sock

$ mkdir -p $HOME/.kube
$ sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
$ sudo chown $(id -u):$(id -g) $HOME/.kube/config
```

输出：

```bash
Your Kubernetes control-plane has initialized successfully!

To start using your cluster, you need to run the following as a regular user:

  mkdir -p $HOME/.kube
  sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

You should now deploy a Pod network to the cluster.
Run "kubectl apply -f [podnetwork].yaml" with one of the options listed at:
  /docs/concepts/cluster-administration/addons/

You can now join any number of machines by running the following on each node
as root:

  kubeadm join <control-plane-host>:<control-plane-port> --token <token> --discovery-token-ca-cert-hash sha256:<hash>
```

要让 kubectl 为你的非 root 用户工作，请运行这些命令，这些命令也是 kubeadm init 输出的一部分：

```bash
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config
```

### 设置 KubeEdge 主节点

请参阅 [使用 Keadm 进行部署](https://kubeedge.io/zh/docs/setup/keadm/)。

重要提醒:

1. kubeconfig 或 master 中的至少一项必须配置正确，才能用于验证 k8s 集群的版本等信息。
2. 请确保边缘节点可以使用云节点的本地 IP 连接云节点，或者你需要使用 `--advertise-address` 标志指定云节点的公共 IP。
3. `--advertise-address`（仅 1.3 版本起有效）为云端公开的地址（将添加到 CloudCore 证书的 SAN 中），默认值为本地IP。

```bash
wget https://github.com/kubeedge/kubeedge/releases/download/v1.8.0/keadm-v1.8.0-linux-amd64.tar.gz
tar xzvf keadm-v1.8.0-linux-amd64.tar.gz
cd keadm-v1.8.0-linux-amd64/keadm/
sudo ./keadm init --advertise-address=192.168.122.160 --kube-config=/home/${user}/.kube/config
```

输出：

```bash
Kubernetes version verification passed, KubeEdge installation will start...
...
KubeEdge cloudcore is running, For logs visit:  /var/log/kubeedge/cloudcore.log
```

## 2. 设置边缘端（KubeEdge 工作节点）

你可以使用 CRI-O install.sh 脚本 `crun` 在 Ubuntu 20.04 上安装 CRI-O。

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/install.sh | bash
```

### 在工作节点安装 Go

```bash
$ wget https://golang.org/dl/go1.17.3.linux-amd64.tar.gz
$ tar xzvf go1.17.3.linux-amd64.tar.gz

$ export PATH=/home/${user}/go/bin:$PATH
$ go version
go version go1.17.3 linux/amd64
```

### 从云端获取令牌

在云端运行 `keadm gettoken` 会返回 token ，在加入边缘节点时使用。

```bash
$ sudo ./keadm gettoken --kube-config=/home/${user}/.kube/config
27a37ef16159f7d3be8fae95d588b79b3adaaf92727b72659eb89758c66ffda2.eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE1OTAyMTYwNzd9.JBj8LLYWXwbbvHKffJBpPd5CyxqapRQYDIXtFZErgYE
```

### 下载 Kubeedge 并加入边缘节点

请参阅 [使用 CRI 设置不同的容器运行时](https://kubeedge.io/zh/docs/advanced/cri/#cri-o) 和 [使用 Keadm 部署](https://kubeedge.io/zh/docs/setup/keadm/).

```bash
$ wget https://github.com/kubeedge/kubeedge/releases/download/v1.8.0/keadm-v1.8.0-linux-amd64.tar.gz
$ tar xzvf keadm-v1.8.0-linux-amd64.tar.gz
$ cd keadm-v1.8.0-linux-amd64/keadm/

$ sudo ./keadm join \
--cloudcore-ipport=192.168.122.160:10000 \
--edgenode-name=edge \
--token=b4550d45b773c0480446277eed1358dcd8a02a0c214646a8082d775f9c447d81.eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2Mzg4ODUzNzd9.A9WOYJFrgL2swVGnydpb4gMojyvyoNPCXaA4rXGowqU \
--remote-runtime-endpoint=unix:///var/run/crio/crio.sock \
--runtimetype=remote \
--cgroupdriver=systemd
```

输出：

```bash
Host has mosquit+ already installed and running. Hence skipping the installation steps !!!
...
KubeEdge edgecore is running, For logs visit:  /var/log/kubeedge/edgecore.log
```

### 从云端获取边缘节点状态

输出：

```bash
kubectl get node
NAME       STATUS    ROLES                  AGE   VERSION
edge       Ready     agent,edge             10s   v1.19.3-kubeedge-v1.8.2
master     Ready     control-plane,master   68m   v1.21.0
```

## 3. 启用 kubectl 日志功能

在部署 metrics-server 之前，必须激活 kubectl 日志功能，请[参阅这里](https://kubeedge.io/zh/docs/setup/keadm/#enable-kubectl-logs-feature).

## 4. 运行一个简单的 WebAssembly 应用

我们可以从 Kubernetes 集群中的 Docker Hub 运行基于 WebAssembly 的镜像。

### 云端

```bash
$ kubectl run -it --restart=Never wasi-demo --image=hydai/wasm-wasi-example:with-wasm-annotation --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000

Random number: -1694733782
Random bytes: [6, 226, 176, 126, 136, 114, 90, 2, 216, 17, 241, 217, 143, 189, 123, 197, 17, 60, 49, 37, 71, 69, 67, 108, 66, 39, 105, 9, 6, 72, 232, 238, 102, 5, 148, 243, 249, 183, 52, 228, 54, 176, 63, 249, 216, 217, 46, 74, 88, 204, 130, 191, 182, 19, 118, 193, 77, 35, 189, 6, 139, 68, 163, 214, 231, 100, 138, 246, 185, 47, 37, 49, 3, 7, 176, 97, 68, 124, 20, 235, 145, 166, 142, 159, 114, 163, 186, 46, 161, 144, 191, 211, 69, 19, 179, 241, 8, 207, 8, 112, 80, 170, 33, 51, 251, 33, 105, 0, 178, 175, 129, 225, 112, 126, 102, 219, 106, 77, 242, 104, 198, 238, 193, 247, 23, 47, 22, 29]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
/wasi_example_main.wasm
50000000
File content is This is in a file
```

WebAssembly 应用的 pod 成功部署到边缘节点。

```bash
$ kubectl describe pod wasi-demo

Name:         wasi-demo
Namespace:    default
Priority:     0
Node:         edge/192.168.122.229
Start Time:   Mon, 06 Dec 2021 15:45:34 +0000
Labels:       run=wasi-demo
Annotations:  module.wasm.image/variant: compat-smart
Status:       Succeeded
IP:           
IPs:          <none>
Containers:
  wasi-demo:
    Container ID:  cri-o://1ae4d0d7f671050331a17e9b61b5436bf97ad35ad0358bef043ab820aed81069
    Image:         hydai/wasm-wasi-example:with-wasm-annotation
    Image ID:      docker.io/hydai/wasm-wasi-example@sha256:525aab8d6ae8a317fd3e83cdac14b7883b92321c7bec72a545edf276bb2100d6
    Port:          <none>
    Host Port:     <none>
    Args:
      /wasi_example_main.wasm
      50000000
    State:          Terminated
      Reason:       Completed
      Exit Code:    0
      Started:      Mon, 06 Dec 2021 15:45:33 +0000
      Finished:     Mon, 06 Dec 2021 15:45:33 +0000
    Ready:          False
    Restart Count:  0
    Environment:    <none>
    Mounts:
      /var/run/secrets/kubernetes.io/serviceaccount from kube-api-access-bhszr (ro)
Conditions:
  Type           Status
  Initialized    True 
  Ready          False 
  PodScheduled   True 
Volumes:
  kube-api-access-bhszr:
    Type:                    Projected (a volume that contains injected data from multiple sources)
    TokenExpirationSeconds:  3607
    ConfigMapName:           kube-root-ca.crt
    ConfigMapOptional:       <nil>
    DownwardAPI:             true
QoS Class:                   BestEffort
Node-Selectors:              <none>
Tolerations:                 node.kubernetes.io/not-ready:NoExecute op=Exists for 300s
                             node.kubernetes.io/unreachable:NoExecute op=Exists for 300s
Events:
  Type    Reason     Age   From               Message
  ----    ------     ----  ----               -------
```

### 边缘端

```bash
$ sudo crictl ps -a
CONTAINER           IMAGE                                                                                           CREATED             STATE               NAME                ATTEMPT             POD ID
1ae4d0d7f6710       0423b8eb71e312b8aaa09a0f0b6976381ff567d5b1e5729bf9b9aa87bff1c9f3                                16 minutes ago      Exited              wasi-demo           0                   2bc2ac0c32eda
1e6c7cb6bc731       registry.k8s.io/kube-proxy@sha256:2a25285ff19f9b4025c8e54dac42bb3cd9aceadc361f2570489b8d723cb77135   18 minutes ago      Running             kube-proxy          0                   8b7e7388ad866
```

就是这样。

## 5. 运行示例的录屏

[![asciicast](https://asciinema.org/a/wkLOu6xnAOSAQdmYayumwrAvh.svg)](https://asciinema.org/a/wkLOu6xnAOSAQdmYayumwrAvh)
