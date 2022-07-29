# 从源码在 OpenWrt 上构建和测试 WasmEdge

请参考这个教程来从源码在 OpenWrt(x86_64) 中构建和测试 WasmEdge.

> 目前，我们仅支持解释器模式的运行时。

## 环境准备

### OpenWrt 系统

首先，我们需要获取 OpenWrt 的源码，并安装相关的编译工具。以下命令以 Debian / Ubuntu 系统为例，其他系统安装 OpenWrt 编译工具的命令具体见 [构建 OpenWrt 系统设置](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem).

```bash
$ git clone https://github.com/openwrt/openwrt
$ sudo apt update
$ sudo apt install build-essential ccache ecj fastjar file g++ gawk \
gettext git java-propose-classpath libelf-dev libncurses5-dev \
libncursesw5-dev libssl-dev python python2.7-dev python3 unzip wget \
python-distutils-extra python3-setuptools python3-dev rsync subversion \
swig time xsltproc zlib1g-dev 
```

之后，获取 OpenWrt 最新的软件包定义，将所有软件包的安装符号链接下载到本地。

```bash
cd openwrt
./scripts/feeds update -a
./scripts/feeds install -a
```

## 构建 WasmEdge

### 获取 WasmEdge 源码

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

### 执行构建脚本

在 WasmEdge 源代码中运行构建脚本 `build_for_openwrt.sh`，传入 OpenWrt 源码的路径作为参数。此脚本将自动将 WasmEdge 加入 OpenWrt 的软件包列表，并编译 OpenWrt 固件，生成的 OpenWrt 镜像文件位于 `openwrt/bin/targets/x86/64` 文件夹中。

```bash
./utils/openwrt/build_for_openwrt.sh ~/openwrt
```

运行构建脚本时，会出现 OpenWrt 配置界面，在该界面中，我们需要将 `Target System` 设置为 x86，将 `Target Profile` 设置为 Generic x86/64，并在 `Runtime` 一栏中找到 `WasmEdge` 勾选上。设置完成后，脚本会自动构建 WasmEdge 并编译 OpenWrt 系统。

## 测试 WasmEdge

### 在 Vmware 中部署 OpenWrt

为了验证 WasmEdge 的可用性，我们使用 VMware 虚拟机来安装经过编译得到的 OpenWrt 镜像，创建虚拟机之前需要使用 `QEMU` 的命令将 OpenWrt 的镜像转换成 vmdk 格式。

```bash
cd ~/openwrt/bin/targets/x86/64
sudo apt install qemu
gunzip openwrt-x86-64-generic-squashfs-combined.img.gz
qemu-img convert -f raw -O vmdk openwrt-x86-64-generic-squashfs-combined.img Openwrt.vmdk
```

之后，在 VMware 中创建虚拟机，安装 OpenWrt 系统。

### 上传测试文件

根据主机的网关来设置 OpenWrt 的 ip 地址后，使用 `scp` 传输主机上的 wasm 文件到 OpenWrt 系统上。

例如，设置 OpenWrt 的 ip 地址为 192.168.0.111，则使用以下命令来上传 [hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.wasm) 和 [add.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/add.wasm) 测试文件到 OpenWrt。

```bash
scp hello.wasm root@192.168.0.111:/
scp add.wasm root@192.168.0.111:/
```

### 测试 wasmedge 程序

```bash
$ wasmedge hello.wasm second state
hello
second
state
$ wasmedge --reactor add.wasm add 2 2
4
```
