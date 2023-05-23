mod ffi {
  #[link(wasm_import_module = "logging")]
  extern "C" {
    pub fn log(
      Level: u32,
      CxtPtr: u32,
      CxtLen: u32,
      MsgPtr: u32,
      MsgLen: u32,
    );
  }
}

pub enum Level {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Critical,
}

pub fn log(
  level: Level,
  context: &str,
  message: &str,
) {
  unsafe {
    let cxt_ptr = context.as_ptr() as u32;
    let cxt_len = context.len() as u32;
    let msg_ptr = message.as_ptr() as u32;
    let msg_len = message.len() as u32;
    ffi::log(match level {
      Level::Trace => 0,
      Level::Debug => 1,
      Level::Info => 2,
      Level::Warn => 3,
      Level::Error => 4,
      Level::Critical => 5,
    }, cxt_ptr, cxt_len, msg_ptr, msg_len);
  }
}