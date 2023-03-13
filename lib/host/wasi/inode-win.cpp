// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "common/errcode.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "win.h"
#include <algorithm>
#include <boost/align/aligned_allocator.hpp>
#include <new>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#define NANOSECONDS_PER_TICK 100ULL
#define TICKS_PER_SECOND 10000000ULL
#define SEC_TO_UNIX_EPOCH 11644473600ULL
#define TICKS_TO_UNIX_EPOCH (TICKS_PER_SECOND * SEC_TO_UNIX_EPOCH)
#define PATH_BUFFER_MAX 32767

namespace WasmEdge {
namespace Host {
namespace WASI {

// clang-format off
  /*

  ## Implementation Status

  ### Host Functions: Function-wise Summary

  | Function               | Status             | Comment                                                                                                                                                                                                                                                          |
  | ---------------------- | ------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
  | `open`                 | complete           | some flags may not have an equivalent                                                                                                                                                                                                                            |
  | `fdAdvise`             | no equivalent      | have to find an solution                                                                                                                                                                                                                                         |
  | `fdAllocate`           | complete           | None                                                                                                                                                                                                                                                             |
  | `fdDatasync`           | complete           | documentation is not clear on whether metadata is also flushed, refer [here](https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers#remarks)                                                                                     |
  | `fdFdstatGet`          | complete           | depends on a partially complete function - `fromFileType` (this function has been implemented partially in linux), find appropriate functions to query the equivalent flags and fill the other fields (the implementation for linux has not filled these fields) |
  | `fdFdstatSetFlags`     | complete           | depends on a partially complete function - `fromFileType` and an equivalent for device ID needs to be found which may be related to the file index                                                                                                               |
  | `fdFilestatSetSize`    | complete           | None                                                                                                                                                                                                                                                             |
  | `fdFilestatSetTimes`   | complete           | None                                                                                                                                                                                                                                                             |
  | `fdPread`              | complete           | there maybe issues due to casting                                                                                                                                                                                                                                |
  | `fdPwrite`             | complete           | there maybe issues due to casting                                                                                                                                                                                                                                |
  | `fdRead`               | complete           | had already been implemented                                                                                                                                                                                                                                     |
  | `fdWrite`              | complete           | had already been implemented                                                                                                                                                                                                                                     |
  | `fdReaddir`            | complete           | Need to optimise the function and it depends on a partially implemented function - `fromFileType`                                                                                                                                                                |
  | `fdSeek`               | complete           | None                                                                                                                                                                                                                                                             |
  | `fdSync`               | complete           | works when the file has been opened with the flags `FILE_FLAG_NO_BUFFERING` and `FILE_FLAG_WRITE_THROUGH` which I suspect is the desired behaviour, refer [here](https://devblogs.microsoft.com/oldnewthing/20210729-00/?p=105494)                               |
  | `fdTell`               | complete           | None                                                                                                                                                                                                                                                             |
  | `getNativeHandler`     | complete           | had already been implemented                                                                                                                                                                                                                                     |
  | `pathCreateDirectory`  | complete           | None                                                                                                                                                                                                                                                             |
  | `pathFilestatGet`      | complete           | similar to `stat` which uses absolute paths                                                                                                                                                                                                                      |
  | `pathFilestatSetTimes` | complete           | None                                                                                                                                                                                                                                                             |
  | `pathLink`             | complete           | None                                                                                                                                                                                                                                                             |
  | `pathOpen`             | complete           | None                                                                                                                                                                                                                                                             |
  | `pathReadlink`         | complete           | None                                                                                                                                                                                                                                                             |
  | `pathRemoveDirectory`  | complete           | had been already implemented                                                                                                                                                                                                                                     |
  | `pathRename`           | complete           | None                                                                                                                                                                                                                                                             |
  | `pathSymlink`          | complete           | None                                                                                                                                                                                                                                                             |
  | `pathUnlinkFile`       | complete           | None                                                                                                                                                                                                                                                             |
  | `pollOneoff`           | incomplete         | could not find a similar concept on windows                                                                                                                                                                                                                      |
  | `sockGetPeerAddr`      | incomplete         | behaviour is unspecified                                                                                                                                                                                                                                         |
  | `unsafeFiletype`       | partially complete | need to find equivalent flags for three file types                                                                                                                                                                                                               |
  | `filetype`             | partially complete | need to find equivalent flags for three file types                                                                                                                                                                                                               |
  | `isDirectory`          | complete           | None                                                                                                                                                                                                                                                             |
  | `isSymlink`            | complete           | None                                                                                                                                                                                                                                                             |
  | `filesize`             | complete           | None                                                                                                                                                                                                                                                             |
  | `canBrowse`            | incomplete         | need to find appropriate functions                                                                                                                                                                                                                               |
  | `Poller::clock`        | incomplete         | could not find a similar concept on windows                                                                                                                                                                                                                      |
  | `Poller::read`         | incomplete         | could not find a similar concept on windows                                                                                                                                                                                                                      |
  | `Poller::write`        | incomplete         | could not find a similar concept on windows                                                                                                                                                                                                                      |
  | `Poller::wait`         | incomplete         | could not find a similar concept on windows                                                                                                                                                                                                                      |

  Resolves #1227 and #1477

  Reference: https://github.com/WasmEdge/WasmEdge/issues/1477

  */
// clang-format on

namespace {

namespace winapi = boost::winapi;

const LARGE_INTEGER ZERO_OFFSET = {.LowPart = 0, .HighPart = 0};

inline constexpr __wasi_size_t
calculateAddrinfoLinkedListSize(struct addrinfo *const Addrinfo) {
  __wasi_size_t Length = 0;
  for (struct addrinfo *TmpPointer = Addrinfo; TmpPointer != nullptr;
       TmpPointer = TmpPointer->ai_next) {
    Length++;
  }
  return Length;
};

static bool isSocket(LPVOID H) {
  if (likely(::GetFileType(H) != FILE_TYPE_PIPE)) {
    return false;
  }
  return !::GetNamedPipeInfo(H, nullptr, nullptr, nullptr, nullptr);
}

static SOCKET toSocket(boost::winapi::HANDLE_ H) {
  return reinterpret_cast<SOCKET>(H);
}

static __wasi_address_family_t *getAddressFamily(uint8_t *AddressBuf) {
  return reinterpret_cast<__wasi_address_family_t *>(AddressBuf);
}

static uint8_t *getAddress(uint8_t *AddressBuf) {
  // The first 2 bytes is for AddressFamily
  const auto Diff = sizeof(uint16_t);
  return &AddressBuf[Diff];
}

std::pair<const char *, std::unique_ptr<char[]>>
createNullTerminatedString(std::string_view View) noexcept {
  const char *CStr = nullptr;
  std::unique_ptr<char[]> Buffer;
  if (!View.empty()) {
    if (const auto Pos = View.find_first_of('\0');
        Pos != std::string_view::npos) {
      CStr = View.data();
    } else {
      Buffer = std::make_unique<char[]>(View.size() + 1);
      std::copy(View.begin(), View.end(), Buffer.get());
      CStr = Buffer.get();
    }
  }
  return {CStr, std::move(Buffer)};
}

inline LARGE_INTEGER toLargeIntegerFromUnsigned(unsigned long long Value) {
  LARGE_INTEGER Result;

  // Does the compiler natively support 64-bit integers?
#ifdef INT64_MAX
  Result.QuadPart = static_cast<int64_t>(Value);
#else
  Result.high_part = (value & 0xFFFFFFFF00000000) >> 32;
  Result.low_part = value & 0xFFFFFFFF;
#endif
  return Result;
}

inline LARGE_INTEGER toLargeIntegerFromSigned(long long Value) {
  LARGE_INTEGER Result;

#ifdef INT64_MAX
  Result.QuadPart = static_cast<int>(Value);
#else
  Result.high_part = (value & 0xFFFFFFFF00000000) >> 32;
  Result.low_part = value & 0xFFFFFFFF;
#endif
  return Result;
}

inline constexpr __wasi_errno_t fromWinError(DWORD Winerr) {
  __wasi_errno_t Error = __WASI_ERRNO_NOSYS;
  switch (Winerr) {
  case ERROR_ACCESS_DENIED:
  case ERROR_ACCOUNT_DISABLED:
  case ERROR_ACCOUNT_RESTRICTION:
  case ERROR_CANNOT_MAKE:
  case ERROR_CURRENT_DIRECTORY:
  case ERROR_INVALID_ACCESS:
  case ERROR_INVALID_LOGON_HOURS:
  case ERROR_INVALID_WORKSTATION:
  case ERROR_LOGON_FAILURE:
  case ERROR_NO_SUCH_PRIVILEGE:
  case ERROR_PASSWORD_EXPIRED:
  case ERROR_CANT_ACCESS_FILE:
  case ERROR_NOACCESS:
  case WSAEACCES:
  case ERROR_ELEVATION_REQUIRED:
    Error = __WASI_ERRNO_ACCES;
    break;
  case ERROR_ALREADY_ASSIGNED:
  case ERROR_BUSY_DRIVE:
  case ERROR_DEVICE_IN_USE:
  case ERROR_DRIVE_LOCKED:
  case ERROR_LOCKED:
  case ERROR_OPEN_FILES:
  case ERROR_PATH_BUSY:
  case ERROR_PIPE_BUSY:
  case ERROR_BUSY:
  case ERROR_LOCK_VIOLATION:
  case ERROR_SHARING_VIOLATION:
    Error = __WASI_ERRNO_BUSY;
    break;
  case ERROR_ALREADY_EXISTS:
  case ERROR_FILE_EXISTS:
    Error = __WASI_ERRNO_EXIST;
    break;
  case ERROR_ARITHMETIC_OVERFLOW:
    Error = __WASI_ERRNO_RANGE;
    break;
  case ERROR_BAD_COMMAND:
  case ERROR_CANTOPEN:
  case ERROR_CANTREAD:
  case ERROR_CANTWRITE:
  case ERROR_CRC:
  case ERROR_DISK_CHANGE:
  case ERROR_GEN_FAILURE:
  case ERROR_INVALID_TARGET_HANDLE:
  case ERROR_IO_DEVICE:
  case ERROR_NO_MORE_SEARCH_HANDLES:
  case ERROR_OPEN_FAILED:
  case ERROR_READ_FAULT:
  case ERROR_SEEK:
  case ERROR_WRITE_FAULT:
  case ERROR_BEGINNING_OF_MEDIA:
  case ERROR_BUS_RESET:
  case ERROR_DEVICE_DOOR_OPEN:
  case ERROR_DEVICE_REQUIRES_CLEANING:
  case ERROR_DISK_CORRUPT:
  case ERROR_EOM_OVERFLOW:
  case ERROR_INVALID_BLOCK_LENGTH:
  case ERROR_NO_DATA_DETECTED:
  case ERROR_NO_SIGNAL_SENT:
  case ERROR_SETMARK_DETECTED:
  case ERROR_SIGNAL_REFUSED:
  case ERROR_FILEMARK_DETECTED:
    Error = __WASI_ERRNO_IO;
    break;
  case ERROR_BAD_UNIT:
  case ERROR_BAD_DEVICE:
  case ERROR_DEV_NOT_EXIST:
  case ERROR_FILE_INVALID:
  case ERROR_INVALID_DRIVE:
  case ERROR_UNRECOGNIZED_VOLUME:
    Error = __WASI_ERRNO_NODEV;
    break;
  case ERROR_BAD_DRIVER_LEVEL:
  case ERROR_UNRECOGNIZED_MEDIA:
    Error = __WASI_ERRNO_NXIO;
    break;
  case ERROR_BAD_EXE_FORMAT:
  case ERROR_BAD_FORMAT:
  case ERROR_EXE_MARKED_INVALID:
  case ERROR_INVALID_EXE_SIGNATURE:
    Error = __WASI_ERRNO_NOEXEC;
    break;
  case ERROR_BAD_USERNAME:
  case ERROR_BAD_LENGTH:
  case ERROR_ENVVAR_NOT_FOUND:
  case ERROR_INVALID_DATA:
  case ERROR_INVALID_FLAGS:
  case ERROR_INVALID_NAME:
  case ERROR_INVALID_OWNER:
  case ERROR_INVALID_PARAMETER:
  case ERROR_INVALID_PRIMARY_GROUP:
  case ERROR_INVALID_SIGNAL_NUMBER:
  case ERROR_MAPPED_ALIGNMENT:
  case ERROR_NONE_MAPPED:
  case ERROR_SYMLINK_NOT_SUPPORTED:
    Error = __WASI_ERRNO_INVAL;
    break;
  case ERROR_BAD_PATHNAME:
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
  case ERROR_SWAPERROR:
  case ERROR_DIRECTORY:
  case ERROR_INVALID_REPARSE_DATA:
  case ERROR_MOD_NOT_FOUND:
    Error = __WASI_ERRNO_NOENT;
    break;
  case ERROR_BROKEN_PIPE:
  case ERROR_BAD_PIPE:
  case ERROR_MORE_DATA:
  case ERROR_NO_DATA:
  case ERROR_PIPE_CONNECTED:
  case ERROR_PIPE_LISTENING:
  case ERROR_PIPE_NOT_CONNECTED:
    Error = __WASI_ERRNO_PIPE;
    break;
  case ERROR_BUFFER_OVERFLOW:
  case ERROR_FILENAME_EXCED_RANGE:
    Error = __WASI_ERRNO_NAMETOOLONG;
    break;
  case ERROR_CALL_NOT_IMPLEMENTED:
  case ERROR_INVALID_FUNCTION:
    Error = __WASI_ERRNO_NOSYS;
    break;
  case ERROR_DIR_NOT_EMPTY:
    Error = __WASI_ERRNO_NOTEMPTY;
    break;
  case ERROR_DISK_FULL:
  case ERROR_HANDLE_DISK_FULL:
  case ERROR_EA_TABLE_FULL:
  case ERROR_END_OF_MEDIA:
    Error = __WASI_ERRNO_NOSPC;
    break;
  case ERROR_INSUFFICIENT_BUFFER:
  case ERROR_NOT_ENOUGH_MEMORY:
  case ERROR_OUTOFMEMORY:
  case ERROR_STACK_OVERFLOW:
    Error = __WASI_ERRNO_NOMEM;
    break;
  case ERROR_INVALID_ADDRESS:
  case ERROR_INVALID_BLOCK:
    Error = __WASI_ERRNO_FAULT;
    break;
  case ERROR_NOT_READY:
  case ERROR_NO_PROC_SLOTS:
  case ERROR_ADDRESS_ALREADY_ASSOCIATED:
    Error = __WASI_ERRNO_ADDRINUSE;
    break;
  case ERROR_INVALID_PASSWORD:
  case ERROR_PRIVILEGE_NOT_HELD:
    Error = __WASI_ERRNO_PERM;
    break;
  case ERROR_IO_INCOMPLETE:
  case ERROR_OPERATION_ABORTED:
    Error = __WASI_ERRNO_INTR;
    break;
  case ERROR_META_EXPANSION_TOO_LONG:
    Error = __WASI_ERRNO_2BIG;
    break;
  case ERROR_NEGATIVE_SEEK:
  case ERROR_SEEK_ON_DEVICE:
    Error = __WASI_ERRNO_SPIPE;
    break;
  case ERROR_NOT_SAME_DEVICE:
    Error = __WASI_ERRNO_XDEV;
    break;
  case ERROR_SHARING_BUFFER_EXCEEDED:
    Error = __WASI_ERRNO_NFILE;
    break;
  case ERROR_TOO_MANY_MODULES:
  case ERROR_TOO_MANY_OPEN_FILES:
    Error = __WASI_ERRNO_MFILE;
    break;
  case ERROR_WAIT_NO_CHILDREN:
    Error = __WASI_ERRNO_CHILD;
    break;
  case ERROR_WRITE_PROTECT:
    Error = __WASI_ERRNO_ROFS;
    break;
  case ERROR_CANT_RESOLVE_FILENAME:
    Error = __WASI_ERRNO_LOOP;
    break;
  case ERROR_CONNECTION_ABORTED:
    Error = __WASI_ERRNO_CONNABORTED;
    break;
  case ERROR_CONNECTION_REFUSED:
    Error = __WASI_ERRNO_CONNREFUSED;
    break;
  case ERROR_HOST_UNREACHABLE:
    Error = __WASI_ERRNO_HOSTUNREACH;
    break;
  case ERROR_INVALID_HANDLE:
    Error = __WASI_ERRNO_BADF;
    break;
  case ERROR_NETNAME_DELETED:
    Error = __WASI_ERRNO_CONNRESET;
    break;
  case ERROR_NETWORK_UNREACHABLE:
    Error = __WASI_ERRNO_NETUNREACH;
    break;
  case ERROR_NOT_CONNECTED:
    Error = __WASI_ERRNO_NOTCONN;
    break;
  case ERROR_NOT_SUPPORTED:
    Error = __WASI_ERRNO_NOTSUP;
    break;
  case ERROR_SEM_TIMEOUT:
    Error = __WASI_ERRNO_TIMEDOUT;
    break;
  case ERROR_TOO_MANY_LINKS:
    Error = __WASI_ERRNO_MLINK;
    break;
  default:
    assumingUnreachable();
  }
  return Error;
}

constexpr DWORD attributeFlags(__wasi_oflags_t OpenFlags,
                               __wasi_fdflags_t FdFlags) noexcept {
  DWORD Flags = FILE_ATTRIBUTE_NORMAL;
  if ((FdFlags & __WASI_FDFLAGS_NONBLOCK) != 0) {
    Flags |= FILE_FLAG_OVERLAPPED;
  }

  // Source: https://devblogs.microsoft.com/oldnewthing/20210729-00/?p=105494
  if ((FdFlags & __WASI_FDFLAGS_SYNC) || (FdFlags & __WASI_FDFLAGS_RSYNC)) {
    // Linux does not implement O_RSYNC and glibc defines O_RSYNC as O_SYNC
    Flags |= FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING;
  }
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    Flags |= FILE_FLAG_WRITE_THROUGH;
  }
  if (OpenFlags & __WASI_OFLAGS_DIRECTORY) {
    Flags |= FILE_ATTRIBUTE_DIRECTORY | FILE_FLAG_BACKUP_SEMANTICS;
  }

