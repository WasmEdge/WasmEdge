// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "host/wasi/clock.h"
#include "host/wasi/error.h"
#include "host/wasi/vfs.h"
#include "host/wasi/vinode.h"
#include "wasi/api.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASI {

inline namespace detail {
inline constexpr const int32_t kIOVMax = 1024;
// Large enough to store SaData in sockaddr_in6
// = sizeof(sockaddr_in6) - sizeof(sockaddr_in6::sin6_family)
inline constexpr const int32_t kMaxSaDataLen = 26;
} // namespace detail

struct WasiAddrStorage {
  uint16_t AddressFamily;
  uint8_t Address[128 - sizeof(uint16_t)];
  __wasi_address_family_t getAddressFamily() const noexcept {
    return static_cast<__wasi_address_family_t>(AddressFamily);
  }
  void setAddressFamily(__wasi_address_family_t AddrFamily) noexcept {
    AddressFamily = static_cast<uint16_t>(AddrFamily);
  }
  Span<uint8_t> getAddress() noexcept { return Address; }
  Span<const uint8_t> getAddress() const noexcept { return Address; }
};
static_assert(sizeof(WasiAddrStorage) == 128, "wrong size");

class EVPoller;
class Environ : public PollerContext {
public:
  ~Environ() noexcept;

  void init(Span<const std::string> Dirs, std::string ProgramName,
            Span<const std::string> Args, Span<const std::string> Envs);

  void fini() noexcept;

