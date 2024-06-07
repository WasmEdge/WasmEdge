# A simple WebAssembly example

In this article, I will show you how to build a container image for a WebAssembly application. It can then be started and managed by Kubernetes ecosystem tools, such as CRI-O, Docker, crun, and Kubernetes.

## Prerequisites

> If you simply want a wasm bytecode file to test as a container image, you can skip the building process and just [download the wasm file here](https://github.com/second-state/wasm-learning/blob/master/cli/wasi/wasi_example_main.wasm).

If you have not done so already, follow these simple instructions to [install Rust](https://www.rust-lang.org/tools/install).

## Download example code

```bash
git clone https://github.com/second-state/wasm-learning
cd wasm-learning/cli/wasi
```

## Build the WASM bytecode

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

The wasm bytecode application is in the `target/wasm32-wasi/release/wasi_example_main.wasm` file. You can now publish and use it as a container image.

## Apply executable permission on the Wasm bytecode

```bash
chmod +x target/wasm32-wasi/release/wasi_example_main.wasm
```

## Create Dockerfile

Create a file called `Dockerfile` in the `target/wasm32-wasi/release/` folder with the following content:

```dockerfile
FROM scratch
ADD wasi_example_main.wasm /
CMD ["/wasi_example_main.wasm"]
```

### Create and publish a container image with buildah

In the `target/wasm32-wasi/release/` folder, do the following.

```bash
$ sudo buildah build -t wasm-wasi-example .
# make sure docker is install and running
# systemctl status docker
# to make sure regular user can use docker
# sudo usermod -aG docker $USER
# newgrp docker

# You may need to use docker login to create the `~/.docker/config.json` for auth.
$ sudo buildah push --authfile ~/.docker/config.json wasm-wasi-example docker://docker.io/wasmedge/example-wasi:latest
```

That's it! Now you can try to run it in [CRI-O](../cri/crio.md#run-a-simple-webassembly-app) or [Kubernetes](../kubernetes/kubernetes-crio.md#a-simple-webassembly-app)!
