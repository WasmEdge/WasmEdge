# Knative

Knative 是一个构建在Kubernetes上的开源serverless架构方案。

## 快速开始

您可以参考文档 [Kubernetes + containerd](./kubernetes-containerd.md) 来创建一个Kubernetes集群。但是这个文档中默认容器运行时从runc替换成了crun。它可能不适用于已经创建好的k8s集群。

这篇文档中我们将crun设置位可选的runtimeClass,这样可以无入侵的将Wasm runtime加入到k8s集群。并且我们会部署Knative serving服务，并展示如何创建一个WASM serverless service.

## 编译crun

下面代码参考文档 [crun](../container/crun.md)

```bash
# 安装依赖
$ sudo apt update
$ sudo apt install -y make git gcc build-essential pkgconf libtool \
    libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
    go-md2man libtool autoconf python3 automake

# 编译crun
$ git clone https://github.com/containers/crun
$ cd crun
$ ./autogen.sh
$ ./configure --with-wasmedge
$ make
$ sudo make install
```

## 安装和配置containerd

为了使这个教程简单，我们直接使用apt安装containerd，文档在[document for ubuntu](https://docs.docker.com/engine/install/ubuntu/)。
安装好containerd后，编辑配置文件/etc/containerd/config.toml

```bash
$ cat /etc/containerd/config.toml

# 注释掉下面这一行让cri可以工作
# disabled_plugins = ["cri"]

# 在下面位置添加crun的配置，确保里面的BinaryName等于crun二进制文件的路径
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

# 重启containerd服务
$ sudo systemctl restart containerd

# 检查crun是否能正常运行WASM文件
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

## 用kubeadm创建kubernetes集群

参考下面文档 [安装kubeadm](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/), [使用kubeadm创建一个集群](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/create-cluster-kubeadm/) and [安装flannel CNI](https://github.com/flannel-io/flannel#deploying-flannel-manually), to create a kubernetes cluster.

```bash
# 安装kubeadm
$ sudo apt-get update
$ sudo apt-get install -y apt-transport-https ca-certificates curl
$ sudo curl -fsSLo /usr/share/keyrings/kubernetes-archive-keyring.gpg https://packages.cloud.google.com/apt/doc/apt-key.gpg
$ echo "deb [signed-by=/usr/share/keyrings/kubernetes-archive-keyring.gpg] https://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee /etc/apt/sources.list.d/kubernetes.list
$ sudo apt-get update
$ sudo apt-get install -y kubelet kubeadm kubectl
$ sudo apt-mark hold kubelet kubeadm kubectl

# 创建k8s集群
$ swapoff -a
$ kubeadm init --pod-network-cidr=10.244.0.0/16 --cri-socket unix:///var/run/containerd/containerd.sock

# 安装CNI
$ kubectl apply -f https://raw.githubusercontent.com/flannel-io/flannel/master/Documentation/kube-flannel.yml

# untaint master node
$ kubectl taint nodes --all node-role.kubernetes.io/control-plane-
$ export KUBECONFIG=/etc/kubernetes/admin.conf

# 添加 crun runtimeClass
$ cat > runtime.yaml <<EOF
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: crun
handler: crun
EOF
$ kubectl apply runtime.yaml

# 验证crun runtimeClass是否可以正常工作
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

## 设置Knative Serving

参考下面文档[Installing Knative Serving using YAML files](https://knative.dev/docs/install/yaml-install/serving/install-serving-with-yaml/)。

```bash
# 安装Knative Serving相关组件
$ kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.7.2/serving-crds.yaml
$ kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.7.2/serving-core.yaml

# 安装网络层
$ kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v1.7.0/kourier.yaml
$ kubectl patch configmap/config-network \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"ingress-class":"kourier.ingress.networking.knative.dev"}}'
$ kubectl --namespace kourier-system get service kourier

# 验证安装结果
$ kubectl get pods -n knative-serving

# 在knative中打开runtimeClass feature gate
$ kubectl patch configmap/config-features -n knative-serving --type merge --patch '{"data":{"kubernetes.podspec-runtimeclassname":"enabled"}}'
```

## 创建一个WASM serverless service

现在我们尝试创建一个WASM serverless service.

```bash
# WASM serverless service的配置文件
# 注意配置annotations, 指定runtimeClassName和端口
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

# 稍等一会，检查服务是否可用
$ kubectl get ksvc http-wasm
NAME          URL                                              LATESTCREATED       LATESTREADY         READY   REASON
http-wasm     http://http-wasm.default.knative.example.com     http-wasm-00001     http-wasm-00001     True

# 尝试请求服务
# 因为我们没有配置DNS，所以要通过Kourier, 也就是Knative Serving ingress port访问服务。
# 首先获得Kourier端口号，下面例子里端口号是31997.
$ kubectl --namespace kourier-system get service kourier
NAME      TYPE           CLUSTER-IP      EXTERNAL-IP       PORT(S)                      AGE
kourier   LoadBalancer   10.105.58.134                     80:31997/TCP,443:31019/TCP   53d
$ curl -H "Host: http-wasm.default.knative.example.com" -d "name=WasmEdge" -X POST http://localhost:31997

# 检查响应请求而新启动的pods
$ kubectl get pods
NAME                                           READY   STATUS    RESTARTS   AGE
http-wasm-00001-deployment-748bdc7cf-96l4r     2/2     Running   0          19s
```
