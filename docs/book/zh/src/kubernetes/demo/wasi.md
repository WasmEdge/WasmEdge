# 一个简单的 WASI 例子

在这篇文章中，我会向你展示如何去构建一个 WebAssemblely 应用的容器镜像。然后可以通过 Kubernetes 生态系统工具（例如 CRI-O、Docker、crun 和 Kubernetes）来启动和管理它。

## 先决条件

> 如果你只是想要一个 wasm 字节码文件作为容器镜像进行测试，你可以跳过构建步骤，只需[在此处下载 wasm 文件](https://github.com/second-state/wasm-learning/blob/master/cli/wasi/wasi_example_main.wasm)。

首先，请按照这些简单的说明来[安装 Rust](https://www.rust-lang.org/tools/install)。

## 下载样例代码

```bash
git clone https://github.com/second-state/wasm-learning
cd wasm-learning/cli/wasi
```

## 构建 WASM 字节码

```bash
rustup target add wasm32-wasi
cargo build --target wasm32-wasi --release
```

wasm 字节码应用程序位于 `target/wasm32-wasi/release/wasi_example_main.wasm` 文件中。你现在可以将其发布并用作容器镜像。

## 申请 Wasm 字节码的执行权限

```bash
chmod +x target/wasm32-wasi/release/wasi_example_main.wasm
```

## 创建 Dockerfile

在 `target/wasm32-wasi/release` 文件夹中创建一个命名为 `Dockerfile` 的文件，其内容如下：

```dockerfile
FROM scratch
ADD wasi_example_main.wasm /
CMD ["/wasi_example_main.wasm"]
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
$ sudo buildah build --annotation "module.wasm.image/variant=compat-smart" -t wasm-wasi-example .
# make sure docker is install and running
# systemctl status docker
# to make sure regular user can use docker
# sudo usermod -aG docker $USER
# newgrp docker

# You may need to use docker login to create the `~/.docker/config.json` for auth.
$ sudo buildah push --authfile ~/.docker/config.json wasm-wasi-example docker://docker.io/hydai/wasm-wasi-example:with-wasm-annotation
```

好了！ 现在你可以尝试在 [CRI-O](../cri/crio.md#run-a-http-server-app) 或 [Kubernetes](../kubernetes/kubernetes.md#run-a-http-server-app) 上运行它了！
