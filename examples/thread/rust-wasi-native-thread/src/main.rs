use libc::{c_void};
use std::ptr::null_mut;

pub type pthread_t = i32;
pub type size_t = usize;
pub type pthread_key_t = u32;
pub struct pthread_attr_t { /* private fields */ }

#[link(wasm_import_module = "wasi_snapshot_preview1")]
extern {
  pub fn pthread_attr_getstack(attr: *const pthread_attr_t, stackaddr: *mut *mut c_void,
                               stacksize: *mut size_t) -> i32;
  pub fn pthread_attr_setstack(attr: *mut pthread_attr_t, stackaddr: *mut c_void,
                               stacksize: size_t) -> i32;
  pub fn pthread_create(newthread: *mut pthread_t, attr: *const pthread_attr_t,
                        start_routine: extern fn (data: *mut c_void) -> *mut c_void,
                        arg: *mut c_void) -> i32;
  pub fn pthread_exit(retval: *mut c_void);
  pub fn pthread_getspecific(key: pthread_key_t) -> *mut c_void;
  pub fn pthread_join(th: pthread_t, thread_return: *mut *mut c_void) -> i32;
  pub fn pthread_setspecific(key: pthread_key_t, pointer: *const c_void) -> i32;
}

struct State {
  tid: i32,
}

pub extern "C" fn my_thread(data: *mut c_void) -> *mut c_void {
  let data: &mut State = unsafe { &mut *(data as *mut State) };
  println!("Hello from thread id: {}", data.tid);
  return null_mut();
}


fn main() {
  let mut threads: Vec<pthread_t> = Vec::new();
  let mut states: Vec<State> = Vec::new();

  for i in 0..10 {
    let state = State {tid : i};
    states.push(state);
  }

  for i in 0..10 {
    unsafe {
      let mut thread: pthread_t = std::mem::zeroed();
      threads.push(thread);

      let state_ptr: *mut c_void = (&mut states[i]) as *mut _ as *mut c_void;
      pthread_create(&mut thread, null_mut(), my_thread, state_ptr);
    }
  }

  for thread in threads {
    unsafe {
      pthread_join(thread, null_mut());
    }
  }
}
