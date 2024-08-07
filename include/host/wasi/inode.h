// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "host/wasi/error.h"
#include "host/wasi/vfs.h"
#include <functional>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dirent.h>
#include <sys/stat.h>
#include <unordered_map>
#elif WASMEDGE_OS_WINDOWS
#include "system/winapi.h"
#endif

#if WASMEDGE_OS_LINUX
#if defined(__GLIBC_PREREQ)
#if defined(_LIBCPP_GLIBC_PREREQ)
#undef _LIBCPP_GLIBC_PREREQ
#endif
#define _LIBCPP_GLIBC_PREREQ(a, b) 0
#else
#if defined(_LIBCPP_GLIBC_PREREQ)
#define __GLIBC_PREREQ(a, b) _LIBCPP_GLIBC_PREREQ(a, b)
#else
#define __GLIBC_PREREQ(a, b) 1
#endif
#endif
#endif

#if WASMEDGE_OS_LINUX
#include <sys/epoll.h>
#endif
#if WASMEDGE_OS_MACOS
#include <sys/event.h>
#endif

namespace WasmEdge {
namespace Host {
namespace WASI {

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
struct FdHolder {
  FdHolder(const FdHolder &) = delete;
  FdHolder &operator=(const FdHolder &) = delete;
  FdHolder(FdHolder &&RHS) noexcept
      : Fd(std::exchange(RHS.Fd, -1)), Cleanup(RHS.Cleanup),
        Append(RHS.Append) {
    RHS.Cleanup = true;
    RHS.Append = false;
  }
  FdHolder &operator=(FdHolder &&RHS) noexcept {
    using std::swap;
    swap(Fd, RHS.Fd);
    Cleanup = RHS.Cleanup;
    Append = RHS.Append;
    RHS.Cleanup = true;
    RHS.Append = false;
    return *this;
  }

  constexpr FdHolder() noexcept : Fd(-1), Cleanup(true), Append(false) {}
  ~FdHolder() noexcept {
    if (Cleanup) {
      reset();
    }
  }
  explicit constexpr FdHolder(int Fd, bool Cleanup = true,
                              bool Append = false) noexcept
      : Fd(Fd), Cleanup(Cleanup), Append(Append) {}
  constexpr bool ok() const noexcept { return Fd >= 0; }
  void reset() noexcept;
  int release() noexcept { return std::exchange(Fd, -1); }
  void emplace(int NewFd) noexcept {
    reset();
    Fd = NewFd;
  }
  int getFd() noexcept { return Fd; }
  int Fd = -1;
  bool Cleanup : 1;
  mutable bool Append : 1;
};

struct DirHolder {
  DirHolder(const DirHolder &) = delete;
  DirHolder &operator=(const DirHolder &) = delete;
  DirHolder(DirHolder &&RHS) noexcept {
    using std::swap;
    swap(Dir, RHS.Dir);
    swap(Cookie, RHS.Cookie);
  }
  DirHolder &operator=(DirHolder &&RHS) noexcept {
    using std::swap;
    swap(Dir, RHS.Dir);
    swap(Cookie, RHS.Cookie);
    return *this;
  }

  DirHolder() noexcept = default;
  explicit DirHolder(DIR *D) noexcept : Dir(D) {}
  constexpr bool ok() const noexcept { return Dir != nullptr; }
  ~DirHolder() noexcept { reset(); }
  void reset() noexcept;
  void emplace(DIR *NewDir) noexcept {
    reset();
    Dir = NewDir;
  }

  DIR *Dir = nullptr;
  uint64_t Cookie = 0;
  static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= alignof(__wasi_dirent_t));
  std::vector<uint8_t> Buffer;
};
#endif

#if WASMEDGE_OS_LINUX
struct TimerHolder {
  TimerHolder(const TimerHolder &) = delete;
  TimerHolder &operator=(const TimerHolder &) = delete;
  TimerHolder(TimerHolder &&RHS) noexcept {
    using std::swap;
    swap(Id, RHS.Id);
  }
  TimerHolder &operator=(TimerHolder &&RHS) noexcept {
    using std::swap;
    swap(Id, RHS.Id);
    return *this;
  }

