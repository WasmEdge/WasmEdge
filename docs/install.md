# Install WasmEdge

* [Quickstart](#quickstart)
* [What's installed](#whats-installed)
* [Install WasmEdge for Node.js](#install-wasmedge-for-nodejs)
* [Install a Docker / k8s compatible WasmEdge "container"](#install-a-docker--k8s-compatible-wasmedge-container)

## Quickstart

The easiest way to install WasmEdge is to run the following commands.

```
$ wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
```

If you would like to install WasmEdge with its [Tensorflow and image processing extensions](https://www.secondstate.io/articles/wasi-tensorflow/), please run the following command. It will attempt to install Tensorflow and image shared libraries on your system.

```
$ wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash -s -- -e all
```

Source the environment using `source $HOME/.wasmedge/env`

**That's it!** You can now use WasmEdge from the [CLI](run.md), or launch it from an application. To update WasmEdge to a new release, just re-run the above command to write over the old files.

## What's installed

After installation, you have

* The `/usr/local/bin` directory contains the WasmEdge Runtime CLI executable files. You can copy and move them around on your file system.
  * The `wasmedge` tool is the standard WasmEdge runtime. You can use it from the CLI. `wasmedge --dir .:. app.wasm`
  * The `wasmedgec` tool is the AOT compiler to compile a `wasm` file into a native `so` file. `wasmedgec app.wasm app.so` The `wasmedge` can then execute the `so` file. `wasmedge --dir .:. app.so`
  * The `wasmedge-tensorflow`, `wasmedge-tensorflow-lite` and `wasmedgec-tensorflow` tools are runtimes and compilers that support the WasmEdge tensorflow SDK.
* The `/usr/local/lib` directory contains WasmEdge shared libraries, as well as dependency libraries. They are useful for WasmEdge SDKs to launch WasmEdge programs and functions from host applications.
* The `/usr/local/include` directory contains the WasmEdge header files. They are useful for WasmEdge SDKs.

To uninstall WasmEdge, you can run the following command.

```
$ wget https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh
$ chmod +x ./uninstall.sh
$ sudo ./uninstall.sh -p /usr/local -e all
```

It will remove WasmEdge files, and also print out dependency Tensorflow and image shared libraries for you to manually remove.


## Install WasmEdge for Node.js

WasmEdge can run [WebAssembly functions emebedded in Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) applications. To install the WasmEdge module in your Node.js environment is easy. Just use the `npm` tool.

```
$ npm install -g wasmedge-core # Append --unsafe-perm if permission denied
```

To install WasmEdge with [Tensorflow and other extensions](https://www.secondstate.io/articles/wasi-tensorflow/).

```
$ npm install -g wasmedge-extensions # Append --unsafe-perm if permission denied
```

The [Second State Functions](https://www.secondstate.io/faas/) is a WasmEdge-based FaaS service build on Node.js.


## Install a Docker / k8s compatible WasmEdge "container"

You can also use Docker tools, such as the CRI-O, to manage and execute WebAssembly programs as if they are Docker images. To do that, you need to install the [runw](https://github.com/second-state/runw) runtime into CRI-O. The runw already embeds a WasmEdge runtime so you do not need to install WasmEdge separately.

First, make sure that you are on Ubuntu 20.04 with LLVM-10 installed. If you are on a different platform, please refer to [the project documentation](https://github.com/second-state/runw#build-from-source) on how to build [runw](https://github.com/second-state/runw) for your OS.

```
$ sudo apt install -y llvm-10-dev liblld-10-dev
```

Also make sure that you have [cri-o](https://cri-o.io/), [crictl](https://github.com/kubernetes-sigs/cri-tools), [containernetworking-plugins](https://github.com/containernetworking/plugins), and [buildah](https://github.com/containers/buildah) or [docker](https://github.com/docker/cli) installed.

Next, download the runw binary build.

```
$ wget https://github.com/second-state/runw/releases/download/0.1.0/runw
```

Now, you can install [runw](https://github.com/second-state/runw) into CRI-O as an alternative runtime for WebAssembly.

```
# Get the wasm-pause utility
$ sudo crictl pull docker.io/beststeve/wasm-pause

# Install runw into cri-o
$ sudo cp -v runw /usr/lib/cri-o-runc/sbin/runw
$ sudo chmod +x /usr/lib/cri-o-runc/sbin/runw
$ sudo sed -i -e 's@default_runtime = "runc"@default_runtime = "runw"@' /etc/crio/crio.conf
$ sudo sed -i -e 's@pause_image = "k8s.gcr.io/pause:3.2"@pause_image = "docker.io/beststeve/wasm-pause"@' /etc/crio/crio.conf
$ sudo sed -i -e 's@pause_command = "/pause"@pause_command = "pause.wasm"@' /etc/crio/crio.conf
$ sudo tee -a /etc/crio/crio.conf.d/01-crio-runc.conf <<EOF
[crio.runtime.runtimes.runw]
runtime_path = "/usr/lib/cri-o-runc/sbin/runw"
runtime_type = "oci"
runtime_root = "/run/runw"
EOF
```

Finally, restart cri-o for the new WebAssembly runner to take effect.

```
sudo systemctl restart crio
```

[This article](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) shows a complete example on how to use CRI-O to manage a WebAssembly program.
