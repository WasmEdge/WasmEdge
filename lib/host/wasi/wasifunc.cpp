// SPDX-License-Identifier: Apache-2.0

#include "host/wasi/wasifunc.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "runtime/instance/memory.h"
#include "vfs-linux.ipp"
#include "vfs-macos.ipp"
#include "vfs-windows.ipp"
#include <chrono>
#include <csignal>
#include <ctime>
#include <limits>
#include <numeric>
#include <random>
#include <string_view>

namespace {

static constexpr int signal2SysSignal(uint8_t Signal) noexcept {
  switch (Signal) {
  case __WASI_SIGNAL_HUP:
    return SIGHUP;
  case __WASI_SIGNAL_INT:
    return SIGINT;
  case __WASI_SIGNAL_QUIT:
    return SIGQUIT;
  case __WASI_SIGNAL_ILL:
    return SIGILL;
  case __WASI_SIGNAL_TRAP:
    return SIGTRAP;
  case __WASI_SIGNAL_ABRT:
    return SIGABRT;
  case __WASI_SIGNAL_BUS:
    return SIGBUS;
  case __WASI_SIGNAL_FPE:
    return SIGFPE;
  case __WASI_SIGNAL_KILL:
    return SIGKILL;
  case __WASI_SIGNAL_USR1:
    return SIGUSR1;
  case __WASI_SIGNAL_SEGV:
    return SIGSEGV;
  case __WASI_SIGNAL_USR2:
    return SIGUSR2;
  case __WASI_SIGNAL_PIPE:
    return SIGPIPE;
  case __WASI_SIGNAL_ALRM:
    return SIGALRM;
  case __WASI_SIGNAL_TERM:
    return SIGTERM;
  case __WASI_SIGNAL_CHLD:
    return SIGCHLD;
  case __WASI_SIGNAL_CONT:
    return SIGCONT;
  case __WASI_SIGNAL_STOP:
    return SIGSTOP;
  case __WASI_SIGNAL_TSTP:
    return SIGTSTP;
  case __WASI_SIGNAL_TTIN:
    return SIGTTIN;
  case __WASI_SIGNAL_TTOU:
    return SIGTTOU;
  case __WASI_SIGNAL_URG:
    return SIGURG;
  case __WASI_SIGNAL_XCPU:
    return SIGXCPU;
  case __WASI_SIGNAL_XFSZ:
    return SIGXFSZ;
  case __WASI_SIGNAL_VTALRM:
    return SIGVTALRM;
  case __WASI_SIGNAL_PROF:
    return SIGPROF;
  case __WASI_SIGNAL_WINCH:
    return SIGWINCH;
  case __WASI_SIGNAL_POLL:
    return SIGPOLL;
  case __WASI_SIGNAL_PWR:
    return SIGPWR;
  case __WASI_SIGNAL_SYS:
    return SIGSYS;
  default:
    return 0;
  }
}

template <typename Container>
inline constexpr uint32_t CalculateBufferSize(const Container &Array) {
  uint32_t Lengths[std::size(Array)];
  std::transform(
      std::begin(Array), std::end(Array), Lengths,
      [](const auto &String) -> uint32_t { return std::size(String) + 1; });
  return std::accumulate(Lengths, Lengths + std::size(Array), UINT32_C(0));
}

} // namespace

namespace WasmEdge {
namespace Host {

namespace {

template <typename T>
inline WasiExpect<void> getIOVS(Runtime::Instance::MemoryInstance *MemInst,
                                std::vector<Span<T>> &IOVS, uint32_t IOVSPtr,
                                int32_t IOVSLen) noexcept {
  IOVS.clear();
  try {
    IOVS.reserve(IOVSLen);
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
  const auto IOVSArray =
      MemInst->getPointer<__wasi_iovec_t *>(IOVSPtr, IOVSLen);
  if (unlikely(IOVSArray == nullptr)) {
    return WasiUnexpect(__WASI_ERRNO_FAULT);
  }
  for (const auto &IOV : Span<const __wasi_iovec_t>(IOVSArray, IOVSLen)) {
    const auto IOVPtr = MemInst->getPointer<uint8_t *>(IOV.buf, IOV.buf_len);
    if (unlikely(IOVPtr == nullptr)) {
      return WasiUnexpect(__WASI_ERRNO_FAULT);
    }
    IOVS.emplace_back(IOVPtr, IOV.buf_len);
  }
  return {};
}

} // namespace

Expect<uint32_t> WasiArgsGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                   uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Argv.
  const std::vector<std::string> &CmdArgs = Env.getCmdArgs();
  const uint32_t ArgvSize = CmdArgs.size() + 1;
  const uint32_t ArgvBufSize = CalculateBufferSize(CmdArgs);