  constexpr TimerHolder() = default;
  explicit constexpr TimerHolder(timer_t T) noexcept : Id(T) {}
  ~TimerHolder() noexcept { reset(); }
  void reset() noexcept;
  void emplace(timer_t NewId) noexcept {
    reset();
    Id = NewId;
  }
  std::optional<timer_t> Id;
};
#endif

#if WASMEDGE_OS_WINDOWS
struct HandleHolder {
  enum class HandleType : uint8_t {
    NormalHandle,
    StdHandle,
    NormalSocket,
  };

  HandleHolder(const HandleHolder &) = delete;
  HandleHolder &operator=(const HandleHolder &) = delete;
  HandleHolder(HandleHolder &&RHS) noexcept {
    using std::swap;
    swap(*this, RHS);
  }
  HandleHolder &operator=(HandleHolder &&RHS) noexcept {
    using std::swap;
    swap(*this, RHS);
    return *this;
  }

  constexpr HandleHolder() noexcept = default;
  ~HandleHolder() noexcept { reset(); }
  HandleHolder(winapi::HANDLE_ Handle, bool IsStdHandle) noexcept
      : Handle(likely(Handle != winapi::INVALID_HANDLE_VALUE_) ? Handle
                                                               : nullptr),
        Type(IsStdHandle ? HandleType::StdHandle : HandleType::NormalHandle) {}
  HandleHolder(winapi::SOCKET_ Socket) noexcept
      : Socket(Socket), Type(HandleType::NormalSocket) {}
  HandleHolder(const std::filesystem::path &Path,
               const winapi::DWORD_ AccessFlags,
               const winapi::DWORD_ ShareFlags,
               const winapi::DWORD_ CreationDisposition,
               const winapi::DWORD_ AttributeFlags) noexcept;
  bool reopen(const winapi::DWORD_ AccessFlags, const winapi::DWORD_ ShareFlags,

              const winapi::DWORD_ AttributeFlags) noexcept;
  constexpr bool ok() const noexcept { return Handle != nullptr; }
  constexpr bool isStdHandle() const noexcept {
    return Type == HandleType::StdHandle;
  }
  constexpr bool isSocket() const noexcept {
    return Type == HandleType::NormalSocket;
  }
  void reset() noexcept;
  winapi::HANDLE_ release() noexcept { return std::exchange(Handle, nullptr); }
  winapi::HANDLE_ exchange(winapi::HANDLE_ NewHandle) noexcept {
    return std::exchange(Handle, NewHandle);
  }
  void emplace(winapi::HANDLE_ NewHandle) noexcept {
    reset();
    Handle = NewHandle;
  }
  friend void swap(HandleHolder &LHS, HandleHolder &RHS) noexcept {
    using std::swap;
    swap(LHS.Handle, RHS.Handle);
    swap(LHS.Type, RHS.Type);
  }
  WasiExpect<void> filestatGet(__wasi_filestat_t &FileStat) const noexcept;

