# Use the slim Linux container

An easy way to run WebAssembly applications in the Docker ecosystem is to simply embed the WebAssembly bytecode file in a Linux container image. Specifically, we trim down the Linux OS inside the container to the point where it is just enough to support the `wasmedge` runtime. This approach has many advantages.

* It works seamlessly with any tool in the Docker or container ecosystem since the WebAssembly application is wrapped in a regular container.
* The memory footprint of the entire image of Linux OS and WasmEdge can be reduced to as low as 4MB.
* The attack surface of the slimmed Linux OS is dramatically reduced from a regular Linux OS.
* The overall application security is managed by the WebAssembly sandbox. The risk for software supply chain attack is greatly reduced since the WebAssembly sandbox only has access to explicitly declared capabilities.
* The above three advantages are amplified if the application is complex. For example, a WasmEdge AI inference application would NOT require a Python install. A WasmEdge node.js application would NOT require a Node.js and v8 install.

However, this approach still requires starting up a Linux container. The containerized Linux OS, however slim, still takes 80% of the total image size. There is still a lot of room for optimization. The performance and security of this approach would not be as great as running WebAssembly applications directly in [crun](../container/crun.md) or in a [containerd shim](containerd.md).

## Run a simple WebAssembly app

We can run a simple WebAssembly program using Docker. An slim Linux image with WasmEdge installed is only 4MB as opposed to 30MB for a general Linux image for native compiled applications. The Linux + WasmEdge image is similar to a unikernel OS image. It minimizes the footprint, performance overhead, and potential attack surface for WebAssembly applications.

