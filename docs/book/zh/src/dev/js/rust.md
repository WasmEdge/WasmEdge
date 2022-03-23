# 用 Rust 实现 JS API

对于JavaScript开发人员来说，将Rust函数集成到 JavaScript API 中是很有用的。 这使得开发人员能够用"纯JavaScript"编写程序，但仍然可以利用高性能的Rust函数。 可以使用 [WasmEdge Runtime](https://github.com/WasmEdge/WasmEdge) 做到这一点。

查看 [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs/) Github repo 并更改为 `examples/embed_js` 文件夹以进行后续操作。

```bash
git clone https://github.com/second-state/wasmedge-quickjs
cd examples/embed_js
```

> 您必须安装 [Rust](https://www.rust-lang.org/tools/install) 和 [WasmEdge](../../start/install.md) 才能构建和运行我们展示的示例。

`embed_js` 演示展示了如何在 Rust 中嵌入 JavaScript 的几个不同示例。 您可以按如下方式构建和运行所有示例。

```bash
cargo build --target wasm32-wasi --release
wasmedge --dir .:. target/wasm32-wasi/release/embed_js.wasm
```

> 注意：命令行中的 `--dir .:.` 是授予wasmedge 读取文件系统中本地目录的权限。

## 创建一个 JavaScript 函数 API

下面的代码片段定义了一个可以作为 API 并入 JavaScript 解释器的 Rust 函数。

```rust
fn run_rust_function(ctx: &mut Context) {

  struct HelloFn;
  impl JsFn for HelloFn {
    fn call(_ctx: &mut Context, _this_val: JsValue, argv: &[JsValue]) -> JsValue {
      println!("hello from rust");
      println!("argv={:?}", argv);
      JsValue::UnDefined
    }
  }
  
  ...
}
```

下面的代码片段展示了如何将这个 Rust 函数添加到 JavaScript 解释器中，命名为 `hi()` 作为它的 JavaScript API，然后从 JavaScript 代码中调用它。

```rust
fn run_rust_function(ctx: &mut Context) {
  ...
  
  let f = ctx.new_function::<HelloFn>("hello");
  ctx.get_global().set("hi", f.into());
  let code = r#"hi(1,2,3)"#;
  let r = ctx.eval_global_str(code);
  println!("return value:{:?}", r);
}
```

执行结果如下。

```bash
hello from rust
argv=[Int(1), Int(2), Int(3)]
return value:UnDefined
```

使用这种方法，您可以创建具有自定义 API 函数的 JavaScript 解释器。 解释器在 WasmEdge 内部运行，并且可以从 CLI 或网络执行调用此类 API 函数的 JavaScript 代码。

## 创建一个 JavaScript 对象 API

在 JavaScript API 设计中，我们有时需要提供一个同时封装数据和函数的对象。 在下面的示例中，我们为 JavaScript API 定义了一个 Rust 函数。

```rust
fn rust_new_object_and_js_call(ctx: &mut Context) {
  struct ObjectFn;
  impl JsFn for ObjectFn {
    fn call(_ctx: &mut Context, this_val: JsValue, argv: &[JsValue]) -> JsValue {
      println!("hello from rust");
      println!("argv={:?}", argv);
      if let JsValue::Object(obj) = this_val {
        let obj_map = obj.to_map();
        println!("this={:#?}", obj_map);
      }
      JsValue::UnDefined
    }
  }

  ...
}
```

然后我们在 Rust 端创建一个“对象”，设置它的数据字段，然后将 Rust 函数注册为与对象关联的 JavaScript 函数。

```rust
let mut obj = ctx.new_object();
obj.set("a", 1.into());
obj.set("b", ctx.new_string("abc").into());

let f = ctx.new_function::<ObjectFn>("anything");
obj.set("f", f.into());
```

接下来，我们将 Rust“对象”作为 JavaScript 对象 `test_obj` 在 JavaScript 解释器中可用。

```rust
ctx.get_global().set("test_obj", obj.into());
```

在 JavaScript 代码中，您现在可以直接使用 `test_obj` 作为 API 的一部分。

```rust
let code = r#"
  print('test_obj keys=',Object.keys(test_obj))
  print('test_obj.a=',test_obj.a)
  print('test_obj.b=',test_obj.b)
  test_obj.f(1,2,3,"hi")
"#;

ctx.eval_global_str(code);
```

执行结果如下。

```bash
test_obj keys= a,b,f
test_obj.a= 1
test_obj.b= abc
hello from rust
argv=[Int(1), Int(2), Int(3), String(JsString(hi))]
this=Ok(
  {
    "a": Int(
      1,
    ),
    "b": String(
      JsString(
        abc,
      ),
    ),
    "f": Function(
      JsFunction(
        function anything() {
          [native code]
        },
      ),
    ),
  },
)
```
