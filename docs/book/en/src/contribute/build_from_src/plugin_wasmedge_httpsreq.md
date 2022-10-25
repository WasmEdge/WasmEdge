# Build WasmEdge With WasmEdge-HttpsReq Plug-in

## Prerequisites

Currently, WasmEdge used `OpenSSL 1.1` or `3.0` for the the dependency of WasmEdge-HttpsReq.

For installing `OpenSSL 1.1` development package on `Ubuntu 20.04`, we recommend the following commands:

```bash
sudo apt update
sudo apt install -y libssl-dev
```

For legacy systems such as `CensOS 7.6`, or if you want to build `OpenSSL 1.1` from source, you can refer to the following commands:

```bash
# Download and extract the OpenSSL source to the current directory.
curl -s -L -O --remote-name-all https://www.openssl.org/source/openssl-1.1.1n.tar.gz
echo "40dceb51a4f6a5275bde0e6bf20ef4b91bfc32ed57c0552e2e8e15463372b17a openssl-1.1.1n.tar.gz" | sha256sum -c
tar -xf openssl-1.1.1n.tar.gz
cd ./openssl-1.1.1n
# OpenSSL configure need newer perl.
curl -s -L -O --remote-name-all https://www.cpan.org/src/5.0/perl-5.34.0.tar.gz
tar -xf perl-5.34.0.tar.gz
cd perl-5.34.0
mkdir localperl
./Configure -des -Dprefix=$(pwd)/localperl/
make -j
make install
export PATH="$(pwd)/localperl/bin/:$PATH"
cd ..
# Configure by previous perl.
mkdir openssl
./perl-5.34.0/localperl/bin/perl ./config --prefix=$(pwd)/openssl --openssldir=$(pwd)/openssl
make -j
make test
make install
cd ..
# The OpenSSL installation directory is at `$(pwd)/openssl-1.1.1n/openssl`.
# Then you can use the `-DOPENSSL_ROOT_DIR=` option of cmake to assign the directory.
```

> We'll soon update this chapter to use `OpenSSL 3.0`.

## Build WasmEdge with WasmEdge-HttpsReq Plug-in

To enable the WasmEdge WasmEdge-HttpsReq, developers need to [building the WasmEdge from source](linux.md) with the cmake option `-DWASMEDGE_PLUGIN_HTTPSREQ=On`.

```bash
cd <path/to/your/wasmedge/source/folder>
mkdir -p build && cd build
# For using self-get OpenSSL, you can assign the cmake option `-DOPENSSL_ROOT_DIR=<path/to/openssl>`.
cmake -DCMAKE_BUILD_TYPE=Release -DWASMEDGE_PLUGIN_HTTPSREQ=On .. && make -j
# For the WasmEdge-HttpsReq plugin, you should install this project.
cmake --install .
```

> If the built `wasmedge` CLI tool cannot find the WasmEdge-HttpsReq plug-in, you can set the `WASMEDGE_PLUGIN_PATH` environment variable to the plug-in installation path (`/usr/local/lib/wasmedge/`, or the built plug-in path `build/plugins/wasmedge_httpsreq/`) to try to fix this issue.

Then you will have an executable `wasmedge` runtime under `/usr/local/bin` and the WasmEdge-HttpsReq plug-in under `/usr/local/lib/wasmedge/libwasmedgePluginHttpsReq.so` after installation.
