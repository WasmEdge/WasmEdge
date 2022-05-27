# 用 Rust 实现 JS API

对于 JavaScript 开发者来说，将 Rust 函数结合到 JavaScript API 中是非常有用的。这样做，使得开发者在用"纯 JavaScript"编写程序的同时，也可使用高性能的 Rust 函数。你可以使用 [WasmEdge Runtime](https://github.com/WasmEdge/WasmEdge) 做到这一点。

在开始之前，克隆 GitHub 仓库 [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs/)，并切换到目录 `examples/embed_js`。该操作如下所示。

```bash
git clone https://github.com/second-state/wasmedge-quickjs
cd examples/embed_js
```

> 你必须安装有 [Rust](https://www.rust-lang.org/tools/install) 和 [WasmEdge](../../start/install.md) 以构建与运行我们展示给你的示例。

对于如何将 JavaScript 嵌入到 Rust 中，`embed_js` 列出了几个不同的例子。你可以按如下的命令构建并运行这些示例。

```bash
cargo build --target wasm32-wasi --release
wasmedge --dir .:. target/wasm32-wasi/release/embed_js.wasm
```

> 注：命令行中的 `--dir .:.` 给予了 wasmedge 读取文件系统的本地目录的权限。

## 创建一个 JavaScript 函数 API

下面这个代码片段定义了一个 Rust 函数，该函数可以作为一个 API 结合到 JavaScript 解释器中。

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

下面的代码展示了如何把这个 Rust 函数添加到 JavaScript 解释器中，并为该函数的 JavaScript API 指定一个签名 `hi()`，随之在 JavaScript 代码中调用它。

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

执行的结果如下所示。

```bash
hello from rust
argv=[Int(1), Int(2), Int(3)]
return value:UnDefined
```

通过这个方法，你可以创建出一个带有自定义 API 函数的 JavaScript 解释器。这个解释器是运行在 WasmEdge 里的，并且可以通过 CLI 或者网络来执行调用了这些 API 函数的 JavaScript 代码。

## 创建一个 JavaScript 对象 API

在 JavaScript API 的设计中，我们有时需要提供一个封装了数据以及函数的对象。在下面的示例中，我们为 JavaScript API 定义了一个 Rust 函数。

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

然后，我们在 Rust 端创建一个"对象"，设置它的数据域，然后将该 Rust 函数注册为关联到该对象的 JavaScript 函数。

```rust
let mut obj = ctx.new_object();
obj.set("a", 1.into());
obj.set("b", ctx.new_string("abc").into());

let f = ctx.new_function::<ObjectFn>("anything");
obj.set("f", f.into());
```

接下来，我们使这个 Rust "对象" 能在 JavaScript 解释器中作为 JavaScript 对象 `test_obj` 使用。

```rust
ctx.get_global().set("test_obj", obj.into());
```

在 JavaScript 代码中，你可以直接将 `test_obj` 作为 API 的一部分使用了。

```rust
let code = r#"
  print('test_obj keys=',Object.keys(test_obj))
  print('test_obj.a=',test_obj.a)
  print('test_obj.b=',test_obj.b)
  test_obj.f(1,2,3,"hi")
"#;

ctx.eval_global_str(code);
```

代码执行的结果如下。

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

## 一个完整的 JavaScript 对象 API

在前面的示例中，我们简单地演示了如何用 Rust 创建 JavaScript API。在这个例子中，我们将会创建一个完整的 Rust 模块并让它成为一个 JavaScript 对象 API。这个项目在文件夹 [examples/embed_rust_module](https://github.com/second-state/wasmedge-quickjs/tree/main/examples/embed_rust_module) 中。你可以构建并将其当做一个标准的 Rust 应用，在 WasmEdge 中运行它。

```bash
cargo build --target wasm32-wasi --release
wasmedge --dir .:. target/wasm32-wasi/release/embed_rust_module.wasm
```

该对象的 Rust 实现，如下面这个模块所示。它含有数据域、构造函数、访问器、设置器以及函数。

```rust
mod point {
  use wasmedge_quickjs::*;

  #[derive(Debug)]
  struct Point(i32, i32);

  struct PointDef;

  impl JsClassDef<Point> for PointDef {
    const CLASS_NAME: &'static str = "Point\0";
    const CONSTRUCTOR_ARGC: u8 = 2;

    fn constructor(_: &mut Context, argv: &[JsValue]) -> Option<Point> {
      println!("rust-> new Point {:?}", argv);
      let x = argv.get(0);
      let y = argv.get(1);
      if let ((Some(JsValue::Int(ref x)), Some(JsValue::Int(ref y)))) = (x, y) {
        Some(Point(*x, *y))
      } else {
        None
      }
    }

    fn proto_init(p: &mut JsClassProto<Point, PointDef>) {
      struct X;
      impl JsClassGetterSetter<Point> for X {
        const NAME: &'static str = "x\0";

        fn getter(_: &mut Context, this_val: &mut Point) -> JsValue {
          println!("rust-> get x");
          this_val.0.into()
        }

        fn setter(_: &mut Context, this_val: &mut Point, val: JsValue) {
          println!("rust-> set x:{:?}", val);
          if let JsValue::Int(x) = val {
            this_val.0 = x
          }
        }
      }

      struct Y;
      impl JsClassGetterSetter<Point> for Y {
        const NAME: &'static str = "y\0";

        fn getter(_: &mut Context, this_val: &mut Point) -> JsValue {
          println!("rust-> get y");
          this_val.1.into()
        }

        fn setter(_: &mut Context, this_val: &mut Point, val: JsValue) {
          println!("rust-> set y:{:?}", val);
          if let JsValue::Int(y) = val {
            this_val.1 = y
          }
        }
      }

      struct FnPrint;
      impl JsMethod<Point> for FnPrint {
        const NAME: &'static str = "pprint\0";
        const LEN: u8 = 0;

        fn call(_: &mut Context, this_val: &mut Point, _argv: &[JsValue]) -> JsValue {
          println!("rust-> pprint: {:?}", this_val);
          JsValue::Int(1)
        }
      }

      p.add_getter_setter(X);
      p.add_getter_setter(Y);
      p.add_function(FnPrint);
    }
  }

  struct PointModule;
  impl ModuleInit for PointModule {
    fn init_module(ctx: &mut Context, m: &mut JsModuleDef) {
      m.add_export("Point\0", PointDef::class_value(ctx));
    }
  }

  pub fn init_point_module(ctx: &mut Context) {
    ctx.register_class(PointDef);
    ctx.register_module("point\0", PointModule, &["Point\0"]);
  }
}
```

在解释器的实现中，我们先调用 `point::init_point_module` 以将 Rust 模块注册到 JavaScript 的上下文中，然后运行一个使用了对象 `point` 的 JavaScript 程序。

```rust
use wasmedge_quickjs::*;
fn main() {
  let mut ctx = Context::new();
  point::init_point_module(&mut ctx);

  let code = r#"
    import('point').then((point)=>{
    let p0 = new point.Point(1,2)
    print("js->",p0.x,p0.y)
    p0.pprint()
    try{
      let p = new point.Point()
      print("js-> p:",p)
      print("js->",p.x,p.y)
      p.x=2
      p.pprint()
    } catch(e) {
      print("An error has been caught");
      print(e)
    }  
    })
  "#;

  ctx.eval_global_str(code);
  ctx.promise_loop_poll();
}
```

上面这个应用的执行结果如下所示。

```bash
rust-> new Point [Int(1), Int(2)]
rust-> get x
rust-> get y
js-> 1 2
rust-> pprint: Point(1, 2)
rust-> new Point []
js-> p: undefined
An error has been caught
TypeError: cannot read property 'x' of undefined
```
