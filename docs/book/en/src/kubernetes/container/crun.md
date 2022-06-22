# crun

The [crun project](https://github.com/containers/crun) has WasmEdge support baked in.
For now, the easiest approach is just built it yourself from source. First, let's make sure that `crun` dependencies are installed on your Ubuntu 20.04.
For other Linux distributions, please [see here](https://github.com/containers/crun#readme).

```bash
sudo apt update
sudo apt install -y make git gcc build-essential pkgconf libtool \
    libsystemd-dev libprotobuf-c-dev libcap-dev libseccomp-dev libyajl-dev \
    go-md2man libtool autoconf python3 automake
```

Next, configure, build, and install a `crun` binary with WasmEdge support.

```bash
git clone https://github.com/containers/crun
cd crun
./autogen.sh
./configure --with-wasmedge
make
sudo make install
```
