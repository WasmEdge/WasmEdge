// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "env.h"

#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class TypesModule : public Runtime::Instance::ComponentInstance {
public:
  TypesModule();
  WasiFilesystemEnvironment &getEnv() { return Env; }

private:
  WasiFilesystemEnvironment Env;
};

class PreopensModule : public Runtime::Instance::ComponentInstance {
public:
  PreopensModule();
  WasiFilesystemEnvironment &getEnv() { return Env; }

private:
  WasiFilesystemEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
