// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

/**
 * THIS FILE IS AUTO-GENERATED from the following files:
 *   typenames.witx
 *
 * @file
 * This file describes the [WASI] interface, consisting of functions, types,
 * and defined values (macros).
 *
 * The interface described here is greatly inspired by [CloudABI]'s clean,
 * thoughtfully-designed, capability-oriented, POSIX-style API.
 *
 * [CloudABI]: https://github.com/NuxiNL/cloudlibc
 * [WASI]: https://github.com/WebAssembly/WASI/
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

using const_uint8_t_ptr = uint32_t;
using uint8_t_ptr = uint32_t;

#define DEFINE_ENUM_OPERATORS(type)                                            \
  inline constexpr type operator~(type a) noexcept {                           \
    return static_cast<type>(~static_cast<std::underlying_type_t<type>>(a));   \
  }                                                                            \
  inline constexpr type operator|(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) |    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator|=(type &a, type b) noexcept {                \
    a = a | b;                                                                 \
    return a;                                                                  \
  }                                                                            \
  inline constexpr type operator&(type a, type b) noexcept {                   \
    return static_cast<type>(static_cast<std::underlying_type_t<type>>(a) &    \
                             static_cast<std::underlying_type_t<type>>(b));    \
  }                                                                            \
  inline constexpr type &operator&=(type &a, type b) noexcept {                \
    a = a & b;                                                                 \
    return a;                                                                  \
  }

static_assert(alignof(int8_t) == 1, "non-wasi data layout");
static_assert(alignof(uint8_t) == 1, "non-wasi data layout");
static_assert(alignof(int16_t) == 2, "non-wasi data layout");
static_assert(alignof(uint16_t) == 2, "non-wasi data layout");
static_assert(alignof(int32_t) == 4, "non-wasi data layout");
static_assert(alignof(uint32_t) == 4, "non-wasi data layout");
static_assert(alignof(int64_t) == 8, "non-wasi data layout");
static_assert(alignof(uint64_t) == 8, "non-wasi data layout");
static_assert(alignof(const_uint8_t_ptr) == 4, "non-wasi data layout");
static_assert(alignof(uint8_t_ptr) == 4, "non-wasi data layout");

using __wasi_size_t = uint32_t;

static_assert(sizeof(__wasi_size_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_size_t) == 4, "witx calculated align");

/**
 * Non-negative file size or length of a region within a file.
 */
using __wasi_filesize_t = uint64_t;

static_assert(sizeof(__wasi_filesize_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_filesize_t) == 8, "witx calculated align");

/**
 * Timestamp in nanoseconds.
 */
using __wasi_timestamp_t = uint64_t;

static_assert(sizeof(__wasi_timestamp_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_timestamp_t) == 8, "witx calculated align");

/**
 * Identifiers for clocks.
 */
enum __wasi_clockid_t : uint32_t {
  /**
   * The clock measuring real time. Time value zero corresponds with
   * 1970-01-01T00:00:00Z.
   */
  __WASI_CLOCKID_REALTIME = 0,

  /**
   * The store-wide monotonic clock, which is defined as a clock measuring
   * real time, whose value cannot be adjusted and which cannot have negative
   * clock jumps. The epoch of this clock is undefined. The absolute time
   * value of this clock therefore has no meaning.
   */
  __WASI_CLOCKID_MONOTONIC = 1,

  /**
   * The CPU-time clock associated with the current process.
   */
  __WASI_CLOCKID_PROCESS_CPUTIME_ID = 2,

  /**
   * The CPU-time clock associated with the current thread.
   */
  __WASI_CLOCKID_THREAD_CPUTIME_ID = 3,

};
static_assert(sizeof(__wasi_clockid_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_clockid_t) == 4, "witx calculated align");

/**
 * Error codes returned by functions.
 * Not all of these error codes are returned by the functions provided by this
 * API; some are used in higher-level library layers, and others are provided
 * merely for alignment with POSIX.
 */
enum __wasi_errno_t : uint16_t {
  /**
   * No error occurred. System call completed successfully.
   */
  __WASI_ERRNO_SUCCESS = 0,

  /**
   * Argument list too long.
   */
  __WASI_ERRNO_2BIG = 1,

  /**
   * Permission denied.
   */
  __WASI_ERRNO_ACCES = 2,

  /**
   * Address in use.
   */
  __WASI_ERRNO_ADDRINUSE = 3,

  /**
   * Address not available.
   */
  __WASI_ERRNO_ADDRNOTAVAIL = 4,

  /**
   * Address family not supported.
   */
  __WASI_ERRNO_AFNOSUPPORT = 5,

  /**
   * Resource unavailable, or operation would block.
   */
  __WASI_ERRNO_AGAIN = 6,

  /**
   * Connection already in progress.
   */
  __WASI_ERRNO_ALREADY = 7,

  /**
   * Bad file descriptor.
   */
  __WASI_ERRNO_BADF = 8,

  /**
   * Bad message.
   */
  __WASI_ERRNO_BADMSG = 9,

  /**
   * Device or resource busy.
   */
  __WASI_ERRNO_BUSY = 10,

  /**
   * Operation canceled.
   */
  __WASI_ERRNO_CANCELED = 11,

  /**
   * No child processes.
   */
  __WASI_ERRNO_CHILD = 12,

  /**
   * Connection aborted.
   */
  __WASI_ERRNO_CONNABORTED = 13,

  /**
   * Connection refused.
   */
  __WASI_ERRNO_CONNREFUSED = 14,

  /**
   * Connection reset.
   */
  __WASI_ERRNO_CONNRESET = 15,

  /**
   * Resource deadlock would occur.
   */
  __WASI_ERRNO_DEADLK = 16,

  /**
   * Destination address required.
   */
  __WASI_ERRNO_DESTADDRREQ = 17,

  /**
   * Mathematics argument out of domain of function.
   */
  __WASI_ERRNO_DOM = 18,

  /**
   * Reserved.
   */
  __WASI_ERRNO_DQUOT = 19,

  /**
   * File exists.
   */
  __WASI_ERRNO_EXIST = 20,

  /**
   * Bad address.
   */
  __WASI_ERRNO_FAULT = 21,

  /**
   * File too large.
   */
  __WASI_ERRNO_FBIG = 22,

  /**
   * Host is unreachable.
   */
  __WASI_ERRNO_HOSTUNREACH = 23,

  /**
   * Identifier removed.
   */
  __WASI_ERRNO_IDRM = 24,

  /**
   * Illegal byte sequence.
   */
  __WASI_ERRNO_ILSEQ = 25,

