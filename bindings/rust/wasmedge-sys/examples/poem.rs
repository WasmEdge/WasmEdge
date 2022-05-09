use poem::{get, handler, listener::TcpListener, web::Query, Route, Server};
use wasmedge_sys::{Config, Module, Value, Vm};

#[derive(serde::Deserialize)]
struct AddReq {
    a: i32,
    b: i32,
}

#[handler]
fn call_add(Query(AddReq { a, b }): Query<AddReq>) -> String {
    let config = Config::create().expect("Failed to create Config");

    // Source code of add.wasm
    // #[no_mangle]
    // pub extern "C" fn add(a: i32, b: i32) -> i32 {
    //     a + b
    // }
    let mut module =
        Module::create_from_file(&config, "examples/add.wasm").expect("Failed to create Module");

    let mut vm = Vm::create(Some(&config), None)
        .expect("Failed to create VM")
        .load_wasm_from_module(&mut module)
        .expect("Failed to register wasm")
        .validate()
        .expect("Failed to validate vm")
        .instantiate()
        .expect("Failed to instantiate vm");

    match vm.run_function("add", [a.into(), b.into()]) {
        Ok(v) => {
            let res = v.collect::<Vec<Value>>()[0];
            format!("a + b = {:?}", res)
        }
        Err(e) => format!("Failed to execute function: {:?}", e),
    }
}

#[tokio::main]
async fn main() -> Result<(), std::io::Error> {
    // http://127.0.0.1:3000/add?a=1&b=2
    // a + b = I32(3)
    Server::new(TcpListener::bind("127.0.0.1:3000"))
        .run(Route::new().at("/add", get(call_add)))
        .await
}