  union {
    winapi::HANDLE_ Handle = nullptr;
    winapi::SOCKET_ Socket;
  };
  HandleType Type = HandleType::NormalHandle;
};

template <typename T> class FindHolderBase {
private:
  struct Proxy : T {
    static void doReset(T &Base) noexcept {
      const auto Ptr = &T::doReset;
      return (Base.*Ptr)();
    }
    static WasiExpect<void> doRewind(T &Base, bool First) noexcept {
      const auto Ptr = &T::doRewind;
      return (Base.*Ptr)(First);
    }
    static bool doNext(T &Base) noexcept {
      const auto Ptr = &T::doNext;
      return (Base.*Ptr)();
    }
    static WasiExpect<void> doLoadDirent(T &Base) noexcept {
      const auto Ptr = &T::doLoadDirent;
      return (Base.*Ptr)();
    }
  };

public:
  FindHolderBase(FindHolderBase &&RHS) noexcept {
    using std::swap;
    swap(Handle, RHS.Handle);
    swap(Cookie, RHS.Cookie);
    swap(Buffer, RHS.Buffer);
  }
  FindHolderBase &operator=(FindHolderBase &&RHS) noexcept {
    using std::swap;
    swap(Handle, RHS.Handle);
    swap(Cookie, RHS.Cookie);
    swap(Buffer, RHS.Buffer);
    return *this;
  }

  FindHolderBase() noexcept = default;
  FindHolderBase(const FindHolderBase &) noexcept = delete;
  FindHolderBase &operator=(const FindHolderBase &) noexcept = delete;
  constexpr bool ok() const noexcept { return Handle != nullptr; }
  ~FindHolderBase() noexcept { reset(); }

  WasiExpect<void> emplace(winapi::HANDLE_ PathHandle) noexcept;
  void reset() noexcept {
    Proxy::doReset(static_cast<T &>(*this));
    Path.clear();
    Handle = nullptr;
    Cookie = 0;
    Buffer.clear();
  }
  WasiExpect<void> seek(uint64_t NewCookie) noexcept;
  bool next() noexcept;
  WasiExpect<void> loadDirent() noexcept;
  size_t write(Span<uint8_t> Output) noexcept;

protected:
  auto getPath() const noexcept { return Path; }
  auto getHandle() const noexcept { return Handle; }
  void setHandle(winapi::HANDLE_ New) noexcept { Handle = New; }
  auto getCookie() const noexcept { return Cookie; }
  Span<const uint8_t> getBuffer() const noexcept { return Buffer; }
  Span<uint8_t> getBuffer() noexcept { return Buffer; }
  void resizeBuffer(size_t Size) noexcept { Buffer.resize(Size); }

private:
  std::filesystem::path Path;
  winapi::HANDLE_ Handle = nullptr;
  uint64_t Cookie = 0;
  static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= alignof(__wasi_dirent_t));
  std::vector<uint8_t> Buffer;
};

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_VISTA
class FindHolder : public FindHolderBase<FindHolder> {
public:
  const winapi::FILE_ID_BOTH_DIR_INFO_ &getData() const noexcept;

protected:
  void doReset() noexcept;
  WasiExpect<void> doRewind(bool First) noexcept;
  bool doNext() noexcept;
  WasiExpect<void> doLoadDirent() noexcept;

private:
  bool nextData() noexcept;
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    winapi::FILE_ID_BOTH_DIR_INFO_ FindData;
    std::array<uint8_t,
               sizeof(winapi::FILE_ID_BOTH_DIR_INFO_) +
                   sizeof(wchar_t[winapi::UNICODE_STRING_MAX_CHARS_ + 1])>
        FindDataPadding;
  } FindDataUnion;
  uint32_t Cursor = 0;
  static_assert(std::numeric_limits<decltype(Cursor)>::max() >
                sizeof(FindDataUnion));
};
#else
class FindHolder : public FindHolderBase<FindHolder> {
protected:
  WasiExpect<winapi::HANDLE_> doEmplace(const std::filesystem::path &NewPath,
                                        winapi::HANDLE_ PathHandle) noexcept;
  void doReset() noexcept;
  WasiExpect<void> doRewind(bool First) noexcept;
  bool doNext() noexcept;
  WasiExpect<void> doLoadDirent() noexcept;

private:
  winapi::WIN32_FIND_DATAW_ FindData;
};
#endif

#endif

enum class TriggerType {
  Level,
  Edge,
};
class Poller;

class INode
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
    : public FdHolder
#elif WASMEDGE_OS_WINDOWS
    : public HandleHolder