  /**
   * Operation in progress.
   */
  __WASI_ERRNO_INPROGRESS = 26,

  /**
   * Interrupted function.
   */
  __WASI_ERRNO_INTR = 27,

  /**
   * Invalid argument.
   */
  __WASI_ERRNO_INVAL = 28,

  /**
   * I/O error.
   */
  __WASI_ERRNO_IO = 29,

  /**
   * Socket is connected.
   */
  __WASI_ERRNO_ISCONN = 30,

  /**
   * Is a directory.
   */
  __WASI_ERRNO_ISDIR = 31,

  /**
   * Too many levels of symbolic links.
   */
  __WASI_ERRNO_LOOP = 32,

  /**
   * File descriptor value too large.
   */
  __WASI_ERRNO_MFILE = 33,

  /**
   * Too many links.
   */
  __WASI_ERRNO_MLINK = 34,

  /**
   * Message too large.
   */
  __WASI_ERRNO_MSGSIZE = 35,

  /**
   * Reserved.
   */
  __WASI_ERRNO_MULTIHOP = 36,

  /**
   * Filename too long.
   */
  __WASI_ERRNO_NAMETOOLONG = 37,

  /**
   * Network is down.
   */
  __WASI_ERRNO_NETDOWN = 38,

  /**
   * Connection aborted by network.
   */
  __WASI_ERRNO_NETRESET = 39,

  /**
   * Network unreachable.
   */
  __WASI_ERRNO_NETUNREACH = 40,

  /**
   * Too many files open in system.
   */
  __WASI_ERRNO_NFILE = 41,

  /**
   * No buffer space available.
   */
  __WASI_ERRNO_NOBUFS = 42,

  /**
   * No such device.
   */
  __WASI_ERRNO_NODEV = 43,

  /**
   * No such file or directory.
   */
  __WASI_ERRNO_NOENT = 44,

  /**
   * Executable file format error.
   */
  __WASI_ERRNO_NOEXEC = 45,

  /**
   * No locks available.
   */
  __WASI_ERRNO_NOLCK = 46,

  /**
   * Reserved.
   */
  __WASI_ERRNO_NOLINK = 47,

  /**
   * Not enough space.
   */
  __WASI_ERRNO_NOMEM = 48,

  /**
   * No message of the desired type.
   */
  __WASI_ERRNO_NOMSG = 49,

  /**
   * Protocol not available.
   */
  __WASI_ERRNO_NOPROTOOPT = 50,

  /**
   * No space left on device.
   */
  __WASI_ERRNO_NOSPC = 51,

  /**
   * Function not supported.
   */
  __WASI_ERRNO_NOSYS = 52,

  /**
   * The socket is not connected.
   */
  __WASI_ERRNO_NOTCONN = 53,

  /**
   * Not a directory or a symbolic link to a directory.
   */
  __WASI_ERRNO_NOTDIR = 54,

  /**
   * Directory not empty.
   */
  __WASI_ERRNO_NOTEMPTY = 55,

  /**
   * State not recoverable.
   */
  __WASI_ERRNO_NOTRECOVERABLE = 56,

  /**
   * Not a socket.
   */
  __WASI_ERRNO_NOTSOCK = 57,

  /**
   * Not supported, or operation not supported on socket.
   */
  __WASI_ERRNO_NOTSUP = 58,

  /**
   * Inappropriate I/O control operation.
   */
  __WASI_ERRNO_NOTTY = 59,

  /**
   * No such device or address.
   */
  __WASI_ERRNO_NXIO = 60,

  /**
   * Value too large to be stored in data type.
   */
  __WASI_ERRNO_OVERFLOW = 61,

  /**
   * Previous owner died.
   */
  __WASI_ERRNO_OWNERDEAD = 62,

  /**
   * Operation not permitted.
   */
  __WASI_ERRNO_PERM = 63,

  /**
   * Broken pipe.
   */
  __WASI_ERRNO_PIPE = 64,

  /**
   * Protocol error.
   */
  __WASI_ERRNO_PROTO = 65,

  /**
   * Protocol not supported.
   */
  __WASI_ERRNO_PROTONOSUPPORT = 66,

  /**
   * Protocol wrong type for socket.
   */
  __WASI_ERRNO_PROTOTYPE = 67,

  /**
   * Result too large.
   */
  __WASI_ERRNO_RANGE = 68,

  /**
   * Read-only file system.
   */
  __WASI_ERRNO_ROFS = 69,

  /**
   * Invalid seek.
   */
  __WASI_ERRNO_SPIPE = 70,

  /**
   * No such process.
   */
  __WASI_ERRNO_SRCH = 71,

  /**
   * Reserved.
   */
  __WASI_ERRNO_STALE = 72,

  /**
   * Connection timed out.
   */
  __WASI_ERRNO_TIMEDOUT = 73,

  /**
   * Text file busy.
   */
  __WASI_ERRNO_TXTBSY = 74,

  /**
   * Cross-device link.
   */
  __WASI_ERRNO_XDEV = 75,

  /**
   * Extension: Capabilities insufficient.
   */
  __WASI_ERRNO_NOTCAPABLE = 76,

  /**
   * The specified network host does not have any network addresses in the
   * requested address family.
   */
  __WASI_ERRNO_AIADDRFAMILY = 77,

  /**
   * Try again later.
   */
  __WASI_ERRNO_AIAGAIN = 78,

  /**
   * Hints.ai_flags contains invalid flags
   */
  __WASI_ERRNO_AIBADFLAG = 79,

  /**
   * The name server returned a permanent failure indication.
   */
  __WASI_ERRNO_AIFAIL = 80,

  /**
   * The requested address family is not supported.
   */
  __WASI_ERRNO_AIFAMILY = 81,

  /**
   * Addrinfo out of memory.
   */
  __WASI_ERRNO_AIMEMORY = 82,

  /**
   * Network host exists, but does not have any network addresses defined.
   */
  __WASI_ERRNO_AINODATA = 83,

  /**
   * Node or service is not known; or both node and service are NULL
   */
  __WASI_ERRNO_AINONAME = 84,

  /**
   * Service is not available for the requested socket type.
   */
  __WASI_ERRNO_AISERVICE = 85,

  /**
   * The requested socket type is not supported.
   */
  __WASI_ERRNO_AISOCKTYPE = 86,

  /**
   * Other system error;
   */
  __WASI_ERRNO_AISYSTEM = 87,

};
static_assert(sizeof(__wasi_errno_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_errno_t) == 2, "witx calculated align");

/**
 * File descriptor rights, determining which actions may be performed.
 */
enum __wasi_rights_t : uint64_t {

