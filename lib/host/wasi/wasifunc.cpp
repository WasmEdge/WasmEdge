// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "host/wasi/wasifunc.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "executor/executor.h"
#include "host/wasi/environ.h"
#include "runtime/instance/memory.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>

#if defined(_MSC_VER) && !defined(__clang__)
#define __restrict__ __restrict
#endif

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
template <> struct WasiRawType<uint8_t> {
  using Type = uint8_t;
};
template <> struct WasiRawType<uint16_t> {
  using Type = uint16_t;
};
template <> struct WasiRawType<uint32_t> {
  using Type = uint32_t;
};
template <> struct WasiRawType<uint64_t> {
  using Type = uint64_t;
};

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
    const auto WasiFstFlags = static_cast<__wasi_fstflags_t>(FdFlags);
    if ((WasiFstFlags & __WASI_FSTFLAGS_ATIM) &&
        (WasiFstFlags & __WASI_FSTFLAGS_ATIM_NOW)) {
      return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    if ((WasiFstFlags & __WASI_FSTFLAGS_MTIM) &&
        (WasiFstFlags & __WASI_FSTFLAGS_MTIM_NOW)) {
      return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    return WasiFstFlags;
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
  case __WASI_ADDRESS_FAMILY_AF_UNIX:
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

template <>
WASI::WasiExpect<__wasi_sock_opt_level_t>
cast<__wasi_sock_opt_level_t>(uint64_t SockOptLevel) noexcept {
  switch (WasiRawTypeT<__wasi_sock_opt_level_t>(SockOptLevel)) {
  case __WASI_SOCK_OPT_LEVEL_SOL_SOCKET:
    return static_cast<__wasi_sock_opt_level_t>(SockOptLevel);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <>
WASI::WasiExpect<__wasi_sock_opt_so_t>
cast<__wasi_sock_opt_so_t>(uint64_t SockOptName) noexcept {
  switch (WasiRawTypeT<__wasi_sock_opt_so_t>(SockOptName)) {
  case __WASI_SOCK_OPT_SO_REUSEADDR:
  case __WASI_SOCK_OPT_SO_TYPE:
  case __WASI_SOCK_OPT_SO_ERROR:
  case __WASI_SOCK_OPT_SO_DONTROUTE:
  case __WASI_SOCK_OPT_SO_BROADCAST:
  case __WASI_SOCK_OPT_SO_SNDBUF:
  case __WASI_SOCK_OPT_SO_RCVBUF:
  case __WASI_SOCK_OPT_SO_KEEPALIVE:
  case __WASI_SOCK_OPT_SO_OOBINLINE:
  case __WASI_SOCK_OPT_SO_LINGER:
  case __WASI_SOCK_OPT_SO_RCVLOWAT:
  case __WASI_SOCK_OPT_SO_RCVTIMEO:
  case __WASI_SOCK_OPT_SO_SNDTIMEO:
  case __WASI_SOCK_OPT_SO_ACCEPTCONN:
  case __WASI_SOCK_OPT_SO_BINDTODEVICE:
    return static_cast<__wasi_sock_opt_so_t>(SockOptName);
  default:
    return WASI::WasiUnexpect(__WASI_ERRNO_INVAL);
  }
}

template <typename T, size_t MaxSize> class StaticVector {
public:
  constexpr StaticVector() = default;
  constexpr const T *data() const noexcept {
    return reinterpret_cast<T *>(Storage);
  }
  constexpr T *data() noexcept { return reinterpret_cast<T *>(Storage); }
  constexpr size_t size() const noexcept { return Size; }
  template <typename... ArgsT>
  void emplace_back_unchecked(ArgsT &&...Args) noexcept(
      std::is_nothrow_constructible_v<T, ArgsT...>) {
    assuming(Size < MaxSize);
    new (data() + Size) T(std::forward<ArgsT>(Args)...);
    ++Size;
  }
  ~StaticVector() noexcept(std::is_nothrow_destructible_v<T>) {
    std::destroy_n(data(), Size);
  }

private:
  size_t Size = 0;
  alignas(alignof(T)) uint8_t Storage[sizeof(T[MaxSize])];
};

bool AllowAFUNIX(const Runtime::CallingFrame &Frame,
                 __wasi_address_family_t AddressFamily) {
  if (AddressFamily == __WASI_ADDRESS_FAMILY_AF_UNIX) {
    return Frame.getExecutor()
        ->getConfigure()
        .getRuntimeConfigure()
        .isAllowAFUNIX();
  }
  return true;
}
} // namespace

Expect<uint32_t> WasiArgsGet::body(const Runtime::CallingFrame &Frame,
                                   uint32_t ArgvPtr, uint32_t ArgvBufPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // Store **Argv.
  const auto &Arguments = Env.getArguments();
  const uint32_t ArgvSize = static_cast<uint32_t>(Arguments.size());
  const uint32_t ArgvBufSize = calculateBufferSize(Arguments);

  // Check for invalid address.
  const auto Argv = MemInst->getSpan<uint8_t_ptr>(ArgvPtr, ArgvSize);
  if (unlikely(Argv.size() != ArgvSize)) {
    return __WASI_ERRNO_FAULT;
  }
  const auto ArgvBuf = MemInst->getSpan<uint8_t>(ArgvBufPtr, ArgvBufSize);
  if (unlikely(ArgvBuf.size() != ArgvBufSize)) {
    return __WASI_ERRNO_FAULT;
  }

  if (!Argv.empty()) {
    Argv[0] = ArgvBufPtr;
  }

  if (auto Res = Env.argsGet(Argv, ArgvBuf); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiArgsSizesGet::body(const Runtime::CallingFrame &Frame,
                                        uint32_t /* Out */ ArgcPtr,
                                        uint32_t /* Out */ ArgvBufSizePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // Check for invalid address.
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

Expect<uint32_t> WasiEnvironGet::body(const Runtime::CallingFrame &Frame,
                                      uint32_t EnvPtr, uint32_t EnvBufPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // Store **Env.
  const auto &EnvironVariables = Env.getEnvironVariables();
  const uint32_t EnvSize = static_cast<uint32_t>(EnvironVariables.size());
  const uint32_t EnvBufSize = calculateBufferSize(EnvironVariables);

  // Check for invalid address.
  const auto EnvSpan = MemInst->getSpan<uint8_t_ptr>(EnvPtr, EnvSize);
  if (unlikely(EnvSpan.size() != EnvSize)) {
    return __WASI_ERRNO_FAULT;
  }
  const auto EnvBuf = MemInst->getSpan<uint8_t>(EnvBufPtr, EnvBufSize);
  if (unlikely(EnvBuf.size() != EnvBufSize)) {
    return __WASI_ERRNO_FAULT;
  }

  if (!EnvSpan.empty()) {
    EnvSpan[0] = EnvBufPtr;
  }

  if (auto Res = this->Env.environGet(EnvSpan, EnvBuf); unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiEnvironSizesGet::body(const Runtime::CallingFrame &Frame,
                                           uint32_t /* Out */ EnvCntPtr,
                                           uint32_t /* Out */ EnvBufSizePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // Check for invalid address.
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

Expect<uint32_t> WasiClockResGet::body(const Runtime::CallingFrame &Frame,
                                       uint32_t ClockId,
                                       uint32_t /* Out */ ResolutionPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> WasiClockTimeGet::body(const Runtime::CallingFrame &Frame,
                                        uint32_t ClockId, uint64_t Precision,
                                        uint32_t /* Out */ TimePtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> WasiFdAdvise::body(const Runtime::CallingFrame &, int32_t Fd,
                                    uint64_t Offset, uint64_t Len,
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

Expect<uint32_t> WasiFdAllocate::body(const Runtime::CallingFrame &, int32_t Fd,
                                      uint64_t Offset, uint64_t Len) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;
  const __wasi_filesize_t WasiLen = Len;

  if (auto Res = Env.fdAllocate(WasiFd, WasiOffset, WasiLen); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdClose::body(const Runtime::CallingFrame &, int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdClose(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdDatasync::body(const Runtime::CallingFrame &,
                                      int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdDatasync(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdFdstatGet::body(const Runtime::CallingFrame &Frame,
                                       int32_t Fd,
                                       uint32_t /* Out */ FdStatPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> WasiFdFdstatSetFlags::body(const Runtime::CallingFrame &,
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

Expect<uint32_t> WasiFdFdstatSetRights::body(const Runtime::CallingFrame &,
                                             int32_t Fd, uint64_t FsRightsBase,
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

Expect<uint32_t> WasiFdFilestatGet::body(const Runtime::CallingFrame &Frame,
                                         int32_t Fd,
                                         uint32_t /* Out */ FilestatPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> WasiFdFilestatSetSize::body(const Runtime::CallingFrame &,
                                             int32_t Fd, uint64_t Size) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiSize = Size;

  if (auto Res = Env.fdFilestatSetSize(WasiFd, WasiSize); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdFilestatSetTimes::body(const Runtime::CallingFrame &,
                                              int32_t Fd, uint64_t ATim,
                                              uint64_t MTim,
                                              uint32_t FstFlags) {
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

Expect<uint32_t> WasiFdPread::body(const Runtime::CallingFrame &Frame,
                                   int32_t Fd, uint32_t IOVsPtr,
                                   uint32_t IOVsLen, uint64_t Offset,
                                   uint32_t /* Out */ NReadPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  // Check for invalid address.
  const auto IOVsArray = MemInst->getSpan<__wasi_iovec_t>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray.size() != WasiIOVsLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (auto &IOV : IOVsArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto ReadArr = MemInst->getSpan<uint8_t>(IOV.buf, BufLen);
    if (unlikely(ReadArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs.emplace_back_unchecked(ReadArr);
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;

  if (auto Res = Env.fdPread(WasiFd, WasiIOVs, WasiOffset, *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPrestatDirName::body(const Runtime::CallingFrame &Frame,
                                            int32_t Fd, uint32_t PathBufPtr,
                                            uint32_t PathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto PathBuf = MemInst->getSpan<uint8_t>(PathBufPtr, PathLen);
  if (unlikely(PathBuf.size() != PathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdPrestatDirName(WasiFd, PathBuf); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdPrestatGet::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd,
                                        uint32_t /* Out */ PreStatPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

Expect<uint32_t> WasiFdPwrite::body(const Runtime::CallingFrame &Frame,
                                    int32_t Fd, uint32_t IOVsPtr,
                                    uint32_t IOVsLen, uint64_t Offset,
                                    uint32_t /* Out */ NWrittenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  // Check for invalid address.
  const auto IOVsArray =
      MemInst->getSpan<__wasi_ciovec_t>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray.size() != WasiIOVsLen)) {
    return __WASI_ERRNO_FAULT;
  }

  // Check for invalid address.
  auto *const NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (auto &IOV : IOVsArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto WriteArr = MemInst->getSpan<const uint8_t>(IOV.buf, BufLen);
    if (unlikely(WriteArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs.emplace_back_unchecked(WriteArr);
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_filesize_t WasiOffset = Offset;

  if (auto Res = Env.fdPwrite(WasiFd, WasiIOVs, WasiOffset, *NWritten);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdRead::body(const Runtime::CallingFrame &Frame,
                                  int32_t Fd, uint32_t IOVsPtr,
                                  uint32_t IOVsLen,
                                  uint32_t /* Out */ NReadPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  // Check for invalid address.
  const auto IOVsArray = MemInst->getSpan<__wasi_iovec_t>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray.size() != WasiIOVsLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (auto &IOV : IOVsArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto ReadArr = MemInst->getSpan<uint8_t>(IOV.buf, BufLen);
    if (unlikely(ReadArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs.emplace_back_unchecked(ReadArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdRead(WasiFd, WasiIOVs, *NRead); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdReadDir::body(const Runtime::CallingFrame &Frame,
                                     int32_t Fd, uint32_t BufPtr,
                                     uint32_t BufLen, uint64_t Cookie,
                                     uint32_t /* Out */ NReadPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  // Check for invalid address.
  const auto Buf = MemInst->getSpan<uint8_t>(BufPtr, WasiBufLen);
  if (unlikely(Buf.size() != WasiBufLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_dircookie_t WasiCookie = Cookie;

  if (auto Res = Env.fdReaddir(WasiFd, Buf, WasiCookie, *NRead);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdRenumber::body(const Runtime::CallingFrame &, int32_t Fd,
                                      int32_t ToFd) {
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_fd_t WasiToFd = ToFd;

  if (auto Res = Env.fdRenumber(WasiFd, WasiToFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<int32_t> WasiFdSeek::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                                 int64_t Offset, uint32_t Whence,
                                 uint32_t /* Out */ NewOffsetPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_whence_t WasiWhence;
  if (auto Res = cast<__wasi_whence_t>(Whence); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiWhence = *Res;
  }

  // Check for invalid address.
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

Expect<uint32_t> WasiFdSync::body(const Runtime::CallingFrame &, int32_t Fd) {
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdSync(WasiFd); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiFdTell::body(const Runtime::CallingFrame &Frame,
                                  int32_t Fd, uint32_t /* Out */ OffsetPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // Check for invalid address.
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

Expect<uint32_t> WasiFdWrite::body(const Runtime::CallingFrame &Frame,
                                   int32_t Fd, uint32_t IOVsPtr,
                                   uint32_t IOVsLen,
                                   uint32_t /* Out */ NWrittenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiIOVsLen = IOVsLen;
  if (unlikely(WasiIOVsLen > WASI::kIOVMax)) {
    return __WASI_ERRNO_INVAL;
  }

  // Check for invalid address.
  const auto IOVsArray =
      MemInst->getSpan<__wasi_ciovec_t>(IOVsPtr, WasiIOVsLen);
  if (unlikely(IOVsArray.size() != WasiIOVsLen)) {
    return __WASI_ERRNO_FAULT;
  }

  // Check for invalid address.
  auto *const NWritten = MemInst->getPointer<__wasi_size_t *>(NWrittenPtr);
  if (unlikely(NWritten == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiIOVs;

  for (auto &IOV : IOVsArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(IOV.buf_len > Space) ? Space : IOV.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto WriteArr = MemInst->getSpan<const uint8_t>(IOV.buf, BufLen);
    if (unlikely(WriteArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiIOVs.emplace_back_unchecked(WriteArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.fdWrite(WasiFd, WasiIOVs, *NWritten); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathCreateDirectory::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                              uint32_t PathPtr, uint32_t PathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathCreateDirectory(WasiFd, Path); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathFilestatGet::body(const Runtime::CallingFrame &Frame,
                                           int32_t Fd, uint32_t Flags,
                                           uint32_t PathPtr, uint32_t PathLen,
                                           uint32_t /* Out */ FilestatPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathFilestatGet(WasiFd, Path, WasiFlags, *Filestat);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathFilestatSetTimes::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                               uint32_t Flags, uint32_t PathPtr,
                               uint32_t PathLen, uint64_t ATim, uint64_t MTim,
                               uint32_t FstFlags) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_timestamp_t WasiATim = ATim;
  const __wasi_timestamp_t WasiMTim = MTim;

  if (auto Res = Env.pathFilestatSetTimes(WasiFd, Path, WasiFlags, WasiATim,
                                          WasiMTim, WasiFstFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathLink::body(const Runtime::CallingFrame &Frame,
                                    int32_t OldFd, uint32_t OldFlags,
                                    uint32_t OldPathPtr, uint32_t OldPathLen,
                                    int32_t NewFd, uint32_t NewPathPtr,
                                    uint32_t NewPathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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
  const auto OldPath = MemInst->getStringView(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath.size() != WasiOldPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;
  const auto NewPath = MemInst->getStringView(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath.size() != WasiNewPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiOldFd = OldFd;
  const __wasi_fd_t WasinewFd = NewFd;

  if (auto Res =
          Env.pathLink(WasiOldFd, OldPath, WasinewFd, NewPath, WasiOldFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathOpen::body(
    const Runtime::CallingFrame &Frame, int32_t DirFd, uint32_t DirFlags,
    uint32_t PathPtr, uint32_t PathLen, uint32_t OFlags, uint64_t FsRightsBase,
    uint64_t FsRightsInheriting, uint32_t FsFlags, uint32_t /* Out */ FdPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const Fd = MemInst->getPointer<__wasi_fd_t *>(FdPtr);
  if (unlikely(Fd == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  // Open directory and read/write rights should fail with isdir
  if ((WasiOFlags & __WASI_OFLAGS_DIRECTORY) &&
      (WasiFsRightsBase & __WASI_RIGHTS_FD_READ) &&
      (WasiFsRightsBase & __WASI_RIGHTS_FD_WRITE)) {
    return __WASI_ERRNO_ISDIR;
  }

  const __wasi_fd_t WasiDirFd = DirFd;

  if (auto Res =
          Env.pathOpen(WasiDirFd, Path, WasiDirFlags, WasiOFlags,
                       WasiFsRightsBase, WasiFsRightsInheriting, WasiFsFlags);
      unlikely(!Res)) {
    return Res.error();
  } else {
    *Fd = *Res;
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathReadLink::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd, uint32_t PathPtr,
                                        uint32_t PathLen, uint32_t BufPtr,
                                        uint32_t BufLen,
                                        uint32_t /* Out */ NReadPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  const auto Buf = MemInst->getSpan<char>(BufPtr, WasiBufLen);
  if (unlikely(Buf.size() != WasiBufLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NRead = MemInst->getPointer<__wasi_size_t *>(NReadPtr);
  if (unlikely(NRead == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathReadlink(WasiFd, Path, Buf, *NRead); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiPathRemoveDirectory::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                              uint32_t PathPtr, uint32_t PathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathRemoveDirectory(WasiFd, Path); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathRename::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t OldPathPtr,
                                      uint32_t OldPathLen, int32_t NewFd,
                                      uint32_t NewPathPtr,
                                      uint32_t NewPathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiOldPathLen = OldPathLen;
  const auto OldPath = MemInst->getStringView(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath.size() != WasiOldPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;
  const auto NewPath = MemInst->getStringView(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath.size() != WasiNewPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;
  const __wasi_fd_t WasiNewFd = NewFd;

  if (auto Res = Env.pathRename(WasiFd, OldPath, WasiNewFd, NewPath);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathSymlink::body(const Runtime::CallingFrame &Frame,
                                       uint32_t OldPathPtr, uint32_t OldPathLen,
                                       int32_t Fd, uint32_t NewPathPtr,
                                       uint32_t NewPathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiOldPathLen = OldPathLen;
  const auto OldPath = MemInst->getStringView(OldPathPtr, WasiOldPathLen);
  if (unlikely(OldPath.size() != WasiOldPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNewPathLen = NewPathLen;
  const auto NewPath = MemInst->getStringView(NewPathPtr, WasiNewPathLen);
  if (unlikely(NewPath.size() != WasiNewPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  if (OldPath.empty() || NewPath.empty()) {
    return __WASI_ERRNO_NOENT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathSymlink(OldPath, WasiFd, NewPath); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiPathUnlinkFile::body(const Runtime::CallingFrame &Frame,
                                          int32_t Fd, uint32_t PathPtr,
                                          uint32_t PathLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiPathLen = PathLen;
  const auto Path = MemInst->getStringView(PathPtr, WasiPathLen);
  if (unlikely(Path.size() != WasiPathLen)) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.pathUnlinkFile(WasiFd, Path); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}
template <WASI::TriggerType Trigger>
Expect<uint32_t> WasiPollOneoff<Trigger>::body(
    const Runtime::CallingFrame &Frame, uint32_t InPtr, uint32_t OutPtr,
    uint32_t NSubscriptions, uint32_t /* Out */ NEventsPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiNSub = NSubscriptions;

  const auto Subs =
      MemInst->getSpan<const __wasi_subscription_t>(InPtr, WasiNSub);
  if (unlikely(Subs.size() != WasiNSub)) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Events = MemInst->getSpan<__wasi_event_t>(OutPtr, WasiNSub);
  if (unlikely(Events.size() != WasiNSub)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const NEvents = MemInst->getPointer<__wasi_size_t *>(NEventsPtr);
  if (unlikely(NEvents == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  // Validate contents
  if (auto Poll = this->Env.acquirePoller(Events); unlikely(!Poll)) {
    for (__wasi_size_t I = 0; I < WasiNSub; ++I) {
      Events[I].userdata = Subs[I].userdata;
      Events[I].error = Poll.error();
      Events[I].type = Subs[I].u.tag;
    }
    *NEvents = WasiNSub;
    return Poll.error();
  } else {
    auto &Poller = *Poll;
    for (auto &Sub : Subs) {
      const __wasi_userdata_t WasiUserData = Sub.userdata;

      __wasi_eventtype_t Type;
      if (auto Res = cast<__wasi_eventtype_t>(Sub.u.tag); unlikely(!Res)) {
        Poller.error(WasiUserData, Res.error(), Sub.u.tag);
        continue;
      } else {
        Type = *Res;
      }

      switch (Type) {
      case __WASI_EVENTTYPE_CLOCK: {
        __wasi_clockid_t WasiClockId;
        if (auto Res = cast<__wasi_clockid_t>(Sub.u.u.clock.id);
            unlikely(!Res)) {
          Poller.error(WasiUserData, Res.error(), Type);
          continue;
        } else {
          WasiClockId = *Res;
        }

        __wasi_subclockflags_t WasiFlags;
        if (auto Res = cast<__wasi_subclockflags_t>(Sub.u.u.clock.flags);
            unlikely(!Res)) {
          Poller.error(WasiUserData, Res.error(), Type);
          continue;
        } else {
          WasiFlags = *Res;
        }

        const __wasi_timestamp_t WasiTimeout = Sub.u.u.clock.timeout;
        const __wasi_timestamp_t WasiPrecision = Sub.u.u.clock.precision;

        Poller.clock(WasiClockId, WasiTimeout, WasiPrecision, WasiFlags,
                     WasiUserData);
        continue;
      }
      case __WASI_EVENTTYPE_FD_READ: {
        const __wasi_fd_t WasiFd = Sub.u.u.fd_read.file_descriptor;
        Poller.read(WasiFd, Trigger, WasiUserData);
        continue;
      }
      case __WASI_EVENTTYPE_FD_WRITE: {
        const __wasi_fd_t WasiFd = Sub.u.u.fd_write.file_descriptor;
        Poller.write(WasiFd, Trigger, WasiUserData);
        continue;
      }
      default:
        assumingUnreachable();
      }
    }
    Poller.wait();
    *NEvents = Poller.result();
    Poller.reset();
    this->Env.releasePoller(std::move(Poller));
  }

  return __WASI_ERRNO_SUCCESS;
}

template class WasiPollOneoff<WASI::TriggerType::Level>;
template class WasiPollOneoff<WASI::TriggerType::Edge>;

Expect<void> WasiProcExit::body(const Runtime::CallingFrame &,
                                uint32_t ExitCode) {
  Env.procExit(ExitCode);
  return Unexpect(ErrCode::Value::Terminated);
}

Expect<uint32_t> WasiProcRaise::body(const Runtime::CallingFrame &,
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

Expect<uint32_t> WasiSchedYield::body(const Runtime::CallingFrame &) {
  if (auto Res = Env.schedYield(); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiRandomGet::body(const Runtime::CallingFrame &Frame,
                                     uint32_t BufPtr, uint32_t BufLen) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_size_t WasiBufLen = BufLen;

  const auto Buf = MemInst->getSpan<uint8_t>(BufPtr, WasiBufLen);
  if (unlikely(Buf.size() != WasiBufLen)) {
    return __WASI_ERRNO_FAULT;
  }

  if (auto Res = Env.randomGet(Buf); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockOpenV1::body(const Runtime::CallingFrame &Frame,
                                      uint32_t AddressFamily, uint32_t SockType,
                                      uint32_t /* Out */ RoFdPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_fd_t *const RoFd = MemInst->getPointer<__wasi_fd_t *>(RoFdPtr);
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  if (auto Res = cast<__wasi_address_family_t>(AddressFamily); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAddressFamily = *Res;
  }

  if (!AllowAFUNIX(Frame, WasiAddressFamily)) {
    return __WASI_ERRNO_NOSYS;
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

Expect<uint32_t> WasiSockBindV1::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t AddressPtr,
                                      uint32_t Port) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<const uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockBind(WasiFd, WasiAddressFamily, Address,
                              static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockListenV1::body(const Runtime::CallingFrame &,
                                        int32_t Fd, int32_t Backlog) {
  const __wasi_fd_t WasiFd = Fd;
  if (auto Res = Env.sockListen(WasiFd, Backlog); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockAcceptV1::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd,
                                        uint32_t /* Out */ RoFdPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_fd_t *const RoFd = MemInst->getPointer<__wasi_fd_t *>(RoFdPtr);
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  const __wasi_fd_t WasiFd = Fd;
  const __wasi_fdflags_t WasiFdFlags = static_cast<__wasi_fdflags_t>(0);
  if (auto Res = Env.sockAccept(WasiFd, WasiFdFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    *RoFd = *Res;
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockAcceptV2::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd, uint32_t FsFlags,
                                        uint32_t /* Out */ RoFdPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_fd_t *const RoFd = MemInst->getPointer<__wasi_fd_t *>(RoFdPtr);
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  const __wasi_fd_t WasiFd = Fd;

  __wasi_fdflags_t WasiFdFlags;
  if (auto Res = cast<__wasi_fdflags_t>(FsFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiFdFlags = *Res;
  }

  if (auto Res = Env.sockAccept(WasiFd, WasiFdFlags); unlikely(!Res)) {
    return Res.error();
  } else {
    *RoFd = *Res;
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockConnectV1::body(const Runtime::CallingFrame &Frame,
                                         int32_t Fd, uint32_t AddressPtr,
                                         uint32_t Port) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;
  if (auto Res = Env.sockConnect(WasiFd, WasiAddressFamily, Address,
                                 static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecvV1::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t RiDataPtr,
                                      uint32_t RiDataLen, uint32_t RiFlags,
                                      uint32_t /* Out */ RoDataLenPtr,
                                      uint32_t /* Out */ RoFlagsPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

  // Check for invalid address.
  const auto RiDataArray =
      MemInst->getSpan<__wasi_iovec_t>(RiDataPtr, WasiRiDataLen);
  if (unlikely(RiDataArray.size() != WasiRiDataLen)) {
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
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiRiData;

  for (auto &RiData : RiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(RiData.buf_len > Space) ? Space : RiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto RiDataArr = MemInst->getSpan<uint8_t>(RiData.buf, BufLen);
    // Check for invalid address.
    if (unlikely(RiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiRiData.emplace_back_unchecked(RiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockRecv(WasiFd, WasiRiData, WasiRiFlags, *RoDataLen, *RoFlags);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecvFromV1::body(const Runtime::CallingFrame &Frame,
                                          int32_t Fd, uint32_t RiDataPtr,
                                          uint32_t RiDataLen,
                                          uint32_t AddressPtr, uint32_t RiFlags,
                                          uint32_t /* Out */ RoDataLenPtr,
                                          uint32_t /* Out */ RoFlagsPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
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

  const auto RiDataArray =
      MemInst->getSpan<__wasi_iovec_t>(RiDataPtr, WasiRiDataLen);
  if (unlikely(RiDataArray.size() != WasiRiDataLen)) {
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
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiRiData;

  for (auto &RiData : RiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(RiData.buf_len > Space) ? Space : RiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto RiDataArr = MemInst->getSpan<uint8_t>(RiData.buf, BufLen);
    // Check for invalid address.
    if (unlikely(RiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiRiData.emplace_back_unchecked(RiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockRecvFrom(WasiFd, WasiRiData, WasiRiFlags, nullptr,
                                  Address, nullptr, *RoDataLen, *RoFlags);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSendV1::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t SiDataPtr,
                                      uint32_t SiDataLen, uint32_t SiFlags,
                                      uint32_t /* Out */ SoDataLenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

  // Check for invalid address.
  const auto SiDataArray =
      MemInst->getSpan<__wasi_ciovec_t>(SiDataPtr, WasiSiDataLen);
  if (unlikely(SiDataArray.size() != WasiSiDataLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const SoDataLen = MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiSiData;

  for (auto &SiData : SiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(SiData.buf_len > Space) ? Space : SiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto SiDataArr = MemInst->getSpan<uint8_t>(SiData.buf, BufLen);
    if (unlikely(SiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiSiData.emplace_back_unchecked(SiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockSend(WasiFd, WasiSiData, WasiSiFlags, *SoDataLen);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSendToV1::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd, uint32_t SiDataPtr,
                                        uint32_t SiDataLen, uint32_t AddressPtr,
                                        int32_t Port, uint32_t SiFlags,
                                        uint32_t /* Out */ SoDataLenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  default:
    return __WASI_ERRNO_INVAL;
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

  // Check for invalid address.
  const auto SiDataArray =
      MemInst->getSpan<__wasi_ciovec_t>(SiDataPtr, WasiSiDataLen);
  if (unlikely(SiDataArray.size() != WasiSiDataLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const SoDataLen = MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiSiData;

  for (auto &SiData : SiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(SiData.buf_len > Space) ? Space : SiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto SiDataArr = MemInst->getSpan<uint8_t>(SiData.buf, BufLen);
    if (unlikely(SiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiSiData.emplace_back_unchecked(SiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockSendTo(WasiFd, WasiSiData, WasiSiFlags, WasiAddressFamily,
                         Address, static_cast<uint16_t>(Port), *SoDataLen);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockShutdown::body(const Runtime::CallingFrame &,
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

Expect<uint32_t> WasiSockSetOpt::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t SockOptLevel,
                                      uint32_t SockOptName, uint32_t FlagPtr,
                                      uint32_t FlagSize) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_sock_opt_level_t WasiSockOptLevel;
  if (auto Res = cast<__wasi_sock_opt_level_t>(SockOptLevel); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSockOptLevel = *Res;
  }

  __wasi_sock_opt_so_t WasiSockOptName;
  if (auto Res = cast<__wasi_sock_opt_so_t>(SockOptName); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSockOptName = *Res;
  }

  const auto Flag = MemInst->getSpan<uint8_t>(FlagPtr, FlagSize);
  if (Flag.size() != FlagSize) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockSetOpt(WasiFd, WasiSockOptLevel, WasiSockOptName, Flag);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockGetAddrinfo::body(
    const Runtime::CallingFrame &Frame, uint32_t NodePtr, uint32_t NodeLen,
    uint32_t ServicePtr, uint32_t ServiceLen, uint32_t HintsPtr,
    uint32_t ResPtr, uint32_t MaxResLength, uint32_t ResLengthPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Node = MemInst->getStringView(NodePtr, NodeLen);
  if (Node.size() != NodeLen) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Service = MemInst->getStringView(ServicePtr, ServiceLen);
  if (Service.size() != ServiceLen) {
    return __WASI_ERRNO_FAULT;
  }

  auto *Hint = MemInst->getPointer<const __wasi_addrinfo_t *>(HintsPtr);
  if (Hint == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const ResBuf = MemInst->getPointer<const uint8_t_ptr *>(ResPtr);
  if (ResBuf == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const ResLength = MemInst->getPointer<__wasi_size_t *>(ResLengthPtr);
  if (ResLength == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  // service and node can not be empty at the same time
  if (Service.empty() && Node.empty()) {
    return __WASI_ERRNO_AINONAME;
  }

  if (MaxResLength < 1) {
    return __WASI_ERRNO_AIMEMORY;
  }

  if (Hint->ai_flags &
      ~(__WASI_AIFLAGS_AI_PASSIVE | __WASI_AIFLAGS_AI_CANONNAME |
        __WASI_AIFLAGS_AI_NUMERICHOST | __WASI_AIFLAGS_AI_NUMERICSERV |
        __WASI_AIFLAGS_AI_ADDRCONFIG | __WASI_AIFLAGS_AI_V4MAPPED |
        __WASI_AIFLAGS_AI_ALL)) {
    return __WASI_ERRNO_AIBADFLAG;
  }
  if (Hint->ai_flags & __WASI_AIFLAGS_AI_CANONNAME && Node.empty()) {
    return __WASI_ERRNO_AIBADFLAG;
  }
  switch (Hint->ai_family) {
  case __WASI_ADDRESS_FAMILY_UNSPEC:
  case __WASI_ADDRESS_FAMILY_INET4:
  case __WASI_ADDRESS_FAMILY_INET6:
    break;
  default:
    return __WASI_ERRNO_AIFAMILY;
  }
  switch (Hint->ai_protocol) {
  case __WASI_PROTOCOL_IPPROTO_IP:
  case __WASI_PROTOCOL_IPPROTO_TCP:
  case __WASI_PROTOCOL_IPPROTO_UDP:
    break;
  default:
    return __WASI_ERRNO_NOSYS;
  }
  switch (Hint->ai_socktype) {
  case __WASI_SOCK_TYPE_SOCK_ANY:
  case __WASI_SOCK_TYPE_SOCK_DGRAM:
  case __WASI_SOCK_TYPE_SOCK_STREAM:
    break;
  default:
    return __WASI_ERRNO_NOSYS;
  }

  auto initWasiAddrinfoArray =
      [&MemInst](uint8_t_ptr Base, uint32_t Length,
                 Span<__wasi_addrinfo_t *> WasiAddrinfoArray) noexcept
      -> WASI::WasiExpect<void> {
    for (uint32_t Item = 0; Item < Length; Item++) {
      auto *const TmpAddrinfo = MemInst->getPointer<__wasi_addrinfo_t *>(Base);
      if (TmpAddrinfo == nullptr) {
        return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
      }
      WasiAddrinfoArray[Item] = TmpAddrinfo;
      Base = TmpAddrinfo->ai_next;
    }
    return {};
  };

  auto initAiAddrArray =
      [&MemInst](Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                 Span<__wasi_sockaddr_t *> WasiSockAddrArray) noexcept
      -> WASI::WasiExpect<void> {
    for (uint32_t Item = 0; Item < WasiAddrinfoArray.size(); Item++) {
      auto *const Addr = MemInst->getPointer<__wasi_sockaddr_t *>(
          WasiAddrinfoArray[Item]->ai_addr);
      if (Addr == nullptr) {
        return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
      }
      WasiSockAddrArray[Item] = Addr;
    }
    return {};
  };

  auto initAiAddrSaDataArray =
      [&MemInst](Span<__wasi_sockaddr_t *> WasiSockAddrArray,
                 Span<char *> AiSockAddrSaDataArray) noexcept
      -> WASI::WasiExpect<void> {
    for (uint32_t Item = 0; Item < WasiSockAddrArray.size(); Item++) {
      const auto WasiSockAddr =
          MemInst->getSpan<char>(WasiSockAddrArray[Item]->sa_data,
                                 WasiSockAddrArray[Item]->sa_data_len);
      if (WasiSockAddr.size() != WasiSockAddrArray[Item]->sa_data_len) {
        return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
      }
      AiSockAddrSaDataArray[Item] = WasiSockAddr.data();
    }
    return {};
  };

  auto initAiCanonnameArray =
      [&MemInst](Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                 Span<char *> WasiAddrinfoCanonnameArray) noexcept
      -> WASI::WasiExpect<void> {
    for (uint32_t Item = 0; Item < WasiAddrinfoArray.size(); Item++) {
      const auto CanonName =
          MemInst->getSpan<char>(WasiAddrinfoArray[Item]->ai_canonname,
                                 WasiAddrinfoArray[Item]->ai_canonname_len);
      if (CanonName.size() != WasiAddrinfoArray[Item]->ai_canonname_len) {
        return WASI::WasiUnexpect(__WASI_ERRNO_FAULT);
      }
      WasiAddrinfoCanonnameArray[Item] = CanonName.data();
    }
    return {};
  };

  std::vector<__wasi_addrinfo_t *> WasiAddrinfoArray(MaxResLength, nullptr);
  std::vector<__wasi_sockaddr_t *> WasiSockAddrArray(MaxResLength, nullptr);
  std::vector<char *> AiAddrSaDataArray(MaxResLength, nullptr);
  std::vector<char *> AiCanonnameArray(MaxResLength, nullptr);

  if (auto Res =
          initWasiAddrinfoArray(*ResBuf, MaxResLength, WasiAddrinfoArray);
      unlikely(!Res)) {
    return WASI::WasiUnexpect(Res);
  }
  initAiAddrArray(WasiAddrinfoArray, WasiSockAddrArray);
  if (auto Res = initAiAddrSaDataArray(WasiSockAddrArray, AiAddrSaDataArray);
      unlikely(!Res)) {
    return WASI::WasiUnexpect(Res);
  }
  if (auto Res = initAiCanonnameArray(WasiAddrinfoArray, AiCanonnameArray);
      unlikely(!Res)) {
    return WASI::WasiUnexpect(Res);
  }

  if (auto Res = Env.getAddrInfo(
          Node, Service, *Hint, MaxResLength, WasiAddrinfoArray,
          WasiSockAddrArray, AiAddrSaDataArray, AiCanonnameArray, *ResLength);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSockGetLocalAddrV1::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                             uint32_t AddressPtr, uint32_t AddressTypePtr,
                             uint32_t PortPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  switch (Address.size()) {
  case 4:
    break;
  case 16:
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  uint32_t *const RoAddressType =
      MemInst->getPointer<uint32_t *>(AddressTypePtr);
  if (RoAddressType == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_family_t AddressType;

  uint32_t *const RoPort = MemInst->getPointer<uint32_t *>(PortPtr);
  if (RoPort == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  uint16_t Port;
  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockGetLocalAddr(WasiFd, &AddressType, Address, &Port);
      unlikely(!Res)) {
    return Res.error();
  }
  *RoPort = Port;
  // XXX: This is a workaround
  // The correct one should be `*RoAddressType = AddressType;`
  // However, due to this bugfix will break the existing applications.
  // So we changed back to the old way.
  switch (AddressType) {
  case __WASI_ADDRESS_FAMILY_INET4:
    *RoAddressType = 4;
    break;
  case __WASI_ADDRESS_FAMILY_INET6:
    *RoAddressType = 6;
    break;
  default:
    assumingUnreachable();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockGetPeerAddrV1::body(const Runtime::CallingFrame &Frame,
                                             int32_t Fd, uint32_t AddressPtr,
                                             uint32_t AddressTypePtr,
                                             uint32_t PortPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  switch (Address.size()) {
  case 4:
    break;
  case 16:
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }

  uint32_t *const RoAddressType =
      MemInst->getPointer<uint32_t *>(AddressTypePtr);
  if (RoAddressType == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_family_t AddressType;

  uint32_t *const RoPort = MemInst->getPointer<uint32_t *>(PortPtr);
  if (RoPort == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  uint16_t Port;

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockGetPeerAddr(WasiFd, &AddressType, Address, &Port);
      unlikely(!Res)) {
    return Res.error();
  }
  *RoPort = Port;
  // XXX: This is a workaround
  // The correct one should be `*RoAddressType = AddressType;`
  // However, due to this bugfix will break the existing applications.
  // So we changed back to the old way.
  switch (AddressType) {
  case __WASI_ADDRESS_FAMILY_INET4:
    *RoAddressType = 4;
    break;
  case __WASI_ADDRESS_FAMILY_INET6:
    *RoAddressType = 6;
    break;
  default:
    assumingUnreachable();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockOpenV2::body(const Runtime::CallingFrame &Frame,
                                      uint32_t AddressFamily, uint32_t SockType,
                                      uint32_t /* Out */ RoFdPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_fd_t *const RoFd = MemInst->getPointer<__wasi_fd_t *>(RoFdPtr);
  if (RoFd == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  if (auto Res = cast<__wasi_address_family_t>(AddressFamily); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiAddressFamily = *Res;
  }

  if (!AllowAFUNIX(Frame, WasiAddressFamily)) {
    return __WASI_ERRNO_NOSYS;
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

Expect<uint32_t> WasiSockBindV2::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t AddressPtr,
                                      uint32_t Port) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  auto *const InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<const uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  case 128: {
    auto &Storage =
        *reinterpret_cast<const WASI::WasiAddrStorage *>(Address.data());
    WasiAddressFamily = Storage.getAddressFamily();
    Address = Storage.getAddress();
    break;
  }
  default:
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockBind(WasiFd, WasiAddressFamily, Address,
                              static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockListenV2::body(const Runtime::CallingFrame &,
                                        int32_t Fd, int32_t Backlog) {
  const __wasi_fd_t WasiFd = Fd;
  if (auto Res = Env.sockListen(WasiFd, Backlog); unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockConnectV2::body(const Runtime::CallingFrame &Frame,
                                         int32_t Fd, uint32_t AddressPtr,
                                         uint32_t Port) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<const uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  case 128: {
    auto &Storage =
        *reinterpret_cast<const WASI::WasiAddrStorage *>(Address.data());
    WasiAddressFamily = Storage.getAddressFamily();
    Address = Storage.getAddress();
    break;
  }
  default:
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;
  if (auto Res = Env.sockConnect(WasiFd, WasiAddressFamily, Address,
                                 static_cast<uint16_t>(Port));
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecvV2::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t RiDataPtr,
                                      uint32_t RiDataLen, uint32_t RiFlags,
                                      uint32_t /* Out */ RoDataLenPtr,
                                      uint32_t /* Out */ RoFlagsPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

  // Check for invalid address.
  const auto RiDataArray =
      MemInst->getSpan<__wasi_iovec_t>(RiDataPtr, WasiRiDataLen);
  if (unlikely(RiDataArray.size() != WasiRiDataLen)) {
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
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiRiData;

  for (auto &RiData : RiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(RiData.buf_len > Space) ? Space : RiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto RiDataArr = MemInst->getSpan<uint8_t>(RiData.buf, BufLen);
    if (unlikely(RiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiRiData.emplace_back_unchecked(RiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockRecv(WasiFd, WasiRiData, WasiRiFlags, *RoDataLen, *RoFlags);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockRecvFromV2::body(const Runtime::CallingFrame &Frame,
                                          int32_t Fd, uint32_t RiDataPtr,
                                          uint32_t RiDataLen,
                                          uint32_t AddressPtr, uint32_t RiFlags,
                                          uint32_t /* Out */ PortPtr,
                                          uint32_t /* Out */ RoDataLenPtr,
                                          uint32_t /* Out */ RoFlagsPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  WASI::WasiAddrStorage *Storage = nullptr;
  switch (Address.size()) {
  case 4:
  case 16:
    break;
  case 128:
    Storage = reinterpret_cast<WASI::WasiAddrStorage *>(Address.data());
    Address = Storage->getAddress();
    break;
  default:
    return __WASI_ERRNO_INVAL;
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

  // Check for invalid address.
  uint16_t *const RoPort = MemInst->getPointer<uint16_t *>(PortPtr);
  if (RoPort == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  const auto RiDataArray =
      MemInst->getSpan<__wasi_iovec_t>(RiDataPtr, WasiRiDataLen);
  if (unlikely(RiDataArray.size() != WasiRiDataLen)) {
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
  StaticVector<Span<uint8_t>, WASI::kIOVMax> WasiRiData;

  for (auto &RiData : RiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(RiData.buf_len > Space) ? Space : RiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto RiDataArr = MemInst->getSpan<uint8_t>(RiData.buf, BufLen);
    if (unlikely(RiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiRiData.emplace_back_unchecked(RiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockRecvFrom(WasiFd, WasiRiData, WasiRiFlags, &WasiAddressFamily,
                           Address, RoPort, *RoDataLen, *RoFlags);
      unlikely(!Res)) {
    return Res.error();
  }
  if (Storage) {
    Storage->setAddressFamily(WasiAddressFamily);
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSendV2::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t SiDataPtr,
                                      uint32_t SiDataLen, uint32_t SiFlags,
                                      uint32_t /* Out */ SoDataLenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
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

  // Check for invalid address.
  const auto SiDataArray =
      MemInst->getSpan<__wasi_ciovec_t>(SiDataPtr, WasiSiDataLen);
  if (unlikely(SiDataArray.size() != WasiSiDataLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const SoDataLen = MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiSiData;

  for (auto &SiData : SiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(SiData.buf_len > Space) ? Space : SiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto SiDataArr = MemInst->getSpan<uint8_t>(SiData.buf, BufLen);
    if (unlikely(SiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiSiData.emplace_back_unchecked(SiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res = Env.sockSend(WasiFd, WasiSiData, WasiSiFlags, *SoDataLen);
      unlikely(!Res)) {
    return Res.error();
  }
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockSendToV2::body(const Runtime::CallingFrame &Frame,
                                        int32_t Fd, uint32_t SiDataPtr,
                                        uint32_t SiDataLen, uint32_t AddressPtr,
                                        int32_t Port, uint32_t SiFlags,
                                        uint32_t /* Out */ SoDataLenPtr) {
  // Check memory instance from module.
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<const uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_address_family_t WasiAddressFamily;
  switch (Address.size()) {
  case 4:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET4;
    break;
  case 16:
    WasiAddressFamily = __WASI_ADDRESS_FAMILY_INET6;
    break;
  case 128: {
    auto &Storage =
        *reinterpret_cast<const WASI::WasiAddrStorage *>(Address.data());
    WasiAddressFamily = Storage.getAddressFamily();
    Address = Storage.getAddress();
    break;
  }
  default:
    return __WASI_ERRNO_INVAL;
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

  // Check for invalid address.
  const auto SiDataArray =
      MemInst->getSpan<__wasi_ciovec_t>(SiDataPtr, WasiSiDataLen);
  if (unlikely(SiDataArray.size() != WasiSiDataLen)) {
    return __WASI_ERRNO_FAULT;
  }

  auto *const SoDataLen = MemInst->getPointer<__wasi_size_t *>(SoDataLenPtr);
  if (unlikely(SoDataLen == nullptr)) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_size_t TotalSize = 0;
  StaticVector<Span<const uint8_t>, WASI::kIOVMax> WasiSiData;

  for (auto &SiData : SiDataArray) {
    // Capping total size.
    const __wasi_size_t Space =
        std::numeric_limits<__wasi_size_t>::max() - TotalSize;
    const __wasi_size_t BufLen =
        unlikely(SiData.buf_len > Space) ? Space : SiData.buf_len;
    TotalSize += BufLen;

    // Check for invalid address.
    const auto SiDataArr = MemInst->getSpan<uint8_t>(SiData.buf, BufLen);
    if (unlikely(SiDataArr.size() != BufLen)) {
      return __WASI_ERRNO_FAULT;
    }
    WasiSiData.emplace_back_unchecked(SiDataArr);
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockSendTo(WasiFd, WasiSiData, WasiSiFlags, WasiAddressFamily,
                         Address, static_cast<uint16_t>(Port), *SoDataLen);
      unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockGetOpt::body(const Runtime::CallingFrame &Frame,
                                      int32_t Fd, uint32_t SockOptLevel,
                                      uint32_t SockOptName, uint32_t FlagPtr,
                                      uint32_t FlagSizePtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  __wasi_sock_opt_level_t WasiSockOptLevel;
  if (auto Res = cast<__wasi_sock_opt_level_t>(SockOptLevel); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSockOptLevel = *Res;
  }

  __wasi_sock_opt_so_t WasiSockOptName;
  if (auto Res = cast<__wasi_sock_opt_so_t>(SockOptName); unlikely(!Res)) {
    return Res.error();
  } else {
    WasiSockOptName = *Res;
  }

  auto *const SysFlagSizePtr = MemInst->getPointer<uint32_t *>(FlagSizePtr);
  if (SysFlagSizePtr == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Flag = MemInst->getSpan<uint8_t>(FlagPtr, *SysFlagSizePtr);
  if (Flag.size() != *SysFlagSizePtr) {
    return __WASI_ERRNO_FAULT;
  }

  const __wasi_fd_t WasiFd = Fd;

  if (auto Res =
          Env.sockGetOpt(WasiFd, WasiSockOptLevel, WasiSockOptName, Flag);
      unlikely(!Res)) {
    return Res.error();
  }

  *SysFlagSizePtr = static_cast<uint32_t>(Flag.size());
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiSockGetLocalAddrV2::body(const Runtime::CallingFrame &Frame, int32_t Fd,
                             uint32_t AddressPtr, uint32_t PortPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t *const RoPort = MemInst->getPointer<uint32_t *>(PortPtr);
  if (RoPort == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  if (Address.size() != 128) {
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;
  WASI::WasiAddrStorage &Storage =
      *reinterpret_cast<WASI::WasiAddrStorage *>(Address.data());
  Address = Storage.getAddress();
  __wasi_address_family_t WasiAddressFamily;
  uint16_t Port;

  if (auto Res =
          Env.sockGetLocalAddr(WasiFd, &WasiAddressFamily, Address, &Port);
      unlikely(!Res)) {
    return Res.error();
  }

  Storage.setAddressFamily(WasiAddressFamily);
  *RoPort = Port;
  return __WASI_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiSockGetPeerAddrV2::body(const Runtime::CallingFrame &Frame,
                                             int32_t Fd, uint32_t AddressPtr,
                                             uint32_t PortPtr) {
  auto *MemInst = Frame.getMemoryByIndex(0);
  if (MemInst == nullptr) {
    return __WASI_ERRNO_FAULT;
  }
  __wasi_address_t *InnerAddress =
      MemInst->getPointer<__wasi_address_t *>(AddressPtr);
  if (InnerAddress == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  auto Address =
      MemInst->getSpan<uint8_t>(InnerAddress->buf, InnerAddress->buf_len);
  if (Address.size() != InnerAddress->buf_len) {
    return __WASI_ERRNO_FAULT;
  }

  uint32_t *const RoPort = MemInst->getPointer<uint32_t *>(PortPtr);
  if (RoPort == nullptr) {
    return __WASI_ERRNO_FAULT;
  }

  if (Address.size() != 128) {
    return __WASI_ERRNO_INVAL;
  }

  const __wasi_fd_t WasiFd = Fd;
  WASI::WasiAddrStorage &Storage =
      *reinterpret_cast<WASI::WasiAddrStorage *>(Address.data());
  Address = Storage.getAddress();
  __wasi_address_family_t WasiAddressFamily;
  uint16_t Port;

  if (auto Res =
          Env.sockGetPeerAddr(WasiFd, &WasiAddressFamily, Address, &Port);
      unlikely(!Res)) {
    return Res.error();
  }
  Storage.setAddressFamily(WasiAddressFamily);
  *RoPort = Port;
  return __WASI_ERRNO_SUCCESS;
}
} // namespace Host
} // namespace WasmEdge
