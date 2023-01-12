# Table and FuncRef

In this example, we'll present how to use [Table](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.Table.html) and [FuncRef](https://wasmedge.github.io/WasmEdge/wasmedge_sdk/struct.FuncRef.html) stored in a slot of a `Table` instance to implement indirect function invocation.

> The code in the following example is verified on
>
> * wasmedge-sdk v0.5.0
> * wasmedge-sys v0.10.0
> * wasmedge-types v0.3.0

Let's start off by getting all imports right away so you can follow along

```rust
// If the version of rust used is less than v1.63, please uncomment the follow attribute.
// #![feature(explicit_generic_args_with_impl_trait)]

#![feature(never_type)]

use wasmedge_sdk::{
    config::{CommonConfigOptions, ConfigBuilder},
    error::HostFuncError,
    host_function, params,
    types::Val,
    Caller, Executor, Func, ImportObjectBuilder, RefType, Store, Table, TableType, ValType,
    WasmVal, WasmValue,
};
```

## Define host function

In this example we defines a native function `real_add` that takes two numbers and returns their sum. This function will be registered as a host function into WasmEdge runtime environment

```rust
#[host_function]
fn real_add(_caller: &Caller, input: Vec<WasmValue>) -> Result<Vec<WasmValue>, HostFuncError> {
    println!("Rust: Entering Rust function real_add");

    if input.len() != 2 {
        return Err(HostFuncError::User(1));
    }

    let a = if input[0].ty() == ValType::I32 {
        input[0].to_i32()
    } else {
        return Err(HostFuncError::User(2));
    };

    let b = if input[1].ty() == ValType::I32 {
        input[1].to_i32()
    } else {
        return Err(HostFuncError::User(3));
    };

    let c = a + b;
    println!("Rust: calcuating in real_add c: {:?}", c);

    println!("Rust: Leaving Rust function real_add");
    Ok(vec![WasmValue::from_i32(c)])
}
```

## Register Table instance

The first thing we need to do is to create a `Table` instance. After that, we register the table instance along with an import module into the WasmEdge runtime environment. Now let's see the code.

```rust
// create an executor
let config = ConfigBuilder::new(CommonConfigOptions::default()).build()?;
let mut executor = Executor::new(Some(&config), None)?;

// create a store
let mut store = Store::new()?;

// create a table instance
let result = Table::new(TableType::new(RefType::FuncRef, 10, Some(20)));
assert!(result.is_ok());
let table = result.unwrap();

// create an import object
let import = ImportObjectBuilder::new()
    .with_table("my-table", table)?
    .build("extern")?;

// register the import object into the store
store.register_import_module(&mut executor, &import)?;
```

In the code snippet above, we create a `Table` instance with the initial size of `10` and the maximum size of 20. The element type of the `Table` instance is `reference to function`.

## Store a function reference into Table

In the previous steps, we defined a native function `real_add` and registered a `Table` instance named `my-table` into the runtime environment. Now we'll save a reference to `read_add` function to a slot of `my-table`.

```rust
// get the imported module instance
let instance = store
    .module_instance("extern")
    .expect("Not found module instance named 'extern'");

// get the exported table instance
let mut table = instance
    .table("my-table")
    .expect("Not found table instance named 'my-table'");

// create a host function
let host_func = Func::wrap::<(i32, i32), i32, !>(Box::new(real_add), None)?;

// store the reference to host_func at the given index of the table instance
table.set(3, Val::FuncRef(Some(host_func.as_ref())))?;
```

We save the reference to `host_func` into the third slot of `my-table`. Next, we can retrieve the function reference from the table instance by index and call the function via its reference.

## Call native function via `FuncRef`

```rust
// retrieve the function reference at the given index of the table instance
let value = table.get(3)?;
if let Val::FuncRef(Some(func_ref)) = value {
    // get the function type by func_ref
    let func_ty = func_ref.ty()?;

    // arguments
    assert_eq!(func_ty.args_len(), 2);
    let param_tys = func_ty.args().unwrap();
    assert_eq!(param_tys, [ValType::I32, ValType::I32]);

    // returns
    assert_eq!(func_ty.returns_len(), 1);
    let return_tys = func_ty.returns().unwrap();
    assert_eq!(return_tys, [ValType::I32]);

    // call the function by func_ref
    let returns = func_ref.call(&mut executor, params!(1, 2))?;
    assert_eq!(returns.len(), 1);
    assert_eq!(returns[0].to_i32(), 3);
}
```

The complete code of this example can be found in [table_and_funcref.rs](https://github.com/WasmEdge/WasmEdge/blob/master/bindings/rust/wasmedge-sdk/examples/table_and_funcref.rs).
