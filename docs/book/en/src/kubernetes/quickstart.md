# Quick start

You can use the CRI-O [install.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/crio/install.sh) script to install CRI-O and `crun` on Ubuntu 20.04.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/crio/install.sh | bash
```

Next, install Kubernetes using the [following script](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes/install.sh).

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes/install.sh | bash
``` 

The [simple_wasi_application.sh](https://github.com/second-state/wasmedge-containers-examples/blob/main/kubernetes/simple_wasi_application.sh) script shows how to pull [a WebAssembly application](demo/wasi.md) from Docker Hub, and then run it as a containerized application in Kubernetes.

```bash
wget -qO- https://raw.githubusercontent.com/second-state/wasmedge-containers-examples/main/kubernetes/simple_wasi_application.sh | bash
```

You should see results from the WebAssembly prpogram printed in the console log. [Here is an example](https://github.com/second-state/wasmedge-containers-examples/runs/4186005677?check_suite_focus=true#step:6:3007).

