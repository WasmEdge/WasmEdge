// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

// BUILTIN-PLUGIN: Temporarily move the wasi-logging plugin sources here until
// the new plugin architecture is ready in 0.15.0.

#include "plugin/wasi_logging/func.h"

#include "common/span.h"
#include "host/filesystem.h"

#include <spdlog/pattern_formatter.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace WASILogging {

using namespace std::literals;

namespace {

/// Append the line to the file of the path in the filesystem of the caller,
/// creating the directories of the path if missing.
WASI::WasiExpect<void> appendFile(const Host::FileSystem *FS,
                                  std::string_view Path,
                                  std::string_view Line) noexcept {
  if (FS == nullptr) {
    return WASI::WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  }
  auto File = FS->open(Path, __WASI_OFLAGS_CREAT, __WASI_RIGHTS_FD_WRITE,
                       __WASI_FDFLAGS_APPEND);
  if (!File && File.error() == __WASI_ERRNO_NOENT) {
    // Go up to the deepest directory of the path which exists or can be
    // created, then create the ones below it.
    const auto End = Path.rfind('/');
    auto Pos = End;
    while (Pos != std::string_view::npos) {
      auto Res = FS->createDirectory(Path.substr(0, Pos));
      if (Res || Res.error() == __WASI_ERRNO_EXIST) {
        break;
      }
      if (Res.error() != __WASI_ERRNO_NOENT || Pos == 0) {
        return WASI::WasiUnexpect(Res);
      }
      Pos = Path.rfind('/', Pos - 1);
    }
    while (Pos != std::string_view::npos && Pos < End) {
      Pos = Path.find('/', Pos + 1);
      if (auto Res = FS->createDirectory(Path.substr(0, Pos));
          !Res && Res.error() != __WASI_ERRNO_EXIST) {
        return WASI::WasiUnexpect(Res);
      }
    }
    File = FS->open(Path, __WASI_OFLAGS_CREAT, __WASI_RIGHTS_FD_WRITE,
                    __WASI_FDFLAGS_APPEND);
  }
  if (!File) {
    return WASI::WasiUnexpect(File);
  }
  return (*File)->write(Span<const uint8_t>(
      reinterpret_cast<const uint8_t *>(Line.data()), Line.size()));
}

} // namespace

Expect<void> Log::body(const Runtime::CallingFrame &Frame, uint32_t Level,
                       uint32_t CxtPtr, uint32_t CxtLen, uint32_t MsgPtr,
                       uint32_t MsgLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Get Context and Message string_view with full-extent bounds check.
  auto CxtSV = MemInst->getStringView(CxtPtr, CxtLen);
  auto MsgSV = MemInst->getStringView(MsgPtr, MsgLen);
  if (CxtSV.data() == nullptr || MsgSV.data() == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  // Get the spdlog level of the Logging Level
  spdlog::level::level_enum SpdlogLevel = spdlog::level::off;
  switch (static_cast<LogLevel>(Level)) {
  case LogLevel::Trace:
    SpdlogLevel = spdlog::level::trace;
    break;
  case LogLevel::Debug:
    SpdlogLevel = spdlog::level::debug;
    break;
  case LogLevel::Info:
    SpdlogLevel = spdlog::level::info;
    break;
  case LogLevel::Warn:
    SpdlogLevel = spdlog::level::warn;
    break;
  case LogLevel::Error:
    SpdlogLevel = spdlog::level::err;
    break;
  case LogLevel::Critical:
    SpdlogLevel = spdlog::level::critical;
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

  // Print Message to Stdout or Stderr
  if (CxtSV == "stdout"sv || CxtSV == ""sv) {
    Env.StdoutLogger->log(SpdlogLevel, MsgSV);
    return {};
  }
  if (CxtSV == "stderr"sv) {
    Env.StderrLogger->log(SpdlogLevel, MsgSV);
    return {};
  }

  // Any other context names a file in the filesystem of the caller, which is
  // appended with the line formatted as on the console.
  spdlog::memory_buf_t Line;
  spdlog::pattern_formatter(Env.DefFormat)
      .format(spdlog::details::log_msg(
                  spdlog::string_view_t(), SpdlogLevel,
                  spdlog::string_view_t(MsgSV.data(), MsgSV.size())),
              Line);
  if (auto Res = appendFile(Frame.getFileSystem(), CxtSV,
                            std::string_view(Line.data(), Line.size()));
      !Res) {
    // Without the right to the file, the context is a label of the message.
    if (!Env.FileWarned.exchange(true)) {
      spdlog::warn("[WasiLogging] Cannot log into file {}: {} The file must be "
                   "under a writable directory preopened for the module. "
                   "Print the message to stdout instead."sv,
                   CxtSV, Res.error());
    }
    Env.StdoutLogger->log(SpdlogLevel, "[{}] {}"sv, CxtSV, MsgSV);
  }
  return {};
}

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
