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

class GetDirectories : public WasiFilesystem<GetDirectories> {
public:
  GetDirectories(WasiFilesystemEnvironment &HostEnv)
      : WasiFilesystem(HostEnv) {}
  // TODO
  // get-directories: func() -> list<tuple<descriptor, string>>;
  Expect<void> body() { return {}; }
};

} // namespace Host
} // namespace WasmEdge