  /**
   * The right to invoke `fd_datasync`.
   * If `path_open` is set, includes the right to invoke
   * `path_open` with `fdflags::dsync`.
   */
  __WASI_RIGHTS_FD_DATASYNC = 1ULL << 0,

  /**
   * The right to invoke `fd_read` and `sock_recv`.
   * If `rights::fd_seek` is set, includes the right to invoke `fd_pread`.
   */
  __WASI_RIGHTS_FD_READ = 1ULL << 1,

  /**
   * The right to invoke `fd_seek`. This flag implies `rights::fd_tell`.
   */
  __WASI_RIGHTS_FD_SEEK = 1ULL << 2,

  /**
   * The right to invoke `fd_fdstat_set_flags`.
   */
  __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS = 1ULL << 3,

  /**
   * The right to invoke `fd_sync`.
   * If `path_open` is set, includes the right to invoke
   * `path_open` with `fdflags::rsync` and `fdflags::dsync`.
   */
  __WASI_RIGHTS_FD_SYNC = 1ULL << 4,

  /**
   * The right to invoke `fd_seek` in such a way that the file offset
   * remains unaltered (i.e., `whence::cur` with offset zero), or to
   * invoke `fd_tell`.
   */
  __WASI_RIGHTS_FD_TELL = 1ULL << 5,

  /**
   * The right to invoke `fd_write` and `sock_send`.
   * If `rights::fd_seek` is set, includes the right to invoke `fd_pwrite`.
   */
  __WASI_RIGHTS_FD_WRITE = 1ULL << 6,

  /**
   * The right to invoke `fd_advise`.
   */
  __WASI_RIGHTS_FD_ADVISE = 1ULL << 7,

  /**
   * The right to invoke `fd_allocate`.
   */
  __WASI_RIGHTS_FD_ALLOCATE = 1ULL << 8,

  /**
   * The right to invoke `path_create_directory`.
   */
  __WASI_RIGHTS_PATH_CREATE_DIRECTORY = 1ULL << 9,

  /**
   * If `path_open` is set, the right to invoke `path_open` with
   * `oflags::creat`.
   */
  __WASI_RIGHTS_PATH_CREATE_FILE = 1ULL << 10,

  /**
   * The right to invoke `path_link` with the file descriptor as the
   * source directory.
   */
  __WASI_RIGHTS_PATH_LINK_SOURCE = 1ULL << 11,

  /**
   * The right to invoke `path_link` with the file descriptor as the
   * target directory.
   */
  __WASI_RIGHTS_PATH_LINK_TARGET = 1ULL << 12,

  /**
   * The right to invoke `path_open`.
   */
  __WASI_RIGHTS_PATH_OPEN = 1ULL << 13,

  /**
   * The right to invoke `fd_readdir`.
   */
  __WASI_RIGHTS_FD_READDIR = 1ULL << 14,

  /**
   * The right to invoke `path_readlink`.
   */
  __WASI_RIGHTS_PATH_READLINK = 1ULL << 15,

  /**
   * The right to invoke `path_rename` with the file descriptor as the source
   * directory.
   */
  __WASI_RIGHTS_PATH_RENAME_SOURCE = 1ULL << 16,

  /**
   * The right to invoke `path_rename` with the file descriptor as the target
   * directory.
   */
  __WASI_RIGHTS_PATH_RENAME_TARGET = 1ULL << 17,

  /**
   * The right to invoke `path_filestat_get`.
   */
  __WASI_RIGHTS_PATH_FILESTAT_GET = 1ULL << 18,

  /**
   * The right to change a file's size (there is no `path_filestat_set_size`).
   * If `path_open` is set, includes the right to invoke `path_open` with
   * `oflags::trunc`.
   */
  __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE = 1ULL << 19,

  /**
   * The right to invoke `path_filestat_set_times`.
   */
  __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES = 1ULL << 20,

  /**
   * The right to invoke `fd_filestat_get`.
   */
  __WASI_RIGHTS_FD_FILESTAT_GET = 1ULL << 21,

  /**
   * The right to invoke `fd_filestat_set_size`.
   */
  __WASI_RIGHTS_FD_FILESTAT_SET_SIZE = 1ULL << 22,

  /**
   * The right to invoke `fd_filestat_set_times`.
   */
  __WASI_RIGHTS_FD_FILESTAT_SET_TIMES = 1ULL << 23,

  /**
   * The right to invoke `path_symlink`.
   */
  __WASI_RIGHTS_PATH_SYMLINK = 1ULL << 24,

  /**
   * The right to invoke `path_remove_directory`.
   */
  __WASI_RIGHTS_PATH_REMOVE_DIRECTORY = 1ULL << 25,

  /**
   * The right to invoke `path_unlink_file`.
   */
  __WASI_RIGHTS_PATH_UNLINK_FILE = 1ULL << 26,

  /**
   * If `rights::fd_read` is set, includes the right to invoke `poll_oneoff` to
   * subscribe to `eventtype::fd_read`. If `rights::fd_write` is set, includes
   * the right to invoke `poll_oneoff` to subscribe to `eventtype::fd_write`.
   */
  __WASI_RIGHTS_POLL_FD_READWRITE = 1ULL << 27,

  /**
   * The right to invoke `sock_shutdown`.
   */
  __WASI_RIGHTS_SOCK_SHUTDOWN = 1ULL << 28,

  /**
   * The right to invoke `sock_open`.
   */
  __WASI_RIGHTS_SOCK_OPEN = 1ULL << 29,

  /**
   * The right to invoke `sock_close`.
   */
  __WASI_RIGHTS_SOCK_CLOSE = 1ULL << 30,

  /**
   * The right to invoke `sock_bind`.
   */
  __WASI_RIGHTS_SOCK_BIND = 1ULL << 31,

  /**
   * The right to invoke `sock_recv`.
   */
  __WASI_RIGHTS_SOCK_RECV = 1ULL << 32,

  /**
   * The right to invoke `sock_recv_from`.
   */
  __WASI_RIGHTS_SOCK_RECV_FROM = 1ULL << 33,

  /**
   * The right to invoke `sock_send`.
   */
  __WASI_RIGHTS_SOCK_SEND = 1ULL << 34,

  /**
   * The right to invoke `sock_send_to`.
   */
  __WASI_RIGHTS_SOCK_SEND_TO = 1ULL << 35,

};
DEFINE_ENUM_OPERATORS(__wasi_rights_t)

static_assert(sizeof(__wasi_rights_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_rights_t) == 8, "witx calculated align");

/**
 * A file descriptor handle.
 */
