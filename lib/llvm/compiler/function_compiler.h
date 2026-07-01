// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "compiler/context.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {

class FunctionCompiler {
  struct Control;

public:
  FunctionCompiler(LLVM::Compiler::CompileContext &Context,
                   LLVM::FunctionCallee F, Span<const ValType> Locals,
                   bool Interruptible, bool InstructionCounting,
                   bool GasMeasuring, bool IsLazyJIT) noexcept;

  LLVM::BasicBlock getTrapBB(ErrCode::Value Error) noexcept;

  Expect<void>
  compile(const AST::CodeSegment &Code,
          std::pair<std::vector<ValType>, std::vector<ValType>> Type) noexcept;

  Expect<void> compile(AST::InstrView Instrs) noexcept;
  void compileSignedTrunc(LLVM::Type IntType) noexcept;
  void compileSignedTruncSat(LLVM::Type IntType) noexcept;
  void compileUnsignedTrunc(LLVM::Type IntType) noexcept;
  void compileUnsignedTruncSat(LLVM::Type IntType) noexcept;

  void compileAtomicCheckOffsetAlignment(LLVM::Value Offset,
                                         LLVM::Type IntType) noexcept;

  void compileMemoryFence() noexcept;
  void compileAtomicNotify(unsigned MemoryIndex,
                           uint64_t MemoryOffset) noexcept;
  void compileAtomicWait(unsigned MemoryIndex, uint64_t MemoryOffset,
                         LLVM::Type TargetType, uint32_t BitWidth) noexcept;
  void compileAtomicLoad(unsigned MemoryIndex, uint64_t MemoryOffset,
                         unsigned Alignment, LLVM::Type IntType,
                         LLVM::Type TargetType, bool Signed = false) noexcept;
  void compileAtomicStore(unsigned MemoryIndex, uint64_t MemoryOffset,
                          unsigned Alignment, LLVM::Type, LLVM::Type TargetType,
                          bool Signed = false) noexcept;

  void compileAtomicRMWOp(unsigned MemoryIndex, uint64_t MemoryOffset,
                          [[maybe_unused]] unsigned Alignment,
                          LLVMAtomicRMWBinOp BinOp, LLVM::Type IntType,
                          LLVM::Type TargetType, bool Signed = false) noexcept;
  void compileAtomicCompareExchange(unsigned MemoryIndex, uint64_t MemoryOffset,
                                    [[maybe_unused]] unsigned Alignment,
                                    LLVM::Type IntType, LLVM::Type TargetType,
                                    bool Signed = false) noexcept;

  void compileReturn() noexcept;

  void updateInstrCount() noexcept;

  void updateGas() noexcept;

  void updateGasAtTrap() noexcept;

private:
  void compileCallOp(const unsigned int FuncIndex) noexcept;

  void compileIndirectCallOp(const uint32_t TableIndex,
                             const uint32_t FuncTypeIndex) noexcept;

  void compileReturnCallOp(const unsigned int FuncIndex) noexcept;

  void compileReturnIndirectCallOp(const uint32_t TableIndex,
                                   const uint32_t FuncTypeIndex) noexcept;

  void compileCallRefOp(const unsigned int TypeIndex) noexcept;

  void compileReturnCallRefOp(const unsigned int TypeIndex) noexcept;