[The sample application is here](https://github.com/second-state/wasm-learning/tree/master/cli/wasi). First, create a `Dockerfile` based on our release image. Include the [wasm application file](https://github.com/second-state/wasm-learning/raw/master/cli/wasi/wasi_example_main.wasm) in the new image, and run the `wasmedge` command at start up.

```shell
FROM wasmedge/slim-runtime:0.10.1
ADD wasi_example_main.wasm /
CMD ["wasmedge", "--dir", ".:/", "/wasi_example_main.wasm"]
```

Run the WebAssembly application in Docker CLI as follows.

```shell
$ docker build -t wasmedge/myapp -f Dockerfile ./
... ...
Successfully tagged wasmedge/myapp:latest

$ docker run --rm wasmedge/myapp
Random number: -807910034
Random bytes: [113, 123, 78, 85, 63, 124, 68, 66, 151, 71, 91, 249, 242, 160, 164, 133, 35, 209, 106, 143, 202, 87, 147, 87, 236, 49, 238, 165, 125, 175, 172, 114, 136, 205, 200, 176, 30, 122, 149, 21, 39, 58, 221, 102, 165, 179, 124, 13, 60, 166, 188, 127, 83, 95, 145, 0, 25, 209, 226, 190, 10, 184, 139, 191, 243, 149, 197, 85, 186, 160, 162, 156, 181, 74, 255, 99, 203, 161, 108, 153, 90, 155, 247, 183, 106, 79, 48, 255, 172, 17, 193, 36, 245, 195, 170, 202, 119, 238, 104, 254, 214, 227, 149, 20, 8, 147, 105, 227, 114, 146, 246, 153, 251, 139, 130, 1, 219, 56, 228, 154, 146, 203, 205, 56, 27, 115, 79, 254]
Printed from wasi: This is from a main function
This is from a main function
The env vars are as follows.
The args are as follows.
wasi_example_main.wasm
File content is This is in a file
```

## Run a HTTP server app

We can run a simple WebAssembly-based HTTP micro-service using the Docker CLI. The [sample application is here](https://github.com/second-state/wasmedge_wasi_socket/tree/main/examples/http_server). Follow instructions to compile and build the `http_server.wasm` file.

Create a `Dockerfile` based on our release image. Include the `http_server.wasm` application file in the new image, and run the `wasmedge` command at start up.

```shell
FROM wasmedge/slim-runtime:0.10.1
ADD http_server.wasm /
CMD ["wasmedge", "--dir", ".:/", "/http_server.wasm"]
```

Run the WebAssembly server application in Docker CLI as follows. Notice that we map the server port from the container to the host.

```shell
$ docker build -t wasmedge/myapp -f Dockerfile ./
... ...
Successfully tagged wasmedge/myapp:latest

$ docker run --rm -p 1234:1234 wasmedge/myapp
new connection at 1234
```

You can now access the server from another terminal.

```shell
$ curl -X POST http://127.0.0.1:1234 -d "name=WasmEdge"
echo: name=WasmEdge
```

## Run a lightweight Node.js server

With WasmEdge QuickJS support for the Node.js API, we can run a lightweight and secure node.js server from Docker CLI. The slim Linux + WasmEdge + Node.js support image size is less than 15MB as opposed to over 350MB for a standard Node.js image. You will need to do the following.

* [Download the WasmEdge QuickJS runtime](https://github.com/second-state/wasmedge-quickjs/releases/download/v0.4.0-alpha/wasmedge_quickjs.wasm) here. You will have the `wasmedge_quickjs.wasm` file.
* [Download the modules](https://github.com/second-state/wasmedge-quickjs/tree/main/modules) directory from the WasmEdge QuickJS repo.
* Create a JavaScript file for the server. Below is an example `http_echo.js` file you can use.

```javascript
import { createServer, request, fetch } from 'http';

createServer((req, resp) => {
  req.on('data', (body) => {
    resp.write('echo:')
    resp.end(body)
  })
}).listen(8001, () => {
  print('listen 8001 ...\n');
})
```

Add those files to the Docker image and run the JavaScript file at startup.

```shell
FROM wasmedge/slim-runtime:0.10.1
ADD wasmedge_quickjs.wasm /
ADD http_echo.js /
ADD modules /modules
CMD ["wasmedge", "--dir", ".:/", "/wasmedge_quickjs.wasm", "http_echo.js"]
```

Start the server from Docker CLI.

```shell
$ docker build -t wasmedge/myapp -f Dockerfile ./
... ...
Successfully tagged wasmedge/myapp:latest

$ docker run --rm -p 8001:8001 wasmedge/myapp
listen 8001 ...
```

You can now access the server from another terminal.

```shell
$ curl -X POST http://127.0.0.1:8001 -d "WasmEdge"
echo:WasmEdge
```

## Run a lightweight Tensorflow inference application

A unique and powerful feature of the WasmEdge runtime is its support for AI frameworks. In this example, we will show you how to run an image recognition service from Docker CLI. [The sample application is here](https://github.com/WasmEdge/wasmedge_hyper_demo/tree/main/server-tflite). First, create a `Dockerfile` based on our `tensorflow` release image. Include the [wasm application file](https://github.com/WasmEdge/wasmedge_hyper_demo/raw/main/server-tflite/wasmedge_hyper_server_tflite.wasm) in the new image, and run the `wasmedge-tensorflow-lite` command at start up.

The Dockerfile is as follows. The whole package is 115MB. It is less than 1/4 of a typically Linux + Python + Tensorflow setup.

```shell
FROM wasmedge/slim-tf:0.10.1
ADD wasmedge_hyper_server_tflite.wasm /
CMD ["wasmedge-tensorflow-lite", "--dir", ".:/", "/wasmedge_hyper_server_tflite.wasm"]
```

Start the server from Docker CLI.

```shell
$ docker build -t wasmedge/myapp -f Dockerfile ./
... ...
Successfully tagged wasmedge/myapp:latest

$ docker run --rm -p 3000:3000 wasmedge/myapp
listen 3000 ...
```

You can now access the server from another terminal.

```shell
$ curl http://localhost:3000/classify -X POST --data-binary "@grace_hopper.jpg"
military uniform is detected with 206/255 confidence
```
