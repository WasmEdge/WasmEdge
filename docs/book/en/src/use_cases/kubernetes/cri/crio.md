# CRI-O

## Quick start

The [GitHub repo](https://github.com/second-state/wasmedge-containers-examples/) contains scripts and Github Actions for running our example
apps on CRI-O.

* Simple WebAssembly example [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/crio.yml)
* HTTP service example [Quick start](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/http_server/README.md) | [Github Actions](https://github.com/second-state/wasmedge-containers-examples/blob/main/.github/workflows/crio-server.yml)

In the sections below, we will explain the steps in the quick start scripts.

* [Install CRI-O](#install-cri-o)
* [Configure CRI-O and crun](#configure-cri-o-to-use-crun)
* [Example 1: Simple WebAssembly](#run-a-simple-webassembly-app)
* [Example 2: HTTP server in WebAssembly](#run-a-http-server-app)

## Install CRI-O

Use the following commands to install CRI-O on your system.

```bash
export OS="xUbuntu_20.04"
export VERSION="1.21"
apt update
apt install -y libseccomp2 || sudo apt update -y libseccomp2
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/ /" > /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/$VERSION/$OS/ /" > /etc/apt/sources.list.d/devel:kubic:libcontainers:stable:cri-o:$VERSION.list

curl -L https://download.opensuse.org/repositories/devel:kubic:libcontainers:stable:cri-o:$VERSION/$OS/Release.key | apt-key add -
curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/Release.key | apt-key add -

apt-get update
apt-get install criu libyajl2
apt-get install cri-o cri-o-runc cri-tools containernetworking-plugins
systemctl start crio
```

## Configure CRI-O to use crun

CRI-O uses the `runc` runtime by default and we need to configure it to use `crun` instead.
That is done by adding to two configuration files.

> Make sure that you have [built and installed the `crun` binary with WasmEdge support](../container/crun.md) before starting the following steps.

First, create a `/etc/crio/crio.conf` file and add the following lines as its content. It tells CRI-O to use `crun` by default.

```conf
[crio.runtime]
default_runtime = "crun"
```

The `crun` runtime is in turn defined in the `/etc/crio/crio.conf.d/01-crio-runc.conf` file.

```conf
[crio.runtime.runtimes.runc]
runtime_path = "/usr/lib/cri-o-runc/sbin/runc"
runtime_type = "oci"
runtime_root = "/run/runc"
# The above is the original content

# Add our crunw runtime here
[crio.runtime.runtimes.crun]
runtime_path = "/usr/bin/crun"
runtime_type = "oci"
runtime_root = "/run/crun"
```

Next, restart CRI-O to apply the configuration changes.

```bash
systemctl restart crio
```

## Run a simple WebAssembly app

Now, we can run a simple WebAssembly program using CRI-O.
[A separate article](../demo/wasi.md) explains how to compile, package, and publish the WebAssembly
program as a container image to Docker hub.
In this section, we will start off pulling this WebAssembly-based container
image from Docker hub using CRI-O tools.

```bash
sudo crictl pull docker.io/hydai/wasm-wasi-example:with-wasm-annotation
```

Next, we need to create two simple configuration files that specifies how
CRI-O should run this WebAssembly image in a sandbox. We already have those
two files [container_wasi.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/container_wasi.json) and [sandbox_config.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/sandbox_config.json).
You can just download them to your local directory as follows.

```bash
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/sandbox_config.json
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/container_wasi.json
```

Now you can use CRI-O to create a pod and a container using the specified configurations.

```bash
# Create the POD. Output will be different from example.
$ sudo crictl runp sandbox_config.json
7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4
# Set a helper variable for later use.
$ POD_ID=7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4

# Create the container instance. Output will be different from example.
$ sudo crictl create $POD_ID container_wasi.json sandbox_config.json
# Set a helper variable for later use.
CONTAINER_ID=1d056e4a8a168f0c76af122d42c98510670255b16242e81f8e8bce8bd3a4476f
```

Starting the container would execute the WebAssembly program. You can see the output in the console.

```bash
# List the container, the state should be `Created`
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       wasmedge/example-wasi:latest                   About a minute ago   Created             podsandbox1-wasm-wasi   0                   7992e75df00cc

# Start the container
$ sudo crictl start $CONTAINER_ID

# Check the container status again.
# If the container is not finishing its job, you will see the Running state
# Because this example is very tiny. You may see Exited at this moment.
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       wasmedge/example-wasi:latest                   About a minute ago   Running             podsandbox1-wasm-wasi   0                   7992e75df00cc

# When the container is finished. You can see the state becomes Exited.
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED              STATE               NAME                     ATTEMPT             POD ID
1d056e4a8a168       wasmedge/example-wasi:latest                   About a minute ago   Exited              podsandbox1-wasm-wasi   0                   7992e75df00cc

# Check the container's logs. It should show outputs from the WebAssembly programs
$ sudo crictl logs $CONTAINER_ID

Test 1: Print Random Number
Random number: 960251471

Test 2: Print Random Bytes
Random bytes: [50, 222, 62, 128, 120, 26, 64, 42, 210, 137, 176, 90, 60, 24, 183, 56, 150, 35, 209, 211, 141, 146, 2, 61, 215, 167, 194, 1, 15, 44, 156, 27, 179, 23, 241, 138, 71, 32, 173, 159, 180, 21, 198, 197, 247, 80, 35, 75, 245, 31, 6, 246, 23, 54, 9, 192, 3, 103, 72, 186, 39, 182, 248, 80, 146, 70, 244, 28, 166, 197, 17, 42, 109, 245, 83, 35, 106, 130, 233, 143, 90, 78, 155, 29, 230, 34, 58, 49, 234, 230, 145, 119, 83, 44, 111, 57, 164, 82, 120, 183, 194, 201, 133, 106, 3, 73, 164, 155, 224, 218, 73, 31, 54, 28, 124, 2, 38, 253, 114, 222, 217, 202, 59, 138, 155, 71, 178, 113]

Test 3: Call an echo function
Printed from wasi: This is from a main function
This is from a main function

Test 4: Print Environment Variables
The env vars are as follows.
PATH: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
TERM: xterm
HOSTNAME: crictl_host
PATH: /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
The args are as follows.
/var/lib/containers/storage/overlay/006e7cf16e82dc7052994232c436991f429109edea14a8437e74f601b5ee1e83/merged/wasi_example_main.wasm
50000000

Test 5: Create a file `/tmp.txt` with content `This is in a file`

Test 6: Read the content from the previous file
File content is This is in a file

Test 7: Delete the previous file
```

Next, you can try to run the app in [Kubernetes](../kubernetes/kubernetes-crio.md)!

## Run a HTTP server app

Finally, we can run a simple WebAssembly-based HTTP micro-service in CRI-O.
[A separate article](../demo/server.md) explains how to compile, package, and publish the WebAssembly
program as a container image to Docker hub.
In this section, we will start off pulling this WebAssembly-based container
image from Docker hub using CRI-O tools.

```bash
sudo crictl pull docker.io/avengermojo/http_server:with-wasm-annotation
```

Next, we need to create two simple configuration files that specifies how
CRI-O should run this WebAssembly image in a sandbox. We already have those
two files [container_http_server.json](https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/http_server/container_http_server.json) and [sandbox_config.json](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/sandbox_config.json).
You can just download them to your local directory as follows.

> The `sandbox_config.json` file is the same for the simple WASI example and the HTTP server example. The other `container_*.json` file is application specific as it contains the application's Docker Hub URL.

```bash
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/sandbox_config.json
wget https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/http_server/container_http_server.json
```

Now you can use CRI-O to create a pod and a container using the specified configurations.

```bash
# Create the POD. Output will be different from example.
$ sudo crictl runp sandbox_config.json
7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4
# Set a helper variable for later use.
$ POD_ID=7992e75df00cc1cf4bff8bff660718139e3ad973c7180baceb9c84d074b516a4

# Create the container instance. Output will be different from example.
$ sudo crictl create $POD_ID container_http_server.json sandbox_config.json
# Set a helper variable for later use.
CONTAINER_ID=1d056e4a8a168f0c76af122d42c98510670255b16242e81f8e8bce8bd3a4476f
```

Starting the container would execute the WebAssembly program. You can see the output in the console.

```bash
# Start the container
$ sudo crictl start $CONTAINER_ID

# Check the container status. It should be Running. 
# If not, wait a few seconds and check again
$ sudo crictl ps -a
CONTAINER           IMAGE                                          CREATED                  STATE               NAME                ATTEMPT             POD ID
4eeddf8613691       wasmedge/example-wasi-http:latest              Less than a second ago   Running             http_server         0                   1d84f30e7012e

# Check the container's logs to see the HTTP server is listening at port 1234
$ sudo crictl logs $CONTAINER_ID
new connection at 1234

# Get the IP address assigned to the container
$ sudo crictl inspect $CONTAINER_ID | grep IP.0 | cut -d: -f 2 | cut -d'"' -f 2
10.85.0.2

# Test the HTTP service at that IP address
$ curl -d "name=WasmEdge" -X POST http://10.85.0.2:1234
echo: name=WasmEdge
```

Next, you can try to run it in [Kubernetes](../kubernetes/kubernetes-crio.md)!
