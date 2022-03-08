# Kubernetes

大多数高级容器运行时都实现了 Kubernetes 的 CRI（Container Runtime Interface）规范，以便能够被 Kubernetes 工具管理。这个意味着可以利用 Kubernetes 工具来管理 pod 和命名空间中 WebAssembly 应用程序镜像。

不同场景的 Kubernetes 设置具体说明如下所示：

* [Kubernetes + CRI-O](kubernetes/kubernetes-crio.md)
* [Kubernetes + containerd](kubernetes/kubernetes-containerd.md)
* [KubeEdge](kubernetes/kubeedge.md)
* [SuperEdge](kubernetes/superedge.md)
* [OpenYurt](kubernetes/openyurt.md)
