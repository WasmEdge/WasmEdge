# Kubernetes in Docker (KinD)

KinD is a Kubernetes distribution that runs inside Docker and is well suited for local development or integration testing. It runs containerd as CRI and runc as OCI Runtime.

## Quick start

As prerequisite we need to install KinD first. To do that the [quick start guide](https://kind.sigs.k8s.io/docs/user/quick-start/#installing-from-release-binaries) and the [release page](https://github.com/kubernetes-sigs/kind/releases) can be used to install the latest version of the KinD CLI.

If KinD is installed we can directly start with the example from [here](https://github.com/Liquid-Reply/kind-crun-wasm):

```bash
# Create a "WASM in KinD" Cluster
kind create cluster --image ghcr.io/liquid-reply/kind-crun-wasm:v1.23.0
# Run the example
kubectl run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```

In the rest of this section, we will explain how to create a KinD node image with wasmedge support.

## Build crun

KinD uses the `kindest/node` image for the control plane and worker nodes. The image contains containerd as CRI and runc as OCI Runtime. To enable WasmEdge support we replace `runc` with `crun`.

For the node image we only need the crun binary and not the entire build toolchain. Therefore we use a multistage dockerfile where we create crun in the first step and only copy the crun binary to the node image.

```Dockerfile
FROM ubuntu:21.10 AS builder
WORKDIR /data
RUN DEBIAN_FRONTEND=noninteractive apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y curl make git gcc build-essential pkgconf libtool libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev go-md2man libtool autoconf python3 automake \
    && curl https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -p /usr/local \
    && git clone --single-branch --branch feat/handler_per_container https://github.com/liquid-reply/crun \
    && cd crun \
    && ./autogen.sh \
    && ./configure --with-wasmedge --enable-embedded-yajl\
    && make 

...
```

Now we have a fresh `crun` binary with wasmedge enabled under `/data/crun/crun` that we can copy from this container in the next step.

## Replace crun and configure containerd

Both runc and crun implement the OCI runtime spec and they have the same CLI parametes. Therefore we can just replace the runc binary with our crun-wasmedge binary we created before.

Since crun is using some shared libraries we need to install libyajl, wasmedge and criu to make our crun work.

Now we already have a KinD that uses crun instead of runc. Now we just need two config changes. The first one in the `/etc/containerd/config.toml` where we add the `pod_annotations`that can be passed to the runtime:

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
  pod_annotations = ["*.wasm.*", "wasm.*", "module.wasm.image/*", "*.module.wasm.image", "module.wasm.image/variant.*"]
```

And the second one to the `/etc/containerd/cri-base.json` where we remove a hook that causes some issues.

The resulting dockerfile looks as follows:

```Dockerfile
...

FROM kindest/node:v1.23.0

COPY config.toml /etc/containerd/config.toml
COPY --from=builder /data/crun/crun /usr/local/sbin/runc
COPY --from=builder /usr/local/lib/libwasmedge.so /usr/local/lib/libwasmedge.so

RUN echo "Installing Packages ..." \
    && bash -c 'cat <<< $(jq "del(.hooks.createContainer)" /etc/containerd/cri-base.json) > /etc/containerd/cri-base.json' \
    && ldconfig
```

## Build and test

Finally we can build a new `node-wasmedge` image. To test it, we create a kind cluster from that image and run the simple app example.

```bash
docker build -t node-wasmedge .
kind create cluster --image node-wasmedge
# Now you can run the example to validate your cluster
kubectl run -it --rm --restart=Never wasi-demo --image=wasmedge/example-wasi:latest --annotations="module.wasm.image/variant=compat-smart" /wasi_example_main.wasm 50000000
```
