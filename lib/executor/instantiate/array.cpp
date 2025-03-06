// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "runtime/instance/array.h"
#include "runtime/instance/module.h"

namespace WasmEdge::Runtime::Instance {
ArrayInstance::ArrayInstance(GC::Allocator &Allocator,
                             const AST::CompositeType &CompType,
                             const uint32_t Size,
                             const ValVariant &Init) noexcept {
  assuming(CompType.getFieldTypes().size() == Size);
  Data = static_cast<RawArray *>(Allocator.allocate(
      CompType, sizeof(RawArray) + Size * sizeof(ValVariant)));
  Data->Length = Size;
  std::fill(Data->Data, Data->Data + Size, Init);
}
ArrayInstance::ArrayInstance(GC::Allocator &Allocator,
                             const AST::CompositeType &CompType,
                             std::vector<ValVariant> &&Init) noexcept {
  assuming(CompType.getFieldTypes().size() == Init.size());
  Data = static_cast<RawArray *>(Allocator.allocate(
      CompType, sizeof(RawArray) + Init.size() * sizeof(ValVariant)));
  Data->Length = Init.size();
  std::copy(Init.begin(), Init.end(), Data->Data);
}
} // namespace WasmEdge::Runtime::Instance
