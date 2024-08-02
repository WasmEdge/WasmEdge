### Integrating WasmEdge with k3s

This guide will help you integrate WasmEdge with k3s, a lightweight Kubernetes distribution for edge computing. Follow the steps below to set up and deploy WasmEdge applications on k3s.

#### Prerequisites
- A machine with Linux (preferably Ubuntu)
- Docker installed
- `k3s` installed
- `kubectl` command-line tool installed
- `crun` container runtime installed

#### Step 1: Install k3s

First, install k3s on your machine. You can do this by running the following command:

```sh
curl -sfL https://get.k3s.io | sh -
```

This command installs k3s and starts it automatically. To check the status of k3s, run:

```sh
sudo systemctl status k3s
```

#### Step 2: Install containerd and crun-wasmedge

WasmEdge requires the `crun` runtime to run WebAssembly applications. Follow these steps to install and configure `containerd` and `crun-wasmedge`.

1. **Install containerd**:

```sh
sudo apt-get update
sudo apt-get install -y containerd
```

2. **Install crun**:

```sh
sudo apt-get install -y crun
```

3. **Install WasmEdge**:

Follow the installation instructions from the [WasmEdge documentation](https://wasmedge.org/docs/quick-start/install/). For example:

```sh
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | sudo bash
```

4. **Configure containerd to use crun with WasmEdge**:

Edit the `config.toml` file for containerd to enable crun as the runtime handler for WasmEdge. This file is typically located at `/etc/containerd/config.toml`.

Add or modify the following lines:

```toml
[plugins]
  [plugins.cri.containerd]
    [plugins.cri.containerd.runtimes]
      [plugins.cri.containerd.runtimes.crun-wasmedge]
        runtime_type = "io.containerd.runc.v2"
        [plugins.cri.containerd.runtimes.crun-wasmedge.options]
          BinaryName = "crun"
          SystemdCgroup = true
```

Restart `containerd` to apply the changes:

```sh
sudo systemctl restart containerd
```

#### Step 3: Configure k3s to use containerd with crun-wasmedge

Edit the k3s service configuration to use the configured containerd runtime. Typically, the configuration file is located at `/etc/systemd/system/k3s.service` or `/etc/systemd/system/k3s.service.d/k3s.conf`.

Add the following environment variable to the service configuration:

```ini
[Service]
Environment="CONTAINERD_RUNTIME_HANDLER=crun-wasmedge"
```

Reload the systemd configuration and restart k3s:

```sh
sudo systemctl daemon-reload
sudo systemctl restart k3s
```

#### Step 4: Deploy a WasmEdge application on k3s

Create a Kubernetes deployment YAML file for your WasmEdge application. For example, `wasmedge-deployment.yaml`:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: wasmedge-demo
spec:
  replicas: 1
  selector:
    matchLabels:
      app: wasmedge-demo
  template:
    metadata:
      labels:
        app: wasmedge-demo
    spec:
      containers:
      - name: wasmedge-demo
        image: your-wasmedge-image:latest
        command: ["your-wasmedge-command"]
        resources:
          limits:
            memory: "128Mi"
            cpu: "500m"
        ports:
        - containerPort: 8080
```

Apply the deployment file using `kubectl`:

```sh
kubectl apply -f wasmedge-deployment.yaml
```

Check the status of your deployment:

```sh
kubectl get pods
```

#### Step 5: Access your WasmEdge application

Expose your WasmEdge application using a Kubernetes service. Create a service YAML file, e.g., `wasmedge-service.yaml`:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: wasmedge-service
spec:
  selector:
    app: wasmedge-demo
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: LoadBalancer
```

Apply the service file:

```sh
kubectl apply -f wasmedge-service.yaml
```

Retrieve the external IP address of the service:

```sh
kubectl get svc wasmedge-service
```

You can now access your WasmEdge application via the external IP address.

#### Conclusion

You have successfully integrated WasmEdge with k3s and deployed a WebAssembly application. You can now build and deploy more WasmEdge applications on your k3s cluster. 
