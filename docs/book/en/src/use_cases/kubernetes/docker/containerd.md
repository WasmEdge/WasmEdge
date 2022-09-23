# Use the containerd shim

As we discussed, wrapping WebAssembly inside a Docker Linux container results in performance and security penalties. However, we cannot easily replace the OCI runtime (`runc`) in the Docker toolchain as well. In this chapter, we will discuss another approach to start and run WebAssembly bytecode applications directly from the Docker CLI.

Coming soon
