pub mod logging;

fn title_bar(context: &str, message: &str) {
  logging::log(logging::Level::Info, "", "===================================");
  logging::log(logging::Level::Info, context, message);
  logging::log(logging::Level::Info, "", "-----------------------------------");
}

fn demo_template(context: &str) {
  logging::log(logging::Level::Trace, context, "Trace Level Message");
  logging::log(logging::Level::Debug, context, "Debug Level Message");
  logging::log(logging::Level::Info, context, "Info Level Message");
  logging::log(logging::Level::Warn, context, "Warn Level Message");
  logging::log(logging::Level::Error, context, "Error Level Message");
  logging::log(logging::Level::Critical, context, "Critical Level Message");
}

pub fn wasi_logging_demo() {
  title_bar("Demo 1", "Stdout Message Demo");
  demo_template("Context");
  title_bar("Demo 2", "Stdout Message Without Context");
  demo_template("");
  title_bar("Demo 3", "Stderr Message Demo");
  demo_template("stderr");
}