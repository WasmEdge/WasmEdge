// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

// TODO: complete this resource
class Descriptor : public AST::Component::ResourceType {
public:
  Descriptor()
      : ResourceType(new Runtime::Instance::ComponentInstance("descriptor")) {}
};

TypesModule::TypesModule() : ComponentInstance("wasi:filesystem/types@0.2.0") {
  addHostType("descriptor", Descriptor());
  addHostFunc("[method]descriptor.write-via-stream",
              std::make_unique<Descriptor_WriteViaStream>(Env));
  addHostFunc("[method]descriptor.append-via-stream",
              std::make_unique<Descriptor_AppendViaStream>(Env));
  addHostFunc("[method]descriptor.get-type",
              std::make_unique<Descriptor_GetType>(Env));
  addHostFunc("[method]descriptor.stat",
              std::make_unique<Descriptor_Stat>(Env));

  addHostFunc("filesystem-error-code",
              std::make_unique<FilesystemErrorCode>(Env));
}

PreopensModule::PreopensModule()
    : ComponentInstance("wasi:filesystem/preopens@0.2.0") {
  addHostFunc("get-directories", std::make_unique<GetDirectories>(Env));
}

} // namespace Host
} // namespace WasmEdge
