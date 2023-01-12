# Kubernetes + CRI-O

## Quick start

The [GitHub repo](https://github.com/second-state/wasmedge-containers-examples/) contains scripts and Github Actions for running our example apps on Kubernetes + CRI-O.

* Simple WebAssembly example [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio.yml)
* WebAssembly-based HTTP service [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/kubernetes-crio-server.yml)

In the rest of this section, we will explain the steps in detail.
We will assume that you have already [installed and configured CRI-O](../cri/crio.md) to work with WasmEdge container images.

## Install and start Kubernetes

Run the following commands from a terminal window.
It sets up Kubernetes for local development.

```bash
# Install go
$ wget https://golang.org/dl/go1.17.1.linux-amd64.tar.gz
$ sudo rm -rf /usr/local/go
sudo tar -C /usr/local -xzf go1.17.1.linux-amd64.tar.gz
source /home/${USER}/.profile

# Clone k8s
git clone https://github.com/kubernetes/kubernetes.git
cd kubernetes
git checkout v1.22.2

# Install etcd with hack script in k8s
sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/install-etcd.sh
export PATH="/home/${USER}/kubernetes/third_party/etcd:${PATH}"
sudo cp third_party/etcd/etcd* /usr/local/bin/

# After run the above command, you can find the following files: /usr/local/bin/etcd  /usr/local/bin/etcdctl  /usr/local/bin/etcdutl

# Build and run k8s with CRI-O
sudo apt-get install -y build-essential
sudo CGROUP_DRIVER=systemd CONTAINER_RUNTIME=remote CONTAINER_RUNTIME_ENDPOINT='unix:///var/run/crio/crio.sock' ./hack/local-up-cluster.sh

... ...
Local Kubernetes cluster is running. Press Ctrl-C to shut it down.
```
  
Do NOT close your terminal window. Kubernetes is running!

## Run WebAssembly container images in Kubernetes

Finally, we can run WebAssembly programs in Kubernetes as containers in pods.
In this section, we will start from **another terminal window** and start using the cluster.

```bash
export KUBERNETES_PROVIDER=local

sudo cluster/kubectl.sh config set-cluster local --server=https://localhost:6443 --certificate-authority=/var/run/kubernetes/server-ca.crt
sudo cluster/kubectl.sh config set-credentials myself --client-key=/var/run/kubernetes/client-admin.key --client-certificate=/var/run/kubernetes/client-admin.crt
sudo cluster/kubectl.sh config set-context local --cluster=local --user=myself
sudo cluster/kubectl.sh config use-context local
sudo cluster/kubectl.sh
```

Let's check the status to make sure that the cluster is running.

```bash
$ sudo cluster/kubectl.sh cluster-info

# Expected output
Cluster "local" set.
User "myself" set.
Context "local" created.
Switched to context "local".
Kubernetes control plane is running at https://localhost:6443
CoreDNS is running at https://localhost:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.
```

### A simple WebAssembly app

[A separate article](../demo/wasi.md) explains how to compile, package, and publish a simple WebAssembly WASI program as a container image to Docker hub.
Run the WebAssembly-based image from Docker Hub in the Kubernetes cluster as follows.

```bash
sudo cluster/kubectl.sh run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```

The output from the containerized application is printed into the console.

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

### A WebAssembly-based HTTP service

[A separate article](../demo/server.md) explains how to compile, package, and publish a simple WebAssembly HTTP service application as a container image to Docker hub.
Since the HTTP service container requires networking support provided by Kubernetes, we will use a [k8s-http_server.yaml](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes_crio/http_server/k8s-http_server.yaml) file to specify its exact configuration.

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
    image: wasmedge/example-wasi-http:latest
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

Run the WebAssembly-based image from Docker Hub using the above `k8s-http_server.yaml` file in the Kubernetes cluster as follows.

```bash
sudo ./kubernetes/cluster/kubectl.sh apply -f k8s-http_server.yaml
```

Use the following command to see the running container applications and their IP addresses.
Since we are using `hostNetwork` in the yaml configuration, the HTTP server image is running on the local network with IP address `127.0.0.1`.

```bash
$ sudo cluster/kubectl.sh get pod --all-namespaces -o wide

NAMESPACE     NAME                       READY   STATUS             RESTARTS      AGE   IP          NODE        NOMINATED NODE   READINESS GATES
default       http-server                1/1     Running            1 (26s ago)     60s     127.0.0.1   127.0.0.1   <none>           <none>
```

Now, you can use the `curl` command to access the HTTP service.

```bash
$ curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```

That's it!