using __wasi_fd_t = int32_t;

static_assert(sizeof(__wasi_fd_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_fd_t) == 4, "witx calculated align");

/**
 * A socket descriptor is currently an alias type of a typical file descriptor.
 */
using __wasi_sock_d_t = __wasi_fd_t;

static_assert(sizeof(__wasi_sock_d_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_sock_d_t) == 4, "witx calculated align");

/**
 * A region of memory for scatter/gather reads.
 */
struct __wasi_iovec_t {
  /**
   * The address of the buffer to be filled.
   */
  uint8_t_ptr buf;

  /**
   * The length of the buffer to be filled.
   */
  __wasi_size_t buf_len;
};

static_assert(sizeof(__wasi_iovec_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_iovec_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_iovec_t, buf) == 0, "witx calculated offset");
static_assert(offsetof(__wasi_iovec_t, buf_len) == 4, "witx calculated offset");

/**
 * A region of memory for scatter/gather writes.
 */
struct __wasi_ciovec_t {
  /**
   * The address of the buffer to be written.
   */
  const_uint8_t_ptr buf;

  /**
   * The length of the buffer to be written.
   */
  __wasi_size_t buf_len;
};

static_assert(sizeof(__wasi_ciovec_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_ciovec_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_ciovec_t, buf) == 0, "witx calculated offset");
static_assert(offsetof(__wasi_ciovec_t, buf_len) == 4,
              "witx calculated offset");

/**
 * Relative offset within a file.
 */
using __wasi_filedelta_t = int64_t;

static_assert(sizeof(__wasi_filedelta_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_filedelta_t) == 8, "witx calculated align");

/**
 * The position relative to which to set the offset of the file descriptor.
 */
enum __wasi_whence_t : uint8_t {
  /**
   * Seek relative to start-of-file.
   */
  __WASI_WHENCE_SET = 0,

  /**
   * Seek relative to current position.
   */
  __WASI_WHENCE_CUR = 1,

  /**
   * Seek relative to end-of-file.
   */
  __WASI_WHENCE_END = 2,

};
static_assert(sizeof(__wasi_whence_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_whence_t) == 1, "witx calculated align");

/**
 * A reference to the offset of a directory entry.
 *
 * The value 0 signifies the start of the directory.
 */
using __wasi_dircookie_t = uint64_t;

static_assert(sizeof(__wasi_dircookie_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_dircookie_t) == 8, "witx calculated align");

/**
 * The type for the `dirent::d_namlen` field of `dirent` struct.
 */
using __wasi_dirnamlen_t = uint32_t;

static_assert(sizeof(__wasi_dirnamlen_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_dirnamlen_t) == 4, "witx calculated align");

/**
 * File serial number that is unique within its file system.
 */
using __wasi_inode_t = uint64_t;

static_assert(sizeof(__wasi_inode_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_inode_t) == 8, "witx calculated align");

/**
 * The type of a file descriptor or file.
 */
enum __wasi_filetype_t : uint8_t {
  /**
   * The type of the file descriptor or file is unknown or is different from any
   * of the other types specified.
   */
  __WASI_FILETYPE_UNKNOWN = 0,

  /**
   * The file descriptor or file refers to a block device inode.
   */
  __WASI_FILETYPE_BLOCK_DEVICE = 1,

  /**
   * The file descriptor or file refers to a character device inode.
   */
  __WASI_FILETYPE_CHARACTER_DEVICE = 2,

  /**
   * The file descriptor or file refers to a directory inode.
   */
  __WASI_FILETYPE_DIRECTORY = 3,

  /**
   * The file descriptor or file refers to a regular file inode.
   */
  __WASI_FILETYPE_REGULAR_FILE = 4,

  /**
   * The file descriptor or file refers to a datagram socket.
   */
  __WASI_FILETYPE_SOCKET_DGRAM = 5,

  /**
   * The file descriptor or file refers to a byte-stream socket.
   */
  __WASI_FILETYPE_SOCKET_STREAM = 6,

  /**
   * The file refers to a symbolic link inode.
   */
  __WASI_FILETYPE_SYMBOLIC_LINK = 7,

};
static_assert(sizeof(__wasi_filetype_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_filetype_t) == 1, "witx calculated align");

/**
 * A directory entry.
 */
struct __wasi_dirent_t {
  /**
   * The offset of the next directory entry stored in this directory.
   */
  __wasi_dircookie_t d_next;

  /**
   * The serial number of the file referred to by this directory entry.
   */
  __wasi_inode_t d_ino;

  /**
   * The length of the name of the directory entry.
   */
  __wasi_dirnamlen_t d_namlen;

  /**
   * The type of the file referred to by this directory entry.
   */
  __wasi_filetype_t d_type;
};

static_assert(sizeof(__wasi_dirent_t) == 24, "witx calculated size");
static_assert(alignof(__wasi_dirent_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_dirent_t, d_next) == 0, "witx calculated offset");
static_assert(offsetof(__wasi_dirent_t, d_ino) == 8, "witx calculated offset");
static_assert(offsetof(__wasi_dirent_t, d_namlen) == 16,
              "witx calculated offset");
static_assert(offsetof(__wasi_dirent_t, d_type) == 20,
              "witx calculated offset");

/**
 * File or memory access pattern advisory information.
 */
enum __wasi_advice_t : uint8_t {
  /**
   * The application has no advice to give on its behavior with respect to the
   * specified data.
   */
  __WASI_ADVICE_NORMAL = 0,

  /**
   * The application expects to access the specified data sequentially from
   * lower offsets to higher offsets.
   */
  __WASI_ADVICE_SEQUENTIAL = 1,

  /**
   * The application expects to access the specified data in a random order.
   */
  __WASI_ADVICE_RANDOM = 2,

  /**
   * The application expects to access the specified data in the near future.
   */
  __WASI_ADVICE_WILLNEED = 3,

  /**
   * The application expects that it will not access the specified data in the
   * near future.
   */
  __WASI_ADVICE_DONTNEED = 4,

  /**
   * The application expects to access the specified data once and then not
   * reuse it thereafter.
   */
  __WASI_ADVICE_NOREUSE = 5,

};
static_assert(sizeof(__wasi_advice_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_advice_t) == 1, "witx calculated align");

/**
 * File descriptor flags.
 */
enum __wasi_fdflags_t : uint16_t {

  /**
   * Append mode: Data written to the file is always appended to the file's end.
   */
  __WASI_FDFLAGS_APPEND = 1ULL << 0,

  /**
   * Write according to synchronized I/O data integrity completion. Only the
   * data stored in the file is synchronized.
   */
  __WASI_FDFLAGS_DSYNC = 1ULL << 1,

