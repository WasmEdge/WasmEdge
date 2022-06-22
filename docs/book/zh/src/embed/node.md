# Node.js SDK

在这篇教程中，我会向你展示如何通过 WasmEdge Node.js SDK 将用 Rust 写的 WebAssembly 函数合并进 Node.js 的服务端应用程序
中。这个方法可以将 Rust 的**性能**，WebAssembly 的**安全性**和**可移植性**，和 JavaScript 的**易用性**结合。一个典型的应用程序就像这样。

* host 应用程序是一个用 JavaScript 写的 Node.js web 应用程序，它可以调用 WebAssembly 函数。
* WebAssembly 字节码程序是用 Rust 写的，运行在 WasmEdge Runtime，可以被 Node.js web 应用程序调用。

> [Fork 这个 Github 仓库](https://github.com/second-state/wasmedge-nodejs-starter/fork)来开始写代码！

## 先决条件

为了搭建一个包含 Rust 和 WebAssembly 的高性能 Node.js 环境，你需要如下准备：

* 一个现代的 Linux 发行版, 比如 Ubuntu Server 20.04 LTS
* [Rust 语言](https://www.rust-lang.org/tools/install)
* [Node.js](https://nodejs.org/en/download/package-manager/)
* Node.js 的 [WasmEdge Runtime](../start/install.md#install-wasmedge-for-node.js)
* [rustwasmc 编译器工具链](/dev/rust/bindgen.md)

### Docker

最简单的启动方式就是使用 Docker 来搭建开发环境。只需要[克隆这个模板](https://github.com/second-state/wasmedge-nodejs-starter/)到你的电脑，然后运行如下 Docker 命令：

```bash
# 克隆代码到本地
$ git clone https://github.com/second-state/wasmedge-nodejs-starter
$ cd wasmedge-nodejs-starter

# 启动 Docker 容器
$ docker pull wasmedge/appdev_x86_64:0.8.2
$ docker run -p 3000:3000 --rm -it -v $(pwd):/app wasmedge/appdev_x86_64:0.8.2
(docker) $ cd /app
```

好了，你现在可以编译和运行代码了。

### 没有 Docker 的手动启动

命令如下。

```bash
# 安装 Rust
$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
$ source $HOME/.cargo/env
$ rustup override set 1.50.0

# 安装 Node.js 和 npm
$ curl -sL https://deb.nodesource.com/setup_14.x |  bash
$ sudo apt-get install -y nodejs npm

# 安装 rustwasmc 工具链
$ npm install -g rustwasmc # 如果权限有问题，加上 --unsafe-perm

# WasmEdge 需要的系统依赖
$ sudo apt-get update
$ sudo apt-get -y upgrade
$ sudo apt install -y build-essential curl wget git vim libboost-all-dev llvm-dev liblld-10-dev

# 安装 WasmEdge 需要的 nodejs addon 
$ npm install wasmedge-core
$ npm install wasmedge-extensions
```

> WasmEdge Runtime 需要最新版本的 `libstdc++`。 Ubuntu 20.04 LTS 已经有最新的库了。 如果你使用的是比较老的 Linux 发行版中，有一些选项需要升级，[更详细的信息在这儿](https://www.secondstate.io/articles/ubuntu-req-ssvm-20200715/)。

然后，克隆示例源代码仓库。

```bash
git clone https://github.com/second-state/wasmedge-nodejs-starter
cd wasmedge-nodejs-starter
```

## Hello World

第一个示例是一个 hello world，向你展示应用程序的各个部分如何组合在一起。

### Rust 写的 WebAssembly 程序

在这个例子中，Rust 程序将输入的字符串添加到 “hello” 后面。下面是 Rust 程序内容，位于 src/lib.rs。你可以在这个库文件中定义多个外部方法，所有的这些方法都可以在 host JavaScript 应用中通过 WebAssembly 调用。记得需要给每个函数添加 `#[wasm_bindgen]` 注解，这样 [rustwasmc](https://github.com/second-state/rustwasmc) 就知道在构建时为这些函数生成正确的 JavaScript 到 Rust 接口。

```rust
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn say(s: String) -> String {
  let r = String::from("hello ");
  return r + &s;
}
```

然后你可以将 Rust 源代码编译成 WebAssembly 字节码，并且生成相应的 JavaScript 模块供 Node.js host 环境调用。

```bash
rustwasmc build
```

生成的文件在 `pkg/` 目录下，`.wasm` 文件是 WebAssembly 字节码程序，`.js` 文件是 JavaScript 模块。

### Node.js host 应用程序

然后进入 `node` 文件夹下，检查 JavaScript 程序 `app.js`。有了生成的 `wasmedge_nodejs_starter_lib.js` 模块，就很容易写出调用 WebAssembly 函数的 JavaScript 了。下面是 node 应用程序 `app.js`。简单的从生成的模块中引入 `say()` 函数。 node 应用程序从 HTTP GET 请求中拿到 `name` 参数后返回 “hello `name`”。

```javascript
const { say } = require('../pkg/wasmedge_nodejs_starter_lib.js');

const http = require('http');
const url = require('url');
const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) => {
  const queryObject = url.parse(req.url,true).query;
  res.statusCode = 200;
  res.setHeader('Content-Type', 'text/plain');
  res.end(say(queryObject['name']));
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});
```

像下面一样启动 Node.js 应用程序。

```bash
$ node node/app.js
Server running at http://127.0.0.1:3000/
```

然后，你可以在另外一个终端窗口中测试。

```bash
$ curl http://127.0.0.1:3000/?name=Wasm
hello Wasm
```

## 完整的 web 应用程序

下面的例子展示了一个计算二次方程根的 web 应用程序，请在这里查看[完整源代码](https://github.com/second-state/wasm-learning/tree/master/nodejs/quadratic).

用户在 web 表单中输入 `a`, `b`, `c` 三个值，web 应用程序调用 web 服务 `/solve`，计算出二次方程的根。

```src
a*X^2 + b*X + c = 0
```

`X` 的根展示在输入表单下面。

![getting-started-with-rust-function](https://www.secondstate.io/articles/getting-started-with-rust-function-01.png)

[HTML 文件](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/node/public/index.html) 包含提交 web 表单到 `/solve` 的客户端 JavaScript，并且将结果放到页面的 `#roots` HTML 元素里。

```javascript
$(function() {
    var options = {
      target: '#roots',
      url: "/solve",
      type: "post"
    };
    $('#solve').ajaxForm(options);
});
```

`/solve` URL 端点后的 [Node.js 应用程序](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/node/server.js)如下所示。他从输入表单中读取数据，将他们作为数组传递给 `solve` 函数，将返回结果放到 HTTP 返回内容中。

```javascript
app.post('/solve', function (req, res) {
  var a = parseFloat(req.body.a);
  var b = parseFloat(req.body.b);
  var c = parseFloat(req.body.c);
  res.send(solve([a, b, c]))
})
```

[用 Rust 写的 `solve` 函数](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/src/lib.rs)，运行在 WasmEdge Runtime。如果 JavaScript 端的调用参数是数组，Rust 函数接收到一个封装数组的 JSON 对象。在 Rust 代码中，我们首先解码 JSON，执行计算，然后返回一个 JSON 字符串的结果。

```rust
#[wasm_bindgen]
pub fn solve(params: &str) -> String {
  let ps: (f32, f32, f32) = serde_json::from_str(&params).unwrap();
  let discriminant: f32 = (ps.1 * ps.1) - (4. * ps.0 * ps.2);
  let mut solution: (f32, f32) = (0., 0.);
  if discriminant >= 0. {
    solution.0 = (((-1.) * ps.1) + discriminant.sqrt()) / (2. * ps.0);
    solution.1 = (((-1.) * ps.1) - discriminant.sqrt()) / (2. * ps.0);
    return serde_json::to_string(&solution).unwrap();
  } else {
    return String::from("not real numbers");
  }
}
```

让我们试试。

```bash
rustwasmc build
npm install express # 这个应用程序需要 Node.js 的 express 框架
node node/server.js
```

在 web 浏览器中，输入 `http://ip-addr:8080/` 来获取应用程序。注意：如果你使用的是 Docker，确保 Docker 容器中的 8080 端口映射到宿主的 8080 端口。

这就是二次方程的例子。

## 更多例子

在 Rust 和 JavaScript 之间除了可以传递字符串值外， `rustwasmc` 工具支持下面的数据类型。

* Rust 调用参数可以是 `i32`、`String`、`&str`、`Vec<u8>` 和 `&[u8]` 的组合。
* 返回值可能是 `i32` 或者 `String` 或者 `Vec<u8>` 或者 void。
* 对于复杂的数据结构，比如结构体，你可以使用 JSON 字符串来传递数据。

> 支持了 JSON，你可以用任意数量的输入参数调用 Rust 函数，并返回任意数量、任意类型的结果。

[函数示例](https://github.com/second-state/wasm-learning/tree/master/nodejs/functions)中的 Rust 程序 `src/lib.rs` 演示了如何传递多个不同类型的调用参数和返回结果。

```rust
#[wasm_bindgen]
pub fn obfusticate(s: String) -> String {
  (&s).chars().map(|c| {
    match c {
      'A' ..= 'M' | 'a' ..= 'm' => ((c as u8) + 13) as char,
      'N' ..= 'Z' | 'n' ..= 'z' => ((c as u8) - 13) as char,
      _ => c
    }
  }).collect()
}

#[wasm_bindgen]
pub fn lowest_common_denominator(a: i32, b: i32) -> i32 {
  let r = lcm(a, b);
  return r;
}

#[wasm_bindgen]
pub fn sha3_digest(v: Vec<u8>) -> Vec<u8> {
  return Sha3_256::digest(&v).as_slice().to_vec();
}

#[wasm_bindgen]
pub fn keccak_digest(s: &[u8]) -> Vec<u8> {
  return Keccak256::digest(s).as_slice().to_vec();
}
```

最有意思的可能是 `create_line()` 函数。它需要两个 JSON 字符串，每一个都代表一个 `Point` 结构，返回一个 JSON 字符串代表 `Line` 结构。注意，`Point` 和 `Line` 结构都使用了 `Serialize` 和 `Deserialize` 注解，这样 Rust 编译器就会自动生成必要的代码来支持和 JSON 字符串之间的转换。

```rust
use wasm_bindgen::prelude::*;
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize, Debug)]
struct Point {
  x: f32, 
  y: f32
}

#[derive(Serialize, Deserialize, Debug)]
struct Line {
  points: Vec<Point>,
  valid: bool,
  length: f32,
  desc: String
}

#[wasm_bindgen]
pub fn create_line (p1: &str, p2: &str, desc: &str) -> String {
  let point1: Point = serde_json::from_str(p1).unwrap();
  let point2: Point = serde_json::from_str(p2).unwrap();
  let length = ((point1.x - point2.x) * (point1.x - point2.x) + (point1.y - point2.y) * (point1.y - point2.y)).sqrt();

  let valid = if length == 0.0 { false } else { true };
  let line = Line { points: vec![point1, point2], valid: valid, length: length, desc: desc.to_string() };
  return serde_json::to_string(&line).unwrap();
}

#[wasm_bindgen]
pub fn say(s: &str) -> String {
  let r = String::from("hello ");
  return r + s;
}
```

然后，让我们来检查下 JavaScript 程序 [`app.js`](https://github.com/second-state/wasm-learning/blob/master/nodejs/functions/node/app.js)，它展示了如何调用 Rust 函数。你可以看到，`String` 和 `&str` 在 JavaScript 是简单的字符串，`i32` 是数字，`Vec<u8>` 或者 `&[8]` 是 JavaScript `Uint8Array`。JavaScript 对象在传入或者从 Rust 函数结果返回需要通过 `JSON.stringify()` 或者 `JSON.parse()` 转换。

```javascript
const { say, obfusticate, lowest_common_denominator, sha3_digest, keccak_digest, create_line } = require('./functions_lib.js');

var util = require('util');
const encoder = new util.TextEncoder();
console.hex = (d) => console.log((Object(d).buffer instanceof ArrayBuffer ? new Uint8Array(d.buffer) : typeof d === 'string' ? (new util.TextEncoder('utf-8')).encode(d) : new Uint8ClampedArray(d)).reduce((p, c, i, a) => p + (i % 16 === 0 ? i.toString(16).padStart(6, 0) + '  ' : ' ') + c.toString(16).padStart(2, 0) + (i === a.length - 1 || i % 16 === 15 ?  ' '.repeat((15 - i % 16) * 3) + Array.from(a).splice(i - i % 16, 16).reduce((r, v) => r + (v > 31 && v < 127 || v > 159 ? String.fromCharCode(v) : '.'), '  ') + '\n' : ''), ''));

console.log( say("WasmEdge") );
console.log( obfusticate("A quick brown fox jumps over the lazy dog") );
console.log( lowest_common_denominator(123, 2) );
console.hex( sha3_digest(encoder.encode("This is an important message")) );
console.hex( keccak_digest(encoder.encode("This is an important message")) );

var p1 = {x:1.5, y:3.8};
var p2 = {x:2.5, y:5.8};
var line = JSON.parse(create_line(JSON.stringify(p1), JSON.stringify(p2), "A thin red line"));
console.log( line );
```

在运行 `rustwasmc` 来构建 Rust 库后，在 Node.js 环境中运行 `app.js` 会产生如下结果。

```bash
$ rustwasmc build
... Building the wasm file and JS shim file in pkg/ ...

$ node node/app.js
hello WasmEdge
N dhvpx oebja sbk whzcf bire gur ynml qbt
246
000000  57 1b e7 d1 bd 69 fb 31 9f 0a d3 fa 0f 9f 9a b5  W.çÑ½iû1..Óú...µ
000010  2b da 1a 8d 38 c7 19 2d 3c 0a 14 a3 36 d3 c3 cb  +Ú..8Ç.-<..£6ÓÃË

000000  7e c2 f1 c8 97 74 e3 21 d8 63 9f 16 6b 03 b1 a9  ~ÂñÈ.tã!Øc..k.±©
000010  d8 bf 72 9c ae c1 20 9f f6 e4 f5 85 34 4b 37 1b  Ø¿r.®Á .öäõ.4K7.

{ points: [ { x: 1.5, y: 3.8 }, { x: 2.5, y: 5.8 } ],
  valid: true,
  length: 2.2360682,
  desc: 'A thin red line' }  
```