  void compileLoadOp(unsigned MemoryIndex, uint64_t Offset, unsigned Alignment,
                     LLVM::Type LoadTy) noexcept;
  void compileLoadOp(unsigned MemoryIndex, uint64_t Offset, unsigned Alignment,
                     LLVM::Type LoadTy, LLVM::Type ExtendTy,
                     bool Signed) noexcept;
  void compileVectorLoadOp(unsigned MemoryIndex, uint64_t Offset,
                           unsigned Alignment, LLVM::Type LoadTy) noexcept;
  void compileVectorLoadOp(unsigned MemoryIndex, uint64_t Offset,
                           unsigned Alignment, LLVM::Type LoadTy,
                           LLVM::Type ExtendTy, bool Signed) noexcept;
  void compileSplatLoadOp(unsigned MemoryIndex, uint64_t Offset,
                          unsigned Alignment, LLVM::Type LoadTy,
                          LLVM::Type VectorTy) noexcept;
  void compileLoadLaneOp(unsigned MemoryIndex, uint64_t Offset,
                         unsigned Alignment, unsigned Index, LLVM::Type LoadTy,
                         LLVM::Type VectorTy) noexcept;
  void compileStoreOp(uint32_t MemoryIndex, uint64_t Offset, uint32_t Alignment,
                      LLVM::Type LoadTy, bool Trunc = false,
                      bool BitCast = false) noexcept;
  void compileStoreLaneOp(uint32_t MemoryIndex, uint64_t Offset,
                          uint32_t Alignment, uint8_t Index, LLVM::Type LoadTy,
                          LLVM::Type VectorTy) noexcept;
  void compileSplatOp(LLVM::Type VectorTy) noexcept;
  void compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index) noexcept;
  void compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index,
                            LLVM::Type ExtendTy, bool Signed) noexcept;
  void compileReplaceLaneOp(LLVM::Type VectorTy, unsigned Index) noexcept;
  void compileVectorCompareOp(LLVM::Type VectorTy,
                              LLVMIntPredicate Predicate) noexcept;
  void compileVectorCompareOp(LLVM::Type VectorTy, LLVMRealPredicate Predicate,
                              LLVM::Type ResultTy) noexcept;
  template <typename Func>
  void compileVectorOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto V = Builder.createBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.createBitCast(Op(V), Context.Int64x2Ty);
  }
  void compileVectorAbs(LLVM::Type VectorTy) noexcept;
  void compileVectorNeg(LLVM::Type VectorTy) noexcept;
  void compileVectorPopcnt() noexcept;
  template <typename Func>
  void compileVectorReduceIOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto V = Builder.createBitCast(Stack.back(), VectorTy);
    Stack.back() = Builder.createZExt(Op(V), Context.Int32Ty);
  }
  void compileVectorAnyTrue() noexcept;
  void compileVectorAllTrue(LLVM::Type VectorTy) noexcept;
  void compileVectorBitMask(LLVM::Type VectorTy) noexcept;
  template <typename Func>
  void compileVectorShiftOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    const bool Trunc = VectorTy.getElementType().getIntegerBitWidth() < 32;
    const uint32_t Mask = VectorTy.getElementType().getIntegerBitWidth() - 1;
    auto N = Builder.createAnd(stackPop(), LLContext.getInt32(Mask));
    auto RHS = Builder.createVectorSplat(
        VectorTy.getVectorSize(),
        Trunc ? Builder.createTrunc(N, VectorTy.getElementType())
              : Builder.createZExtOrTrunc(N, VectorTy.getElementType()));
    auto LHS = Builder.createBitCast(stackPop(), VectorTy);
    stackPush(Builder.createBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorShl(LLVM::Type VectorTy) noexcept;
  void compileVectorLShr(LLVM::Type VectorTy) noexcept;
  void compileVectorAShr(LLVM::Type VectorTy) noexcept;
  template <typename Func>
  void compileVectorVectorOp(LLVM::Type VectorTy, Func &&Op) noexcept {
    auto RHS = Builder.createBitCast(stackPop(), VectorTy);
    auto LHS = Builder.createBitCast(stackPop(), VectorTy);
    stackPush(Builder.createBitCast(Op(LHS, RHS), Context.Int64x2Ty));
  }
  void compileVectorVectorAdd(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorAddSat(LLVM::Type VectorTy, bool Signed) noexcept;
  void compileVectorVectorSub(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorSubSat(LLVM::Type VectorTy, bool Signed) noexcept;
  void compileVectorVectorMul(LLVM::Type VectorTy) noexcept;
  void compileVectorSwizzle() noexcept;

  void compileVectorVectorQ15MulSat() noexcept;
  void compileVectorVectorSMin(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorUMin(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorSMax(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorUMax(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorUAvgr(LLVM::Type VectorTy) noexcept;
  void compileVectorNarrow(LLVM::Type FromTy, bool Signed) noexcept;
  void compileVectorExtend(LLVM::Type FromTy, bool Signed, bool Low) noexcept;
  void compileVectorExtMul(LLVM::Type FromTy, bool Signed, bool Low) noexcept;
  void compileVectorExtAddPairwise(LLVM::Type VectorTy, bool Signed) noexcept;
  void compileVectorFAbs(LLVM::Type VectorTy) noexcept;
  void compileVectorFNeg(LLVM::Type VectorTy) noexcept;
  void compileVectorFSqrt(LLVM::Type VectorTy) noexcept;
  void compileVectorFCeil(LLVM::Type VectorTy) noexcept;
  void compileVectorFFloor(LLVM::Type VectorTy) noexcept;
  void compileVectorFTrunc(LLVM::Type VectorTy) noexcept;
  void compileVectorFNearest(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFAdd(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFSub(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFMul(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFDiv(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFMin(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFMax(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFPMin(LLVM::Type VectorTy) noexcept;
  void compileVectorVectorFPMax(LLVM::Type VectorTy) noexcept;
  void compileVectorTruncSatS32(LLVM::Type VectorTy, bool PadZero) noexcept;
  void compileVectorTruncSatU32(LLVM::Type VectorTy, bool PadZero) noexcept;
  void compileVectorConvertS(LLVM::Type VectorTy, LLVM::Type FPVectorTy,
                             bool Low) noexcept;
  void compileVectorConvertU(LLVM::Type VectorTy, LLVM::Type FPVectorTy,
                             bool Low) noexcept;
  void compileVectorDemote() noexcept;
  void compileVectorPromote() noexcept;

  void compileVectorVectorMAdd(LLVM::Type VectorTy) noexcept;

  void compileVectorVectorNMAdd(LLVM::Type VectorTy) noexcept;

  void compileVectorRelaxedIntegerDotProduct() noexcept;

  void compileVectorRelaxedIntegerDotProductAdd() noexcept;

  void
  enterBlock(LLVM::BasicBlock JumpBlock, LLVM::BasicBlock NextBlock,
             LLVM::BasicBlock ElseBlock, std::vector<LLVM::Value> Args,
             std::pair<std::vector<ValType>, std::vector<ValType>> Type,
             std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
                 ReturnPHI = {}) noexcept;

  Control leaveBlock() noexcept;

  void checkStop() noexcept;

  void setUnreachable() noexcept;

  bool isUnreachable() const noexcept;

  void
  buildPHI(Span<const ValType> RetType,
           Span<const std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
               Incomings) noexcept;

  void setLableJumpPHI(unsigned int Index) noexcept;

  LLVM::BasicBlock getLabel(unsigned int Index) const noexcept;

  void stackPush(LLVM::Value Value) noexcept { Stack.push_back(Value); }
  LLVM::Value stackPop() noexcept;

  LLVM::Value switchEndian(LLVM::Value Value);

  LLVM::Compiler::CompileContext &Context;
  LLVM::Context LLContext;
  std::vector<std::pair<LLVM::Type, LLVM::Value>> Local;
  std::vector<LLVM::Value> Stack;
  LLVM::Value LocalInstrCount = nullptr;
  LLVM::Value LocalGas = nullptr;
  std::unordered_map<ErrCode::Value, LLVM::BasicBlock> TrapBB;
  bool IsUnreachable = false;
  bool Interruptible = false;
  struct Control {
    size_t StackSize;
    bool Unreachable;
    LLVM::BasicBlock JumpBlock;
    LLVM::BasicBlock NextBlock;
    LLVM::BasicBlock ElseBlock;
    std::vector<LLVM::Value> Args;
    std::pair<std::vector<ValType>, std::vector<ValType>> Type;
    std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
        ReturnPHI;
    Control(size_t S, bool U, LLVM::BasicBlock J, LLVM::BasicBlock N,
            LLVM::BasicBlock E, std::vector<LLVM::Value> A,
            std::pair<std::vector<ValType>, std::vector<ValType>> T,
            std::vector<std::tuple<std::vector<LLVM::Value>, LLVM::BasicBlock>>
                R) noexcept
        : StackSize(S), Unreachable(U), JumpBlock(J), NextBlock(N),
          ElseBlock(E), Args(std::move(A)), Type(std::move(T)),
          ReturnPHI(std::move(R)) {}
    Control(const Control &) = default;
    Control(Control &&) = default;
    Control &operator=(const Control &) = default;
    Control &operator=(Control &&) = default;
  };
  bool IsLazyJIT;
  std::vector<Control> ControlStack;
  LLVM::FunctionCallee F;
  LLVM::Value ExecCtx;
  LLVM::Builder Builder;
};

} // namespace WasmEdge
