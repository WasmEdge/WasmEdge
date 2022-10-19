# WasmEdge Node.js SDK

In this tutorial, we will show you how to incorporate the WebAssembly functions written in Rust into Node.js applications on the server via the WasmEdge Node.js SDK. This approach combines Rust's **performance**, WebAssembly's **security and portability**, and JavaScript's **ease-of-use**. A typical application works like this.

* The host application is a Node.js web application written in JavaScript. It makes WebAssembly function calls.
* The WebAssembly application is written in Rust. It runs inside the WasmEdge Runtime, and is called from the Node.js web application.

> [Fork this Github repository](https://github.com/second-state/wasmedge-nodejs-starter/fork) to start coding!

## Prerequisites

To set up a high-performance Node.js environment with Rust and WebAssembly, you will need the following:

* A modern Linux distribution, such as Ubuntu Server 20.04 LTS
* [Rust language](https://www.rust-lang.org/tools/install)
* [Node.js](https://nodejs.org/en/download/package-manager/)
* [The WasmEdge Runtime](../quick_start/install.md#install-wasmedge-for-nodejs) for Node.js
* [The rustwasmc compiler toolchain](../write_wasm/rust/bindgen.md)

### Docker

The easiest way to get started is to use Docker to build a dev environment. Just [clone this template project](https://github.com/second-state/wasmedge-nodejs-starter/) to your computer and run the following Docker commands.

```bash
# Get the code
git clone https://github.com/second-state/wasmedge-nodejs-starter
cd wasmedge-nodejs-starter

# Run Docker container
docker pull wasmedge/appdev_x86_64:0.8.2
docker run -p 3000:3000 --rm -it -v $(pwd):/app wasmedge/appdev_x86_64:0.8.2

# In docker
cd /app
```

That's it! You are now ready to compile and run the code.

### Manual setup without Docker

The commands are as follows.

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
rustup override set 1.50.0

# Install Node.js and npm
curl -sL https://deb.nodesource.com/setup_14.x |  bash
sudo apt-get install -y nodejs npm

# Install rustwasmc toolchain
npm install -g rustwasmc # Append --unsafe-perm if permission denied

# OS dependencies for WasmEdge
sudo apt-get update
sudo apt-get -y upgrade
sudo apt install -y build-essential curl wget git vim libboost-all-dev llvm-dev liblld-10-dev

# Install the nodejs addon for WasmEdge
npm install wasmedge-core
npm install wasmedge-extensions
```

> The WasmEdge Runtime depends on the latest version of `libstdc++`. Ubuntu 20.04 LTS already has the latest libraries. If you are running an older Linux distribution, you have [several options to upgrade](/articles/ubuntu-req-ssvm-20200715/).

Next, clone the example source code repository.

```bash
git clone https://github.com/second-state/wasmedge-nodejs-starter
cd wasmedge-nodejs-starter
```

## Hello World

The first example is a hello world to show you how various parts of the application fit together.

### WebAssembly program in Rust

In this example, our Rust program appends the input string after “hello”. Below is the content of the Rust program `src/lib.rs`. You can define multiple external functions in this library file, and all of them will be available to the host JavaScript app via WebAssembly. Just remember to annotate each function with `#[wasm_bindgen]` so that [rustwasmc](https://github.com/second-state/rustwasmc) knows to generate the correct JavaScript to Rust interface for it when you build it.

```rust
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn say(s: String) -> String {
  let r = String::from("hello ");
  return r + &s;
}
```

Next, you can compile the Rust source code into WebAssembly bytecode and generate the accompanying JavaScript module for the Node.js host environment.

```bash
rustwasmc build
```

The result are files in the `pkg/` directory. the `.wasm` file is the WebAssembly bytecode program, and the `.js` files are for the JavaScript module.

### The Node.js host application

Next, go to the `node` folder and examine the JavaScript program `app.js`. With the generated `wasmedge_nodejs_starter_lib.js` module, it is very easy to write JavaScript to call WebAssembly functions. Below is the node application `app.js`. It simply imports the `say()` function from the generated module. The node application takes the `name` parameter from incoming an HTTP GET request, and responds with “hello `name`”.

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

Start the Node.js application server as follows.

```bash
$ node node/app.js
Server running at http://127.0.0.1:3000/
```

Then, you can test it from another terminal window.

```bash
$ curl http://127.0.0.1:3000/?name=Wasm
hello Wasm
```

## A complete web application

The next example shows a web application that computes the roots for quadratic equations. Please checkout the [full source code here](https://github.com/second-state/wasm-learning/tree/master/nodejs/quadratic).

The user enters the values for `a`, `b`, `c` on the web form, and the web application calls the web service at `/solve` to compute the roots for the quadratic equation.

```src
a*X^2 + b*X + c = 0
```

The roots for `X` are displayed in the area below the input form.

![getting-started-with-rust-function](https://www.secondstate.io/articles/getting-started-with-rust-function-01.png)

The [HTML file](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/node/public/index.html) contains the client side JavaScript to submit the web form to `/solve`, and put result into the `#roots` HTML element on the page.

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

The [Node.js application](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/node/server.js) behind the `/solve` URL endpoint is as follows. It reads the data from the input form, passes them into the `solve` function as an array, and puts the return value in the HTTP response.

```javascript
app.post('/solve', function (req, res) {
  var a = parseFloat(req.body.a);
  var b = parseFloat(req.body.b);
  var c = parseFloat(req.body.c);
  res.send(solve([a, b, c]))
})
```

The [`solve` function is written in Rust](https://github.com/second-state/wasm-learning/blob/master/nodejs/quadratic/src/lib.rs) and runs inside the WasmEdge Runtime. While the call arguments in the JavaScript side is an array of values, the Rust function receives a JSON object that encapsulates the array. In the Rust code, we first decode the JSON, perform the computation, and return the result values in a JSON string.

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

Let's try it.

```bash
rustwasmc build
npm install express # The application requires the Express framework in Node.js

node node/server.js
```

From the web browser, go to `http://ip-addr:8080/` to access this application. Note: If you are using Docker, make sure that the Docker container port 8080 is mapped to the host port 8080.

That’s it for the quadratic equation example.

## More examples

Besides passing string values between Rust and JavaScript, the `rustwasmc` tool supports the following data types.

* Rust call parameters can be any combo of `i32`, `String`, `&str`, `Vec<u8>`, and `&[u8]`
* Return value can be `i32` or `String` or `Vec<u8>` or void
* For complex data types, such as structs, you could use JSON strings to pass data.

> With JSON support, you can call Rust functions with any number of input parameters and return any number of return values of any type.

The Rust program `src/lib.rs` in the [functions example](https://github.com/second-state/wasm-learning/tree/master/nodejs/functions) demonstrates  how to pass in call arguments in various supported types, and return values.

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

Perhaps the most interesting is the `create_line()` function. It takes two JSON strings, each representing a `Point` struct, and returns a JSON string representing a `Line` struct. Notice that both the `Point` and `Line` structs are annotated with `Serialize` and `Deserialize` so that the Rust compiler automatically generates necessary code to support their conversion to and from JSON strings.

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

Next, let's examine the JavaScript program [`app.js`](https://github.com/second-state/wasm-learning/blob/master/nodejs/functions/node/app.js). It shows how to call the Rust functions. As you can see `String` and `&str` are simply strings in JavaScript, `i32` are numbers, and `Vec<u8>` or `&[8]` are JavaScript `Uint8Array`. JavaScript objects need to go through `JSON.stringify()` or `JSON.parse()` before being passed into or returned from Rust functions.

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

After running `rustwasmc` to build the Rust library, running `app.js` in Node.js environment produces the following output.

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
