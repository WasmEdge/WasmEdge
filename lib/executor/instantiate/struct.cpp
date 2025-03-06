// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "runtime/instance/struct.h"
#include "runtime/instance/module.h"

namespace WasmEdge::Runtime::Instance {

StructInstance::StructInstance(GC::Allocator &Allocator,
                               const AST::CompositeType &CompType,
                               std::vector<ValVariant> &&Init) noexcept {
  assuming(CompType.getFieldTypes().size() == Init.size());
  Data = static_cast<RawStruct *>(Allocator.allocate(
      CompType, sizeof(RawStruct) + Init.size() * sizeof(ValVariant)));
  Data->Length = Init.size();
  std::copy(Init.begin(), Init.end(), Data->Data);
}

} // namespace WasmEdge::Runtime::Instance