#endif
{
public:
  INode(const INode &) = delete;
  INode &operator=(const INode &) = delete;
  INode(INode &&RHS) noexcept = default;
  INode &operator=(INode &&RHS) noexcept = default;

  static INode stdIn() noexcept;

  static INode stdOut() noexcept;

  static INode stdErr() noexcept;

  /// Open a file or directory.
  ///
  /// @param[in] Path The absolute path of the file or directory to open.
  /// @param[in] OpenFlags The method by which to open the file.
  /// @param[in] FdFlags The method by which to open the file.
  /// @param[in] VFSFlags The method by which to open the file.
  /// @return The file descriptor of the file that has been opened, or WASI
  /// error.
  static WasiExpect<INode> open(std::string Path, __wasi_oflags_t OpenFlags,
                                __wasi_fdflags_t FdFlags,
                                VFS::Flags VFSFlags) noexcept;

  /// Provide file advisory information on a file descriptor.
  ///
  /// Note: This is similar to `posix_fadvise` in POSIX.
  ///
  /// @param[in] Offset The offset within the file to which the advisory
  /// applies.
  /// @param[in] Len The length of the region to which the advisory applies.
  /// @param[in] Advice The advice.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t Len,
                            __wasi_advice_t Advice) const noexcept;

  /// Force the allocation of space in a file.
  ///
  /// Note: This is similar to `posix_fallocate` in POSIX.
  ///
  /// @param[in] Offset The offset at which to start the allocation.
  /// @param[in] Len The length of the area that is allocated.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAllocate(__wasi_filesize_t Offset,
                              __wasi_filesize_t Len) const noexcept;

  /// Synchronize the data of a file to disk.
  ///
  /// Note: This is similar to `fdatasync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdDatasync() const noexcept;

  /// Get the attributes of a file descriptor.
  ///
  /// Note: This returns similar flags to `fsync(fd, F_GETFL)` in POSIX, as
  /// well
  ///
  /// as additional fields.
  /// @param[out] FdStat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept;

  /// Adjust the flags associated with a file descriptor.
  ///
  /// Note: This is similar to `fcntl(fd, F_SETFL, flags)` in POSIX.
  ///
  /// @param[in] FdFlags The desired values of the file descriptor flags.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept;

  /// Return the attributes of an open file.
  ///
  /// @param[out] Filestat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept;

  /// Adjust the size of an open file. If this increases the file's size, the
  /// extra bytes are filled with zeros.
  ///
  /// Note: This is similar to `ftruncate` in POSIX.
  ///
  /// @param[in] Size The desired file size.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatSetSize(__wasi_filesize_t Size) const noexcept;

  /// Adjust the timestamps of an open file or directory.
  ///
  /// Note: This is similar to `futimens` in POSIX.
  ///
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void>
  fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                     __wasi_fstflags_t FstFlags) const noexcept;

  /// Read from a file descriptor, without using and updating the file
  /// descriptor's offset.
  ///
  /// Note: This is similar to `preadv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors in which to store data.
  /// @param[in] Offset The offset within the file at which to read.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPread(Span<Span<uint8_t>> IOVs, __wasi_filesize_t Offset,
                           __wasi_size_t &NRead) const noexcept;

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
  WasiExpect<void> fdPwrite(Span<Span<const uint8_t>> IOVs,
                            __wasi_filesize_t Offset,
                            __wasi_size_t &NWritten) const noexcept;

  /// Read from a file descriptor.
  ///
  /// Note: This is similar to `readv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors to which to store data.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdRead(Span<Span<uint8_t>> IOVs,
                          __wasi_size_t &NRead) const noexcept;

  /// Read directory entries from a directory.
  ///
  /// When successful, the contents of the output buffer consist of a sequence
  /// of directory entries. Each directory entry consists of a `dirent`
  /// object, followed by `dirent::d_namlen` bytes holding the name of the
  /// directory entry.
  ///
  /// This function fills the output buffer as much as possible,
  /// potentially truncating the last directory entry. This allows the caller
  /// to grow its read buffer size in case it's too small to fit a single
  /// large directory entry, or skip the oversized directory entry.
  ///
  /// @param[out] Buffer The buffer where directory entries are stored.
  /// @param[in] Cookie The location within the directory to start reading
  /// @param[out] Size The number of bytes stored in the read buffer. If less
  /// than the size of the read buffer, the end of the directory has been
  /// reached.
  /// @return Nothing or WASI error
  WasiExpect<void> fdReaddir(Span<uint8_t> Buffer, __wasi_dircookie_t Cookie,
                             __wasi_size_t &Size) noexcept;

  /// Move the offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek` in POSIX.
  ///
  /// @param[in] Offset The number of bytes to move.
  /// @param[in] Whence The base from which the offset is relative.
  /// @param[out] Size The new offset of the file descriptor, relative to the
  /// start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdSeek(__wasi_filedelta_t Offset, __wasi_whence_t Whence,
                          __wasi_filesize_t &Size) const noexcept;

  /// Synchronize the data and metadata of a file to disk.
  ///
  /// Note: This is similar to `fsync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdSync() const noexcept;

  /// Return the current offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek(fd, 0, SEEK_CUR)` in POSIX.
  ///
  /// @param[out] Size The current offset of the file descriptor, relative to
  /// the start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdTell(__wasi_filesize_t &Size) const noexcept;

  /// Write to a file descriptor.
  ///
  /// Note: This is similar to `writev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdWrite(Span<Span<const uint8_t>> IOVs,
                           __wasi_size_t &NWritten) const noexcept;

  /// Get the native handler.
  ///
  /// Note: Users should cast this native handler to corresponding types
  /// on different operating systems. E.g. int on POSIX or void * on Windows
  ///
  /// @return The native handler in uint64_t.
  WasiExpect<uint64_t> getNativeHandler() const noexcept;

  /// Create a directory.
  ///
  /// Note: This is similar to `mkdirat` in POSIX.
  ///
  /// @param[in] Path The path at which to create the directory.
  /// @return Nothing or WASI error
  WasiExpect<void> pathCreateDirectory(std::string Path) const noexcept;

  /// Return the attributes of a file or directory.
  ///
  /// Note: This is similar to `stat` in POSIX.
  ///
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[out] Filestat The buffer where the file's attributes are stored.
  /// @return Nothing or WASI error
  WasiExpect<void> pathFilestatGet(std::string Path,
                                   __wasi_filestat_t &Filestat) const noexcept;

  /// Adjust the timestamps of a file or directory.
  ///
  /// Note: This is similar to `utimensat` in POSIX.
  ///
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void>
  pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                       __wasi_timestamp_t MTim,
                       __wasi_fstflags_t FstFlags) const noexcept;

  /// Create a hard link.
  ///
  /// Note: This is similar to `linkat` in POSIX.
  ///
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path from which to link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the hard
  /// link. resolved.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathLink(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept;

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
  /// @param[in] Path The relative path of the file or directory to open,
  /// relative to the `path_open::fd` directory.
  /// @param[in] OpenFlags The method by which to open the file.
  /// @param[in] FdFlags The method by which to open the file.
  /// @param[in] VFSFlags The method by which to open the file.
  /// @return The file descriptor of the file that has been opened, or WASI
  /// error.
  WasiExpect<INode> pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                             __wasi_fdflags_t FdFlags,
                             VFS::Flags VFSFlags) const noexcept;

  /// Read the contents of a symbolic link.
  ///
  /// Note: This is similar to `readlinkat` in POSIX.
  ///
  /// @param[in] Path The path of the symbolic link from which to read.
  /// @param[out] Buffer The buffer to which to write the contents of the
  /// symbolic link.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathReadlink(std::string Path, Span<char> Buffer,
                                __wasi_size_t &NRead) const noexcept;

  /// Remove a directory.
  ///
  /// Return `errno::notempty` if the directory is not empty.
  ///
  /// Note: This is similar to `unlinkat(fd, path, AT_REMOVEDIR)` in POSIX.
  ///
  /// @param[in] Path The path to a directory to remove.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathRemoveDirectory(std::string Path) const noexcept;

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
  static WasiExpect<void> pathRename(const INode &Old, std::string OldPath,
                                     const INode &New,
                                     std::string NewPath) noexcept;

  /// Create a symbolic link.
  ///
  /// Note: This is similar to `symlinkat` in POSIX.
  ///
  /// @param[in] OldPath The contents of the symbolic link.
  /// @param[in] NewPath The destination path at which to create the symbolic
  /// link.
  /// @return Nothing or WASI error
  WasiExpect<void> pathSymlink(std::string OldPath,
                               std::string NewPath) const noexcept;

  /// Unlink a file.
  ///
  /// Return `errno::isdir` if the path refers to a directory.
  ///
  /// Note: This is similar to `unlinkat(fd, path, 0)` in POSIX.
  ///
  /// @param[in] Path The path to a file to unlink.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathUnlinkFile(std::string Path) const noexcept;

  static WasiExpect<void>
  getAddrinfo(std::string_view NodeStr, std::string_view ServiceStr,
              const __wasi_addrinfo_t &Hint, uint32_t MaxResLength,
              Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
              Span<__wasi_sockaddr_t *> WasiSockaddrArray,
              Span<char *> AiAddrSaDataArray, Span<char *> AiCanonnameArray,
              /*Out*/ __wasi_size_t &ResLength) noexcept;

  static WasiExpect<INode> sockOpen(__wasi_address_family_t SysDomain,
                                    __wasi_sock_type_t SockType) noexcept;

  WasiExpect<void> sockBind(__wasi_address_family_t AddressFamily,
                            Span<const uint8_t> Address,
                            uint16_t Port) noexcept;

  WasiExpect<void> sockListen(int32_t Backlog) noexcept;

  WasiExpect<INode> sockAccept(__wasi_fdflags_t FdFlags) noexcept;

  WasiExpect<void> sockConnect(__wasi_address_family_t AddressFamily,
                               Span<const uint8_t> Address,
                               uint16_t Port) noexcept;

  /// Receive a message from a socket.
  ///
  /// Note: This is similar to `recv` in POSIX, though it also supports
  /// reading the data into multiple buffers in the manner of `readv`.
  ///
  /// @param[in] RiData List of scatter/gather vectors to which to store data.
  /// @param[in] RiFlags Message flags.
  /// @param[out] NRead Return the number of bytes stored in RiData.
  /// @param[out] RoFlags Return message flags.
  /// @return Nothing or WASI error.
  WasiExpect<void> sockRecv(Span<Span<uint8_t>> RiData,
                            __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                            __wasi_roflags_t &RoFlags) const noexcept;

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
  WasiExpect<void> sockRecvFrom(Span<Span<uint8_t>> RiData,
                                __wasi_riflags_t RiFlags,
                                __wasi_address_family_t *AddressFamilyPtr,
                                Span<uint8_t> Address, uint16_t *PortPtr,
                                __wasi_size_t &NRead,
                                __wasi_roflags_t &RoFlags) const noexcept;

  /// Send a message on a socket.
  ///
  /// Note: This is similar to `send` in POSIX, though it also supports
  /// writing the data from multiple buffers in the manner of `writev`.
  ///
  /// @param[in] SiData List of scatter/gather vectors to which to retrieve
  /// data.
  /// @param[in] SiFlags Message flags.
  /// @param[out] NWritten The number of bytes transmitted.
  /// @return Nothing or WASI error
  WasiExpect<void> sockSend(Span<Span<const uint8_t>> SiData,
                            __wasi_siflags_t SiFlags,
                            __wasi_size_t &NWritten) const noexcept;

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
  WasiExpect<void> sockSendTo(Span<Span<const uint8_t>> SiData,
                              __wasi_siflags_t SiFlags,
                              __wasi_address_family_t AddressFamily,
                              Span<const uint8_t> Address, uint16_t Port,
                              __wasi_size_t &NWritten) const noexcept;

  /// Shut down socket send and receive channels.
  ///
  /// Note: This is similar to `shutdown` in POSIX.
  ///
  /// @param[in] SdFlags Which channels on the socket to shut down.
  /// @return Nothing or WASI error
  WasiExpect<void> sockShutdown(__wasi_sdflags_t SdFlags) const noexcept;

  WasiExpect<void> sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName,
                              Span<uint8_t> &Flag) const noexcept;

  WasiExpect<void> sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName,
                              Span<const uint8_t> Flag) const noexcept;

  WasiExpect<void> sockGetLocalAddr(__wasi_address_family_t *AddressFamilyPtr,
                                    Span<uint8_t> Address,
                                    uint16_t *PortPtr) const noexcept;

  WasiExpect<void> sockGetPeerAddr(__wasi_address_family_t *AddressFamilyPtr,
                                   Span<uint8_t> Address,
                                   uint16_t *PortPtr) const noexcept;

  /// File type.
  WasiExpect<__wasi_filetype_t> filetype() const noexcept;

  /// Check if this inode is a directory.
  bool isDirectory() const noexcept;

  /// Check if this inode is a symbolic link.
  bool isSymlink() const noexcept;

  /// File size.
  WasiExpect<__wasi_filesize_t> filesize() const noexcept;

  /// Check if current user has execute permission on this inode.
  bool canBrowse() const noexcept;

