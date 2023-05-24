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

  // Get Buffer Pointer
  char *CxtBuf = MemInst->getPointer<char *>(CxtPtr);
  char *MsgBuf = MemInst->getPointer<char *>(MsgPtr);
  if (CxtBuf == nullptr || MsgBuf == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Copy Context String and Message String
  std::string CxtStr, MsgStr;
  std::copy_n(CxtBuf, CxtLen, std::back_inserter(CxtStr));
  std::copy_n(MsgBuf, MsgLen, std::back_inserter(MsgStr));

  // Setup Logger for Stdout or Stderr
  CxtStr == "stderr"sv ? Env.isCxtStrStderr = true : Env.isCxtStrStderr = false;
  auto logger = Env.isCxtStrStderr ? Env.StderrLogger : Env.StdoutLogger;

  // Construct Spdlog Message
  std::string SpdlogMsg;
  if (!CxtStr.empty()) {
    SpdlogMsg = CxtStr + ": " + MsgStr;
  } else {
    SpdlogMsg = MsgStr;
  }

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
                  static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Trace));
    spdlog::error("[WasiLogging] Debug Level = {}"sv,
                  static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Debug));
    spdlog::error("[WasiLogging] Info Level = {}"sv,
                  static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Info));
    spdlog::error("[WasiLogging] Warn Level = {}"sv,
                  static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Warn));
    spdlog::error("[WasiLogging] Error Level = {}"sv,
                  static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Error));
    spdlog::error(
        "[WasiLogging] Critical Level = {}"sv,
        static_cast<uint32_t>(WASILOGGING::WasiLoggingLevel::Critical));
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

} // namespace Host
} // namespace WasmEdge