  /**
   * Non-blocking mode.
   */
  __WASI_FDFLAGS_NONBLOCK = 1ULL << 2,

  /**
   * Synchronized read I/O operations.
   */
  __WASI_FDFLAGS_RSYNC = 1ULL << 3,

  /**
   * Write according to synchronized I/O file integrity completion. In
   * addition to synchronizing the data stored in the file, the implementation
   * may also synchronously update the file's metadata.
   */
  __WASI_FDFLAGS_SYNC = 1ULL << 4,

};
DEFINE_ENUM_OPERATORS(__wasi_fdflags_t)

static_assert(sizeof(__wasi_fdflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_fdflags_t) == 2, "witx calculated align");

/**
 * File descriptor attributes.
 */
struct __wasi_fdstat_t {
  /**
   * File type.
   */
  __wasi_filetype_t fs_filetype;

  /**
   * File descriptor flags.
   */
  __wasi_fdflags_t fs_flags;

  /**
   * Rights that apply to this file descriptor.
   */
  __wasi_rights_t fs_rights_base;

  /**
   * Maximum set of rights that may be installed on new file descriptors that
   * are created through this file descriptor, e.g., through `path_open`.
   */
  __wasi_rights_t fs_rights_inheriting;
};

static_assert(sizeof(__wasi_fdstat_t) == 24, "witx calculated size");
static_assert(alignof(__wasi_fdstat_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_fdstat_t, fs_filetype) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_fdstat_t, fs_flags) == 2,
              "witx calculated offset");
static_assert(offsetof(__wasi_fdstat_t, fs_rights_base) == 8,
              "witx calculated offset");
static_assert(offsetof(__wasi_fdstat_t, fs_rights_inheriting) == 16,
              "witx calculated offset");

/**
 * Identifier for a device containing a file system. Can be used in combination
 * with `inode` to uniquely identify a file or directory in the filesystem.
 */
using __wasi_device_t = uint64_t;

static_assert(sizeof(__wasi_device_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_device_t) == 8, "witx calculated align");

/**
 * Which file time attributes to adjust.
 */
enum __wasi_fstflags_t : uint16_t {

  /**
   * Adjust the last data access timestamp to the value stored in
   * `filestat::atim`.
   */
  __WASI_FSTFLAGS_ATIM = 1ULL << 0,

  /**
   * Adjust the last data access timestamp to the time of clock
   * `clockid::realtime`.
   */
  __WASI_FSTFLAGS_ATIM_NOW = 1ULL << 1,

  /**
   * Adjust the last data modification timestamp to the value stored in
   * `filestat::mtim`.
   */
  __WASI_FSTFLAGS_MTIM = 1ULL << 2,

  /**
   * Adjust the last data modification timestamp to the time of clock
   * `clockid::realtime`.
   */
  __WASI_FSTFLAGS_MTIM_NOW = 1ULL << 3,

};
DEFINE_ENUM_OPERATORS(__wasi_fstflags_t)

static_assert(sizeof(__wasi_fstflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_fstflags_t) == 2, "witx calculated align");

/**
 * Flags determining the method of how paths are resolved.
 */
enum __wasi_lookupflags_t : uint32_t {

  /**
   * As long as the resolved path corresponds to a symbolic link, it is
   * expanded.
   */
  __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW = 1ULL << 0,

};
DEFINE_ENUM_OPERATORS(__wasi_lookupflags_t)

static_assert(sizeof(__wasi_lookupflags_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_lookupflags_t) == 4, "witx calculated align");

/**
 * Open flags used by `path_open`.
 */
enum __wasi_oflags_t : uint16_t {

  /**
   * Create file if it does not exist.
   */
  __WASI_OFLAGS_CREAT = 1ULL << 0,

  /**
   * Fail if not a directory.
   */
  __WASI_OFLAGS_DIRECTORY = 1ULL << 1,

  /**
   * Fail if file already exists.
   */
  __WASI_OFLAGS_EXCL = 1ULL << 2,

  /**
   * Truncate file to size 0.
   */
  __WASI_OFLAGS_TRUNC = 1ULL << 3,

};
DEFINE_ENUM_OPERATORS(__wasi_oflags_t)

static_assert(sizeof(__wasi_oflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_oflags_t) == 2, "witx calculated align");

/**
 * Number of hard links to an inode.
 */
using __wasi_linkcount_t = uint64_t;

static_assert(sizeof(__wasi_linkcount_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_linkcount_t) == 8, "witx calculated align");

/**
 * File attributes.
 */
struct __wasi_filestat_t {
  /**
   * Device ID of device containing the file.
   */
  __wasi_device_t dev;

  /**
   * File serial number.
   */
  __wasi_inode_t ino;

  /**
   * File type.
   */
  __wasi_filetype_t filetype;

  /**
   * Number of hard links to the file.
   */
  __wasi_linkcount_t nlink;

  /**
   * For regular files, the file size in bytes. For symbolic links, the length
   * in bytes of the pathname contained in the symbolic link.
   */
  __wasi_filesize_t size;

  /**
   * Last data access timestamp.
   */
  __wasi_timestamp_t atim;

  /**
   * Last data modification timestamp.
   */
  __wasi_timestamp_t mtim;

  /**
   * Last file status change timestamp.
   */
  __wasi_timestamp_t ctim;
};

static_assert(sizeof(__wasi_filestat_t) == 64, "witx calculated size");
static_assert(alignof(__wasi_filestat_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_filestat_t, dev) == 0, "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, ino) == 8, "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, filetype) == 16,
              "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, nlink) == 24,
              "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, size) == 32,
              "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, atim) == 40,
              "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, mtim) == 48,
              "witx calculated offset");
static_assert(offsetof(__wasi_filestat_t, ctim) == 56,
              "witx calculated offset");

/**
 * User-provided value that may be attached to objects that is retained when
 * extracted from the implementation.
 */
using __wasi_userdata_t = uint64_t;

static_assert(sizeof(__wasi_userdata_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_userdata_t) == 8, "witx calculated align");

/**
 * Type of a subscription to an event or its occurrence.
 */
enum __wasi_eventtype_t : uint8_t {
  /**
   * The time value of clock `subscription_clock::id` has
   * reached timestamp `subscription_clock::timeout`.
   */
  __WASI_EVENTTYPE_CLOCK = 0,

  /**
   * File descriptor `subscription_fd_readwrite::file_descriptor` has data
   * available for reading. This event always triggers for regular files.
   */
  __WASI_EVENTTYPE_FD_READ = 1,

