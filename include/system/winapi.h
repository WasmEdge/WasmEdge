// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/system/winapi.h - Wrapper for Windows API-----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains helper to call Windows API.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include <cstdint>

#if WASMEDGE_OS_WINDOWS

#if defined(__GNUC__) || defined(__clang__)
#define WASMEDGE_WINAPI_DETAIL_EXTENSION __extension__
#define WASMEDGE_WINAPI_FORCEINLINE [[gnu::always_inline]]
#define WASMEDGE_WINAPI_SYMBOL_IMPORT [[gnu::dllimport]]
#else
#define WASMEDGE_WINAPI_DETAIL_EXTENSION
#define WASMEDGE_WINAPI_FORCEINLINE __forceinline
#define WASMEDGE_WINAPI_SYMBOL_IMPORT __declspec(dllimport)
#endif

#if defined(_M_IX86) || defined(__i386__)
#ifdef __GNUC__
#define WASMEDGE_WINAPI_WINAPI_CC [[gnu::stdcall]]
#else
#define WASMEDGE_WINAPI_WINAPI_CC __stdcall
#endif
#else
#define WASMEDGE_WINAPI_WINAPI_CC
#endif

// _WIN32_WINNT version constants
#define _WIN32_WINNT_NT4 0x0400     // Windows NT 4.0
#define _WIN32_WINNT_WIN2K 0x0500   // Windows 2000
#define _WIN32_WINNT_WINXP 0x0501   // Windows XP
#define _WIN32_WINNT_WS03 0x0502    // Windows Server 2003
#define _WIN32_WINNT_VISTA 0x0600   // Windows Vista
#define _WIN32_WINNT_WS08 0x0600    // Windows Server 2008
#define _WIN32_WINNT_WIN7 0x0601    // Windows 7
#define _WIN32_WINNT_WIN8 0x0602    // Windows 8
#define _WIN32_WINNT_WINBLUE 0x0603 // Windows 8.1
#define _WIN32_WINNT_WIN10 0x0A00   // Windows 10

#define NTDDI_WIN2K 0x05000000
#define NTDDI_WINXP 0x05010000
#define NTDDI_WINXPSP1 0x05010100
#define NTDDI_WINXPSP2 0x05010200
#define NTDDI_WINXPSP3 0x05010300
#define NTDDI_WS03 0x05020000
#define NTDDI_WS03SP1 0x05020100
#define NTDDI_WS03SP2 0x05020200
#define NTDDI_VISTA 0x06000000
#define NTDDI_VISTASP1 0x06000100
#define NTDDI_WS08 0x06000100
#define NTDDI_WIN7 0x06010000
#define NTDDI_WIN8 0x06020000
#define NTDDI_WINBLUE 0x06030000
#define NTDDI_WIN10 0x0A000000
#define NTDDI_WIN10_TH2 0x0A000001
#define NTDDI_WIN10_RS1 0x0A000002
#define NTDDI_WIN10_RS2 0x0A000003
#define NTDDI_WIN10_RS3 0x0A000004
#define NTDDI_WIN10_RS4 0x0A000005
#define NTDDI_WIN10_RS5 0x0A000006
#define NTDDI_WIN10_19H1 0x0A000007
#define WDK_NTDDI_VERSION NTDDI_WIN10_19H1

// Set default version to Windows 8

#ifndef _WIN32_WINNT
#ifdef WINVER
#define _WIN32_WINNT WINVER
#else
#define _WIN32_WINNT _WIN32_WINNT_WIN8
#endif
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION (_WIN32_WINNT << 16)
#endif

#define WINAPI_FAMILY_PC_APP 2        /// Windows Store Applications
#define WINAPI_FAMILY_PHONE_APP 3     /// Windows Phone Applications
#define WINAPI_FAMILY_SYSTEM 4        /// Windows Drivers and Tools
#define WINAPI_FAMILY_SERVER 5        /// Windows Server Applications
#define WINAPI_FAMILY_GAMES 6         /// Windows Games and Applications
#define WINAPI_FAMILY_DESKTOP_APP 100 /// Windows Desktop Applications

#ifndef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#endif

#define WINAPI_PARTITION_DESKTOP (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

#define WINAPI_PARTITION_APP                                                   \
  (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP ||                               \
   WINAPI_FAMILY == WINAPI_FAMILY_PC_APP ||                                    \
   WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)

#ifndef WINAPI_PARTITION_PC_APP
#define WINAPI_PARTITION_PC_APP                                                \
  (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP ||                               \
   WINAPI_FAMILY == WINAPI_FAMILY_PC_APP)
#endif

#ifndef WINAPI_PARTITION_PHONE_APP
#define WINAPI_PARTITION_PHONE_APP (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#endif

#ifndef WINAPI_PARTITION_GAMES
#define WINAPI_PARTITION_GAMES                                                 \
  (WINAPI_FAMILY == WINAPI_FAMILY_GAMES ||                                     \
   WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif

#define WINAPI_PARTITION_SYSTEM                                                \
  (WINAPI_FAMILY == WINAPI_FAMILY_SYSTEM ||                                    \
   WINAPI_FAMILY == WINAPI_FAMILY_SERVER)

#define WINAPI_FAMILY_PARTITION(V) (V)

namespace WasmEdge::winapi {

using BOOL_ = int;
using PBOOL_ = BOOL_ *;
using LPBOOL_ = BOOL_ *;
using BYTE_ = uint8_t;
using PBYTE_ = BYTE_ *;
using LPBYTE_ = BYTE_ *;
using UCHAR_ = uint8_t;
using PUCHAR_ = UCHAR_ *;
using BOOLEAN_ = BYTE_;
using PBOOLEAN_ = BOOLEAN_ *;
using WORD_ = uint16_t;
using PWORD_ = WORD_ *;
using LPWORD_ = WORD_ *;
using DWORD_ = uint32_t;
using PDWORD_ = DWORD_ *;
using LPDWORD_ = DWORD_ *;
using VOID_ = void;
using PVOID_ = void *;
using LPVOID_ = void *;
using LPCVOID_ = const void *;
using HANDLE_ = void *;
using PHANDLE_ = HANDLE_ *;

using SHORT_ = short;
using PSHORT_ = SHORT_ *;
using USHORT_ = unsigned short;
using PUSHORT_ = USHORT_ *;
using INT_ = int;
using PINT_ = INT_ *;
using LPINT_ = INT_ *;
using UINT_ = unsigned int;
using PUINT_ = UINT_ *;
using LONG_ = int32_t;
using ULONG_ = uint32_t;
using PLONG_ = LONG_ *;
using LPLONG_ = LONG_ *;
using PULONG_ = ULONG_ *;
using LONGLONG_ = int64_t;
using ULONGLONG_ = uint64_t;

using ULONG64_ = uint64_t;

using INT_PTR_ = intptr_t;
using UINT_PTR_ = uintptr_t;
using ULONG_PTR_ = uintptr_t;

using SIZE_T_ = size_t;

using CHAR_ = char;
using CCHAR_ = char;
using LPSTR_ = CHAR_ *;
using PCSTR_ = const CHAR_ *;
using LPCSTR_ = const CHAR_ *;
using WCHAR_ = wchar_t;
using PWSTR_ = WCHAR_ *;
using LPWSTR_ = WCHAR_ *;
using PCWSTR_ = const WCHAR_ *;
using LPCWSTR_ = const WCHAR_ *;

using NTSTATUS_ = LONG_;

using LARGE_INTEGER_ = union _LARGE_INTEGER {
  WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
    DWORD_ LowPart;
    LONG_ HighPart;
  };
  struct {
    DWORD_ LowPart;
    LONG_ HighPart;
  } u;
  LONGLONG_ QuadPart;

  _LARGE_INTEGER() = default;
  constexpr _LARGE_INTEGER(LONGLONG_ Quad) : QuadPart(Quad) {}
};

using ULARGE_INTEGER_ = union _ULARGE_INTEGER {
  WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
    DWORD_ LowPart;
    DWORD_ HighPart;
  };
  struct {
    DWORD_ LowPart;
    DWORD_ HighPart;
  } u;
  ULONGLONG_ QuadPart;

  constexpr _ULARGE_INTEGER(DWORD_ LowPart, DWORD_ HighPart)
      : LowPart(LowPart), HighPart(HighPart) {}
  constexpr _ULARGE_INTEGER(ULONGLONG_ Quad) : QuadPart(Quad) {}
};

using SECURITY_ATTRIBUTES_ = struct _SECURITY_ATTRIBUTES {
  DWORD_ nLength;
  LPVOID_ lpSecurityDescriptor;
  BOOL_ bInheritHandle;
};
using PSECURITY_ATTRIBUTES_ = SECURITY_ATTRIBUTES_ *;
using LPSECURITY_ATTRIBUTES_ = SECURITY_ATTRIBUTES_ *;

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_WIN8
using CREATEFILE2_EXTENDED_PARAMETERS_ =
    struct _CREATEFILE2_EXTENDED_PARAMETERS {
  DWORD_ dwSize;
  DWORD_ dwFileAttributes;
  DWORD_ dwFileFlags;
  DWORD_ dwSecurityQosFlags;
  LPSECURITY_ATTRIBUTES_ lpSecurityAttributes;
  HANDLE_ hTemplateFile;
};
using LPCREATEFILE2_EXTENDED_PARAMETERS_ = CREATEFILE2_EXTENDED_PARAMETERS_ *;
#endif

#if WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_WIN10
using LPPROGRESS_ROUTINE_ = DWORD_(WASMEDGE_WINAPI_WINAPI_CC *)(
    LARGE_INTEGER_ TotalFileSize, LARGE_INTEGER_ TotalBytesTransferred,
    LARGE_INTEGER_ StreamSize, LARGE_INTEGER_ StreamBytesTransferred,
    DWORD_ dwStreamNumber, DWORD_ dwCallbackReason, HANDLE_ hSourceFile,
    HANDLE_ hDestinationFile, LPVOID_ lpData);
#endif

using FILETIME_ = struct _FILETIME {
  DWORD_ dwLowDateTime;
  DWORD_ dwHighDateTime;
};
using LPFILETIME_ = FILETIME_ *;

