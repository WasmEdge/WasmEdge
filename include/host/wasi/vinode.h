// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/filesystem.h"
#include "host/wasi/error.h"
#include "host/wasi/inode.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WASI {

class VFS;
class VPoller;
class VEpoller;
class VINode : public std::enable_shared_from_this<VINode> {
public:
  VINode(const VINode &) = delete;
  VINode &operator=(const VINode &) = delete;
  VINode(VINode &&) = default;
  VINode &operator=(VINode &&) = default;

  /// Create a VINode with a parent.
  ///
  /// @param[in] FS Filesystem.
  /// @param[in] Node System INode.
  /// @param[in] Parent Parent VINode.
  VINode(VFS &FS, INode Node, std::shared_ptr<VINode> Parent);

  /// Create a orphan VINode.
  ///
  /// @param[in] FS Filesystem.
  /// @param[in] Node System INode.
  /// @param[in] FRB The desired rights of the VINode.
  /// @param[in] FRI The desired rights of the VINode.
  VINode(VFS &FS, INode Node, __wasi_rights_t FRB, __wasi_rights_t FRI,
         std::string N = {});

  static std::shared_ptr<VINode> stdIn(VFS &FS, __wasi_rights_t FRB,
                                       __wasi_rights_t FRI);
  static std::shared_ptr<VINode> stdOut(VFS &FS, __wasi_rights_t FRB,
                                        __wasi_rights_t FRI);
  static std::shared_ptr<VINode> stdErr(VFS &FS, __wasi_rights_t FRB,
                                        __wasi_rights_t FRI);

  static std::string canonicalGuest(std::string_view Path);

  static WasiExpect<std::shared_ptr<VINode>> bind(VFS &FS, __wasi_rights_t FRB,
                                                  __wasi_rights_t FRI,
                                                  std::string Name,
                                                  std::string SystemPath);

  bool isPreopened() const { return !Parent && !Name.empty(); }

