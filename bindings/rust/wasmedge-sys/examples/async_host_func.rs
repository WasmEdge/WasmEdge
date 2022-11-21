//! This example demonstrates how to create host functions and call them asynchronously.
//!
//! To run this example, use the following command:
//!
//! ```bash
//! cd <wasmedge-root-dir>/bindings/rust/
//!
//! cargo run -p wasmedge-sys --features async --example async_host_func
//! ```

#[cfg(feature = "async")]
use wasmedge_sys::{Executor, FuncType, Function};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "async")]
    {
        // create a FuncType
        let func_ty = FuncType::create(vec![], vec![])?;

        // create a host function
        let async_host_func = Function::create_async(
            &func_ty,
            |_frame, _input| {
                Box::new(async {
                    println!("Hello, world!");
                    tokio::time::sleep(std::time::Duration::from_secs(1)).await;
                    // Wrap the future with a `Timeout` set to expire in 10 milliseconds.
                    let res = tokio::time::timeout(std::time::Duration::from_millis(100), async {
                        tokio::time::sleep(std::time::Duration::from_secs(1)).await;
                    })
                    .await;
                    if res.is_err() {
                        println!("did not receive value within 100 ms");
                    }
                    println!("Rust: Leaving Rust function real_add");
                    println!("Hello, world after sleep!");
                    Ok(vec![])
                })
            },
            0,
        )?;

        // run this function
        let mut executor = Executor::create(None, None)?;

        async_host_func.call_async(&mut executor, vec![]).await?;
    }

    Ok(())
}
