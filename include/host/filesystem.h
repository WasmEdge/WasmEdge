// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/filesystem.h - Filesystem of a guest ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file defines the FileSystem and File interfaces, through which the host
/// functions open the files of a guest with the rights of the guest.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "host/wasi/error.h"
#include "wasi/api.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace WasmEdge {
namespace Host {

/// A file opened by a FileSystem, with the rights granted at the open.
class File {
public:
  virtual ~File() noexcept = default;

  /// Read until the buffer is full or the file ends, and return the size read.
  virtual WASI::WasiExpect<std::size_t> read(Span<uint8_t> Buffer) noexcept = 0;

  /// Write the whole buffer.
  virtual WASI::WasiExpect<void> write(Span<const uint8_t> Buffer) noexcept = 0;

  /// Move the offset of the file, and return the new offset.
  virtual WASI::WasiExpect<__wasi_filesize_t>
  seek(__wasi_filedelta_t Offset, __wasi_whence_t Whence) noexcept = 0;

  virtual WASI::WasiExpect<__wasi_filesize_t> tell() noexcept = 0;

  /// Get the native handle: an int on POSIX, or a HANDLE on Windows.
  virtual WASI::WasiExpect<uint64_t> getNativeHandle() noexcept = 0;
};

/// The filesystem of a guest: the paths are the paths of the guest, and the
/// files are opened with the rights of the guest.
class FileSystem {
public:
  virtual ~FileSystem() noexcept = default;

  virtual WASI::WasiExpect<std::shared_ptr<File>>
  open(std::string_view Path, __wasi_oflags_t OpenFlags, __wasi_rights_t Rights,
       __wasi_fdflags_t FdFlags) const noexcept = 0;

  virtual WASI::WasiExpect<void>
  createDirectory(std::string_view Path) const noexcept = 0;
};

} // namespace Host
} // namespace WasmEdge