static inline constexpr const DWORD_ MAX_PATH_ = 260;
using WIN32_FIND_DATAW_ = struct _WIN32_FIND_DATAW {
  DWORD_ dwFileAttributes;
  FILETIME_ ftCreationTime;
  FILETIME_ ftLastAccessTime;
  FILETIME_ ftLastWriteTime;
  DWORD_ nFileSizeHigh;
  DWORD_ nFileSizeLow;
  DWORD_ dwReserved0;
  DWORD_ dwReserved1;
  WCHAR_ cFileName[MAX_PATH_];
  WCHAR_ cAlternateFileName[14];
  [[deprecated]] DWORD_ dwFileType;    // Obsolete. Do not use
  [[deprecated]] DWORD_ dwCreatorType; // Obsolete. Do not use
  [[deprecated]] WORD_ wFinderFlags;   // Obsolete. Do not use
};
using PWIN32_FIND_DATAW_ = WIN32_FIND_DATAW_ *;
using LPWIN32_FIND_DATAW_ = WIN32_FIND_DATAW_ *;

using BY_HANDLE_FILE_INFORMATION_ = struct _BY_HANDLE_FILE_INFORMATION {
  DWORD_ dwFileAttributes;
  FILETIME_ ftCreationTime;
  FILETIME_ ftLastAccessTime;
  FILETIME_ ftLastWriteTime;
  DWORD_ dwVolumeSerialNumber;
  DWORD_ nFileSizeHigh;
  DWORD_ nFileSizeLow;
  DWORD_ nNumberOfLinks;
  DWORD_ nFileIndexHigh;
  DWORD_ nFileIndexLow;
};
using LPBY_HANDLE_FILE_INFORMATION_ = BY_HANDLE_FILE_INFORMATION_ *;

using FILE_STANDARD_INFO_ = struct _FILE_STANDARD_INFO {
  LARGE_INTEGER_ AllocationSize;
  LARGE_INTEGER_ EndOfFile;
  DWORD_ NumberOfLinks;
  BOOLEAN_ DeletePending;
  BOOLEAN_ Directory;
};

using FILE_BASIC_INFO_ = struct _FILE_BASIC_INFO {
  LARGE_INTEGER_ CreationTime;
  LARGE_INTEGER_ LastAccessTime;
  LARGE_INTEGER_ LastWriteTime;
  LARGE_INTEGER_ ChangeTime;
  DWORD_ FileAttributes;
};

using FILE_END_OF_FILE_INFO_ = struct _FILE_END_OF_FILE_INFO {
  LARGE_INTEGER_ EndOfFile;
};

using FILE_ATTRIBUTE_TAG_INFO_ = struct _FILE_ATTRIBUTE_TAG_INFO {
  DWORD_ FileAttributes;
  DWORD_ ReparseTag;
};

using FILE_ID_BOTH_DIR_INFO_ = struct _FILE_ID_BOTH_DIR_INFO {
  DWORD_ NextEntryOffset;
  DWORD_ FileIndex;
  LARGE_INTEGER_ CreationTime;
  LARGE_INTEGER_ LastAccessTime;
  LARGE_INTEGER_ LastWriteTime;
  LARGE_INTEGER_ ChangeTime;
  LARGE_INTEGER_ EndOfFile;
  LARGE_INTEGER_ AllocationSize;
  DWORD_ FileAttributes;
  DWORD_ FileNameLength;
  DWORD_ EaSize;
  CCHAR_ ShortNameLength;
  WCHAR_ ShortName[12];
  LARGE_INTEGER_ FileId;
  WCHAR_ FileName[1];
};

using FILE_INFO_BY_HANDLE_CLASS_ = enum _FILE_INFO_BY_HANDLE_CLASS {
  FileBasicInfo_,
  FileStandardInfo_,
  FileNameInfo_,
  FileRenameInfo_,
  FileDispositionInfo_,
  FileAllocationInfo_,
  FileEndOfFileInfo_,
  FileStreamInfo_,
  FileCompressionInfo_,
  FileAttributeTagInfo_,
  FileIdBothDirectoryInfo_,
  FileIdBothDirectoryRestartInfo_,
  FileIoPriorityHintInfo_,
  FileRemoteProtocolInfo_,
  FileFullDirectoryInfo_,
  FileFullDirectoryRestartInfo_,
#if NTDDI_VERSION >= NTDDI_WIN8
  FileStorageInfo_,
  FileAlignmentInfo_,
  FileIdInfo_,
  FileIdExtdDirectoryInfo_,
  FileIdExtdDirectoryRestartInfo_,
#endif
#if NTDDI_VERSION >= NTDDI_WIN10_RS1
  FileDispositionInfoEx_,
  FileRenameInfoEx_,
#endif
#if NTDDI_VERSION >= NTDDI_WIN10_19H1
  FileCaseSensitiveInfo_,
  FileNormalizedNameInfo_,
#endif
  MaximumFileInfoByHandleClass_
};

using OVERLAPPED_ = struct _OVERLAPPED {
  ULONG_PTR_ Internal;
  ULONG_PTR_ InternalHigh;
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      DWORD_ Offset;
      DWORD_ OffsetHigh;
    };
    PVOID_ Pointer;
  };
  HANDLE_ hEvent;
};
using LPOVERLAPPED_ = OVERLAPPED_ *;

using LPOVERLAPPED_COMPLETION_ROUTINE_ = VOID_(WASMEDGE_WINAPI_WINAPI_CC *)(
    DWORD_ dwErrorCode, DWORD_ dwNumberOfBytesTransfered,
    LPOVERLAPPED_ lpOverlapped) noexcept;

using REPARSE_DATA_BUFFER_ = struct _REPARSE_DATA_BUFFER {
  ULONG_ ReparseTag;
  USHORT_ ReparseDataLength;
  USHORT_ Reserved;
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      USHORT_ SubstituteNameOffset;
      USHORT_ SubstituteNameLength;
      USHORT_ PrintNameOffset;
      USHORT_ PrintNameLength;
      ULONG_ Flags;
      WCHAR_ PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      USHORT_ SubstituteNameOffset;
      USHORT_ SubstituteNameLength;
      USHORT_ PrintNameOffset;
      USHORT_ PrintNameLength;
      WCHAR_ PathBuffer[1];
    } MountPointReparseBuffer;
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      UCHAR_ DataBuffer[1];
    } GenericReparseBuffer;
  };
};
using PREPARSE_DATA_BUFFER = REPARSE_DATA_BUFFER_ *;

#if WINAPI_PARTITION_DESKTOP
using IO_STATUS_BLOCK_ = struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS_ Status;
    PVOID_ Pointer;
  };
  ULONG_PTR_ Information;
};
using PIO_STATUS_BLOCK_ = IO_STATUS_BLOCK_ *;

using FILE_INFORMATION_CLASS_ = enum _FILE_INFORMATION_CLASS {
  FileDirectoryInformation_ = 1,
  FileFullDirectoryInformation_,
  FileBothDirectoryInformation_,
  FileBasicInformation_,
  FileStandardInformation_,
  FileInternalInformation_,
  FileEaInformation_,
  FileAccessInformation_,
  FileNameInformation_,
  FileRenameInformation_,
  FileLinkInformation_,
  FileNamesInformation_,
  FileDispositionInformation_,
  FilePositionInformation_,
  FileFullEaInformation_,
  FileModeInformation_,
  FileAlignmentInformation_,
  FileAllInformation_,
  FileAllocationInformation_,
  FileEndOfFileInformation_,
  FileAlternateNameInformation_,
  FileStreamInformation_,
  FilePipeInformation_,
  FilePipeLocalInformation_,
  FilePipeRemoteInformation_,
  FileMailslotQueryInformation_,
  FileMailslotSetInformation_,
  FileCompressionInformation_,
  FileObjectIdInformation_,
  FileCompletionInformation_,
  FileMoveClusterInformation_,
  FileQuotaInformation_,
  FileReparsePointInformation_,
  FileNetworkOpenInformation_,
  FileAttributeTagInformation_,
  FileTrackingInformation_,
  FileIdBothDirectoryInformation_,
  FileIdFullDirectoryInformation_,
  FileValidDataLengthInformation_,
  FileShortNameInformation_,
  FileIoCompletionNotificationInformation_,
  FileIoStatusBlockRangeInformation_,
  FileIoPriorityHintInformation_,
  FileSfioReserveInformation_,
  FileSfioVolumeInformation_,
  FileHardLinkInformation_,
  FileProcessIdsUsingFileInformation_,
  FileNormalizedNameInformation_,
  FileNetworkPhysicalNameInformation_,
  FileIdGlobalTxDirectoryInformation_,
  FileIsRemoteDeviceInformation_,
  FileUnusedInformation_,
  FileNumaNodeInformation_,
  FileStandardLinkInformation_,
  FileRemoteProtocolInformation_,
  FileRenameInformationBypassAccessCheck_,
  FileLinkInformationBypassAccessCheck_,
  FileVolumeNameInformation_,
  FileIdInformation_,
  FileIdExtdDirectoryInformation_,
  FileReplaceCompletionInformation_,
  FileHardLinkFullIdInformation_,
  FileIdExtdBothDirectoryInformation_,
  FileDispositionInformationEx_,
  FileRenameInformationEx_,
  FileRenameInformationExBypassAccessCheck_,
  FileDesiredStorageClassInformation_,
  FileStatInformation_,
  FileMemoryPartitionInformation_,
  FileStatLxInformation_,
  FileCaseSensitiveInformation_,
  FileLinkInformationEx_,
  FileLinkInformationExBypassAccessCheck_,
  FileStorageReserveIdInformation_,
  FileCaseSensitiveInformationForceAccessCheck_,
  FileKnownFolderInformation_,
  FileMaximumInformation_
};

using ACCESS_MASK_ = ULONG_;
using FILE_ACCESS_INFORMATION_ = struct _FILE_ACCESS_INFORMATION {
  ACCESS_MASK_ AccessFlags;
};

using FILE_MODE_INFORMATION_ = struct _FILE_MODE_INFORMATION {
  ULONG_ Mode;
};

using OBJECT_INFORMATION_CLASS_ = enum _OBJECT_INFORMATION_CLASS {
  ObjectBasicInformation_,
  ObjectNameInformation_,
  ObjectTypeInformation_,
  ObjectAllInformation_,
  ObjectDataInformation_
};

