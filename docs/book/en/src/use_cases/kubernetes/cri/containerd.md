# containerd

## Quick start

The [GitHub repo](https://github.com/second-state/wasmedge-containers-examples/) contains scripts and Github Actions for running our example
apps on containerd.

* Simple WebAssembly example [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/containerd.yml)
* HTTP service example [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/containerd/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/containerd-server.yml)

In the sections below, we will explain the steps in the quick start scripts.

* [Install containerd](#install-containerd)
* [Example 1: Simple WebAssembly](#run-a-simple-webassembly-app)
* [Example 2: HTTP server in WebAssembly](#run-a-http-server-app)

## Install containerd

Use the following commands to install containerd on your system.

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

Configure containerd to use `crun` as the underlying OCI runtime.
It makes changes to the `/etc/containerd/config.toml` file.

```bash
sudo mkdir -p /etc/containerd/
sudo bash -c "containerd config default > /etc/containerd/config.toml"
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/containerd/containerd_config.diff
sudo patch -d/ -p0 < containerd_config.diff
```

Start the containerd service.

```bash
sudo systemctl start containerd
```

Next, make sure that you have [built and installed the `crun` binary with WasmEdge support](../container/crun.md) before running the following examples.

## Run a simple WebAssembly app

Now, we can run a simple WebAssembly program using containerd.
[A separate article](../demo/wasi.md) explains how to compile, package, and publish the WebAssembly
program as a container image to Docker hub.
In this section, we will start off pulling this WebAssembly-based container
image from Docker hub using containerd tools.

```bash
sudo ctr i pull docker.io/wasmedge/example-wasi:latest
```

Now, you can run the example in just one line with ctr (the containerd cli).

```bash
sudo ctr run --rm --runc-binary crun --runtime io.containerd.runc.v2 --label module.wasm.image/variant=compat-smart docker.io/wasmedge/example-wasi:latest wasm-example /wasi_example_main.wasm 50000000
```

Starting the container would execute the WebAssembly program. You can see the output in the console.

```bash
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

Next, you can try to run it in [Kubernetes](../kubernetes/kubernetes-containerd.md#a-simple-webassembly-app)!

## Run a HTTP server app

Finally, we can run a simple WebAssembly-based HTTP micro-service in containerd.
[A separate article](../demo/server.md) explains how to compile, package, and publish the WebAssembly
program as a container image to Docker hub.
In this section, we will start off pulling this WebAssembly-based container
image from Docker hub using containerd tools.

```bash
sudo ctr i pull docker.io/wasmedge/example-wasi-http:latest
```

Now, you can run the example in just one line with ctr (the containerd cli). Notice that we are running the container with `--net-host` so that the HTTP server inside the WasmEdge container is accessible from the outside shell.

```bash
sudo ctr run --rm --net-host --runc-binary crun --runtime io.containerd.runc.v2 --label module.wasm.image/variant=compat-smart docker.io/wasmedge/example-wasi-http:latest http-server-example /http_server.wasm
```

Starting the container would execute the WebAssembly program. You can see the output in the console.

```bash
new connection at 1234

# Test the HTTP service at that IP address
curl -d "name=WasmEdge" -X POST http://127.0.0.1:1234
echo: name=WasmEdge
```

Next, you can try to run it in [Kubernetes](../kubernetes/kubernetes-containerd.md#a-webassembly-based-http-service)!
