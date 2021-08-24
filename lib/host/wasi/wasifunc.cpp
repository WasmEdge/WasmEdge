#include "host/wasi/wasifunc.h"
#include "common/filesystem.h"
#include "common/log.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"
#include <chrono>
#include <csignal>
#include <limits>
#include <numeric>
#include <random>
#include <string_view>

namespace WasmEdge {
namespace Host {

namespace {

template <typename Container>
inline __wasi_size_t calculateBufferSize(const Container &Array) noexcept {
  std::vector<__wasi_size_t> Lengths(Array.size());
  std::transform(Array.begin(), Array.end(), Lengths.begin(),
                 [](const auto &String) -> __wasi_size_t {
                   return static_cast<__wasi_size_t>(String.size()) +
                          UINT32_C(1);
                 });
  return std::accumulate(Lengths.begin(), Lengths.end(), UINT32_C(0));
}

template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> { using Type = uint8_t; };
template <> struct WasiRawType<uint16_t> { using Type = uint16_t; };
template <> struct WasiRawType<uint32_t> { using Type = uint32_t; };
template <> struct WasiRawType<uint64_t> { using Type = uint64_t; };

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <typename T> WASI::WasiExpect<T> cast(uint64_t) noexcept;

template <>
WASI::WasiExpect<__wasi_clockid_t>
cast<__wasi_clockid_t>(uint64_t ClockId) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_clockid_t>>(ClockId)) {
  case __WASI_CLOCKID_REALTIME:
  case __WASI_CLOCKID_MONOTONIC:
  case __WASI_CLOCKID_PROCESS_CPUTIME_ID:
  case __WASI_CLOCKID_THREAD_CPUTIME_ID:
    return static_cast<__wasi_clockid_t>(ClockId);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_advice_t>
cast<__wasi_advice_t>(uint64_t Advice) noexcept {
  switch (WasiRawTypeT<__wasi_advice_t>(Advice)) {
  case __WASI_ADVICE_NORMAL:
  case __WASI_ADVICE_SEQUENTIAL:
  case __WASI_ADVICE_RANDOM:
  case __WASI_ADVICE_WILLNEED:
  case __WASI_ADVICE_DONTNEED:
  case __WASI_ADVICE_NOREUSE:
    return static_cast<__wasi_advice_t>(Advice);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_whence_t>
cast<__wasi_whence_t>(uint64_t Whence) noexcept {
  switch (WasiRawTypeT<__wasi_whence_t>(Whence)) {
  case __WASI_WHENCE_SET:
  case __WASI_WHENCE_CUR:
  case __WASI_WHENCE_END:
    return static_cast<__wasi_whence_t>(Whence);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_eventtype_t>
cast<__wasi_eventtype_t>(uint64_t Eventtype) noexcept {
  switch (WasiRawTypeT<__wasi_eventtype_t>(Eventtype)) {
  case __WASI_EVENTTYPE_CLOCK:
  case __WASI_EVENTTYPE_FD_READ:
  case __WASI_EVENTTYPE_FD_WRITE:
    return static_cast<__wasi_eventtype_t>(Eventtype);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_signal_t>
cast<__wasi_signal_t>(uint64_t Signal) noexcept {
  switch (WasiRawTypeT<__wasi_signal_t>(Signal)) {
  case __WASI_SIGNAL_NONE:
  case __WASI_SIGNAL_HUP:
  case __WASI_SIGNAL_INT:
  case __WASI_SIGNAL_QUIT:
  case __WASI_SIGNAL_ILL:
  case __WASI_SIGNAL_TRAP:
  case __WASI_SIGNAL_ABRT:
  case __WASI_SIGNAL_BUS:
  case __WASI_SIGNAL_FPE:
  case __WASI_SIGNAL_KILL:
  case __WASI_SIGNAL_USR1:
  case __WASI_SIGNAL_SEGV:
  case __WASI_SIGNAL_USR2:
  case __WASI_SIGNAL_PIPE:
  case __WASI_SIGNAL_ALRM:
  case __WASI_SIGNAL_TERM:
  case __WASI_SIGNAL_CHLD:
  case __WASI_SIGNAL_CONT:
  case __WASI_SIGNAL_STOP:
  case __WASI_SIGNAL_TSTP:
  case __WASI_SIGNAL_TTIN:
  case __WASI_SIGNAL_TTOU:
  case __WASI_SIGNAL_URG:
  case __WASI_SIGNAL_XCPU:
  case __WASI_SIGNAL_XFSZ:
  case __WASI_SIGNAL_VTALRM:
  case __WASI_SIGNAL_PROF:
  case __WASI_SIGNAL_WINCH:
  case __WASI_SIGNAL_POLL:
  case __WASI_SIGNAL_PWR:
  case __WASI_SIGNAL_SYS:
    return static_cast<__wasi_signal_t>(Signal);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_rights_t>
cast<__wasi_rights_t>(uint64_t Rights) noexcept {
  const auto Mask =
      __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ |
      __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
      __WASI_RIGHTS_FD_SYNC | __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_FD_WRITE |
      __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_ALLOCATE |
      __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
      __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_LINK_TARGET |
      __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READDIR |
      __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_RENAME_SOURCE |
      __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_FILESTAT_GET |
      __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
      __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_FD_FILESTAT_GET |
      __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
      __WASI_RIGHTS_PATH_SYMLINK | __WASI_RIGHTS_PATH_REMOVE_DIRECTORY |
      __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE |
      __WASI_RIGHTS_SOCK_SHUTDOWN | __WASI_RIGHTS_SOCK_OPEN |
      __WASI_RIGHTS_SOCK_CLOSE | __WASI_RIGHTS_SOCK_BIND |
      __WASI_RIGHTS_SOCK_RECV | __WASI_RIGHTS_SOCK_RECV_FROM |
      __WASI_RIGHTS_SOCK_SEND | __WASI_RIGHTS_SOCK_SEND_TO;
  if ((WasiRawTypeT<__wasi_rights_t>(Rights) & ~Mask) == 0) {
    return static_cast<__wasi_rights_t>(Rights);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_fdflags_t>
cast<__wasi_fdflags_t>(uint64_t FdFlags) noexcept {
  const auto Mask = __WASI_FDFLAGS_APPEND | __WASI_FDFLAGS_DSYNC |
                    __WASI_FDFLAGS_NONBLOCK | __WASI_FDFLAGS_RSYNC |
                    __WASI_FDFLAGS_SYNC;
  if ((WasiRawTypeT<__wasi_fdflags_t>(FdFlags) & ~Mask) == 0) {
    return static_cast<__wasi_fdflags_t>(FdFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_fstflags_t>
cast<__wasi_fstflags_t>(uint64_t FdFlags) noexcept {
  const auto Mask = __WASI_FSTFLAGS_ATIM | __WASI_FSTFLAGS_ATIM_NOW |
                    __WASI_FSTFLAGS_MTIM | __WASI_FSTFLAGS_MTIM_NOW;
  if ((WasiRawTypeT<__wasi_fstflags_t>(FdFlags) & ~Mask) == 0) {
    return static_cast<__wasi_fstflags_t>(FdFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_lookupflags_t>
cast<__wasi_lookupflags_t>(uint64_t LookupFlags) noexcept {
  const auto Mask = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW;
  if ((WasiRawTypeT<__wasi_lookupflags_t>(LookupFlags) & ~Mask) == 0) {
    return static_cast<__wasi_lookupflags_t>(LookupFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_oflags_t>
cast<__wasi_oflags_t>(uint64_t OFlags) noexcept {
  const auto Mask = __WASI_OFLAGS_CREAT | __WASI_OFLAGS_DIRECTORY |
                    __WASI_OFLAGS_EXCL | __WASI_OFLAGS_TRUNC;
  if ((WasiRawTypeT<__wasi_oflags_t>(OFlags) & ~Mask) == 0) {
    return static_cast<__wasi_oflags_t>(OFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_subclockflags_t>
cast<__wasi_subclockflags_t>(uint64_t SubClockFlags) noexcept {
  const auto Mask = __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME;
  if ((WasiRawTypeT<__wasi_subclockflags_t>(SubClockFlags) & ~Mask) == 0) {
    return static_cast<__wasi_subclockflags_t>(SubClockFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_riflags_t>
cast<__wasi_riflags_t>(uint64_t RiFlags) noexcept {
  const auto Mask = __WASI_RIFLAGS_RECV_PEEK | __WASI_RIFLAGS_RECV_WAITALL;
  if ((WasiRawTypeT<__wasi_riflags_t>(RiFlags) & ~Mask) == 0) {
    return static_cast<__wasi_riflags_t>(RiFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_siflags_t>
cast<__wasi_siflags_t>(uint64_t SiFlags) noexcept {
  const auto Mask = 0;
  if ((WasiRawTypeT<__wasi_siflags_t>(SiFlags) & ~Mask) == 0) {
    return static_cast<__wasi_siflags_t>(SiFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_sdflags_t>
cast<__wasi_sdflags_t>(uint64_t SdFlags) noexcept {
  const auto Mask = __WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR;
  if ((WasiRawTypeT<__wasi_sdflags_t>(SdFlags) & ~Mask) == 0) {
    return static_cast<__wasi_sdflags_t>(SdFlags);
  }
  return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
}

template <>
WASI::WasiExpect<__wasi_address_family_t>
cast<__wasi_address_family_t>(uint64_t Family) noexcept {
  switch (WasiRawTypeT<__wasi_address_family_t>(Family)) {
  case __WASI_ADDRESS_FAMILY_INET4:
  case __WASI_ADDRESS_FAMILY_INET6:
    return static_cast<__wasi_address_family_t>(Family);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_sock_type_t>
cast<__wasi_sock_type_t>(uint64_t SockType) noexcept {
  switch (WasiRawTypeT<__wasi_sock_type_t>(SockType)) {
  case __WASI_SOCK_TYPE_SOCK_DGRAM:
  case __WASI_SOCK_TYPE_SOCK_STREAM:
    return static_cast<__wasi_sock_type_t>(SockType);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

} // namespace

Expect<uint32_t> WasiArgsGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                   uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Argv.
  const auto &Arguments = Env.getArguments();
  const uint32_t ArgvSize =
      static_cast<uint32_t>(Arguments.size()) + UINT32_C(1);
  const uint32_t ArgvBufSize = calculateBufferSize(Arguments);

  /// Check for invalid address.
  auto *const Argv = MemInst->getPointer<uint8_t_ptr *>(ArgvPtr, ArgvSize);
  if (unlikely(Argv == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  auto *const ArgvBuf = MemInst->getPointer<uint8_t *>(ArgvBufPtr, ArgvBufSize);
  if (unlikely(ArgvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  *Argv = ArgvBufPtr;

  if (auto Res = Env.argsGet({Argv, ArgvSize}, {ArgvBuf, ArgvBufSize});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiArgsSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t /* Out */ ArgcPtr,
                       uint32_t /* Out */ ArgvBufSizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  auto *const __restrict__ Argc = MemInst->getPointer<__wasi_size_t *>(ArgcPtr);
  if (unlikely(Argc == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  auto *const __restrict__ ArgvBufSize =
      MemInst->getPointer<__wasi_size_t *>(ArgvBufSizePtr);
  if (unlikely(ArgvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  if (auto Res = Env.argsSizesGet(*Argc, *ArgvBufSize); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironGet::body(Runtime::Instance::MemoryInstance *MemInst,
                     uint32_t EnvPtr, uint32_t EnvBufPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Store **Env.
  const auto &EnvironVariables = Env.getEnvironVariables();
  const uint32_t EnvSize =
      static_cast<uint32_t>(EnvironVariables.size()) + UINT32_C(1);
  const uint32_t EnvBufSize = calculateBufferSize(EnvironVariables);

  /// Check for invalid address.
  auto *const Env = MemInst->getPointer<uint8_t_ptr *>(EnvPtr, EnvSize);
  if (unlikely(Env == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  auto *const EnvBuf = MemInst->getPointer<uint8_t *>(EnvBufPtr, EnvBufSize);
  if (unlikely(EnvBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  *Env = EnvBufPtr;

  if (auto Res = this->Env.environGet({Env, EnvSize}, {EnvBuf, EnvBufSize});
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiEnvironSizesGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          uint32_t /* Out */ EnvCntPtr,
                          uint32_t /* Out */ EnvBufSizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  auto *const __restrict__ Envc =
      MemInst->getPointer<__wasi_size_t *>(EnvCntPtr);
  if (unlikely(Envc == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  auto *const __restrict__ EnvBufSize =
      MemInst->getPointer<__wasi_size_t *>(EnvBufSizePtr);
  if (unlikely(EnvBufSize == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  if (auto Res = Env.environSizesGet(*Envc, *EnvBufSize); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockResGet::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t ClockId, uint32_t /* Out */ ResolutionPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const Resolution =
      MemInst->getPointer<__wasi_timestamp_t *>(ResolutionPtr);
  if (unlikely(Resolution == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_clockid_t WasiClockId;
  if (auto Res = cast<__wasi_clockid_t>(ClockId); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiClockId = *Res;
  }

  if (auto Res = Env.clockResGet(WasiClockId, *Resolution); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiClockTimeGet::body(Runtime::Instance::MemoryInstance *MemInst,
                       uint32_t ClockId, uint64_t Precision,
                       uint32_t /* Out */ TimePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const Time = MemInst->getPointer<__wasi_timestamp_t *>(TimePtr);
  if (unlikely(Time == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_clockid_t WasiClockId;
  if (auto Res = cast<__wasi_clockid_t>(ClockId); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiClockId = *Res;
  }

  const __wasi_timestamp_t WasiPrecision = Precision;

  if (auto Res = Env.clockTimeGet(WasiClockId, WasiPrecision, *Time);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdAdvise::body(Runtime::Instance::MemoryInstance *,
                                    int32_t Fd, uint64_t Offset, uint64_t Len,
                                    uint32_t Advice) {
  __wasi_advice_t WasiAdvice;
  if (auto Res = cast<__wasi_advice_t>(Advice); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAdvice = *Res;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;
  const __wasi_filesize_t WasiLen = Len;

  if (auto Res = Env.fdAdvise(WasiFd, WasiOffset, WasiLen, WasiAdvice);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdAllocate::body(Runtime::Instance::MemoryInstance *,
                                      int32_t Fd, uint64_t Offset,
                                      uint64_t Len) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;
  const __wasi_filesize_t WasiLen = Len;

  if (auto Res = Env.fdAllocate(WasiFd, WasiOffset, WasiLen); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdClose::body(Runtime::Instance::MemoryInstance *,
                                   int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdClose(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdDatasync::body(Runtime::Instance::MemoryInstance *,
                                      int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdDatasync(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFdstatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                      uint32_t /* Out */ FdStatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const FdStat = MemInst->getPointer<__wasi_fdstat_t *>(FdStatPtr);
  if (unlikely(FdStat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdFdstatGet(WasiFd, *FdStat); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdFdstatSetFlags::body(Runtime::Instance::MemoryInstance *,
                                            int32_t Fd, uint32_t FsFlags) {
  __wasi_fdflags_t WasiFdFlags;
  if (auto Res = cast<__wasi_fdflags_t>(FsFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFdFlags = *Res;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdFdstatSetFlags(WasiFd, WasiFdFlags); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFdstatSetRights::body(Runtime::Instance::MemoryInstance *, int32_t Fd,
                            uint64_t FsRightsBase,
                            uint64_t FsRightsInheriting) {
  __wasi_rights_t WasiFsRightsBase;
  if (auto Res = cast<__wasi_rights_t>(FsRightsBase); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFsRightsBase = *Res;
  }

  __wasi_rights_t WasiFsRightsInheriting;
  if (auto Res = cast<__wasi_rights_t>(FsRightsInheriting); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFsRightsInheriting = *Res;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdFdstatSetRights(WasiFd, WasiFsRightsBase,
                                       WasiFsRightsInheriting);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t /* Out */ FilestatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const Filestat = MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
  if (unlikely(Filestat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdFilestatGet(WasiFd, *Filestat); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatSetSize::body(Runtime::Instance::MemoryInstance *, int32_t Fd,
                            uint64_t Size) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiSize = Size;

  if (auto Res = Env.fdFilestatSetSize(WasiFd, WasiSize); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdFilestatSetTimes::body(Runtime::Instance::MemoryInstance *, int32_t Fd,
                             uint64_t ATim, uint64_t MTim, uint32_t FstFlags) {
  __wasi_fstflags_t WasiFstFlags;
  if (auto Res = cast<__wasi_fstflags_t>(FstFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFstFlags = *Res;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_timestamp_t WasiATim = ATim;
  const __wasi_timestamp_t WasiMTim = MTim;

  if (auto Res =
          Env.fdFilestatSetTimes(WasiFd, WasiATim, WasiMTim, WasiFstFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPread::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVsPtr,
                                   uint32_t IOVsLen, uint64_t Offset,
                                   uint32_t /* Out */ NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const IOVsArray =
      MemInst->getPointer<__wasi_iovec_t *>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  std::array<Span<uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (__wasi_size_t I = 0; I < WasiIOVsLen; ++I) {
    __wasi_iovec_t &IOV = IOVsArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const ReadArr = MemInst->getPointer<uint8_t *>(IOV.buf, BufLen);
    if (unlikely(ReadArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs[I] = {ReadArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;

  if (auto Res = Env.fdPread(WasiFd, {WasiIOVs.data(), WasiIOVsLen}, WasiOffset,
                             *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdPrestatDirName::body(Runtime::Instance::MemoryInstance *MemInst,
                           int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const PathBuf = MemInst->getPointer<uint8_t *>(PathBufPtr, PathLen);
  if (unlikely(PathBuf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdPrestatDirName(WasiFd, {PathBuf, PathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiFdPrestatGet::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t /* Out */ PreStatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_prestat_t *const PreStat =
      MemInst->getPointer<__wasi_prestat_t *>(PreStatPtr);
  if (unlikely(PreStat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdPrestatGet(WasiFd, *PreStat); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPwrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t IOVsPtr,
                                    uint32_t IOVsLen, uint64_t Offset,
                                    uint32_t /* Out */ NWrittenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const IOVsArray =
      MemInst->getPointer<__wasi_ciovec_t *>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  auto *const NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  std::array<Span<const uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (__wasi_size_t I = 0; I < WasiIOVsLen; ++I) {
    __wasi_ciovec_t &IOV = IOVsArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const WriteArr =
        MemInst->getPointer<const uint8_t *>(IOV.buf, BufLen);
    if (unlikely(WriteArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs[I] = {WriteArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;

  if (auto Res = Env.fdPwrite(WasiFd, {WasiIOVs.data(), WasiIOVsLen},
                              WasiOffset, *NWritten);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdRead::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, uint32_t IOVsPtr,
                                  uint32_t IOVsLen,
                                  uint32_t /* Out */ NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const IOVsArray =
      MemInst->getPointer<__wasi_iovec_t *>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  std::array<Span<uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (__wasi_size_t I = 0; I < WasiIOVsLen; ++I) {
    __wasi_iovec_t &IOV = IOVsArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const ReadArr = MemInst->getPointer<uint8_t *>(IOV.buf, BufLen);
    if (unlikely(ReadArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs[I] = {ReadArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdRead(WasiFd, {WasiIOVs.data(), WasiIOVsLen}, *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdReadDir::body(Runtime::Instance::MemoryInstance *MemInst,
                                     int32_t Fd, uint32_t BufPtr,
                                     uint32_t BufLen, uint64_t Cookie,
                                     uint32_t /* Out */ NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  /// Check for invalid address.
  auto *Buf = MemInst->getPointer<uint8_t *>(BufPtr, WasiBufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_dircookie_t WasiCookie = Cookie;

  if (auto Res = Env.fdReaddir(WasiFd, {Buf, WasiBufLen}, WasiCookie, *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdRenumber::body(Runtime::Instance::MemoryInstance *,
                                      int32_t Fd, int32_t ToFd) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_fd_t WasiToFd = ToFd;

  if (auto Res = Env.fdRenumber(WasiFd, WasiToFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<int32_t> WasiFdSeek::body(Runtime::Instance::MemoryInstance *MemInst,
                                 int32_t Fd, int64_t Offset, uint32_t Whence,
                                 uint32_t /* Out */ NewOffsetPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_whence_t WasiWhence;
  if (auto Res = cast<__wasi_whence_t>(Whence); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiWhence = *Res;
  }

  /// Check for invalid address.
  auto *NewOffset = MemInst->getPointer<__wasi_filesize_t *>(NewOffsetPtr);
  if (unlikely(NewOffset == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filedelta_t WasiOffset = Offset;

  if (auto Res = Env.fdSeek(WasiFd, WasiOffset, WasiWhence, *NewOffset);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdSync::body(Runtime::Instance::MemoryInstance *,
                                  int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdSync(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdTell::body(Runtime::Instance::MemoryInstance *MemInst,
                                  int32_t Fd, uint32_t /* Out */ OffsetPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  __wasi_filesize_t *Offset =
      MemInst->getPointer<__wasi_filesize_t *>(OffsetPtr);
  if (unlikely(Offset == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdTell(WasiFd, *Offset); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdWrite::body(Runtime::Instance::MemoryInstance *MemInst,
                                   int32_t Fd, uint32_t IOVsPtr,
                                   uint32_t IOVsLen,
                                   uint32_t /* Out */ NWrittenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const IOVsArray =
      MemInst->getPointer<__wasi_ciovec_t *>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  /// Check for invalid address.
  auto *const NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  std::array<Span<const uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (__wasi_size_t I = 0; I < WasiIOVsLen; ++I) {
    __wasi_ciovec_t &IOV = IOVsArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const WriteArr =
        MemInst->getPointer<const uint8_t *>(IOV.buf, BufLen);
    if (unlikely(WriteArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs[I] = {WriteArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdWrite(WasiFd, {WasiIOVs.data(), WasiIOVsLen}, *NWritten);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathCreateDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathCreateDirectory(WasiFd, {Path, WasiPathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathFilestatGet::body(Runtime::Instance::MemoryInstance *MemInst,
                          int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                          uint32_t PathLen, uint32_t /* Out */ FilestatPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_lookupflags_t WasiFlags;
  if (auto Res = cast<__wasi_lookupflags_t>(Flags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFlags = *Res;
  }

  auto *const Filestat = MemInst->getPointer<__wasi_filestat_t *>(FilestatPtr);
  if (unlikely(Filestat == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathFilestatGet(WasiFd, {Path, WasiPathLen}, WasiFlags,
                                     *Filestat);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathFilestatSetTimes::body(Runtime::Instance::MemoryInstance *MemInst,
                               int32_t Fd, uint32_t Flags, uint32_t PathPtr,
                               uint32_t PathLen, uint64_t ATim, uint64_t MTim,
                               uint32_t FstFlags) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_lookupflags_t WasiFlags;
  if (auto Res = cast<__wasi_lookupflags_t>(Flags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFlags = *Res;
  }

  __wasi_fstflags_t WasiFstFlags;
  if (auto Res = cast<__wasi_fstflags_t>(FstFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFstFlags = *Res;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_timestamp_t WasiATim = ATim;
  const __wasi_timestamp_t WasiMTim = MTim;

  if (auto Res =
          Env.pathFilestatSetTimes(WasiFd, {Path, WasiPathLen}, WasiFlags,
                                   WasiATim, WasiMTim, WasiFstFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathLink::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t OldFd, uint32_t OldFlags,
                                    uint32_t OldPathPtr, uint32_t OldPathLen,
                                    int32_t NewFd, uint32_t NewPathPtr,
                                    uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_lookupflags_t WasiOldFlags;
  if (auto Res = cast<__wasi_lookupflags_t>(OldFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiOldFlags = *Res;
  }

  const __wasi_size_t WasiOldPathLen = OldPathLen;

  auto *const OldPath =
      MemInst->getPointer<const char *>(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;

  auto *const NewPath =
      MemInst->getPointer<const char *>(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiOldFd = OldFd;
  const __wasi_fd_t WasinewFd = NewFd;

  if (auto Res = Env.pathLink(WasiOldFd, {OldPath, WasiOldPathLen}, WasinewFd,
                              {NewPath, WasiNewPathLen}, WasiOldFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t DirFd, uint32_t DirFlags,
                                    uint32_t PathPtr, uint32_t PathLen,
                                    uint32_t OFlags, uint64_t FsRightsBase,
                                    uint64_t FsRightsInheriting,
                                    uint32_t FsFlags,
                                    uint32_t /* Out */ FdPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_lookupflags_t WasiDirFlags;
  if (auto Res = cast<__wasi_lookupflags_t>(DirFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiDirFlags = *Res;
  }

  __wasi_oflags_t WasiOFlags;
  if (auto Res = cast<__wasi_oflags_t>(OFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiOFlags = *Res;
  }

  __wasi_rights_t WasiFsRightsBase;
  if (auto Res = cast<__wasi_rights_t>(FsRightsBase); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFsRightsBase = *Res;
  }

  __wasi_rights_t WasiFsRightsInheriting;
  if (auto Res = cast<__wasi_rights_t>(FsRightsInheriting); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFsRightsInheriting = *Res;
  }

  __wasi_fdflags_t WasiFsFlags;
  if (auto Res = cast<__wasi_fdflags_t>(FsFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFsFlags = *Res;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const Fd = MemInst->getPointer<__wasi_fd_t *>(FdPtr);
  if (unlikely(Fd == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiDirFd = DirFd;

  if (auto Res =
          Env.pathOpen(WasiDirFd, {Path, WasiPathLen}, WasiDirFlags, WasiOFlags,
                       WasiFsRightsBase, WasiFsRightsInheriting, WasiFsFlags);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Fd = *Res;
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathReadLink::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                       uint32_t BufLen, uint32_t /* Out */ NReadPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  auto *const Buf = MemInst->getPointer<char *>(BufPtr, WasiBufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathReadlink(WasiFd, {Path, WasiPathLen},
                                  {Buf, WasiBufLen}, *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathRemoveDirectory::body(Runtime::Instance::MemoryInstance *MemInst,
                              int32_t Fd, uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathRemoveDirectory(WasiFd, {Path, WasiPathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathRename::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     uint32_t OldPathPtr, uint32_t OldPathLen, int32_t NewFd,
                     uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiOldPathLen = OldPathLen;

  auto *const OldPath =
      MemInst->getPointer<const char *>(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;

  auto *const NewPath =
      MemInst->getPointer<const char *>(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_fd_t WasiNewFd = NewFd;

  if (auto Res = Env.pathRename(WasiFd, {OldPath, WasiOldPathLen}, WasiNewFd,
                                {NewPath, WasiNewPathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathSymlink::body(Runtime::Instance::MemoryInstance *MemInst,
                      uint32_t OldPathPtr, uint32_t OldPathLen, int32_t Fd,
                      uint32_t NewPathPtr, uint32_t NewPathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiOldPathLen = OldPathLen;

  auto *const OldPath =
      MemInst->getPointer<const char *>(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;

  auto *const NewPath =
      MemInst->getPointer<const char *>(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathSymlink({OldPath, WasiOldPathLen}, WasiFd,
                                 {NewPath, WasiNewPathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathUnlinkFile::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                         uint32_t PathPtr, uint32_t PathLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;

  auto *const Path = MemInst->getPointer<const char *>(PathPtr, WasiPathLen);
  if (unlikely(Path == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathUnlinkFile(WasiFd, {Path, WasiPathLen});
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPollOneoff::body(Runtime::Instance::MemoryInstance *MemInst, uint32_t InPtr,
                     uint32_t OutPtr, uint32_t NSubscriptions,
                     uint32_t /* Out */ NEventsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNSub = NSubscriptions;

  auto *const SubscriptionArray =
      MemInst->getPointer<const __wasi_subscription_t *>(InPtr, WasiNSub);
  if (unlikely(SubscriptionArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const EventArray =
      MemInst->getPointer<__wasi_event_t *>(OutPtr, WasiNSub);
  if (unlikely(EventArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NEvents = MemInst->getPointer<__wasi_size_t *>(NEventsPtr);
  if (unlikely(NEvents == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t EventCount = 0;

  if (auto Poll = Env.pollOneoff(WasiNSub); unlikely(!Poll)) {
    return Poll.error();
  } else {
    /// Validate contents
    const Span<const __wasi_subscription_t> Subs(SubscriptionArray, WasiNSub);
    Span<__wasi_event_t> Events(EventArray, WasiNSub);
    auto Record = [&Events, &EventCount](
                      __wasi_userdata_t UserData, __wasi_errno_t Errno,
                      __wasi_eventtype_t EventType, __wasi_filesize_t NBytes,
                      __wasi_eventrwflags_t Flags) {
      auto &Event = Events[EventCount];
      Event.userdata = UserData;
      Event.error = Errno;
      Event.type = EventType;
      if (Errno == __WASI_ERRNO_SUCCESS &&
          (EventType &
           (__WASI_EVENTTYPE_FD_READ | __WASI_EVENTTYPE_FD_WRITE))) {
        Event.fd_readwrite.nbytes = NBytes;
        Event.fd_readwrite.flags = Flags;
      }
      ++EventCount;
    };
    for (auto &Sub : Subs) {
      const __wasi_userdata_t WasiUserData = Sub.userdata;
      const __wasi_eventrwflags_t NoFlags =
          static_cast<__wasi_eventrwflags_t>(0);

      __wasi_eventtype_t Type;
      if (auto Res = cast<__wasi_eventtype_t>(Sub.u.tag); unlikely(!Res)) {
        Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
        continue;
      } else {
        Type = *Res;
      }

      switch (Type) {
      case __WASI_EVENTTYPE_CLOCK: {
        __wasi_clockid_t WasiClockId;
        if (auto Res = cast<__wasi_clockid_t>(Sub.u.u.clock.id);
            unlikely(!Res)) {
          Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
          continue;
        } else {
          WasiClockId = *Res;
        }

        __wasi_subclockflags_t WasiFlags;
        if (auto Res = cast<__wasi_subclockflags_t>(Sub.u.u.clock.flags);
            unlikely(!Res)) {
          Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
          continue;
        } else {
          WasiFlags = *Res;
        }

        const __wasi_timestamp_t WasiTimeout = Sub.u.u.clock.timeout;
        const __wasi_timestamp_t WasiPrecision = Sub.u.u.clock.precision;

        if (auto Res = Poll->clock(WasiClockId, WasiTimeout, WasiPrecision,
                                   WasiFlags, WasiUserData);
            unlikely(!Res)) {
          Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
        }
        continue;
      }
      case __WASI_EVENTTYPE_FD_READ: {
        const __wasi_fd_t WasiFd = Sub.u.u.fd_read.file_descriptor;
        if (auto Res = Poll->read(WasiFd, WasiUserData); unlikely(!Res)) {
          Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
        }
        continue;
      }
      case __WASI_EVENTTYPE_FD_WRITE: {
        const __wasi_fd_t WasiFd = Sub.u.u.fd_write.file_descriptor;
        if (auto Res = Poll->write(WasiFd, WasiUserData); unlikely(!Res)) {
          Record(WasiUserData, Res.error(), Sub.u.tag, 0, NoFlags);
        }
        continue;
      }
      default:
        __builtin_unreachable();
      }
    }

    if (auto Res = Poll->wait(Record); unlikely(!Res)) {
      return Res.error();
    }
  }

  *NEvents = EventCount;
  return __WASI_ERRNO_SUCCESS;
}

Expect<void> WasiProcExit::body(Runtime::Instance::MemoryInstance *,
                                uint32_t ExitCode) {
  Env.procExit(ExitCode);
  return Unexpect(ErrCode::Terminated);
}

Expect<uint32_t> WasiProcRaise::body(Runtime::Instance::MemoryInstance *,
                                     uint32_t Signal) {
  __wasi_signal_t WasiSignal;
  if (auto Res = cast<__wasi_signal_t>(Signal); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSignal = *Res;
  }

  if (auto Res = Env.procRaise(WasiSignal); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSchedYield::body(Runtime::Instance::MemoryInstance *) {
  if (auto Res = Env.schedYield(); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiRandomGet::body(Runtime::Instance::MemoryInstance *MemInst,
                                     uint32_t BufPtr, uint32_t BufLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  auto *const Buf = MemInst->getPointer<uint8_t *>(BufPtr, WasiBufLen);
  if (unlikely(Buf == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  if (auto Res = Env.randomGet({Buf, WasiBufLen}); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t AddressFamily, int32_t SockType,
                                    uint32_t /* Out */ RoFdPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_fd_t *const RoFd =
      MemInst->getPointer<__wasi_fd_t *>(RoFdPtr, sizeof(__wasi_fd_t));
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  if (auto Res = cast<__wasi_address_family_t>(AddressFamily); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAddressFamily = *Res;
  }

  __wasi_sock_type_t WasiSockType;
  if (auto Res = cast<__wasi_sock_type_t>(SockType); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSockType = *Res;
  }

  if (auto Res = Env.sockOpen(WasiAddressFamily, WasiSockType);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *RoFd = *Res;
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockBind::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t AddressPtr,
                                    uint32_t Port) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress = MemInst->getPointer<__wasi_address_t *>(
      AddressPtr, sizeof(__wasi_address_t));
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  if (InnerAddress->buf_len != 4 && InnerAddress->buf_len != 16) {
    return __WASI_ERRNO_INVAL;
  }

  uint8_t *AddressBuf = MemInst->getPointer<uint8_t *>(
      InnerAddress->buf, sizeof(uint8_t) * InnerAddress->buf_len);
  if (AddressBuf == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockBind(WasiFd, AddressBuf, InnerAddress->buf_len,
                              static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockListen::body(
    [[maybe_unused]] Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
    uint32_t Backlog) {
  const __wasi_fd_t WasiFd = Fd;
  if (auto Res = Env.sockListen(WasiFd, Backlog); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSockAccept::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                     int32_t Port, uint32_t /* Out */ RoFdPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_fd_t *const RoFd =
      MemInst->getPointer<__wasi_fd_t *>(RoFdPtr, sizeof(__wasi_fd_t));
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockAccept(WasiFd, static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  } else {
    *RoFd = *Res;
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSockConnect::body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                      uint32_t AddressPtr, uint32_t Port) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress = MemInst->getPointer<__wasi_address_t *>(
      AddressPtr, sizeof(__wasi_address_t));
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  if (InnerAddress->buf_len != 4 && InnerAddress->buf_len != 16) {
    return __WASI_ERRNO_INVAL;
  }

  uint8_t *AddressBuf = MemInst->getPointer<uint8_t *>(
      InnerAddress->buf, sizeof(uint8_t) * InnerAddress->buf_len);
  if (AddressBuf == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  if (auto Res =
          Env.sockConnect(WasiFd, AddressBuf, InnerAddress->buf_len, Port);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecv::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t RiDataPtr,
                                    uint32_t RiDataLen, uint32_t RiFlags,
                                    uint32_t /* Out */ RoDataLenPtr,
                                    uint32_t /* Out */ RoFlagsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_riflags_t WasiRiFlags;
  if (auto Res = cast<__wasi_riflags_t>(RiFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiRiFlags = *Res;
  }

  const __wasi_size_t WasiRiDataLen = RiDataLen;
  if (unlikely(WasiRiDataLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const RiDataArray =
      MemInst->getPointer<__wasi_iovec_t *>(RiDataPtr, WasiRiDataLen);
  if (unlikely(RiDataArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const RoDataLen = MemInst->getPointer<__wasi_size_t *>(RoDataLenPtr);
  if (unlikely(RoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const RoFlags = MemInst->getPointer<__wasi_roflags_t *>(RoFlagsPtr);
  if (unlikely(RoFlags == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_size_t TotalSize = 0;
  std::array<Span<uint8_t>, WASI::kIOVMax> WasiRiData;

  for (__wasi_size_t I = 0; I < WasiRiDataLen; ++I) {
    __wasi_iovec_t &RiData = RiDataArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(RiData.buf_len > Space) ? Space : RiData.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const RiDataArr = MemInst->getPointer<uint8_t *>(RiData.buf, BufLen);
    /// Check for invalid address.
    if (unlikely(RiDataArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiRiData[I] = {RiDataArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockRecv(WasiFd, {WasiRiData.data(), WasiRiDataLen},
                              WasiRiFlags, *RoDataLen, *RoFlags);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSend::body(Runtime::Instance::MemoryInstance *MemInst,
                                    int32_t Fd, uint32_t SiDataPtr,
                                    uint32_t SiDataLen, uint32_t SiFlags,
                                    uint32_t /* Out */ SoDataLenPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_siflags_t WasiSiFlags;
  if (auto Res = cast<__wasi_siflags_t>(SiFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSiFlags = *Res;
  }

  const __wasi_size_t WasiSiDataLen = SiDataLen;
  if (unlikely(WasiSiDataLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  /// Check for invalid address.
  auto *const SiDataArray =
      MemInst->getPointer<__wasi_ciovec_t *>(SiDataPtr, WasiSiDataLen);
  if (unlikely(SiDataArray == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const SoDataLen = MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  std::array<Span<const uint8_t>, WASI::kIOVMax> WasiSiData;

  for (__wasi_size_t I = 0; I < WasiSiDataLen; ++I) {
    __wasi_ciovec_t &SiData = SiDataArray[I];

    /// Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(SiData.buf_len > Space) ? Space : SiData.buf_len;
    TotalSize += BufLen;

    /// Check for invalid address.
    auto *const SiDataArr = MemInst->getPointer<uint8_t *>(SiData.buf, BufLen);
    /// Check for invalid address.
    if (unlikely(SiDataArr == nullptr)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiSiData[I] = {SiDataArr, BufLen};
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockSend(WasiFd, {WasiSiData.data(), WasiSiDataLen},
                              WasiSiFlags, *SoDataLen);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockShutdown::body(Runtime::Instance::MemoryInstance *,
                                        int32_t Fd, uint32_t SdFlags) {
  __wasi_sdflags_t WasiSdFlags;
  if (auto Res = cast<__wasi_sdflags_t>(SdFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSdFlags = *Res;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockShutdown(WasiFd, WasiSdFlags); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

} // namespace Host
} // namespace WasmEdge
