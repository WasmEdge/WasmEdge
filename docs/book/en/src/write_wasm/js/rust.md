# Use Rust to implement JS API

For JavaScript developers, incorporating Rust functions into JavaScript APIs is useful. That enables developers to write programs in "pure JavaScript" and yet still take advantage of the high performance Rust functions. With the [WasmEdge Runtime](https://github.com/WasmEdge/WasmEdge), you can do exactly that.

> The [internal_module](https://github.com/second-state/wasmedge-quickjs/tree/main/src/internal_module) folder in the official WasmEdge QuickJS distribution provides Rust-based implementations of some built-in JavaScript API functions. Those functions typically require interactions with host functions in the WasmEdge runtime (e.g., networking and tensorflow), and hence cannot be accessed by pure JavaScript implementations in [modules](modules.md).

Check out the [wasmedge-quickjs](https://github.com/second-state/wasmedge-quickjs/) Github repo and change to the `examples/embed_js` folder to follow along.

```bash
git clone https://github.com/second-state/wasmedge-quickjs
cd examples/embed_js
```

> You must have [Rust](https://www.rust-lang.org/tools/install) and [WasmEdge](../../quick_start/install.md) installed to build and run the examples we show you.

The `embed_js` demo showcases several different examples on how to embed JavaScript inside Rust. You can build and run all the examples as follows.

```bash
cargo build --target wasm32-wasi --release
wasmedge --dir .:. target/wasm32-wasi/release/embed_js.wasm
```

> Note: The `--dir .:.` on the command line is to give wasmedge permission to read the local directory in the file system.

## Create a JavaScript function API

The following code snippet defines a Rust function that can be incorporate into the JavaScript interpreter as an API.

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

The following code snippet shows how to add this Rust function into the JavaScript interpreter, give a name `hi()` as its JavaScript API, and then call it from JavaScript code.

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

The execution result is as follows.

```bash
hello from rust
argv=[Int(1), Int(2), Int(3)]
return value:UnDefined
```

Using this approach, you can create a JavaScript interpreter with customized API functions. The interpreter runs inside WasmEdge, and can execute JavaScript code, which calls such API functions, from CLI or the network.

## Create a JavaScript object API

In the JavaScript API design, we sometimes need to provide an object that encapsulates both data and function. In the following example, we define a Rust function for the JavaScript API.

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

We then create an "object" on the Rust side, set its data fields, and then register the Rust function as a JavaScript function associated with the objects.

```rust
let mut obj = ctx.new_object();
obj.set("a", 1.into());
obj.set("b", ctx.new_string("abc").into());

let f = ctx.new_function::<ObjectFn>("anything");
obj.set("f", f.into());
```

Next, we make the Rust "object" available as JavaScript object `test_obj` in the JavaScript interpreter.

```rust
ctx.get_global().set("test_obj", obj.into());
```

In the JavaScript code, you can now directly use `test_obj` as part of the API.

```rust
let code = r#"
  print('test_obj keys=',Object.keys(test_obj))
  print('test_obj.a=',test_obj.a)
  print('test_obj.b=',test_obj.b)
  test_obj.f(1,2,3,"hi")
"#;

ctx.eval_global_str(code);
```

The execution result is as follows.

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

## A complete JavaScript object API

In the previous example, we demonstrated simple examples to create JavaScript APIs from Rust. In this example, we will create a complete Rust module and make it available as a JavaScript object API. The project is in the [examples/embed_rust_module](https://github.com/second-state/wasmedge-quickjs/tree/main/examples/embed_rust_module) folder. You can build and run it as a standard Rust application in WasmEdge.

```bash
cargo build --target wasm32-wasi --release
wasmedge --dir .:. target/wasm32-wasi/release/embed_rust_module.wasm
```

The Rust implementation of the object is a module as follows. It has data fields, constructor, getters and setters, and functions.

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

In the interpreter implementation, we call `point::init_point_module` first to register the Rust module with the JavaScript context, and then we can run a JavaScript program that simply use the `point` object.

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

The execution result from the above application is as follows.

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

## Code reuse

Using the Rust API, we could create JavaScript classes that inherit (or extend) from existing classes. That allows developers to create complex JavaScript APIs using Rust by building on existing solutions. You can see [an example here](https://github.com/second-state/wasmedge-quickjs/blob/main/examples/js_extend.rs).

Next, you can see the Rust code in the [internal_module](https://github.com/second-state/wasmedge-quickjs/tree/main/src/internal_module) folder for more examples on how to implement common JavaScript built-in functions including [Node.js](nodejs.md) APIs.
