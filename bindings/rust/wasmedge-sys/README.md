## `-sys` library design principles

In general, the `-sys` library should keep only `unsafe` C interface bindings and should not have redundant security abstractions.

However, for a pure Rust SDK like `wasmedge-rs` that we will eventually build, there should not be too many C binding interfaces. So,  the `wasmedge-sys` library uses a thin layer of Rust wrappers, exposing only the appropriate interfaces for the upper layer of `wasmedge-rs` to abstract a more usable pure Rust SDK.

The interfaces exposed by the `-sys` library are supposed to be stable. That is, when the C interface changes, only the `-sys` library needs to be changed, not the upper-layer SDK.

## Library interface description

- Error handling: return `ErrReport` structures uniformly, and do further Rust-style error handling in the upper-level `-wasmedge-rs` SDK.
- Incorporate basic types into the `-sys` library, e.g. Strings/Value etc., which the upper level SDK just needs to use.
- Configuration/module loading/creating VMs/starting VMs to compute return results, these are put into the `-sys` library as the base interface for the SDK.
- The base interface, which involves creation or initialization, uses basic "encapsulation" and is not open to downstream modifications to ensure the stability of the base interface.
- The corresponding `C-API` header file and the corresponding Rust binding interface are recorded in the Dosc directory as documentation.