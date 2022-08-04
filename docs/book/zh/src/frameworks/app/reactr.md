# Reactr

[Reactr](https://github.com/suborbital/reactr) 是一个用 Go 语言编写的高性能函数调用库。Reactr 的设计十分灵活，你可将其嵌入 Go 程序中运行，它也为 WebAssembly 提供了一流的支持。
得益于 Go 语言出色的并发性能，Reactr 可以同时管理和执行上百个 WebAssembly 运行时实例，这使得 Reactr 成为了服务端程序框架的一个优秀选择。

Reactr 让你可以在 Go 程序中调用 WebAssembly 函数，[WasmEdge Go SDK](../../embed/go.md) 也是如此。
Reactr 的特别之处在于，它在 Go 语言中提供了一套丰富的主机函数，其中包括了对访问网络、数据库等功能的支持。此外，Reactr 提供了 Rust (以及 Swift / AssemblyScript) 的 API，你可以用它在 WebAssembly 函数中调用上述的主机函数。

本文将向你展示如何将 WasmEdge 与 Reactr 结合起来使用，以充分利用二者的优势。
WasmEdge 是[性能最佳且最具拓展性的 WebAssembly 运行时](../../intro/features.md)，
并在 [Reactr 的官方测试用例](https://github.com/suborbital/reactr/runs/4476074960?check_suite_focus=true)中是性能最好的。
我们将演示如何在 WasmEdge 和 Reactr 中运行编译为 WebAssembly 的 Rust 函数以及 JavaScript 程序。

> WasmEdge 为 JavaScript 提供了[增强支持](../../dev/js.md)，包括[将 Rust 与 JavaScript 混合](../../dev/js/rust.md)以提高性能。

* [Hello world](#hello-world)
* [数据库查询](#数据库查询)
* [在 Go 中嵌入 JavaScript](#在-Go-中嵌入-JavaScript)

## 环境准备

你的系统中需要装有 [Rust](https://www.rust-lang.org/zh-CN/tools/install)、[Go](https://go.dev/doc/install) 以及 [WasmEdge](../../start/install.md)。
WasmEdge 需要用到 GCC 编译器（`build-essential` 中包含 GCC 编译器）。

```bash
sudo apt-get update
sudo apt-get -y upgrade
sudo apt install build-essential

curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
rustup target add wasm32-wasi

curl -OL https://golang.org/dl/go1.17.5.linux-amd64.tar.gz
sudo tar -C /usr/local -xvf go1.17.5.linux-amd64.tar.gz
export PATH=$PATH:/usr/local/go/bin

wget -qO- https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash
source $HOME/.wasmedge/env
```

## Hello world

你可以在[这里](https://github.com/second-state/wasm-learning/tree/master/reactr/hello)找到一个简单的关于 Reactr 的 `hello world` 示例。

### Hello world: 编译为 WebAssembly 的 Rust 函数

我们首先编写一个[简单的 Rust 函数](https://github.com/second-state/wasm-learning/blob/master/reactr/hello/hello-echo/src/lib.rs)，它所做的事只是回应一个 "hello"。
我们将通过 Reactr 把下面的 Rust 函数 `HelloEcho::run()` 暴露给 Go 主机应用。

```rust
use suborbital::runnable::*;

struct HelloEcho{}

impl Runnable for HelloEcho {
  fn run(&self, input: Vec<u8>) -> Result<Vec<u8>, RunErr> {
    let in_string = String::from_utf8(input).unwrap();
    Ok(format!("hello {}", in_string).as_bytes().to_vec())
  }
}
```

接下来，把这个 Rust 函数构建为 WebAssembly 字节码文件。

```bash
cd hello-echo
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/hello_echo.wasm ..
cd ..
```

### Hello world: Go 主机程序

接下来，让我们看看执行这个 WebAssembly 函数的 [Go 主机程序](https://github.com/second-state/wasm-learning/blob/master/reactr/hello/main.go)。
`Runnable` 结构体中的 `runBundle()` 函数所做的事是调用 `run()` 函数。

```go
func runBundle() {
  r := rt.New()
  doWasm := r.Register("hello-echo", rwasm.NewRunner("./hello_echo.wasm"))

  res, err := doWasm([]byte("wasmWorker!")).Then()
  if err != nil {
    fmt.Println(err)
    return
  }

  fmt.Println(string(res.([]byte)))
}
```

`runGroup()` 函数会在组内异步地执行由 Rust 编译的 WebAssembly `run()` 函数，并在返回结果的时候接收它们。

```go
func runGroup() {
  r := rt.New()

  doWasm := r.Register("hello-echo", rwasm.NewRunner("./hello_echo.wasm"))

  grp := rt.NewGroup()
  for i := 0; i < 100000; i++ {
    grp.Add(doWasm([]byte(fmt.Sprintf("world %d", i))))
  }

  if err := grp.Wait(); err != nil {
    fmt.Println(err)
  }
}
```

现在，我们可以将 Go 主机程序运行起来，并观察控制台中所打印的结果。

> 只有在设置了 `-tags wasmedge` 这一标签后，你才能获得 WasmEdge 提供的性能优势以及拓展 WebAssembly API。

```bash
go mod tidy
go run -tags wasmedge main.go
```

## 数据库查询

这个[示例](https://github.com/second-state/wasm-learning/tree/master/reactr/db)演示了如何在 WebAssembly 函数中使用 Reactr 主机函数及 API 查询 PostgreSQL 数据库。

### 数据库查询：下载并安装 PostgreSQL

我们可以在 Docker 中启动一个 PostgreSQL 实例。

```bash
docker pull postgres
docker run --name reactr-postgres -p 5432:5432 -e POSTGRES_PASSWORD=12345 -d postgres
```

接下来，创建一个数据库并向其中填入一些样本数据。

```bash
$ docker run -it --rm --network host postgres psql -h 127.0.0.1 -U postgres
postgres=# CREATE DATABASE reactr;
postgres=# \c reactr;

# Create a table:
postgres=# CREATE TABLE users (
    uuid        varchar(100) CONSTRAINT firstkey PRIMARY KEY,
    email       varchar(50) NOT NULL,
    created_at  date,
    state       char(1),
    identifier  integer
);
```

保持容器的运行状态，并打开一个新的终端窗口来与 PostgreSQL 服务器进行交互。

### 数据库查询：编译为 WebAssembly 的 Rust 函数

让我们编写一个访问 PostgreSQL 数据库的 [Rust 函数](https://github.com/second-state/wasm-learning/blob/master/reactr/db/rs-db/src/lib.rs)。
如下所示的 Rust 函数 `RsDbtest::run()` 将会通过 Reactr 暴露给 Go 主机程序。它使用 `PGInsertUser`、`PGSelectUserWithUUID` 等命名查询来操作数据库，稍后我们会看到定义在 Go 主机程序中的查询。

```rust
use suborbital::runnable::*;
use suborbital::db;
use suborbital::util;
use suborbital::db::query;
use suborbital::log;
use uuid::Uuid;

struct RsDbtest{}

impl Runnable for RsDbtest {
  fn run(&self, _: Vec<u8>) -> Result<Vec<u8>, RunErr> {
    let uuid = Uuid::new_v4().to_string();

    let mut args: Vec<query::QueryArg> = Vec::new();
    args.push(query::QueryArg::new("uuid", uuid.as_str()));
    args.push(query::QueryArg::new("email", "connor@suborbital.dev"));

    match db::insert("PGInsertUser", args) {
      Ok(_) => log::info("insert successful"),
      Err(e) => {
        return Err(RunErr::new(500, e.message.as_str()))
      }
    };

    let mut args2: Vec<query::QueryArg> = Vec::new();
    args2.push(query::QueryArg::new("uuid", uuid.as_str()));

    match db::update("PGUpdateUserWithUUID", args2.clone()) {
      Ok(rows) => log::info(format!("update: {}", util::to_string(rows).as_str()).as_str()),
      Err(e) => {
        return Err(RunErr::new(500, e.message.as_str()))
      }
    }

    match db::select("PGSelectUserWithUUID", args2.clone()) {
      Ok(result) => log::info(format!("select: {}", util::to_string(result).as_str()).as_str()),
      Err(e) => {
        return Err(RunErr::new(500, e.message.as_str()))
      }
    }

    match db::delete("PGDeleteUserWithUUID", args2.clone()) {
      Ok(rows) => log::info(format!("delete: {}", util::to_string(rows).as_str()).as_str()),
      Err(e) => {
        return Err(RunErr::new(500, e.message.as_str()))
      }
    }

    ... ...
  }
}
```

让我们把这个 Rust 函数构建为 WebAssembly 字节码文件。

```bash
cd rs-db
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/rs_db.wasm ..
cd ..
```

### 数据库查询：Go 主机程序

在这个 [Go 主机程序](https://github.com/second-state/wasm-learning/blob/master/reactr/db/main.go)中，我们首先定义了几个 SQL 查询，并为其命名。
我们将把这些查询作为配置传入 Reactr 运行时。

```go
func main() {
  dbConnString, exists := os.LookupEnv("REACTR_DB_CONN_STRING")
  if !exists {
    fmt.Println("skipping as conn string env var not set")
    return
  }

  q1 := rcap.Query{
    Type:     rcap.QueryTypeInsert,
    Name:     "PGInsertUser",
    VarCount: 2,
    Query: `
    INSERT INTO users (uuid, email, created_at, state, identifier)
    VALUES ($1, $2, NOW(), 'A', 12345)`,
  }

  q2 := rcap.Query{
    Type:     rcap.QueryTypeSelect,
    Name:     "PGSelectUserWithUUID",
    VarCount: 1,
    Query: `
    SELECT * FROM users
    WHERE uuid = $1`,
  }

  q3 := rcap.Query{
    Type:     rcap.QueryTypeUpdate,
    Name:     "PGUpdateUserWithUUID",
    VarCount: 1,
    Query: `
    UPDATE users SET state='B' WHERE uuid = $1`,
  }

  q4 := rcap.Query{
    Type:     rcap.QueryTypeDelete,
    Name:     "PGDeleteUserWithUUID",
    VarCount: 1,
    Query: `
    DELETE FROM users WHERE uuid = $1`,
  }

  config := rcap.DefaultConfigWithDB(vlog.Default(), rcap.DBTypePostgres, dbConnString, []rcap.Query{q1, q2, q3, q4})

  r, err := rt.NewWithConfig(config)
  if err != nil {
    fmt.Println(err)
    return
  }

  ... ...
}
```

接下来便可以在 Reactr 中运行这个 WebAssembly 函数了。

```go
func main() {
  ... ...

  doWasm := r.Register("rs-db", rwasm.NewRunner("./rs_db.wasm"))

  res, err := doWasm(nil).Then()
  if err != nil {
    fmt.Println(err)
    return
  }

  fmt.Println(string(res.([]byte)))
}
```

最后，让我们运行 Go 主机程序并观察控制台中打印出的结果。

> 只有在设置了 `-tags wasmedge` 这一标签后，你才能获得 WasmEdge 提供的性能优势以及拓展 WebAssembly API。

```bash
export REACTR_DB_CONN_STRING='postgresql://postgres:12345@127.0.0.1:5432/reactr'
go mod tidy
go run -tags wasmedge main.go
```

## 在 Go 中嵌入 JavaScript

我们前面提过，WasmEdge 运行时的一大特点就是它[对 JavaScript 的高级支持](../../dev/js.md)，这使得我们能在轻量、高性能、安全、支持多语言的[Kubernetes 管理的 WasmEdge 容器](../../kubernetes.md)中运行 JavaScript 程序。
你可以在[这里](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs)找到一个在 Reactr 中嵌入的 JavaScript 函数示例。

### JavaScript 示例

这个[示例函数](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/hello.js)十分简单，仅仅是返回一个字符串值。

```javascript
let h = 'hello';
let w = 'wasmedge';
`${h} ${w}`;
```

### JavaScript 示例：Go 主机程序

这个 [Go 主机程序](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/main.go) 使用了 Reactr API 来运行 WasmEdge 的标准 JavaScript 解释器（[rs_embed_js.wasm](https://github.com/second-state/wasm-learning/blob/master/reactr/quickjs/rs_embed_js.wasm)）。你也可以通过修改这个 [Rust 项目](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/rs-embed-js) 来使用其他版本的 JavaScript 解释器。

> 你可以点击这两个超链接来了解如何[将 JavaScript 代码嵌入 Rust](https://github.com/second-state/wasmedge-quickjs/tree/main/examples/embed_js) 以及[使用 Rust 实现 JavaScript API](../../dev/js/rust.md)。

Go 主机程序只需要启动 `rs_embed_js.wasm` 的任务并将 JavaScript 内容传入它。接下来可以用 Go 程序来捕获并打印出返回值。

```go
func main() {
  r := rt.New()
  doWasm := r.Register("hello-quickjs", rwasm.NewRunner("./rs_embed_js.wasm"))

  code, err := ioutil.ReadFile(os.Args[1])
  if err != nil {
    fmt.Print(err)
  }
  res, err := doWasm(code).Then()
  if err != nil {
    fmt.Println(err)
    return
  }

  fmt.Println(string(res.([]byte)))
}
```

使用下面的命令运行 Go 主机程序。

```bash
$ cd quickjs
$ go mod tidy
$ go run -tags wasmedge main.go hello.js
String(JsString(hello wasmedge))
```

打印出来的结果中包含了 Rust 和 Go API 中的字符串类型信息。你可以对 Rust / Go 程序进行修改来将其移除。

### JavaScript 示例：功能实例

WasmEdge 支持许多 JavaScript 高级特性。在了解了上面的三个简单例子后，你可以尝试使用我们的 [React SSR 示例](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr)来通过一个 Reactr 函数生成 HTML UI。
你可以只从 React SSR 示例中构建 `dist/main.js`，并将其复制到这个示例的文件夹中，然后你就能让它工作起来了！

```bash
$ cd quickjs
# 从 react ssr 示例中拷贝出 dist/main.js
$ go mod tidy
$ go run -tags wasmedge main.go main.js
<div data-reactroot=""><div>This is home</div><div><div>This is page</div></div></div>
UnDefined
```
