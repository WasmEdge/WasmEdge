# OpenYurt + containerd + crun

In this article, we will introduce how to run a WasmEdge simple demo app with Containerd over [OpenYurt](https://github.com/openyurtio/openyurt).

## Set up an OpenYurt Cluster

Here, we introduce two ways to set up an OpenYurt Cluster.
The first one is to set up an OpenYurt Cluster from scratch, use `yurtctl convert` to realize a K8s Cluster conversion to an OpenYurt Cluster.
The second one is to use the ability of OpenYurt Experience Center, which is easy to achieve an OpenYurt Cluster.

### Prerequisite

|        | OS/kernel                            | Private IP/Public IP         |
| ------ | ------------------------------------ | ---------------------------- |
| Master | Ubuntu  20.04.3 LTS/5.4.0-91-generic | 192.168.3.169/120.55.126.18  |
| Node   | Ubuntu  20.04.3 LTS/5.4.0-91-generic | 192.168.3.170/121.43.113.152 |  

It should be noted that some steps may differ slightly depending on the operating system differences.
Please refer to the installation of [OpenYurt](https://github.com/openyurtio/openyurt) and [crun](https://github.com/containers/crun).

We use `yurtctl convert` to convert a K8s Cluster to OpenYurt Cluster, so we should set up a K8s Cluster.
If you use `yurtctl init/join` to set up an OpenYurt Cluster, you can skip this step which introduces the process of installing K8s.

Find the difference between `yurtctl convert/revert` and `yurtctl init/join`, you can refer to the following two articles.

[how use `Yurtctl init/join`](https://openyurt.io/docs/installation/yurtctl-init-join)

[Conversion between OpenYurt and Kubernetes:`yurtctl convert/revert`](https://openyurt.io/docs/installation/yurtctl-convert-revert)

- Close the swap space of the master and node firstly.

```bash
sudo swapoff -a
//verify
free -m
```

- Configure the file /etc/hosts of two nodes as the following.

```bash
192.168.3.169  oy-master 
120.55.126.18  oy-master
92.168.3.170   oy-node
121.43.113.152 oy-node
```

- Load the br_netfilter Kernel module and modify the Kernel parameter.

```bash
//load the module
sudo modprobe br_netfilter
//verify   
lsmod | grep br_netfilter
// create k8s.conf
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
EOF
sudo sysctl --system 
```

- Setup the value of rp-filter (adjusting the value of two parameters in `/etc/sysctl.d/10-network-security.conf` from 2 to 1 and setting up the value of /proc/sys/net/ipv4/ip_forward to 1)

```bash
sudo vi /etc/sysctl.d/10-network-security.conf
echo 1 > /proc/sys/net/ipv4/ip_forward
sudo sysctl --system
```

#### Install containerd and modify the default configure of containerd

Use the following commands to install containerd on your edge node which will run a WasmEdge simple demo.

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

As the crun project support WasmEdge as default, we just need to configure the containerd configuration for runc. So we need to modify the runc parameters in /etc/containerd/config.toml to curn and add pod_annotation.

```bash
sudo mkdir -p /etc/containerd/
sudo bash -c "containerd config default > /etc/containerd/config.toml"
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/containerd_config.diff
sudo patch -d/ -p0 < containerd_config.diff
```

After that, restart containerd to make the configuration take effect.

```bash
systemctl start containerd
```

#### Install WasmEdge

Use the [simple install script](../../../quick_start/install.md) to install WasmEdge on your edge node.

```bash
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

#### Build and install crun

We need a crun binary that supports WasmEdge on the edge node. For now, the most straightforward approach is to build it yourself from the source. First, let's ensure that crun dependencies are installed on your Ubuntu 20.04. For other Linux distributions, please see [here](https://github.com/containers/crun#readme).

- Dependencies are required for the build

```bash
sudo apt update
sudo apt install -y make git gcc build-essential pkgconf libtool \
  libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
  go-md2man libtool autoconf python3 automake
```

- Configure, build, and install a crun binary with WasmEdge support.

```bash
git clone https://github.com/containers/crun
cd crun
./autogen.sh
./configure --with-wasmedge
make
sudo make install
```

### From scratch set up an OpenYurt Cluster

In this demo, we will use two machines to set up an OpenYurt Cluster. One simulated cloud node is called Master, the other one simulated edge node is called Node. These two nodes form the simplest
OpenYurt Cluster, where OpenYurt components run on.

#### Set up a K8s Cluster

Kubernetes version 1.18.9

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

#### Install yurtctl

Use the following command line to install yurtctl. The yurtctl CLI tool helps install/uninstall OpenYurt and also convert a standard Kubernetes cluster to an OpenYurt cluster.

```bash
git clone https://github.com/openyurtio/openyurt.git
cd openyurt
make build WHAT=cmd/yurtctl
```

#### Install OpenYurt components

OpenYurt includes several components. YurtHub is the traffic proxy between the components on the node and Kube-apiserver. The YurtHub on the edge will cache the data returned from the cloud. Yurt controller supplements the upstream node controller to support edge computing requirements. TunnelServer connects with the TunnelAgent daemon running in each edge node via a reverse proxy to establish secure network access between the cloud site control plane and the edge nodes that are connected to the intranet. For more detailed information, you could refer to the [OpenYurt docs](https://github.com/openyurtio/openyurt).

```bash
yurtctl convert --deploy-yurttunnel --cloud-nodes oy-master --provider kubeadm\
--yurt-controller-manager-image="openyurt/yurt-controller-manager:v0.5.0"\
--yurt-tunnel-agent-image="openyurt/yurt-tunnel-agent:v0.5.0"\
--yurt-tunnel-server-image="openyurt/yurt-tunnel-server:v0.5.0"\
--node-servant-image="openyurt/node-servant:latest"\
--yurthub-image="openyurt/yurthub:v0.5.0"
```

We need to change the `openyurt/node-server-version` to latest here: `--node-servant-image="openyurt/node-servant:latest"`

Actually, OpenYurt components 0.6.0 version is recommended to be installed and proved to be a success to run a WasmEdge demo.
How to install OpenYurt:0.6.0, you can see [this](https://github.com/openyurtio/openyurt/releases/tag/v0.6.0)

### Use OpenYurt Experience Center to quickly set up an OpenYurt Cluster

An easier way to set up an OpenYurt Cluster is to use the OpenYurt Experience Center. All you need to do is to sign up for an account for testing, and then you will get an OpenYurt cluster. Next, you could just use `yurtctl join` command line to join an edge node. See more OpenYurt Experience Center details [here](https://openyurt.io/docs/installation/openyurt-experience-center/overview/).

## Run a simple WebAssembly app

Next, let's run a WebAssembly program through the OpenYurt cluster as a container in the pod. This section will start off pulling this WebAssembly-based container image from Docker hub. If you want to learn how to compile, package, and publish the WebAssembly program as a container image to Docker hub, please refer to [WasmEdge Book](../demo/wasi.md).

One thing is to note that because the kubectl run (version 1.18.9 ) missed annotations parameters, we need to adjust the command line here. If you are using OpenYurt Experience Center with OpenYurt 0.6.0 and Kubernetes 1.20.11 by default, please refer to [the Kubernetes sections](../kubernetes.md) in the WasmEdge book to run the wasm app.

```bash
// kubectl 1.18.9
$ sudo kubectl run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest  --overrides='{"kind":"Pod","metadata":{"annotations":{"module.wasm.image/variant":"compat-smart"}} , "apiVersion":"v1", "spec": {"hostNetwork": true}}' /wasi_example_main.wasm 50000000

// kubectl 1.20.11
$ sudo kubectl run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest --annotations="module.wasm.image/variant=compat-smart" --overrides='{"kind":"Pod", "apiVersion":"v1", "spec": {"hostNetwork": true}}' /wasi_example_main.wasm 50000000

```

The output from the containerized application is printed into the console. It is the same for all Kubernetes versions.

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

You can now check out the pod status through the Kubernetes command line.

```bash
crictl ps -a
```

You can see the events from scheduling to running the WebAssembly workload in the log.

```bash
CONTAINER           IMAGE               CREATED             STATE               NAME                 ATTEMPT             POD ID
0c176ed65599a       0423b8eb71e31       8 seconds ago       Exited              wasi-demo  
```
