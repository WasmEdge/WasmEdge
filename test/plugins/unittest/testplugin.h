// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"
#include "po/argument_parser.h"
#include "po/list.h"
#include "po/option.h"

#include <cstdint>
#include <string>

namespace WasmEdge {
namespace Host {

class WasmEdgePluginTestEnv {
public:
  WasmEdgePluginTestEnv() noexcept = default;

  static PO::List<std::string> CmdArgs;
  static PO::Option<std::string> CmdName;
  static PO::Option<PO::Toggle> CmdOpt;
};

template <typename T>
class WasmEdgePluginTestFunc : public Runtime::HostFunction<T> {
public:
  WasmEdgePluginTestFunc(WasmEdgePluginTestEnv &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WasmEdgePluginTestEnv &Env;
};

class WasmEdgePluginTestFuncAdd
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncAdd> {
public:
  WasmEdgePluginTestFuncAdd(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t A, uint32_t B) {
    return A + B;
  }
};

class WasmEdgePluginTestFuncSub
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncSub> {
public:
  WasmEdgePluginTestFuncSub(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t A, uint32_t B) {
    return A - B;
  }
};

class WasmEdgePluginTestFuncArgLen
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncArgLen> {
public:
  WasmEdgePluginTestFuncArgLen(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    return static_cast<uint32_t>(Env.CmdArgs.value().size());
  }
};

class WasmEdgePluginTestFuncOpt
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncOpt> {
public:
  WasmEdgePluginTestFuncOpt(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    return static_cast<uint32_t>(Env.CmdOpt.value());
  }
};

class WasmEdgePluginTestFuncNameSize
    : public WasmEdgePluginTestFunc<WasmEdgePluginTestFuncNameSize> {
public:
  WasmEdgePluginTestFuncNameSize(WasmEdgePluginTestEnv &HostEnv)
      : WasmEdgePluginTestFunc(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    return static_cast<uint32_t>(Env.CmdName.value().size());
  }
};

class WasmEdgePluginTestModule : public Runtime::Instance::ModuleInstance {
public:
  WasmEdgePluginTestModule()
      : Runtime::Instance::ModuleInstance("wasmedge_plugintest_cpp_module") {
    addHostFunc("add", std::make_unique<WasmEdgePluginTestFuncAdd>(Env));
    addHostFunc("sub", std::make_unique<WasmEdgePluginTestFuncSub>(Env));
    addHostFunc("arg_len", std::make_unique<WasmEdgePluginTestFuncArgLen>(Env));
    addHostFunc("opt", std::make_unique<WasmEdgePluginTestFuncOpt>(Env));
    addHostFunc("name_size",
                std::make_unique<WasmEdgePluginTestFuncNameSize>(Env));
  }

  WasmEdgePluginTestEnv &getEnv() { return Env; }

private:
  WasmEdgePluginTestEnv Env;
};

} // namespace Host
} // namespace WasmEdge