using UNICODE_STRING_ = struct _UNICODE_STRING {
  USHORT_ Length;
  USHORT_ MaximumLength;
  PWSTR_ Buffer;
};

using OBJECT_NAME_INFORMATION_ = struct _OBJECT_NAME_INFORMATION {
  UNICODE_STRING_ Name;
};

#endif

static inline constexpr const DWORD_ ERROR_INVALID_FUNCTION_ = 1;
static inline constexpr const DWORD_ ERROR_FILE_NOT_FOUND_ = 2;
static inline constexpr const DWORD_ ERROR_ACCESS_DENIED_ = 5;
static inline constexpr const DWORD_ ERROR_INVALID_HANDLE_ = 6;
static inline constexpr const DWORD_ ERROR_NOT_ENOUGH_MEMORY_ = 8;
static inline constexpr const DWORD_ ERROR_NO_MORE_FILES_ = 18;
static inline constexpr const DWORD_ ERROR_SHARING_VIOLATION_ = 32;
static inline constexpr const DWORD_ ERROR_HANDLE_EOF_ = 38;
static inline constexpr const DWORD_ ERROR_FILE_EXISTS_ = 80;
static inline constexpr const DWORD_ ERROR_INVALID_PARAMETER_ = 87;
static inline constexpr const DWORD_ ERROR_INSUFFICIENT_BUFFER_ = 122;
static inline constexpr const DWORD_ ERROR_INVALID_NAME_ = 123;
static inline constexpr const DWORD_ ERROR_NEGATIVE_SEEK_ = 131;
static inline constexpr const DWORD_ ERROR_DIR_NOT_EMPTY_ = 145;
static inline constexpr const DWORD_ ERROR_ALREADY_EXISTS_ = 183;
static inline constexpr const DWORD_ ERROR_PIPE_BUSY_ = 231;
static inline constexpr const DWORD_ ERROR_DIRECTORY_ = 267;
static inline constexpr const DWORD_ ERROR_IO_PENDING_ = 997;
static inline constexpr const DWORD_ ERROR_INVALID_FLAGS_ = 1004;
static inline constexpr const DWORD_ ERROR_NO_UNICODE_TRANSLATION_ = 1113;
static inline constexpr const DWORD_ ERROR_NOT_ALL_ASSIGNED_ = 1300;
static inline constexpr const DWORD_ ERROR_PRIVILEGE_NOT_HELD_ = 1314;

static inline const HANDLE_ INVALID_HANDLE_VALUE_ =
    reinterpret_cast<HANDLE_>(-1);

static inline constexpr const DWORD_ INFINITE_ =
    static_cast<DWORD_>(0xffffffff);
static inline constexpr const DWORD_ WAIT_TIMEOUT_ = 258;
static inline constexpr const DWORD_ WAIT_OBJECT_0_ = 0;
static inline constexpr const DWORD_ WAIT_FAILED_ =
    static_cast<DWORD_>(0xffffffff);
static inline constexpr const DWORD_ MAXIMUM_WAIT_OBJECTS_ = 64;

static inline constexpr const DWORD_ VOLUME_NAME_DOS_ = 0x0;
static inline constexpr const DWORD_ FILE_NAME_NORMALIZED_ = 0x0;

static inline constexpr const DWORD_ FILE_TYPE_UNKNOWN_ = 0x0;
static inline constexpr const DWORD_ FILE_TYPE_DISK_ = 0x1;
static inline constexpr const DWORD_ FILE_TYPE_CHAR_ = 0x2;
static inline constexpr const DWORD_ FILE_TYPE_PIPE_ = 0x3;

static inline constexpr const DWORD_ FILE_FLAG_WRITE_THROUGH_ = 0x80000000;
static inline constexpr const DWORD_ FILE_FLAG_OVERLAPPED_ = 0x40000000;
static inline constexpr const DWORD_ FILE_FLAG_NO_BUFFERING_ = 0x20000000;
static inline constexpr const DWORD_ FILE_FLAG_SEQUENTIAL_SCAN_ = 0x08000000;
static inline constexpr const DWORD_ FILE_FLAG_BACKUP_SEMANTICS_ = 0x02000000;
static inline constexpr const DWORD_ FILE_FLAG_OPEN_REPARSE_POINT_ = 0x00200000;
static inline constexpr const DWORD_ READ_CONTROL_ = 0x00020000;
static inline constexpr const DWORD_ SYNCHRONIZE_ = 0x00100000;
static inline constexpr const DWORD_ STANDARD_RIGHTS_REQUIRED_ = 0x000F0000;
static inline constexpr const DWORD_ STANDARD_RIGHTS_READ_ = READ_CONTROL_;
static inline constexpr const DWORD_ STANDARD_RIGHTS_WRITE_ = READ_CONTROL_;
static inline constexpr const DWORD_ STANDARD_RIGHTS_EXECUTE_ = READ_CONTROL_;
static inline constexpr const DWORD_ GENERIC_READ_ = 0x80000000;
static inline constexpr const DWORD_ GENERIC_WRITE_ = 0x40000000;
static inline constexpr const DWORD_ FILE_READ_DATA_ = 0x0001;
static inline constexpr const DWORD_ FILE_WRITE_DATA_ = 0x0002;
static inline constexpr const DWORD_ FILE_APPEND_DATA_ = 0x0004;
static inline constexpr const DWORD_ FILE_READ_EA_ = 0x0008;
static inline constexpr const DWORD_ FILE_WRITE_EA_ = 0x0010;
static inline constexpr const DWORD_ FILE_EXECUTE_ = 0x0020;
static inline constexpr const DWORD_ FILE_READ_ATTRIBUTES_ = 0x0080;
static inline constexpr const DWORD_ FILE_WRITE_ATTRIBUTES_ = 0x0100;
static inline constexpr const DWORD_ FILE_GENERIC_READ_ =
    STANDARD_RIGHTS_READ_ | FILE_READ_DATA_ | FILE_READ_ATTRIBUTES_ |
    FILE_READ_EA_ | SYNCHRONIZE_;
static inline constexpr const DWORD_ FILE_GENERIC_WRITE_ =
    STANDARD_RIGHTS_WRITE_ | FILE_WRITE_DATA_ | FILE_WRITE_ATTRIBUTES_ |
    FILE_WRITE_EA_ | FILE_APPEND_DATA_ | SYNCHRONIZE_;
static inline constexpr const DWORD_ FILE_GENERIC_EXECUTE_ =
    STANDARD_RIGHTS_EXECUTE_ | FILE_EXECUTE_ | FILE_READ_ATTRIBUTES_ |
    SYNCHRONIZE_;

static inline constexpr const DWORD_ TOKEN_ASSIGN_PRIMARY_ = 0x0001;
static inline constexpr const DWORD_ TOKEN_DUPLICATE_ = 0x0002;
static inline constexpr const DWORD_ TOKEN_IMPERSONATE_ = 0x0004;
static inline constexpr const DWORD_ TOKEN_QUERY_ = 0x0008;
static inline constexpr const DWORD_ TOKEN_QUERY_SOURCE_ = 0x0010;
static inline constexpr const DWORD_ TOKEN_ADJUST_PRIVILEGES_ = 0x0020;
static inline constexpr const DWORD_ TOKEN_ADJUST_GROUPS_ = 0x0040;
static inline constexpr const DWORD_ TOKEN_ADJUST_DEFAULT_ = 0x0080;
static inline constexpr const DWORD_ TOKEN_ADJUST_SESSIONID_ = 0x0100;

static inline constexpr const DWORD_ FILE_DEVICE_FILE_SYSTEM_ = 0x9;
static inline constexpr const DWORD_ METHOD_BUFFERED_ = 0;
static inline constexpr const DWORD_ FILE_ANY_ACCESS_ = 0;
static inline constexpr DWORD_ CTL_CODE_(const DWORD_ DeviceType,
                                         const DWORD_ Function,
                                         const DWORD_ Method,
                                         const DWORD_ Access) noexcept {
  return (DeviceType << 16) | (Access << 14) | (Function << 2) | Method;
}
static inline constexpr const DWORD_ FSCTL_GET_REPARSE_POINT_ =
    CTL_CODE_(FILE_DEVICE_FILE_SYSTEM_, 42, METHOD_BUFFERED_, FILE_ANY_ACCESS_);
static inline constexpr const ULONG_ SYMLINK_FLAG_RELATIVE_ = 1;

static inline constexpr const DWORD_ IO_REPARSE_TAG_SYMLINK_ = 0xA000000CL;
static inline constexpr const DWORD_ IO_REPARSE_TAG_MOUNT_POINT_ = 0xA0000003L;

static inline constexpr const DWORD_ TOKEN_ALL_ACCESS_P_ =
    (STANDARD_RIGHTS_REQUIRED_ | TOKEN_ASSIGN_PRIMARY_ | TOKEN_DUPLICATE_ |
     TOKEN_IMPERSONATE_ | TOKEN_QUERY_ | TOKEN_QUERY_SOURCE_ |
     TOKEN_ADJUST_PRIVILEGES_ | TOKEN_ADJUST_GROUPS_ | TOKEN_ADJUST_DEFAULT_);
static inline constexpr const DWORD_ TOKEN_ALL_ACCESS_ =
    (TOKEN_ALL_ACCESS_P_ | TOKEN_ADJUST_SESSIONID_);

static inline constexpr const DWORD_ FILE_ATTRIBUTE_DIRECTORY_ = 0x00000010;
static inline constexpr const DWORD_ FILE_ATTRIBUTE_ARCHIVE_ = 0x00000020;
static inline constexpr const DWORD_ FILE_ATTRIBUTE_NORMAL_ = 0x00000080;
static inline constexpr const DWORD_ FILE_ATTRIBUTE_SPARSE_FILE_ = 0x00000200;
static inline constexpr const DWORD_ FILE_ATTRIBUTE_REPARSE_POINT_ = 0x00000400;

static inline constexpr const DWORD_ CREATE_NEW_ = 1;
static inline constexpr const DWORD_ CREATE_ALWAYS_ = 2;
static inline constexpr const DWORD_ OPEN_EXISTING_ = 3;
static inline constexpr const DWORD_ OPEN_ALWAYS_ = 4;
static inline constexpr const DWORD_ TRUNCATE_EXISTING_ = 5;

