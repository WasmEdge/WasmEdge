// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runTableGetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Read the index in place (peek, not pop): table.get consumes one i32 and
  // produces one ref, so it overwrites this same top slot below.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Idx = extractAddr(StackMgr.peekTop<ValVariant>(), AddrType);

  // Load table[Idx] and replace the top slot in place.
  return TabInst.getRefAddr(Idx)
      .map_error([&Instr, Idx, AddrType](auto E) {
        spdlog::error(ErrInfo::InfoInstruction(
            Instr.getOpCode(), Instr.getOffset(), {Idx},
            {AddrType == AddressType::I64 ? ValTypeFromType<uint64_t>()
                                          : ValTypeFromType<uint32_t>()}));
        return E;
      })
      .and_then([&](RefVariant Ref) -> Expect<void> {
        StackMgr.emplaceTop(std::move(Ref));
        return {};
      });
}

Expect<void> Executor::runTableSetOp(Runtime::StackManager &StackMgr,
                                     Runtime::Instance::TableInstance &TabInst,
                                     const AST::Instruction &Instr) {
  // Peek Ref and Idx, keeping Ref on the GC-rooted value stack across
  // setRefAddr() so a concurrent collection cannot reclaim it first. Pop after.
  const RefVariant Ref = StackMgr.peekTop<RefVariant>();
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  const uint64_t Idx = extractAddr(StackMgr.peekTopN<ValVariant>(2), AddrType);

  // Set table[Idx] with Ref.
  auto Res =
      TabInst.setRefAddr(Idx, Ref).map_error([&Instr, Idx, AddrType](auto E) {
        spdlog::error(ErrInfo::InfoInstruction(
            Instr.getOpCode(), Instr.getOffset(), {Idx},
            {AddrType == AddressType::I64 ? ValTypeFromType<uint64_t>()
                                          : ValTypeFromType<uint32_t>()}));
        return E;
      });
  StackMgr.eraseValueStack(2, 0);
  return Res;
}

Expect<void>
Executor::runTableInitOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst,
                         Runtime::Instance::ElementInstance &ElemInst,
                         const AST::Instruction &Instr) {
  // Pop the length, source, and destination from the stack.
  // Currently, the length and source offset from the element instance are
  // 32-bit.
  auto [Len, Src, RawDst] = StackMgr.pops<uint32_t, uint32_t, ValVariant>();
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  uint64_t Dst = extractAddr(RawDst, AddrType);

  // Replace tab[Dst : Dst + Len] with elem[Src : Src + Len].
  return ElemInst.getRefs(Src, Len)
      .and_then([&, Dst](Span<const RefVariant> Refs) {
        return TabInst.setRefs(Refs, Dst);
      })
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

Expect<void>
Executor::runElemDropOp(Runtime::Instance::ElementInstance &ElemInst) {
  // Clear element instance.
  ElemInst.clear();
  return {};
}

Expect<void>
Executor::runTableCopyOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInstDst,
                         Runtime::Instance::TableInstance &TabInstSrc,
                         const AST::Instruction &Instr) {
  // Pop the length, source, and destination from the stack.
  auto [RawLen, RawSrc, RawDst] =
      StackMgr.pops<ValVariant, ValVariant, ValVariant>();
  const auto AddrTypeSrc = TabInstSrc.getTableType().getLimit().getAddrType();
  const auto AddrTypeDst = TabInstDst.getTableType().getLimit().getAddrType();
  // Each address operand is typed by its own table; the length uses the
  // smaller of the two address types (per the table64 rules).
  uint64_t Len = extractAddr(RawLen, std::min(AddrTypeSrc, AddrTypeDst));
  uint64_t Src = extractAddr(RawSrc, AddrTypeSrc);
  uint64_t Dst = extractAddr(RawDst, AddrTypeDst);

  // Replace tab_dst[Dst : Dst + Len] with tab_src[Src : Src + Len].
  return TabInstSrc.getRefs(Src, Len)
      .and_then([&, Dst](Span<const RefVariant> Refs) {
        return TabInstDst.setRefs(Refs, Dst);
      })
      .map_error([&Instr](auto E) {
        spdlog::error(
            ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
        return E;
      });
}

Expect<void>
Executor::runTableGrowOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Peek N and the init Ref, keeping Ref on the GC-rooted value stack across
  // growTable() so a concurrent collection cannot reclaim it before the new
  // slots are filled. Pop after.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  const uint64_t N = extractAddr(StackMgr.peekTop<ValVariant>(), AddrType);
  const RefVariant Ref = StackMgr.peekTopN<RefVariant>(2);

  // Grow size and push result.
  const uint64_t CurrSize = TabInst.getSize();
  const bool Grown = TabInst.growTable(N, Ref);
  StackMgr.eraseValueStack(2, 0);
  if (Grown) {
    StackMgr.push(emplaceAddr(CurrSize, AddrType));
  } else {
    StackMgr.push(emplaceAddr(static_cast<uint64_t>(-1), AddrType));
  }
  return {};
}

Expect<void>
Executor::runTableSizeOp(Runtime::StackManager &StackMgr,
                         Runtime::Instance::TableInstance &TabInst) {
  // Push SZ = size to the stack.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  StackMgr.push(emplaceAddr(TabInst.getSize(), AddrType));
  return {};
}

Expect<void> Executor::runTableFillOp(Runtime::StackManager &StackMgr,
                                      Runtime::Instance::TableInstance &TabInst,
                                      const AST::Instruction &Instr) {
  // Peek the length, ref_value, and offset, keeping the fill Ref on the
  // GC-rooted value stack across fillRefs() so a concurrent collection cannot
  // reclaim it before the slots are written. Pop after.
  const auto AddrType = TabInst.getTableType().getLimit().getAddrType();
  const uint64_t Len = extractAddr(StackMgr.peekTop<ValVariant>(), AddrType);
  const RefVariant Val = StackMgr.peekTopN<RefVariant>(2);
  const uint64_t Off = extractAddr(StackMgr.peekTopN<ValVariant>(3), AddrType);

  // Fill refs with ref_value.
  auto Res = TabInst.fillRefs(Val, Off, Len).map_error([&Instr](auto E) {
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return E;
  });
  StackMgr.eraseValueStack(3, 0);
  return Res;
}

} // namespace Executor
} // namespace WasmEdge
