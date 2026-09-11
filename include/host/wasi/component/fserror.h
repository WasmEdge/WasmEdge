// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/fserror.h - Filesystem error codes ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the filesystem error codes shared by the 0.2 and 0.3
/// hosts, in the order of the 0.2 `error-code` enum, and their mapping from
/// the preview-1 errno.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "wasi/api.hpp"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// `wasi:filesystem/types@0.2` `error-code`; 0.3 drops `would-block` and
/// adds `other(option<string>)` at the end.
enum class FsErrorCode : uint32_t {
  Access,
  WouldBlock,
  Already,
  BadDescriptor,
  Busy,
  Deadlock,
  Quota,
  Exist,
  FileTooLarge,
  IllegalByteSequence,
  InProgress,
  Interrupted,
  Invalid,
  Io,
  IsDirectory,
  Loop,
  TooManyLinks,
  MessageSize,
  NameTooLong,
  NoDevice,
  NoEntry,
  NoLock,
  InsufficientMemory,
  InsufficientSpace,
  NotDirectory,
  NotEmpty,
  NotRecoverable,
  Unsupported,
  NoTty,
  NoSuchDevice,
  Overflow,
  NotPermitted,
  Pipe,
  ReadOnly,
  InvalidSeek,
  TextFileBusy,
  CrossDevice,
};

inline FsErrorCode fsErrorOf(__wasi_errno_t Errno) noexcept {
  switch (Errno) {
  case __WASI_ERRNO_ACCES:
    return FsErrorCode::Access;
  case __WASI_ERRNO_AGAIN:
    return FsErrorCode::WouldBlock;
  case __WASI_ERRNO_ALREADY:
    return FsErrorCode::Already;
  case __WASI_ERRNO_BADF:
    return FsErrorCode::BadDescriptor;
  case __WASI_ERRNO_BUSY:
    return FsErrorCode::Busy;
  case __WASI_ERRNO_DEADLK:
    return FsErrorCode::Deadlock;
  case __WASI_ERRNO_DQUOT:
    return FsErrorCode::Quota;
  case __WASI_ERRNO_EXIST:
    return FsErrorCode::Exist;
  case __WASI_ERRNO_FBIG:
    return FsErrorCode::FileTooLarge;
  case __WASI_ERRNO_ILSEQ:
    return FsErrorCode::IllegalByteSequence;
  case __WASI_ERRNO_INPROGRESS:
    return FsErrorCode::InProgress;
  case __WASI_ERRNO_INTR:
    return FsErrorCode::Interrupted;
  case __WASI_ERRNO_INVAL:
    return FsErrorCode::Invalid;
  case __WASI_ERRNO_IO:
    return FsErrorCode::Io;
  case __WASI_ERRNO_ISDIR:
    return FsErrorCode::IsDirectory;
  case __WASI_ERRNO_LOOP:
    return FsErrorCode::Loop;
  case __WASI_ERRNO_MLINK:
    return FsErrorCode::TooManyLinks;
  case __WASI_ERRNO_MSGSIZE:
    return FsErrorCode::MessageSize;
  case __WASI_ERRNO_NAMETOOLONG:
    return FsErrorCode::NameTooLong;
  case __WASI_ERRNO_NODEV:
    return FsErrorCode::NoDevice;
  case __WASI_ERRNO_NOENT:
    return FsErrorCode::NoEntry;
  case __WASI_ERRNO_NOLCK:
    return FsErrorCode::NoLock;
  case __WASI_ERRNO_NOMEM:
    return FsErrorCode::InsufficientMemory;
  case __WASI_ERRNO_NOSPC:
    return FsErrorCode::InsufficientSpace;
  case __WASI_ERRNO_NOTDIR:
    return FsErrorCode::NotDirectory;
  case __WASI_ERRNO_NOTEMPTY:
    return FsErrorCode::NotEmpty;
  case __WASI_ERRNO_NOTRECOVERABLE:
    return FsErrorCode::NotRecoverable;
  case __WASI_ERRNO_NOTSUP:
    return FsErrorCode::Unsupported;
  case __WASI_ERRNO_NOTTY:
    return FsErrorCode::NoTty;
  case __WASI_ERRNO_NXIO:
    return FsErrorCode::NoSuchDevice;
  case __WASI_ERRNO_OVERFLOW:
    return FsErrorCode::Overflow;
  case __WASI_ERRNO_PERM:
  case __WASI_ERRNO_NOTCAPABLE:
    return FsErrorCode::NotPermitted;
  case __WASI_ERRNO_PIPE:
    return FsErrorCode::Pipe;
  case __WASI_ERRNO_ROFS:
    return FsErrorCode::ReadOnly;
  case __WASI_ERRNO_SPIPE:
    return FsErrorCode::InvalidSeek;
  case __WASI_ERRNO_TXTBSY:
    return FsErrorCode::TextFileBusy;
  case __WASI_ERRNO_XDEV:
    return FsErrorCode::CrossDevice;
  default:
    return FsErrorCode::Io;
  }
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