  /**
   * File descriptor `subscription_fd_readwrite::file_descriptor` has capacity
   * available for writing. This event always triggers for regular files.
   */
  __WASI_EVENTTYPE_FD_WRITE = 2,

};
static_assert(sizeof(__wasi_eventtype_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_eventtype_t) == 1, "witx calculated align");

/**
 * The state of the file descriptor subscribed to with
 * `eventtype::fd_read` or `eventtype::fd_write`.
 */
enum __wasi_eventrwflags_t : uint16_t {

  /**
   * The peer of this socket has closed or disconnected.
   */
  __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP = 1ULL << 0,

};
DEFINE_ENUM_OPERATORS(__wasi_eventrwflags_t)

static_assert(sizeof(__wasi_eventrwflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_eventrwflags_t) == 2, "witx calculated align");

/**
 * The contents of an `event` when type is `eventtype::fd_read` or
 * `eventtype::fd_write`.
 */
struct __wasi_event_fd_readwrite_t {
  /**
   * The number of bytes available for reading or writing.
   */
  __wasi_filesize_t nbytes;

  /**
   * The state of the file descriptor.
   */
  __wasi_eventrwflags_t flags;
};

static_assert(sizeof(__wasi_event_fd_readwrite_t) == 16,
              "witx calculated size");
static_assert(alignof(__wasi_event_fd_readwrite_t) == 8,
              "witx calculated align");
static_assert(offsetof(__wasi_event_fd_readwrite_t, nbytes) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_event_fd_readwrite_t, flags) == 8,
              "witx calculated offset");

/**
 * An event that occurred.
 */
struct __wasi_event_t {
  /**
   * User-provided value that got attached to `subscription::userdata`.
   */
  __wasi_userdata_t userdata;

  /**
   * If non-zero, an error that occurred while processing the subscription
   * request.
   */
  __wasi_errno_t error;

  /**
   * The type of event that occured
   */
  __wasi_eventtype_t type;

  /**
   * The contents of the event, if it is an `eventtype::fd_read` or
   * `eventtype::fd_write`. `eventtype::clock` events ignore this field.
   */
  __wasi_event_fd_readwrite_t fd_readwrite;
};

static_assert(sizeof(__wasi_event_t) == 32, "witx calculated size");
static_assert(alignof(__wasi_event_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_event_t, userdata) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_event_t, error) == 8, "witx calculated offset");
static_assert(offsetof(__wasi_event_t, type) == 10, "witx calculated offset");
static_assert(offsetof(__wasi_event_t, fd_readwrite) == 16,
              "witx calculated offset");

/**
 * Flags determining how to interpret the timestamp provided in
 * `subscription_clock::timeout`.
 */
enum __wasi_subclockflags_t : uint16_t {

  /**
   * If set, treat the timestamp provided in
   * `subscription_clock::timeout` as an absolute timestamp of clock
   * `subscription_clock::id`. If clear, treat the timestamp
   * provided in `subscription_clock::timeout` relative to the
   * current time value of clock `subscription_clock::id`.
   */
  __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME = 1ULL << 0,

};
DEFINE_ENUM_OPERATORS(__wasi_subclockflags_t)

static_assert(sizeof(__wasi_subclockflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_subclockflags_t) == 2, "witx calculated align");

/**
 * The contents of a `subscription` when type is `eventtype::clock`.
 */
struct __wasi_subscription_clock_t {
  /**
   * The clock against which to compare the timestamp.
   */
  __wasi_clockid_t id;

  /**
   * The absolute or relative timestamp.
   */
  __wasi_timestamp_t timeout;

  /**
   * The amount of time that the implementation may wait additionally
   * to coalesce with other events.
   */
  __wasi_timestamp_t precision;

  /**
   * Flags specifying whether the timeout is absolute or relative
   */
  __wasi_subclockflags_t flags;
};

static_assert(sizeof(__wasi_subscription_clock_t) == 32,
              "witx calculated size");
static_assert(alignof(__wasi_subscription_clock_t) == 8,
              "witx calculated align");
static_assert(offsetof(__wasi_subscription_clock_t, id) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_subscription_clock_t, timeout) == 8,
              "witx calculated offset");
static_assert(offsetof(__wasi_subscription_clock_t, precision) == 16,
              "witx calculated offset");
static_assert(offsetof(__wasi_subscription_clock_t, flags) == 24,
              "witx calculated offset");

/**
 * The contents of a `subscription` when type is type is
 * `eventtype::fd_read` or `eventtype::fd_write`.
 */
struct __wasi_subscription_fd_readwrite_t {
  /**
   * The file descriptor on which to wait for it to become ready for reading or
   * writing.
   */
  __wasi_fd_t file_descriptor;
};

static_assert(sizeof(__wasi_subscription_fd_readwrite_t) == 4,
              "witx calculated size");
static_assert(alignof(__wasi_subscription_fd_readwrite_t) == 4,
              "witx calculated align");
static_assert(offsetof(__wasi_subscription_fd_readwrite_t, file_descriptor) ==
                  0,
              "witx calculated offset");

/**
 * The contents of a `subscription`.
 */
union __wasi_subscription_u_u_t {
  __wasi_subscription_clock_t clock;
  __wasi_subscription_fd_readwrite_t fd_read;
  __wasi_subscription_fd_readwrite_t fd_write;
};
struct __wasi_subscription_u_t {
  __wasi_eventtype_t tag;
  __wasi_subscription_u_u_t u;
};

static_assert(sizeof(__wasi_subscription_u_t) == 40, "witx calculated size");
static_assert(alignof(__wasi_subscription_u_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_subscription_u_t, u) == 8,
              "witx calculated union offset");

/**
 * Subscription to an event.
 */
struct __wasi_subscription_t {
  /**
   * User-provided value that is attached to the subscription in the
   * implementation and returned through `event::userdata`.
   */
  __wasi_userdata_t userdata;

  /**
   * The type of the event to which to subscribe, and its contents
   */
  __wasi_subscription_u_t u;
};

static_assert(sizeof(__wasi_subscription_t) == 48, "witx calculated size");
static_assert(alignof(__wasi_subscription_t) == 8, "witx calculated align");
static_assert(offsetof(__wasi_subscription_t, userdata) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_subscription_t, u) == 8,
              "witx calculated offset");

/**
 * Exit code generated by a process when exiting.
 */
using __wasi_exitcode_t = uint32_t;

static_assert(sizeof(__wasi_exitcode_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_exitcode_t) == 4, "witx calculated align");

/**
 * Signal condition.
 */
