// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/expected.h"
#include "common/span.h"
#include "runtime/instance/memory.h"
#include "wasi/api.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <utility>

namespace WasmEdge {
namespace Host {

/// Type aliasing for Expected<T, __wasi_errno_t>.
template <typename T> using WasiExpect = Expected<T, __wasi_errno_t>;

/// Helper function for Unexpected<__wasi_errno_t>.
constexpr auto WasiUnexpect(const __wasi_errno_t &Val) {
  return Unexpected<__wasi_errno_t>(Val);
}
template <typename T> constexpr auto WasiUnexpect(const WasiExpect<T> &Val) {
  return Unexpected<__wasi_errno_t>(Val.error());
}

/// Getter callback function
template <typename... Ts> using GetterR = WasiExpect<std::tuple<Ts...>>;
template <typename... Ts> using GetterT = std::function<GetterR<Ts...>()>;

template <typename ImplT> class WasiFileBase {
private:
  WasiFileBase(const WasiFileBase &) = delete;
  WasiFileBase &operator=(const WasiFileBase &) = delete;

protected:
  WasiFileBase(__wasi_rights_t R, __wasi_rights_t IR, std::string PN) noexcept
      : Rights(R), InheritingRights(IR), PreopenName(std::move(PN)) {}

public:
  static inline constexpr const __wasi_rights_t kReadRights =
      __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_FILESTAT_GET |
      __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_FD_SEEK |
      __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_PATH_FILESTAT_GET |
      __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_OPEN |
      __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_RENAME_SOURCE |
      __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_SOCK_SHUTDOWN;
  static inline constexpr const __wasi_rights_t kWriteRights =
      __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_ALLOCATE |
      __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
      __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
      __WASI_RIGHTS_FD_SYNC | __WASI_RIGHTS_FD_WRITE |
      __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
      __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_PATH_OPEN |
      __WASI_RIGHTS_PATH_REMOVE_DIRECTORY | __WASI_RIGHTS_PATH_RENAME_TARGET |
      __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE |
      __WASI_RIGHTS_SOCK_SHUTDOWN;
  static inline constexpr const __wasi_rights_t kCreateRights =
      __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
      __WASI_RIGHTS_PATH_LINK_TARGET | __WASI_RIGHTS_PATH_OPEN |
      __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_SYMLINK;
  static inline constexpr const __wasi_rights_t kStdInDefaultRights =
      __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_FILESTAT_GET |
      __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_POLL_FD_READWRITE;
  static inline constexpr const __wasi_rights_t kStdOutDefaultRights =
      __WASI_RIGHTS_FD_ADVISE | __WASI_RIGHTS_FD_DATASYNC |
      __WASI_RIGHTS_FD_FILESTAT_GET | __WASI_RIGHTS_FD_SYNC |
      __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_POLL_FD_READWRITE;
  static inline constexpr const __wasi_rights_t kStdErrDefaultRights =
      kStdOutDefaultRights;
  static inline constexpr const __wasi_rights_t kNoInheritingRights =
      static_cast<__wasi_rights_t>(0);

  struct Subscription {
    __wasi_userdata_t Userdata;
    __wasi_eventtype_t Tag;
    union {
      __wasi_subscription_clock_t Clock;
      ImplT *FdRead;
      ImplT *FdWrite;
    };
  };

  WasiFileBase(WasiFileBase &&) = default;
  WasiFileBase &operator=(WasiFileBase &&) = default;
  friend void swap(WasiFileBase &LHS, WasiFileBase &RHS) noexcept {
    using std::swap;
    swap(LHS.Rights, RHS.Rights);
    swap(LHS.InheritingRights, RHS.InheritingRights);
    swap(LHS.PreopenName, RHS.PreopenName);
  }

  bool checkRights(__wasi_rights_t RequiredRights,
                   __wasi_rights_t RequiredInheritingRights =
                       static_cast<__wasi_rights_t>(0)) const noexcept {
    return (Rights & RequiredRights) == RequiredRights &&
           (InheritingRights & RequiredInheritingRights) ==
               RequiredInheritingRights;
  }
  bool isPreopened() const noexcept { return !PreopenName.empty(); }
  auto rights() const noexcept { return Rights; }
  auto inheritingRights() const noexcept { return InheritingRights; }
  auto preopenName() const noexcept { return PreopenName; }
  bool ok() const noexcept { return static_cast<const ImplT *>(this)->doOk(); }

  __wasi_errno_t fdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t Len,
                          __wasi_advice_t Advice) noexcept;

  __wasi_errno_t fdAllocate(__wasi_filesize_t Offset,
                            __wasi_filesize_t Len) noexcept;

  __wasi_errno_t fdClose() noexcept;

  __wasi_errno_t fdDatasync() noexcept;

  __wasi_errno_t fdFdstatGet(GetterT<__wasi_fdstat_t *> Getter) noexcept;

