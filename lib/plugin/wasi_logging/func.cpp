// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#include "plugin/wasi_logging/func.h"

#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASILogging {

using namespace std::literals;

Expect<void> Log::body(const Runtime::CallingFrame &Frame, uint32_t Level,
                       uint32_t CxtPtr, uint32_t CxtLen, uint32_t MsgPtr,
                       uint32_t MsgLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Get Buffer Pointer.
  char *CxtBuf = MemInst->getPointer<char *>(CxtPtr);
  char *MsgBuf = MemInst->getPointer<char *>(MsgPtr);
  if (CxtBuf == nullptr || MsgBuf == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Get Context and Message string_view
  std::string_view CxtSV(CxtBuf, CxtLen);
  std::string_view MsgSV(MsgBuf, MsgLen);

  // Setup Logger for Stdout or Stderr
  std::shared_ptr<spdlog::logger> Logger;
  if (CxtSV == "stdout"sv || CxtSV == ""sv) {
    Logger = Env.StdoutLogger;
  } else if (CxtSV == "stderr"sv) {
    Logger = Env.StderrLogger;
  } else {
    if (CxtSV != Env.getLogFileName()) {
      try {
        spdlog::drop(Env.getLogRegName());
        Env.FileLogger =
            spdlog::basic_logger_mt(Env.getLogRegName(), std::string(CxtSV));
        Env.FileLogger->set_level(spdlog::level::trace);
        Env.FileLogger->set_pattern(Env.DefFormat);
        Env.setLogFileName(CxtSV);
      } catch (const spdlog::spdlog_ex &Ex) {
        spdlog::error("[WasiLogging] Cannot log into file: {}"sv, Ex.what());
        return Unexpect(ErrCode::Value::HostFuncError);
      }
    }
    Logger = Env.FileLogger;
  }

  // Print Message by Logging Level
  switch (static_cast<LogLevel>(Level)) {
  case LogLevel::Trace:
    Logger->trace(MsgSV);
    break;
  case LogLevel::Debug:
    Logger->debug(MsgSV);
    break;
  case LogLevel::Info:
    Logger->info(MsgSV);
    break;
  case LogLevel::Warn:
    Logger->warn(MsgSV);
    break;
  case LogLevel::Error:
    Logger->error(MsgSV);
    break;
  case LogLevel::Critical:
    Logger->critical(MsgSV);
    break;
  default:
    spdlog::error("[WasiLogging] Unrecognized Logging Level: {}"sv, Level);
    spdlog::error("[WasiLogging] Trace Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Trace));
    spdlog::error("[WasiLogging] Debug Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Debug));
    spdlog::error("[WasiLogging] Info Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Info));
    spdlog::error("[WasiLogging] Warn Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Warn));
    spdlog::error("[WasiLogging] Error Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Error));
    spdlog::error("[WasiLogging] Critical Level = {}"sv,
                  static_cast<uint32_t>(LogLevel::Critical));
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return {};
}

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
