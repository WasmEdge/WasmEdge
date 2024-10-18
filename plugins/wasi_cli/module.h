// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiCliEnvironmentModule : public Runtime::Instance::ComponentInstance {
public:
  WasiCliEnvironmentModule();

  WasiCliEnvironment &getEnv() { return Env; }

private:
  WasiCliEnvironment Env;
};

class WasiCliExitModule : public Runtime::Instance::ComponentInstance {
public:
  WasiCliExitModule();

  WasiCliEnvironment &getEnv() { return Env; }

private:
  WasiCliEnvironment Env;
};

class WasiCliStdinModule : public Runtime::Instance::ComponentInstance {
public:
  WasiCliStdinModule();

  WasiCliEnvironment &getEnv() { return Env; }

private:
  WasiCliEnvironment Env;
};

class WasiCliStdoutModule : public Runtime::Instance::ComponentInstance {
public:
  WasiCliStdoutModule();

  WasiCliEnvironment &getEnv() { return Env; }

private:
  WasiCliEnvironment Env;
};

class WasiCliStderrModule : public Runtime::Instance::ComponentInstance {
public:
  WasiCliStderrModule();

  WasiCliEnvironment &getEnv() { return Env; }

private:
  WasiCliEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