  __wasi_errno_t fdFdstatSetFlags(__wasi_fdflags_t FdFlags) noexcept;

  __wasi_errno_t fdFdstatSetRights(__wasi_rights_t FsRightsBase,
                                   __wasi_rights_t FsRightsInheriting) noexcept;

  __wasi_errno_t fdFilestatGet(GetterT<__wasi_filestat_t *> Getter) noexcept;

  __wasi_errno_t fdFilestatSetSize(__wasi_filesize_t Size) noexcept;

  __wasi_errno_t fdFilestatSetTimes(__wasi_timestamp_t ATim,
                                    __wasi_timestamp_t MTim,
                                    __wasi_fstflags_t FstFlags) noexcept;

  __wasi_errno_t fdPread(GetterT<Span<Span<uint8_t>>, __wasi_size_t *> Getter,
                         __wasi_filesize_t Offset) noexcept;

  __wasi_errno_t fdPrestatGet(GetterT<__wasi_prestat_t *> Getter) noexcept;

  __wasi_errno_t fdPrestatDirName(GetterT<Span<uint8_t>> Getter) noexcept;

  __wasi_errno_t
  fdPwrite(GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter,
           __wasi_filesize_t Offset) noexcept;

  __wasi_errno_t
  fdRead(GetterT<Span<Span<uint8_t>>, __wasi_size_t *> Getter) noexcept;

  __wasi_errno_t fdReaddir(GetterT<Span<uint8_t>, __wasi_size_t *> Getter,
                           __wasi_dircookie_t Cookie) noexcept;

  __wasi_errno_t fdSeek(GetterT<__wasi_filedelta_t *> Getter,
                        __wasi_filedelta_t Offset,
                        __wasi_whence_t Whence) noexcept;

  __wasi_errno_t fdSync() noexcept;

  __wasi_errno_t fdTell(GetterT<__wasi_filesize_t *> Getter) noexcept;

  __wasi_errno_t
  fdWrite(GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter) noexcept;

  __wasi_errno_t
  pathCreateDirectory(GetterT<Span<const uint8_t>> Getter) noexcept;

  __wasi_errno_t
  pathFilestatGet(GetterT<Span<const uint8_t>, __wasi_filestat_t *> Getter,
                  __wasi_lookupflags_t Flags) noexcept;

  __wasi_errno_t pathFilestatSetTimes(GetterT<Span<const uint8_t>> Getter,
                                      __wasi_lookupflags_t Flags,
                                      __wasi_timestamp_t ATim,
                                      __wasi_timestamp_t MTim,
                                      __wasi_fstflags_t FstFlags) noexcept;

  static __wasi_errno_t
  pathLink(GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter, ImplT &Old,
           __wasi_lookupflags_t OldFlags, ImplT &New) noexcept;

  WasiExpect<ImplT> pathOpen(GetterT<Span<const uint8_t>> Getter,
                             __wasi_lookupflags_t DirFlags,
                             __wasi_oflags_t OFlags,
                             __wasi_rights_t FsRightsBase,
                             __wasi_rights_t FsRightsInheriting,
                             __wasi_fdflags_t FdFlags) noexcept;

  __wasi_errno_t
  pathReadlink(GetterT<Span<const uint8_t>, Span<uint8_t>, __wasi_size_t *>
                   Getter) noexcept;

  __wasi_errno_t
  pathRemoveDirectory(GetterT<Span<const uint8_t>> Getter) noexcept;

  static __wasi_errno_t
  pathRename(GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter,
             ImplT &Old, ImplT &New) noexcept;

  __wasi_errno_t pathSymlink(
      GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter) noexcept;

  __wasi_errno_t pathUnlinkFile(GetterT<Span<const uint8_t>> Getter) noexcept;

  static __wasi_errno_t
  pollOneoff(GetterT<Span<const __wasi_subscription_t>, Span<__wasi_event_t>,
                     __wasi_size_t *>
                 Getter,
             std::function<GetterR<Span<const Subscription>>(
                 Span<const __wasi_subscription_t>)>
                 SubscriptionTranslator) noexcept;

  __wasi_errno_t sockRecv(
      GetterT<Span<Span<uint8_t>>, __wasi_size_t *, __wasi_roflags_t *> Getter,
      __wasi_riflags_t RiFlags) noexcept;

  __wasi_errno_t
  sockSend(GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter,
           __wasi_siflags_t SiFlags) noexcept;

  __wasi_errno_t sockShutdown(__wasi_sdflags_t How) noexcept;

private:
  __wasi_rights_t Rights;
  __wasi_rights_t InheritingRights;
  std::string PreopenName;

  template <typename T> void cappingIOVS(T &IOVS) noexcept {
    /// Check for total size overflow.
    const size_t TotalSizeLimit =
        std::numeric_limits<int32_t>::max() &
        ~(Runtime::Instance::MemoryInstance::kPageSize - 1);
    size_t TotalSize = 0;
    for (auto &IOV : IOVS) {
      const size_t Overflow = TotalSizeLimit - TotalSize;
      if (unlikely(IOV.size() > Overflow)) {
        /// capping IOV.
        IOV = IOV.subspan(0, Overflow);
      }
      TotalSize += IOV.size();
    }
  }

