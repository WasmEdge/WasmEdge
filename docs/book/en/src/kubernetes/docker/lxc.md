# Use the slim Linux container

An easy way to run WebAssembly applications in the Docker ecosystem is to simply embed the WebAssembly bytecode file in a Linux container image. Specifically, we trim down the Linux OS inside the container to the point where it is just enough to support the `wasmedge` runtime. This approach has many advantages.

* It works seamlessly with any tool in the Docker or container ecosystem since the WebAssembly application is wrapped in a regular container.
* The memory footprint of the entire image of Linux OS and WasmEdge can be reduced to as low as 10MB.
* The attack surface of the slimmed Linux OS is dramatically reduced from a regular Linux OS.
* The overall application security is managed by the WebAssembly sandbox. The risk for software supply chain attack is greatly reduced since the WebAssembly sandbox only has access to explicitly declared capabilities.
* The above three advantages are amplified if the application is complex. For example, a WasmEdge AI inference application would NOT require a Python install. A WasmEdge node.js application would NOT require a Node.js and v8 install.

However, this approach still requires starting up a Linux container. The performance and security of this approach would not be as great as running WebAssembly applications directly in [crun](../container/crun.md) or in a [containerd shim]().

## Run a simple WebAssembly app

We can run a simple WebAssembly program using Docker. The sample application is here. First, create a Docker file based on our release image. Include the application file in the new image, and run the `wasmedge` command at start up.

```shell
FROM wasmedge/wasmedge:release-0.10.1
ADD wasi_example_main.wasm /
CMD ["wasmedge /wasi_example_main.wasm"]
```

Running the WebAssembly application in Docker CLI as follows.

```shell
xxx
```

## Run a HTTP server app

We can run a simple WebAssembly-based HTTP micro-service using the Docker CLI. The sample application is here. First, create a Docker file based on our release image. Include the application file in the new image, and run the `wasmedge` command at start up.

```shell
FROM wasmedge/wasmedge:release-0.10.1
ADD http_server.wasm /
CMD ["wasmedge /http_server.wasm"]
```

Running the WebAssembly server application in Docker CLI as follows. Notice that we map the server port from the container to the host.

```shell
xxx
```

You can now access the server from another terminal.

```shell
xxx
```