enum __wasi_signal_t : uint8_t {
  /**
   * No signal. Note that POSIX has special semantics for `kill(pid, 0)`,
   * so this value is reserved.
   */
  __WASI_SIGNAL_NONE = 0,

  /**
   * Hangup.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_HUP = 1,

  /**
   * Terminate interrupt signal.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_INT = 2,

  /**
   * Terminal quit signal.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_QUIT = 3,

  /**
   * Illegal instruction.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_ILL = 4,

  /**
   * Trace/breakpoint trap.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_TRAP = 5,

  /**
   * Process abort signal.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_ABRT = 6,

  /**
   * Access to an undefined portion of a memory object.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_BUS = 7,

  /**
   * Erroneous arithmetic operation.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_FPE = 8,

  /**
   * Kill.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_KILL = 9,

  /**
   * User-defined signal 1.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_USR1 = 10,

  /**
   * Invalid memory reference.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_SEGV = 11,

  /**
   * User-defined signal 2.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_USR2 = 12,

  /**
   * Write on a pipe with no one to read it.
   * Action: Ignored.
   */
  __WASI_SIGNAL_PIPE = 13,

  /**
   * Alarm clock.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_ALRM = 14,

  /**
   * Termination signal.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_TERM = 15,

  /**
   * Child process terminated, stopped, or continued.
   * Action: Ignored.
   */
  __WASI_SIGNAL_CHLD = 16,

  /**
   * Continue executing, if stopped.
   * Action: Continues executing, if stopped.
   */
  __WASI_SIGNAL_CONT = 17,

  /**
   * Stop executing.
   * Action: Stops executing.
   */
  __WASI_SIGNAL_STOP = 18,

  /**
   * Terminal stop signal.
   * Action: Stops executing.
   */
  __WASI_SIGNAL_TSTP = 19,

  /**
   * Background process attempting read.
   * Action: Stops executing.
   */
  __WASI_SIGNAL_TTIN = 20,

  /**
   * Background process attempting write.
   * Action: Stops executing.
   */
  __WASI_SIGNAL_TTOU = 21,

  /**
   * High bandwidth data is available at a socket.
   * Action: Ignored.
   */
  __WASI_SIGNAL_URG = 22,

  /**
   * CPU time limit exceeded.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_XCPU = 23,

  /**
   * File size limit exceeded.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_XFSZ = 24,

  /**
   * Virtual timer expired.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_VTALRM = 25,

  /**
   * Profiling timer expired.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_PROF = 26,

  /**
   * Window changed.
   * Action: Ignored.
   */
  __WASI_SIGNAL_WINCH = 27,

  /**
   * I/O possible.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_POLL = 28,

  /**
   * Power failure.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_PWR = 29,

  /**
   * Bad system call.
   * Action: Terminates the process.
   */
  __WASI_SIGNAL_SYS = 30,

};
static_assert(sizeof(__wasi_signal_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_signal_t) == 1, "witx calculated align");

/**
 * Socket address family
 */
enum __wasi_address_family_t : uint8_t {
  __WASI_ADDRESS_FAMILY_UNSPEC = 0,

  __WASI_ADDRESS_FAMILY_INET4 = 1,

  __WASI_ADDRESS_FAMILY_INET6 = 2,

  __WASI_ADDRESS_FAMILY_AF_UNIX = 3
};
static_assert(sizeof(__wasi_address_family_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_address_family_t) == 1, "witx calculated align");

/**
 * Socket address
 */
struct __wasi_address_t {
  uint8_t_ptr buf;

  __wasi_size_t buf_len;
};

static_assert(sizeof(__wasi_address_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_address_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_address_t, buf) == 0, "witx calculated offset");
static_assert(offsetof(__wasi_address_t, buf_len) == 4,
              "witx calculated offset");

enum __wasi_sock_opt_level_t : uint32_t {
  __WASI_SOCK_OPT_LEVEL_SOL_SOCKET = 0,

};
static_assert(sizeof(__wasi_sock_opt_level_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_sock_opt_level_t) == 4, "witx calculated align");

enum __wasi_sock_opt_so_t : uint32_t {
  __WASI_SOCK_OPT_SO_REUSEADDR = 0,

  __WASI_SOCK_OPT_SO_TYPE = 1,

  __WASI_SOCK_OPT_SO_ERROR = 2,

  __WASI_SOCK_OPT_SO_DONTROUTE = 3,

  __WASI_SOCK_OPT_SO_BROADCAST = 4,

  __WASI_SOCK_OPT_SO_SNDBUF = 5,

  __WASI_SOCK_OPT_SO_RCVBUF = 6,

  __WASI_SOCK_OPT_SO_KEEPALIVE = 7,

  __WASI_SOCK_OPT_SO_OOBINLINE = 8,

  __WASI_SOCK_OPT_SO_LINGER = 9,

  __WASI_SOCK_OPT_SO_RCVLOWAT = 10,

  __WASI_SOCK_OPT_SO_RCVTIMEO = 11,

  __WASI_SOCK_OPT_SO_SNDTIMEO = 12,

  __WASI_SOCK_OPT_SO_ACCEPTCONN = 13,

  __WASI_SOCK_OPT_SO_BINDTODEVICE = 14,

};
static_assert(sizeof(__wasi_sock_opt_so_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_sock_opt_so_t) == 4, "witx calculated align");

/**
 * Flags provided to `getaddrinfo`.
 */
enum __wasi_aiflags_t : uint16_t {

  /**
   * Socket address is intended for bind()
   */
  __WASI_AIFLAGS_AI_PASSIVE = 1ULL << 0,

  /**
   * Request for canonical name.
   */
  __WASI_AIFLAGS_AI_CANONNAME = 1ULL << 1,

  /**
   * Return numeric host address as name.
   */
  __WASI_AIFLAGS_AI_NUMERICHOST = 1ULL << 2,

  /**
   * Inhibit service name resolution.
   */
  __WASI_AIFLAGS_AI_NUMERICSERV = 1ULL << 3,

  /**
   * If no IPv6 addresses are found, query for IPv4 addresses and return them to
   * the caller as IPv4-mapped IPv6 addresses.
   */
  __WASI_AIFLAGS_AI_V4MAPPED = 1ULL << 4,

  /**
   * Query for both IPv4 and IPv6 addresses.
   */
  __WASI_AIFLAGS_AI_ALL = 1ULL << 5,

  /**
   * Query for IPv4 addresses only when an IPv4 address is configured; query for
   * IPv6 addresses only when an IPv6 address is configured.
   */
  __WASI_AIFLAGS_AI_ADDRCONFIG = 1ULL << 6,

};
DEFINE_ENUM_OPERATORS(__wasi_aiflags_t)