  template <typename T> static bool isValidEnum(T Enum, T Min, T Max) noexcept {
    return likely(Min <= Enum && Enum <= Max);
  }
  static bool isValidEnum(__wasi_whence_t Enum) noexcept {
    return isValidEnum(Enum, __WASI_WHENCE_SET, __WASI_WHENCE_END);
  }
  static bool isValidEnum(__wasi_eventtype_t Enum) noexcept {
    return isValidEnum(Enum, __WASI_EVENTTYPE_CLOCK, __WASI_EVENTTYPE_FD_WRITE);
  }
  static bool isValidEnum(__wasi_clockid_t Enum) noexcept {
    return isValidEnum(Enum, __WASI_CLOCKID_REALTIME,
                       __WASI_CLOCKID_THREAD_CPUTIME_ID);
  }

  template <typename T> static bool isValidFlags(T AllFlags, T Flags) noexcept {
    return likely((Flags | AllFlags) == AllFlags);
  }
  template <typename T>
  static bool isExclusiveValidFlags(T Flag1, T Flag2, T Flags) noexcept {
    return likely((Flags & (Flag1 | Flag2)) != (Flag1 | Flag2));
  }
  static bool isValidFlags(__wasi_lookupflags_t Flags) noexcept {
    return isValidFlags(__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW, Flags);
  }
  static bool isValidFlags(__wasi_fdflags_t Flags) noexcept {
    return isValidFlags(__WASI_FDFLAGS_NONBLOCK | __WASI_FDFLAGS_APPEND |
                            __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC |
                            __WASI_FDFLAGS_SYNC,
                        Flags);
  }
  static bool isValidFlags(__wasi_oflags_t Flags) noexcept {
    return isValidFlags(__WASI_OFLAGS_CREAT | __WASI_OFLAGS_DIRECTORY |
                            __WASI_OFLAGS_EXCL | __WASI_OFLAGS_TRUNC,
                        Flags);
  }
  static bool isValidFlags(__wasi_subclockflags_t Flags) noexcept {
    return isValidFlags(__WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME, Flags);
  }
  static bool isValidFlags(__wasi_rights_t Flags) noexcept {
    return isValidFlags(
        __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ |
            __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS |
            __WASI_RIGHTS_FD_SYNC | __WASI_RIGHTS_FD_TELL |
            __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_ADVISE |
            __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_PATH_CREATE_DIRECTORY |
            __WASI_RIGHTS_PATH_CREATE_FILE | __WASI_RIGHTS_PATH_LINK_SOURCE |
            __WASI_RIGHTS_PATH_LINK_TARGET | __WASI_RIGHTS_PATH_OPEN |
            __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_PATH_READLINK |
            __WASI_RIGHTS_PATH_RENAME_SOURCE |
            __WASI_RIGHTS_PATH_RENAME_TARGET | __WASI_RIGHTS_PATH_FILESTAT_GET |
            __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
            __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES |
            __WASI_RIGHTS_FD_FILESTAT_GET | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE |
            __WASI_RIGHTS_FD_FILESTAT_SET_TIMES | __WASI_RIGHTS_PATH_SYMLINK |
            __WASI_RIGHTS_PATH_REMOVE_DIRECTORY |
            __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE |
            __WASI_RIGHTS_SOCK_SHUTDOWN,
        Flags);
  }
  static bool isValidFlags(__wasi_riflags_t Flags) noexcept {
    return isValidFlags(__WASI_RIFLAGS_RECV_PEEK | __WASI_RIFLAGS_RECV_WAITALL,
                        Flags);
  }
  static bool isValidFlags(__wasi_siflags_t Flags) noexcept {
    return isValidFlags(static_cast<__wasi_siflags_t>(0), Flags);
  }
  static bool isValidFlags(__wasi_sdflags_t Flags) noexcept {
    return isValidFlags(__WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR, Flags);
  }
  static bool isValidFlags(__wasi_fstflags_t Flags) noexcept {
    return isValidFlags(__WASI_FSTFLAGS_ATIM | __WASI_FSTFLAGS_ATIM_NOW |
                            __WASI_FSTFLAGS_MTIM | __WASI_FSTFLAGS_MTIM_NOW,
                        Flags) &&
           isExclusiveValidFlags(__WASI_FSTFLAGS_ATIM, __WASI_FSTFLAGS_ATIM_NOW,
                                 Flags) &&
           isExclusiveValidFlags(__WASI_FSTFLAGS_MTIM, __WASI_FSTFLAGS_MTIM_NOW,
                                 Flags);
  }
};

class WasiFile;

} // namespace Host
} // namespace WasmEdge
