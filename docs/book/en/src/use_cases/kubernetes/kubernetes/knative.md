# Knative

Knative is a platform-agnostic solution for running serverless deployments.

## Quick start

You can refer to [Kubernetes + containerd](./kubernetes-containerd.md) to build a kubernetes cluster. However, as the default runtime is replaced from runc to crun in this document, it is not suitable for existing k8s cluster.

Here we setup crun as a runtimeClass in kubernetes cluster, rather than replace the default runtime. Then deploy Knative serving service and run a WASM serverless service.

## Compile crun

Please refer to the document [crun](../container/crun.md)

```bash
# Install dependencies
$ sudo apt update
$ sudo apt install -y make git gcc build-essential pkgconf libtool \
    libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
    go-md2man libtool autoconf python3 automake

# Compile crun
$ git clone https://github.com/containers/crun
$ cd crun
$ ./autogen.sh
$ ./configure --with-wasmedge
$ make
$ sudo make install
```

## Install and setup Containerd

To make things easy, we use apt to install containerd. Here is the [document for ubuntu](https://docs.docker.com/engine/install/ubuntu/)
Once you have installed the containerd, edit the configuration /etc/containerd/config.toml

```bash
$ cat /etc/containerd/config.toml

# comment this line to make cri wokrs
# disabled_plugins = ["cri"]

# add the following section to setup crun runtime, make sure the BinaryName equal to your crun binary path
[plugins]
  [plugins.cri]
    [plugins.cri.containerd]
      [plugins.cri.containerd.runtimes]
...
        [plugins.cri.containerd.runtimes.crun]
           runtime_type = "io.containerd.runc.v2"
           pod_annotations = ["*.wasm.*", "wasm.*", "module.wasm.image/*", "*.module.wasm.image", "module.wasm.image/variant.*"]
           privileged_without_host_devices = false
           [plugins.cri.containerd.runtimes.crun.options]
             BinaryName = "/usr/local/bin/crun"
...

# restart containerd service
$ sudo systemctl restart containerd

# check if crun works
$ ctr image pull docker.io/wasmedge/example-wasi:latest
$ ctr run --rm --runc-binary crun --runtime io.containerd.runc.v2 --label module.wasm.image/variant=compat-smart docker.io/wasmedge/example-wasi:latest wasm-example /wasi_example_main.wasm 50000000
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

## Creating a cluster with kubeadm

Refering to the tree documents [Installing kubeadm](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/), [Creating a cluster with kubeadm](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/create-cluster-kubeadm/) and [Install flannel cni](https://github.com/flannel-io/flannel#deploying-flannel-manually), to create a kubernetes cluster.

```bash
# install kubeadm
$ sudo apt-get update
$ sudo apt-get install -y apt-transport-https ca-certificates curl
$ sudo curl -fsSLo /usr/share/keyrings/kubernetes-archive-keyring.gpg https://packages.cloud.google.com/apt/doc/apt-key.gpg
$ echo "deb [signed-by=/usr/share/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list
$ sudo apt-get update
$ sudo apt-get install -y kubelet kubeadm kubectl
$ sudo apt-mark hold kubelet kubeadm kubectl

# create kubernetes cluster
$ swapoff -a
$ kubeadm init --pod-network-cidr=10.244.0.0/16 --cri-socket unix:///var/run/containerd/containerd.sock

# install cni
$ kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# untaint master node
$ kubectl taint nodes --all node-role.kubernetes.io/control-plane-
$ export KUBECONFIG=/etc/kubernetes/admin.conf

# add crun runtimeClass
$ cat > runtime.yaml <<EOF
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: crun
handler: crun
EOF
$ kubectl apply runtime.yaml

# Verify if configuration works
$ kubectl run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest --annotations="module.wasm.image/variant=compat-smart" --overrides='{"kind":"Pod", "apiVersion":"v1", "spec": {"hostNetwork": true, "runtimeClassName": "crun"}}' /wasi_example_main.wasm 50000000
Random number: 1534679888
Random bytes: [88, 170, 82, 181, 231, 47, 31, 34, 195, 243, 134, 247, 211, 145, 28, 30, 162, 127, 234, 208, 213, 192, 205, 141, 83, 161, 121, 206, 214, 163, 196, 141, 158, 96, 137, 151, 49, 172, 88, 234, 195, 137, 44, 152, 7, 130, 41, 33, 85, 144, 197, 25, 104, 236, 201, 91, 210, 17, 59, 248, 80, 164, 19, 10, 46, 116, 182, 111, 112, 239, 140, 16, 6, 249, 89, 176, 55, 6, 41, 62, 236, 132, 72, 70, 170, 7, 248, 176, 209, 218, 214, 160, 110, 93, 232, 175, 124, 199, 33, 144, 2, 147, 219, 236, 255, 95, 47, 15, 95, 192, 239, 63, 157, 103, 250, 200, 85, 237, 44, 119, 98, 211, 163, 26, 157, 248, 24, 0]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
/wasi_example_main.wasm
50000000
File content is This is in a file
pod "wasi-demo" deleted
```

## Setting up Knative Serving

Refering to [Installing Knative Serving using YAML files](https://knative.dev/docs/install/yaml-install/serving/install-serving-with-yaml/), install the knative serving service.

```bash
# install the Knative Serving component
$ kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.7.2/serving-crds.yaml
$ kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.7.2/serving-core.yaml

# install a networking layer
$ kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v1.7.0/kourier.yaml
$ kubectl patch configmap/config-network \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"ingress-class":"kourier.ingress.networking.knative.dev"}}'
$ kubectl --namespace kourier-system get service kourier

# verify the installation
$ kubectl get pods -n knative-serving

# open runtimeClass feature gate in Knative
$ kubectl patch configmap/config-features -n knative-serving --type merge --patch '{"data":{"kubernetes.podspec-runtimeclassname":"enabled"}}'
```

## WASM cases in Knative Serving

Now we can try to run a WASM serverless service.

```bash
# apply the serverless service configuration
# we need setup annotations, runtimeClassName and ports.
$ cat > http-wasm-serverless.yaml <<EOF
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: http-wasm
  namespace: default
spec:
  template:
    metadata:
      annotations:
        module.wasm.image/variant: compat-smart
    spec:
      runtimeClassName: crun
      timeoutSeconds: 1
      containers:
      - name: http-server
        image: docker.io/wasmedge/example-wasi-http:latest
        ports:
        - containerPort: 1234
          protocol: TCP
        livenessProbe:
          tcpSocket:
            port: 1234
EOF

$ kubectl apply http-wasm-serverless.yaml

# wait for a while, check if the serverless service available
$ kubectl get ksvc http-wasm
NAME          URL                                              LATESTCREATED       LATESTREADY         READY   REASON
http-wasm     http://http-wasm.default.knative.example.com     http-wasm-00001     http-wasm-00001     True

# try to call the service
# As we do not setup DNS, so we can only call the service via Kourier, Knative Serving ingress port.
# get Kourier port which is 31997 in following example
$ kubectl --namespace kourier-system get service kourier
NAME      TYPE           CLUSTER-IP      EXTERNAL-IP       PORT(S)                      AGE
kourier   LoadBalancer   10.105.58.134                     80:31997/TCP,443:31019/TCP   53d
$ curl -H "Host: http-wasm.default.knative.example.com" -d "name=WasmEdge" -X POST http://localhost:31997

# check the new start pod
$ kubectl get pods
NAME                                           READY   STATUS    RESTARTS   AGE
http-wasm-00001-deployment-748bdc7cf-96l4r     2/2     Running   0          19s
```

