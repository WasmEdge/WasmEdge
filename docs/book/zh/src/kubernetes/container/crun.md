# crun

[crun 项目](https://github.com/containers/crun)已经内置了对 WasmEdge 的支持。
目前，最简单的方法是自己从源代码构建它。首先，让我们确保 crun 在你的 Ubuntu 20.04 上安装了依赖包。
对于其他 Linux 发行版，请[参见此处](https://github.com/containers/crun#readme)。

```bash
$ sudo apt update
$ sudo apt install -y make git gcc build-essential pkgconf libtool \
      libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
      go-md2man libtool autoconf python3 automake
```

接下来，配置、构建及安装一个支持 WasmEdge 的 crun 二进制文件。

```bash
git clone https://github.com/containers/crun
cd crun
./autogen.sh
./configure --with-wasmedge
make
sudo make install
```