  return Flags;
}

constexpr DWORD accessFlags(__wasi_fdflags_t FdFlags,
                            uint8_t VFSFlags) noexcept {
  DWORD Flags = 0;

  if (VFSFlags & VFS::Read) {
    if (VFSFlags & VFS::Write) {
      Flags |= GENERIC_READ | GENERIC_WRITE;
    } else {
      Flags |= GENERIC_READ;
    }
  } else if (VFSFlags & VFS::Write) {
    Flags |= GENERIC_WRITE;
  }

  if ((FdFlags & __WASI_FDFLAGS_APPEND) != 0) {
    Flags |= FILE_APPEND_DATA;
  }

  return Flags;
}

constexpr DWORD creationDisposition(__wasi_oflags_t OpenFlags) {
  DWORD Flags = OPEN_EXISTING;
  if (OpenFlags & __WASI_OFLAGS_CREAT) {
    Flags = OPEN_ALWAYS;
  }
  if (OpenFlags & __WASI_OFLAGS_TRUNC) {
    Flags = TRUNCATE_EXISTING;
  }
  if (OpenFlags & __WASI_OFLAGS_EXCL) {
    Flags = CREATE_NEW;
  }
  return Flags;
}

inline constexpr __wasi_filetype_t fromFileType(DWORD Attribute,
                                                DWORD FileType) noexcept {
  switch (Attribute) {
  case FILE_ATTRIBUTE_DIRECTORY:
    return __WASI_FILETYPE_DIRECTORY;
  case FILE_ATTRIBUTE_NORMAL:
    return __WASI_FILETYPE_REGULAR_FILE;
  case FILE_ATTRIBUTE_REPARSE_POINT:
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  }
  switch (FileType) {
  case FILE_TYPE_CHAR:
    return __WASI_FILETYPE_CHARACTER_DEVICE;
  }
  return __WASI_FILETYPE_UNKNOWN;
}

constexpr inline DWORD fromWhence(__wasi_whence_t Whence) {
  switch (Whence) {
  case __WASI_WHENCE_SET:
    return FILE_BEGIN;
  case __WASI_WHENCE_END:
    return FILE_END;
  case __WASI_WHENCE_CUR:
    return FILE_CURRENT;
  }
}

} // namespace

void HandleHolder::reset() noexcept {
  if (likely(ok())) {
    if (likely(!isSocket(&Handle))) {
      winapi::CloseHandle(Handle);
    } else {
      ::closesocket(reinterpret_cast<SOCKET>(Handle));
    }
    Handle = nullptr;
  }
}

INode INode::stdIn() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_INPUT_HANDLE_));
}

