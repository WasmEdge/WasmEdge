// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "runtime/instance/struct.h"
#include "runtime/instance/module.h"

namespace WasmEdge::Runtime::Instance {

StructInstance::StructInstance(GC::Allocator &Allocator,
                               const ModuleInstance *ModInst, uint32_t TypeIdx,
                               std::vector<ValVariant> &&Init) noexcept {
  assuming(Init.size() <=
           (std::numeric_limits<uint32_t>::max() - sizeof(RawData)) /
               sizeof(ValVariant));
  Data = static_cast<RawData *>(Allocator.allocate(
      [&](void *Pointer) {
        auto Raw = static_cast<RawData *>(Pointer);
        Raw->ModInst = ModInst;
        Raw->TypeIdx = TypeIdx;
        Raw->Length = static_cast<uint32_t>(Init.size());
        std::copy(Init.begin(), Init.end(), Raw->Data);
      },
      static_cast<uint32_t>(sizeof(RawData) +
                            Init.size() * sizeof(ValVariant))));
}

} // namespace WasmEdge::Runtime::Instance
