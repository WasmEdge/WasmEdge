use wasmedge_sys::{Executor, FuncType, Function, Store};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // create a FuncType
    let func_ty = FuncType::create(vec![], vec![])?;
    let mut store = Store::create().expect("123");

    // create a host function
    let async_host_func = Function::create_async(
        &func_ty,
        Box::new(|_frame, _input, _data| {
            Box::new(async {
                println!("Hello, world!");
                tokio::time::sleep(std::time::Duration::from_secs(1)).await;
                // Wrap the future with a `Timeout` set to expire in 10 milliseconds.
                if let Err(_) = tokio::time::timeout(std::time::Duration::from_millis(100), async {
                    tokio::time::sleep(std::time::Duration::from_secs(1)).await;
                })
                .await
                {
                    println!("did not receive value within 100 ms");
                }
                println!("Rust: Leaving Rust function real_add");
                println!("Hello, world after sleep!");
                Ok(vec![])
            })
        }),
        Some(&mut store),
        0,
    )?;

    // run this function
    let mut executor = Executor::create(None, None)?;

    let res = async_host_func.call_async(&mut executor, vec![]).await?;
    dbg!(res);
    Ok(())
}
