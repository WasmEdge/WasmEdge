wit_bindgen::generate!({ path: "wit" });

use wasi::logging::logging::{
    Level, 
    log,
};

fn title_bar(context: &str, message: &str) {
  log(Level::Info, "", "===================================");
  log(Level::Info, context, message);
  log(Level::Info, "", "-----------------------------------");
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
  title_bar("Demo 1", "Stdout Message Demo");
  demo_template("Context");
  title_bar("Demo 2", "Stdout Message Without Context");
  demo_template("");
  title_bar("Demo 3", "Stderr Message Demo");
  demo_template("stderr");
}