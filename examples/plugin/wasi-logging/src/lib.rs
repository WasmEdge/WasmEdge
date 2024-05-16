wit_bindgen::generate!({ path: "wit" });

use wasi::logging::logging::{
    Level, 
    log,
};

fn title_bar(message: &str) {
  log(Level::Info, "stdout", "===================================");
  log(Level::Info, "stdout", message);
  log(Level::Info, "stdout", "-----------------------------------");
}

fn demo_template(context: &str) {
  log(Level::Trace, context, "Trace Level Message");
  log(Level::Debug, context, "Debug Level Message");
  log(Level::Info, context, "Info Level Message");
  log(Level::Warn, context, "Warn Level Message");
  log(Level::Error, context, "Error Level Message");
  log(Level::Critical, context, "Critical Level Message");
}

pub fn wasi_logging_demo() {
  title_bar("Stdout Message Demo");
  demo_template("stdout");
  title_bar("Stderr Message Demo");
  demo_template("stderr");
  title_bar("File Message Demo: log/output.log");
  demo_template("log/output.log");
  title_bar("File Message Demo: log/output2.log");
  demo_template("log/output2.log");
  title_bar("File Message Demo: continue to log/output.log");
  demo_template("log/output.log");
}