  WasiExpect<void> getAddrInfo(std::string_view Node, std::string_view Service,
                               const __wasi_addrinfo_t &Hint,
                               uint32_t MaxResLength,
                               Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                               Span<__wasi_sockaddr_t *> WasiSockaddrArray,
                               Span<char *> AiAddrSaDataArray,
                               Span<char *> AiCanonnameArray,
                               /*Out*/ __wasi_size_t &ResLength) {

    if (auto Res = VINode::getAddrinfo(
            Node, Service, Hint, MaxResLength, WasiAddrinfoArray,
            WasiSockaddrArray, AiAddrSaDataArray, AiCanonnameArray, ResLength);
        unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
    return {};
  }

  constexpr const std::vector<std::string> &getArguments() const noexcept {
    return Arguments;
  }

  constexpr const std::vector<std::string> &
  getEnvironVariables() const noexcept {
    return EnvironVariables;
  }

  constexpr __wasi_exitcode_t getExitCode() const noexcept { return ExitCode; }

  /// Read command-line argument data.
  ///
  /// The size of the array should match that returned by `args_sizes_get`.
  ///
  /// Each argument is expected to be `\0` terminated.
  ///
  /// @param[out] Argv Return the pointers to arguments.
  /// @param[out] ArgvBuffer Return the argument string data.
  /// @return Nothing or WASI error
  WasiExpect<void> argsGet(Span<uint8_t_ptr> Argv,
                           Span<uint8_t> ArgvBuffer) const noexcept {
    for (const auto &Argument : Arguments) {
      const __wasi_size_t Size = static_cast<__wasi_size_t>(Argument.size());
      std::copy_n(Argument.begin(), Size, ArgvBuffer.begin());
      ArgvBuffer[Size] = '\0';
      ArgvBuffer = ArgvBuffer.subspan(Size + UINT32_C(1));
      if (Argv.size() > 1) {
        Argv[1] = Argv[0] + Size + UINT32_C(1);
      }
      Argv = Argv.subspan(1);
    }
    assert(ArgvBuffer.empty());
    assert(Argv.empty());

    return {};
  }

  /// Return command-line argument data sizes.
  ///
  /// @param[out] Argc Return the number of arguments
  /// @param[out] ArgvSize Return the size of the argument string data
  /// @return Nothing or WASI error
  WasiExpect<void> argsSizesGet(__wasi_size_t &Argc,
                                __wasi_size_t &ArgvSize) const noexcept {
    Argc = static_cast<__wasi_size_t>(Arguments.size());
    ArgvSize = 0;
    for (const auto &Argument : Arguments) {
      ArgvSize += static_cast<__wasi_size_t>(Argument.size()) + UINT32_C(1);
    }

    return {};
  }

  /// Read environment variable data.
  ///
  /// The sizes of the buffers should match that returned by
  /// `environ_sizes_get`.
  ///
  /// Key/value pairs are expected to be joined with `=`s, and terminated with
  /// `\0`s.
  ///
  /// @param[out] Env Return the pointers to environment variables.
  /// @param[out] EnvBuffer Return the environment variable data.
  /// @return Nothing or WASI error
  WasiExpect<void> environGet(Span<uint8_t_ptr> Env,
                              Span<uint8_t> EnvBuffer) const noexcept {
    for (const auto &EnvironVariable : EnvironVariables) {
      const __wasi_size_t Size =
          static_cast<__wasi_size_t>(EnvironVariable.size());
      std::copy_n(EnvironVariable.begin(), Size, EnvBuffer.begin());
      EnvBuffer[Size] = '\0';
      EnvBuffer = EnvBuffer.subspan(Size + UINT32_C(1));
      if (Env.size() > 1) {
        Env[1] = Env[0] + Size + UINT32_C(1);
      }
      Env = Env.subspan(1);
    }
    assert(EnvBuffer.empty());
    assert(Env.empty());

    return {};
  }

  /// Return environment variable data sizes.
  ///
  /// @param[out] Envc Returns the number of environment variable arguments
  /// @param[out] EnvSize Return the size of the environment variable data.
  /// @return Nothing or WASI error
  WasiExpect<void> environSizesGet(__wasi_size_t &Envc,
                                   __wasi_size_t &EnvSize) const noexcept {
    Envc = static_cast<__wasi_size_t>(EnvironVariables.size());
    EnvSize = 0;
    for (const auto &EnvironVariable : EnvironVariables) {
      EnvSize +=
          static_cast<__wasi_size_t>(EnvironVariable.size()) + UINT32_C(1);
    }

    return {};
  }

  /// Return the resolution of a clock.
  ///
  /// Implementations are required to provide a non-zero value for supported
  /// clocks. For unsupported clocks, return `errno::inval`.
  ///
  /// @param[in] Id The clock for which to return the resolution.
  /// @param[out] Resolution The resolution of the clock.
  /// @return Nothing or WASI error
  static WasiExpect<void> clockResGet(__wasi_clockid_t Id,
                                      __wasi_timestamp_t &Resolution) noexcept {
    return Clock::clockResGet(Id, Resolution);
  }

  /// Return the time value of a clock.
  ///
  /// Note: This is similar to `clock_gettime` in POSIX.
  ///
  /// @param[in] Id The clock for which to return the time.
  /// @param[in] Precision The maximum lag (exclusive) that the returned time
  /// value may have, compared to its actual value.
  /// @param[out] Time The time value of the clock.
  /// @return Nothing or WASI error
  static WasiExpect<void> clockTimeGet(__wasi_clockid_t Id,
                                       __wasi_timestamp_t Precision,
                                       __wasi_timestamp_t &Time) noexcept {
    return Clock::clockTimeGet(Id, Precision, Time);
  }

  /// Provide file advisory information on a file descriptor.
  ///
  /// Note: This is similar to `posix_fadvise` in POSIX.
  ///
  /// @param[in] Fd The file descriptor.
  /// @param[in] Offset The offset within the file to which the advisory
  /// applies.
  /// @param[in] Len The length of the region to which the advisory applies.
  /// @param[in] Advice The advice.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAdvise(__wasi_fd_t Fd, __wasi_filesize_t Offset,
                            __wasi_filesize_t Len,
                            __wasi_advice_t Advice) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdAdvise(Offset, Len, Advice);
    }
  }

  /// Force the allocation of space in a file.
  ///
  /// Note: This is similar to `posix_fallocate` in POSIX.
  ///
  /// @param[in] Offset The offset at which to start the allocation.
  /// @param[in] Len The length of the area that is allocated.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAllocate(__wasi_fd_t Fd, __wasi_filesize_t Offset,
                              __wasi_filesize_t Len) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdAllocate(Offset, Len);
    }
  }

  /// Close a file descriptor.
  ///
  /// Note: This is similar to `close` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdClose(__wasi_fd_t Fd) noexcept {
    std::unique_lock Lock(FdMutex);
    if (auto It = FdMap.find(Fd); It == FdMap.end()) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      close(It->second);
      FdMap.erase(It);
      return {};
    }
  }

  /// Synchronize the data of a file to disk.
  ///
  /// Note: This is similar to `fdatasync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdDatasync(__wasi_fd_t Fd) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdDatasync();
    }
  }

  /// Get the attributes of a file descriptor.
  ///
  /// Note: This returns similar flags to `fsync(fd, F_GETFL)` in POSIX, as well
  ///
  /// as additional fields.
  /// @param[out] FdStat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatGet(__wasi_fd_t Fd,
                               __wasi_fdstat_t &FdStat) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFdstatGet(FdStat);
    }
  }

  /// Adjust the flags associated with a file descriptor.
  ///
  /// Note: This is similar to `fcntl(fd, F_SETFL, flags)` in POSIX.
  ///
  /// @param[in] FdFlags The desired values of the file descriptor flags.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatSetFlags(__wasi_fd_t Fd,
                                    __wasi_fdflags_t FdFlags) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFdstatSetFlags(FdFlags);
    }
  }

  /// Adjust the rights associated with a file descriptor.
  ///
  /// This can only be used to remove rights, and returns `errno::notcapable` if
  /// called in a way that would attempt to add rights
  ///
  /// @param[in] FsRightsBase The desired rights of the file descriptor.
  /// @param[in] FsRightsInheriting The desired rights of the file descriptor.
  /// @return Nothing or WASI error
  WasiExpect<void>
  fdFdstatSetRights(__wasi_fd_t Fd, __wasi_rights_t FsRightsBase,
                    __wasi_rights_t FsRightsInheriting) noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFdstatSetRights(FsRightsBase, FsRightsInheriting);
    }

    return {};
  }

  /// Return the attributes of an open file.
  ///
  /// @param[out] Filestat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatGet(__wasi_fd_t Fd,
                                 __wasi_filestat_t &Filestat) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFilestatGet(Filestat);
    }
  }

  /// Adjust the size of an open file. If this increases the file's size, the
  /// extra bytes are filled with zeros.
  ///
  /// Note: This is similar to `ftruncate` in POSIX.
  ///
  /// @param[in] Size The desired file size.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatSetSize(__wasi_fd_t Fd,
                                     __wasi_filesize_t Size) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFilestatSetSize(Size);
    }
  }

  /// Adjust the timestamps of an open file or directory.
  ///
  /// Note: This is similar to `futimens` in POSIX.
  ///
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void>
  fdFilestatSetTimes(__wasi_fd_t Fd, __wasi_timestamp_t ATim,
                     __wasi_timestamp_t MTim,
                     __wasi_fstflags_t FstFlags) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdFilestatSetTimes(ATim, MTim, FstFlags);
    }
  }

  /// Read from a file descriptor, without using and updating the file
  /// descriptor's offset.
  ///
  /// Note: This is similar to `preadv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors in which to store data.
  /// @param[in] Offset The offset within the file at which to read.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPread(__wasi_fd_t Fd, Span<Span<uint8_t>> IOVs,
                           __wasi_filesize_t Offset,
                           __wasi_size_t &NRead) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdPread(IOVs, Offset, NRead);
    }
  }

  /// Return a description of the given preopened file descriptor.
  ///
  /// @param[out] PreStat The buffer where the description is stored.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPrestatGet(__wasi_fd_t Fd,
                                __wasi_prestat_t &PreStat) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      if (const auto &Path = Node->name(); Path.empty()) {
        return WasiUnexpect(__WASI_ERRNO_INVAL);
      } else {
        PreStat.tag = __WASI_PREOPENTYPE_DIR;
        PreStat.u.dir.pr_name_len = static_cast<__wasi_size_t>(Path.size());
      }
    }
    return {};
  }

  /// Return a description of the given preopened file descriptor.
  ///
  /// @param[out] Buffer A buffer into which to write the preopened directory
  /// name.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPrestatDirName(__wasi_fd_t Fd,
                                    Span<uint8_t> Buffer) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      if (const auto &Path = Node->name(); Path.empty()) {
        return WasiUnexpect(__WASI_ERRNO_INVAL);
      } else if (Buffer.size() < Path.size()) {
        return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
      } else {
        std::copy_n(Path.begin(), Path.size(), Buffer.begin());
      }
    }
    return {};
  }

  /// Write to a file descriptor, without using and updating the file
  /// descriptor's offset.
  ///
  /// Note: This is similar to `pwritev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[in] Offset The offset within the file at which to write.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPwrite(__wasi_fd_t Fd, Span<Span<const uint8_t>> IOVs,
                            __wasi_filesize_t Offset,
                            __wasi_size_t &NWritten) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdPwrite(IOVs, Offset, NWritten);
    }
  }

  /// Read from a file descriptor.
  ///
  /// Note: This is similar to `readv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors to which to store data.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdRead(__wasi_fd_t Fd, Span<Span<uint8_t>> IOVs,
                          __wasi_size_t &NRead) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdRead(IOVs, NRead);
    }
  }

  /// Read directory entries from a directory.
  ///
  /// When successful, the contents of the output buffer consist of a sequence
  /// of directory entries. Each directory entry consists of a `dirent` object,
  /// followed by `dirent::d_namlen` bytes holding the name of the directory
  /// entry.
  ///
  /// This function fills the output buffer as much as possible,
  /// potentially truncating the last directory entry. This allows the caller to
  /// grow its read buffer size in case it's too small to fit a single large
  /// directory entry, or skip the oversized directory entry.
  ///
  /// @param[out] Buffer The buffer where directory entries are stored.
  /// @param[in] Cookie The location within the directory to start reading
  /// @param[out] Size The number of bytes stored in the read buffer. If less
  /// than the size of the read buffer, the end of the directory has been
  /// reached.
  /// @return Nothing or WASI error
  WasiExpect<void> fdReaddir(__wasi_fd_t Fd, Span<uint8_t> Buffer,
                             __wasi_dircookie_t Cookie,
                             __wasi_size_t &Size) noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdReaddir(Buffer, Cookie, Size);
    }
  }

  /// Atomically replace a file descriptor by renumbering another file
  /// descriptor.
  ///
  /// Due to the strong focus on thread safety, this environment does not
  /// provide a mechanism to duplicate or renumber a file descriptor to an
  /// arbitrary number, like `dup2()`. This would be prone to race conditions,
  /// as an actual file descriptor with the same number could be allocated by a
  /// different thread at the same time.
  ///
  /// This function provides a way to atomically renumber file descriptors,
  /// which would disappear if `dup2()` were to be removed entirely.
  ///
  /// @param[in] To The file descriptor to overwrite.
  /// @return Nothing or WASI error
  WasiExpect<void> fdRenumber(__wasi_fd_t Fd, __wasi_fd_t To) noexcept {
    std::unique_lock Lock(FdMutex);
    if (auto It = FdMap.find(Fd); It == FdMap.end()) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else if (Fd == To) {
      return {};
    } else if (auto It2 = FdMap.find(To); It2 == FdMap.end()) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      FdMap.erase(It2);
      auto Node = FdMap.extract(It);
      Node.key() = To;
      FdMap.insert(std::move(Node));
      return {};
    }
  }

  /// Move the offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek` in POSIX.
  ///
  /// @param[in] Offset The number of bytes to move.
  /// @param[in] Whence The base from which the offset is relative.
  /// @param[out] Size The new offset of the file descriptor, relative to the
  /// start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdSeek(__wasi_fd_t Fd, __wasi_filedelta_t Offset,
                          __wasi_whence_t Whence,
                          __wasi_filesize_t &Size) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdSeek(Offset, Whence, Size);
    }
  }

  /// Synchronize the data and metadata of a file to disk.
  ///
  /// Note: This is similar to `fsync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdSync(__wasi_fd_t Fd) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdSync();
    }
  }

  /// Return the current offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek(fd, 0, SEEK_CUR)` in POSIX.
  ///
  /// @param[out] Size The current offset of the file descriptor, relative to
  /// the start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdTell(__wasi_fd_t Fd,
                          __wasi_filesize_t &Size) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdTell(Size);
    }
  }

  /// Write to a file descriptor.
  ///
  /// Note: This is similar to `writev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdWrite(__wasi_fd_t Fd, Span<Span<const uint8_t>> IOVs,
                           __wasi_size_t &NWritten) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->fdWrite(IOVs, NWritten);
    }
  }

  /// Create a directory.
  ///
  /// Note: This is similar to `mkdirat` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path at which to create the directory.
  /// @return Nothing or WASI error
  WasiExpect<void> pathCreateDirectory(__wasi_fd_t Fd, std::string_view Path) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathCreateDirectory(std::move(Node), Path);
  }

  /// Return the attributes of a file or directory.
  ///
  /// Note: This is similar to `stat` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] Flags Flags determining the method of how the path is resolved.
  /// @param[out] Filestat The buffer where the file's attributes are stored.
  /// @return Nothing or WASI error
  WasiExpect<void> pathFilestatGet(__wasi_fd_t Fd, std::string_view Path,
                                   __wasi_lookupflags_t Flags,
                                   __wasi_filestat_t &Filestat) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathFilestatGet(std::move(Node), Path, Flags, Filestat);
  }

  /// Adjust the timestamps of a file or directory.
  ///
  /// Note: This is similar to `utimensat` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] Flags Flags determining the method of how the path is resolved.
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void> pathFilestatSetTimes(__wasi_fd_t Fd, std::string_view Path,
                                        __wasi_lookupflags_t Flags,
                                        __wasi_timestamp_t ATim,
                                        __wasi_timestamp_t MTim,
                                        __wasi_fstflags_t FstFlags) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathFilestatSetTimes(std::move(Node), Path, Flags, ATim,
                                        MTim, FstFlags);
  }

  /// Create a hard link.
  ///
  /// Note: This is similar to `linkat` in POSIX.
  ///
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path from which to link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the hard link.
  /// @param[in] LookupFlags Flags determining the method of how the path is
  /// resolved.
  /// @return Nothing or WASI error
  WasiExpect<void> pathLink(__wasi_fd_t Old, std::string_view OldPath,
                            __wasi_fd_t New, std::string_view NewPath,
                            __wasi_lookupflags_t LookupFlags) {
    if (!VINode::isPathValid(OldPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    if (!VINode::isPathValid(NewPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto OldNode = getNodeOrNull(Old);
    auto NewNode = getNodeOrNull(New);
    return VINode::pathLink(std::move(OldNode), OldPath, std::move(NewNode),
                            NewPath, LookupFlags);
  }

  /// Open a file or directory.
  ///
  /// The returned file descriptor is not guaranteed to be the lowest-numbered
  /// file descriptor not currently open; it is randomized to prevent
  /// applications from depending on making assumptions about indexes, since
  /// this is error-prone in multi-threaded contexts. The returned file
  /// descriptor is guaranteed to be less than 2**31.
  ///
  /// Note: This is similar to `openat` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The relative path of the file or directory to open,
  /// relative to the `path_open::fd` directory.
  /// @param[in] LookupFlags Flags determining the method of how the path is
  /// resolved.
  /// @param[in] OpenFlags The method by which to open the file.
  /// @param[in] FsRightsBase The initial rights of the newly created file
  /// descriptor. The implementation is allowed to return a file descriptor with
  /// fewer rights than specified, if and only if those rights do not apply to
  /// the type of file being opened. The *base* rights are rights that will
  /// apply to operations using the file descriptor itself.
  /// @param[in] FsRightsInheriting The initial rights of the newly created file
  /// descriptor. The implementation is allowed to return a file descriptor with
  /// fewer rights than specified, if and only if those rights do not apply to
  /// the type of file being opened. The *inheriting* rights are rights that
  /// apply to file descriptors derived from it.
  /// @param[in] FdFlags The method by which to open the file.
  /// @return The file descriptor of the file that has been opened, or WASI
  /// error.
  WasiExpect<__wasi_fd_t> pathOpen(__wasi_fd_t Fd, std::string_view Path,
                                   __wasi_lookupflags_t LookupFlags,
                                   __wasi_oflags_t OpenFlags,
                                   __wasi_rights_t FsRightsBase,
                                   __wasi_rights_t FsRightsInheriting,
                                   __wasi_fdflags_t FdFlags) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    if (auto Res =
            VINode::pathOpen(std::move(Node), Path, LookupFlags, OpenFlags,
                             FsRightsBase, FsRightsInheriting, FdFlags);
        unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      Node = std::move(*Res);
    }

    return generateRandomFdToNode(Node);
  }

  /// Read the contents of a symbolic link.
  ///
  /// Note: This is similar to `readlinkat` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the symbolic link from which to read.
  /// @param[out] Buffer The buffer to which to write the contents of the
  /// symbolic link.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathReadlink(__wasi_fd_t Fd, std::string_view Path,
                                Span<char> Buffer, __wasi_size_t &NRead) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathReadlink(std::move(Node), Path, Buffer, NRead);
  }

  /// Remove a directory.
  ///
  /// Return `errno::notempty` if the directory is not empty.
  ///
  /// Note: This is similar to `unlinkat(fd, path, AT_REMOVEDIR)` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path to a directory to remove.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathRemoveDirectory(__wasi_fd_t Fd, std::string_view Path) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathRemoveDirectory(std::move(Node), Path);
  }

  /// Rename a file or directory.
  ///
  /// Note: This is similar to `renameat` in POSIX.
  ///
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path of the file or directory to rename.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path to which to rename the file or
  /// directory.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathRename(__wasi_fd_t Old, std::string_view OldPath,
                              __wasi_fd_t New, std::string_view NewPath) {
    if (!VINode::isPathValid(OldPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    if (!VINode::isPathValid(NewPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto OldNode = getNodeOrNull(Old);
    auto NewNode = getNodeOrNull(New);
    return VINode::pathRename(std::move(OldNode), OldPath, std::move(NewNode),
                              NewPath);
  }

  /// Create a symbolic link.
  ///
  /// Note: This is similar to `symlinkat` in POSIX.
  ///
  /// @param[in] OldPath The contents of the symbolic link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the symbolic
  /// link.
  /// @return Nothing or WASI error
  WasiExpect<void> pathSymlink(std::string_view OldPath, __wasi_fd_t New,
                               std::string_view NewPath) {
    if (!VINode::isPathValid(OldPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    if (!VINode::isPathValid(NewPath)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    // forbid absolute path
    if (!OldPath.empty() && OldPath[0] == '/') {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto NewNode = getNodeOrNull(New);
    return VINode::pathSymlink(OldPath, std::move(NewNode), NewPath);
  }

  /// Unlink a file.
  ///
  /// Return `errno::isdir` if the path refers to a directory.
  ///
  /// Note: This is similar to `unlinkat(fd, path, 0)` in POSIX.
  ///
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path to a file to unlink.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathUnlinkFile(__wasi_fd_t Fd, std::string_view Path) {
    if (!VINode::isPathValid(Path)) {
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    auto Node = getNodeOrNull(Fd);
    return VINode::pathUnlinkFile(std::move(Node), Path);
  }

  /// Acquire a Poller for concurrently poll for the occurrence of a set of
  /// events.
  ///
  /// @param[in] Events The output buffer for events.
  /// @return Poll helper or WASI error.
  WasiExpect<EVPoller> acquirePoller(Span<__wasi_event_t> Events) noexcept;

  /// Release a used Poller object.
  ///
  /// @param[in] Poller Used poller object.
  void releasePoller(EVPoller &&Poller) noexcept;

  /// Close unused Fd in Pollers.
  ///
  /// @param[in] Node The Node to be deleted.
  void close(std::shared_ptr<VINode> Node) noexcept;

  /// Terminate the process normally. An exit code of 0 indicates successful
  /// termination of the program. The meanings of other values is dependent on
  /// the environment.
  ///
  /// @param[in] Code The exit code returned by the process.
  void procExit(__wasi_exitcode_t Code) noexcept { ExitCode = Code; }

  /// Send a signal to the process of the calling thread.
  ///
  /// Note: This is similar to `raise` in POSIX.
  ///
  /// @param[in] Signal The signal condition to trigger.
  /// @return Nothing or WASI error
  WasiExpect<void> procRaise(__wasi_signal_t Signal) const noexcept;

  /// Temporarily yield execution of the calling thread.
  ///
  /// Note: This is similar to `sched_yield` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> schedYield() const noexcept;

  /// Write high-quality random data into a buffer.
  ///
  /// This function blocks when the implementation is unable to immediately
  /// provide sufficient high-quality random data.
  ///
  /// This function may execute slowly, so when large mounts of random data are
  /// required, it's advisable to use this function to seed a pseudo-random
  /// number generator, rather than to provide the random data directly.
  ///
  /// @param[out] Buffer The buffer to fill with random data.
  /// @return Nothing or WASI error
  WasiExpect<void> randomGet(Span<uint8_t> Buffer) const noexcept {
    std::random_device Device;
    std::default_random_engine Engine(Device());
    std::uniform_int_distribution<uint32_t> Distribution;
    auto BufferSpan = cxx20::as_writable_bytes(Buffer);
    while (!BufferSpan.empty()) {
      const uint32_t Value = Distribution(Engine);
      const auto ValueSpan =
          cxx20::as_bytes(cxx20::span<const uint32_t, 1>(&Value, 1));
      const auto Size = std::min(BufferSpan.size(), ValueSpan.size());
      std::copy_n(ValueSpan.begin(), Size, BufferSpan.begin());
      BufferSpan = BufferSpan.subspan(Size);
    }

    return {};
  }

  WasiExpect<__wasi_fd_t> sockOpen(__wasi_address_family_t AddressFamily,
                                   __wasi_sock_type_t SockType) noexcept {

    std::shared_ptr<VINode> Node;
    if (auto Res = VINode::sockOpen(AddressFamily, SockType); unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      Node = std::move(*Res);
    }

    return generateRandomFdToNode(Node);
  }

  WasiExpect<void> sockBind(__wasi_fd_t Fd,
                            __wasi_address_family_t AddressFamily,
                            Span<const uint8_t> Address,
                            uint16_t Port) noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockBind(AddressFamily, Address, Port);
    }
  }

  WasiExpect<void> sockListen(__wasi_fd_t Fd, int32_t Backlog) noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockListen(Backlog);
    }
  }

  WasiExpect<__wasi_fd_t> sockAccept(__wasi_fd_t Fd,
                                     __wasi_fdflags_t FdFlags) noexcept {
    auto Node = getNodeOrNull(Fd);
    std::shared_ptr<VINode> NewNode;

    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else if (auto Res = Node->sockAccept(FdFlags); unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      NewNode = std::move(*Res);
    }

    return generateRandomFdToNode(NewNode);
  }

  WasiExpect<void> sockConnect(__wasi_fd_t Fd,
                               __wasi_address_family_t AddressFamily,
                               Span<const uint8_t> Address,
                               uint16_t Port) noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockConnect(AddressFamily, Address, Port);
    }
  }

  /// Receive a message from a socket.
  ///
  /// Note: This is similar to `recv` in POSIX, though it also supports reading
  /// the data into multiple buffers in the manner of `readv`.
  ///
  /// @param[in] RiData List of scatter/gather vectors to which to store data.
  /// @param[in] RiFlags Message flags.
  /// @param[out] NRead Return the number of bytes stored in RiData.
  /// @param[out] RoFlags Return message flags.
  /// @return Nothing or WASI error.
  WasiExpect<void> sockRecv(__wasi_fd_t Fd, Span<Span<uint8_t>> RiData,
                            __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                            __wasi_roflags_t &RoFlags) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockRecv(RiData, RiFlags, NRead, RoFlags);
    }
  }

  /// Receive a message from a socket.
  ///
  /// Note: This is similar to `recvfrom` in POSIX, though it also supports
  /// reading the data into multiple buffers in the manner of `readv`.
  ///
  /// @param[in] RiData List of scatter/gather vectors to which to store data.
  /// @param[in] RiFlags Message flags.
  /// @param[out] AddressFamilyPtr The pointer to store address family.
  /// @param[out] Address The buffer to store address.
  /// @param[out] PortPtr The pointer to store port.
  /// @param[out] NRead Return the number of bytes stored in RiData.
  /// @param[out] RoFlags Return message flags.
  /// @return Nothing or WASI error.
  WasiExpect<void> sockRecvFrom(__wasi_fd_t Fd, Span<Span<uint8_t>> RiData,
                                __wasi_riflags_t RiFlags,
                                __wasi_address_family_t *AddressFamilyPtr,
                                Span<uint8_t> Address, uint16_t *PortPtr,
                                __wasi_size_t &NRead,
                                __wasi_roflags_t &RoFlags) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockRecvFrom(RiData, RiFlags, AddressFamilyPtr, Address,
                                PortPtr, NRead, RoFlags);
    }
  }

  /// Send a message on a socket.
  ///
  /// Note: This is similar to `send` in POSIX, though it also supports writing
  /// the data from multiple buffers in the manner of `writev`.
  ///
  /// @param[in] SiData List of scatter/gather vectors to which to retrieve
  /// data.
  /// @param[in] SiFlags Message flags.
  /// @param[out] NWritten The number of bytes transmitted.
  /// @return Nothing or WASI error
  WasiExpect<void> sockSend(__wasi_fd_t Fd, Span<Span<const uint8_t>> SiData,
                            __wasi_siflags_t SiFlags,
                            __wasi_size_t &NWritten) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockSend(SiData, SiFlags, NWritten);
    }
  }

  /// Send a message on a socket.
  ///
  /// Note: This is similar to `sendto` in POSIX, though it also supports
  /// writing the data from multiple buffers in the manner of `writev`.
  ///
  /// @param[in] SiData List of scatter/gather vectors to which to retrieve
  /// data.
  /// @param[in] SiFlags Message flags.
  /// @param[in] AddressFamily Address family of the target.
  /// @param[in] Address Address of the target.
  /// @param[in] Port Connected port.
  /// @param[out] NWritten The number of bytes transmitted.
  /// @return Nothing or WASI error
  WasiExpect<void> sockSendTo(__wasi_fd_t Fd, Span<Span<const uint8_t>> SiData,
                              __wasi_siflags_t SiFlags,
                              __wasi_address_family_t AddressFamily,
                              Span<const uint8_t> Address, uint16_t Port,
                              __wasi_size_t &NWritten) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockSendTo(SiData, SiFlags, AddressFamily, Address, Port,
                              NWritten);
    }
  }

  /// Shut down socket send and receive channels.
  ///
  /// Note: This is similar to `shutdown` in POSIX.
  ///
  /// @param[in] SdFlags Which channels on the socket to shut down.
  /// @return Nothing or WASI error
  WasiExpect<void> sockShutdown(__wasi_fd_t Fd,
                                __wasi_sdflags_t SdFlags) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockShutdown(SdFlags);
    }
  }

  WasiExpect<void> sockGetOpt(__wasi_fd_t Fd,
                              __wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName,
                              Span<uint8_t> &Flag) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockGetOpt(SockOptLevel, SockOptName, Flag);
    }
  }

  WasiExpect<void> sockSetOpt(__wasi_fd_t Fd,
                              __wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName,
                              Span<const uint8_t> Flag) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockSetOpt(SockOptLevel, SockOptName, Flag);
    }
  }

  /// Return the address and port of the file descriptor.
  WasiExpect<void> sockGetLocalAddr(__wasi_fd_t Fd,
                                    __wasi_address_family_t *AddressFamilyPtr,
                                    Span<uint8_t> Address,
                                    uint16_t *PortPtr) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockGetLocalAddr(AddressFamilyPtr, Address, PortPtr);
    }
  }

  /// Retrieve the remote address and port from the given file descriptor.
  WasiExpect<void> sockGetPeerAddr(__wasi_fd_t Fd,
                                   __wasi_address_family_t *AddressFamilyPtr,
                                   Span<uint8_t> Address,
                                   uint16_t *PortPtr) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->sockGetPeerAddr(AddressFamilyPtr, Address, PortPtr);
    }
  }

  WasiExpect<uint64_t> getNativeHandler(__wasi_fd_t Fd) const noexcept {
    auto Node = getNodeOrNull(Fd);
    if (unlikely(!Node)) {
      return WasiUnexpect(__WASI_ERRNO_BADF);
    } else {
      return Node->getNativeHandler();
    }
  }

  static std::string randomFilename() noexcept {
    using namespace std::literals;
    static constexpr const auto Charset =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
    std::random_device Device;
    std::default_random_engine Engine(Device());
    std::uniform_int_distribution<uint32_t> Distribution(
        0, static_cast<uint32_t>(Charset.size() - 1));
    std::array<char, 8> Buffer;
    std::array<uint32_t, 2> Values = {Distribution(Engine),
                                      Distribution(Engine)};
    auto ValuesSpan = cxx20::as_bytes(cxx20::span(Values));
    std::copy(ValuesSpan.begin(), ValuesSpan.end(),
              cxx20::as_writable_bytes(cxx20::span(Buffer)).begin());
    return std::string(Buffer.data(), Buffer.size());
  }

