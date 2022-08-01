# Build and test WasmEdge on RISC-V 64 arch

Please follow this tutorial to make a RISC-V 64 filesystem with linux kernel, build WasmEdge with riscv-gnu-toolchain and test WasmEdge on the RISC-V64 system.

> Currently, we only support the runtime for the interpreter mode.

## Prepare the Environment

This tutorial is based on Ubuntu 21.10.

### Dependencies

We need to install the following dependencies to support subsequent work.

```bash
$ sudo apt install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev \
                 gawk build-essential bison flex texinfo gperf libtool patchutils bc \
                 zlib1g-dev libexpat-dev git \
                 libglib2.0-dev libfdt-dev libpixman-1-dev \
                 libncurses5-dev libncursesw5-dev
```

### Download and build riscv-gnu-toolchain

First,we get the riscv-gnu-toolchain source code.It's noted that before downloading the complete submodule, in order to save time, we delete the `qemu` submodule in advance.

```bash
$ git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git
$ cd riscv-gnu-toolchain
$ git rm qemu
$ git submodule update --init --recursive
```

Then we configure the installation path of the toolchain and build the toolchain.

```bash
$ ./configure --prefix=/opt/riscv64 
$ sudo make linux -j $(nproc)
```

### Download and build qemu

Get the qemu code and compile qemu. Laterly, we will use `qemu` to make the system image and emulate the RISC-V 64 environment.

```bash
$ wget https://download.qemu.org/qemu-7.0.0.tar.xz
$ tar xvJf qemu-7.0.0.tar.xz
$ cd qemu-7.0.0/
$ ./configure --target-list=riscv64-softmmu,riscv64-linux-user --prefix=/opt/qemu
$ make -j $(nproc)
$ sudo make install
```

### Download and compile linux kernel

Download the tar.gz archive of linux-v5.14 and rename the extracted folder to `linux`.

```bash
$ wget https://github.com/torvalds/linux/archive/refs/tags/v5.14.tar.gz
$ tar xzvf v5.14.tar.gz
$ mv linux-v5.14 linux
```

Configure the crosscompile toolchain and build linux kernel.

```bash
$ cd linux
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- defconfig
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j $(nproc)
```

### Download and compile busyboxsource

Download the busybox source code and open the configuration menu to change the settings.

```bash
$ git clone https://github.com/mirror/busybox.git
$ cd busybox
$ CROSS_COMPILE=riscv64-unknown-linux-gnu- make menuconfig
```

After opening the configuration menu, go to "Settings" on the first line, in the "Build Options" section, select "Build static binary (no shared libs)", and exit to save the configuration after setting.

```bash
$ CROSS_COMPILE=riscv64-unknown-linux-gnu- make -j $(nproc)
$ CROSS_COMPILE=riscv64-unknown-linux-gnu- make install
```

## Build WasmEdge 

### Get WasmEdge source code

```bash
$ git clone https://github.com/WasmEdge/WasmEdge.git
$ cd WasmEdge
```

### Compile

The riscv64-gnu-toolchain is installed in the `/opt/riscv64` fold, so we set the compiler path and the sysroot path as following. If the user installs the toolchain in another path, you can modify the compilation items according to the example.

```bash
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_BUILD_AOT_RUNTIME=OFF \
-DCMAKE_INSTALL_PREFIX:PATH=../_install \
-DCMAKE_CROSSCOMPILING=TRUE -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSROOT="/opt/riscv64/sysroot" \
-DCMAKE_C_COMPILER=/opt/riscv64/bin/riscv64-unknown-linux-gnu-gcc \
-DCMAKE_CXX_COMPILER=/opt/riscv64/bin/riscv64-unknown-linux-gnu-g++ \
-DCMAKE_FIND_ROOT_PATH=/opt/riscv64/riscv64-unknown-linux-gnu \
-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
$ make
$ make install
```

## Test

### Make a riscv-architecture filesystem

Make an image, allocate a disk file size of 1G, and format the disk file as ext4 file format.

```bash
(user@xxxxxx:/home/riscv64)$ ls
riscv-gnu-toolchain qemu-7.0.0 linux busybox WasmEdge
(user@xxxxxx:/home/riscv64)$ qemu-img create rootfs.img 1g
(user@xxxxxx:/home/riscv64)$ mkfs.ext4 rootfs.img
```

Mount the image locally, copy the previously generated files to the file system, and create some necessary files and directories. Then create a the `rcS` for init.

```bash
(user@xxxxxx:/home/riscv64)$ mkdir rootfs
(user@xxxxxx:/home/riscv64)$ sudo mount -o loop rootfs.img  rootfs
(user@xxxxxx:/home/riscv64/rootfs)$ cd rootfs
(user@xxxxxx:/home/riscv64/rootfs)$ sudo cp -r ../busybox/_install/* .
(user@xxxxxx:/home/riscv64/rootfs)$ sudo cp -r /opt/riscv64/sysroot/* .
(user@xxxxxx:/home/riscv64/rootfs)$ sudo mkdir proc sys dev etc etc/init.d
(user@xxxxxx:/home/riscv64/rootfs)$ cd etc/init.d/
(user@xxxxxx:/home/riscv64/rootfs/etc/init.d)$ sudo touch rcS
(user@xxxxxx:/home/riscv64/rootfs/etc/init.d)$ sudo vi rcS
```

Edit the `rcS` file and input the following contents:

```
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
/sbin/mdev -s
```

Then modify the rcS file permissions and add executable permissions, so that when busybox's init runs, the `/etc/init.d/rcS` script can be run.

```bash
(user@xxxxxx:/home/riscv64/rootfs/etc/init.d) sudo chmod +x rcS
```

Copy the generated WasmEdge and the test files to the file system.

```bash
(user@xxxxxx:/home/riscv64/rootfs/etc/init.d)$ cd ../../
(user@xxxxxx:/home/riscv64/rootfs)$ sudo cp -r ../WasmEdge/_install/* .
(user@xxxxxx:/home/riscv64/rootfs)$ sudo cp ../WasmEdge/examples/wasm/hello.wasm .
(user@xxxxxx:/home/riscv64/rootfs)$ sudo cp ../WasmEdge/examples/wasm/add.wasm .
```

Finally, exit the rootfs directory and unmount the filesystem.

```bash
(user@xxxxxx:/home/riscv64/rootfs)$ cd ..
(user@xxxxxx:/home/riscv64)$ sudo umount rootfs
```

### Test the wasmedge program

Execute the following command to start the system.

```bash
(user@xxxxxx:/home/riscv64)$ qemu-system-riscv64 -M virt -m 256M -nographic -kernel linux/arch/riscv/boot/Image -drive file=rootfs.img,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -append "root=/dev/vda rw console=ttyS0"
```

Test WasmEdge in the system by following command.

```bash
~ # wasmedge hello.wasm second state
hello
second
state
~ # wasmedge --reactor add.wasm add 2 2
4
```

The user can force quit QEMU by pressing Ctrl+A and then pressing X.

