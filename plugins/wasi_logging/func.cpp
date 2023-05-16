#include "wasi_logging/func.h"
#include "wasi_logging/enum.h"
#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals;

Expect<void> WasiLoggingLog::body(const Runtime::CallingFrame &Frame,
                                  uint32_t Level, uint32_t CxtPtr,
                                  uint32_t CxtLen, uint32_t MsgPtr,
                                  uint32_t MsgLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  char *CxtBuf = MemInst->getPointer<char *>(CxtPtr);
  char *MsgBuf = MemInst->getPointer<char *>(MsgPtr);
  bool isCxtStrExist = false;
  bool isCxtStrStderr = false;
  std::string CxtStr, MsgStr;

  // Copy Context String and Message String
  if (CxtBuf != nullptr) {
    std::copy_n(CxtBuf, CxtLen, std::back_inserter(CxtStr));
    CxtStr == "stderr"sv ? isCxtStrStderr = true : isCxtStrExist = true;
  }
  if (MsgBuf != nullptr) {
    std::copy_n(MsgBuf, MsgLen, std::back_inserter(MsgStr));
  } else {
    MsgStr = "";
  }

  // Construct Spdlog Message
  std::string SpdlogMsg;
  if (isCxtStrExist && !CxtStr.empty()) {
    SpdlogMsg = std::string_view(CxtStr + ": " + MsgStr);
  } else {
    SpdlogMsg = std::string_view(MsgStr);
  }

  // Setup Logger for Stdout or Stderr
  auto logger = isCxtStrStderr ? Env.StderrLogger : Env.StdoutLogger;

  // Print Message by Logging Level
  switch (Level) {
  case WASILOGGING::WasiLoggingLevel::Trace:
    logger->trace(SpdlogMsg);
    break;
  case WASILOGGING::WasiLoggingLevel::Debug:
    logger->debug(SpdlogMsg);
    break;
  case WASILOGGING::WasiLoggingLevel::Info:
    logger->info(SpdlogMsg);
    break;
  case WASILOGGING::WasiLoggingLevel::Warn:
    logger->warn(SpdlogMsg);
    break;
  case WASILOGGING::WasiLoggingLevel::Error:
    logger->error(SpdlogMsg);
    break;
  case WASILOGGING::WasiLoggingLevel::Critical:
    logger->critical(SpdlogMsg);
    break;
  default:
    spdlog::error("[WasiLogging] Unrecognized Logging Level: {}"sv, Level);
    spdlog::error("[WasiLogging] Trace Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Trace);
    spdlog::error("[WasiLogging] Debug Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Debug);
    spdlog::error("[WasiLogging] Info Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Info);
    spdlog::error("[WasiLogging] Warn Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Warn);
    spdlog::error("[WasiLogging] Error Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Error);
    spdlog::error("[WasiLogging] Critical Level = {}"sv,
                  WASILOGGING::WasiLoggingLevel::Critical);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

} // namespace Host
} // namespace WasmEdge