private:
  std::vector<std::string> Arguments;
  std::vector<std::string> EnvironVariables;
  __wasi_exitcode_t ExitCode = 0;

  mutable std::shared_mutex PollerMutex; ///< Protect PollerPool
  std::vector<EVPoller> PollerPool;
  friend class EVPoller;

  mutable std::shared_mutex FdMutex; ///< Protect FdMap
  std::unordered_map<__wasi_fd_t, std::shared_ptr<VINode>> FdMap;

  std::shared_ptr<VINode> getNodeOrNull(__wasi_fd_t Fd) const {
    std::shared_lock Lock(FdMutex);
    if (auto It = FdMap.find(Fd); It != FdMap.end()) {
      return It->second;
    }
    return {};
  }

  WasiExpect<__wasi_fd_t> generateRandomFdToNode(std::shared_ptr<VINode> Node) {
    std::random_device Device;
    std::default_random_engine Engine(Device());
    std::uniform_int_distribution<__wasi_fd_t> Distribution(0, 0x7FFFFFFF);
    bool Success = false;
    __wasi_fd_t NewFd;
    while (!Success) {
      NewFd = Distribution(Engine);
      std::unique_lock Lock(FdMutex);
      Success = FdMap.emplace(NewFd, Node).second;
    }
    return NewFd;
  }
};

