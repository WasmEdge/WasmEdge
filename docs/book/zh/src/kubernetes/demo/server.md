# HTTP 服务端实例

让我们构建一个基于 WebAssembly 的 HTTP 服务容器镜像。

这个 HTTP 服务应用程序是基于 Rust 开发，并采用了 [WasmEdge 网络 socket API](https://github.com/second-state/wasmedge_wasi_socket)。

Kubernetes 可以使用 CRI-O、Docker 和 Containerd 来管理 wasm 应用程序生命周期。

## 先决条件

这是一个 Rust 示例，它需要你先安装 [Rust](https://www.rust-lang.org/tools/install) 和 [WasmEdge](../../start/install.md) 以便可以编译运行 http 服务。

## 下载样例代码

```bash
mkdir http_server
cd http_server
wget -q https://raw.githubusercontent.com/second-state/wasmedge_wasi_socket/main/examples/http_server/Cargo.toml
mkdir src
cd src
wget -q https://raw.githubusercontent.com/second-state/wasmedge_wasi_socket/main/examples/http_server/src/main.rs
cd ../
```

## 构建 WASM 字节码

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

wasm 字节码应用程序现在应该位于 `./target/wasm32-wasi/release/http_server.wasm` 目录下。
你现在可以使用 wasmedge 测试运行它，然后将其发布为容器镜像。

## 在 Wasm 字节码上申请执行权限

```bash
chmod +x ./target/wasm32-wasi/release/http_server.wasm
```

## 使用 wasmedge 运行 http_server 应用程序字节码

当你在使用 wasmedge 来执行字节码并看到如下结果时，你已完成了将字节码打包到容器中的准备工作。

```bash
$ wasmedge ./target/wasm32-wasi/release/http_server.wasm
new connection at 1234
```

你可以在另一个的终端窗口中测试这个服务端。

```bash
$ curl -X POST http://127.0.0.1:1234 -d 'name=WasmEdge'
echo: name=WasmEdge
```

## 创建 Dockerfile

在 `target/wasm32-wasi/release` 文件夹中创建一个命名为 `Dockerfile` 的文件，其内容如下：

```dockerfile
FROM scratch
ADD http_server.wasm /
CMD ["/http_server.wasm"]
```

## 创建一个带注释的容器镜像

> 请注意，添加自定义注释仍然是 buildah 中的新功能。

`crun` 容器运行时可以启动上述基于 WebAssembly 的容器镜像。但它需要容器镜像上的 `module.wasm.image/variant=compat-smart` 注释来表明它是一个没有客人操作系统（安装在虚拟机上的系统）的 WebAssembly 应用程序。你可以在[官方 crun repo](https://github.com/containers/crun/blob/main/docs/wasm-wasi-example.md) 中找到详细信息。

要在容器镜像中添加 `module.wasm.image/variant=compat-smart` 注释，你需要最新的 [buildah](https://buildah.io/)。 目前，Docker 不支持此功能。请按照 [buildah 的安装说明](https://github.com/containers/buildah/blob/main/install.md) 构建最新的 buildah 二进制文件。

### 在 Ubuntu 上编译并安装最新的 buildah

在 Ubuntu zesty 和 xenial 上，使用这些命令为 buildah 做准备。

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

然后，按照下列步骤在 Ubuntu 上编译和安装 buildah。

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

### 创建和发布具备 buildah 的容器镜像

在 `target/wasm32-wasi/release/` 文件夹下，执行下列指令。

```bash
$ sudo buildah build --annotation "module.wasm.image/variant=compat-smart" -t http_server .

#
# make sure docker is install and running
# systemctl status docker
# to make sure regular user can use docker
# sudo usermod -aG docker $USER#
# newgrp docker

# You may need to use docker login to create the `~/.docker/config.json` for auth.
#
# docker login

$ sudo buildah push --authfile ~/.docker/config.json http_server docker://docker.io/avengermojo/http_server:with-wasm-annotation
```

好了！ 现在你可以尝试在 [CRI-O](../cri/crio.md#run-a-http-server-app) 或 [Kubernetes](../kubernetes/kubernetes.md#run-a-http-server-app) 上运行它了！
