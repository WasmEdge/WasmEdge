# HTTP server example

Let's build a container image for a WebAssembly HTTP service.
The HTTP service application is developed in Rust using the [WasmEdge networking socket API](https://github.com/second-state/wasmedge_wasi_socket).
Kubernetes could manage the wasm application lifecycle with CRI-O, Docker and Containerd.

## Prerequisites

This is a Rust example, which require you to install [Rust](https://www.rust-lang.org/tools/install) and [WasmEdge](../../../quick_start/install.md) before you can Compile and Run the http service.

## Download example code

```bash
mkdir http_server
cd http_server
wget -q https://raw.githubusercontent.com/second-state/wasmedge_wasi_socket/main/examples/http_server/Cargo.toml
mkdir src
cd src
wget -q https://raw.githubusercontent.com/second-state/wasmedge_wasi_socket/main/examples/http_server/src/main.rs
cd ../
```

## Build the WASM bytecode

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

The wasm bytecode application is now should be located in the `./target/wasm32-wasi/release/http_server.wasm`
You can now test run it with wasmedge and then publish it as a container image.

## Apply executable permission on the Wasm bytecode

```bash
chmod +x ./target/wasm32-wasi/release/http_server.wasm
```

## Running the http_server application bytecode with wasmedge

When you run the bytecode with wasmedge and see the result as the following, you are ready to package the bytecode into the container.

```bash
$ wasmedge ./target/wasm32-wasi/release/http_server.wasm
new connection at 1234
```

You can test the server from another terminal window.

```bash
$ curl -X POST http://127.0.0.1:1234 -d 'name=WasmEdge'
echo: name=WasmEdge
```

## Create Dockerfile

Create a file called `Dockerfile` in the `target/wasm32-wasi/release/` folder with the following content:

```dockerfile
FROM scratch
ADD http_server.wasm /
CMD ["/http_server.wasm"]
```

## Create container image with annotations

> Please note that adding self-defined annotation is still a new feature in buildah.

The `crun` container runtime can start the above WebAssembly-based container image. But it requires the `module.wasm.image/variant=compat-smart` annotation on the container image to indicate that it is a WebAssembly application without a guest OS. You can find the details in [Official crun repo](https://github.com/containers/crun/blob/main/docs/wasm-wasi-example.md).

To add `module.wasm.image/variant=compat-smart` annotation in the container image, you will need the latest [buildah](https://buildah.io/). Currently, Docker does not support this feature. Please follow [the install instructions of buildah](https://github.com/containers/buildah/blob/main/install.md) to build the latest buildah binary.

### Build and install the latest buildah on Ubuntu

On Ubuntu zesty and xenial, use these commands to prepare for buildah.

```bash
sudo apt-get -y install software-properties-common

export OS="xUbuntu_20.04"
sudo bash -c "echo \"deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/ /\" > /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list"
sudo bash -c "curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/Release.key | apt-key add -"

sudo add-apt-repository -y ppa:alexlarsson/flatpak
sudo apt-get -y -qq update

sudo apt-get -y install bats git libapparmor-dev libdevmapper-dev libglib2.0-dev libgpgme-dev libseccomp-dev libselinux1-dev skopeo-containers go-md2man containers-common
sudo apt-get -y install golang-1.16 make
```

Then, follow these steps to build and install buildah on Ubuntu.

```bash
mkdir -p ~/buildah
cd ~/buildah
export GOPATH=`pwd`
git clone https://github.com/containers/buildah ./src/github.com/containers/buildah
cd ./src/github.com/containers/buildah
PATH=/usr/lib/go-1.16/bin:$PATH make
cp bin/buildah /usr/bin/buildah
buildah --help
```

### Create and publish a container image with buildah

In the `target/wasm32-wasi/release/` folder, do the following.

```bash
sudo buildah build --annotation "module.wasm.image/variant=compat-smart" -t http_server .

#
# make sure docker is install and running
# systemctl status docker
# to make sure regular user can use docker
# sudo usermod -aG docker $USER#
# newgrp docker

# You may need to use docker login to create the `~/.docker/config.json` for auth.
#
# docker login

sudo buildah push --authfile ~/.docker/config.json http_server docker://docker.io/wasmedge/example-wasi-http:latest
```

That's it! Now you can try to run it in [CRI-O](../cri/crio.md#run-a-http-server-app) or [Kubernetes](../kubernetes/kubernetes-crio.md#a-webassembly-based-http-service)!
