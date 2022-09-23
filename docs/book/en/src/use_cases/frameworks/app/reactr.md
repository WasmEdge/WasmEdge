# Reactr

[Reactr](https://github.com/suborbital/reactr) is a fast, performant function scheduling library written in Go. Reactr is designed to be flexible, with the ability to run embedded in your Go applications and first-class support for WebAssembly.
Taking advantage of Go's superior concurrency capabilities, Reactr can manage and execute hundreds of WebAssembly runtime instances all at once, making a great framework for server-side applications.

Reactr allows you to run WebAssembly functions in Go, so does the [WasmEdge Go SDK](../../../sdk/go.md).
The unique feature of Reactr is that it provides a rich set of host functions in Go, which support access to networks and databases etc. Reactr then provides Rust (and Swift / AssemblyScript) APIs to call those host functions from within the WebAssembly function.

In this article, we will show you how to use WasmEdge together with Reactr to take advantage of the best of both worlds. WasmEdge is the [fastest and most extensible WebAssembly runtime](../../../features.md).
It is also the fastest in [Reactr's official test suite](https://github.com/suborbital/reactr/runs/4476074960?check_suite_focus=true).
We will show you how to run Rust functions compiled to WebAssembly as well as JavaScript programs in WasmEdge and Reactr.

> WasmEdge provides [advanced support for JavaScript](../../../write_wasm/js.md) including [mixing Rust with JavaScript](../../../write_wasm/js/rust.md) for improved performance.

* [Hello world](#hello-world)
* [Database query](#database-query)
* [Embed JavaScript in Go](#embed-javascript-in-go)

## Prerequisites

You need have [Rust](https://www.rust-lang.org/tools/install), [Go](https://go.dev/doc/install), and [WasmEdge](../../../quick_start/install.md) installed on your system.
The GCC compiler (installed via the `build-essential` package) is also needed for WasmEdge.

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

A simple `hello world` example for Reactr is [available here](https://github.com/second-state/wasm-learning/tree/master/reactr/hello).

### Hello world: Rust function compiled to WebAssembly

Let's first create [a simple Rust function](https://github.com/second-state/wasm-learning/blob/master/reactr/hello/hello-echo/src/lib.rs) to echo hello.
The Rust function `HelloEcho::run()` is as follows. It will be exposed to the Go host application through Reactr.

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

Let's build the Rust function into a WebAssembly bytecode file.

```bash
cd hello-echo
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/hello_echo.wasm ..
cd ..
```

### Hello world: Go host application

Next, lets look into the [Go host app](https://github.com/second-state/wasm-learning/blob/master/reactr/hello/main.go) that executes the WebAssembly functions.
The `runBundle()` function executes the `run()` function in the `Runnable` struct once.

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

The `runGroup()` function executes the Rust-compiled WebAssembly `run()` function multiple times asynchronously in a group, and receives the results as they come in.

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

Finally, let's run the Go host application and see the results printed to the console.

> You must use the `-tags wasmedge` flag to take advantage of the performance and extended WebAssembly APIs provided by WasmEdge.

```bash
go mod tidy
go run -tags wasmedge main.go
```

## Database query

In [this example](https://github.com/second-state/wasm-learning/tree/master/reactr/db), we will demonstrate how to use Reactr host functions and APIs to query a PostgreSQL database from your WebAssembly function.

### Database query: Install and set up a PostgreSQL database

We will start a PostgreSQL instance through Docker.

```bash
docker pull postgres
docker run --name reactr-postgres -p 5432:5432 -e POSTGRES_PASSWORD=12345 -d postgres
```

Next, let's create a database and populate it with some sample data.

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

Leave this running and start another terminal window to interact with this PostgreSQL server.

### Database query: Rust function compiled to WebAssembly

Let's create [a Rust function](https://github.com/second-state/wasm-learning/blob/master/reactr/db/rs-db/src/lib.rs) to access the PostgreSQL database.
The Rust function `RsDbtest::run()` is as follows. It will be exposed to the Go host application through Reactr. It uses named queries such as `PGInsertUser` and `PGSelectUserWithUUID` to operate the database. Those queries are defined in the Go host application, and we will see them later.

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

Let's build the Rust function into a WebAssembly bytecode file.

```bash
cd rs-db
cargo build --target wasm32-wasi --release
cp target/wasm32-wasi/release/rs_db.wasm ..
cd ..
```

### Database query: Go host application

The [Go host app](https://github.com/second-state/wasm-learning/blob/master/reactr/db/main.go) first defines the SQL queries and gives each of them a name.
We will then pass those queries to the Reactr runtime as a configuration.

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

Then, we can run the WebAssembly function from Reactr.

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

Finally, let's run the Go host application and see the results printed to the console.

> You must use the `-tags wasmedge` flag to take advantage of the performance and extended WebAssembly APIs provided by WasmEdge.

```bash
export REACTR_DB_CONN_STRING='postgresql://postgres:12345@127.0.0.1:5432/reactr'
go mod tidy
go run -tags wasmedge main.go
```

## Embed JavaScript in Go

As we mentioned, a key feature of the WasmEdge Runtime is its advanced [JavaScript support](../../../write_wasm/js.md), which allows JavaScript programs to run in lightweight, high-performance, safe, multi-language, and [Kubernetes-managed WasmEdge containers](../../kubernetes.md).
A simple example of embedded JavaScript function in Reactr is [available here](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs).

### JavaScript example

The [JavaScript example function](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/hello.js) is very simple. It just returns a string value.

```javascript
let h = 'hello';
let w = 'wasmedge';
`${h} ${w}`;
```

### JavaScript example: Go host application

The [Go host app](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/main.go) uses the Reactr API to run WasmEdge's standard JavaScript interpreter [rs_embed_js.wasm](https://github.com/second-state/wasm-learning/blob/master/reactr/quickjs/rs_embed_js.wasm). You can build your own version of JavaScript interpreter by modifying [this Rust project](https://github.com/second-state/wasm-learning/tree/master/reactr/quickjs/rs-embed-js).

> Learn more about how to embed [JavaScript code in Rust](https://github.com/second-state/wasmedge-quickjs/tree/main/examples/embed_js), and how to [use Rust to implement JavaScript APIs](../../../write_wasm/js/rust.md) in WasmEdge.

The Go host application just need to start the job for `rs_embed_js.wasm` and pass the JavaScript content to it. The Go application can then capture and print the return value from JavaScript.

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

Run the Go host application as follows.

```bash
$ cd quickjs
$ go mod tidy
$ go run -tags wasmedge main.go hello.js
String(JsString(hello wasmedge))
```

The printed result shows the type information of the string in Rust and Go APIs. You can strip out this information by changing the Rust or Go applications.

### JavaScript example: Feature examples

WasmEdge supports many advanced JavaScript features. For the next step, you could try our [React SSR example](https://github.com/second-state/wasmedge-quickjs/tree/main/example_js/react_ssr) to generate an HTML UI from a Reactr function!
You can just build the `dist/main.js` from the React SSR example, and copy it over to this example folder to see it in action!

```bash
$ cd quickjs
# copy over the dist/main.js file from the react ssr example
$ go mod tidy
$ go run -tags wasmedge main.go main.js
<div data-reactroot=""><div>This is home</div><div><div>This is page</div></div></div>
UnDefined
```
