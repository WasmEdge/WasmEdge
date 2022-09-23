# wasm-nginx-module

The wasm-nginx-module is an Nginx module built upon OpenResty. By implementing the [Proxy Wasm ABI](https://github.com/proxy-wasm/spec), any Wasm program written with Proxy Wasm SDK can be run inside it. Hence, you can write Go or Rust code, compile them into Wasm, then load & execute it in Nginx.

> The wasm-nginx-module is already used in APISIX and allows it to [run Wasm plugin like Lua plugin](https://github.com/apache/apisix/blob/master/docs/en/latest/wasm.md).

In order to follow along the tutorials in this chapter, you will need to first [build your Nginx with wasm-nginx-module included and WasmEdge shared library installed in the right path](https://github.com/api7/wasm-nginx-module#install-dependencies).

Once you have Nginx installed, let me show you a real world example - using Wasm to inject custom responses in Nginx.

## Inject Custom Response via Go in Nginx, Step by Step

### Go Step 1: Write code based on proxy-wasm-go-sdk

The implementation code (including `go.mod` and others) can be found at [here](https://github.com/apache/apisix/tree/master/t/wasm).

It should be explained that although the proxy-wasm-go-sdk project carries the Go name, it actually uses tinygo instead of native Go, which has some problems supporting WASI (which you can think of as a non-browser WASM runtime interface), see [here](https://github.com/tetratelabs/proxy-wasm-go-sdk/blob/main/doc/OVERVIEW.md#tinygo-vs-the-official-go-compiler) for more details.

We also provide a Rust version (including Cargo.toml and others) [there](https://github.com/api7/wasm-nginx-module/tree/main/t/testdata/rust/fault-injection).

### Go Step 2: Build the corresponding Wasm file

```shell
tinygo build -o ./fault-injection/main.go.wasm -scheduler=none -target=wasi ./fault-injection/main.go
```

### Go Step 3: Load and execute the Wasm file

Then, start Nginx with the configuration below:

```conf
worker_processes  1;

error_log  /tmp/error.log warn;

events {
    worker_connections  10240;
}

http {
    wasm_vm wasmedge;
    init_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        package.loaded.plugin = assert(wasm.load("fault_injection",
            "/path/to/fault-injection/main.go.wasm"))
    }
    server {
        listen 1980;
        location / {
            content_by_lua_block {
                local wasm = require("resty.proxy-wasm")
                local ctx = assert(wasm.on_configure(package.loaded.plugin,
                    '{"http_status": 403, "body": "powered by wasm-nginx-module"}'))
                assert(wasm.on_http_request_headers(ctx))
            }
        }
    }
}
```

This configuration loads the Wasm file we just built, executes it with the configuration `{"http_status": 403, "body": "powered by wasm-nginx-module"}`.

### Go Step 4: verify the result

After Nginx starts, we can use `curl http://127.0.0.1:1980/ -i` to verify the execution result of the Wasm.

It is expected to see the output:

```bash
HTTP/1.1 403 Forbidden
...

powered by wasm-nginx-module
```

## Inject Custom Response via Rust in Nginx, Step by Step

### Rust Step 1: Write code based on proxy-wasm-rust-sdk

We also provide a Rust version (including Cargo.toml and others) [here](https://github.com/api7/wasm-nginx-module/tree/main/t/testdata/rust/fault-injection).

### Rust Step 2: Build the corresponding Wasm file

```shell
cargo build --target=wasm32-wasi
```

### Rust Step 3: Load and execute the Wasm file

Then, start Nginx with the configuration below:

```conf
worker_processes  1;

error_log  /tmp/error.log warn;

events {
    worker_connections  10240;
}

http {
    wasm_vm wasmedge;
    init_by_lua_block {
        local wasm = require("resty.proxy-wasm")
        package.loaded.plugin = assert(wasm.load("fault_injection",
            "/path/to/fault-injection/target/wasm32-wasi/debug/fault_injection.wasm"))
    }
    server {
        listen 1980;
        location / {
            content_by_lua_block {
                local wasm = require("resty.proxy-wasm")
                local ctx = assert(wasm.on_configure(package.loaded.plugin,
                    '{"http_status": 403, "body": "powered by wasm-nginx-module"}'))
                assert(wasm.on_http_request_headers(ctx))
            }
        }
    }
}
```

This configuration loads the Wasm file we just built, executes it with the configuration `{"http_status": 403, "body": "powered by wasm-nginx-module"}`.

### Rust Step 4: verify the result

After Nginx starts, we can use `curl http://127.0.0.1:1980/ -i` to verify the execution result of the Wasm.

It is expected to see the output:

```bash
HTTP/1.1 403 Forbidden
...

powered by wasm-nginx-module
```