class EVPoller : protected VPoller {
public:
  EVPoller(EVPoller &&) = default;
  EVPoller &operator=(EVPoller &&) = default;

  using VPoller::clock;
  using VPoller::close;
  using VPoller::error;
  using VPoller::prepare;
  using VPoller::reset;
  using VPoller::result;
  using VPoller::VPoller;
  using VPoller::wait;

  /// Concurrently poll for a ready-to-read event.
  ///
  /// @param[in] Fd The file descriptor on which to wait for it to become ready
  /// for reading.
  /// @param[in] Trigger Specifying whether the notification is level-trigger or
  /// edge-trigger.
  /// @param[in] UserData User-provided value that may be attached to objects
  /// that is retained when extracted from the implementation.
  void read(__wasi_fd_t Fd, TriggerType Trigger,
            __wasi_userdata_t UserData) noexcept {
    if (auto Node = env().getNodeOrNull(Fd); unlikely(!Node)) {
      VPoller::error(UserData, __WASI_ERRNO_BADF, __WASI_EVENTTYPE_FD_READ);
    } else {
      VPoller::read(Node, Trigger, UserData);
    }
  }

  /// Concurrently poll for a ready-to-write event.
  ///
  /// @param[in] Fd The file descriptor on which to wait for it to become ready
  /// for writing.
  /// @param[in] Trigger Specifying whether the notification is level-trigger or
  /// edge-trigger.
  /// @param[in] UserData User-provided value that may be attached to objects
  /// that is retained when extracted from the implementation.
  void write(__wasi_fd_t Fd, TriggerType Trigger,
             __wasi_userdata_t UserData) noexcept {
    if (auto Node = env().getNodeOrNull(Fd); unlikely(!Node)) {
      VPoller::error(UserData, __WASI_ERRNO_BADF, __WASI_EVENTTYPE_FD_WRITE);
    } else {
      VPoller::write(Node, Trigger, UserData);
    }
  }

  void close(std::shared_ptr<VINode> Node) noexcept { VPoller::close(Node); }

private:
  Environ &env() noexcept { return static_cast<Environ &>(Ctx.get()); }
};

inline WasiExpect<EVPoller>
Environ::acquirePoller(Span<__wasi_event_t> Events) noexcept {
  auto Poller = [this]() noexcept -> EVPoller {
    std::unique_lock Lock(PollerMutex);
    if (PollerPool.empty()) {
      return EVPoller(*this);
    } else {
      EVPoller Result(std::move(PollerPool.back()));
      PollerPool.pop_back();
      return Result;
    }
  }();

  if (auto Res = Poller.prepare(Events); !Res) {
    return WasiUnexpect(Res);
  }
  return Poller;
}

inline void Environ::close(std::shared_ptr<VINode> Node) noexcept {
  std::unique_lock Lock(PollerMutex);
  for (auto &Poller : PollerPool) {
    Poller.close(Node);
  }
}

inline void Environ::releasePoller(EVPoller &&Poller) noexcept {
  std::unique_lock Lock(PollerMutex);
  PollerPool.push_back(std::move(Poller));
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