INode INode::stdOut() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_OUTPUT_HANDLE_));
}

INode INode::stdErr() noexcept {
  return INode(winapi::GetStdHandle(winapi::STD_ERROR_HANDLE_));
}

WasiExpect<INode> INode::open(std::string Path, __wasi_oflags_t OpenFlags,
                              __wasi_fdflags_t FdFlags,
                              uint8_t VFSFlags) noexcept {

  const DWORD AttributeFlags = attributeFlags(OpenFlags, FdFlags);
  const DWORD AccessFlags = accessFlags(FdFlags, VFSFlags);
  const DWORD CreationDisposition = creationDisposition(OpenFlags);

  WCHAR PathBuffer[PATH_BUFFER_MAX];

  int NumCharacters = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, Path.c_str(),
                                          -1, PathBuffer, PATH_BUFFER_MAX);

  if (unlikely(NumCharacters <= 0)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  HANDLE FileHandle =
      CreateFileW(PathBuffer, AccessFlags,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, CreationDisposition, AttributeFlags, nullptr);

  if (unlikely(FileHandle == INVALID_HANDLE_VALUE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  } else {
    INode New(FileHandle);
    return New;
  }
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t, __wasi_filesize_t,
                                 __wasi_advice_t) const noexcept {
  // FIXME: No equivalent function was found for this purpose in the Win32 API
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t Offset,
                                   __wasi_filesize_t Len) const noexcept {

  LARGE_INTEGER FileSize;
  if (unlikely(GetFileSizeEx(Handle, &FileSize) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  // We need to check if the request size (Offset + Len) is lesser than the
  // current the size and if it is lesser then we don't truncate the file

  if (static_cast<int64_t>((Offset + Len) & 0x7FFFFFFFFFFFFFFF) >
      FileSize.QuadPart) {

    FILE_STANDARD_INFO StandardInfo;
    FILE_ALLOCATION_INFO AllocationInfo;

    if (unlikely(GetFileInformationByHandleEx(Handle, FileStandardInfo,
                                              &StandardInfo,
                                              sizeof(StandardInfo))) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    // TODO: Since the unsigned integer is cast into a signed integer the range
    // of the values will be twice as small as the parameter - for very large
    // unsigned integers the cast to a signed integer may overflow the range of
    // the signed integer. Is this fine?
    AllocationInfo.AllocationSize.QuadPart =
        static_cast<int64_t>((Offset + Len) & 0x7FFFFFFFFFFFFFFF);

    if (SetFileInformationByHandle(Handle, FileAllocationInfo, &AllocationInfo,
                                   sizeof(AllocationInfo)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
  }
  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept {
  if (unlikely(FlushFileBuffers(Handle) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept {
  // TODO: Complete this partially implemented function after finding equivalent
  // flags/attributes for fs_filetype, fs_flags and fs_rights_base and
  // fs_rights_inheriting. The linux implementation has not implemented this
  // function completely.

  // Update the file information
  FileInfo.emplace();
  if (unlikely(GetFileInformationByHandle(Handle, &(*FileInfo)) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  FdStat.fs_filetype =
      fromFileType((*FileInfo).dwFileAttributes, GetFileType(Handle));

  // We don't have a function to retrieve the equivalent Fd Flags used to
  // open the file in the win32 API hence it will be better to retrieve the
  // saved flags used during the file open
  // FIXME: Find a better way
  FdStat.fs_flags = (*SavedFdFlags);

  return {};
}

WasiExpect<void>
INode::fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept {
  // The __WASI_FDFLAGS_APPEND flag is ignored as it cannot be changed for an
  // open file

  DWORD Attributes = FILE_ATTRIBUTE_NORMAL;

  FILE_BASIC_INFO BasicInfo;

  // Source: https://devblogs.microsoft.com/oldnewthing/20210729-00/?p=105494
  if ((FdFlags & __WASI_FDFLAGS_SYNC) || (FdFlags & __WASI_FDFLAGS_RSYNC)) {
    // Linux does not implement RSYNC and glibc defines O_RSYNC as O_SYNC
    Attributes |= FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING;
  }
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    Attributes |= FILE_FLAG_NO_BUFFERING;
  }
  if (FdFlags & __WASI_FDFLAGS_NONBLOCK) {
    Attributes |= FILE_FLAG_OVERLAPPED;
  }

  if (unlikely(GetFileInformationByHandleEx(Handle, FileBasicInfo, &BasicInfo,
                                            sizeof(BasicInfo))) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  // Update the attributes
  BasicInfo.FileAttributes = Attributes;

  if (unlikely(SetFileInformationByHandle(Handle, FileBasicInfo, &BasicInfo,
                                          sizeof(BasicInfo))) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<void>
INode::fdFilestatGet(__wasi_filestat_t &FileStat) const noexcept {
  // TODO: Complete this partially implemented function after finding equivalent
  // flags/attributes for __wasi_filetype_t.

  // Update the File information
  FileInfo.emplace();
  if (unlikely(GetFileInformationByHandle(Handle, &*FileInfo) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  FileStat.filetype =
      fromFileType((*FileInfo).dwFileAttributes, GetFileType(Handle));

  // Windows does not have an equivalent for the INode number.
  // A possible equivalent could be the File Index
  // Source:
  // https://stackoverflow.com/questions/28252850/open-windows-file-using-unique-id/28253123#28253123
  // this
  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilebyid?redirectedfrom=MSDN
  // and this
  // https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info
  FileStat.ino = 0;
  // TODO: Find an equivalent for device ID in windows
  FileStat.dev = 0;
  FileStat.nlink = (*FileInfo).nNumberOfLinks;
  FileStat.size = static_cast<uint64_t>((*FileInfo).nFileSizeLow) +
                  (static_cast<uint64_t>((*FileInfo).nFileSizeHigh) << 32);
  FileStat.atim =
      (static_cast<uint64_t>((*FileInfo).ftLastAccessTime.dwHighDateTime)
       << 32) +
      (static_cast<uint64_t>((*FileInfo).ftLastAccessTime.dwLowDateTime)) -
      TICKS_TO_UNIX_EPOCH;
  FileStat.mtim =
      (static_cast<uint64_t>((*FileInfo).ftLastWriteTime.dwHighDateTime)
       << 32) +
      (static_cast<uint64_t>((*FileInfo).ftLastWriteTime.dwLowDateTime)) -
      TICKS_TO_UNIX_EPOCH;
  FileStat.ctim =
      (static_cast<uint64_t>((*FileInfo).ftCreationTime.dwHighDateTime) << 32) +
      (static_cast<uint64_t>((*FileInfo).ftCreationTime.dwLowDateTime)) -
      TICKS_TO_UNIX_EPOCH;

  return {};
}

WasiExpect<void>
INode::fdFilestatSetSize(__wasi_filesize_t Size) const noexcept {

  FILE_STANDARD_INFO StandardInfo;
  FILE_ALLOCATION_INFO AllocationInfo;

  if (unlikely(GetFileInformationByHandleEx(Handle, FileStandardInfo,
                                            &StandardInfo,
                                            sizeof(StandardInfo))) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  uint64_t PreviousSize =
      (static_cast<uint64_t>(StandardInfo.AllocationSize.HighPart) << 32) +
      static_cast<uint64_t>(StandardInfo.AllocationSize.LowPart);

  // Update the size attribute
  AllocationInfo.AllocationSize = toLargeIntegerFromUnsigned(Size);

  if (SetFileInformationByHandle(Handle, FileAllocationInfo, &AllocationInfo,
                                 sizeof(AllocationInfo)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  if (Size > PreviousSize) {
    OVERLAPPED FileOffsetProvider;
    FileOffsetProvider.Offset =
        static_cast<DWORD>(StandardInfo.AllocationSize.LowPart);
    FileOffsetProvider.OffsetHigh =
        static_cast<DWORD>(StandardInfo.AllocationSize.HighPart);

    // Write null byte by byte
    uint64_t Count = static_cast<uint64_t>(Size - PreviousSize);
    while (Count > 0) {
      DWORD BytesWritten;
      BOOL WriteResult =
          WriteFile(Handle, "\0", 1, nullptr, &FileOffsetProvider);

      if (GetLastError() == ERROR_IO_PENDING) {
        // Wait for the Write to complete
        if (unlikely(GetOverlappedResult(Handle, &FileOffsetProvider,
                                         &BytesWritten, TRUE)) == FALSE) {
          return WasiUnexpect(fromWinError(GetLastError()));
        }
      } else if (unlikely(WriteResult == FALSE)) {
        return WasiUnexpect(fromWinError(GetLastError()));
      }
      Count++;
    }

    // Restore pointer
    LARGE_INTEGER FileOffset;
    FileOffset.QuadPart = static_cast<int64_t>(PreviousSize - Size);
    if (unlikely(SetFilePointerEx(Handle, FileOffset, nullptr, FILE_CURRENT) ==
                 FALSE)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
  }

  return {};
}

WasiExpect<void>
INode::fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                          __wasi_fstflags_t FstFlags) const noexcept {

  // Let FileTime be initialized to zero if the times need not be changed
  FILETIME AFileTime = {0, 0};
  FILETIME MFileTime = {0, 0};

  // For setting access time
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    uint64_t Aticks = ATim / NANOSECONDS_PER_TICK + TICKS_TO_UNIX_EPOCH;
    AFileTime.dwLowDateTime = static_cast<DWORD>(Aticks & 0xFFFFFFFF);
    AFileTime.dwHighDateTime =
        static_cast<DWORD>((Aticks & 0xFFFFFFFF00000000) >> 32);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
    GetSystemTimeAsFileTime(&AFileTime);
  }

  // For setting modification time
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    uint64_t Mticks = MTim / NANOSECONDS_PER_TICK + TICKS_TO_UNIX_EPOCH;
    MFileTime.dwLowDateTime = static_cast<DWORD>(Mticks & 0xFFFFFFFF);
    MFileTime.dwHighDateTime =
        static_cast<DWORD>((Mticks & 0xFFFFFFFF00000000) >> 32);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
    GetSystemTimeAsFileTime(&MFileTime);
  }

  if (unlikely(SetFileTime(Handle, nullptr, &AFileTime, &MFileTime)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<void> INode::fdPread(Span<Span<uint8_t>> IOVs,
                                __wasi_filesize_t Offset,
                                __wasi_size_t &NRead) const noexcept {
  NRead = 0;
  uint64_t LocalOffset = Offset;

  for (auto IOV : IOVs) {
    DWORD NumberOfBytesRead = 0;
    OVERLAPPED Result;

    Result.Offset = static_cast<uint32_t>(LocalOffset);
    Result.OffsetHigh = static_cast<uint32_t>(LocalOffset >> 32);

    // Casting the 64 bit `IOV.size()` integer may overflow the range
    // of the 32 bit integer it is cast into
    BOOL ReadResult =
        ReadFile(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                 &NumberOfBytesRead, &Result);
    if (GetLastError() == ERROR_IO_PENDING) {
      // Wait for the Write to complete
      if (unlikely(GetOverlappedResult(Handle, &Result, &NumberOfBytesRead,
                                       TRUE)) == FALSE) {
        return WasiUnexpect(fromWinError(GetLastError()));
      }
    } else if (unlikely(ReadResult == FALSE)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    LocalOffset += NumberOfBytesRead;
    NRead += NumberOfBytesRead;
  }

  return {};
}

WasiExpect<void> INode::fdPwrite(Span<Span<const uint8_t>> IOVs,
                                 __wasi_filesize_t Offset,
                                 __wasi_size_t &NWritten) const noexcept {
  NWritten = 0;
  uint64_t LocalOffset = Offset;

  for (auto IOV : IOVs) {
    DWORD NumberOfBytesWritten = 0;
    OVERLAPPED Result;

    Result.Offset = static_cast<uint32_t>(LocalOffset);
    Result.OffsetHigh = static_cast<uint32_t>(LocalOffset >> 32);

    // There maybe issues due to casting IOV.size() to unit32_t
    BOOL WriteResult = WriteFile(
        Handle, static_cast<const uint8_t *>(IOV.data()),
        static_cast<uint32_t>(IOV.size()), &NumberOfBytesWritten, &Result);

    if (GetLastError() == ERROR_IO_PENDING) {
      // Wait for the Write to complete
      if (unlikely(GetOverlappedResult(Handle, &Result, &NumberOfBytesWritten,
                                       TRUE)) == FALSE) {
        return WasiUnexpect(fromWinError(GetLastError()));
      }
    } else if (unlikely(WriteResult == FALSE)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    LocalOffset += NumberOfBytesWritten;
    NWritten += NumberOfBytesWritten;
  }

  return {};
}

WasiExpect<void> INode::fdRead(Span<Span<uint8_t>> IOVs,
                               __wasi_size_t &NRead) const noexcept {
  NRead = 0;
  for (auto IOV : IOVs) {
    winapi::DWORD_ NumberOfBytesRead = 0;
    if (!winapi::ReadFile(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                          &NumberOfBytesRead, nullptr)) {
      return WasiUnexpect(fromLastError(winapi::GetLastError()));
    }
    NRead += NumberOfBytesRead;
  }
  return {};
}

WasiExpect<void> INode::fdReaddir(Span<uint8_t> Buffer,
                                  __wasi_dircookie_t Cookie,
                                  __wasi_size_t &Size) noexcept {

  WIN32_FIND_DATAW FindData;
  uint64_t Seek = 0;
  wchar_t HandleFullPathW[MAX_PATH];
  wchar_t FullPathW[MAX_PATH];

  std::vector<uint8_t, boost::alignment::aligned_allocator<
                           uint8_t, alignof(__wasi_dirent_t)>>
      LocalBuffer;

  // First get the path of the handle
  if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPathW, MAX_PATH,
                                         FILE_NAME_NORMALIZED)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  // Check if the path is a directory or not
  if (!PathIsDirectoryW(HandleFullPathW)) {
    return WasiUnexpect(__WASI_ERRNO_NOTDIR);
  }

  // WildCard to match every file/directory present in the directory
  const wchar_t WildCard[]{L"\\*"};
  HRESULT CombineResult =
      PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW, WildCard);

  switch (CombineResult) {
  case S_OK:
    break;
  case E_INVALIDARG:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  case E_OUTOFMEMORY:
    return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
  default:
    return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
  }

  // Begin the search for files
  HANDLE LocalFindHandle = FindFirstFileW(FullPathW, &FindData);
  if (unlikely(LocalFindHandle == INVALID_HANDLE_VALUE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  // seekdir() emulation - go to the Cookie'th file/directory
  while (Seek < Cookie) {
    if (unlikely(FindNextFileW(LocalFindHandle, &FindData) == FALSE)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    Seek++;
  }

  uint32_t NumberOfBytesRead = 0;
  BOOL FindNextResult = FALSE;

  do {
    if (!LocalBuffer.empty()) {
      const auto NewDataSize =
          std::min<uint32_t>(static_cast<uint32_t>(Buffer.size()),
                             static_cast<uint32_t>(LocalBuffer.size()));
      std::copy(LocalBuffer.begin(), LocalBuffer.begin() + NewDataSize,
                Buffer.begin());
      Buffer = Buffer.subspan(NewDataSize);
      Size += NewDataSize;
      LocalBuffer.erase(LocalBuffer.begin(), LocalBuffer.begin() + NewDataSize);
      if (unlikely(Buffer.empty())) {
        break;
      }
    }

    std::wstring_view FileName = FindData.cFileName;

    __wasi_dirent_t DirentObject = {.d_next = 0,
                                    .d_ino = 0,
                                    .d_namlen = 0,
                                    .d_type = __WASI_FILETYPE_UNKNOWN};

    LocalBuffer.resize(sizeof(__wasi_dirent_t) + (FileName.size() * 2));

    NumberOfBytesRead += sizeof(__wasi_dirent_t) + (FileName.size() * 2);

    __wasi_dirent_t *const Dirent =
        reinterpret_cast<__wasi_dirent_t *>(LocalBuffer.data());

    // The opening and closing of the handles may have a negative
    // impact on the performance

    HANDLE LocalFileHandle;

    CombineResult = PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW,
                                   FindData.cFileName);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

    LocalFileHandle = CreateFileW(FullPathW, GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, 0, nullptr);

    if (LocalFileHandle == INVALID_HANDLE_VALUE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    DWORD FileType = GetFileType(LocalFileHandle);

    if (GetLastError() != NO_ERROR) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    CloseHandle(LocalFileHandle);

    DirentObject.d_type = fromFileType(FindData.dwFileAttributes, FileType);

    // Since windows does not have any equivalent to the INode number,
    // we set this to 0
    // Possible equivalent could be the File Index
    // Source:
    // https://stackoverflow.com/questions/28252850/open-windows-file-using-unique-id/28253123#28253123
    // this
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilebyid?redirectedfrom=MSDN
    // and this
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info
    Dirent->d_ino = 0;

    // The filed size may not be sufficient to hold the complete length
    Dirent->d_namlen = static_cast<uint32_t>(sizeof(FindData.cFileName));
    Dirent->d_next = sizeof(__wasi_dirent_t) + Dirent->d_namlen;
    Dirent->d_ino = 0;

    std::copy(FileName.cbegin(), FileName.cend(),
              LocalBuffer.begin() + sizeof(__wasi_dirent_t));
    // Check if there no more files left or if an error has been encountered
    FindNextResult = FindNextFileW(LocalFindHandle, &FindData);
  } while (FindNextResult != ERROR_NO_MORE_FILES || FindNextResult != FALSE);

  FindClose(LocalFindHandle);

  if (GetLastError() != ERROR_NO_MORE_FILES) {
    // The FindNextFileW() function has failed
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  Size = NumberOfBytesRead;

  return {};
}

WasiExpect<void> INode::fdSeek(__wasi_filedelta_t Offset,
                               __wasi_whence_t Whence,
                               __wasi_filesize_t &Size) const noexcept {

  DWORD MoveMethod = fromWhence(Whence);
  LARGE_INTEGER DistanceToMove = toLargeIntegerFromSigned(Offset);
  LARGE_INTEGER Pointer;
  if (unlikely(SetFilePointerEx(Handle, DistanceToMove, &Pointer,
                                MoveMethod)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  } else {
    Size = static_cast<uint64_t>(Pointer.QuadPart);
  }
  return {};
}

WasiExpect<void> INode::fdSync() const noexcept {
  if (unlikely(FlushFileBuffers(Handle)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::fdTell(__wasi_filesize_t &Size) const noexcept {
  LARGE_INTEGER Pointer;

  if (unlikely(SetFilePointerEx(Handle, ZERO_OFFSET, &Pointer, FILE_CURRENT)) ==
      FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  } else {
    Size = static_cast<uint64_t>(Pointer.QuadPart);
  }
  return {};
}

WasiExpect<void> INode::fdWrite(Span<Span<const uint8_t>> IOVs,
                                __wasi_size_t &NWritten) const noexcept {
  NWritten = 0;
  for (auto IOV : IOVs) {
    winapi::DWORD_ NumberOfBytesWritten = 0;
    if (!winapi::WriteFile(Handle, IOV.data(),
                           static_cast<uint32_t>(IOV.size()),
                           &NumberOfBytesWritten, nullptr)) {
      return WasiUnexpect(fromLastError(winapi::GetLastError()));
    }
    NWritten += NumberOfBytesWritten;
  }
  return {};
}

WasiExpect<uint64_t> INode::getNativeHandler() const noexcept {
  return reinterpret_cast<uint64_t>(Handle);
}

WasiExpect<void> INode::pathCreateDirectory(std::string Path) const noexcept {
  wchar_t FullPathW[MAX_PATH];

  if (PathIsRelativeA(Path.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the paths of the handles
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPathW, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t OldPathW[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(OldPathW, Path.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW, OldPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
  }

  else {
    mbstowcs(FullPathW, Path.c_str(), MAX_PATH);
  }

  if (unlikely(CreateDirectoryW(FullPathW, nullptr) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }
  return {};
}

WasiExpect<void>
INode::pathFilestatGet(std::string Path,
                       __wasi_filestat_t &FileStat) const noexcept {
  // Since there is no way to get the stat of a file without a HANDLE to it,
  // we open a handle to the requested file, call GetFileInformationByHandle
  // on it and then update our WASI FileStat

  // Since the required function is similar to `stat` we assume Path is an
  // absolute path

  HANDLE LocalFileHandle;
  if (LocalFileHandle = CreateFileA(Path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                    nullptr, OPEN_EXISTING, 0, nullptr);
      likely(LocalFileHandle != INVALID_HANDLE_VALUE)) {
    BY_HANDLE_FILE_INFORMATION LocalFileInfo;
    if (likely(GetFileInformationByHandle(LocalFileHandle, &LocalFileInfo) !=
               FALSE)) {
      FileStat.filetype = fromFileType(LocalFileInfo.dwFileAttributes,
                                       GetFileType(LocalFileHandle));

      // Windows does not have an equivalent for the INode number
      // Possible equivalent could be the File Index
      // Source:
      // https://stackoverflow.com/questions/28252850/open-windows-file-using-unique-id/28253123#28253123
      // this
      // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilebyid?redirectedfrom=MSDN
      // and this
      // https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_info
      FileStat.ino = 0;
      // TODO: Find an equivalent for device ID in windows
      FileStat.dev = 0;
      FileStat.nlink = (LocalFileInfo).nNumberOfLinks;
      FileStat.size =
          static_cast<uint64_t>((LocalFileInfo).nFileSizeLow) +
          (static_cast<uint64_t>((LocalFileInfo).nFileSizeHigh) << 32);
      FileStat.atim = (static_cast<uint64_t>(
                           (LocalFileInfo).ftLastAccessTime.dwHighDateTime)
                       << 32) +
                      (static_cast<uint64_t>(
                          (LocalFileInfo).ftLastAccessTime.dwLowDateTime)) -
                      TICKS_TO_UNIX_EPOCH;
      FileStat.mtim =
          (static_cast<uint64_t>((LocalFileInfo).ftLastWriteTime.dwHighDateTime)
           << 32) +
          (static_cast<uint64_t>(
              (LocalFileInfo).ftLastWriteTime.dwLowDateTime)) -
          TICKS_TO_UNIX_EPOCH;
      FileStat.ctim =
          (static_cast<uint64_t>((LocalFileInfo).ftCreationTime.dwHighDateTime)
           << 32) +
          (static_cast<uint64_t>(
              (LocalFileInfo).ftCreationTime.dwLowDateTime)) -
          TICKS_TO_UNIX_EPOCH;

      return {};
    }
    CloseHandle(LocalFileHandle);
  }

  return WasiUnexpect(fromWinError(GetLastError()));
}

WasiExpect<void>
INode::pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                            __wasi_timestamp_t MTim,
                            __wasi_fstflags_t FstFlags) const noexcept {

  wchar_t FullPathW[MAX_PATH];

  if (PathIsRelativeA(Path.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the path of the handle
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPathW, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t OldPathW[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(OldPathW, Path.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW, OldPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
  } else {
    mbstowcs(FullPathW, Path.c_str(), MAX_PATH);
  }

  HANDLE LocalFileHandle;
  if (LocalFileHandle = CreateFileW(FullPathW, GENERIC_READ, FILE_SHARE_READ,
                                    nullptr, OPEN_EXISTING, 0, nullptr);
      likely(LocalFileHandle != INVALID_HANDLE_VALUE)) {
    // Let FileTime be initialized to zero if the times need not be changed
    FILETIME AFileTime = {0, 0};
    FILETIME MFileTime = {0, 0};

    // For setting access time
    if (FstFlags & __WASI_FSTFLAGS_ATIM) {
      uint64_t Aticks = ATim / NANOSECONDS_PER_TICK + TICKS_TO_UNIX_EPOCH;
      AFileTime.dwLowDateTime = static_cast<DWORD>(Aticks % 0x100000000ULL);
      AFileTime.dwHighDateTime = static_cast<DWORD>(Aticks / 0x100000000ULL);
    } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
      GetSystemTimeAsFileTime(&AFileTime);
    }

    // For setting modification time
    if (FstFlags & __WASI_FSTFLAGS_MTIM) {
      uint64_t Mticks = MTim / NANOSECONDS_PER_TICK + TICKS_TO_UNIX_EPOCH;
      MFileTime.dwLowDateTime = static_cast<DWORD>(Mticks % 0x100000000ULL);
      MFileTime.dwHighDateTime = static_cast<DWORD>(Mticks / 0x100000000ULL);
    } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
      GetSystemTimeAsFileTime(&MFileTime);
    }

    if (unlikely(SetFileTime(LocalFileHandle, nullptr, &AFileTime,
                             &MFileTime)) == FALSE) {
      CloseHandle(LocalFileHandle);
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    CloseHandle(LocalFileHandle);
    return {};
  }
  return WasiUnexpect(fromWinError(GetLastError()));
}

WasiExpect<void> INode::pathLink(const INode &Old, std::string OldPath,
                                 const INode &New,
                                 std::string NewPath) noexcept {

  wchar_t OldFullPathW[MAX_PATH];
  wchar_t NewFullPathW[MAX_PATH];

  if (PathIsRelativeA(OldPath.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the paths of the handle
    if (unlikely(GetFinalPathNameByHandleW(Old.Handle, HandleFullPathW,
                                           MAX_PATH, FILE_NAME_NORMALIZED)) ==
        FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t OldPathW[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(OldPathW, OldPath.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(OldFullPathW, MAX_PATH, HandleFullPathW, OldPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(OldFullPathW, OldPath.c_str(), MAX_PATH);
  }

  if (PathIsRelativeA(NewPath.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the paths of the handle
    if (unlikely(GetFinalPathNameByHandleW(New.Handle, HandleFullPathW,
                                           MAX_PATH, FILE_NAME_NORMALIZED)) ==
        FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t NewPathW[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(NewPathW, OldPath.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(NewFullPathW, MAX_PATH, HandleFullPathW, NewPathW);
    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(NewFullPathW, NewPath.c_str(), MAX_PATH);
  }

  // Create the hard link from the paths
  if (unlikely(CreateHardLinkW(NewFullPathW, OldFullPathW, nullptr)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<INode> INode::pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                                  __wasi_fdflags_t FdFlags,
                                  uint8_t VFSFlags) const noexcept {
  wchar_t FullPathW[MAX_PATH];

  if (PathIsRelativeA(Path.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the paths of the handles
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPathW, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t PathW[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(PathW, Path.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW, PathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(FullPathW, Path.c_str(), MAX_PATH);
  }

  DWORD AttributeFlags = attributeFlags(OpenFlags, FdFlags);
  DWORD AccessFlags = accessFlags(FdFlags, VFSFlags);
  DWORD CreationDisposition = creationDisposition(OpenFlags);

  HANDLE FileHandle =
      CreateFileW(FullPathW, AccessFlags,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr, CreationDisposition, AttributeFlags, nullptr);
  if (unlikely(FileHandle == INVALID_HANDLE_VALUE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  } else {
    INode New(FileHandle);
    return New;
  }
}

WasiExpect<void> INode::pathReadlink(std::string Path, Span<char> Buffer,
                                     __wasi_size_t &NRead) const noexcept {

  wchar_t FullPathW[MAX_PATH];

  if (PathIsRelativeA(Path.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];
    // First get the paths of the handles
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPathW, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t PathSuffixW[MAX_PATH];

    // Convert the paths from char_t to wchar_t
    mbstowcs(PathSuffixW, Path.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(FullPathW, MAX_PATH, HandleFullPathW, PathSuffixW);
    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(FullPathW, Path.c_str(), MAX_PATH);
  }

  // Fill the Buffer with the contents of the link
  HANDLE LocalFileHandle;

  LocalFileHandle = CreateFileW(FullPathW, GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, 0, nullptr);

  if (likely(LocalFileHandle != INVALID_HANDLE_VALUE)) {
    if (unlikely(GetFinalPathNameByHandleA(LocalFileHandle, Buffer.data(),
                                           static_cast<uint32_t>(Buffer.size()),
                                           FILE_NAME_NORMALIZED)) != FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    NRead = static_cast<uint32_t>(Buffer.size());
    CloseHandle(LocalFileHandle);
    return {};
  }

  return WasiUnexpect(fromWinError(GetLastError()));
}

WasiExpect<void> INode::pathRemoveDirectory(std::string Path) const noexcept {
  if (RemoveDirectoryA(Path.c_str()) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::pathRename(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept {

  wchar_t OldFullPathW[MAX_PATH];
  wchar_t NewFullPathW[MAX_PATH];

  if (PathIsRelativeA(OldPath.c_str())) {
    wchar_t HandleFullPath[MAX_PATH];

    // First get the paths of the handles
    if (unlikely(GetFinalPathNameByHandleW(Old.Handle, HandleFullPath, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPath) && !OldPath.empty()) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t OldPathW[MAX_PATH];

    // Convert the paths from char_t to wchar_t
    mbstowcs(OldPathW, OldPath.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(OldFullPathW, MAX_PATH, OldFullPathW, OldPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(OldFullPathW, OldPath.c_str(), MAX_PATH);
  }

  if (PathIsRelativeA(NewPath.c_str())) {
    wchar_t HandleFullPathW[MAX_PATH];

    // First get the paths of the handles
    if (unlikely(GetFinalPathNameByHandleW(New.Handle, HandleFullPathW,
                                           MAX_PATH, FILE_NAME_NORMALIZED)) ==
        FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }

    if (!PathIsDirectoryW(HandleFullPathW) && !NewPath.empty()) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }

    wchar_t NewPathW[MAX_PATH];

    // Convert the paths from char_t to wchar_t
    mbstowcs(NewPathW, NewPath.c_str(), MAX_PATH);

    // Append the paths together
    HRESULT CombineResult =
        PathCchCombine(NewFullPathW, MAX_PATH, HandleFullPathW, NewPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }

  } else {
    mbstowcs(NewFullPathW, NewPath.c_str(), MAX_PATH);
  }

  // Rename the file from the paths
  if (unlikely(MoveFileW(OldFullPathW, NewFullPathW)) == FALSE) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<void> INode::pathSymlink(std::string OldPath,
                                    std::string NewPath) const noexcept {

  wchar_t OldFullPathW[MAX_PATH];
  wchar_t NewFullPathW[MAX_PATH];

  if (PathIsRelativeA(OldPath.c_str())) {
    wchar_t OldPathW[MAX_PATH];
    wchar_t HandleFullPath[MAX_PATH];

    // Convert the paths from char_t to wchar_t
    mbstowcs(OldPathW, OldPath.c_str(), MAX_PATH);

    // First get the paths of the handle
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPath, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    // If check it is a directory or not
    if (!PathIsDirectoryW(HandleFullPath) && !OldPath.empty()) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }
    HRESULT CombineResult =
        PathCchCombine(OldFullPathW, MAX_PATH, HandleFullPath, OldPathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
  }
  if (PathIsRelativeA(NewPath.c_str())) {
    wchar_t NewPathW[MAX_PATH];
    wchar_t HandleFullPath[MAX_PATH];

    // Convert the path from char_t to wchar_t
    mbstowcs(NewPathW, NewPath.c_str(), MAX_PATH);

    // First get the path of the handle
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPath, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    __wasi_fdstat_t HandleStat;
    fdFdstatGet(HandleStat);

    // Remove file names if the handle refers to a file
    if (!PathIsDirectoryW(HandleFullPath) && !OldPath.empty()) {
      PathCchRemoveFileSpec(HandleFullPath, MAX_PATH);
    }
    HRESULT CombineResult =
        PathCchCombine(NewFullPathW, MAX_PATH, HandleFullPath, NewPathW);
    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
  }
  DWORD TargetType = 0;
  if (PathIsDirectoryW(OldFullPathW)) {
    TargetType = SYMBOLIC_LINK_FLAG_DIRECTORY;
  }

  if (unlikely(CreateSymbolicLinkW(NewFullPathW, OldFullPathW, TargetType))) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<void> INode::pathUnlinkFile(std::string Path) const noexcept {

  wchar_t PathFullW[MAX_PATH];

  if (PathIsRelativeA(Path.c_str())) {
    wchar_t PathW[MAX_PATH];
    wchar_t HandleFullPath[MAX_PATH];

    // Convert the paths from char_t to wchar_t
    mbstowcs(PathW, Path.c_str(), MAX_PATH);

    // First get the paths of the handle
    if (unlikely(GetFinalPathNameByHandleW(Handle, HandleFullPath, MAX_PATH,
                                           FILE_NAME_NORMALIZED)) == FALSE) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    if (!PathIsDirectoryW(HandleFullPath)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    HRESULT CombineResult =
        PathCchCombine(PathFullW, MAX_PATH, HandleFullPath, PathW);

    switch (CombineResult) {
    case S_OK:
      break;
    case E_INVALIDARG:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    case E_OUTOFMEMORY:
      return WasiUnexpect(__WASI_ERRNO_OVERFLOW);
    default:
      return WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
  }

  if (PathIsDirectoryW(PathFullW)) {
    if (unlikely(RemoveDirectoryW(PathFullW) == FALSE)) {
      return WasiUnexpect(fromWinError(GetLastError()));
    }
    return {};
  }

  if (unlikely(DeleteFileW(PathFullW) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }

  return {};
}

WasiExpect<Poller> INode::pollOneoff(__wasi_size_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<Epoller> INode::epollOneoff(__wasi_size_t, int) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

static bool EnsureWSAStartup() {
  static bool WSALoad = false;
  static WSADATA WSAData;

  if (!WSALoad) {
    int Err = WSAStartup(MAKEWORD(2, 2), &WSAData);
    if (Err == 0) {
      WSALoad = true;
    }
  }

  return WSALoad;
}

WasiExpect<void> INode::getAddrinfo(std::string_view Node,
                                    std::string_view Service,
                                    const __wasi_addrinfo_t &Hint,
                                    uint32_t MaxResLength,
                                    Span<__wasi_addrinfo_t *> WasiAddrinfoArray,
                                    Span<__wasi_sockaddr_t *> WasiSockaddrArray,
                                    Span<char *> AiAddrSaDataArray,
                                    Span<char *> AiCanonnameArray,
                                    /*Out*/ __wasi_size_t &ResLength) noexcept {
  const auto [NodeCStr, NodeBuf] = createNullTerminatedString(Node);
  const auto [ServiceCStr, ServiceBuf] = createNullTerminatedString(Service);

  struct addrinfo SysHint;
  SysHint.ai_flags = toAIFlags(Hint.ai_flags);
  SysHint.ai_family = toAddressFamily(Hint.ai_family);
  SysHint.ai_socktype = toSockType(Hint.ai_socktype);
  SysHint.ai_protocol = toProtocol(Hint.ai_protocol);
  SysHint.ai_addrlen = Hint.ai_addrlen;
  SysHint.ai_addr = nullptr;
  SysHint.ai_canonname = nullptr;
  SysHint.ai_next = nullptr;

  struct addrinfo *SysResPtr = nullptr;
  if (auto Res = ::getaddrinfo(NodeCStr, ServiceCStr, &SysHint, &SysResPtr);
      unlikely(Res != 0)) {
    // By MSDN, on failure, getaddrinfo returns a nonzero Windows Sockets error
    // code.
    return WasiUnexpect(fromWSAToEAIError(Res));
  }
  // calculate ResLength
  if (ResLength = calculateAddrinfoLinkedListSize(SysResPtr);
      ResLength > MaxResLength) {
    ResLength = MaxResLength;
  }

  struct addrinfo *SysResItem = SysResPtr;
  for (uint32_t Idx = 0; Idx < ResLength; Idx++) {
    auto &CurAddrinfo = WasiAddrinfoArray[Idx];
    CurAddrinfo->ai_flags = fromAIFlags(SysResItem->ai_flags);
    CurAddrinfo->ai_socktype = fromSockType(SysResItem->ai_socktype);
    CurAddrinfo->ai_protocol = fromProtocol(SysResItem->ai_protocol);
    CurAddrinfo->ai_family = fromAddressFamily(SysResItem->ai_family);
    CurAddrinfo->ai_addrlen = static_cast<uint32_t>(SysResItem->ai_addrlen);

    // process ai_canonname in addrinfo
    if (SysResItem->ai_canonname != nullptr) {
      CurAddrinfo->ai_canonname_len =
          static_cast<uint32_t>(std::strlen(SysResItem->ai_canonname));
      auto &CurAiCanonname = AiCanonnameArray[Idx];
      std::memcpy(CurAiCanonname, SysResItem->ai_canonname,
                  CurAddrinfo->ai_canonname_len + 1);
    } else {
      CurAddrinfo->ai_canonname_len = 0;
    }

    // process socket address
    if (SysResItem->ai_addrlen > 0) {
      auto &CurSockaddr = WasiSockaddrArray[Idx];
      CurSockaddr->sa_family =
          fromAddressFamily(SysResItem->ai_addr->sa_family);

      // process sa_data in socket address
      size_t SaSize = 0;
      switch (CurSockaddr->sa_family) {
      case __wasi_address_family_t::__WASI_ADDRESS_FAMILY_INET4:
        SaSize = sizeof(sockaddr_in) - sizeof(sockaddr_in::sin_family);
        break;
      case __wasi_address_family_t::__WASI_ADDRESS_FAMILY_INET6:
        SaSize = sizeof(sockaddr_in6) - sizeof(sockaddr_in6::sin6_family);
        break;
      default:
        assumingUnreachable();
      }
      std::memcpy(AiAddrSaDataArray[Idx], SysResItem->ai_addr->sa_data, SaSize);
      CurSockaddr->sa_data_len = __wasi_size_t(SaSize);
    }
    // process ai_next in addrinfo
    SysResItem = SysResItem->ai_next;
  }
  ::freeaddrinfo(SysResPtr);

  return {};
}

WasiExpect<INode> INode::sockOpen(__wasi_address_family_t AddressFamily,
                                  __wasi_sock_type_t SockType) noexcept {
  EnsureWSAStartup();

  int SysProtocol = IPPROTO_IP;

  int SysDomain = 0;
  int SysType = 0;

  switch (AddressFamily) {
  case __WASI_ADDRESS_FAMILY_INET4:
    SysDomain = AF_INET;
    break;
  case __WASI_ADDRESS_FAMILY_INET6:
    SysDomain = AF_INET6;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  switch (SockType) {
  case __WASI_SOCK_TYPE_SOCK_DGRAM:
    SysType = SOCK_DGRAM;
    break;
  case __WASI_SOCK_TYPE_SOCK_STREAM:
    SysType = SOCK_STREAM;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  if (auto NewSock = ::socket(SysDomain, SysType, SysProtocol);
      unlikely(NewSock == INVALID_SOCKET)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    INode New(reinterpret_cast<boost::winapi::HANDLE_>(NewSock));
    return New;
  }
}

WasiExpect<void> INode::sockBind(uint8_t *AddressBuf,
                                 [[maybe_unused]] uint8_t AddressLength,
                                 uint16_t Port) noexcept {
  EnsureWSAStartup();

  int AddrFamily;
  uint8_t *Address;

  if (AddressLength != 128) {
    // Fallback
    switch (AddressLength) {
    case 4:
      AddrFamily = AF_INET;
      break;
    case 16:
      AddrFamily = AF_INET6;
      break;
    default:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    Address = AddressBuf;
  } else {
    AddrFamily = toAddressFamily(*getAddressFamily(AddressBuf));
    Address = getAddress(AddressBuf);
  }

  struct sockaddr_in ServerAddr4 = {};
  struct sockaddr_in6 ServerAddr6 = {};
  struct sockaddr *ServerAddr = nullptr;
  int RealSize = 0;

  if (AddrFamily == AF_INET) {
    ServerAddr = reinterpret_cast<struct sockaddr *>(&ServerAddr4);
    RealSize = sizeof(ServerAddr4);

    ServerAddr4.sin_family = AF_INET;
    ServerAddr4.sin_port = htons(Port);
    std::memcpy(&ServerAddr4.sin_addr, Address, sizeof(in_addr));
  } else if (AddrFamily == AF_INET6) {
    ServerAddr = reinterpret_cast<struct sockaddr *>(&ServerAddr6);
    RealSize = sizeof(ServerAddr6);

    ServerAddr6.sin6_family = AF_INET6;
    ServerAddr6.sin6_port = htons(Port);
    ServerAddr6.sin6_flowinfo = 0;
    std::memcpy(&ServerAddr6.sin6_addr.s6_addr, Address, sizeof(in6_addr));
  }

  if (auto Res = ::bind(toSocket(Handle), ServerAddr, RealSize);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }
  return {};
}

WasiExpect<void> INode::sockListen(int32_t Backlog) noexcept {
  EnsureWSAStartup();
  if (auto Res = ::listen(toSocket(Handle), Backlog);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }
  return {};
}

WasiExpect<INode> INode::sockAccept() noexcept {
  EnsureWSAStartup();
  struct sockaddr_in ServerSocketAddr;
  ServerSocketAddr.sin_family = AF_INET;
  ServerSocketAddr.sin_addr.s_addr = INADDR_ANY;
  socklen_t AddressLen = sizeof(ServerSocketAddr);

  if (auto NewSock = ::accept(
          toSocket(Handle),
          reinterpret_cast<struct sockaddr *>(&ServerSocketAddr), &AddressLen);
      unlikely(NewSock == INVALID_SOCKET)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    INode New(reinterpret_cast<boost::winapi::HANDLE_>(NewSock));
    return New;
  }
}

WasiExpect<void> INode::sockConnect(uint8_t *AddressBuf,
                                    [[maybe_unused]] uint8_t AddressLength,
                                    uint16_t Port) noexcept {
  EnsureWSAStartup();

  int AddrFamily;
  uint8_t *Address;

  if (AddressLength != 128) {
    // Fallback
    switch (AddressLength) {
    case 4:
      AddrFamily = AF_INET;
      break;
    case 16:
      AddrFamily = AF_INET6;
      break;
    default:
      return WasiUnexpect(__WASI_ERRNO_INVAL);
    }
    Address = AddressBuf;
  } else {
    AddrFamily = toAddressFamily(*getAddressFamily(AddressBuf));
    Address = getAddress(AddressBuf);
  }

  struct sockaddr_in ClientAddr4 {};
  struct sockaddr_in6 ClientAddr6 {};
  struct sockaddr *ClientAddr = nullptr;
  int RealSize = 0;

  if (AddrFamily == AF_INET) {
    ClientAddr = reinterpret_cast<struct sockaddr *>(&ClientAddr4);
    RealSize = sizeof(ClientAddr4);

    ClientAddr4.sin_family = AF_INET;
    ClientAddr4.sin_port = htons(Port);
    std::memcpy(&ClientAddr4.sin_addr, Address, sizeof(in_addr));
  } else if (AddrFamily == AF_INET6) {
    ClientAddr = reinterpret_cast<struct sockaddr *>(&ClientAddr6);
    RealSize = sizeof(ClientAddr6);

    ClientAddr6.sin6_family = AF_INET6;
    ClientAddr6.sin6_flowinfo = 0;
    ClientAddr6.sin6_port = htons(Port);
    std::memcpy(&ClientAddr6.sin6_addr, Address, sizeof(in6_addr));
  }

  if (auto Res = ::connect(toSocket(Handle), ClientAddr, RealSize);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }
  return {};
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  return sockRecvFrom(RiData, RiFlags, nullptr, 0, nullptr, NRead, RoFlags);
}

WasiExpect<void> INode::sockRecvFrom(Span<Span<uint8_t>> RiData,
                                     __wasi_riflags_t RiFlags,
                                     uint8_t *AddressBuf,
                                     [[maybe_unused]] uint8_t AddressLength,
                                     uint32_t *PortPtr, __wasi_size_t &NRead,
                                     __wasi_roflags_t &RoFlags) const noexcept {
  EnsureWSAStartup();
  // recvmsg is not available on WINDOWS. fall back to call recvfrom

  uint8_t *Address = nullptr;
  __wasi_address_family_t *AddrFamily = nullptr;
  __wasi_address_family_t Dummy; // Write garbage on fallback mode.

  if (AddressBuf) {
    if (AddressLength != 128) {
      // Fallback
      AddrFamily = &Dummy;
      Address = AddressBuf;
    } else {
      AddrFamily = getAddressFamily(AddressBuf);
      Address = getAddress(AddressBuf);
    }
  }

  int SysRiFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    SysRiFlags |= MSG_PEEK;
  }
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    SysRiFlags |= MSG_WAITALL;
  }

  std::size_t TmpBufSize = 0;
  for (auto &IOV : RiData) {
    TmpBufSize += IOV.size();
  }

  std::vector<uint8_t> TmpBuf(TmpBufSize, 0);

  sockaddr_storage SockAddrStorage;
  int MaxAllowLength = sizeof(SockAddrStorage);

  if (auto Res = ::recvfrom(
          toSocket(Handle), reinterpret_cast<char *>(TmpBuf.data()),
          static_cast<int>(TmpBufSize), SysRiFlags,
          reinterpret_cast<sockaddr *>(&SockAddrStorage), &MaxAllowLength);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    NRead = static_cast<__wasi_size_t>(Res);
  }

  if (AddressBuf) {
    *AddrFamily = fromAddressFamily(SockAddrStorage.ss_family);
    if (SockAddrStorage.ss_family == AF_INET) {
      std::memcpy(Address,
                  &reinterpret_cast<sockaddr_in *>(&SockAddrStorage)->sin_addr,
                  sizeof(in_addr));
    } else if (SockAddrStorage.ss_family == AF_INET6) {
      std::memcpy(
          Address,
          &reinterpret_cast<sockaddr_in6 *>(&SockAddrStorage)->sin6_addr,
          sizeof(in6_addr));
    }
  }

  if (PortPtr) {
    *AddrFamily = fromAddressFamily(SockAddrStorage.ss_family);
    if (SockAddrStorage.ss_family == AF_INET) {
      *PortPtr = reinterpret_cast<sockaddr_in *>(&SockAddrStorage)->sin_port;
    } else if (SockAddrStorage.ss_family == AF_INET6) {
      *PortPtr = reinterpret_cast<sockaddr_in6 *>(&SockAddrStorage)->sin6_port;
    }
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);
  // TODO : check MSG_TRUNC

  size_t BeginIdx = 0;
  for (auto &IOV : RiData) {
    std::copy(TmpBuf.data() + BeginIdx, TmpBuf.data() + BeginIdx + IOV.size(),
              IOV.begin());
    BeginIdx += IOV.size();
  }

  return {};
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t SiFlags,
                                 __wasi_size_t &NWritten) const noexcept {
  return sockSendTo(SiData, SiFlags, nullptr, 0, 0, NWritten);
}

WasiExpect<void> INode::sockSendTo(Span<Span<const uint8_t>> SiData,
                                   __wasi_siflags_t, uint8_t *AddressBuf,
                                   [[maybe_unused]] uint8_t AddressLength,
                                   int32_t Port,
                                   __wasi_size_t &NWritten) const noexcept {
  EnsureWSAStartup();
  // sendmsg is not available on WINDOWS. fall back to call sendto
  int SysSiFlags = 0;

  uint8_t *Address = nullptr;
  int AddrFamily = 0;

  if (AddressBuf) {
    if (AddressLength != 128) {
      // Fallback
      switch (AddressLength) {
      case 4:
        AddrFamily = AF_INET;
        break;
      case 16:
        AddrFamily = AF_INET6;
        break;
      default:
        return WasiUnexpect(__WASI_ERRNO_INVAL);
      }
      Address = AddressBuf;
    } else {
      AddrFamily = toAddressFamily(*getAddressFamily(AddressBuf));
      Address = getAddress(AddressBuf);
    }
  }

  std::vector<uint8_t> TmpBuf;
  for (auto &IOV : SiData) {
    copy(IOV.begin(), IOV.end(), std::back_inserter(TmpBuf));
  }
  std::size_t TmpBufSize = TmpBuf.size();

  struct sockaddr_in ClientAddr4 = {};
  struct sockaddr_in6 ClientAddr6 = {};
  struct sockaddr *ClientAddr = nullptr;
  socklen_t RealSize = 0;

  if (Address) {
    if (AddrFamily == AF_INET) {
      ClientAddr = reinterpret_cast<struct sockaddr *>(&ClientAddr4);
      RealSize = sizeof(ClientAddr4);

      ClientAddr4.sin_family = AF_INET;
      ClientAddr4.sin_port = htons(static_cast<u_short>(Port));
      std::memcpy(&ClientAddr4.sin_addr, Address, sizeof(in_addr));
    } else if (AddrFamily == AF_INET6) {
      ClientAddr = reinterpret_cast<struct sockaddr *>(&ClientAddr6);
      RealSize = sizeof(ClientAddr6);

      ClientAddr6.sin6_family = AF_INET6;
      ClientAddr6.sin6_flowinfo = 0;
      ClientAddr6.sin6_port = htons(static_cast<u_short>(Port));
      std::memcpy(&ClientAddr6.sin6_addr, Address, sizeof(in6_addr));
    }
  }

  if (auto Res = ::sendto(
          toSocket(Handle), reinterpret_cast<char *>(TmpBuf.data()),
          static_cast<int>(TmpBufSize), SysSiFlags, ClientAddr, RealSize);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  } else {
    NWritten = static_cast<__wasi_size_t>(Res);
  }

  return {};
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
  EnsureWSAStartup();
  int SysFlags = 0;
  if (SdFlags == __WASI_SDFLAGS_RD) {
    SysFlags = SD_RECEIVE;
  } else if (SdFlags == __WASI_SDFLAGS_WR) {
    SysFlags = SD_SEND;
  } else if (SdFlags == (__WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR)) {
    SysFlags = SD_BOTH;
  }

  if (auto Res = ::shutdown(toSocket(Handle), SysFlags);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  return {};
}

WasiExpect<void> INode::sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   void *FlagPtr,
                                   uint32_t *FlagSizePtr) const noexcept {
  EnsureWSAStartup();
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  auto UnsafeFlagSizePtr = reinterpret_cast<int *>(FlagSizePtr);
  if (SockOptName == __WASI_SOCK_OPT_SO_ERROR) {
    char ErrorCode = 0;
    int *WasiErrorPtr = static_cast<int *>(FlagPtr);
    if (auto Res = ::getsockopt(toSocket(Handle), SysSockOptLevel,
                                SysSockOptName, &ErrorCode, UnsafeFlagSizePtr);
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
    *WasiErrorPtr = fromErrNo(ErrorCode);
  } else {
    char *CFlagPtr = static_cast<char *>(FlagPtr);
    if (auto Res = ::getsockopt(toSocket(Handle), SysSockOptLevel,
                                SysSockOptName, CFlagPtr, UnsafeFlagSizePtr);
        unlikely(Res == SOCKET_ERROR)) {
      return WasiUnexpect(fromWSALastError(WSAGetLastError()));
    }
  }

  return {};
}

WasiExpect<void> INode::sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   void *FlagPtr,
                                   uint32_t FlagSize) const noexcept {
  EnsureWSAStartup();
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  char *CFlagPtr = static_cast<char *>(FlagPtr);
  auto UnsafeFlagSize = static_cast<int>(FlagSize);

  if (auto Res = ::setsockopt(toSocket(Handle), SysSockOptLevel, SysSockOptName,
                              CFlagPtr, UnsafeFlagSize);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  return {};
}

WasiExpect<void> INode::sockGetLocalAddr(uint8_t *AddressBufPtr,
                                         uint32_t *PortPtr) const noexcept {
  EnsureWSAStartup();

  auto AddrFamilyPtr = getAddressFamily(AddressBufPtr);
  auto AddressPtr = getAddress(AddressBufPtr);

  struct sockaddr_storage SocketAddr;
  socklen_t Slen = sizeof(SocketAddr);
  std::memset(&SocketAddr, 0, sizeof(SocketAddr));

  if (auto Res = ::getsockname(
          toSocket(Handle), reinterpret_cast<sockaddr *>(&SocketAddr), &Slen);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  if (SocketAddr.ss_family == AF_INET) {
    auto SocketAddrv4 = reinterpret_cast<struct sockaddr_in *>(&SocketAddr);

    *AddrFamilyPtr = fromAddressFamily(AF_INET);
    *PortPtr = ntohs(SocketAddrv4->sin_port);
    std::memcpy(AddressPtr, &SocketAddrv4->sin_addr, sizeof(in_addr));
  } else if (SocketAddr.ss_family == AF_INET6) {
    auto SocketAddrv6 = reinterpret_cast<struct sockaddr_in6 *>(&SocketAddr);

    *AddrFamilyPtr = fromAddressFamily(AF_INET6);
    *PortPtr = ntohs(SocketAddrv6->sin6_port);
    std::memcpy(AddressPtr, &SocketAddrv6->sin6_addr, sizeof(in_addr6));
  } else {
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }

  return {};
}

WasiExpect<void> INode::sockGetPeerAddr(uint8_t *AddressBufPtr,
                                        uint32_t *PortPtr) const noexcept {
  EnsureWSAStartup();

  auto AddrFamilyPtr = getAddressFamily(AddressBufPtr);
  auto AddressPtr = getAddress(AddressBufPtr);

  struct sockaddr_storage SocketAddr;
  socklen_t Slen = sizeof(SocketAddr);
  std::memset(&SocketAddr, 0, sizeof(SocketAddr));

  if (auto Res = ::getpeername(
          toSocket(Handle), reinterpret_cast<sockaddr *>(&SocketAddr), &Slen);
      unlikely(Res == SOCKET_ERROR)) {
    return WasiUnexpect(fromWSALastError(WSAGetLastError()));
  }

  if (SocketAddr.ss_family == AF_INET) {
    auto SocketAddrv4 = reinterpret_cast<struct sockaddr_in *>(&SocketAddr);

    *AddrFamilyPtr = fromAddressFamily(AF_INET);
    *PortPtr = ntohs(SocketAddrv4->sin_port);
    std::memcpy(AddressPtr, &SocketAddrv4->sin_addr, sizeof(in_addr));
  } else if (SocketAddr.ss_family == AF_INET6) {
    auto SocketAddrv6 = reinterpret_cast<struct sockaddr_in6 *>(&SocketAddr);

    *AddrFamilyPtr = fromAddressFamily(AF_INET6);
    *PortPtr = ntohs(SocketAddrv6->sin6_port);
    std::memcpy(AddressPtr, &SocketAddrv6->sin6_addr, sizeof(in_addr6));
  } else {
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
  return {};
}

__wasi_filetype_t INode::unsafeFiletype() const noexcept {

  // TODO: Find equivalents to the other file types
  // To be completed along with other similar functions

  if (unlikely(GetFileInformationByHandle(Handle, &(*FileInfo))) == FALSE) {
    return __WASI_FILETYPE_UNKNOWN;
  }
  return fromFileType((*FileInfo).dwFileAttributes, GetFileType(Handle));
}

WasiExpect<__wasi_filetype_t> INode::filetype() const noexcept {

  // TODO: Find equivalents to the other file types
  // To be completed along with other similar functions

  return unsafeFiletype();
}

WasiExpect<void> INode::updateFileInfo() const noexcept {
  FileInfo.emplace();
  if (unlikely(GetFileInformationByHandle(Handle, &(*FileInfo)) == FALSE)) {
    return WasiUnexpect(fromWinError(GetLastError()));
  }
  return {};
}

bool INode::isDirectory() const noexcept {
  updateFileInfo();
  return (*FileInfo).dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY;
}

bool INode::isSymlink() const noexcept {
  updateFileInfo();
  return (*FileInfo).dwFileAttributes == FILE_ATTRIBUTE_REPARSE_POINT;
}

WasiExpect<__wasi_filesize_t> INode::filesize() const noexcept {
  updateFileInfo();
  return static_cast<uint64_t>((*FileInfo).nFileSizeLow) +
         (static_cast<uint64_t>((*FileInfo).nFileSizeHigh) << 32);
}

bool INode::canBrowse() const noexcept { return false; }

Poller::Poller(__wasi_size_t Count) { Events.reserve(Count); }

WasiExpect<void> Poller::clock(__wasi_clockid_t, __wasi_timestamp_t,
                               __wasi_timestamp_t, __wasi_subclockflags_t,
                               __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::read(const INode &, __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::write(const INode &, __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Poller::wait(CallbackType) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

Epoller::Epoller(__wasi_size_t Count, int) { Events.reserve(Count); }

WasiExpect<void> Epoller::clock(__wasi_clockid_t, __wasi_timestamp_t,
                                __wasi_timestamp_t, __wasi_subclockflags_t,
                                __wasi_userdata_t) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Epoller::read(const INode &, __wasi_userdata_t,
                               std::unordered_map<int, uint32_t> &) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Epoller::write(const INode &, __wasi_userdata_t,
                                std::unordered_map<int, uint32_t> &) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

WasiExpect<void> Epoller::wait(CallbackType,
                               std::unordered_map<int, uint32_t> &) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