  /// Check for invalid address.
  uint32_t *const Argv = MemInst->getPointer<uint32_t *>(ArgvPtr, ArgvSize);
  if (unlikely(Argv == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint8_t *const ArgvBuf =
      MemInst->getPointer<uint8_t *>(ArgvBufPtr, ArgvBufSize);
  if (unlikely(ArgvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *ArgvBufEnd = ArgvBuf;
  for (uint32_t I = 0; I < ArgvSize - 1; ++I) {
    /// Calcuate Argv[i] offset.
    Argv[I] = ArgvBufPtr + (ArgvBufEnd - ArgvBuf);
    /// Concate Argv.
    ArgvBufEnd = std::copy(CmdArgs[I].cbegin(), CmdArgs[I].cend(), ArgvBufEnd);
    *ArgvBufEnd = '\0';
    ++ArgvBufEnd;
  }
  /// Store nullptr
  Argv[ArgvSize - 1] = UINT32_C(0);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiArgsSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t ArgcPtr, uint32_t ArgvBufSizePtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  uint32_t *const __restrict__ Argc = MemInst->getPointer<uint32_t *>(ArgcPtr);
  if (unlikely(Argc == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint32_t *const __restrict__ ArgvBufSize =
      MemInst->getPointer<uint32_t *>(ArgvBufSizePtr);
  if (unlikely(ArgvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const std::vector<std::string> &CmdArgs = Env.getCmdArgs();

  /// Store Argc.
  *Argc = CmdArgs.size();

  /// Calculate and store ArgvBufSize.
  *ArgvBufSize = CalculateBufferSize(CmdArgs);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironGet::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t EnvPtr, uint32_t EnvBufPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Env.
  const std::vector<std::string> &Environs = Env.getEnvirons();
  const uint32_t EnvSize = Environs.size() + 1;
  const uint32_t EnvBufSize = CalculateBufferSize(Environs);

  /// Check for invalid address.
  uint32_t *const Env = MemInst->getPointer<uint32_t *>(EnvPtr, EnvSize);
  if (unlikely(Env == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint8_t *const EnvBuf = MemInst->getPointer<uint8_t *>(EnvBufPtr, EnvBufSize);
  if (unlikely(EnvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *EnvBufEnd = EnvBuf;
  for (uint32_t I = 0; I < EnvSize - 1; ++I) {
    /// Calculate Env[i] offset.
    Env[I] = EnvBufPtr + (EnvBufEnd - EnvBuf);
    /// Concate Env.
    EnvBufEnd = std::copy(Environs[I].cbegin(), Environs[I].cend(), EnvBufEnd);
    *EnvBufEnd = '\0';
    ++EnvBufEnd;
  }
  /// Store nullptr
  Env[EnvSize - 1] = UINT32_C(0);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          uint32_t EnvCntPtr, uint32_t EnvBufSizePtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  uint32_t *const __restrict__ EnvCnt =
      MemInst->getPointer<uint32_t *>(EnvCntPtr);
  if (unlikely(EnvCnt == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  uint32_t *const __restrict__ EnvBufSize =
      MemInst->getPointer<uint32_t *>(EnvBufSizePtr);
  if (unlikely(EnvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const std::vector<std::string> &Environs = Env.getEnvirons();

  /// Store EnvCnt.
  *EnvCnt = Environs.size();

  /// Calculate and store EnvBufSize.
  *EnvBufSize = CalculateBufferSize(Environs);

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockResGet::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t ClockId, uint32_t ResolutionPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCKID_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCKID_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  __wasi_timestamp_t *const Resolution =
      MemInst->getPointer<__wasi_timestamp_t *>(ResolutionPtr);
  if (unlikely(Resolution == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  timespec SysTimespec;
  if (unlikely(clock_getres(SysClockId, &SysTimespec) != 0)) {
    return convertErrNo(errno);
  }

  *Resolution = timespec2Timestamp(SysTimespec);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockTimeGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t ClockId, uint64_t Precision [[maybe_unused]],
                       uint32_t TimePtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  clockid_t SysClockId;
  switch (ClockId) {
  case __WASI_CLOCKID_REALTIME:
    SysClockId = CLOCK_REALTIME;
    break;
  case __WASI_CLOCKID_MONOTONIC:
    SysClockId = CLOCK_MONOTONIC;
    break;
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
    SysClockId = CLOCK_PROCESS_CPUTIME_ID;
    break;
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    SysClockId = CLOCK_THREAD_CPUTIME_ID;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  __wasi_timestamp_t *const Time =
      MemInst->getPointer<__wasi_timestamp_t *>(TimePtr);
  if (unlikely(Time == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  timespec SysTimespec;
  if (unlikely(clock_gettime(SysClockId, &SysTimespec) != 0)) {
    return convertErrNo(errno);
  }

  *Time = timespec2Timestamp(SysTimespec);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdAdvise::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint64_t Offset, uint64_t Len,
                                    uint32_t Advice) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdAdvise(static_cast<__wasi_filesize_t>(Offset),
                                static_cast<__wasi_filesize_t>(Len),
                                static_cast<__wasi_advice_t>(Advice));
}

Expect<uint32_t>
WasiFdAllocate::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     uint64_t Offset, uint64_t Len) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdAllocate(static_cast<__wasi_filesize_t>(Offset),
                                  static_cast<__wasi_filesize_t>(Len));
}

Expect<uint32_t> WasiFdClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (const auto Err = Entry->second.fdClose();
      unlikely(Err != __WASI_ERRNO_SUCCESS)) {
    return Err;
  }

  Env.eraseFile(Entry);
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdDatasync::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdDatasync();
}

Expect<uint32_t>
WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                      uint32_t FdStatPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFdstatGet(
      [&]() noexcept -> GetterR<__wasi_fdstat_t *> {
        /// Check for invalid address.
        const auto FdStat = MemInst->getPointer<__wasi_fdstat_t *>(FdStatPtr);
        if (unlikely(FdStat == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return FdStat;
      });
}

Expect<uint32_t>
WasiFdFdstatSetFlags::body(Runtime::Instance::MemoryInstance *MemInst,
                           int32_t Fd, uint32_t FsFlags) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFdstatSetFlags(static_cast<__wasi_fdflags_t>(FsFlags));
}

Expect<uint32_t>
WasiFdFdstatSetRights::body(Runtime::Instance::MemoryInstance *MemInst,
                            int32_t Fd, uint64_t FsRightsBase,
                            uint64_t FsRightsInheriting) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFdstatSetRights(
      static_cast<__wasi_rights_t>(FsRightsBase),
      static_cast<__wasi_rights_t>(FsRightsInheriting));
}

Expect<uint32_t>
WasiFdFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FilestatPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFilestatGet(
      [&]() noexcept -> GetterR<__wasi_filestat_t *> {
        /// Check for invalid address.
        const auto Filestat =
            MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
        if (unlikely(Filestat == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return Filestat;
      });
}

Expect<uint32_t>
WasiFdFilestatSetSize::body(Runtime::Instance::MemoryInstance *MemInst,
                            int32_t Fd, uint64_t FileSize) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFilestatSetSize(
      static_cast<__wasi_filesize_t>(FileSize));
}

Expect<uint32_t>
WasiFdFilestatSetTimes::body(Runtime::Instance::MemoryInstance *MemInst,
                             int32_t Fd, uint64_t ATim, uint64_t MTim,
                             uint32_t FstFlags) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdFilestatSetTimes(
      static_cast<__wasi_timestamp_t>(ATim),
      static_cast<__wasi_timestamp_t>(MTim),
      static_cast<__wasi_fstflags_t>(FstFlags));
}

Expect<uint32_t> WasiFdPread::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   int32_t IOVSLen, uint64_t Offset,
                                   uint32_t NReadPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdPread(
      [&, IOVS = std::vector<Span<uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<uint8_t>>, __wasi_size_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, IOVS, IOVSPtr, IOVSLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
        if (unlikely(NRead == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(IOVS, NRead);
      },
      static_cast<__wasi_filesize_t>(Offset));
}

Expect<uint32_t>
WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance *MemInst,
                           int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdPrestatDirName(
      [&]() noexcept -> GetterR<Span<uint8_t>> {
        /// Check for invalid address.
        const auto PathBuf =
            MemInst->getPointer<uint8_t *>(PathBufPtr, PathLen);
        if (unlikely(PathBuf == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return Span<uint8_t>(PathBuf, PathLen);
      });
}

Expect<uint32_t>
WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t PreStatPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdPrestatGet(
      [&]() noexcept -> GetterR<__wasi_prestat_t *> {
        /// Check for invalid address.
        const auto PreStat =
            MemInst->getPointer<__wasi_prestat_t *>(PreStatPtr);
        if (unlikely(PreStat == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return PreStat;
      });
}

Expect<uint32_t> WasiFdPwrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t IOVSPtr,
                                    int32_t IOVSLen, uint64_t Offset,
                                    uint32_t NWrittenPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdPwrite(
      [&, IOVS = std::vector<Span<const uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<const uint8_t>>, __wasi_size_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, IOVS, IOVSPtr, IOVSLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
        if (unlikely(NWritten == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(IOVS, NWritten);
      },
      static_cast<__wasi_filesize_t>(Offset));
}

Expect<uint32_t> WasiFdRead::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, uint32_t IOVSPtr, int32_t IOVSLen,
                                  uint32_t NReadPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdRead(
      [&, IOVS = std::vector<Span<uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<uint8_t>>, __wasi_size_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, IOVS, IOVSPtr, IOVSLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
        if (unlikely(NRead == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(IOVS, NRead);
      });
}

Expect<uint32_t> WasiFdReadDir::body(Runtime::Instance::MemoryInstance *MemInst,
                                     int32_t Fd, uint32_t BufPtr,
                                     uint32_t BufLen, uint64_t Cookie,
                                     uint32_t BufUsedSizePtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdReaddir(
      [&]() noexcept -> GetterR<Span<uint8_t>, __wasi_size_t *> {
        /// Check for invalid address.
        const auto Buf = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
        if (unlikely(Buf == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        const auto BufUsedSize =
            MemInst->getPointer<__wasi_size_t *>(BufUsedSizePtr);
        if (unlikely(BufUsedSize == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        return std::forward_as_tuple(Span<uint8_t>(Buf, BufLen), BufUsedSize);
      },
      static_cast<__wasi_dircookie_t>(Cookie));
}

Expect<uint32_t>
WasiFdRenumber::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     int32_t ToFd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  /// Refuse to renumber preopened fd.
  if (unlikely(Entry->second.isPreopened())) {
    return __WASI_ERRNO_NOTSUP;
  }

  /// Close existed fd renumbering into.
  if (const auto OldEntry = Env.getFile(ToFd);
      unlikely(OldEntry != Env.getFileEnd())) {
    if (unlikely(OldEntry->second.isPreopened())) {
      return __WASI_ERRNO_NOTSUP;
    }
    if (const auto Err = OldEntry->second.fdClose();
        unlikely(Err != __WASI_ERRNO_SUCCESS)) {
      return Err;
    }
    Env.eraseFile(OldEntry);
  }

  Env.changeFd(Entry, ToFd);

  return __WASI_ERRNO_SUCCESS;
}

Expect<int32_t> WasiFdSeek::body(Runtime::Instance::MemoryInstance *MemInst,
                                 int32_t Fd, int64_t Offset, uint32_t Whence,
                                 uint32_t NewOffsetPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdSeek(
      [&]() noexcept -> GetterR<__wasi_filedelta_t *> {
        /// Check for invalid address.
        const auto NewOffset =
            MemInst->getPointer<__wasi_filedelta_t *>(NewOffsetPtr);
        if (unlikely(NewOffset == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return NewOffset;
      },
      static_cast<__wasi_filedelta_t>(Offset),
      static_cast<__wasi_whence_t>(Whence));
}

Expect<uint32_t> WasiFdSync::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdSync();
}

Expect<uint32_t> WasiFdTell::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, int32_t OffsetPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdTell([&]() noexcept -> GetterR<__wasi_filesize_t *> {
    /// Check for invalid address.
    const auto Offset = MemInst->getPointer<__wasi_filesize_t *>(OffsetPtr);
    if (unlikely(Offset == nullptr)) {
      return WasiUnexpect(__WASI_ERRNO_FAULT);
    }
    return Offset;
  });
}

Expect<uint32_t> WasiFdWrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVSPtr,
                                   int32_t IOVSLen, uint32_t NWrittenPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.fdWrite(
      [&, IOVS = std::vector<Span<const uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<const uint8_t>>, __wasi_size_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, IOVS, IOVSPtr, IOVSLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
        if (unlikely(NWritten == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(IOVS, NWritten);
      });
}

Expect<uint32_t>
WasiPathCreateDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathCreateDirectory(
      [&]() noexcept -> GetterR<Span<const uint8_t>> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return Span<const uint8_t>(Path, PathLen);
      });
}

Expect<uint32_t>
WasiPathFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                          uint32_t PathLen, uint32_t FilestatPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathFilestatGet(
      [&]() noexcept -> GetterR<Span<const uint8_t>, __wasi_filestat_t *> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        const auto Filestat =
            MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
        if (unlikely(Filestat == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(Span<const uint8_t>(Path, PathLen),
                                     Filestat);
      },
      static_cast<__wasi_lookupflags_t>(Flags));
}

Expect<uint32_t>
WasiPathFilestatSetTimes::body(Runtime::Instance::MemoryInstance *MemInst,
                               int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                               uint32_t PathLen, uint32_t ATim, uint32_t MTim,
                               uint32_t FstFlags) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathFilestatSetTimes(
      [&]() noexcept -> GetterR<Span<const uint8_t>> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return Span<const uint8_t>(Path, PathLen);
      },
      static_cast<__wasi_lookupflags_t>(Flags),
      static_cast<__wasi_timestamp_t>(ATim),
      static_cast<__wasi_timestamp_t>(MTim),
      static_cast<__wasi_fstflags_t>(FstFlags));
}

Expect<uint32_t> WasiPathLink::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t OldFd, uint32_t OldFlags,
                                    uint32_t OldPathPtr, uint32_t OldPathLen,
                                    int32_t NewFd, uint32_t NewPathPtr,
                                    uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto OldEntry = Env.getFile(OldFd);
  if (unlikely(OldEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  const auto NewEntry = OldFd == NewFd ? OldEntry : Env.getFile(NewFd);
  if (unlikely(NewEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return WasiFile::pathLink(
      [&]() noexcept -> GetterR<Span<const uint8_t>, Span<const uint8_t>> {
        /// Check for invalid address.
        const auto OldPath =
            MemInst->getPointer<const uint8_t *>(OldPathPtr, OldPathLen);
        if (unlikely(OldPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        const auto NewPath =
            MemInst->getPointer<const uint8_t *>(NewPathPtr, NewPathLen);
        if (unlikely(NewPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(Span<const uint8_t>(OldPath, OldPathLen),
                                     Span<const uint8_t>(NewPath, NewPathLen));
      },
      OldEntry->second, static_cast<__wasi_lookupflags_t>(OldFlags),
      NewEntry->second);
}

Expect<uint32_t> WasiPathOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t DirFd, uint32_t DirFlags,
                                    uint32_t PathPtr, uint32_t PathLen,
                                    uint32_t OFlags, uint64_t FsRightsBase,
                                    uint64_t FsRightsInheriting,
                                    uint32_t FsFlags, uint32_t FdPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(DirFd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  if (auto Res = Entry->second.pathOpen(
          [&]() noexcept -> GetterR<Span<const uint8_t>> {
            /// Check for invalid address.
            const auto Path =
                MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
            if (unlikely(Path == nullptr)) {
              return WasiUnexpect(__WASI_ERRNO_FAULT);
            }
            return Span<const uint8_t>(Path, PathLen);
          },
          static_cast<__wasi_lookupflags_t>(DirFlags),
          static_cast<__wasi_oflags_t>(OFlags),
          static_cast<__wasi_rights_t>(FsRightsBase),
          static_cast<__wasi_rights_t>(FsRightsInheriting),
          static_cast<__wasi_fdflags_t>(FsFlags));
      unlikely(!Res)) {
    return Res.error();
  } else {
    const auto Fd = MemInst->getPointer<__wasi_fd_t *>(FdPtr);
    if (unlikely(Fd == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }

    *Fd = Env.getNewFd();
    Env.emplaceFile(*Fd, std::move(*Res));

    return __WASI_ERRNO_SUCCESS;
  }
}

Expect<uint32_t>
WasiPathReadLink::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                       uint32_t BufLen, uint32_t BufUsedPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathReadlink(
      [&]() noexcept
      -> GetterR<Span<const uint8_t>, Span<uint8_t>, __wasi_size_t *> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        const auto Buf = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
        if (unlikely(Buf == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        const auto BufUsed = MemInst->getPointer<__wasi_size_t *>(BufUsedPtr);
        if (unlikely(BufUsed == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        return std::forward_as_tuple(Span<const uint8_t>(Path, PathLen),
                                     Span<uint8_t>(Buf, BufLen), BufUsed);
      });
}

Expect<uint32_t>
WasiPathRemoveDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathRemoveDirectory(
      [&]() noexcept -> GetterR<Span<const uint8_t>> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        return Span<const uint8_t>(Path, PathLen);
      });
}

Expect<uint32_t>
WasiPathRename::body(Runtime::Instance::MemoryInstance *MemInst, int32_t OldFd,
                     uint32_t OldPathPtr, uint32_t OldPathLen, int32_t NewFd,
                     uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto OldEntry = Env.getFile(OldFd);
  if (unlikely(OldEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  const auto NewEntry = NewFd == OldFd ? OldEntry : Env.getFile(NewFd);
  if (unlikely(NewEntry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return WasiFile::pathRename(
      [&]() noexcept -> GetterR<Span<const uint8_t>, Span<const uint8_t>> {
        /// Check for invalid address.
        const auto OldPath =
            MemInst->getPointer<const uint8_t *>(OldPathPtr, OldPathLen);
        if (unlikely(OldPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        const auto NewPath =
            MemInst->getPointer<const uint8_t *>(NewPathPtr, NewPathLen);
        if (unlikely(NewPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        return std::forward_as_tuple(Span<const uint8_t>(OldPath, OldPathLen),
                                     Span<const uint8_t>(NewPath, NewPathLen));
      },
      OldEntry->second, NewEntry->second);
}

Expect<uint32_t>
WasiPathSymlink::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t OldPathPtr, uint32_t OldPathLen, int32_t Fd,
                      uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathSymlink(
      [&]() noexcept -> GetterR<Span<const uint8_t>, Span<const uint8_t>> {
        /// Check for invalid address.
        const auto OldPath =
            MemInst->getPointer<const uint8_t *>(OldPathPtr, OldPathLen);
        if (unlikely(OldPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        const auto NewPath =
            MemInst->getPointer<const uint8_t *>(NewPathPtr, NewPathLen);
        if (unlikely(NewPath == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(Span<const uint8_t>(OldPath, OldPathLen),
                                     Span<const uint8_t>(NewPath, NewPathLen));
      });
}

Expect<uint32_t>
WasiPathUnlinkFile::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                         uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.pathUnlinkFile(
      [&]() noexcept -> GetterR<Span<const uint8_t>> {
        /// Check for invalid address.
        const auto Path =
            MemInst->getPointer<const uint8_t *>(PathPtr, PathLen);
        if (unlikely(Path == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return Span<const uint8_t>(Path, PathLen);
      });
}

Expect<uint32_t>
WasiPollOneoff::body(Runtime::Instance::MemoryInstance *MemInst, uint32_t InPtr,
                     uint32_t OutPtr, uint32_t NSubscriptions,
                     uint32_t NEventsPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  return WasiFile::pollOneoff(
      [&]() mutable noexcept -> GetterR<Span<const __wasi_subscription_t>,
                                        Span<__wasi_event_t>, __wasi_size_t *> {
        /// Check for invalid address.
        const auto SubscriptionArray =
            MemInst->getPointer<const __wasi_subscription_t *>(InPtr,
                                                               NSubscriptions);
        if (unlikely(SubscriptionArray == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        const auto EventArray =
            MemInst->getPointer<__wasi_event_t *>(NEventsPtr, NSubscriptions);
        if (unlikely(EventArray == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        const auto NEvents = MemInst->getPointer<__wasi_size_t *>(NEventsPtr);
        if (unlikely(NEvents == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }

        return std::forward_as_tuple(
            Span<const __wasi_subscription_t>(SubscriptionArray,
                                              NSubscriptions),
            Span<__wasi_event_t>(EventArray, NSubscriptions), NEvents);
      },
      [&, Subs = std::vector<WasiFile::Subscription>()](
          Span<const __wasi_subscription_t> SubscriptionSpan) mutable noexcept
      -> GetterR<Span<const WasiFile::Subscription>> {
        Subs.clear();
        try {
          Subs.reserve(NSubscriptions);
        } catch (std::bad_alloc &) {
          return WasiUnexpect(__WASI_ERRNO_NOMEM);
        }
        /// Translate subscription descriptor
        for (const auto &Sub : SubscriptionSpan) {
          WasiFile::Subscription SysSub = {.Userdata = Sub.userdata,
                                           .Tag = Sub.u.tag};
          switch (Sub.u.tag) {
          case __WASI_EVENTTYPE_CLOCK:
            SysSub.Clock = Sub.u.u.clock;
            break;
          case __WASI_EVENTTYPE_FD_READ:
          case __WASI_EVENTTYPE_FD_WRITE: {
            const bool IsRead = Sub.u.tag == __WASI_EVENTTYPE_FD_READ;
            const int Fd = IsRead ? Sub.u.u.fd_read.file_descriptor
                                  : Sub.u.u.fd_write.file_descriptor;
            const auto Entry = Env.getFile(Fd);
            if (unlikely(Entry == Env.getFileEnd())) {
              return WasiUnexpect(__WASI_ERRNO_BADF);
            }
            (IsRead ? SysSub.FdRead : SysSub.FdWrite) = &Entry->second;
            break;
          }
          default:
            __builtin_unreachable();
          }
          Subs.push_back(std::move(SysSub));
        }
        return Span<const WasiFile::Subscription>(Subs);
      });
}

Expect<void> WasiProcExit::body(Runtime::Instance::MemoryInstance *MemInst,
                                int32_t ExitCode) {
  Env.setExitCode(ExitCode);
  return Unexpect(ErrCode::Terminated);
}

Expect<uint32_t> WasiProcRaise::body(Runtime::Instance::MemoryInstance *MemInst,
                                     int32_t Signal) {
  const int SysSignal = signal2SysSignal(Signal);

  if (unlikely(raise(SysSignal) != 0)) {
    return convertErrNo(errno);
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiRandomGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                     uint32_t BufPtr, uint32_t BufLen) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  uint8_t *const Buf = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  Span<uint8_t> BufSpan(Buf, BufLen);

  /// Use uniform distribution to generate random bytes array
  std::random_device RandomDevice;
  std::mt19937 Generator(RandomDevice());
  std::uniform_int_distribution<> Distribution(
      std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());
  std::generate(BufSpan.begin(), BufSpan.end(), [&Generator, &Distribution] {
    return Distribution(Generator);
  });

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSchedYield::body(Runtime::Instance::MemoryInstance *MemInst) {
  if (unlikely(sched_yield() != 0)) {
    return convertErrNo(errno);
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecv::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t RiDataPtr,
                                    int32_t RiDataLen, uint32_t RiFlags,
                                    uint32_t RoDataLenPtr,
                                    uint32_t RoFlagsPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.sockRecv(
      [&, RiData = std::vector<Span<uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<uint8_t>>, __wasi_size_t *, __wasi_roflags_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, RiData, RiDataPtr, RiDataLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto RoDataLen =
            MemInst->getPointer<__wasi_size_t *>(RoDataLenPtr);
        if (unlikely(RoDataLen == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        const auto RoFlags =
            MemInst->getPointer<__wasi_roflags_t *>(RoFlagsPtr);
        if (unlikely(RoFlags == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(RiData, RoDataLen, RoFlags);
      },
      static_cast<__wasi_riflags_t>(RiFlags));
}

Expect<uint32_t> WasiSockSend::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t SiDataPtr,
                                    int32_t SiDataLen, uint32_t SiFlags,
                                    uint32_t SoDataLenPtr) {
  /// Check memory instance from module.
  if (unlikely(MemInst == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.sockSend(
      [&, SiData = std::vector<Span<const uint8_t>>()]() mutable noexcept
      -> GetterR<Span<Span<const uint8_t>>, __wasi_size_t *> {
        /// Check for invalid address.
        if (auto Res = getIOVS(MemInst, SiData, SiDataPtr, SiDataLen);
            unlikely(!Res)) {
          return WasiUnexpect(Res);
        }
        const auto SoDataLen =
            MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
        if (unlikely(SoDataLen == nullptr)) {
          return WasiUnexpect(__WASI_ERRNO_FAULT);
        }
        return std::forward_as_tuple(SiData, SoDataLen);
      },
      static_cast<__wasi_siflags_t>(SiFlags));
}

Expect<uint32_t>
WasiSockShutdown::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t SdFlags) {
  const auto Entry = Env.getFile(Fd);
  if (unlikely(Entry == Env.getFileEnd())) {
    return __WASI_ERRNO_BADF;
  }

  return Entry->second.sockShutdown(static_cast<__wasi_sdflags_t>(SdFlags));
}

} // namespace Host
} // namespace WasmEdge
