// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

// TODO: Use wasi:error/error error to figure out what a resource host function
// should be.
class ToDebugString : public WasiIO<ToDebugString> {
public:
  ToDebugString(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  using IoError = uint32_t;
  Expect<void> body(IoError) { return {}; }
};

class IoError : public AST::Component::ResourceType {
public:
  IoError(WasiIOEnvironment &Env)
      : ResourceType(new Runtime::Instance::ComponentInstance("error")) {
    // to-debug-string: func() -> string;
    Impl->addHostFunc("to-debug-string", std::make_unique<ToDebugString>(Env));
  }
};

WasiIOErrorModule::WasiIOErrorModule()
    : ComponentInstance("wasi:io/error@0.2.0") {
  addExport("error", IoError(getEnv()));
}

class InputStream : public AST::Component::ResourceType {
public:
  InputStream()
      : ResourceType(new Runtime::Instance::ComponentInstance("input-stream")) {
  }
};
class OutputStream : public AST::Component::ResourceType {
public:
  OutputStream()
      : ResourceType(
            new Runtime::Instance::ComponentInstance("output-stream")) {}
};

class StreamError : public AST::Component::ResourceType {
public:
  StreamError()
      : ResourceType(new Runtime::Instance::ComponentInstance("error")) {}
};

WasiIOStreamsModule::WasiIOStreamsModule()
    : ComponentInstance("wasi:io/streams@0.2.0") {
  addExport("input-stream", InputStream());
  addExport("output-stream", OutputStream());
  addExport("error", StreamError());
}

} // namespace Host
} // namespace WasmEdge
