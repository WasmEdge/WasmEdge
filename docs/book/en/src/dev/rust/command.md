# Command interface

WASI enables WebAssembly programs to call standard library functions in the host operating system. It does so through a fine-grained security model known as “capability-based security”. The WebAssembly VM owner can grant access to host system resources when the VM starts up. The program cannot access any resources (e.g., file folders) that are not explicitly allowed.

Now, why limit ourselves to standard library functions? The same approach can be used to call just any host functions from WebAssembly. WasmEdge provides a WASI-like extension to access any command line programs in the host operating system.

The command line program can

* Take input via command line arguments, as well as the `STDIN` stream.
* Return value and data via the `STDOUT` stream.

Application developers for WasmEdge can use our Rust interface crate to access this functionality. In `Cargo.toml`, make sure that you have this dependency.

```toml
[dependencies]
rust_process_interface_library = "0.1.3"
```

In the Rust application, you can now use the API methods to start a new process for the operating system command program, pass in arguments via the `arg()` method as well as via the `STDIN`, and receives the return values via the `STDOUT`.

```rust
let mut cmd = Command::new("http_proxy");

cmd.arg("post")
   .arg("https://api.sendgrid.com/v3/mail/send")
   .arg(auth_header);  
cmd.stdin_u8vec(payload.to_string().as_bytes());

let out = cmd.output();
```

The Rust function is then compiled into WebAssembly and can run in the WasmEdge.