static inline constexpr const DWORD_ INVALID_FILE_ATTRIBUTES_ =
    static_cast<DWORD_>(-1);

static inline constexpr const DWORD_ FILE_SHARE_READ_ = 0x00000001;
static inline constexpr const DWORD_ FILE_SHARE_WRITE_ = 0x00000002;
static inline constexpr const DWORD_ FILE_SHARE_DELETE_ = 0x00000004;
static inline constexpr const DWORD_ FILE_SHARE_VALID_FLAGS_ = 0x00000007;

static inline constexpr const DWORD_ FILE_BEGIN_ = 0;
static inline constexpr const DWORD_ FILE_CURRENT_ = 1;
static inline constexpr const DWORD_ FILE_END_ = 2;

static inline constexpr const DWORD_ FILE_MAP_READ_ = 0x00000004;

static inline constexpr const DWORD_ MOVEFILE_REPLACE_EXISTING_ = 0x00000001;
static inline constexpr const DWORD_ MOVEFILE_COPY_ALLOWED_ = 0x00000002;

#if NTDDI_VERSION >= NTDDI_VISTA
static inline constexpr const DWORD_ SYMBOLIC_LINK_FLAG_DIRECTORY_ = 0x1;
static inline constexpr const DWORD_
    SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE_ = 0x2;
#endif

static inline constexpr const DWORD_ STD_INPUT_HANDLE_ =
    static_cast<DWORD_>(-10);
static inline constexpr const DWORD_ STD_OUTPUT_HANDLE_ =
    static_cast<DWORD_>(-11);
static inline constexpr const DWORD_ STD_ERROR_HANDLE_ =
    static_cast<DWORD_>(-12);

static inline constexpr const size_t UNICODE_STRING_MAX_BYTES_ = 65534;
static inline constexpr const size_t UNICODE_STRING_MAX_CHARS_ = 32767;

static inline constexpr const UINT_ CP_UTF8_ = 65001u;

#if WINAPI_PARTITION_DESKTOP
static inline constexpr const NTSTATUS_ STATUS_SUCCESS_ = 0x00000000;
[[nodiscard]] static inline constexpr bool
NT_SUCCESS_(NTSTATUS_ Status) noexcept {
  return Status >= 0;
}
static inline constexpr const ULONG_ FILE_SEQUENTIAL_ONLY_ = 0x00000004;
static inline constexpr const ULONG_ FILE_RANDOM_ACCESS_ = 0x00000800;

static inline constexpr const DWORD_ ENABLE_VIRTUAL_TERMINAL_PROCESSING_ =
    0x0004;
#endif

} // namespace WasmEdge::winapi

