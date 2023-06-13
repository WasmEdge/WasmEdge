# Develop Plugin Rust SDK with witc

Once you complete C++ plugin code, you can use witc[^1] to generate Rust Plugin SDK

## Example wasmedge_opencvmini

Consider you get a file `wasmedge_opencvmini.wit` with below content

```wit
imdecode: func(buf: list<u8>) -> u32
imshow: func(window-name: string, mat-key: u32) -> unit
waitkey: func(delay: u32) -> unit
```

Using witc can generate Rust plugin code for it

```shell
witc plugin wasmedge_opencvmini.wit
```

Now, you will create a SDK crate by

```shell
cargo new --lib opencvmini-sdk && cd opencvmini-sdk
```

witc put rust code to stdout, therefore, you might like to create a new module file for generated code

```shell
witc plugin wasmedge_opencvmini.wit > src/generated.rs
```

Finally, you write down `mod generated` in `src/lib.rs` to access the code, and write some wrappers

```rust
mod generated;

pub fn imdecode(buf: &[u8]) -> u32 {
    unsafe { generated::imdecode(buf.as_ptr(), buf.len()) }
}
pub fn imshow(window_name: &str, mat_key: u32) -> () {
    unsafe { generated::imshow(window_name.as_ptr(), window_name.len(), mat_key) }
}
pub fn waitkey(delay: u32) -> () {
    unsafe { generated::waitkey(delay) }
}
```

[^1]: <https://github.com/second-state/witc>
