# Build and test WasmEdge for OpenWrt

Please follow this tutorial to build and test WasmEdge in OpenWrt(x86_64) from source code.

> Currently, we only support the runtime for the interpreter mode.

## Prepare the Environment

### OpenWrt

First, we need to obtain the source code of OpenWrt and install the relevant tools to compile OpenWrt. The following commands take Debian / Ubuntu system as an example. For commands to install OpenWrt compilation tools in other host systems, see [Building OpenWrt System Settings](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem).

```bash
$ git clone https://github.com/openwrt/openwrt
$ sudo apt update
$ sudo apt install build-essential ccache ecj fastjar file g++ gawk \
gettext git java-propose-classpath libelf-dev libncurses5-dev \
libncursesw5-dev libssl-dev python python2.7-dev python3 unzip wget \
python-distutils-extra python3-setuptools python3-dev rsync subversion \
swig time xsltproc zlib1g-dev 
```

Then, obtain all the latest package definitions of OpenWrt and install the symlinks for all obtained packages.

```bash
cd openwrt
./scripts/feeds update -a
./scripts/feeds install -a
```

## Build WasmEdge

### Get WasmEdge source code

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

### Run the build script

Run the build script `build_for_openwrt.sh` in WasmEdge source code, and input the path of the OpenWrt source code as parameter. This script will automatically add the WasmEdge into the packages list which will be built of OpenWrt, and build the OpenWrt firmware. The generated OpenWrt images are in the `openwrt/bin/targets/x86/64` folder.

```bash
./utils/openwrt/build_for_openwrt.sh ~/openwrt
```

When running the build script, the OpenWrt configuration interface will appear. In this interface, we need to set `Target System` to x86, `Target Profile` to Generic x86/64, and find `WasmEdge` in the `Runtime` column and check it . Once set up, the script automatically builds WasmEdge and compiles the OpenWrt system.

## Test

### Deploy OpenWrt in VMware

In order to verify the availability of WasmEdge, we use a VMware virtual machine to install the compiled OpenWrt image. Before creating a virtual machine, we need to use the `QEMU` command to convert the OpenWrt image to vmdk format.

```bash
cd ~/openwrt/bin/targets/x86/64
sudo apt install qemu
gunzip openwrt-x86-64-generic-squashfs-combined.img.gz
qemu-img convert -f raw -O vmdk openwrt-x86-64-generic-squashfs-combined.img Openwrt.vmdk
```

After that, create a virtual machine in VMware and install the OpenWrt system.

### upload the test files

After setting the IP address of OpenWrt according to the gateway of the host, use `scp` to transfer the wasm file on the host to the OpenWrt system.

For example, we set the ip address of OpenWrt as 192.168.0.111, then we use the following commands to upload [hello.wasm](https://github.com/WasmEdge/WasmEdge/raw/master/examples/wasm/hello.wasm) and [add.wasm](https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/examples/wasm/add.wasm) these two test files to OpenWrt.

```bash
scp hello.wasm root@192.168.0.111:/
scp add.wasm root@192.168.0.111:/
```

### Test the wasmedge program

```bash
$ wasmedge hello.wasm second state
hello
second
state
$ wasmedge --reactor add.wasm add 2 2
4
```