  constexpr const std::string &name() const { return Name; }

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
                            __wasi_advice_t Advice) const noexcept {
    if (!can(__WASI_RIGHTS_FD_ADVISE)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdAdvise(Offset, Len, Advice);
  }

  /// Force the allocation of space in a file.
  ///
  /// Note: This is similar to `posix_fallocate` in POSIX.
  ///
  /// @param[in] Offset The offset at which to start the allocation.
  /// @param[in] Len The length of the area that is allocated.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAllocate(__wasi_filesize_t Offset,
                              __wasi_filesize_t Len) const noexcept {
    if (!can(__WASI_RIGHTS_FD_ALLOCATE)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdAllocate(Offset, Len);
  }

  /// Synchronize the data of a file to disk.
  ///
  /// Note: This is similar to `fdatasync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdDatasync() const noexcept {
    if (!can(__WASI_RIGHTS_FD_DATASYNC)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdDatasync();
  }

  /// Get the attributes of a file descriptor.
  ///
  /// Note: This returns similar flags to `fsync(fd, F_GETFL)` in POSIX, as well
  ///
  /// as additional fields.
  /// @param[out] FdStat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept {
    FdStat.fs_rights_base = FsRightsBase;
    FdStat.fs_rights_inheriting = FsRightsInheriting;
    return Node.fdFdstatGet(FdStat);
  }

  /// Adjust the flags associated with a file descriptor.
  ///
  /// Note: This is similar to `fcntl(fd, F_SETFL, flags)` in POSIX.
  ///
  /// @param[in] FdFlags The desired values of the file descriptor flags.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept {
    __wasi_rights_t AdditionalRequiredRights = static_cast<__wasi_rights_t>(0);

    if (FdFlags & __WASI_FDFLAGS_DSYNC) {
      AdditionalRequiredRights |= __WASI_RIGHTS_FD_DATASYNC;
    }
    if (FdFlags & __WASI_FDFLAGS_RSYNC) {
      AdditionalRequiredRights |= __WASI_RIGHTS_FD_SYNC;
    }
    if (FdFlags & __WASI_FDFLAGS_SYNC) {
      AdditionalRequiredRights |= __WASI_RIGHTS_FD_SYNC;
    }

    if (!can(__WASI_RIGHTS_FD_FDSTAT_SET_FLAGS | AdditionalRequiredRights)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdFdstatSetFlags(FdFlags);
  }

  /// Adjust the rights associated with a file descriptor.
  ///
  /// This can only be used to remove rights, and returns `errno::notcapable` if
  /// called in a way that would attempt to add rights
  ///
  /// @param[in] RightsBase The desired rights of the file descriptor.
  /// @param[in] RightsInheriting The desired rights of the file descriptor.
  /// @return Nothing or WASI error
  WasiExpect<void>
  fdFdstatSetRights(__wasi_rights_t RightsBase,
                    __wasi_rights_t RightsInheriting) noexcept {
    if (!can(RightsBase, RightsInheriting)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    FsRightsBase = RightsBase;
    FsRightsInheriting = RightsInheriting;

    return {};
  }

  /// Return the attributes of an open file.
  ///
  /// @param[out] Filestat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept {
    if (!can(__WASI_RIGHTS_FD_FILESTAT_GET)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdFilestatGet(Filestat);
  }

  /// Adjust the size of an open file. If this increases the file's size, the
  /// extra bytes are filled with zeros.
  ///
  /// Note: This is similar to `ftruncate` in POSIX.
  ///
  /// @param[in] Size The desired file size.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatSetSize(__wasi_filesize_t Size) const noexcept {
    if (!can(__WASI_RIGHTS_FD_FILESTAT_SET_SIZE)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdFilestatSetSize(Size);
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
  fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                     __wasi_fstflags_t FstFlags) const noexcept {
    if (!can(__WASI_RIGHTS_FD_FILESTAT_SET_TIMES)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdFilestatSetTimes(ATim, MTim, FstFlags);
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
  WasiExpect<void> fdPread(Span<Span<uint8_t>> IOVs, __wasi_filesize_t Offset,
                           __wasi_size_t &NRead) const noexcept {
    if (!can(__WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdPread(IOVs, Offset, NRead);
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
  WasiExpect<void> fdPwrite(Span<Span<const uint8_t>> IOVs,
                            __wasi_filesize_t Offset,
                            __wasi_size_t &NWritten) const noexcept {
    if (!can(__WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_SEEK)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdPwrite(IOVs, Offset, NWritten);
  }

  /// Read from a file descriptor.
  ///
  /// Note: This is similar to `readv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors to which to store data.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdRead(Span<Span<uint8_t>> IOVs,
                          __wasi_size_t &NRead) const noexcept {
    if (!can(__WASI_RIGHTS_FD_READ)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdRead(IOVs, NRead);
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
  WasiExpect<void> fdReaddir(Span<uint8_t> Buffer, __wasi_dircookie_t Cookie,
                             __wasi_size_t &Size) noexcept {
    if (!can(__WASI_RIGHTS_FD_READDIR)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdReaddir(Buffer, Cookie, Size);
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
  WasiExpect<void> fdSeek(__wasi_filedelta_t Offset, __wasi_whence_t Whence,
                          __wasi_filesize_t &Size) const noexcept {
    if (!can(__WASI_RIGHTS_FD_SEEK)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdSeek(Offset, Whence, Size);
  }

  /// Synchronize the data and metadata of a file to disk.
  ///
  /// Note: This is similar to `fsync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdSync() const noexcept {
    if (!can(__WASI_RIGHTS_FD_SYNC)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdSync();
  }

  /// Return the current offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek(fd, 0, SEEK_CUR)` in POSIX.
  ///
  /// @param[out] Size The current offset of the file descriptor, relative to
  /// the start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdTell(__wasi_filesize_t &Size) const noexcept {
    if (!can(__WASI_RIGHTS_FD_TELL)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdTell(Size);
  }

  /// Write to a file descriptor.
  ///
  /// Note: This is similar to `writev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdWrite(Span<Span<const uint8_t>> IOVs,
                           __wasi_size_t &NWritten) const noexcept {
    if (!can(__WASI_RIGHTS_FD_WRITE)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Node.fdWrite(IOVs, NWritten);
  }

  /// Get the native handler.
  ///
  /// Note: Users should cast this native handler to corresponding types
  /// on different operating systems. E.g. int on POSIX or void * on Windows
  ///
  /// @return The native handler in uint64_t.
  WasiExpect<uint64_t> getNativeHandler() const noexcept {
    return Node.getNativeHandler();
  }

  /// Create a directory.
  ///
  /// Note: This is similar to `mkdirat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path at which to create the directory.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathCreateDirectory(VFS &FS,
                                              std::shared_ptr<VINode> Fd,
                                              std::string_view Path);

  /// Return the attributes of a file or directory.
  ///
  /// Note: This is similar to `stat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] Flags Flags determining the method of how the path is resolved.
  /// @param[out] Filestat The buffer where the file's attributes are stored.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathFilestatGet(VFS &FS, std::shared_ptr<VINode> Fd,
                                          std::string_view Path,
                                          __wasi_lookupflags_t Flags,
                                          __wasi_filestat_t &Filestat);

  /// Adjust the timestamps of a file or directory.
  ///
  /// Note: This is similar to `utimensat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] Flags Flags determining the method of how the path is resolved.
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  static WasiExpect<void>
  pathFilestatSetTimes(VFS &FS, std::shared_ptr<VINode> Fd,
                       std::string_view Path, __wasi_lookupflags_t Flags,
                       __wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                       __wasi_fstflags_t FstFlags);

  /// Create a hard link.
  ///
  /// Note: This is similar to `linkat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path from which to link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the hard link.
  /// @param[in] LookupFlags Flags determining the method of how the path is
  /// resolved.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathLink(VFS &FS, std::shared_ptr<VINode> Old,
                                   std::string_view OldPath,
                                   std::shared_ptr<VINode> New,
                                   std::string_view NewPath,
                                   __wasi_lookupflags_t LookupFlags);

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
  /// @param[in] FS The filesystem.
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
  static WasiExpect<std::shared_ptr<VINode>>
  pathOpen(VFS &FS, std::shared_ptr<VINode> Fd, std::string_view Path,
           __wasi_lookupflags_t LookupFlags, __wasi_oflags_t OpenFlags,
           __wasi_rights_t FsRightsBase, __wasi_rights_t FsRightsInheriting,
           __wasi_fdflags_t FdFlags);

  /// Read the contents of a symbolic link.
  ///
  /// Note: This is similar to `readlinkat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path of the symbolic link from which to read.
  /// @param[out] Buffer The buffer to which to write the contents of the
  /// symbolic link.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error.
  static WasiExpect<void> pathReadlink(VFS &FS, std::shared_ptr<VINode> Fd,
                                       std::string_view Path, Span<char> Buffer,
                                       __wasi_size_t &NRead);

  /// Remove a directory.
  ///
  /// Return `errno::notempty` if the directory is not empty.
  ///
  /// Note: This is similar to `unlinkat(fd, path, AT_REMOVEDIR)` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path to a directory to remove.
  /// @return Nothing or WASI error.
  static WasiExpect<void> pathRemoveDirectory(VFS &FS,
                                              std::shared_ptr<VINode> Fd,
                                              std::string_view Path);

  /// Rename a file or directory.
  ///
  /// Note: This is similar to `renameat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path of the file or directory to rename.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path to which to rename the file or
  /// directory.
  /// @return Nothing or WASI error.
  static WasiExpect<void> pathRename(VFS &FS, std::shared_ptr<VINode> Old,
                                     std::string_view OldPath,
                                     std::shared_ptr<VINode> New,
                                     std::string_view NewPath);

  /// Create a symbolic link.
  ///
  /// Note: This is similar to `symlinkat` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] OldPath The contents of the symbolic link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the symbolic
  /// link.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathSymlink(VFS &FS, std::string_view OldPath,
                                      std::shared_ptr<VINode> New,
                                      std::string_view NewPath);

  /// Unlink a file.
  ///
  /// Return `errno::isdir` if the path refers to a directory.
  ///
  /// Note: This is similar to `unlinkat(fd, path, 0)` in POSIX.
  ///
  /// @param[in] FS The filesystem.
  /// @param[in] Fd The working directory at which the resolution of the path
  /// starts.
  /// @param[in] Path The path to a file to unlink.
  /// @return Nothing or WASI error.
  static WasiExpect<void> pathUnlinkFile(VFS &FS, std::shared_ptr<VINode> Fd,
                                         std::string_view Path);

  /// Concurrently poll for the occurrence of a set of events.
  ///
  /// @param[in] NSubscriptions Both the number of subscriptions and events.
  /// @return Poll helper or WASI error.
  static inline WasiExpect<VPoller>
  pollOneoff(__wasi_size_t NSubscriptions) noexcept;

  /// Concurrently poll for the occurrence of a set of events in edge-triggered
  /// mode.
  ///
  /// @param[in] NSubscriptions Both the number of subscriptions and events.
  /// @param[in] Fd Epoll Descriptor.
  /// @return Poll helper or WASI error.
  static inline WasiExpect<VEpoller> epollOneoff(__wasi_size_t NSubscriptions,
                                                 int Fd) noexcept;

  static WasiExpect<void>
  getAddrinfo(std::string_view Node, std::string_view Service,
              const __wasi_addrinfo_t &Hint, uint32_t MaxResLength,
              Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
              Span<__wasi_sockaddr_t *> WasiSockaddrArray,
              Span<char *> AiAddrSaDataArray, Span<char *> AiCanonnameArray,
              /*Out*/ __wasi_size_t &ResLength) noexcept;

  static WasiExpect<std::shared_ptr<VINode>>
  sockOpen(VFS &FS, __wasi_address_family_t SysDomain,
           __wasi_sock_type_t SockType);

  WasiExpect<void> sockBind(uint8_t *Address, uint8_t AddressLength,
                            uint16_t Port) noexcept {
    return Node.sockBind(Address, AddressLength, Port);
  }

  WasiExpect<void> sockListen(int32_t Backlog) noexcept {
    return Node.sockListen(Backlog);
  }

  WasiExpect<std::shared_ptr<VINode>> sockAccept();

  WasiExpect<void> sockConnect(uint8_t *Address, uint8_t AddressLength,
                               uint16_t Port) noexcept {
    return Node.sockConnect(Address, AddressLength, Port);
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
  WasiExpect<void> sockRecv(Span<Span<uint8_t>> RiData,
                            __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                            __wasi_roflags_t &RoFlags) const noexcept {
    return Node.sockRecv(RiData, RiFlags, NRead, RoFlags);
  }

  /// Receive a message from a socket.
  ///
  /// Note: This is similar to `recvfrom` in POSIX, though it also supports
  /// reading the data into multiple buffers in the manner of `readv`.
  ///
  /// @param[in] RiData List of scatter/gather vectors to which to store data.
  /// @param[in] RiFlags Message flags.
  /// @param[in] Address Address of the target.
  /// @param[in] AddressLength The buffer size of Address.
  /// @param[out] NRead Return the number of bytes stored in RiData.
  /// @param[out] RoFlags Return message flags.
  /// @return Nothing or WASI error.
  WasiExpect<void> sockRecvFrom(Span<Span<uint8_t>> RiData,
                                __wasi_riflags_t RiFlags, uint8_t *Address,
                                uint8_t AddressLength, uint32_t *PortPtr,
                                __wasi_size_t &NRead,
                                __wasi_roflags_t &RoFlags) const noexcept {
    return Node.sockRecvFrom(RiData, RiFlags, Address, AddressLength, PortPtr,
                             NRead, RoFlags);
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
  WasiExpect<void> sockSend(Span<Span<const uint8_t>> SiData,
                            __wasi_siflags_t SiFlags,
                            __wasi_size_t &NWritten) const noexcept {
    return Node.sockSend(SiData, SiFlags, NWritten);
  }

  /// Send a message on a socket.
  ///
  /// Note: This is similar to `send` in POSIX, though it also supports writing
  /// the data from multiple buffers in the manner of `writev`.
  ///
  /// @param[in] SiData List of scatter/gather vectors to which to retrieve
  /// data.
  /// @param[in] SiFlags Message flags.
  /// @param[in] Address Address of the target.
  /// @param[in] AddressLength The buffer size of Address.
  /// @param[out] NWritten The number of bytes transmitted.
  /// @return Nothing or WASI error
  WasiExpect<void> sockSendTo(Span<Span<const uint8_t>> SiData,
                              __wasi_siflags_t SiFlags, uint8_t *Address,
                              uint8_t AddressLength, int32_t Port,
                              __wasi_size_t &NWritten) const noexcept {
    return Node.sockSendTo(SiData, SiFlags, Address, AddressLength, Port,
                           NWritten);
  }

  /// Shut down socket send and receive channels.
  ///
  /// Note: This is similar to `shutdown` in POSIX.
  ///
  /// @param[in] SdFlags Which channels on the socket to shut down.
  /// @return Nothing or WASI error
  WasiExpect<void> sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
    return Node.sockShutdown(SdFlags);
  }

  WasiExpect<void> sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName, void *FlagPtr,
                              uint32_t *FlagSizePtr) const noexcept {
    return Node.sockGetOpt(SockOptLevel, SockOptName, FlagPtr, FlagSizePtr);
  }

  WasiExpect<void> sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                              __wasi_sock_opt_so_t SockOptName, void *FlagPtr,
                              uint32_t FlagSizePtr) const noexcept {
    return Node.sockSetOpt(SockOptLevel, SockOptName, FlagPtr, FlagSizePtr);
  }

  WasiExpect<void> sockGetLocalAddr(uint8_t *Address,
                                    uint32_t *PortPtr) const noexcept {
    return Node.sockGetLocalAddr(Address, PortPtr);
  }

  WasiExpect<void> sockGetPeerAddr(uint8_t *Address,
                                   uint32_t *PortPtr) const noexcept {
    return Node.sockGetPeerAddr(Address, PortPtr);
  }

  __wasi_rights_t fsRightsBase() const noexcept { return FsRightsBase; }

  __wasi_rights_t fsRightsInheriting() const noexcept {
    return FsRightsInheriting;
  }

  /// Check if this vinode is a directory.
  bool isDirectory() const noexcept { return Node.isDirectory(); }

  /// Check if current user has execute permission on this vinode directory.
  bool canBrowse() const noexcept { return Node.canBrowse(); }

  /// Check if this vinode is a symbolic link.
  bool isSymlink() const noexcept { return Node.isSymlink(); }

  static constexpr __wasi_rights_t imply(__wasi_rights_t Rights) noexcept {
    if (Rights & __WASI_RIGHTS_FD_SEEK) {
      Rights |= __WASI_RIGHTS_FD_TELL;
    }
    if (Rights & __WASI_RIGHTS_FD_SYNC) {
      Rights |= __WASI_RIGHTS_FD_DATASYNC;
    }
    return Rights;
  }

  constexpr bool can(__wasi_rights_t RequiredRights,
                     __wasi_rights_t RequiredInheritingRights =
                         static_cast<__wasi_rights_t>(0)) const noexcept {
    const auto Base = imply(FsRightsBase);
    const auto Inheriting = imply(FsRightsInheriting);
    return (Base & RequiredRights) == RequiredRights &&
           (Inheriting & RequiredInheritingRights) == RequiredInheritingRights;
  }

private:
  std::reference_wrapper<VFS> FS;
  INode Node;
  __wasi_rights_t FsRightsBase;
  __wasi_rights_t FsRightsInheriting;
  std::shared_ptr<VINode> Parent;
  std::string Name;

  friend class VPoller;
  friend class VEpoller;

  /// Open path without resolve.
  /// @param Path Path, contains one element only.
  /// @param OpenFlags WASI open flags.
  /// @return VINode found, or WASI error.
  WasiExpect<std::shared_ptr<VINode>> directOpen(std::string_view Path,
                                                 __wasi_oflags_t OpenFlags,
                                                 __wasi_fdflags_t FdFlags,
                                                 uint8_t VFSFlags);

  /// Resolve path until last element.
  /// @param[in] FS Filesystem.
  /// @param[in,out] Fd Fd. Return parent of last part if found.
  /// @param[in,out] Path path. Return last part of path if found.
  /// @param[in] LookupFlags WASI lookup flags.
  /// @param[in] VFSFlags Internal lookup flags.
  /// @param[in] LinkCount Counting symbolic link lookup times.
  /// @return Allocated buffer, or WASI error.
  static WasiExpect<std::vector<char>> resolvePath(
      VFS &FS, std::shared_ptr<VINode> &Fd, std::string_view &Path,
      __wasi_lookupflags_t LookupFlags = __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW,
      uint8_t VFSFlags = 0, uint8_t LinkCount = 0);
};

class VPoller : private Poller {
public:
  using Poller::clock;
  using Poller::wait;

  VPoller(Poller &&P) : Poller(std::move(P)) {}

  WasiExpect<void> read(std::shared_ptr<VINode> Fd,
                        __wasi_userdata_t UserData) noexcept {
    if (!Fd->can(__WASI_RIGHTS_POLL_FD_READWRITE) &&
        !Fd->can(__WASI_RIGHTS_FD_READ)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Poller::read(Fd->Node, UserData);
  }

  WasiExpect<void> write(std::shared_ptr<VINode> Fd,
                         __wasi_userdata_t UserData) noexcept {
    return Poller::write(Fd->Node, UserData);
  }
};

class VEpoller : private Epoller {
public:
  using Epoller::CallbackType;
  using Epoller::clock;
  using Epoller::getFd;
  using Epoller::wait;

  VEpoller(Epoller &&P) : Epoller(std::move(P)) {}

  WasiExpect<void>
  read(std::shared_ptr<VINode> Fd, __wasi_userdata_t UserData,
       std::unordered_map<int, uint32_t> &Registration) noexcept {
    if (!Fd->can(__WASI_RIGHTS_POLL_FD_READWRITE) &&
        !Fd->can(__WASI_RIGHTS_FD_READ)) {
      return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
    }
    return Epoller::read(Fd->Node, UserData, Registration);
  }

  WasiExpect<void>
  write(std::shared_ptr<VINode> Fd, __wasi_userdata_t UserData,
        std::unordered_map<int, uint32_t> &Registration) noexcept {
    return Epoller::write(Fd->Node, UserData, Registration);
  }
  WasiExpect<void>
  wait(CallbackType Callback,
       std::unordered_map<int, uint32_t> &Registration) noexcept {
    return Epoller::wait(Callback, Registration);
  }
  int getFd() noexcept { return Epoller::getFd(); }
};

inline WasiExpect<VPoller>
VINode::pollOneoff(__wasi_size_t NSubscriptions) noexcept {
  return INode::pollOneoff(NSubscriptions).map([](Poller &&P) {
    return VPoller(std::move(P));
  });
}

inline WasiExpect<VEpoller> VINode::epollOneoff(__wasi_size_t NSubscriptions,
                                                int Fd) noexcept {
  return INode::epollOneoff(NSubscriptions, Fd).map([](Epoller &&P) {
    return VEpoller(std::move(P));
  });
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