extern "C" {

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
CancelIo(WasmEdge::winapi::HANDLE_ hFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
CloseHandle(WasmEdge::winapi::HANDLE_ hObject);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
CreateDirectoryW(WasmEdge::winapi::LPCWSTR_ lpPathName,
                 WasmEdge::winapi::LPSECURITY_ATTRIBUTES_ lpSecurityAttributes);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
DeleteFileW(WasmEdge::winapi::LPCWSTR_ lpFileName);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
DeviceIoControl(WasmEdge::winapi::HANDLE_ hDevice,
                WasmEdge::winapi::DWORD_ dwIoControlCode,
                WasmEdge::winapi::LPVOID_ lpInBuffer,
                WasmEdge::winapi::DWORD_ nInBufferSize,
                WasmEdge::winapi::LPVOID_ lpOutBuffer,
                WasmEdge::winapi::DWORD_ nOutBufferSize,
                WasmEdge::winapi::LPDWORD_ lpBytesReturned,
                WasmEdge::winapi::LPOVERLAPPED_ lpOverlapped);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
FindClose(WasmEdge::winapi::HANDLE_ hFindFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    FindFirstFileW(WasmEdge::winapi::LPCWSTR_ lpFileName,
                   WasmEdge::winapi::LPWIN32_FIND_DATAW_ lpFindFileData);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
FindNextFileW(WasmEdge::winapi::HANDLE_ hFindFile,
              WasmEdge::winapi::LPWIN32_FIND_DATAW_ lpFindFileData);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
FlushFileBuffers(WasmEdge::winapi::HANDLE_ hFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC
GetFileAttributesW(WasmEdge::winapi::LPCWSTR_ lpFileName);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetFileSizeEx(WasmEdge::winapi::HANDLE_ hFile,
              WasmEdge::winapi::LARGE_INTEGER_ *lpFileSize);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC
GetFileType(WasmEdge::winapi::HANDLE_ hFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::DWORD_
    WASMEDGE_WINAPI_WINAPI_CC GetLastError(WasmEdge::winapi::VOID_);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetNamedPipeInfo(WasmEdge::winapi::HANDLE_ hNamedPipe,
                 WasmEdge::winapi::LPDWORD_ lpFlags,
                 WasmEdge::winapi::LPDWORD_ lpOutBufferSize,
                 WasmEdge::winapi::LPDWORD_ lpInBufferSize,
                 WasmEdge::winapi::LPDWORD_ lpMaxInstances);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetOverlappedResult(WasmEdge::winapi::HANDLE_ hFile,
                    WasmEdge::winapi::LPOVERLAPPED_ lpOverlapped,
                    WasmEdge::winapi::LPDWORD_ lpNumberOfBytesTransferred,
                    WasmEdge::winapi::BOOL_ bWait);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    GetStdHandle(WasmEdge::winapi::DWORD_ nStdHandle);

WASMEDGE_WINAPI_SYMBOL_IMPORT
void WASMEDGE_WINAPI_WINAPI_CC
GetSystemTimeAsFileTime(WasmEdge::winapi::LPFILETIME_ lpSystemTimeAsFileTime);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
MoveFileExW(WasmEdge::winapi::LPCWSTR_ lpExistingFileName,
            WasmEdge::winapi::LPCWSTR_ lpNewFileName,
            WasmEdge::winapi::DWORD_ dwFlags);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
QueryPerformanceCounter(WasmEdge::winapi::LARGE_INTEGER_ *lpPerformanceCount);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
QueryPerformanceFrequency(WasmEdge::winapi::LARGE_INTEGER_ *lpFrequency);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC ReadFileEx(
    WasmEdge::winapi::HANDLE_ hFile, WasmEdge::winapi::LPVOID_ lpBuffer,
    WasmEdge::winapi::DWORD_ nNumberOfBytesToRead,
    WasmEdge::winapi::LPOVERLAPPED_ lpOverlapped,
    WasmEdge::winapi::LPOVERLAPPED_COMPLETION_ROUTINE_ lpCompletionRoutine);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
RemoveDirectoryW(WasmEdge::winapi::LPCWSTR_ lpPathName);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
SetEndOfFile(WasmEdge::winapi::HANDLE_ hFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
SetFilePointerEx(WasmEdge::winapi::HANDLE_ hFile,
                 WasmEdge::winapi::LARGE_INTEGER_ liDistanceToMove,
                 WasmEdge::winapi::LARGE_INTEGER_ *lpNewFilePointer,
                 WasmEdge::winapi::DWORD_ dwMoveMethod);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
SetFileTime(WasmEdge::winapi::HANDLE_ hFile,
            const WasmEdge::winapi::FILETIME_ *lpCreationTime,
            const WasmEdge::winapi::FILETIME_ *lpLastAccessTime,
            const WasmEdge::winapi::FILETIME_ *lpLastWriteTime);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_
    WASMEDGE_WINAPI_WINAPI_CC SwitchToThread(WasmEdge::winapi::VOID_);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
UnmapViewOfFile(WasmEdge::winapi::LPCVOID_ lpBaseAddress);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC
WaitForMultipleObjects(WasmEdge::winapi::DWORD_ nCount,
                       const WasmEdge::winapi::HANDLE_ *lpHandles,
                       bool bWaitAll, WasmEdge::winapi::DWORD_ dwMilliseconds);

WASMEDGE_WINAPI_SYMBOL_IMPORT
int WASMEDGE_WINAPI_WINAPI_CC WideCharToMultiByte(
    WasmEdge::winapi::UINT_ CodePage, WasmEdge::winapi::DWORD_ dwFlags,
    WasmEdge::winapi::LPCWSTR_ lpWideCharStr, int cchWideChar,
    WasmEdge::winapi::LPSTR_ lpMultiByteStr, int cbMultiByte,
    WasmEdge::winapi::LPCSTR_ lpDefaultChar,
    WasmEdge::winapi::LPBOOL_ lpUsedDefaultChar);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC WriteFileEx(
    WasmEdge::winapi::HANDLE_ hFile, WasmEdge::winapi::LPCVOID_ lpBuffer,
    WasmEdge::winapi::DWORD_ nNumberOfBytesToWrite,
    WasmEdge::winapi::LPOVERLAPPED_ lpOverlapped,
    WasmEdge::winapi::LPOVERLAPPED_COMPLETION_ROUTINE_ lpCompletionRoutine);

#if WINAPI_PARTITION_DESKTOP
WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    CreateFileMappingW(
        WasmEdge::winapi::HANDLE_ hFile,
        WasmEdge::winapi::LPSECURITY_ATTRIBUTES_ lpFileMappingAttributes,
        WasmEdge::winapi::DWORD_ flProtect,
        WasmEdge::winapi::DWORD_ dwMaximumSizeHigh,
        WasmEdge::winapi::DWORD_ dwMaximumSizeLow,
        WasmEdge::winapi::LPCWSTR_ lpName);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    CreateFileW(WasmEdge::winapi::LPCWSTR_ lpFileName,
                WasmEdge::winapi::DWORD_ dwDesiredAccess,
                WasmEdge::winapi::DWORD_ dwShareMode,
                WasmEdge::winapi::LPSECURITY_ATTRIBUTES_ lpSecurityAttributes,
                WasmEdge::winapi::DWORD_ dwCreationDisposition,
                WasmEdge::winapi::DWORD_ dwFlagsAndAttributes,
                WasmEdge::winapi::HANDLE_ hTemplateFile);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
CreateHardLinkW(WasmEdge::winapi::LPCWSTR_ lpFileName,
                WasmEdge::winapi::LPCWSTR_ lpExistingFileName,
                WasmEdge::winapi::LPSECURITY_ATTRIBUTES_ lpSecurityAttributes);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetConsoleMode(WasmEdge::winapi::HANDLE_ hConsoleHandle,
               WasmEdge::winapi::LPDWORD_ lpMode);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetFileInformationByHandle(
    WasmEdge::winapi::HANDLE_ hFile,
    WasmEdge::winapi::LPBY_HANDLE_FILE_INFORMATION_ lpFileInformation);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC GetLogicalDriveStringsW(
    WasmEdge::winapi::DWORD_ nBufferLength, WasmEdge::winapi::LPWSTR_ lpBuffer);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::LPVOID_ WASMEDGE_WINAPI_WINAPI_CC
MapViewOfFile(WasmEdge::winapi::HANDLE_ hFileMappingObject,
              WasmEdge::winapi::DWORD_ dwDesiredAccess,
              WasmEdge::winapi::DWORD_ dwFileOffsetHigh,
              WasmEdge::winapi::DWORD_ dwFileOffsetLow,
              WasmEdge::winapi::SIZE_T_ dwNumberOfBytesToMap);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::NTSTATUS_ WASMEDGE_WINAPI_WINAPI_CC NtQueryInformationFile(
    WasmEdge::winapi::HANDLE_ hFile, WasmEdge::winapi::PIO_STATUS_BLOCK_ io,
    WasmEdge::winapi::PVOID_ ptr, WasmEdge::winapi::ULONG_ len,
    WasmEdge::winapi::FILE_INFORMATION_CLASS_ FileInformationClass);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::NTSTATUS_
    WASMEDGE_WINAPI_WINAPI_CC
    NtQueryObject(
        WasmEdge::winapi::HANDLE_ Handle,
        WasmEdge::winapi::OBJECT_INFORMATION_CLASS_ ObjectInformationClass,
        WasmEdge::winapi::PVOID_ ObjectInformation,
        WasmEdge::winapi::ULONG_ ObjectInformationLength,
        WasmEdge::winapi::PULONG_ ReturnLength);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::NTSTATUS_ WASMEDGE_WINAPI_WINAPI_CC NtSetInformationFile(
    WasmEdge::winapi::HANDLE_ hFile, WasmEdge::winapi::PIO_STATUS_BLOCK_ io,
    WasmEdge::winapi::PVOID_ ptr, WasmEdge::winapi::ULONG_ len,
    WasmEdge::winapi::FILE_INFORMATION_CLASS_ FileInformationClass);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC QueryDosDeviceW(
    WasmEdge::winapi::LPCWSTR_ lpDeviceName,
    WasmEdge::winapi::LPWSTR_ lpTargetPath, WasmEdge::winapi::DWORD_ ucchMax);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC SetConsoleMode(
    WasmEdge::winapi::HANDLE_ hConsoleHandle, WasmEdge::winapi::DWORD_ dwMode);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
SetConsoleOutputCP(WasmEdge::winapi::UINT_ wCodePageID);
#endif

#if NTDDI_VERSION >= NTDDI_VISTA
WASMEDGE_WINAPI_SYMBOL_IMPORT bool WASMEDGE_WINAPI_WINAPI_CC
CommitTransaction(WasmEdge::winapi::HANDLE_ TransactionHandle);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOLEAN_
    WASMEDGE_WINAPI_WINAPI_CC
    CreateSymbolicLinkW(WasmEdge::winapi::LPCWSTR_ lpSymlinkFileName,
                        WasmEdge::winapi::LPCWSTR_ lpTargetFileName,
                        WasmEdge::winapi::DWORD_ dwFlags);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    CreateTransaction(
        WasmEdge::winapi::LPSECURITY_ATTRIBUTES_ lpTransactionAttributes,
        void *UOW, WasmEdge::winapi::DWORD_ CreateOptions,
        WasmEdge::winapi::DWORD_ IsolationLevel,
        WasmEdge::winapi::DWORD_ IsolationFlags,
        WasmEdge::winapi::DWORD_ Timeout,
        WasmEdge::winapi::LPWSTR_ Description);

WASMEDGE_WINAPI_SYMBOL_IMPORT bool WASMEDGE_WINAPI_WINAPI_CC
RemoveDirectoryTransactedW(WasmEdge::winapi::LPCWSTR_ lpPathName,
                           WasmEdge::winapi::HANDLE_ hTransaction);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::HANDLE_ WASMEDGE_WINAPI_WINAPI_CC
ReOpenFile(WasmEdge::winapi::HANDLE_ hOriginalFile,
           WasmEdge::winapi::DWORD_ dwDesiredAccess,
           WasmEdge::winapi::DWORD_ dwShareMode,
           WasmEdge::winapi::DWORD_ dwFlagsAndAttributes);
#endif

#if WINAPI_PARTITION_DESKTOP && NTDDI_VERSION >= NTDDI_VISTA
WASMEDGE_WINAPI_SYMBOL_IMPORT bool WASMEDGE_WINAPI_WINAPI_CC
MoveFileTransactedW(WasmEdge::winapi::LPCWSTR_ lpExistingFileName,
                    WasmEdge::winapi::LPCWSTR_ lpNewFileName,
                    WasmEdge::winapi::LPPROGRESS_ROUTINE_ lpProgressRoutine,
                    WasmEdge::winapi::LPVOID_ lpData,
                    WasmEdge::winapi::DWORD_ dwFlags,
                    WasmEdge::winapi::HANDLE_ hTransaction);
#endif

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_VISTA
WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetFileInformationByHandleEx(
    WasmEdge::winapi::HANDLE_ hFile,
    WasmEdge::winapi::FILE_INFO_BY_HANDLE_CLASS_ FileInformationClass,
    WasmEdge::winapi::LPVOID_ lpFileInformation,
    WasmEdge::winapi::DWORD_ dwBufferSize);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC
GetFinalPathNameByHandleW(WasmEdge::winapi::HANDLE_ hFile,
                          WasmEdge::winapi::LPWSTR_ lpszFilePath,
                          WasmEdge::winapi::DWORD_ cchFilePath,
                          WasmEdge::winapi::DWORD_ dwFlags);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
SetFileInformationByHandle(
    WasmEdge::winapi::HANDLE_ hFile,
    WasmEdge::winapi::FILE_INFO_BY_HANDLE_CLASS_ FileInformationClass,
    WasmEdge::winapi::LPVOID_ lpFileInformation,
    WasmEdge::winapi::DWORD_ dwBufferSize);
#endif

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_WIN8
WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    CreateFile2(
        WasmEdge::winapi::LPCWSTR_ lpFileName,
        WasmEdge::winapi::DWORD_ dwDesiredAccess,
        WasmEdge::winapi::DWORD_ dwShareMode,
        WasmEdge::winapi::DWORD_ dwCreationDisposition,
        WasmEdge::winapi::LPCREATEFILE2_EXTENDED_PARAMETERS_ pCreateExParams);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::HANDLE_ WASMEDGE_WINAPI_WINAPI_CC CreateFileMappingFromApp(
    WasmEdge::winapi::HANDLE_ hFile,
    WasmEdge::winapi::PSECURITY_ATTRIBUTES_ SecurityAttributes,
    WasmEdge::winapi::ULONG_ PageProtection,
    WasmEdge::winapi::ULONG64_ MaximumSize, WasmEdge::winapi::PCWSTR_ Name);

WASMEDGE_WINAPI_SYMBOL_IMPORT
void WASMEDGE_WINAPI_WINAPI_CC GetSystemTimePreciseAsFileTime(
    WasmEdge::winapi::LPFILETIME_ lpSystemTimeAsFileTime);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::PVOID_ WASMEDGE_WINAPI_WINAPI_CC
MapViewOfFileFromApp(WasmEdge::winapi::HANDLE_ hFileMappingObject,
                     WasmEdge::winapi::ULONG_ DesiredAccess,
                     WasmEdge::winapi::ULONG64_ FileOffset,
                     WasmEdge::winapi::SIZE_T_ NumberOfBytesToMap);
#endif

} // extern "C"

namespace WasmEdge::winapi {
using ::CancelIo;
using ::CloseHandle;
using ::CreateDirectoryW;
using ::DeleteFileW;
using ::DeviceIoControl;
using ::FindClose;
using ::FindFirstFileW;
using ::FindNextFileW;
using ::FlushFileBuffers;
using ::GetFileAttributesW;
using ::GetFileSizeEx;
using ::GetFileType;
using ::GetLastError;
using ::GetNamedPipeInfo;
using ::GetOverlappedResult;
using ::GetStdHandle;
using ::GetSystemTimeAsFileTime;
using ::MoveFileExW;
using ::QueryPerformanceCounter;
using ::QueryPerformanceFrequency;
using ::ReadFileEx;
using ::RemoveDirectoryW;
using ::SetEndOfFile;
using ::SetFilePointerEx;
using ::SetFileTime;
using ::SwitchToThread;
using ::UnmapViewOfFile;
using ::WaitForMultipleObjects;
using ::WideCharToMultiByte;
using ::WriteFileEx;

#if WINAPI_PARTITION_DESKTOP
using ::CreateFileMappingW;
using ::CreateFileW;
using ::CreateHardLinkW;
using ::GetConsoleMode;
using ::GetFileInformationByHandle;
using ::GetLogicalDriveStringsW;
using ::MapViewOfFile;
using ::NtQueryInformationFile;
using ::NtQueryObject;
using ::NtSetInformationFile;
using ::QueryDosDeviceW;
using ::SetConsoleMode;
using ::SetConsoleOutputCP;
#endif

#if NTDDI_VERSION >= NTDDI_VISTA
using ::CommitTransaction;
using ::CreateSymbolicLinkW;
using ::CreateTransaction;
using ::RemoveDirectoryTransactedW;
using ::ReOpenFile;
#endif

#if WINAPI_PARTITION_DESKTOP && NTDDI_VERSION >= NTDDI_VISTA
using ::MoveFileTransactedW;
#endif

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_VISTA
using ::GetFileInformationByHandleEx;
using ::GetFinalPathNameByHandleW;
using ::SetFileInformationByHandle;
#endif

#if !WINAPI_PARTITION_DESKTOP || NTDDI_VERSION >= NTDDI_WIN8
using ::CreateFile2;
using ::CreateFileMappingFromApp;
using ::GetSystemTimePreciseAsFileTime;
using ::MapViewOfFileFromApp;
#endif

} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {
using HLOCAL_ = HANDLE_;
using HMODULE_ = void *;

#ifdef _WIN64
using FARPROC_ = INT_PTR_(WASMEDGE_WINAPI_WINAPI_CC *)();
using NEARPROC_ = INT_PTR_(WASMEDGE_WINAPI_WINAPI_CC *)();
using PROC_ = INT_PTR_(WASMEDGE_WINAPI_WINAPI_CC *)();
#else
using FARPROC_ = int(WASMEDGE_WINAPI_WINAPI_CC *)();
using NEARPROC_ = int(WASMEDGE_WINAPI_WINAPI_CC *)();
using PROC_ = int(WASMEDGE_WINAPI_WINAPI_CC *)();
#endif

using RUNTIME_FUNCTION_ = struct _IMAGE_RUNTIME_FUNCTION_ENTRY {
  DWORD_ BeginAddress;
  DWORD_ EndAddress;
  union {
    DWORD_ UnwindInfoAddress;
    DWORD_ UnwindData;
  } DUMMYUNIONNAME;
};
using PRUNTIME_FUNCTION_ = RUNTIME_FUNCTION_ *;

static inline constexpr const DWORD_ FORMAT_MESSAGE_ALLOCATE_BUFFER_ =
    0x00000100;
static inline constexpr const DWORD_ FORMAT_MESSAGE_IGNORE_INSERTS_ =
    0x00000200;
static inline constexpr const DWORD_ FORMAT_MESSAGE_FROM_SYSTEM_ = 0x00001000;
static inline constexpr const WORD_ LANG_NEUTRAL_ = 0x00;
static inline constexpr const WORD_ SUBLANG_DEFAULT_ = 0x01;

WASMEDGE_WINAPI_FORCEINLINE inline constexpr WORD_
MAKELANGID_(WORD_ p, WORD_ s) noexcept {
  return static_cast<WORD_>((static_cast<WORD_>(s) << 10) |
                            static_cast<WORD_>(p));
}

} // namespace WasmEdge::winapi

extern "C" {

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC
FormatMessageA(WasmEdge::winapi::DWORD_ dwFlags,
               WasmEdge::winapi::LPCVOID_ lpSource,
               WasmEdge::winapi::DWORD_ dwMessageId,
               WasmEdge::winapi::DWORD_ dwLanguageId,
               WasmEdge::winapi::LPSTR_ lpBuffer,
               WasmEdge::winapi::DWORD_ nSize, va_list *Arguments);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
FreeLibrary(WasmEdge::winapi::HMODULE_ hModule);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::HMODULE_ WASMEDGE_WINAPI_WINAPI_CC
GetModuleHandleW(WasmEdge::winapi::LPCWSTR_ lpModuleName);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::FARPROC_
    WASMEDGE_WINAPI_WINAPI_CC
    GetProcAddress(WasmEdge::winapi::HMODULE_ hModule,
                   WasmEdge::winapi::LPCSTR_ lpProcName);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HMODULE_
    WASMEDGE_WINAPI_WINAPI_CC
    LoadLibraryExW(WasmEdge::winapi::LPCWSTR_ lpFileName,
                   WasmEdge::winapi::HANDLE_ hFile,
                   WasmEdge::winapi::DWORD_ dwFlags);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOLEAN_
    WASMEDGE_WINAPI_WINAPI_CC
    RtlAddFunctionTable(WasmEdge::winapi::PRUNTIME_FUNCTION_ FunctionTable,
                        WasmEdge::winapi::ULONG_ EntryCount,
                        WasmEdge::winapi::ULONG_PTR_ BaseAddress);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOLEAN_
    WASMEDGE_WINAPI_WINAPI_CC
    RtlDeleteFunctionTable(WasmEdge::winapi::PRUNTIME_FUNCTION_ FunctionTable);

} // extern "C"

namespace WasmEdge::winapi {
using ::FormatMessageA;
using ::FreeLibrary;
using ::GetModuleHandleW;
using ::GetProcAddress;
using ::LoadLibraryExW;
using ::RtlAddFunctionTable;
using ::RtlDeleteFunctionTable;
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {
using HWND_ = void *;
using HRESULT_ = LONG_;
using GUID_ = struct _GUID {
  ULONG_ Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
};
using KNOWNFOLDERID_ = GUID_;
using REFKNOWNFOLDERID_ = const KNOWNFOLDERID_ &;

static inline constexpr const int CSIDL_PROFILE_ = 0x0028;
static inline constexpr const int CSIDL_LOCAL_APPDATA_ = 0x001c;
static inline constexpr const int CSIDL_FLAG_CREATE_ = 0x8000;

static inline constexpr const int KF_FLAG_CREATE_ = 0x00008000;

WASMEDGE_WINAPI_FORCEINLINE inline constexpr bool
SUCCEEDED_(HRESULT_ Stat) noexcept {
  return Stat >= 0;
}

} // namespace WasmEdge::winapi

extern "C" {

extern const WasmEdge::winapi::GUID_ FOLDERID_Profile;
extern const WasmEdge::winapi::GUID_ FOLDERID_LocalAppData;

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HRESULT_
    WASMEDGE_WINAPI_WINAPI_CC
    SHGetFolderPathW(WasmEdge::winapi::HWND_ hwnd, int csidl,
                     WasmEdge::winapi::HANDLE_ hToken,
                     WasmEdge::winapi::DWORD_ dwFlags,
                     WasmEdge::winapi::LPWSTR_ pszPath);

#if NTDDI_VERSION >= NTDDI_VISTA
WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HRESULT_
    WASMEDGE_WINAPI_WINAPI_CC
    SHGetKnownFolderPath(WasmEdge::winapi::REFKNOWNFOLDERID_ rfid,
                         WasmEdge::winapi::DWORD_ dwFlags,
                         WasmEdge::winapi::HANDLE_ hToken,
                         WasmEdge::winapi::PWSTR_ *ppszPath);
#endif

} // extern "C"

namespace WasmEdge::winapi {
using ::FOLDERID_LocalAppData;
using ::FOLDERID_Profile;
using ::SHGetFolderPathW;

#if NTDDI_VERSION >= NTDDI_VISTA
using ::SHGetKnownFolderPath;
#endif
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {
static inline constexpr const DWORD_ MEM_COMMIT_ = 0x00001000;
static inline constexpr const DWORD_ MEM_RESERVE_ = 0x00002000;
static inline constexpr const DWORD_ MEM_RELEASE_ = 0x00008000;

static inline constexpr const DWORD_ PAGE_NOACCESS_ = 0x01;
static inline constexpr const DWORD_ PAGE_READONLY_ = 0x02;
static inline constexpr const DWORD_ PAGE_READWRITE_ = 0x04;
static inline constexpr const DWORD_ PAGE_EXECUTE_READ_ = 0x20;
} // namespace WasmEdge::winapi

extern "C" {

WASMEDGE_WINAPI_SYMBOL_IMPORT void WASMEDGE_WINAPI_WINAPI_CC
CoTaskMemFree(WasmEdge::winapi::LPVOID_ pv);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HLOCAL_
    WASMEDGE_WINAPI_WINAPI_CC
    LocalFree(WasmEdge::winapi::HLOCAL_ hMem);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::LPVOID_
    WASMEDGE_WINAPI_WINAPI_CC
    VirtualAlloc(WasmEdge::winapi::LPVOID_ lpAddress,
                 WasmEdge::winapi::SIZE_T_ dwSize,
                 WasmEdge::winapi::DWORD_ flAllocationType,
                 WasmEdge::winapi::DWORD_ flProtect);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
VirtualFree(WasmEdge::winapi::LPVOID_ lpAddress,
            WasmEdge::winapi::SIZE_T_ dwSize,
            WasmEdge::winapi::DWORD_ dwFreeType);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
VirtualProtect(WasmEdge::winapi::LPVOID_ lpAddress,
               WasmEdge::winapi::SIZE_T_ dwSize,
               WasmEdge::winapi::DWORD_ flNewProtect,
               WasmEdge::winapi::PDWORD_ lpflOldProtect);

} // extern "C"

namespace WasmEdge::winapi {
using ::CoTaskMemFree;
using ::LocalFree;
using ::VirtualAlloc;
using ::VirtualFree;
using ::VirtualProtect;
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {

static inline constexpr const DWORD_ EXCEPTION_MAXIMUM_PARAMETERS_ = 15;
static inline constexpr const DWORD_ EXCEPTION_ACCESS_VIOLATION_ = 0xC0000005L;
static inline constexpr const DWORD_ EXCEPTION_INT_DIVIDE_BY_ZERO_ =
    0xC0000094L;
static inline constexpr const DWORD_ EXCEPTION_INT_OVERFLOW_ = 0xC0000095L;
static inline constexpr const LONG_ EXCEPTION_CONTINUE_EXECUTION_ =
    static_cast<LONG_>(0xffffffff);

using CONTEXT_ = struct _CONTEXT;
using PCONTEXT_ = CONTEXT_ *;

using EXCEPTION_RECORD_ = struct _EXCEPTION_RECORD {
  DWORD_ ExceptionCode;
  DWORD_ ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID_ ExceptionAddress;
  DWORD_ NumberParameters;
  PULONG_ ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS_];
};
using PEXCEPTION_RECORD_ = EXCEPTION_RECORD_ *;

using EXCEPTION_POINTERS_ = struct _EXCEPTION_POINTERS {
  PEXCEPTION_RECORD_ ExceptionRecord;
  PCONTEXT_ ContextRecord;
};
using PEXCEPTION_POINTERS_ = EXCEPTION_POINTERS_ *;

using PVECTORED_EXCEPTION_HANDLER_ =
    LONG_(WASMEDGE_WINAPI_WINAPI_CC *)(PEXCEPTION_POINTERS_ ExceptionInfo);

} // namespace WasmEdge::winapi

extern "C" {
WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::PVOID_ WASMEDGE_WINAPI_WINAPI_CC
AddVectoredExceptionHandler(
    WasmEdge::winapi::ULONG_ First,
    WasmEdge::winapi::PVECTORED_EXCEPTION_HANDLER_ Handler);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::ULONG_ WASMEDGE_WINAPI_WINAPI_CC
RemoveVectoredExceptionHandler(WasmEdge::winapi::PVOID_ Handle);
} // extern "C"

namespace WasmEdge::winapi {
using ::AddVectoredExceptionHandler;
using ::RemoveVectoredExceptionHandler;
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {

static inline constexpr const DWORD_ SE_PRIVILEGE_ENABLED_ = 0x2;

using LUID_ = struct _LUID {
  ULONG_ LowPart;
  LONG_ HighPart;
};
using PLUID_ = LUID_ *;

using LUID_AND_ATTRIBUTES_ = struct _LUID_AND_ATTRIBUTES {
  LUID_ Luid;
  DWORD_ Attributes;
};

using TOKEN_PRIVILEGES_ = struct _TOKEN_PRIVILEGES {
  DWORD_ PrivilegeCount;
  LUID_AND_ATTRIBUTES_ Privileges[1];
};

using PTOKEN_PRIVILEGES_ = TOKEN_PRIVILEGES_ *;

} // namespace WasmEdge::winapi

extern "C" {
WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
AdjustTokenPrivileges(WasmEdge::winapi::HANDLE_ TokenHandle,
                      WasmEdge::winapi::BOOL_ DisableAllPrivileges,
                      WasmEdge::winapi::PTOKEN_PRIVILEGES_ NewState,
                      WasmEdge::winapi::DWORD_ BufferLength,
                      WasmEdge::winapi::PTOKEN_PRIVILEGES_ PreviousState,
                      WasmEdge::winapi::PDWORD_ ReturnLength);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    GetCurrentProcess(void);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::HANDLE_
    WASMEDGE_WINAPI_WINAPI_CC
    GetCurrentThread(void);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetProcessTimes(WasmEdge::winapi::HANDLE_ hProcess,
                WasmEdge::winapi::LPFILETIME_ lpCreationTime,
                WasmEdge::winapi::LPFILETIME_ lpExitTime,
                WasmEdge::winapi::LPFILETIME_ lpKernelTime,
                WasmEdge::winapi::LPFILETIME_ lpUserTime);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
GetThreadTimes(WasmEdge::winapi::HANDLE_ hThread,
               WasmEdge::winapi::LPFILETIME_ lpCreationTime,
               WasmEdge::winapi::LPFILETIME_ lpExitTime,
               WasmEdge::winapi::LPFILETIME_ lpKernelTime,
               WasmEdge::winapi::LPFILETIME_ lpUserTime);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC LookupPrivilegeValueW(
    WasmEdge::winapi::LPCWSTR_ lpSystemName, WasmEdge::winapi::LPCWSTR_ lpName,
    WasmEdge::winapi::PLUID_ lpLuid);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::NTSTATUS_ WASMEDGE_WINAPI_WINAPI_CC
NtQueryTimerResolution(WasmEdge::winapi::PULONG_ MinimumResolution,
                       WasmEdge::winapi::PULONG_ MaximumResolution,
                       WasmEdge::winapi::PULONG_ CurrentResolution);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC
OpenProcessToken(WasmEdge::winapi::HANDLE_ ProcessHandle,
                 WasmEdge::winapi::DWORD_ DesiredAccess,
                 WasmEdge::winapi::PHANDLE_ TokenHandle);

} // extern "C"

namespace WasmEdge::winapi {
using ::AdjustTokenPrivileges;
using ::GetCurrentProcess;
using ::GetCurrentThread;
using ::GetProcessTimes;
using ::GetThreadTimes;
using ::LookupPrivilegeValueW;
using ::NtQueryTimerResolution;
using ::OpenProcessToken;
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {
using u_char = unsigned char;
using u_short = unsigned short;
using u_int = unsigned int;
using u_long = unsigned long;
using u_int64 = unsigned long long;
using socklen_t = int;

using SOCKET_ = UINT_PTR_;
using ADDRESS_FAMILY_ = u_short;
static inline constexpr const SOCKET_ INVALID_SOCKET_ =
    static_cast<SOCKET_>(~0);
static inline constexpr const int SOCKET_ERROR_ = -1;

static inline constexpr const size_t WSADESCRIPTION_LEN_ = 256;
static inline constexpr const size_t WSASYS_STATUS_LEN_ = 128;
using WSADATA_ = struct WSAData {
  WORD_ wVersion;
  WORD_ wHighVersion;
#ifdef _WIN64
  u_short iMaxSockets;
  u_short iMaxUdpDg;
  char *lpVendorInfo;
  char szDescription[WSADESCRIPTION_LEN_ + 1];
  char szSystemStatus[WSASYS_STATUS_LEN_ + 1];
#else
  char szDescription[WSADESCRIPTION_LEN_ + 1];
  char szSystemStatus[WSASYS_STATUS_LEN_ + 1];
  u_short iMaxSockets;
  u_short iMaxUdpDg;
  char *lpVendorInfo;
#endif
};
using LPWSADATA_ = WSADATA_ *;

using IN_ADDR_ = struct in_addr {
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      u_char s_b1;
      u_char s_b2;
      u_char s_b3;
      u_char s_b4;
    } S_un_b;
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      u_short s_w1;
      u_short s_w2;
    } S_un_w;
    u_long S_addr;
  } S_un;
};
#define s_addr S_un.S_addr
#define s_host S_un.S_un_b.s_b2
#define s_net S_un.S_un_b.s_b1
#define s_imp S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh S_un.S_un_b.s_b3

using IN6_ADDR_ = struct in6_addr {
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    u_char Byte[16];
    u_short Word[8];
  } u;
};
#define _S6_un u
#define _S6_u8 Byte
#define s6_addr _S6_un._S6_u8
#define s6_bytes u.Byte
#define s6_words u.Word

using SOCKADDR_ = struct sockaddr {
  ADDRESS_FAMILY_ sa_family;
  CHAR_ sa_data[14];
};

using SOCKADDR_IN_ = struct sockaddr_in {
  ADDRESS_FAMILY_ sin_family;
  u_short sin_port;
  IN_ADDR_ sin_addr;
  CHAR_ sin_zero[8];
};

using SCOPE_ID_ = struct _SCOPE_ID {
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    WASMEDGE_WINAPI_DETAIL_EXTENSION struct {
      u_long Zone : 28;
      u_long Level : 4;
    };
    u_long Value;
  };
};

using SOCKADDR_IN6_LH_ = struct sockaddr_in6 {
  ADDRESS_FAMILY_ sin6_family;
  u_short sin6_port;
  u_long sin6_flowinfo;
  IN6_ADDR_ sin6_addr;
  WASMEDGE_WINAPI_DETAIL_EXTENSION union {
    u_long sin6_scope_id;
    SCOPE_ID_ sin6_scope_struct;
  };
};

static inline constexpr const size_t _SS_MAXSIZE = 128;
static inline constexpr const size_t _SS_ALIGNSIZE = 8;
static inline constexpr const size_t _SS_PAD1SIZE =
    _SS_ALIGNSIZE - sizeof(ADDRESS_FAMILY_);
static inline constexpr const size_t _SS_PAD2SIZE =
    _SS_MAXSIZE - sizeof(ADDRESS_FAMILY_) - _SS_PAD1SIZE - _SS_ALIGNSIZE;
using SOCKADDR_STORAGE_LH = struct sockaddr_storage {
  ADDRESS_FAMILY_ ss_family;
  CHAR_ __ss_pad1[_SS_PAD1SIZE];
  LONGLONG_ __ss_align;
  CHAR_ __ss_pad2[_SS_PAD2SIZE];
};

using ADDRINFOA_ = struct addrinfo {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  char *ai_canonname;
  struct sockaddr *ai_addr;
  struct addrinfo *ai_next;
};
using PADDRINFOA_ = ADDRINFOA_ *;

static inline constexpr const size_t FD_SETSIZE_ = 64;
using FD_SET_ = struct fd_set {
  u_int fd_count;
  SOCKET_ fd_array[FD_SETSIZE_];
};

using TIMEVAL_ = struct timeval {
  long tv_sec;
  long tv_usec;
};

using LINGER = struct linger {
  u_short l_onoff;
  u_short l_linger;
};

static inline constexpr const int AI_PASSIVE = 0x00000001;
static inline constexpr const int AI_CANONNAME = 0x00000002;
static inline constexpr const int AI_NUMERICHOST = 0x00000004;
#if NTDDI_VERSION >= NTDDI_VISTA
static inline constexpr const int AI_NUMERICSERV = 0x00000008;
static inline constexpr const int AI_ALL = 0x00000100;
static inline constexpr const int AI_ADDRCONFIG = 0x00000400;
static inline constexpr const int AI_V4MAPPED = 0x00000800;
#endif

static inline constexpr const long IOCPARM_MASK = 0x7f;
static inline constexpr const long IOC_IN = static_cast<long>(0x80000000);
static inline constexpr long _IOW(long X, long Y) noexcept {
  return IOC_IN | ((static_cast<long>(sizeof(u_long)) & IOCPARM_MASK) << 16) |
         (X << 8) | Y;
}
static inline constexpr const long FIONREAD = _IOW('f', 127);
static inline constexpr const long FIONBIO = _IOW('f', 126);

static inline constexpr const int IPPROTO_IP = 0;
static inline constexpr const int IPPROTO_TCP = 6;
static inline constexpr const int IPPROTO_UDP = 17;

static inline constexpr const u_long INADDR_ANY = 0x00000000;
static inline constexpr const u_long INADDR_LOOPBACK = 0x7f000001;

static inline constexpr const int SOCK_STREAM = 1;
static inline constexpr const int SOCK_DGRAM = 2;

static inline constexpr const int SO_ACCEPTCONN = 0x0002;
static inline constexpr const int SO_REUSEADDR = 0x0004;
static inline constexpr const int SO_KEEPALIVE = 0x0008;
static inline constexpr const int SO_DONTROUTE = 0x0010;
static inline constexpr const int SO_BROADCAST = 0x0020;
static inline constexpr const int SO_LINGER = 0x0080;
static inline constexpr const int SO_OOBINLINE = 0x0100;

static inline constexpr const int SO_SNDBUF = 0x1001;
static inline constexpr const int SO_RCVBUF = 0x1002;
static inline constexpr const int SO_RCVLOWAT = 0x1004;
static inline constexpr const int SO_SNDTIMEO = 0x1005;
static inline constexpr const int SO_RCVTIMEO = 0x1006;
static inline constexpr const int SO_ERROR = 0x1007;
static inline constexpr const int SO_TYPE = 0x1008;

static inline constexpr const int AF_UNSPEC = 0;
static inline constexpr const int AF_INET = 2;
static inline constexpr const int AF_INET6 = 23;

static inline constexpr const int SOL_SOCKET = 0xffff;

static inline constexpr const int MSG_PEEK = 0x2;
#if NTDDI_VERSION >= NTDDI_WS03
static inline constexpr const int MSG_WAITALL = 0x8;
#endif

static inline constexpr const int SD_RECEIVE = 0x0;
static inline constexpr const int SD_SEND = 0x1;
static inline constexpr const int SD_BOTH = 0x2;

static inline constexpr const DWORD_ WSABASEERR_ = 10000;
static inline constexpr const DWORD_ WSAEINTR_ = WSABASEERR_ + 4;
static inline constexpr const DWORD_ WSAEFAULT_ = WSABASEERR_ + 14;
static inline constexpr const DWORD_ WSAEINVAL_ = WSABASEERR_ + 22;
static inline constexpr const DWORD_ WSAEMFILE_ = WSABASEERR_ + 24;
static inline constexpr const DWORD_ WSAEWOULDBLOCK_ = WSABASEERR_ + 35;
static inline constexpr const DWORD_ WSAEINPROGRESS_ = WSABASEERR_ + 36;
static inline constexpr const DWORD_ WSAENOTSOCK_ = WSABASEERR_ + 38;
static inline constexpr const DWORD_ WSAEPROTOTYPE_ = WSABASEERR_ + 41;
static inline constexpr const DWORD_ WSAEPROTONOSUPPORT_ = WSABASEERR_ + 43;
static inline constexpr const DWORD_ WSAESOCKTNOSUPPORT_ = WSABASEERR_ + 44;
static inline constexpr const DWORD_ WSAEAFNOSUPPORT_ = WSABASEERR_ + 47;
static inline constexpr const DWORD_ WSAENETDOWN_ = WSABASEERR_ + 50;
static inline constexpr const DWORD_ WSAENOBUFS_ = WSABASEERR_ + 55;
static inline constexpr const DWORD_ WSAEPROCLIM_ = WSABASEERR_ + 67;
static inline constexpr const DWORD_ WSASYSNOTREADY_ = WSABASEERR_ + 91;
static inline constexpr const DWORD_ WSAVERNOTSUPPORTED_ = WSABASEERR_ + 92;
static inline constexpr const DWORD_ WSANOTINITIALISED_ = WSABASEERR_ + 93;
static inline constexpr const DWORD_ WSAEINVALIDPROCTABLE_ = WSABASEERR_ + 104;
static inline constexpr const DWORD_ WSAEINVALIDPROVIDER_ = WSABASEERR_ + 105;
static inline constexpr const DWORD_ WSAEPROVIDERFAILEDINIT_ =
    WSABASEERR_ + 106;
static inline constexpr const DWORD_ WSATYPE_NOT_FOUND_ = WSABASEERR_ + 109;
static inline constexpr const DWORD_ WSAHOST_NOT_FOUND_ = WSABASEERR_ + 1001;
static inline constexpr const DWORD_ WSATRY_AGAIN_ = WSABASEERR_ + 1002;
static inline constexpr const DWORD_ WSANO_RECOVERY_ = WSABASEERR_ + 1003;
} // namespace WasmEdge::winapi

extern "C" {

extern const WasmEdge::winapi::IN6_ADDR_ in6addr_loopback;

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::SOCKET_
    WASMEDGE_WINAPI_WINAPI_CC
    accept(WasmEdge::winapi::SOCKET_ s, struct WasmEdge::winapi::sockaddr *addr,
           WasmEdge::winapi::socklen_t *addrlen);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
bind(WasmEdge::winapi::SOCKET_ s, const struct WasmEdge::winapi::sockaddr *addr,
     WasmEdge::winapi::socklen_t namelen);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
closesocket(WasmEdge::winapi::SOCKET_ s);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC connect(
    WasmEdge::winapi::SOCKET_ s, const struct WasmEdge::winapi::sockaddr *name,
    WasmEdge::winapi::socklen_t namelen);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::VOID_ WASMEDGE_WINAPI_WINAPI_CC
freeaddrinfo(WasmEdge::winapi::PADDRINFOA_ pAddrInfo);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC getaddrinfo(
    WasmEdge::winapi::PCSTR_ pNodeName, WasmEdge::winapi::PCSTR_ pServiceName,
    const WasmEdge::winapi::ADDRINFOA_ *pHints,
    WasmEdge::winapi::PADDRINFOA_ *ppResult);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC getpeername(
    WasmEdge::winapi::SOCKET_ s, struct WasmEdge::winapi::sockaddr *name,
    WasmEdge::winapi::socklen_t *namelen);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC getsockname(
    WasmEdge::winapi::SOCKET_ s, struct WasmEdge::winapi::sockaddr *name,
    WasmEdge::winapi::socklen_t *namelen);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
getsockopt(WasmEdge::winapi::SOCKET_ s, int level, int optname, char *optval,
           WasmEdge::winapi::socklen_t *optlen);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::u_long WASMEDGE_WINAPI_WINAPI_CC
htonl(WasmEdge::winapi::u_long hostlong);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::u_short
    WASMEDGE_WINAPI_WINAPI_CC
    htons(WasmEdge::winapi::u_short hostshort);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC ioctlsocket(
    WasmEdge::winapi::SOCKET_ s, long cmd, WasmEdge::winapi::u_long *argp);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
listen(WasmEdge::winapi::SOCKET_ s, int backlog);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::u_short
    WASMEDGE_WINAPI_WINAPI_CC
    ntohs(WasmEdge::winapi::u_short netshort);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
recv(WasmEdge::winapi::SOCKET_ s, char *buf, int len, int flags);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
recvfrom(WasmEdge::winapi::SOCKET_ s, char *buf, int len, int flags,
         struct WasmEdge::winapi::sockaddr *from,
         WasmEdge::winapi::socklen_t *from_len);

WASMEDGE_WINAPI_SYMBOL_IMPORT
int WASMEDGE_WINAPI_WINAPI_CC select(int nfds,
                                     WasmEdge::winapi::FD_SET_ *readfds,
                                     WasmEdge::winapi::FD_SET_ *writefds,
                                     WasmEdge::winapi::FD_SET_ *exceptfds,
                                     const WasmEdge::winapi::TIMEVAL_ *timeout);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
send(WasmEdge::winapi::SOCKET_ s, const char *buf, int len, int flags);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
sendto(WasmEdge::winapi::SOCKET_ s, const char *buf, int len, int flags,
       const struct WasmEdge::winapi::sockaddr *to,
       WasmEdge::winapi::socklen_t to_len);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
setsockopt(WasmEdge::winapi::SOCKET_ s, int level, int optname,
           const char *optval, WasmEdge::winapi::socklen_t optlen);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
shutdown(WasmEdge::winapi::SOCKET_ s, int how);

WASMEDGE_WINAPI_SYMBOL_IMPORT WasmEdge::winapi::SOCKET_
    WASMEDGE_WINAPI_WINAPI_CC
    socket(int af, int type, int protocol);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC WSACleanup(void);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
WSAGetLastError(void);

WASMEDGE_WINAPI_SYMBOL_IMPORT int WASMEDGE_WINAPI_WINAPI_CC
WSAStartup(WasmEdge::winapi::WORD_ wVersionRequested,
           WasmEdge::winapi::LPWSADATA_ lpWSAData);

} // extern "C"

namespace WasmEdge::winapi {
using ::accept;
using ::bind;
using ::closesocket;
using ::connect;
using ::freeaddrinfo;
using ::getaddrinfo;
using ::getpeername;
using ::getsockname;
using ::getsockopt;
using ::htonl;
using ::htons;
using ::ioctlsocket;
using ::listen;
using ::ntohs;
using ::recv;
using ::recvfrom;
using ::select;
using ::send;
using ::sendto;
using ::setsockopt;
using ::shutdown;
using ::socket;
using ::WSACleanup;
using ::WSAGetLastError;
using ::WSAStartup;
} // namespace WasmEdge::winapi

#if WINAPI_PARTITION_DESKTOP
namespace WasmEdge::winapi {
inline ULONG_ RtlNtStatusToDosError(NTSTATUS_ Status) noexcept {
  static const auto Func =
      reinterpret_cast<ULONG_ WASMEDGE_WINAPI_WINAPI_CC (*)(NTSTATUS_)>(
          reinterpret_cast<void *>(GetProcAddress(
              GetModuleHandleW(L"Ntdll.dll"), "RtlNtStatusToDosError")));
  return Func(Status);
}
} // namespace WasmEdge::winapi
#endif

extern "C" {
WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::BOOL_ WASMEDGE_WINAPI_WINAPI_CC GetModuleHandleExW(
    WasmEdge::winapi::DWORD_ dwFlags, WasmEdge::winapi::LPCWSTR_ lpModuleName,
    WasmEdge::winapi::HMODULE_ *phModule);

WASMEDGE_WINAPI_SYMBOL_IMPORT
WasmEdge::winapi::DWORD_ WASMEDGE_WINAPI_WINAPI_CC GetModuleFileNameW(
    WasmEdge::winapi::HMODULE_ hModule, WasmEdge::winapi::LPWSTR_ lpFilename,
    WasmEdge::winapi::DWORD_ nSize);
} // extern "C"

namespace WasmEdge::winapi {
using ::GetModuleFileNameW;
using ::GetModuleHandleExW;
} // namespace WasmEdge::winapi

namespace WasmEdge::winapi {
static inline constexpr const DWORD_ GET_MODULE_HANDLE_EX_FLAG_PIN_ =
    0x00000001L;
static inline constexpr const DWORD_
    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT_ = 0x00000002L;
static inline constexpr const DWORD_ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS_ =
    0x00000004L;
} // namespace WasmEdge::winapi

#endif
