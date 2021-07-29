# Install WasmEdge

There are several ways to install WasmEdge depending on your use cases. In this document, we will discuss how to install WasmEdge as a [CLI tool](run.md), a [serverless runtime](https://github.com/second-state/vercel-wasm-runtime), a SDK for languages like [C](c_api.md), [golang](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/) and [Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/), and a [runtime for Docker](https://www.secondstate.io/articles/manage-webassembly-apps-in-wasmedge-using-docker-tools/) and k8s tools.

## Use it from command line or in serverless runtimes

To use WasmEdge from the [command line](run.md) or from container-based serverless runtimes (e.g. [Vercel](https://github.com/second-state/vercel-wasm-runtime) or [Netlify](https://github.com/second-state/netlify-wasm-runtime)), you need to install the standalone WasmEdge virtual machine binary program on top of the operating system. You can get WasmEdge release package from its GitHub repository. The example below shows how to install WasmEdge on a typical x86-based Linux system. You can choose a package that meets your own operating system and CPU requirements.

```
$ curl -sSfL https://github.com/WasmEdge/WasmEdge/releases/download/0.8.2-rc.2/WasmEdge-0.8.2-rc.2-manylinux2014_x86_64.tar.gz -o ./WasmEdge.tar.gz
$ tar --strip-components 2 -xzvf WasmEdge.tar.gz WasmEdge-0.8.2-rc.2-Linux/bin
```

After installation, you will get two binary programs.

* `wasmedgec` is the AOT compiler that compiles WebAssembly bytecode programs (`wasm` programs) into native code (`so` program) on your deployment machine.
* `wasmedge` is the runtime that executes the `wasm` program or the AOT compiled `so` program.

However, in many cases, you need a WebAssembly runtime with [WasmEdge's extensions for imaging processing and tensorflow inference](https://www.secondstate.io/articles/wasi-tensorflow/) etc. You can install the `tensorflow` build of WasmEdge on your Linux system as follows.

```
$ curl -L https://github.com/second-state/WasmEdge-tensorflow-tools/releases/download/0.8.0/WasmEdge-tensorflow-tools-0.8.0-manylinux2014_x86_64.tar.gz -o ./WasmEdge-tensorflow-tools-0.8.0-manylinux2014_x86_64.tar.gz
$ tar xzvf WasmEdge-tensorflow-tools-0.8.0-manylinux2014_x86_64.tar.gz wasmedge-tensorflow-lite
$ tar xzvf WasmEdge-tensorflow-tools-0.8.0-manylinux2014_x86_64.tar.gz wasmedgec-tensorflow

$ curl -L https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/0.8.0/WasmEdge-tensorflow-deps-TFLite-0.8.0-manylinux2014_x86_64.tar.gz -o ./WasmEdge-tensorflow-deps-TFLite-0.8.0-manylinux2014_x86_64.tar.gz
$ tar xzvf WasmEdge-tensorflow-deps-TFLite-0.8.0-manylinux2014_x86_64.tar.gz
```

After installation, you will get two binary programs with the Tensorflow Lite (TFLite) dependency libraries.

* `wasmedgec-tensorflow` is the AOT compiler that compiles WebAssembly bytecode programs (`wasm` programs) into native code (`so` program) on your deployment machine. It is aware of WamsEdge's Tensorflow extension API.
* `wasmedge-tensorflow-lite` is the runtime that executes the `wasm` program or the AOT compiled `so` program with the Tensorflow Lite library.

Most public cloud serverless platforms use Linux-based Docker images as an extension mechanism. You can easily embed the WasmEdge CLI tool into those Docker containers to support WebAssembly-based functions.

## Use it as a language SDK

To embed WasmEdge in an application, you will need to install the WasmEdge library SDK. It can then be accessed via a [C API](c_api.md) or [golang API](https://www.secondstate.io/articles/extend-golang-app-with-webassembly-rust/). Below is how to do it in a Linux system.

```
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge.sh
$ chmod +x ./install_wasmedge.sh
$ sudo ./install_wasmedge.sh /usr/local
```

If you also need the [image processing and Tensorflow extensions](https://www.secondstate.io/articles/wasi-tensorflow/), it is a little more complex.

```
# Install WasmEdge 
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge.sh
$ chmod +x ./install_wasmedge.sh
$ sudo ./install_wasmedge.sh /usr/local

# Install WasmEdge Tensorflow extension
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_tensorflow_deps.sh
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_tensorflow.sh
$ chmod +x ./install_wasmedge_tensorflow_deps.sh
$ chmod +x ./install_wasmedge_tensorflow.sh
$ sudo ./install_wasmedge_tensorflow_deps.sh /usr/local
$ sudo ./install_wasmedge_tensorflow.sh /usr/local

# Install WasmEdge Images extension
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_image_deps.sh
$ wget https://github.com/second-state/WasmEdge-go/releases/download/v0.8.1/install_wasmedge_image.sh
$ chmod +x ./install_wasmedge_image_deps.sh
$ chmod +x ./install_wasmedge_image.sh
$ sudo ./install_wasmedge_image_deps.sh /usr/local
$ sudo ./install_wasmedge_image.sh /usr/local
```

[Here is a good example](https://www.secondstate.io/articles/yomo-wasmedge-real-time-data-streams/) on how to embed WasmEdge Tensorflow SDK into a golang application.

## Use it from Node.js

WasmEdge can run [WebAssembly functions emebedded in Node.js](https://www.secondstate.io/articles/getting-started-with-rust-function/) applications. To install the WasmEdge module in your Node.js environment is easy. Just use the `npm` tool.

```
$ npm install -g wasmedge-core # Append --unsafe-perm if permission denied
```

To install WasmEdge with [Tensorflow and other extensions](https://www.secondstate.io/articles/wasi-tensorflow/).

```
$ npm install -g wasmedge-extensions # Append --unsafe-perm if permission denied
```

The [Second State Functions](https://www.secondstate.io/faas/) is a WasmEdge-based FaaS service build on Node.js.

## Use it with Docker, CRI-O, and Kubernetes

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


