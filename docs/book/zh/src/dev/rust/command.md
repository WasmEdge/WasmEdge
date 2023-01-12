# 使用命令行程序

WASI 通过一个被称为“基于能力的安全”的细粒度安全模型让 WebAssembly 程序可以调用宿主操作系统的标准库函数。WebAssembly 虚拟机所有者可以在虚拟机启动的时候给 VM 赋予访问宿主系统资源的权限。程序默认不能访问任何没有被显式允许访问的资源，比如文件夹。

但为什么要将我们自己限制在标准库函数上呢？ 我们可以用同样的方法在 WebAssembly 中调用任何宿主函数。WasmEdge 提供了一个类似 WASI 的拓展，可以访问在宿主操作系统中的任何命令行程序。

这些命令行程序可以

* 从命令行参数中获取输入，包括 `STDIN` 流。
* 通过 `STDOUT` 流来返回值和数据。

开发者可以使用我们的 Rust 接口库来访问这个功能。确保你在 `Cargo.toml` 中添加了这个依赖项。

```toml
[dependencies]
rust_process_interface_library = "0.1.3"
```

现在你可以在 Rust 应用程序中使用这个接口启动一个新的进程，来运行操作系统中的命令行程序，通过 `arg()` 方法或者 `STDIN` 传入参数，并通过 `STDOUT` 接收返回值。

```rust
let mut cmd = Command::new("http_proxy");

cmd.arg("post")
   .arg("https://api.sendgrid.com/v3/mail/send")
   .arg(auth_header);  
cmd.stdin_u8vec(payload.to_string().as_bytes());

let out = cmd.output();
```

然后编译这个 Rust 函数为 WebAssembly，并在 WasmEdge 中运行。
