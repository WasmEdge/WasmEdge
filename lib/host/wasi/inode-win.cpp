// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#if WASMEDGE_OS_WINDOWS

#include "common/errcode.h"
#include "common/variant.h"
#include "host/wasi/clock.h"
#include "host/wasi/environ.h"
#include "host/wasi/inode.h"
#include "host/wasi/vfs.h"
#include "win.h"
#include <algorithm>
#include <cstddef>
#include <new>
#include <numeric>
#include <vector>

using namespace WasmEdge::winapi;

namespace WasmEdge {
namespace Host {
namespace WASI {

namespace {

inline void WASMEDGE_WINAPI_WINAPI_CC
EmptyOverlappedCompletionRoutine(DWORD_, DWORD_, LPOVERLAPPED_) noexcept {}

#if WINAPI_PARTITION_DESKTOP
inline constexpr uint64_t combineHighLow(uint32_t HighPart,
                                         uint32_t LowPart) noexcept {
  const ULARGE_INTEGER_ Temp = {/* LowPart */ LowPart, /* HighPart */ HighPart};
  return Temp.QuadPart;
}
#endif

inline constexpr __wasi_size_t
calculateAddrinfoLinkedListSize(struct addrinfo *const Addrinfo) noexcept {
  __wasi_size_t Length = 0;
  for (struct addrinfo *TmpPointer = Addrinfo; TmpPointer != nullptr;
       TmpPointer = TmpPointer->ai_next) {
    Length++;
  }
  return Length;
};

union UniversalAddress {
  struct {
    uint16_t AddressFamily;
    uint8_t Address[128 - sizeof(uint16_t)];
  };
  uint8_t Buffer[128];
};
static_assert(sizeof(UniversalAddress) == 128);

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

WasiExpect<std::tuple<DWORD_, DWORD_, DWORD_>> inline constexpr getOpenFlags(
    __wasi_oflags_t OpenFlags, __wasi_fdflags_t FdFlags,
    VFS::Flags VFSFlags) noexcept {
  // Always use FILE_FLAG_BACKUP_SEMANTICS to prevent failure on opening a
  // directory.
  DWORD_ AttributeFlags =
      FILE_FLAG_BACKUP_SEMANTICS_ | FILE_FLAG_OPEN_REPARSE_POINT_;

  // Source: https://devblogs.microsoft.com/oldnewthing/20210729-00/?p=105494
  if (FdFlags &
      (__WASI_FDFLAGS_SYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_DSYNC)) {
    AttributeFlags |= FILE_FLAG_WRITE_THROUGH_;
    FdFlags &=
        ~(__WASI_FDFLAGS_SYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_DSYNC);
  }
  if (FdFlags & __WASI_FDFLAGS_NONBLOCK) {
    // Ignore NONBLOCK flag
    FdFlags &= ~__WASI_FDFLAGS_NONBLOCK;
  }
  if (OpenFlags & __WASI_OFLAGS_DIRECTORY) {
    AttributeFlags |= FILE_ATTRIBUTE_DIRECTORY_;
    OpenFlags &= ~__WASI_OFLAGS_DIRECTORY;
  } else {
    AttributeFlags |= FILE_FLAG_OVERLAPPED_;
  }

  DWORD_ AccessFlags = 0;
  if (FdFlags & __WASI_FDFLAGS_APPEND) {
    AccessFlags |= FILE_APPEND_DATA_;
    FdFlags &= ~__WASI_FDFLAGS_APPEND;
  }
  if ((VFSFlags & VFS::Read) || (VFSFlags == 0)) {
    AccessFlags |= FILE_GENERIC_READ_;
  }
  if (VFSFlags & VFS::Write) {
    AccessFlags |= FILE_GENERIC_WRITE_;
  }

  if (FdFlags) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  if (OpenFlags &
      ~(__WASI_OFLAGS_CREAT | __WASI_OFLAGS_EXCL | __WASI_OFLAGS_TRUNC)) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  DWORD_ CreationDisposition = 0;
  switch (static_cast<uint16_t>(OpenFlags)) {
  case __WASI_OFLAGS_CREAT | __WASI_OFLAGS_EXCL:
  case __WASI_OFLAGS_CREAT | __WASI_OFLAGS_EXCL | __WASI_OFLAGS_TRUNC:
    CreationDisposition = CREATE_NEW_;
    break;
  case __WASI_OFLAGS_CREAT | __WASI_OFLAGS_TRUNC:
    CreationDisposition = CREATE_ALWAYS_;
    break;
  case __WASI_OFLAGS_CREAT:
    CreationDisposition = OPEN_ALWAYS_;
    break;
  case 0:
  case __WASI_OFLAGS_EXCL:
    CreationDisposition = OPEN_EXISTING_;
    break;
  case __WASI_OFLAGS_TRUNC:
  case __WASI_OFLAGS_EXCL | __WASI_OFLAGS_TRUNC:
    if (VFSFlags & VFS::Write) {
      CreationDisposition = TRUNCATE_EXISTING_;
    } else {
      CreationDisposition = OPEN_EXISTING_;
    }
    break;
  default:
    assumingUnreachable();
  }

  return std::tuple{AttributeFlags, AccessFlags, CreationDisposition};
}

inline DWORD_ fastGetFileType(HandleHolder::HandleType Type,
                              HANDLE_ Handle) noexcept {
  switch (Type) {
  case HandleHolder::HandleType::NormalHandle:
    return FILE_TYPE_DISK_;
  case HandleHolder::HandleType::NormalSocket:
    return FILE_TYPE_PIPE_;
  case HandleHolder::HandleType::StdHandle:
    return GetFileType(Handle);
  default:
    assumingUnreachable();
  }
}

inline __wasi_filetype_t getDiskFileType(DWORD_ Attribute) noexcept {
  if (Attribute & FILE_ATTRIBUTE_REPARSE_POINT_) {
    return __WASI_FILETYPE_SYMBOLIC_LINK;
  }
  if (Attribute & FILE_ATTRIBUTE_DIRECTORY_) {
    return __WASI_FILETYPE_DIRECTORY;
  }
  return __WASI_FILETYPE_REGULAR_FILE;
}

inline __wasi_filetype_t getSocketType(SOCKET_ Socket) noexcept {
  int SocketType = 0;
  int Size = sizeof(SocketType);
  if (likely(getsockopt(Socket, SOL_SOCKET, SO_TYPE,
                        reinterpret_cast<char *>(&SocketType), &Size) == 0)) {
    switch (SocketType) {
    case SOCK_STREAM:
      return __WASI_FILETYPE_SOCKET_STREAM;
    case SOCK_DGRAM:
      return __WASI_FILETYPE_SOCKET_DGRAM;
    }
  }
  return __WASI_FILETYPE_UNKNOWN;
}

inline WasiExpect<DWORD_> getAttribute(HANDLE_ Handle) noexcept {
#if NTDDI_VERSION >= NTDDI_VISTA
  FILE_ATTRIBUTE_TAG_INFO_ FileAttributeInfo;
  if (unlikely(!GetFileInformationByHandleEx(Handle, FileAttributeTagInfo_,
                                             &FileAttributeInfo,
                                             sizeof(FileAttributeInfo)))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  const auto Attributes = FileAttributeInfo.FileAttributes;
#else
  BY_HANDLE_FILE_INFORMATION_ FileInfo;
  if (unlikely(!GetFileInformationByHandle(Handle, &FileInfo))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  const auto Attributes = FileInfo.dwFileAttributes;
#endif

  if (unlikely(Attributes == INVALID_FILE_ATTRIBUTES_)) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return Attributes;
}

inline WasiExpect<void> forceDirectory(HANDLE_ Handle) noexcept {
  if (auto Res = getAttribute(Handle); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else if (unlikely(!((*Res) & FILE_ATTRIBUTE_DIRECTORY_))) {
    return WasiUnexpect(__WASI_ERRNO_NOTDIR);
  }

  return {};
}

inline WasiExpect<std::filesystem::path>
getHandlePath(HANDLE_ Handle) noexcept {
  // First get the path of the handle
#if NTDDI_VERSION >= NTDDI_VISTA
  std::array<wchar_t, UNICODE_STRING_MAX_CHARS_ + 1> Buffer;
  const auto Size = GetFinalPathNameByHandleW(
      Handle, Buffer.data(), static_cast<DWORD_>(Buffer.size()),
      FILE_NAME_NORMALIZED_ | VOLUME_NAME_DOS_);
  if (unlikely(Size == 0)) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  std::wstring_view Path(Buffer.data(), Size);
  if (Path.size() >= 4 && Path[0] == L'\\' && Path[1] == L'\\' &&
      Path[2] == L'?' && Path[3] == L'\\') {
    Path = Path.substr(4);
  }
  return std::filesystem::path(Path);
#else
  union {
    OBJECT_NAME_INFORMATION_ Info;
    std::array<char, sizeof(OBJECT_NAME_INFORMATION_) +
                         (MAX_PATH_ + 1) * sizeof(wchar_t)>
        RawData;
  } Buffer;
  ULONG_ ReturnLength;
  if (const auto Status =
          NtQueryObject(Handle, ObjectNameInformation_, &Buffer,
                        sizeof(Buffer) - sizeof(wchar_t), &ReturnLength);
      unlikely(!NT_SUCCESS_(Status))) {
    return WasiUnexpect(detail::fromLastError(RtlNtStatusToDosError(Status)));
  }
  std::wstring_view LogicVolumePath(Buffer.Info.Name.Buffer,
                                    Buffer.Info.Name.Length / sizeof(wchar_t));

  // return format is like "A:\\\0B:\\\0C:\\\0\0"
  std::array<wchar_t, 4 * 26 + 1> Drives;
  const auto Size = GetLogicalDriveStringsW(Drives.size(), Drives.data());
  assuming(Size < Drives.size());
  // logic format is like "\Device\HarddiskVolume1\"
  std::array<wchar_t, MAX_PATH_ + 1> FullName;
  wchar_t Name[] = L" :";
  for (wchar_t *Iter = Drives.data(); *Iter != L'\0';
       Iter += std::wcslen(Iter) + 1) {
    Name[0] = Iter[0];
    if (const auto FullNameSize =
            QueryDosDeviceW(Name, FullName.data(), FullName.size());
        unlikely(!FullNameSize)) {
      return WasiUnexpect(detail::fromLastError(GetLastError()));
    } else {
      // FullNameSize include L'\0', append backslash to FullName
      FullName[FullNameSize - 2] = '\\';
      if (std::wcsncmp(FullName.data(), LogicVolumePath.data(),
                       FullNameSize - 1) == 0) {
        std::filesystem::path Result(Iter);
        Result /= LogicVolumePath.substr(FullNameSize - 1);
        return Result;
      }
    }
  }
  std::filesystem::path Result(LogicVolumePath);
  return Result;
#endif
}

inline WasiExpect<std::filesystem::path>
getRelativePath(HANDLE_ Handle, std::string_view Path) noexcept {
  // Check if the path is a directory or not
  if (auto Res = forceDirectory(Handle); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  std::filesystem::path FullPath;
  if (auto Res = getHandlePath(Handle); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  // Append the paths together
  FullPath /= std::filesystem::u8path(Path);
  return FullPath;
}

class SymlinkPriviledgeHolder {
private:
  SymlinkPriviledgeHolder() noexcept {
    TOKEN_PRIVILEGES_ TokenPrivileges;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED_;
    if (LookupPrivilegeValueW(nullptr, L"SeCreateSymbolicLinkPrivilege",
                              &TokenPrivileges.Privileges[0].Luid)) {
      HandleHolder Token;
      if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS_,
                           &Token.Handle)) {
        if (AdjustTokenPrivileges(Token.Handle, false, &TokenPrivileges, 0,
                                  nullptr, 0)) {
          Succeed = true;
        } else if (const auto Error = GetLastError();
                   Error != ERROR_NOT_ALL_ASSIGNED_) {
          spdlog::error("AdjustTokenPrivileges failed:0x{:08x}",
                        GetLastError());
        }
      } else {
        spdlog::error("OpenProcessToken failed:0x{:08x}", GetLastError());
      }
    } else {
      spdlog::error("LookupPrivilegeValueW failed:0x{:08x}", GetLastError());
    }
  }
  bool Succeed = false;
  static SymlinkPriviledgeHolder Holder;

public:
  static bool ok() noexcept { return Holder.Succeed; }
};

SymlinkPriviledgeHolder SymlinkPriviledgeHolder::Holder;

} // namespace

HandleHolder::HandleHolder(const std::filesystem::path &Path,
                           const DWORD_ AccessFlags, const DWORD_ ShareFlags,
                           const DWORD_ CreationDisposition,
                           const DWORD_ AttributeFlags) noexcept
    : Handle(nullptr), Type(HandleType::NormalHandle) {
#if NTDDI_VERSION >= NTDDI_WIN8
  CREATEFILE2_EXTENDED_PARAMETERS_ Create2ExParams;
  Create2ExParams.dwSize = sizeof(Create2ExParams);
  Create2ExParams.dwFileAttributes = AttributeFlags & 0xFFFF;
  Create2ExParams.dwFileFlags = AttributeFlags & 0xFFF00000;
  Create2ExParams.dwSecurityQosFlags = AttributeFlags & 0x000F0000;
  Create2ExParams.lpSecurityAttributes = nullptr;
  Create2ExParams.hTemplateFile = nullptr;

  Handle = CreateFile2(Path.c_str(), AccessFlags, ShareFlags,
                       CreationDisposition, &Create2ExParams);
#else
  Handle = CreateFileW(Path.c_str(), AccessFlags, ShareFlags, nullptr,
                       CreationDisposition, AttributeFlags, nullptr);
#endif
  if (unlikely(Handle == INVALID_HANDLE_VALUE_)) {
    Handle = nullptr;
  }
}

bool HandleHolder::reopen(const DWORD_ AccessFlags, const DWORD_ ShareFlags,
                          const DWORD_ AttributeFlags) noexcept {
  if (Type != HandleType::NormalHandle) {
    return false;
  }
#if NTDDI_VERSION >= NTDDI_VISTA
  HandleHolder NewFile(
      ReOpenFile(Handle, AccessFlags, ShareFlags, AttributeFlags), false);
#else
  std::filesystem::path Path;
  if (auto Res = getHandlePath(Handle); unlikely(!Res)) {
    return false;
  } else {
    Path = std::move(*Res);
  }
  HandleHolder NewFile(Path, AccessFlags, ShareFlags, OPEN_EXISTING_,
                       AttributeFlags);
#endif
  if (unlikely(!NewFile.ok())) {
    return false;
  }
  std::swap(*this, NewFile);
  return true;
}

void HandleHolder::reset() noexcept {
  if (likely(ok())) {
    switch (Type) {
    case HandleType::NormalHandle:
      CloseHandle(Handle);
      Handle = nullptr;
      break;
    case HandleType::NormalSocket:
      closesocket(Socket);
      Socket = 0;
      break;
    case HandleType::StdHandle:
      // nothing to do
      Handle = nullptr;
      break;
    default:
      assumingUnreachable();
    }
  }
}

WasiExpect<void>
HandleHolder::filestatGet(__wasi_filestat_t &FileStat) const noexcept {
  switch (fastGetFileType(Type, Handle)) {
  case FILE_TYPE_DISK_: {
#if WINAPI_PARTITION_DESKTOP
    BY_HANDLE_FILE_INFORMATION_ FileInfo;
    if (unlikely(!GetFileInformationByHandle(Handle, &FileInfo))) {
      auto Res = detail::fromLastError(GetLastError());
      return WasiUnexpect(Res);
    }

    FileStat.dev = FileInfo.dwVolumeSerialNumber;
    FileStat.ino =
        combineHighLow(FileInfo.nFileIndexHigh, FileInfo.nFileIndexLow);
    FileStat.filetype = getDiskFileType(FileInfo.dwFileAttributes);
    FileStat.nlink = FileInfo.nNumberOfLinks;
    FileStat.size =
        combineHighLow(FileInfo.nFileSizeHigh, FileInfo.nFileSizeLow);
    FileStat.atim = detail::fromFiletime(FileInfo.ftLastAccessTime);
    FileStat.mtim = detail::fromFiletime(FileInfo.ftLastWriteTime);
    FileStat.ctim = detail::fromFiletime(FileInfo.ftCreationTime);
#else
    if (auto Res = getAttribute(Handle); unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      FileStat.filetype = getDiskFileType(*Res);
    }
    using namespace std::literals;
    std::wstring Filename;
    FindHolder Finder;
    HandleHolder Holder;
    switch (FileStat.filetype) {
    case __WASI_FILETYPE_DIRECTORY:
      Filename = L"."s;
      Finder.emplace(Handle);
      break;
    default:
      if (auto Res = getHandlePath(Handle); unlikely(!Res)) {
        return WasiUnexpect(Res);
      } else {
        Filename = Res->filename().native();
        Holder = HandleHolder(Res->parent_path(), FILE_GENERIC_READ_,
                              FILE_SHARE_VALID_FLAGS_, OPEN_EXISTING_,
                              FILE_ATTRIBUTE_DIRECTORY_ |
                                  FILE_FLAG_BACKUP_SEMANTICS_ |
                                  FILE_FLAG_OPEN_REPARSE_POINT_);
        Finder.emplace(Holder.Handle);
      }
      break;
    }
    do {
      const auto &Info = Finder.getData();
      const std::wstring_view CurrName(Info.FileName,
                                       Info.FileNameLength / sizeof(wchar_t));
      if (CurrName != Filename) {
        continue;
      }
      FileStat.dev = 0;
      FileStat.ino = static_cast<__wasi_inode_t>(Info.FileId.QuadPart);
      FileStat.nlink = 0;
      FileStat.size = static_cast<__wasi_filesize_t>(Info.EndOfFile.QuadPart);
      FileStat.atim = detail::fromFiletime(
          reinterpret_cast<const FILETIME_ &>(Info.LastAccessTime.QuadPart));
      FileStat.mtim = detail::fromFiletime(
          reinterpret_cast<const FILETIME_ &>(Info.LastWriteTime.QuadPart));
      FileStat.ctim = detail::fromFiletime(
          reinterpret_cast<const FILETIME_ &>(Info.CreationTime.QuadPart));
      return {};
    } while (Finder.next());
#endif
    break;
  }
  default:
    FileStat.dev = 0;
    FileStat.ino = 0;
    FileStat.nlink = 0;
    FileStat.size = 0;
    FileStat.atim = 0;
    FileStat.mtim = 0;
    FileStat.ctim = 0;
    break;
  }
  return {};
}

template <typename T>
WasiExpect<void> FindHolderBase<T>::emplace(HANDLE_ PathHandle) noexcept {
  reset();
  if (auto Res = getHandlePath(PathHandle); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    Path = std::move(*Res);
  }
  Handle = PathHandle;
  if (auto Res = Proxy::doRewind(static_cast<T &>(*this), true);
      unlikely(!Res)) {
    return WasiUnexpect(Res);
  }
  return {};
}

template <typename T>
WasiExpect<void> FindHolderBase<T>::seek(uint64_t NewCookie) noexcept {
  if (NewCookie < Cookie) {
    if (auto Res = Proxy::doRewind(static_cast<T &>(*this), false);
        unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      Cookie = 0;
      Buffer.clear();
    }
  }
  // seekdir() emulation - go to the Cookie'th file/directory
  if (unlikely(NewCookie != Cookie)) {
    Buffer.clear();
    while (Cookie < NewCookie) {
      if (!next()) {
        return WasiUnexpect(detail::fromLastError(GetLastError()));
      }
    }
  }
  return {};
}

template <typename T> bool FindHolderBase<T>::next() noexcept {
  if (!Proxy::doNext(static_cast<T &>(*this))) {
    return false;
  }
  ++Cookie;
  return true;
}

template <typename T>
WasiExpect<void> FindHolderBase<T>::loadDirent() noexcept {
  return Proxy::doLoadDirent(static_cast<T &>(*this));
}

template <typename T>
size_t FindHolderBase<T>::write(Span<uint8_t> Output) noexcept {
  const auto Size = std::min(Buffer.size(), Output.size());
  const auto Diff = static_cast<std::ptrdiff_t>(Size);
  if (!Buffer.empty()) {
    std::copy(Buffer.begin(), Buffer.begin() + Diff, Output.begin());
    Buffer.clear();
  }
  return Size;
}

#if NTDDI_VERSION >= NTDDI_VISTA
void FindHolder::doReset() noexcept {
  Cursor = 0;
  FindDataUnion.FindData.NextEntryOffset = 0;
}

bool FindHolder::doNext() noexcept {
  if (!nextData()) {
    if (unlikely(!GetFileInformationByHandleEx(
            getHandle(), FileIdBothDirectoryInfo_, &FindDataUnion,
            sizeof(FindDataUnion)))) {
      return false;
    }
    Cursor = 0;
  }
  return true;
}

WasiExpect<void> FindHolder::doRewind(bool) noexcept {
  if (unlikely(!GetFileInformationByHandleEx(
          getHandle(), FileIdBothDirectoryRestartInfo_, &FindDataUnion,
          sizeof(FindDataUnion)))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  Cursor = 0;
  return {};
}

WasiExpect<void> FindHolder::doLoadDirent() noexcept {
  const auto &Info = getData();
  assuming(Info.FileNameLength <= UNICODE_STRING_MAX_BYTES_);
  const std::filesystem::path Filename(
      std::wstring_view(Info.FileName, Info.FileNameLength / sizeof(wchar_t)));

  std::string UTF8FileName = Filename.u8string();
  resizeBuffer(sizeof(__wasi_dirent_t) + UTF8FileName.size());
  __wasi_dirent_t *const Dirent =
      reinterpret_cast<__wasi_dirent_t *>(getBuffer().data());

  Dirent->d_next = getCookie() + 1;
  Dirent->d_namlen = static_cast<uint32_t>(UTF8FileName.size());
  Dirent->d_ino = static_cast<__wasi_inode_t>(Info.FileId.QuadPart);
  Dirent->d_type = getDiskFileType(Info.FileAttributes);
  std::copy(UTF8FileName.cbegin(), UTF8FileName.cend(),
            getBuffer().begin() + sizeof(__wasi_dirent_t));
  return {};
}

const FILE_ID_BOTH_DIR_INFO_ &FindHolder::getData() const noexcept {
  return reinterpret_cast<const FILE_ID_BOTH_DIR_INFO_ &>(
      FindDataUnion.FindDataPadding[Cursor]);
}

bool FindHolder::nextData() noexcept {
  const auto Offset = getData().NextEntryOffset;
  if (Offset == 0) {
    return false;
  }
  Cursor += Offset;
  return true;
}
#else
void FindHolder::doReset() noexcept { FindClose(getHandle()); }

bool FindHolder::doNext() noexcept {
  if (unlikely(!FindNextFileW(getHandle(), &FindData))) {
    return false;
  }
  return true;
}

WasiExpect<void> FindHolder::doRewind(bool First) noexcept {
  auto Path = getPath() / L"*";
  if (HANDLE_ FindHandle = FindFirstFileW(Path.c_str(), &FindData);
      unlikely(FindHandle == INVALID_HANDLE_VALUE_)) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  } else {
    if (!First) {
      FindClose(getHandle());
    }
    setHandle(FindHandle);
    return {};
  }
}

WasiExpect<void> FindHolder::doLoadDirent() noexcept {
  const std::filesystem::path Filename(FindData.cFileName);

  HandleHolder File(getPath() / Filename, FILE_GENERIC_READ_,
                    FILE_SHARE_VALID_FLAGS_, OPEN_EXISTING_,
                    FILE_FLAG_BACKUP_SEMANTICS_ |
                        FILE_FLAG_OPEN_REPARSE_POINT_);

  if (unlikely(!File.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  BY_HANDLE_FILE_INFORMATION_ FileInfo;
  if (unlikely(!GetFileInformationByHandle(File.Handle, &FileInfo))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  std::string UTF8FileName = Filename.u8string();
  resizeBuffer(sizeof(__wasi_dirent_t) + UTF8FileName.size());
  __wasi_dirent_t *const Dirent =
      reinterpret_cast<__wasi_dirent_t *>(getBuffer().data());

  Dirent->d_next = getCookie() + 1;
  Dirent->d_namlen = static_cast<uint32_t>(UTF8FileName.size());
  Dirent->d_ino =
      combineHighLow(FileInfo.nFileIndexHigh, FileInfo.nFileIndexLow);
  Dirent->d_type = getDiskFileType(FileInfo.dwFileAttributes);
  std::copy(UTF8FileName.cbegin(), UTF8FileName.cend(),
            getBuffer().begin() + sizeof(__wasi_dirent_t));
  return {};
}
#endif

INode INode::stdIn() noexcept {
  return INode(GetStdHandle(STD_INPUT_HANDLE_), true);
}

INode INode::stdOut() noexcept {
  return INode(GetStdHandle(STD_OUTPUT_HANDLE_), true);
}

INode INode::stdErr() noexcept {
  return INode(GetStdHandle(STD_ERROR_HANDLE_), true);
}

WasiExpect<INode> INode::open(std::string Path, __wasi_oflags_t OpenFlags,
                              __wasi_fdflags_t FdFlags,
                              VFS::Flags VFSFlags) noexcept {
  DWORD_ AttributeFlags;
  DWORD_ AccessFlags;
  DWORD_ CreationDisposition;
  if (auto Res = getOpenFlags(OpenFlags, FdFlags, VFSFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    std::tie(AttributeFlags, AccessFlags, CreationDisposition) = *Res;
  }

  const DWORD_ ShareFlags = FILE_SHARE_VALID_FLAGS_;
  const auto FullPath = std::filesystem::u8path(Path);

  INode Result(FullPath, AccessFlags, ShareFlags, CreationDisposition,
               AttributeFlags);

  if (unlikely(!Result.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  Result.SavedFdFlags = FdFlags;
  Result.SavedVFSFlags = VFSFlags;
  return Result;
}

WasiExpect<void> INode::fdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t,
                                 __wasi_advice_t Advice
                                 [[maybe_unused]]) const noexcept {
  // Windows only supports whole file advising. Ignore unsupported advises.
  if (Offset != 0) {
    return {};
  }

#if WINAPI_PARTITION_DESKTOP
  IO_STATUS_BLOCK_ IOStatusBlock;
  FILE_MODE_INFORMATION_ FileModeInfo;
  if (const auto Status =
          NtQueryInformationFile(Handle, &IOStatusBlock, &FileModeInfo,
                                 sizeof(FileModeInfo), FileModeInformation_);
      unlikely(!NT_SUCCESS_(Status))) {
    // Silence failure
    return {};
  }

  FileModeInfo.Mode &= ~(FILE_SEQUENTIAL_ONLY_ | FILE_RANDOM_ACCESS_);
  switch (Advice) {
  case __WASI_ADVICE_NORMAL:
  case __WASI_ADVICE_WILLNEED:
  case __WASI_ADVICE_DONTNEED:
  case __WASI_ADVICE_NOREUSE:
    // Ignoring these unsupported flags now
    break;
  case __WASI_ADVICE_SEQUENTIAL:
    FileModeInfo.Mode |= FILE_SEQUENTIAL_ONLY_;
    break;
  case __WASI_ADVICE_RANDOM:
    FileModeInfo.Mode |= FILE_RANDOM_ACCESS_;
    break;
  }

  if (const auto Status =
          NtSetInformationFile(Handle, &IOStatusBlock, &FileModeInfo,
                               sizeof(FileModeInfo), FileModeInformation_);
      unlikely(!NT_SUCCESS_(Status))) {
    // Silence failure
    return {};
  }
#endif

  return {};
}

WasiExpect<void> INode::fdAllocate(__wasi_filesize_t Offset,
                                   __wasi_filesize_t Len) const noexcept {
  if (unlikely(Offset >
               static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  if (unlikely(Len >
               static_cast<uint64_t>((std::numeric_limits<int64_t>::max())))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  if (unlikely((Offset + Len) >
               static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  const int64_t RequestSize = static_cast<int64_t>(Offset + Len);

  if (LARGE_INTEGER_ FileSize; unlikely(!GetFileSizeEx(Handle, &FileSize))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  } else if (FileSize.QuadPart >= RequestSize) {
    // Silence success if current size is larger then requested size.
    return {};
  }

#if NTDDI_VERSION >= NTDDI_VISTA
  FILE_END_OF_FILE_INFO_ EndOfFileInfo;
  EndOfFileInfo.EndOfFile.QuadPart = RequestSize;

  if (!SetFileInformationByHandle(Handle, FileEndOfFileInfo_, &EndOfFileInfo,
                                  sizeof(EndOfFileInfo))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
#else
  LARGE_INTEGER_ Old = _LARGE_INTEGER(0);
  if (unlikely(!SetFilePointerEx(Handle, Old, &Old, FILE_CURRENT_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  LARGE_INTEGER_ New = _LARGE_INTEGER(RequestSize);
  if (unlikely(!SetFilePointerEx(Handle, New, nullptr, FILE_BEGIN_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  if (unlikely(!SetEndOfFile(Handle))) {
    auto LastError = detail::fromLastError(GetLastError());
    SetFilePointerEx(Handle, Old, nullptr, FILE_BEGIN_);
    return WasiUnexpect(LastError);
  }
  SetFilePointerEx(Handle, Old, nullptr, FILE_BEGIN_);
#endif

  return {};
}

WasiExpect<void> INode::fdDatasync() const noexcept {
  if (unlikely(!FlushFileBuffers(Handle))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept {
  if (auto Res = filetype(); unlikely(!Res)) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  } else {
    FdStat.fs_flags = SavedFdFlags;
    FdStat.fs_filetype = *Res;
    return {};
  }
}

WasiExpect<void>
INode::fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept {
  auto This = const_cast<INode *>(this);
  if (Type == HandleType::NormalSocket) {
    // Support __WASI_FDFLAGS_NONBLOCK only, ignore other flags.
    if ((This->SavedFdFlags ^ FdFlags) & __WASI_FDFLAGS_NONBLOCK) {
      const bool NonBlock = FdFlags & __WASI_FDFLAGS_NONBLOCK;
      u_long SysFlag = NonBlock ? 1 : 0;
      if (auto Res = ioctlsocket(Socket, FIONBIO, &SysFlag);
          unlikely(Res == SOCKET_ERROR_)) {
        return WasiUnexpect(detail::fromWSALastError());
      }
      if (NonBlock) {
        This->SavedFdFlags |= __WASI_FDFLAGS_NONBLOCK;
      } else {
        This->SavedFdFlags &= ~__WASI_FDFLAGS_NONBLOCK;
      }
    }
    return {};
  }
  // Support __WASI_FDFLAGS_APPEND only, ignore other flags.
  if ((This->SavedFdFlags ^ FdFlags) & __WASI_FDFLAGS_APPEND) {
    const bool Append = FdFlags & __WASI_FDFLAGS_APPEND;
    if (Append) {
      This->SavedFdFlags |= __WASI_FDFLAGS_APPEND;
    } else {
      This->SavedFdFlags &= ~__WASI_FDFLAGS_APPEND;
    }
  }
  return {};
}

WasiExpect<void>
INode::fdFilestatGet(__wasi_filestat_t &FileStat) const noexcept {
  return filestatGet(FileStat);
}

WasiExpect<void>
INode::fdFilestatSetSize(__wasi_filesize_t Size) const noexcept {
  if (unlikely(Size >
               static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  const int64_t RequestSize = static_cast<int64_t>(Size);

#if NTDDI_VERSION >= NTDDI_VISTA
  FILE_END_OF_FILE_INFO_ EndOfFileInfo;
  EndOfFileInfo.EndOfFile.QuadPart = RequestSize;

  if (!SetFileInformationByHandle(Handle, FileEndOfFileInfo_, &EndOfFileInfo,
                                  sizeof(EndOfFileInfo))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
#else
  LARGE_INTEGER_ Old = _LARGE_INTEGER(0);
  if (unlikely(!SetFilePointerEx(Handle, Old, &Old, FILE_CURRENT_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  LARGE_INTEGER_ New = _LARGE_INTEGER(RequestSize);
  if (unlikely(!SetFilePointerEx(Handle, New, nullptr, FILE_BEGIN_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  if (unlikely(!SetEndOfFile(Handle))) {
    auto LastError = detail::fromLastError(GetLastError());
    SetFilePointerEx(Handle, Old, nullptr, FILE_BEGIN_);
    return WasiUnexpect(LastError);
  }
  SetFilePointerEx(Handle, Old, nullptr, FILE_BEGIN_);
#endif

  return {};
}

WasiExpect<void>
INode::fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                          __wasi_fstflags_t FstFlags) const noexcept {
  // Let FileTime be initialized to zero if the times need not be changed
  FILETIME_ AFileTime = {0, 0};
  FILETIME_ MFileTime = {0, 0};

  // For setting access time
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    AFileTime = detail::toFiletime(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
#if NTDDI_VERSION >= NTDDI_WIN8
    GetSystemTimePreciseAsFileTime(&AFileTime);
#else
    GetSystemTimeAsFileTime(&AFileTime);
#endif
  }

  // For setting modification time
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    MFileTime = detail::toFiletime(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
#if NTDDI_VERSION >= NTDDI_WIN8
    GetSystemTimePreciseAsFileTime(&MFileTime);
#else
    GetSystemTimeAsFileTime(&MFileTime);
#endif
  }

  if (unlikely(!SetFileTime(Handle, nullptr, &AFileTime, &MFileTime))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::fdPread(Span<Span<uint8_t>> IOVs,
                                __wasi_filesize_t Offset,
                                __wasi_size_t &NRead) const noexcept {
  WasiExpect<void> Result;
  std::vector<OVERLAPPED_> Queries(IOVs.size());
  ULARGE_INTEGER_ LocalOffset = _ULARGE_INTEGER(Offset);

  for (size_t I = 0; I < IOVs.size(); ++I) {
    auto &IOV = IOVs[I];
    auto &Query = Queries[I];
    Query.Offset = LocalOffset.LowPart;
    Query.OffsetHigh = LocalOffset.HighPart;
    Query.hEvent = nullptr;
    if (!ReadFileEx(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                    &Query, &EmptyOverlappedCompletionRoutine)) {
      if (unlikely(GetLastError() != ERROR_IO_PENDING_)) {
        Result = WasiUnexpect(detail::fromLastError(GetLastError()));
        Queries.resize(I);
        break;
      }
    }
    LocalOffset.QuadPart += IOV.size();
  }

  NRead = 0;
  for (size_t I = 0; I < Queries.size(); ++I) {
    auto &Query = Queries[I];
    DWORD_ NumberOfBytesRead = 0;
    if (unlikely(
            !GetOverlappedResult(Handle, &Query, &NumberOfBytesRead, true))) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        CancelIo(Handle);
        for (size_t J = I + 1; J < Queries.size(); ++J) {
          GetOverlappedResult(Handle, &Queries[J], &NumberOfBytesRead, true);
        }
        break;
      }
    }
    NRead += NumberOfBytesRead;
  }

  return Result;
}

WasiExpect<void> INode::fdPwrite(Span<Span<const uint8_t>> IOVs,
                                 __wasi_filesize_t Offset,
                                 __wasi_size_t &NWritten) const noexcept {
  WasiExpect<void> Result;
  std::vector<OVERLAPPED_> Queries(IOVs.size());
  ULARGE_INTEGER_ LocalOffset = _ULARGE_INTEGER(Offset);

  for (size_t I = 0; I < IOVs.size(); ++I) {
    auto &IOV = IOVs[I];
    auto &Query = Queries[I];
    Query.Offset = LocalOffset.LowPart;
    Query.OffsetHigh = LocalOffset.HighPart;
    Query.hEvent = nullptr;
    if (!WriteFileEx(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                     &Query, &EmptyOverlappedCompletionRoutine)) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_IO_PENDING_ && Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        Queries.resize(I);
        break;
      }
    }
    LocalOffset.QuadPart += IOV.size();
  }

  NWritten = 0;
  for (size_t I = 0; I < Queries.size(); ++I) {
    auto &Query = Queries[I];
    DWORD_ NumberOfBytesWrite = 0;
    if (unlikely(
            !GetOverlappedResult(Handle, &Query, &NumberOfBytesWrite, true))) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        CancelIo(Handle);
        for (size_t J = I + 1; J < Queries.size(); ++J) {
          GetOverlappedResult(Handle, &Queries[J], &NumberOfBytesWrite, true);
        }
        break;
      }
    }
    NWritten += NumberOfBytesWrite;
  }

  return Result;
}

WasiExpect<void> INode::fdRead(Span<Span<uint8_t>> IOVs,
                               __wasi_size_t &NRead) const noexcept {
  WasiExpect<void> Result;
  std::vector<OVERLAPPED_> Queries(IOVs.size());
  LARGE_INTEGER_ OldOffset = _LARGE_INTEGER(0);
  if (unlikely(
          !SetFilePointerEx(Handle, OldOffset, &OldOffset, FILE_CURRENT_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  LARGE_INTEGER_ LocalOffset = OldOffset;

  for (size_t I = 0; I < IOVs.size(); ++I) {
    auto &IOV = IOVs[I];
    auto &Query = Queries[I];
    Query.Offset = LocalOffset.LowPart;
    Query.OffsetHigh = static_cast<DWORD_>(LocalOffset.HighPart);
    Query.hEvent = nullptr;
    if (!ReadFileEx(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                    &Query, &EmptyOverlappedCompletionRoutine)) {
      if (unlikely(GetLastError() != ERROR_IO_PENDING_)) {
        Result = WasiUnexpect(detail::fromLastError(GetLastError()));
        Queries.resize(I);
        break;
      }
    }
    LocalOffset.QuadPart += IOV.size();
  }

  NRead = 0;
  for (size_t I = 0; I < Queries.size(); ++I) {
    auto &Query = Queries[I];
    DWORD_ NumberOfBytesRead = 0;
    if (unlikely(
            !GetOverlappedResult(Handle, &Query, &NumberOfBytesRead, true))) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        CancelIo(Handle);
        for (size_t J = I + 1; J < Queries.size(); ++J) {
          GetOverlappedResult(Handle, &Queries[J], &NumberOfBytesRead, true);
        }
        break;
      }
    }
    NRead += NumberOfBytesRead;
  }

  OldOffset.QuadPart += NRead;
  SetFilePointerEx(Handle, OldOffset, nullptr, FILE_BEGIN_);

  return Result;
}

WasiExpect<void> INode::fdReaddir(Span<uint8_t> Buffer,
                                  __wasi_dircookie_t Cookie,
                                  __wasi_size_t &Size) noexcept {
  if (likely(Find.ok())) {
    if (auto Res = Find.seek(Cookie); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
  }

  if (unlikely(!Find.ok())) {
    // Begin the search for files
    if (auto Res = Find.emplace(Handle); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }
  }

  bool FindNextResult = true;
  Size = 0;

  do {
    const auto Written = Find.write(Buffer);
    Buffer = Buffer.subspan(Written);
    Size += static_cast<uint32_t>(Written);
    if (unlikely(Buffer.empty())) {
      break;
    }
    if (!FindNextResult) {
      // Check if there no more files left or if an error has been encountered
      if (DWORD_ Code = GetLastError();
          unlikely(Code != ERROR_NO_MORE_FILES_)) {
        // The FindNextFileW() function has failed
        return WasiUnexpect(detail::fromLastError(Code));
      }
      break;
    }

    if (auto Res = Find.loadDirent(); unlikely(!Res)) {
      return WasiUnexpect(Res);
    }

    FindNextResult = Find.next();
  } while (!Buffer.empty());

  return {};
}

WasiExpect<void> INode::fdSeek(__wasi_filedelta_t Offset,
                               __wasi_whence_t Whence,
                               __wasi_filesize_t &Size) const noexcept {
  DWORD_ SysWhence = toWhence(Whence);
  LARGE_INTEGER_ Pointer = _LARGE_INTEGER(Offset);
  if (unlikely(!SetFilePointerEx(Handle, Pointer, &Pointer, SysWhence))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  Size = static_cast<uint64_t>(Pointer.QuadPart);
  return {};
}

WasiExpect<void> INode::fdSync() const noexcept {
  if (unlikely(!FlushFileBuffers(Handle))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::fdTell(__wasi_filesize_t &Size) const noexcept {
  LARGE_INTEGER_ Pointer = _LARGE_INTEGER(0);
  if (unlikely(!SetFilePointerEx(Handle, Pointer, &Pointer, FILE_CURRENT_))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  Size = static_cast<uint64_t>(Pointer.QuadPart);
  return {};
}

WasiExpect<void> INode::fdWrite(Span<Span<const uint8_t>> IOVs,
                                __wasi_size_t &NWritten) const noexcept {
  const bool Append = SavedFdFlags & __WASI_FDFLAGS_APPEND;
  WasiExpect<void> Result;
  std::vector<OVERLAPPED_> Queries(IOVs.size());
  LARGE_INTEGER_ OldOffset = _LARGE_INTEGER(0);
  if (!Append) {
    if (unlikely(
            !SetFilePointerEx(Handle, OldOffset, &OldOffset, FILE_CURRENT_))) {
      return WasiUnexpect(detail::fromLastError(GetLastError()));
    }
  }
  LARGE_INTEGER_ LocalOffset = OldOffset;

  for (size_t I = 0; I < IOVs.size(); ++I) {
    auto &IOV = IOVs[I];
    auto &Query = Queries[I];
    if (!Append) {
      Query.Offset = LocalOffset.LowPart;
      Query.OffsetHigh = static_cast<DWORD_>(LocalOffset.HighPart);
    } else {
      Query.Offset = 0xFFFFFFFF;
      Query.OffsetHigh = 0xFFFFFFFF;
    }
    Query.hEvent = nullptr;
    if (!WriteFileEx(Handle, IOV.data(), static_cast<uint32_t>(IOV.size()),
                     &Query, &EmptyOverlappedCompletionRoutine)) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_IO_PENDING_ && Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        Queries.resize(I);
        break;
      }
    }
    if (!Append) {
      LocalOffset.QuadPart += IOV.size();
    }
  }

  NWritten = 0;
  for (size_t I = 0; I < Queries.size(); ++I) {
    auto &Query = Queries[I];
    DWORD_ NumberOfBytesWrite = 0;
    if (unlikely(
            !GetOverlappedResult(Handle, &Query, &NumberOfBytesWrite, true))) {
      if (const auto Error = GetLastError();
          unlikely(Error != ERROR_HANDLE_EOF_)) {
        Result = WasiUnexpect(detail::fromLastError(Error));
        CancelIo(Handle);
        for (size_t J = I + 1; J < Queries.size(); ++J) {
          GetOverlappedResult(Handle, &Queries[J], &NumberOfBytesWrite, true);
        }
        break;
      }
    }
    NWritten += NumberOfBytesWrite;
  }

  if (!Append) {
    OldOffset.QuadPart += NWritten;
    SetFilePointerEx(Handle, OldOffset, nullptr, FILE_BEGIN_);
  }

  return Result;
}

WasiExpect<uint64_t> INode::getNativeHandler() const noexcept {
  return reinterpret_cast<uint64_t>(Handle);
}

WasiExpect<void> INode::pathCreateDirectory(std::string Path) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  if (unlikely(!CreateDirectoryW(FullPath.c_str(), nullptr))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

WasiExpect<void>
INode::pathFilestatGet(std::string Path,
                       __wasi_filestat_t &FileStat) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  HandleHolder File(
      FullPath, FILE_GENERIC_READ_, FILE_SHARE_VALID_FLAGS_, OPEN_EXISTING_,
      FILE_FLAG_BACKUP_SEMANTICS_ | FILE_FLAG_OPEN_REPARSE_POINT_);

  if (unlikely(!File.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  return File.filestatGet(FileStat);
}

WasiExpect<void>
INode::pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                            __wasi_timestamp_t MTim,
                            __wasi_fstflags_t FstFlags) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  HandleHolder File(FullPath, FILE_GENERIC_READ_ | FILE_GENERIC_WRITE_,
                    FILE_SHARE_VALID_FLAGS_, OPEN_EXISTING_,
                    FILE_FLAG_BACKUP_SEMANTICS_ |
                        FILE_FLAG_OPEN_REPARSE_POINT_);

  if (unlikely(!File.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  // Let FileTime be initialized to zero if the times need not be changed
  FILETIME_ AFileTime = {0, 0};
  FILETIME_ MFileTime = {0, 0};

  // For setting access time
  if (FstFlags & __WASI_FSTFLAGS_ATIM) {
    AFileTime = detail::toFiletime(ATim);
  } else if (FstFlags & __WASI_FSTFLAGS_ATIM_NOW) {
#if NTDDI_VERSION >= NTDDI_WIN8
    GetSystemTimePreciseAsFileTime(&AFileTime);
#else
    GetSystemTimeAsFileTime(&AFileTime);
#endif
  }

  // For setting modification time
  if (FstFlags & __WASI_FSTFLAGS_MTIM) {
    MFileTime = detail::toFiletime(MTim);
  } else if (FstFlags & __WASI_FSTFLAGS_MTIM_NOW) {
#if NTDDI_VERSION >= NTDDI_WIN8
    GetSystemTimePreciseAsFileTime(&MFileTime);
#else
    GetSystemTimeAsFileTime(&MFileTime);
#endif
  }

  if (unlikely(!SetFileTime(File.Handle, nullptr, &AFileTime, &MFileTime))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

#if WINAPI_PARTITION_DESKTOP
WasiExpect<void> INode::pathLink(const INode &Old, std::string OldPath,
                                 const INode &New,
                                 std::string NewPath) noexcept {
  std::filesystem::path OldFullPath;
  if (auto Res = getRelativePath(Old.Handle, OldPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    OldFullPath = std::move(*Res);
  }
  std::filesystem::path NewFullPath;
  if (auto Res = getRelativePath(New.Handle, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    NewFullPath = std::move(*Res);
  }

  // Create the hard link from the paths
  if (unlikely(!CreateHardLinkW(NewFullPath.c_str(), OldFullPath.c_str(),
                                nullptr))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  return {};
}
#else
WasiExpect<void> INode::pathLink(const INode &, std::string, const INode &,
                                 std::string) noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}
#endif

WasiExpect<INode> INode::pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                                  __wasi_fdflags_t FdFlags,
                                  VFS::Flags VFSFlags) const noexcept {
  DWORD_ AttributeFlags;
  DWORD_ AccessFlags;
  DWORD_ CreationDisposition;
  if (auto Res = getOpenFlags(OpenFlags, FdFlags, VFSFlags); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    std::tie(AttributeFlags, AccessFlags, CreationDisposition) = *Res;
  }

  const DWORD_ ShareFlags = FILE_SHARE_VALID_FLAGS_;

  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  INode Result(FullPath, AccessFlags, ShareFlags, CreationDisposition,
               AttributeFlags);

  if (unlikely(!Result.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  if (unlikely(Result.isSymlink())) {
    return WasiUnexpect(__WASI_ERRNO_LOOP);
  }
  const bool NeedDir = OpenFlags & __WASI_OFLAGS_DIRECTORY;
  if (NeedDir && unlikely(!Result.isDirectory())) {
    return WasiUnexpect(__WASI_ERRNO_NOTDIR);
  }

  Result.SavedFdFlags = FdFlags;
  Result.SavedVFSFlags = VFSFlags;
  return Result;
}

WasiExpect<void> INode::pathReadlink(std::string Path, Span<char> Buffer,
                                     __wasi_size_t &NRead) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  // Fill the Buffer with the contents of the link
  HandleHolder Link(FullPath, 0, FILE_SHARE_VALID_FLAGS_, OPEN_EXISTING_,
                    FILE_FLAG_BACKUP_SEMANTICS_ |
                        FILE_FLAG_OPEN_REPARSE_POINT_);

  if (unlikely(!Link.ok())) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  constexpr const size_t MaximumReparseDataBufferSize = 16384;
  std::array<std::byte, MaximumReparseDataBufferSize> DataBuffer;
  DWORD_ BytesReturned;
  if (!DeviceIoControl(
          Link.Handle, FSCTL_GET_REPARSE_POINT_, nullptr, 0, DataBuffer.data(),
          static_cast<DWORD_>(DataBuffer.size()), &BytesReturned, nullptr)) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  auto &Reparse =
      *reinterpret_cast<const REPARSE_DATA_BUFFER_ *>(DataBuffer.data());
  std::wstring_view Data;
  switch (Reparse.ReparseTag) {
  case IO_REPARSE_TAG_SYMLINK_:
    Data = {&Reparse.SymbolicLinkReparseBuffer.PathBuffer
                 [Reparse.SymbolicLinkReparseBuffer.SubstituteNameOffset /
                  sizeof(WCHAR_)],
            Reparse.SymbolicLinkReparseBuffer.SubstituteNameLength /
                sizeof(WCHAR_)};
    using namespace std::literals;
    if (!(Reparse.SymbolicLinkReparseBuffer.Flags & SYMLINK_FLAG_RELATIVE_) &&
        Data.size() >= 4 && Data.substr(0, 4) == L"\\??\\"sv) {
      Data = Data.substr(3);
    }
    break;
  case IO_REPARSE_TAG_MOUNT_POINT_:
    Data = {
        &Reparse.MountPointReparseBuffer
             .PathBuffer[Reparse.MountPointReparseBuffer.SubstituteNameOffset /
                         sizeof(WCHAR_)],
        Reparse.MountPointReparseBuffer.SubstituteNameLength / sizeof(WCHAR_)};
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }

  const auto U8Data = std::filesystem::path{Data}.u8string();
  NRead = static_cast<uint32_t>(std::min(Buffer.size(), U8Data.size()));
  std::copy_n(U8Data.begin(), NRead, Buffer.begin());

  return {};
}

WasiExpect<void> INode::pathRemoveDirectory(std::string Path) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  if (unlikely(!RemoveDirectoryW(FullPath.c_str()))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }
  return {};
}

WasiExpect<void> INode::pathRename(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept {
  std::filesystem::path OldFullPath;
  if (auto Res = getRelativePath(Old.Handle, OldPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    OldFullPath = std::move(*Res);
  }
  std::filesystem::path NewFullPath;
  if (auto Res = getRelativePath(New.Handle, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    NewFullPath = std::move(*Res);
  }

  const auto OldAttr = GetFileAttributesW(OldFullPath.c_str());
  const auto NewAttr = GetFileAttributesW(NewFullPath.c_str());
  if (OldAttr != INVALID_FILE_ATTRIBUTES_ &&
      NewAttr != INVALID_FILE_ATTRIBUTES_) {
    // If source is a directory and destination is a file, fail with NOTDIR
    if ((OldAttr & FILE_ATTRIBUTE_DIRECTORY_) &&
        !(NewAttr & FILE_ATTRIBUTE_DIRECTORY_)) {
      return WasiUnexpect(__WASI_ERRNO_NOTDIR);
    }
    // If source is a file and destination is a directory, fail with ISDIR
    if (!(OldAttr & FILE_ATTRIBUTE_DIRECTORY_) &&
        (NewAttr & FILE_ATTRIBUTE_DIRECTORY_)) {
      return WasiUnexpect(__WASI_ERRNO_ISDIR);
    }
  }

  // Rename the file from the paths
  if (unlikely(
          !MoveFileExW(OldFullPath.c_str(), NewFullPath.c_str(),
                       MOVEFILE_COPY_ALLOWED_ | MOVEFILE_REPLACE_EXISTING_))) {
#if NTDDI_VERSION >= NTDDI_VISTA
    if (const auto Error = GetLastError(); Error != ERROR_ACCESS_DENIED_) {
      return WasiUnexpect(detail::fromLastError(Error));
    }
    // If NewFullPath is an empty directory, remove it and rename.
    HandleHolder Transaction{
        CreateTransaction(nullptr, nullptr, 0, 0, 0, 0, nullptr), false};
    if (Transaction.ok()) {
      if (RemoveDirectoryTransactedW(NewFullPath.c_str(), Transaction.Handle)) {
        if (MoveFileTransactedW(OldFullPath.c_str(), NewFullPath.c_str(),
                                nullptr, nullptr, MOVEFILE_REPLACE_EXISTING_,
                                Transaction.Handle)) {
          if (CommitTransaction(Transaction.Handle)) {
            return {};
          }
        }
      }
    }
#endif
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  return {};
}

#if NTDDI_VERSION >= NTDDI_VISTA
WasiExpect<void> INode::pathSymlink(std::string OldPath,
                                    std::string NewPath) const noexcept {
  if (!SymlinkPriviledgeHolder::ok()) {
    return WasiUnexpect(__WASI_ERRNO_PERM);
  }
  std::filesystem::path NewFullPath;
  if (auto Res = getRelativePath(Handle, NewPath); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    NewFullPath = std::move(*Res);
  }
  if (GetFileAttributesW(NewFullPath.c_str()) == INVALID_FILE_ATTRIBUTES_) {
    assuming(!NewPath.empty());
    if (NewPath.back() == '/') {
      // Dangling link destination shouldn't end with a slash
      return WasiUnexpect(__WASI_ERRNO_NOENT);
    }
  } else {
    return WasiUnexpect(__WASI_ERRNO_EXIST);
  }

  const std::filesystem::path OldU8Path = std::filesystem::u8path(OldPath);

  DWORD_ TargetType = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE_;
  if (OldU8Path.filename().empty()) {
    TargetType = SYMBOLIC_LINK_FLAG_DIRECTORY_;
  }

  if (unlikely(!CreateSymbolicLinkW(NewFullPath.c_str(), OldU8Path.c_str(),
                                    TargetType))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  return {};
}
#else
WasiExpect<void> INode::pathSymlink(std::string, std::string) const noexcept {
  return WasiUnexpect(__WASI_ERRNO_NOSYS);
}
#endif

WasiExpect<void> INode::pathUnlinkFile(std::string Path) const noexcept {
  std::filesystem::path FullPath;
  if (auto Res = getRelativePath(Handle, Path); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    FullPath = std::move(*Res);
  }

  if (unlikely(!DeleteFileW(FullPath.c_str()))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  }

  return {};
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
  struct addrinfo SysHint;
  SysHint.ai_flags = toAIFlags(Hint.ai_flags);
  SysHint.ai_family = toAddressFamily(Hint.ai_family);
  SysHint.ai_socktype = toSockType(Hint.ai_socktype);
  SysHint.ai_protocol = toProtocol(Hint.ai_protocol);
  SysHint.ai_addrlen = Hint.ai_addrlen;
  SysHint.ai_addr = nullptr;
  SysHint.ai_canonname = nullptr;
  SysHint.ai_next = nullptr;

  const auto [NodeCStr, NodeBuf] = createNullTerminatedString(Node);
  const auto [ServiceCStr, ServiceBuf] = createNullTerminatedString(Service);

  struct addrinfo *SysResPtr = nullptr;
  if (auto Res = getaddrinfo(NodeCStr, ServiceCStr, &SysHint, &SysResPtr);
      unlikely(Res != 0)) {
    return WasiUnexpect(fromWSAError(Res));
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
      case __WASI_ADDRESS_FAMILY_INET4:
        SaSize = sizeof(sockaddr_in) - offsetof(sockaddr_in, sin_port);
        break;
      case __WASI_ADDRESS_FAMILY_INET6:
        SaSize = sizeof(sockaddr_in6) - offsetof(sockaddr_in6, sin6_port);
        break;
      default:
        continue;
      }
      std::memcpy(AiAddrSaDataArray[Idx], SysResItem->ai_addr->sa_data, SaSize);
      CurSockaddr->sa_data_len = static_cast<__wasi_size_t>(SaSize);
      CurSockaddr->sa_family =
          fromAddressFamily(SysResItem->ai_addr->sa_family);
    }
    // process ai_next in addrinfo
    SysResItem = SysResItem->ai_next;
  }
  freeaddrinfo(SysResPtr);

  return {};
}

WasiExpect<INode> INode::sockOpen(__wasi_address_family_t AddressFamily,
                                  __wasi_sock_type_t SockType) noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  const int SysAddressFamily = toAddressFamily(AddressFamily);
  const int SysType = toSockType(SockType);
  const int SysProtocol = IPPROTO_IP;

  if (auto NewSock = socket(SysAddressFamily, SysType, SysProtocol);
      unlikely(NewSock == INVALID_SOCKET_)) {
    return WasiUnexpect(detail::fromWSALastError());
  } else {
    INode New(NewSock);
    return New;
  }
}

WasiExpect<void> INode::sockBind(__wasi_address_family_t AddressFamily,
                                 Span<const uint8_t> Address,
                                 uint16_t Port) noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  Variant<sockaddr, sockaddr_in, sockaddr_in6> ServerAddr;
  size_t Size;

  if (AddressFamily == __WASI_ADDRESS_FAMILY_INET4) {
    auto &ServerAddr4 = ServerAddr.emplace<sockaddr_in>();
    Size = sizeof(ServerAddr4);

    ServerAddr4.sin_family = AF_INET;
    ServerAddr4.sin_port = htons(Port);
    assuming(Address.size() >= sizeof(in_addr));
    std::memcpy(&ServerAddr4.sin_addr, Address.data(), sizeof(in_addr));
  } else if (AddressFamily == __WASI_ADDRESS_FAMILY_INET6) {
    auto &ServerAddr6 = ServerAddr.emplace<sockaddr_in6>();
    Size = sizeof(ServerAddr6);

    ServerAddr6.sin6_family = AF_INET6;
    ServerAddr6.sin6_port = htons(Port);
    assuming(Address.size() >= sizeof(in6_addr));
    std::memcpy(&ServerAddr6.sin6_addr, Address.data(), sizeof(in6_addr));
  } else {
    assumingUnreachable();
  }

  if (auto Res =
          bind(Socket, &ServerAddr.get<sockaddr>(), static_cast<int>(Size));
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  return {};
}

WasiExpect<void> INode::sockListen(int32_t Backlog) noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  if (auto Res = listen(Socket, Backlog); unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }
  return {};
}

WasiExpect<INode> INode::sockAccept(__wasi_fdflags_t FdFlags) noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  SOCKET_ NewSock;
  if (NewSock = accept(Socket, nullptr, nullptr);
      unlikely(NewSock == INVALID_SOCKET_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  INode New(NewSock);
  if (FdFlags & __WASI_FDFLAGS_NONBLOCK) {
    u_long SysNonBlockFlag = 1;
    if (auto Res = ioctlsocket(NewSock, FIONBIO, &SysNonBlockFlag);
        unlikely(Res == SOCKET_ERROR_)) {
      return WasiUnexpect(detail::fromWSALastError());
    }
  }

  return New;
}

WasiExpect<void> INode::sockConnect(__wasi_address_family_t AddressFamily,
                                    Span<const uint8_t> Address,
                                    uint16_t Port) noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  Variant<sockaddr, sockaddr_in, sockaddr_in6> ClientAddr;
  size_t Size;

  if (AddressFamily == __WASI_ADDRESS_FAMILY_INET4) {
    auto &ClientAddr4 = ClientAddr.emplace<sockaddr_in>();
    Size = sizeof(ClientAddr4);

    ClientAddr4.sin_family = AF_INET;
    ClientAddr4.sin_port = htons(Port);
    assuming(Address.size() >= sizeof(in_addr));
    std::memcpy(&ClientAddr4.sin_addr, Address.data(), sizeof(in_addr));
  } else if (AddressFamily == __WASI_ADDRESS_FAMILY_INET6) {
    auto &ClientAddr6 = ClientAddr.emplace<sockaddr_in6>();
    Size = sizeof(ClientAddr6);

    ClientAddr6.sin6_family = AF_INET6;
    ClientAddr6.sin6_port = htons(Port);
    assuming(Address.size() >= sizeof(in6_addr));
    std::memcpy(&ClientAddr6.sin6_addr, Address.data(), sizeof(in_addr));
  } else {
    assumingUnreachable();
  }

  if (auto Res =
          connect(Socket, &ClientAddr.get<sockaddr>(), static_cast<int>(Size));
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  return {};
}

WasiExpect<void> INode::sockRecv(Span<Span<uint8_t>> RiData,
                                 __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                                 __wasi_roflags_t &RoFlags) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  int SysRiFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    SysRiFlags |= MSG_PEEK;
  }
#if NTDDI_VERSION >= NTDDI_WS03
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    SysRiFlags |= MSG_WAITALL;
  }
#endif

  std::size_t TmpBufSize = 0;
  for (auto &IOV : RiData) {
    TmpBufSize += IOV.size();
  }

  std::vector<uint8_t> TmpBuf(TmpBufSize, 0);

  if (auto Res = recv(Socket, reinterpret_cast<char *>(TmpBuf.data()),
                      static_cast<int>(TmpBufSize), SysRiFlags);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  } else {
    NRead = static_cast<__wasi_size_t>(Res);
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);

  size_t BeginIdx = 0;
  for (auto &IOV : RiData) {
    std::copy(TmpBuf.data() + BeginIdx, TmpBuf.data() + BeginIdx + IOV.size(),
              IOV.begin());
    BeginIdx += IOV.size();
  }

  return {};
}

WasiExpect<void> INode::sockRecvFrom(Span<Span<uint8_t>> RiData,
                                     __wasi_riflags_t RiFlags,
                                     __wasi_address_family_t *AddressFamilyPtr,
                                     Span<uint8_t> Address, uint16_t *PortPtr,
                                     __wasi_size_t &NRead,
                                     __wasi_roflags_t &RoFlags) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  int SysRiFlags = 0;
  if (RiFlags & __WASI_RIFLAGS_RECV_PEEK) {
    SysRiFlags |= MSG_PEEK;
  }
#if NTDDI_VERSION >= NTDDI_WS03
  if (RiFlags & __WASI_RIFLAGS_RECV_WAITALL) {
    SysRiFlags |= MSG_WAITALL;
  }
#endif

  std::size_t TotalBufSize = 0;
  for (auto &IOV : RiData) {
    TotalBufSize += IOV.size();
  }

  std::vector<uint8_t> TotalBuf(TotalBufSize, 0);

  const bool NeedAddress =
      AddressFamilyPtr != nullptr || !Address.empty() || PortPtr != nullptr;
  Variant<sockaddr_storage, sockaddr_in, sockaddr_in6, sockaddr> SockAddr;
  int MaxAllowLength;
  if (NeedAddress) {
    MaxAllowLength = sizeof(SockAddr);
  }

  if (auto Res = recvfrom(Socket, reinterpret_cast<char *>(TotalBuf.data()),
                          static_cast<int>(TotalBufSize), SysRiFlags,
                          NeedAddress ? &SockAddr.get<sockaddr>() : nullptr,
                          NeedAddress ? &MaxAllowLength : nullptr);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  } else {
    NRead = static_cast<__wasi_size_t>(Res);
  }

  if (NeedAddress) {
    switch (SockAddr.get<sockaddr_storage>().ss_family) {
    case AF_INET: {
      const auto &SockAddr4 = SockAddr.get<sockaddr_in>();
      if (AddressFamilyPtr) {
        *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
      }
      if (Address.size() >= sizeof(in_addr)) {
        std::memcpy(Address.data(), &SockAddr4.sin_addr, sizeof(in_addr));
      }
      if (PortPtr != nullptr) {
        *PortPtr = SockAddr4.sin_port;
      }
      break;
    }
    case AF_INET6: {
      const auto &SockAddr6 = SockAddr.get<sockaddr_in6>();
      if (AddressFamilyPtr) {
        *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
      }
      if (Address.size() >= sizeof(in6_addr)) {
        std::memcpy(Address.data(), &SockAddr6.sin6_addr, sizeof(in6_addr));
      }
      if (PortPtr != nullptr) {
        *PortPtr = SockAddr6.sin6_port;
      }
      break;
    }
    default:
      return WasiUnexpect(__WASI_ERRNO_NOSYS);
    }
  }

  RoFlags = static_cast<__wasi_roflags_t>(0);

  Span<uint8_t> TotalBufView(TotalBuf);
  for (auto &IOV : RiData) {
    const auto Size = std::min(IOV.size(), TotalBufView.size());
    std::copy_n(TotalBufView.begin(), Size, IOV.begin());
    TotalBufView = TotalBufView.subspan(Size);
    if (TotalBufView.empty()) {
      break;
    }
  }

  return {};
}

WasiExpect<void> INode::sockSend(Span<Span<const uint8_t>> SiData,
                                 __wasi_siflags_t,
                                 __wasi_size_t &NWritten) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  std::size_t TotalBufSize = 0;
  for (auto &IOV : SiData) {
    TotalBufSize += IOV.size();
  }
  std::vector<uint8_t> TotalBuf(TotalBufSize);
  Span<uint8_t> TotalBufView(TotalBuf);
  for (auto &IOV : SiData) {
    std::copy_n(IOV.begin(), IOV.size(), TotalBufView.begin());
    TotalBufView = TotalBufView.subspan(IOV.size());
  }
  assuming(TotalBufView.empty());

  if (auto Res = send(Socket, reinterpret_cast<char *>(TotalBuf.data()),
                      static_cast<int>(TotalBuf.size()), 0);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  } else {
    NWritten = static_cast<__wasi_size_t>(Res);
  }

  return {};
}

WasiExpect<void> INode::sockSendTo(Span<Span<const uint8_t>> SiData,
                                   __wasi_siflags_t,
                                   __wasi_address_family_t AddressFamily,
                                   Span<const uint8_t> Address, uint16_t Port,
                                   __wasi_size_t &NWritten) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  std::size_t TotalBufSize = 0;
  for (auto &IOV : SiData) {
    TotalBufSize += IOV.size();
  }
  std::vector<uint8_t> TotalBuf(TotalBufSize);
  Span<uint8_t> TotalBufView(TotalBuf);
  for (auto &IOV : SiData) {
    std::copy_n(IOV.begin(), IOV.size(), TotalBufView.begin());
    TotalBufView = TotalBufView.subspan(IOV.size());
  }
  assuming(TotalBufView.empty());

  Variant<sockaddr, sockaddr_in, sockaddr_in6> ClientAddr;
  socklen_t MsgNameLen = 0;

  if (AddressFamily == __WASI_ADDRESS_FAMILY_INET4) {
    auto &ClientAddr4 = ClientAddr.emplace<sockaddr_in>();
    MsgNameLen = sizeof(ClientAddr4);

    ClientAddr4.sin_family = AF_INET;
    ClientAddr4.sin_port = htons(Port);
    assuming(Address.size() >= sizeof(in_addr));
    std::memcpy(&ClientAddr4.sin_addr, Address.data(), sizeof(in_addr));
  } else if (AddressFamily == __WASI_ADDRESS_FAMILY_INET6) {
    auto &ClientAddr6 = ClientAddr.emplace<sockaddr_in6>();
    MsgNameLen = sizeof(ClientAddr6);

    ClientAddr6.sin6_family = AF_INET6;
    ClientAddr6.sin6_flowinfo = 0;
    ClientAddr6.sin6_port = htons(Port);
    assuming(Address.size() >= sizeof(in6_addr));
    std::memcpy(&ClientAddr6.sin6_addr, Address.data(), sizeof(in6_addr));
  }

  const int SysSiFlags = 0;

  if (auto Res = sendto(Socket, reinterpret_cast<char *>(TotalBuf.data()),
                        static_cast<int>(TotalBufSize), SysSiFlags,
                        MsgNameLen == 0 ? nullptr : &ClientAddr.get<sockaddr>(),
                        MsgNameLen);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  } else {
    NWritten = static_cast<__wasi_size_t>(Res);
  }

  return {};
}

WasiExpect<void> INode::sockShutdown(__wasi_sdflags_t SdFlags) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  int SysFlags;
  switch (static_cast<uint8_t>(SdFlags)) {
  case __WASI_SDFLAGS_RD:
    SysFlags = SD_RECEIVE;
    break;
  case __WASI_SDFLAGS_WR:
    SysFlags = SD_SEND;
    break;
  case __WASI_SDFLAGS_RD | __WASI_SDFLAGS_WR:
    SysFlags = SD_BOTH;
    break;
  default:
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  if (auto Res = shutdown(Socket, SysFlags); unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  return {};
}

WasiExpect<void> INode::sockGetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   Span<uint8_t> &Flag) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);
  socklen_t Size = static_cast<socklen_t>(Flag.size());
  if (auto Res = getsockopt(Socket, SysSockOptLevel, SysSockOptName,
                            reinterpret_cast<char *>(Flag.data()), &Size);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  switch (SockOptName) {
  case __WASI_SOCK_OPT_SO_ERROR: {
    assuming(Size == sizeof(int));
    Flag = Flag.first(static_cast<size_t>(Size));
    auto &Error = *reinterpret_cast<int *>(Flag.data());
    Error = static_cast<int>(fromErrNo(Error));
    break;
  }
  case __WASI_SOCK_OPT_SO_TYPE: {
    assuming(Size == sizeof(int));
    Flag = Flag.first(static_cast<size_t>(Size));
    auto &SockType = *reinterpret_cast<int *>(Flag.data());
    SockType = static_cast<int>(fromSockType(SockType));
    break;
  }
  case __WASI_SOCK_OPT_SO_LINGER: {
    assuming(Size == sizeof(LINGER));
    struct WasiLinger {
      int32_t l_onoff;
      int32_t l_linger;
    };
    if (Flag.size() < sizeof(WasiLinger)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    {
      const auto SysLinger = *reinterpret_cast<const LINGER *>(Flag.data());
      WasiLinger Linger = {SysLinger.l_onoff, SysLinger.l_linger};
      *reinterpret_cast<WasiLinger *>(Flag.data()) = Linger;
    }
    Size = sizeof(WasiLinger);
    Flag = Flag.first(static_cast<size_t>(Size));
    break;
  }
  case __WASI_SOCK_OPT_SO_SNDTIMEO:
  case __WASI_SOCK_OPT_SO_RCVTIMEO: {
    assuming(Size == sizeof(DWORD_));
    struct WasiTimeVal {
      int64_t TVSec;
      int64_t TVUSec;
    };
    if (Flag.size() < sizeof(WasiTimeVal)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto SysTimeout = std::chrono::milliseconds(
        *reinterpret_cast<const DWORD_ *>(Flag.data()));
    const auto Secs =
        std::chrono::duration_cast<std::chrono::seconds>(SysTimeout);
    auto &Timeout = *reinterpret_cast<WasiTimeVal *>(Flag.data());
    Timeout.TVSec = Secs.count();
    Timeout.TVUSec = (SysTimeout - std::chrono::milliseconds(Secs)).count();
    Size = sizeof(WasiTimeVal);
    Flag = Flag.first(static_cast<size_t>(Size));
    break;
  }
  default:
    Flag = Flag.first(static_cast<size_t>(Size));
  }

  return {};
}

WasiExpect<void> INode::sockSetOpt(__wasi_sock_opt_level_t SockOptLevel,
                                   __wasi_sock_opt_so_t SockOptName,
                                   Span<const uint8_t> Flag) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }
  auto SysSockOptLevel = toSockOptLevel(SockOptLevel);
  auto SysSockOptName = toSockOptSoName(SockOptName);

  if (auto Res = setsockopt(Socket, SysSockOptLevel, SysSockOptName,
                            reinterpret_cast<const char *>(Flag.data()),
                            static_cast<int>(Flag.size()));
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  return {};
}

WasiExpect<void>
INode::sockGetLocalAddr(__wasi_address_family_t *AddressFamilyPtr,
                        Span<uint8_t> Address,
                        uint16_t *PortPtr) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  Variant<sockaddr, sockaddr_in, sockaddr_in6, sockaddr_storage> SocketAddr;
  socklen_t Slen = sizeof(SocketAddr);

  if (auto Res = getsockname(Socket, &SocketAddr.get<sockaddr>(), &Slen);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  switch (SocketAddr.get<sockaddr_storage>().ss_family) {
  case AF_INET: {
    if (Address.size() < sizeof(in_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr4 = SocketAddr.get<sockaddr_in>();
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr4.sin_port);
    }
    std::memcpy(Address.data(), &SocketAddr4.sin_addr, sizeof(in_addr));
    return {};
  }
  case AF_INET6: {
    if (Address.size() < sizeof(in6_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr6 = SocketAddr.get<sockaddr_in6>();
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr6.sin6_port);
    }
    std::memcpy(Address.data(), &SocketAddr6.sin6_addr, sizeof(in6_addr));
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

WasiExpect<void>
INode::sockGetPeerAddr(__wasi_address_family_t *AddressFamilyPtr,
                       Span<uint8_t> Address,
                       uint16_t *PortPtr) const noexcept {
  if (auto Res = detail::ensureWSAStartup(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  }

  Variant<sockaddr, sockaddr_in, sockaddr_in6, sockaddr_storage> SocketAddr;
  socklen_t Slen = sizeof(SocketAddr);

  if (auto Res = getpeername(Socket, &SocketAddr.get<sockaddr>(), &Slen);
      unlikely(Res == SOCKET_ERROR_)) {
    return WasiUnexpect(detail::fromWSALastError());
  }

  switch (SocketAddr.get<sockaddr_storage>().ss_family) {
  case AF_INET: {
    if (Address.size() < sizeof(in_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr4 = SocketAddr.get<sockaddr_in>();
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET4;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr4.sin_port);
    }
    std::memcpy(Address.data(), &SocketAddr4.sin_addr, sizeof(in_addr));
    return {};
  }
  case AF_INET6: {
    if (Address.size() < sizeof(in6_addr)) {
      return WasiUnexpect(__WASI_ERRNO_NOMEM);
    }
    const auto &SocketAddr6 = SocketAddr.get<sockaddr_in6>();
    if (AddressFamilyPtr) {
      *AddressFamilyPtr = __WASI_ADDRESS_FAMILY_INET6;
    }
    if (PortPtr) {
      *PortPtr = ntohs(SocketAddr6.sin6_port);
    }
    std::memcpy(Address.data(), &SocketAddr6.sin6_addr, sizeof(in6_addr));
    return {};
  }
  default:
    return WasiUnexpect(__WASI_ERRNO_NOSYS);
  }
}

WasiExpect<__wasi_filetype_t> INode::filetype() const noexcept {
  switch (fastGetFileType(Type, Handle)) {
  case FILE_TYPE_DISK_:
    if (auto Res = getAttribute(Handle); unlikely(!Res)) {
      return WasiUnexpect(Res);
    } else {
      return getDiskFileType(*Res);
    }
  case FILE_TYPE_CHAR_:
    return __WASI_FILETYPE_CHARACTER_DEVICE;
  case FILE_TYPE_PIPE_:
    if (Type == HandleType::NormalSocket) {
      return getSocketType(Socket);
    } else {
      return __WASI_FILETYPE_CHARACTER_DEVICE;
    }
  }
  return __WASI_FILETYPE_UNKNOWN;
}

bool INode::isDirectory() const noexcept {
  if (auto Res = getAttribute(Handle); unlikely(!Res)) {
    return false;
  } else {
    return (*Res) & FILE_ATTRIBUTE_DIRECTORY_;
  }
}

bool INode::isSymlink() const noexcept {
  if (auto Res = getAttribute(Handle); unlikely(!Res)) {
    return false;
  } else {
    return (*Res) & FILE_ATTRIBUTE_REPARSE_POINT_;
  }
}

WasiExpect<__wasi_filesize_t> INode::filesize() const noexcept {
  if (LARGE_INTEGER_ FileSize; unlikely(!GetFileSizeEx(Handle, &FileSize))) {
    return WasiUnexpect(detail::fromLastError(GetLastError()));
  } else {
    return static_cast<__wasi_filesize_t>(FileSize.QuadPart);
  }
}

bool INode::canBrowse() const noexcept { return SavedVFSFlags & VFS::Read; }

Poller::Poller(PollerContext &C) noexcept : Ctx(C) {}

WasiExpect<void> Poller::prepare(Span<__wasi_event_t> E) noexcept {
  WasiEvents = E;
  try {
    Events.reserve(E.size());
  } catch (std::bad_alloc &) {
    return WasiUnexpect(__WASI_ERRNO_NOMEM);
  }
  return {};
}

void Poller::clock(__wasi_clockid_t Clock, __wasi_timestamp_t Timeout,
                   __wasi_timestamp_t Precision, __wasi_subclockflags_t Flags,
                   __wasi_userdata_t UserData) noexcept {
  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_CLOCK;

  if (Flags & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) {
    __wasi_timestamp_t Now;
    if (auto Res = Clock::clockTimeGet(Clock, Precision, Now); unlikely(!Res)) {
      Event.Valid = true;
      Event.error = Res.error();
      return;
    }
    if (Timeout < Now) {
      // already expired
      Event.Valid = true;
      Event.error = __WASI_ERRNO_SUCCESS;
      return;
    }
    Timeout -= Now;
  }
  const auto Micros = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::nanoseconds(Timeout));
  const auto Secs = std::chrono::duration_cast<std::chrono::seconds>(Micros);

  TIMEVAL_ SysTimeout;
  SysTimeout.tv_sec = static_cast<long>(Secs.count());
  SysTimeout.tv_usec =
      static_cast<long>(std::chrono::microseconds(Micros - Secs).count());

  if (TimeoutEvent == nullptr || MinimumTimeout.tv_sec > SysTimeout.tv_sec ||
      (MinimumTimeout.tv_sec == SysTimeout.tv_sec &&
       MinimumTimeout.tv_usec > SysTimeout.tv_usec)) {
    TimeoutEvent = &Event;
    MinimumTimeout = SysTimeout;
  }
}

void Poller::close(const INode &) noexcept {}

void Poller::read(const INode &Node, TriggerType Trigger,
                  __wasi_userdata_t UserData) noexcept {
  if (Node.Type == HandleHolder::HandleType::StdHandle) {
    if (ReadFds.fd_count > 0 || WriteFds.fd_count > 0) {
      // Cannot wait on socket and console at the same time
      error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_READ);
      return;
    }

    assuming(Events.size() < WasiEvents.size());
    auto &Event = Events.emplace_back();
    Event.Valid = false;
    Event.userdata = UserData;
    Event.type = __WASI_EVENTTYPE_FD_READ;

    try {
      auto [Iter, Added] = ConsoleReadEvent.try_emplace(Node.Handle);
      Iter->second = &Event;
    } catch (std::bad_alloc &) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_NOMEM;
      return;
    }

    return;
  }

  if (Node.Type != HandleHolder::HandleType::NormalSocket ||
      Trigger != TriggerType::Level) {
    // Windows does not support polling other then socket, and only with level
    // triggering.
    error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_READ);
    return;
  }
  if (!ConsoleReadEvent.empty() || !ConsoleWriteEvent.empty()) {
    // Cannot wait on socket and console at the same time
    error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_READ);
    return;
  }
  if (ReadFds.fd_count == FD_SETSIZE_) {
    error(UserData, __WASI_ERRNO_NOMEM, __WASI_EVENTTYPE_FD_READ);
    return;
  }

  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_FD_READ;

  if (ReadFds.fd_count == FD_SETSIZE_) {
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOMEM;
    return;
  }

  try {
    auto [Iter, Added] = SocketDatas.try_emplace(Node.Socket);

    if (unlikely(!Added && Iter->second.ReadEvent != nullptr)) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_EXIST;
      return;
    }

    Iter->second.ReadEvent = &Event;
    ReadFds.fd_array[ReadFds.fd_count++] = Node.Socket;
  } catch (std::bad_alloc &) {
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOMEM;
    return;
  }
}

void Poller::write(const INode &Node, TriggerType Trigger,
                   __wasi_userdata_t UserData) noexcept {
  if (Node.Type == HandleHolder::HandleType::StdHandle) {
    if (ReadFds.fd_count > 0 || WriteFds.fd_count > 0) {
      // Cannot wait on socket and console at the same time
      error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_WRITE);
      return;
    }

    assuming(Events.size() < WasiEvents.size());
    auto &Event = Events.emplace_back();
    Event.Valid = false;
    Event.userdata = UserData;
    Event.type = __WASI_EVENTTYPE_FD_WRITE;

    try {
      auto [Iter, Added] = ConsoleWriteEvent.try_emplace(Node.Handle);
      Iter->second = &Event;
    } catch (std::bad_alloc &) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_NOMEM;
      return;
    }

    return;
  }
  if (Node.Type != HandleHolder::HandleType::NormalSocket ||
      Trigger != TriggerType::Level) {
    // Windows does not support polling other then socket, and only with level
    // triggering.
    error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_WRITE);
    return;
  }
  if (!ConsoleReadEvent.empty() || !ConsoleWriteEvent.empty()) {
    // Cannot wait on socket and console at the same time
    error(UserData, __WASI_ERRNO_NOSYS, __WASI_EVENTTYPE_FD_WRITE);
    return;
  }
  if (WriteFds.fd_count == FD_SETSIZE_) {
    error(UserData, __WASI_ERRNO_NOMEM, __WASI_EVENTTYPE_FD_WRITE);
    return;
  }

  assuming(Events.size() < WasiEvents.size());
  auto &Event = Events.emplace_back();
  Event.Valid = false;
  Event.userdata = UserData;
  Event.type = __WASI_EVENTTYPE_FD_WRITE;

  try {
    auto [Iter, Added] = SocketDatas.try_emplace(Node.Socket);

    if (unlikely(!Added && Iter->second.WriteEvent != nullptr)) {
      Event.Valid = true;
      Event.error = __WASI_ERRNO_EXIST;
      return;
    }

    Iter->second.WriteEvent = &Event;
    WriteFds.fd_array[WriteFds.fd_count++] = Node.Socket;
  } catch (std::bad_alloc &) {
    Event.Valid = true;
    Event.error = __WASI_ERRNO_NOMEM;
    return;
  }
}

void Poller::wait() noexcept {
  if (!ConsoleWriteEvent.empty()) {
    assuming(ReadFds.fd_count == 0 && WriteFds.fd_count == 0);
    // Console can always write
    for (const auto &[NodeHandle, Event] : ConsoleWriteEvent) {
      Event->Valid = true;
      Event->error = __WASI_ERRNO_SUCCESS;
    }
    ConsoleWriteEvent.clear();
    ConsoleReadEvent.clear();
    TimeoutEvent = nullptr;
    return;
  }
  if (!ConsoleReadEvent.empty()) {
    assuming(ReadFds.fd_count == 0 && WriteFds.fd_count == 0);
    DWORD_ Timeout = INFINITE_;
    if (TimeoutEvent != nullptr) {
      const std::chrono::microseconds MicroSecs =
          std::chrono::seconds(MinimumTimeout.tv_sec) +
          std::chrono::microseconds(MinimumTimeout.tv_sec);
      Timeout = static_cast<DWORD_>(MicroSecs.count());
    }
    std::vector<HANDLE_> Handles;
    DWORD_ Count = std::min(static_cast<DWORD_>(ConsoleReadEvent.size()),
                            MAXIMUM_WAIT_OBJECTS_);
    Handles.reserve(Count);
    for (const auto &[NodeHandle, Event] : ConsoleReadEvent) {
      if (likely(Handles.size() < Count)) {
        Handles.push_back(NodeHandle);
      }
    }
    const auto Result =
        WaitForMultipleObjects(Count, Handles.data(), false, Timeout);
    assuming(static_cast<DWORD_>(0) <= Result);
    if (likely(Result < Count)) {
      ConsoleReadEvent[Handles[Result]]->Valid = true;
      ConsoleReadEvent[Handles[Result]]->error = __WASI_ERRNO_SUCCESS;
    } else {
      switch (Result) {
      case WAIT_TIMEOUT_:
        if (likely(TimeoutEvent)) {
          TimeoutEvent->Valid = true;
          TimeoutEvent->error = __WASI_ERRNO_SUCCESS;
        }
        break;
      case WAIT_FAILED_:
      default: {
        const auto Error = detail::fromLastError(GetLastError());
        for (const auto &[NodeHandle, Event] : ConsoleWriteEvent) {
          Event->Valid = true;
          Event->error = Error;
        }
        break;
      }
      }
    }
    assuming(ConsoleWriteEvent.empty());
    ConsoleReadEvent.clear();
    TimeoutEvent = nullptr;
    return;
  }
  if (const int Count =
          select(0, &ReadFds, &WriteFds, nullptr,
                 TimeoutEvent != nullptr ? &MinimumTimeout : nullptr);
      Count == 0) {
    if (TimeoutEvent) {
      TimeoutEvent->Valid = true;
      TimeoutEvent->error = __WASI_ERRNO_SUCCESS;
    }
  } else {
    for (const auto Socket :
         Span<const SOCKET_>(ReadFds.fd_array, ReadFds.fd_count)) {
      const auto Iter = SocketDatas.find(Socket);
      assuming(Iter != SocketDatas.end());
      assuming(Iter->second.ReadEvent);
      auto &Event = *Iter->second.ReadEvent;
      assuming(Event.type == __WASI_EVENTTYPE_FD_READ);
      Event.Valid = true;
      Event.error = __WASI_ERRNO_SUCCESS;

      bool UnknownNBytes = false;
      u_long ReadBufUsed = 0;
      if (auto Res = ioctlsocket(Socket, FIONREAD, &ReadBufUsed);
          unlikely(Res == 0)) {
        UnknownNBytes = true;
      }
      if (UnknownNBytes) {
        Event.fd_readwrite.nbytes = 1;
      } else {
        Event.fd_readwrite.nbytes = ReadBufUsed;
      }
    }
    for (const auto Socket :
         Span<const SOCKET_>(WriteFds.fd_array, WriteFds.fd_count)) {
      const auto Iter = SocketDatas.find(Socket);
      assuming(Iter != SocketDatas.end());
      assuming(Iter->second.WriteEvent);
      auto &Event = *Iter->second.WriteEvent;
      assuming(Event.type == __WASI_EVENTTYPE_FD_WRITE);
      Event.Valid = true;
      Event.error = __WASI_ERRNO_SUCCESS;
      Event.fd_readwrite.nbytes = 1;
    }
  }
  SocketDatas.clear();
  ReadFds.fd_count = 0;
  WriteFds.fd_count = 0;
  TimeoutEvent = nullptr;
}

void Poller::reset() noexcept {
  WasiEvents = {};
  Events.clear();
}

bool Poller::ok() noexcept { return true; }

} // namespace WASI
} // namespace Host
} // namespace WasmEdge

#endif
