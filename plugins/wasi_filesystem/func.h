// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

class Descriptor_WriteViaStream
    : public WasiFilesystem<Descriptor_WriteViaStream> {
public:
  Descriptor_WriteViaStream(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // write-via-stream: func(
  //   /// The offset within the file at which to start writing.
  //   offset: filesize,
  // ) -> result<output-stream, error-code>;
  Expect<void> body() { return {}; }
};
class Descriptor_AppendViaStream
    : public WasiFilesystem<Descriptor_AppendViaStream> {
public:
  Descriptor_AppendViaStream(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // append-via-stream: func() -> result<output-stream, error-code>;
  Expect<void> body() { return {}; }
};
class Descriptor_GetType : public WasiFilesystem<Descriptor_GetType> {
public:
  Descriptor_GetType(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // get-type: func() -> result<descriptor-type, error-code>;
  Expect<void> body() { return {}; }
};
class Descriptor_Stat : public WasiFilesystem<Descriptor_Stat> {
public:
  Descriptor_Stat(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // stat: func() -> result<descriptor-stat, error-code>;
  Expect<void> body() { return {}; }
};

class FilesystemErrorCode : public WasiFilesystem<FilesystemErrorCode> {
public:
  FilesystemErrorCode(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // filesystem-error-code: func(err: borrow<error>) -> option<error-code>;
  Expect<void> body() { return {}; }
};

class GetDirectories : public WasiFilesystem<GetDirectories> {
public:
  using Descriptor = uint32_t;

  GetDirectories(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // get-directories: func() -> list<tuple<descriptor, string>>;
  Expect<List<Tuple<Descriptor, std::string>>> body();
};

} // namespace Host
} // namespace WasmEdge