static_assert(sizeof(__wasi_aiflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_aiflags_t) == 2, "witx calculated align");

/**
 * Socket type
 */
enum __wasi_sock_type_t : uint8_t {
  __WASI_SOCK_TYPE_SOCK_ANY = 0,

  __WASI_SOCK_TYPE_SOCK_DGRAM = 1,

  __WASI_SOCK_TYPE_SOCK_STREAM = 2,

};
static_assert(sizeof(__wasi_sock_type_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_sock_type_t) == 1, "witx calculated align");

/**
 * Protocol
 */
enum __wasi_protocol_t : uint8_t {
  __WASI_PROTOCOL_IPPROTO_IP = 0,

  __WASI_PROTOCOL_IPPROTO_TCP = 1,

  __WASI_PROTOCOL_IPPROTO_UDP = 2,

};
static_assert(sizeof(__wasi_protocol_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_protocol_t) == 1, "witx calculated align");

/**
 * Socket address_in provided for getaddrinfo
 */
struct __wasi_sockaddr_in_t {
  __wasi_address_family_t sin_family;

  uint16_t sin_port;

  __wasi_address_t sin_addr;

  __wasi_size_t sin_zero_len;

  uint8_t_ptr sin_zero;
};

static_assert(sizeof(__wasi_sockaddr_in_t) == 20, "witx calculated size");
static_assert(alignof(__wasi_sockaddr_in_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_sockaddr_in_t, sin_family) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_in_t, sin_port) == 2,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_in_t, sin_addr) == 4,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_in_t, sin_zero_len) == 12,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_in_t, sin_zero) == 16,
              "witx calculated offset");

/**
 * Socket address provided for getaddrinfo
 */
struct __wasi_sockaddr_t {
  __wasi_address_family_t sa_family;

  __wasi_size_t sa_data_len;

  uint8_t_ptr sa_data;
};

static_assert(sizeof(__wasi_sockaddr_t) == 12, "witx calculated size");
static_assert(alignof(__wasi_sockaddr_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_sockaddr_t, sa_family) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_t, sa_data_len) == 4,
              "witx calculated offset");
static_assert(offsetof(__wasi_sockaddr_t, sa_data) == 8,
              "witx calculated offset");

/**
 * Address information
 */
struct __wasi_addrinfo_t {
  __wasi_aiflags_t ai_flags;

  __wasi_address_family_t ai_family;

  __wasi_sock_type_t ai_socktype;

  __wasi_protocol_t ai_protocol;

  __wasi_size_t ai_addrlen;

  uint8_t_ptr ai_addr;

  uint8_t_ptr ai_canonname;

  __wasi_size_t ai_canonname_len;

  uint8_t_ptr ai_next;
};

static_assert(sizeof(__wasi_addrinfo_t) == 28, "witx calculated size");
static_assert(alignof(__wasi_addrinfo_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_addrinfo_t, ai_flags) == 0,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_family) == 2,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_socktype) == 3,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_protocol) == 4,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_addrlen) == 8,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_addr) == 12,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_canonname) == 16,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_canonname_len) == 20,
              "witx calculated offset");
static_assert(offsetof(__wasi_addrinfo_t, ai_next) == 24,
              "witx calculated offset");

/**
 * Flags provided to `sock_recv`.
 */
enum __wasi_riflags_t : uint16_t {

  /**
   * Returns the message without removing it from the socket's receive queue.
   */
  __WASI_RIFLAGS_RECV_PEEK = 1ULL << 0,

  /**
   * On byte-stream sockets, block until the full amount of data can be
   * returned.
   */
  __WASI_RIFLAGS_RECV_WAITALL = 1ULL << 1,

};
DEFINE_ENUM_OPERATORS(__wasi_riflags_t)

static_assert(sizeof(__wasi_riflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_riflags_t) == 2, "witx calculated align");

/**
 * Flags returned by `sock_recv`.
 */
enum __wasi_roflags_t : uint16_t {

  /**
   * Returned by `sock_recv`: Message data has been truncated.
   */
  __WASI_ROFLAGS_RECV_DATA_TRUNCATED = 1ULL << 0,

};
DEFINE_ENUM_OPERATORS(__wasi_roflags_t)

static_assert(sizeof(__wasi_roflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_roflags_t) == 2, "witx calculated align");

/**
 * Flags provided to `sock_send`. As there are currently no flags
 * defined, it must be set to zero.
 */
using __wasi_siflags_t = uint16_t;

static_assert(sizeof(__wasi_siflags_t) == 2, "witx calculated size");
static_assert(alignof(__wasi_siflags_t) == 2, "witx calculated align");

/**
 * Which channels on a socket to shut down.
 */
enum __wasi_sdflags_t : uint8_t {

  /**
   * Disables further receive operations.
   */
  __WASI_SDFLAGS_RD = 1ULL << 0,

  /**
   * Disables further send operations.
   */
  __WASI_SDFLAGS_WR = 1ULL << 1,

};
DEFINE_ENUM_OPERATORS(__wasi_sdflags_t)

static_assert(sizeof(__wasi_sdflags_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_sdflags_t) == 1, "witx calculated align");

/**
 * Identifiers for preopened capabilities.
 */
enum __wasi_preopentype_t : uint8_t {
  /**
   * A pre-opened directory.
   */
  __WASI_PREOPENTYPE_DIR = 0,

};
static_assert(sizeof(__wasi_preopentype_t) == 1, "witx calculated size");
static_assert(alignof(__wasi_preopentype_t) == 1, "witx calculated align");

/**
 * The contents of a $prestat when type is `preopentype::dir`.
 */
struct __wasi_prestat_dir_t {
  /**
   * The length of the directory name for use with `fd_prestat_dir_name`.
   */
  __wasi_size_t pr_name_len;
};

static_assert(sizeof(__wasi_prestat_dir_t) == 4, "witx calculated size");
static_assert(alignof(__wasi_prestat_dir_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_prestat_dir_t, pr_name_len) == 0,
              "witx calculated offset");

/**
 * Information about a pre-opened capability.
 */
union __wasi_prestat_u_t {
  __wasi_prestat_dir_t dir;
};
struct __wasi_prestat_t {
  __wasi_preopentype_t tag;
  __wasi_prestat_u_t u;
};

static_assert(sizeof(__wasi_prestat_t) == 8, "witx calculated size");
static_assert(alignof(__wasi_prestat_t) == 4, "witx calculated align");
static_assert(offsetof(__wasi_prestat_t, u) == 4,
              "witx calculated union offset");