private:
  friend class Poller;

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
public:
  using FdHolder::FdHolder;

private:
  mutable std::optional<struct stat> Stat;

  DirHolder Dir;

  __wasi_filetype_t unsafeFiletype() const noexcept;
  WasiExpect<void> updateStat() const noexcept;

#elif WASMEDGE_OS_WINDOWS
public:
  using HandleHolder::HandleHolder;

private:
  __wasi_fdflags_t SavedFdFlags = {};
  uint8_t SavedVFSFlags = {};

  FindHolder Find;
#endif
};

class PollerContext;
class Poller
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
    : public FdHolder
#endif
{
public:
  Poller(const Poller &) = delete;
  Poller &operator=(const Poller &) = delete;
  Poller(Poller &&RHS) noexcept = default;
  Poller &operator=(Poller &&RHS) noexcept = default;
  Poller(PollerContext &) noexcept;

  /// Records an error for polling.
  ///
  /// @param[in] UserData User-provided value that may be attached to objects
  /// that is retained when extracted from the implementation.
  /// @param[in] Error Occurred while processing the subscription request.
  /// @param[in] Type The type of event that occurred
  /// additionally to coalesce with other events.
  void error(__wasi_userdata_t UserData, __wasi_errno_t Error,
             __wasi_eventtype_t Type) noexcept {
    assuming(Events.size() < WasiEvents.size());
    auto &Event = Events.emplace_back();
    Event.Valid = true;
    Event.userdata = UserData;
    Event.error = Error;
    Event.type = Type;
    switch (Type) {
    case __WASI_EVENTTYPE_FD_READ:
    case __WASI_EVENTTYPE_FD_WRITE:
      Event.fd_readwrite.nbytes = 0;
      Event.fd_readwrite.flags = static_cast<__wasi_eventrwflags_t>(0);
      break;
    default:
      break;
    }
  }

  /// Prepare for concurrently polling for the occurrence of a set of events.
  ///
  /// @param[in] Events The output buffer for events.
  WasiExpect<void> prepare(Span<__wasi_event_t> Events) noexcept;

  /// Concurrently poll for a time event.
  ///
  /// @param[in] Clock The clock against which to compare the timestamp.
  /// @param[in] Timeout The absolute or relative timestamp
  /// @param[in] Precision The amount of time that the implementation may wait
  /// @param[in] Flags Specifying whether the timeout is absolute or relative
  /// additionally to coalesce with other events.
  /// @param[in] UserData User-provided value that may be attached to objects
  /// that is retained when extracted from the implementation.
  void clock(__wasi_clockid_t Clock, __wasi_timestamp_t Timeout,
             __wasi_timestamp_t Precision, __wasi_subclockflags_t Flags,
             __wasi_userdata_t UserData) noexcept;

  void read(const INode &Fd, TriggerType Trigger,
            __wasi_userdata_t UserData) noexcept;

  void write(const INode &Fd, TriggerType Trigger,
             __wasi_userdata_t UserData) noexcept;

  void close(const INode &Fd) noexcept;
  /// Concurrently poll for events.
  void wait() noexcept;

  /// Return number of events.
  ///
  /// @return Number of event occurred
  __wasi_size_t result() const noexcept {
    assuming(Events.size() == WasiEvents.size());
    __wasi_size_t NEvent = 0;
    for (const auto &Event : Events) {
      if (Event.Valid) {
        WasiEvents[NEvent] = Event;
        ++NEvent;
      }
    }
    return NEvent;
  }

  /// Reset all status
  void reset() noexcept;

  bool ok() noexcept;

protected:
  std::reference_wrapper<PollerContext> Ctx;

private:
  Span<__wasi_event_t> WasiEvents;
  struct OptionalEvent : __wasi_event_t {
    bool Valid;
  };
  std::vector<OptionalEvent> Events;

#if WASMEDGE_OS_LINUX | WASMEDGE_OS_MACOS
  struct FdData {
    OptionalEvent *ReadEvent = nullptr;
    OptionalEvent *WriteEvent = nullptr;
  };
  std::unordered_map<int, FdData> FdDatas;
  std::unordered_map<int, FdData> OldFdDatas;
#endif

#if WASMEDGE_OS_WINDOWS
  struct SocketData {
    OptionalEvent *ReadEvent = nullptr;
    OptionalEvent *WriteEvent = nullptr;
  };
  std::unordered_map<winapi::SOCKET_, SocketData> SocketDatas;
  winapi::FD_SET_ ReadFds = {0, {}}, WriteFds = {0, {}};
#endif

#if WASMEDGE_OS_LINUX
  friend class PollerContext;
  struct Timer : public FdHolder {
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;
    Timer(Timer &&RHS) noexcept = default;
    Timer &operator=(Timer &&RHS) noexcept = default;
    constexpr Timer(__wasi_clockid_t C) noexcept : Clock(C) {}

    WasiExpect<void> create() noexcept;

    WasiExpect<void> setTime(__wasi_timestamp_t Timeout,
                             __wasi_timestamp_t Precision,
                             __wasi_subclockflags_t Flags) noexcept;

    __wasi_clockid_t Clock;
#if !__GLIBC_PREREQ(2, 8)
    FdHolder Notify;
    TimerHolder TimerId;
#endif
  };

  std::vector<Timer> Timers;
  std::vector<struct epoll_event> EPollEvents;
#endif

#if WASMEDGE_OS_MACOS
  std::vector<struct kevent> KEvents;
  uint64_t NextTimerId = 0;
#endif
#if WASMEDGE_OS_WINDOWS
  std::unordered_map<winapi::HANDLE_, OptionalEvent *> ConsoleReadEvent;
  std::unordered_map<winapi::HANDLE_, OptionalEvent *> ConsoleWriteEvent;
  OptionalEvent *TimeoutEvent = nullptr;
  winapi::TIMEVAL_ MinimumTimeout;
#endif
};

class PollerContext {
#if WASMEDGE_OS_LINUX
public:
  WasiExpect<Poller::Timer> acquireTimer(__wasi_clockid_t Clock) noexcept;
  void releaseTimer(Poller::Timer &&) noexcept;

private:
  std::mutex TimerMutex; ///< Protect TimerPool
  std::unordered_map<__wasi_clockid_t, std::vector<Poller::Timer>> TimerPool;
#else
#endif
};

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
