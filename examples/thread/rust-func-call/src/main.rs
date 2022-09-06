
pub extern "C" fn my_thread(id: i32) {
  println!("Hello from thread id: {}", id);
}

fn caller(f: extern fn (arg1: i32), id: i32) {
  f(id);
}

fn main() {
  println!("Thread {:x}", my_thread as usize);
  caller(my_thread, 1337);
}
