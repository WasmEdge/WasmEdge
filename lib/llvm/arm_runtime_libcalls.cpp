// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "arm_runtime_libcalls.h"

#include "common/spdlog.h"
#include "llvm.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include <array>
#include <string>
#include <tuple>

using namespace std::literals;

namespace WasmEdge {
namespace LLVM {

namespace {

enum class RoundingMode { Ceil, Floor, Trunc, RoundEven };

llvm::Function *addRoundingCore(llvm::Module &Module, llvm::StringRef Name,
                                unsigned Width, RoundingMode Mode) {
  auto &Context = Module.getContext();
  auto *Integer = llvm::Type::getIntNTy(Context, Width);
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(Integer, {Integer}, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->setCallingConv(llvm::CallingConv::ARM_AAPCS);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  Function->addFnAttr(llvm::Attribute::AlwaysInline);

  const unsigned FractionBits = Width == 32 ? 23 : 52;
  const unsigned ExponentBits = Width == 32 ? 8 : 11;
  const unsigned Bias = Width == 32 ? 127 : 1023;
  const uint64_t SignMask = UINT64_C(1) << (Width - 1);
  const uint64_t ExponentMask = (UINT64_C(1) << ExponentBits) - 1;
  const uint64_t FractionMask = (UINT64_C(1) << FractionBits) - 1;
  const uint64_t QuietMask = UINT64_C(1) << (FractionBits - 1);
  const uint64_t HalfBits = static_cast<uint64_t>(Bias - 1) << FractionBits;
  const uint64_t OneBits = static_cast<uint64_t>(Bias) << FractionBits;
  auto *Zero = llvm::ConstantInt::get(Integer, 0);
  auto *SignMaskValue = llvm::ConstantInt::get(Integer, SignMask);

  auto *Entry = llvm::BasicBlock::Create(Context, "entry", Function);
  auto *Small = llvm::BasicBlock::Create(Context, "small", Function);
  auto *SmallExit = llvm::BasicBlock::Create(Context, "small-exit", Function);
  auto *Fractional = llvm::BasicBlock::Create(Context, "fractional", Function);
  auto *Integral = llvm::BasicBlock::Create(Context, "integral", Function);
  auto *RoundFraction =
      llvm::BasicBlock::Create(Context, "round-fraction", Function);
  llvm::IRBuilder<> Builder(Entry);
  auto *Bits = Function->getArg(0);
  auto *Sign = Builder.CreateAnd(Bits, SignMaskValue);
  auto *Magnitude =
      Builder.CreateAnd(Bits, llvm::ConstantInt::get(Integer, SignMask - 1));
  auto *Negative = Builder.CreateICmpNE(Sign, Zero);
  auto *Exponent = Builder.CreateAnd(
      Builder.CreateLShr(Magnitude,
                         llvm::ConstantInt::get(Integer, FractionBits)),
      llvm::ConstantInt::get(Integer, ExponentMask));
  auto *Special = Builder.CreateICmpEQ(
      Exponent, llvm::ConstantInt::get(Integer, ExponentMask));
  auto *BelowOne =
      Builder.CreateICmpULT(Exponent, llvm::ConstantInt::get(Integer, Bias));
  Builder.CreateCondBr(Special, Integral, Small);

  Builder.SetInsertPoint(Small);
  auto *SmallResult = Sign;
  auto *NonZero = Builder.CreateICmpNE(Magnitude, Zero);
  if (Mode == RoundingMode::Ceil) {
    SmallResult = Builder.CreateSelect(
        Builder.CreateAnd(NonZero, Builder.CreateNot(Negative)),
        llvm::ConstantInt::get(Integer, OneBits), Sign);
  } else if (Mode == RoundingMode::Floor) {
    SmallResult = Builder.CreateSelect(
        Builder.CreateAnd(NonZero, Negative),
        llvm::ConstantInt::get(Integer, SignMask | OneBits), Sign);
  } else if (Mode == RoundingMode::RoundEven) {
    auto *AboveHalf = Builder.CreateICmpUGT(
        Magnitude, llvm::ConstantInt::get(Integer, HalfBits));
    SmallResult = Builder.CreateSelect(
        AboveHalf,
        Builder.CreateOr(Sign, llvm::ConstantInt::get(Integer, OneBits)), Sign);
  }
  Builder.CreateCondBr(BelowOne, SmallExit, Fractional);

  Builder.SetInsertPoint(SmallExit);
  Builder.CreateRet(SmallResult);

  Builder.SetInsertPoint(Fractional);
  auto *Unbiased =
      Builder.CreateSub(Exponent, llvm::ConstantInt::get(Integer, Bias));
  auto *HasFraction = Builder.CreateICmpULT(
      Unbiased, llvm::ConstantInt::get(Integer, FractionBits));
  Builder.CreateCondBr(HasFraction, RoundFraction, Integral);

  Builder.SetInsertPoint(RoundFraction);
  auto *Shift = Builder.CreateSub(llvm::ConstantInt::get(Integer, FractionBits),
                                  Unbiased);
  auto *Unit = Builder.CreateShl(llvm::ConstantInt::get(Integer, 1), Shift);
  auto *Mask = Builder.CreateSub(Unit, llvm::ConstantInt::get(Integer, 1));
  auto *Truncated = Builder.CreateAnd(Bits, Builder.CreateNot(Mask));
  auto *Remainder = Builder.CreateAnd(Bits, Mask);
  llvm::Value *Increment = llvm::ConstantInt::getFalse(Context);
  if (Mode == RoundingMode::Ceil) {
    Increment = Builder.CreateAnd(Builder.CreateICmpNE(Remainder, Zero),
                                  Builder.CreateNot(Negative));
  } else if (Mode == RoundingMode::Floor) {
    Increment =
        Builder.CreateAnd(Builder.CreateICmpNE(Remainder, Zero), Negative);
  } else if (Mode == RoundingMode::RoundEven) {
    auto *Half = Builder.CreateLShr(Unit, llvm::ConstantInt::get(Integer, 1));
    auto *AboveHalf = Builder.CreateICmpUGT(Remainder, Half);
    auto *AtHalf = Builder.CreateICmpEQ(Remainder, Half);
    auto *Odd = Builder.CreateICmpNE(
        Builder.CreateAnd(Builder.CreateLShr(Truncated, Shift),
                          llvm::ConstantInt::get(Integer, 1)),
        Zero);
    Increment = Builder.CreateOr(AboveHalf, Builder.CreateAnd(AtHalf, Odd));
  }
  Builder.CreateRet(Builder.CreateAdd(
      Truncated, Builder.CreateSelect(Increment, Unit, Zero)));

  Builder.SetInsertPoint(Integral);
  auto *NaN = Builder.CreateICmpNE(
      Builder.CreateAnd(Bits, llvm::ConstantInt::get(Integer, FractionMask)),
      Zero);
  Builder.CreateRet(Builder.CreateSelect(
      NaN, Builder.CreateOr(Bits, llvm::ConstantInt::get(Integer, QuietMask)),
      Bits));
  return Function;
}

llvm::Function *addMinMaxCore(llvm::Module &Module, llvm::StringRef Name,
                              unsigned Width, bool Minimum) {
  auto &Context = Module.getContext();
  auto *Integer = llvm::Type::getIntNTy(Context, Width);
  auto *Function = llvm::Function::Create(
      llvm::FunctionType::get(Integer, {Integer, Integer}, false),
      llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->setCallingConv(llvm::CallingConv::ARM_AAPCS);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  Function->addFnAttr(llvm::Attribute::AlwaysInline);
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Context, "entry", Function));
  const unsigned FractionBits = Width == 32 ? 23 : 52;
  const unsigned ExponentBits = Width == 32 ? 8 : 11;
  const uint64_t SignMask = UINT64_C(1) << (Width - 1);
  const uint64_t FractionMask = (UINT64_C(1) << FractionBits) - 1;
  const uint64_t ExponentMask = ((UINT64_C(1) << ExponentBits) - 1)
                                << FractionBits;
  const uint64_t QuietMask = UINT64_C(1) << (FractionBits - 1);
  auto *Zero = llvm::ConstantInt::get(Integer, 0);
  auto *ExponentMaskValue = llvm::ConstantInt::get(Integer, ExponentMask);
  auto *FractionMaskValue = llvm::ConstantInt::get(Integer, FractionMask);
  auto *QuietMaskValue = llvm::ConstantInt::get(Integer, QuietMask);
  auto *Left = Function->getArg(0);
  auto *Right = Function->getArg(1);
  const auto IsNaN = [&](llvm::Value *Value) {
    return Builder.CreateAnd(
        Builder.CreateICmpEQ(Builder.CreateAnd(Value, ExponentMaskValue),
                             ExponentMaskValue),
        Builder.CreateICmpNE(Builder.CreateAnd(Value, FractionMaskValue),
                             Zero));
  };
  const auto IsSignaling = [&](llvm::Value *Value) {
    return Builder.CreateAnd(
        IsNaN(Value),
        Builder.CreateICmpEQ(Builder.CreateAnd(Value, QuietMaskValue), Zero));
  };
  auto *LeftNaN = IsNaN(Left);
  auto *RightNaN = IsNaN(Right);
  auto *LeftSignaling = IsSignaling(Left);
  auto *RightSignaling = IsSignaling(Right);
  auto *NaNResult = Builder.CreateSelect(
      LeftSignaling, Builder.CreateOr(Left, QuietMaskValue),
      Builder.CreateSelect(
          RightSignaling, Builder.CreateOr(Right, QuietMaskValue),
          Builder.CreateSelect(LeftNaN, Right,
                               Builder.CreateSelect(RightNaN, Left, Zero))));
  auto *LeftSign = Builder.CreateICmpNE(
      Builder.CreateAnd(Left, llvm::ConstantInt::get(Integer, SignMask)), Zero);
  auto *RightSign = Builder.CreateICmpNE(
      Builder.CreateAnd(Right, llvm::ConstantInt::get(Integer, SignMask)),
      Zero);
  auto *DifferentSigns = Builder.CreateXor(LeftSign, RightSign);
  auto *LeftLessSameSign =
      Builder.CreateSelect(LeftSign, Builder.CreateICmpUGT(Left, Right),
                           Builder.CreateICmpULT(Left, Right));
  auto *LeftLess =
      Builder.CreateSelect(DifferentSigns, LeftSign, LeftLessSameSign);
  auto *NumericResult = Builder.CreateSelect(
      Minimum ? LeftLess : Builder.CreateNot(LeftLess), Left, Right);
  auto *AnyNaN = Builder.CreateOr(LeftNaN, RightNaN);
  Builder.CreateRet(Builder.CreateSelect(AnyNaN, NaNResult, NumericResult));
  return Function;
}

llvm::Function *addScalarWrapper(llvm::Module &Module, llvm::StringRef Name,
                                 llvm::Function *Core, unsigned Width,
                                 ARMFloatABI ABI, bool Binary) {
  auto &Context = Module.getContext();
  auto *Float = Width == 32 ? llvm::Type::getFloatTy(Context)
                            : llvm::Type::getDoubleTy(Context);
  auto *Integer = llvm::Type::getIntNTy(Context, Width);
  std::vector<llvm::Type *> Parameters(Binary ? 2 : 1, Float);
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(Float, Parameters, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->setCallingConv(ABI == ARMFloatABI::Hard
                               ? llvm::CallingConv::ARM_AAPCS_VFP
                               : llvm::CallingConv::ARM_AAPCS);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Context, "entry", Function));
  std::vector<llvm::Value *> Arguments;
  for (auto &Argument : Function->args())
    Arguments.push_back(Builder.CreateBitCast(&Argument, Integer));
  auto *Call = Builder.CreateCall(Core, Arguments);
  Call->setCallingConv(llvm::CallingConv::ARM_AAPCS);
  Builder.CreateRet(Builder.CreateBitCast(Call, Float));
  llvm::InlineFunctionInfo InlineInfo;
  (void)llvm::InlineFunction(*Call, InlineInfo);
  return Function;
}

llvm::Function *addDivModCore(llvm::Module &Module, llvm::StringRef Name,
                              llvm::IntegerType *Integer, bool Wide,
                              bool Signed) {
  auto *Pair = llvm::StructType::get(Integer, Integer);
#if LLVM_VERSION_MAJOR >= 15
  auto *Pointer = llvm::PointerType::getUnqual(Module.getContext());
#else
  auto *Pointer = Wide ? llvm::PointerType::getUnqual(Integer)
                       : llvm::PointerType::getUnqual(Pair);
#endif
  std::vector<llvm::Type *> Parameters{Integer, Integer, Pointer};
  if (Wide)
    Parameters.push_back(Pointer);
  auto *Function = llvm::Function::Create(
      llvm::FunctionType::get(llvm::Type::getVoidTy(Module.getContext()),
                              Parameters, false),
      llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->addFnAttr(llvm::Attribute::NoUnwind);

  auto *Entry =
      llvm::BasicBlock::Create(Module.getContext(), "entry", Function);
  auto *Loop =
      llvm::BasicBlock::Create(Module.getContext(), "divide", Function);
  auto *Exit = llvm::BasicBlock::Create(Module.getContext(), "exit", Function);
  llvm::IRBuilder<> Builder(Entry);
  auto *Zero = llvm::ConstantInt::get(Integer, 0);
  auto *One = llvm::ConstantInt::get(Integer, 1);
  auto *Left = Function->getArg(0);
  auto *Right = Function->getArg(1);
  llvm::Value *LeftSign = Zero;
  llvm::Value *RightSign = Zero;
  llvm::Value *Dividend = Left;
  llvm::Value *Divisor = Right;
  if (Signed) {
    LeftSign = Builder.CreateAShr(
        Left, llvm::ConstantInt::get(Integer, Integer->getBitWidth() - 1));
    RightSign = Builder.CreateAShr(
        Right, llvm::ConstantInt::get(Integer, Integer->getBitWidth() - 1));
    Dividend = Builder.CreateSub(Builder.CreateXor(Left, LeftSign), LeftSign);
    Divisor = Builder.CreateSub(Builder.CreateXor(Right, RightSign), RightSign);
  }
  Builder.CreateBr(Loop);

  Builder.SetInsertPoint(Loop);
  auto *Count = Builder.CreatePHI(Integer, 2, "count");
  auto *Quotient = Builder.CreatePHI(Integer, 2, "quotient");
  auto *Remainder = Builder.CreatePHI(Integer, 2, "remainder");
  Count->addIncoming(llvm::ConstantInt::get(Integer, Integer->getBitWidth()),
                     Entry);
  Quotient->addIncoming(Zero, Entry);
  Remainder->addIncoming(Zero, Entry);
  auto *NextCount = Builder.CreateSub(Count, One);
  auto *DividendBit =
      Builder.CreateAnd(Builder.CreateLShr(Dividend, NextCount), One);
  auto *Candidate =
      Builder.CreateOr(Builder.CreateShl(Remainder, One), DividendBit);
  auto *Subtract = Builder.CreateICmpUGE(Candidate, Divisor);
  auto *NextRemainder = Builder.CreateSelect(
      Subtract, Builder.CreateSub(Candidate, Divisor), Candidate);
  auto *NextQuotient = Builder.CreateOr(Builder.CreateShl(Quotient, One),
                                        Builder.CreateZExt(Subtract, Integer));
  auto *Done = Builder.CreateICmpEQ(NextCount, Zero);
  Builder.CreateCondBr(Done, Exit, Loop);
  Count->addIncoming(NextCount, Loop);
  Quotient->addIncoming(NextQuotient, Loop);
  Remainder->addIncoming(NextRemainder, Loop);

  Builder.SetInsertPoint(Exit);
  llvm::Value *ResultQuotient = NextQuotient;
  llvm::Value *ResultRemainder = NextRemainder;
  auto *DivisorZero = Builder.CreateICmpEQ(Right, Zero);
  if (Signed) {
    auto *QuotientSign = Builder.CreateXor(LeftSign, RightSign);
    ResultQuotient = Builder.CreateSub(
        Builder.CreateXor(ResultQuotient, QuotientSign), QuotientSign);
    ResultRemainder = Builder.CreateSub(
        Builder.CreateXor(ResultRemainder, LeftSign), LeftSign);
  }
  ResultQuotient = Builder.CreateSelect(DivisorZero, Zero, ResultQuotient);
  ResultRemainder = Builder.CreateSelect(DivisorZero, Left, ResultRemainder);
  if (Wide) {
    Builder.CreateStore(ResultQuotient, Function->getArg(2));
    Builder.CreateStore(ResultRemainder, Function->getArg(3));
  } else {
    auto *Result = Function->getArg(2);
    Builder.CreateStore(ResultQuotient,
                        Builder.CreateStructGEP(Pair, Result, 0));
    Builder.CreateStore(ResultRemainder,
                        Builder.CreateStructGEP(Pair, Result, 1));
  }
  Builder.CreateRetVoid();
  return Function;
}

llvm::Function *addDivZeroHook(llvm::Module &Module, llvm::StringRef Name,
                               llvm::IntegerType *Integer) {
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(Integer, {Integer}, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  llvm::IRBuilder<> Builder(
      llvm::BasicBlock::Create(Module.getContext(), "entry", Function));
  Builder.CreateRet(Function->getArg(0));
  return Function;
}

llvm::Function *addIntToFPCore(llvm::Module &Module, llvm::StringRef Name,
                               unsigned ResultBits, bool Signed) {
  auto &Context = Module.getContext();
  auto *I64 = llvm::Type::getInt64Ty(Context);
  auto *ResultType = llvm::Type::getIntNTy(Context, ResultBits);
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(ResultType, {I64}, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->setCallingConv(llvm::CallingConv::ARM_AAPCS);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  Function->addFnAttr(llvm::Attribute::AlwaysInline);

  auto *Entry = llvm::BasicBlock::Create(Context, "entry", Function);
  auto *Convert = llvm::BasicBlock::Create(Context, "convert", Function);
  auto *Round = llvm::BasicBlock::Create(Context, "round", Function);
  auto *Exact = llvm::BasicBlock::Create(Context, "exact", Function);
  auto *Compose = llvm::BasicBlock::Create(Context, "compose", Function);
  llvm::IRBuilder<> Builder(Entry);
  auto *Input = Function->getArg(0);
  auto *Zero64 = llvm::ConstantInt::get(I64, 0);
  llvm::Value *Sign = Zero64;
  llvm::Value *Magnitude = Input;
  if (Signed) {
    Sign = Builder.CreateAShr(Input, llvm::ConstantInt::get(I64, 63));
    Magnitude = Builder.CreateSub(Builder.CreateXor(Input, Sign), Sign);
  }
  auto *IsZero = Builder.CreateICmpEQ(Magnitude, Zero64);
  Builder.CreateCondBr(IsZero, Compose, Convert);

  Builder.SetInsertPoint(Convert);
#if LLVM_VERSION_MAJOR >= 21
  auto CTLZ = llvm::Intrinsic::getOrInsertDeclaration(
      &Module, llvm::Intrinsic::ctlz, {I64});
#else
  auto *CTLZ =
      llvm::Intrinsic::getDeclaration(&Module, llvm::Intrinsic::ctlz, {I64});
#endif
  auto *LeadingZeros = Builder.CreateCall(
      CTLZ, {Magnitude, llvm::ConstantInt::getFalse(Context)});
  auto *Exponent = Builder.CreateSub(llvm::ConstantInt::get(I64, 63),
                                     LeadingZeros, "exponent");
  const unsigned Precision = ResultBits == 32 ? 24 : 53;
  auto *NeedsRounding =
      Builder.CreateICmpUGE(Exponent, llvm::ConstantInt::get(I64, Precision));
  Builder.CreateCondBr(NeedsRounding, Round, Exact);

  Builder.SetInsertPoint(Round);
  auto *Shift =
      Builder.CreateSub(Exponent, llvm::ConstantInt::get(I64, Precision - 1));
  auto *Significand = Builder.CreateLShr(Magnitude, Shift);
  auto *Mask = Builder.CreateSub(
      Builder.CreateShl(llvm::ConstantInt::get(I64, 1), Shift),
      llvm::ConstantInt::get(I64, 1));
  auto *Remainder = Builder.CreateAnd(Magnitude, Mask);
  auto *Half = Builder.CreateShl(
      llvm::ConstantInt::get(I64, 1),
      Builder.CreateSub(Shift, llvm::ConstantInt::get(I64, 1)));
  auto *AboveHalf = Builder.CreateICmpUGT(Remainder, Half);
  auto *AtHalf = Builder.CreateICmpEQ(Remainder, Half);
  auto *Odd = Builder.CreateICmpNE(
      Builder.CreateAnd(Significand, llvm::ConstantInt::get(I64, 1)), Zero64);
  auto *RoundUp = Builder.CreateOr(AboveHalf, Builder.CreateAnd(AtHalf, Odd));
  auto *Rounded =
      Builder.CreateAdd(Significand, Builder.CreateZExt(RoundUp, I64));
  auto *Overflow = Builder.CreateICmpNE(
      Builder.CreateAnd(Rounded,
                        llvm::ConstantInt::get(I64, UINT64_C(1) << Precision)),
      Zero64);
  auto *RoundedSignificand = Builder.CreateSelect(
      Overflow, Builder.CreateLShr(Rounded, llvm::ConstantInt::get(I64, 1)),
      Rounded);
  auto *RoundedExponent =
      Builder.CreateAdd(Exponent, Builder.CreateZExt(Overflow, I64));
  Builder.CreateBr(Compose);

  Builder.SetInsertPoint(Exact);
  auto *ExactSignificand = Builder.CreateShl(
      Magnitude,
      Builder.CreateSub(llvm::ConstantInt::get(I64, Precision - 1), Exponent));
  Builder.CreateBr(Compose);

  Builder.SetInsertPoint(Compose);
  auto *FinalSignificand = Builder.CreatePHI(I64, 3, "significand");
  auto *FinalExponent = Builder.CreatePHI(I64, 3, "final-exponent");
  FinalSignificand->addIncoming(Zero64, Entry);
  FinalSignificand->addIncoming(RoundedSignificand, Round);
  FinalSignificand->addIncoming(ExactSignificand, Exact);
  FinalExponent->addIncoming(Zero64, Entry);
  FinalExponent->addIncoming(RoundedExponent, Round);
  FinalExponent->addIncoming(Exponent, Exact);
  const unsigned FractionBits = ResultBits == 32 ? 23 : 52;
  const unsigned Bias = ResultBits == 32 ? 127 : 1023;
  auto *BiasedExponent = Builder.CreateSelect(
      IsZero, Zero64,
      Builder.CreateAdd(FinalExponent, llvm::ConstantInt::get(I64, Bias)));
  auto *Bits = Builder.CreateOr(
      Builder.CreateShl(BiasedExponent,
                        llvm::ConstantInt::get(I64, FractionBits)),
      Builder.CreateAnd(
          FinalSignificand,
          llvm::ConstantInt::get(I64, (UINT64_C(1) << FractionBits) - 1)));
  if (Signed)
    Bits = Builder.CreateOr(
        Bits,
        Builder.CreateAnd(Sign, llvm::ConstantInt::get(
                                    I64, UINT64_C(1) << (ResultBits - 1))));
  Builder.CreateRet(Builder.CreateTruncOrBitCast(Bits, ResultType));
  return Function;
}

llvm::Function *addFPToIntCore(llvm::Module &Module, llvm::StringRef Name,
                               unsigned SourceBits, bool Signed) {
  auto &Context = Module.getContext();
  auto *I64 = llvm::Type::getInt64Ty(Context);
  auto *SourceType = llvm::Type::getIntNTy(Context, SourceBits);
  auto *Function =
      llvm::Function::Create(llvm::FunctionType::get(I64, {SourceType}, false),
                             llvm::GlobalValue::ExternalLinkage, Name, Module);
  Function->setVisibility(llvm::GlobalValue::HiddenVisibility);
  Function->setDSOLocal(true);
  Function->setCallingConv(llvm::CallingConv::ARM_AAPCS);
  Function->addFnAttr(llvm::Attribute::NoUnwind);
  Function->addFnAttr(llvm::Attribute::AlwaysInline);

  auto *Entry = llvm::BasicBlock::Create(Context, "entry", Function);
  auto *Range = llvm::BasicBlock::Create(Context, "range", Function);
  auto *Convert = llvm::BasicBlock::Create(Context, "convert", Function);
  auto *Right = llvm::BasicBlock::Create(Context, "right", Function);
  auto *Left = llvm::BasicBlock::Create(Context, "left", Function);
  auto *Exit = llvm::BasicBlock::Create(Context, "exit", Function);
  llvm::IRBuilder<> Builder(Entry);
  auto *Bits = Builder.CreateZExt(Function->getArg(0), I64);
  const unsigned FractionBits = SourceBits == 32 ? 23 : 52;
  const unsigned Bias = SourceBits == 32 ? 127 : 1023;
  const unsigned ExponentBits = SourceBits == 32 ? 8 : 11;
  const uint64_t SignMask = UINT64_C(1) << (SourceBits - 1);
  const uint64_t FractionMask = (UINT64_C(1) << FractionBits) - 1;
  auto *ExponentField = Builder.CreateAnd(
      Builder.CreateLShr(Bits, llvm::ConstantInt::get(I64, FractionBits)),
      llvm::ConstantInt::get(I64, (UINT64_C(1) << ExponentBits) - 1));
  auto *Fraction =
      Builder.CreateAnd(Bits, llvm::ConstantInt::get(I64, FractionMask));
  auto *NaN = Builder.CreateAnd(
      Builder.CreateICmpEQ(
          ExponentField,
          llvm::ConstantInt::get(I64, (UINT64_C(1) << ExponentBits) - 1)),
      Builder.CreateICmpNE(Fraction, llvm::ConstantInt::get(I64, 0)));
  auto *Negative = Builder.CreateICmpNE(
      Builder.CreateAnd(Bits, llvm::ConstantInt::get(I64, SignMask)),
      llvm::ConstantInt::get(I64, 0));
  auto *Exponent =
      Builder.CreateSub(ExponentField, llvm::ConstantInt::get(I64, Bias));
  auto *BelowOne =
      Builder.CreateICmpSLT(Exponent, llvm::ConstantInt::get(I64, 0));
  llvm::Value *InvalidLow = BelowOne;
  if (!Signed)
    InvalidLow = Builder.CreateOr(Negative, BelowOne);
  InvalidLow = Builder.CreateOr(NaN, InvalidLow);
  Builder.CreateCondBr(InvalidLow, Exit, Range);

  Builder.SetInsertPoint(Range);
  auto *Overflow =
      Builder.CreateICmpUGE(Exponent, llvm::ConstantInt::get(I64, 64));
  if (Signed) {
    auto *AtSignedLimit =
        Builder.CreateICmpEQ(Exponent, llvm::ConstantInt::get(I64, 63));
    auto *BeyondNegativeLimit = Builder.CreateAnd(
        Negative,
        Builder.CreateICmpNE(Fraction, llvm::ConstantInt::get(I64, 0)));
    Overflow = Builder.CreateOr(
        Overflow,
        Builder.CreateAnd(AtSignedLimit,
                          Builder.CreateOr(Builder.CreateNot(Negative),
                                           BeyondNegativeLimit)));
  }
  llvm::Value *Saturated = llvm::ConstantInt::get(I64, UINT64_MAX);
  if (Signed)
    Saturated = Builder.CreateSelect(
        Negative, llvm::ConstantInt::get(I64, UINT64_C(1) << 63),
        llvm::ConstantInt::get(I64, INT64_MAX));
  Builder.CreateCondBr(Overflow, Exit, Convert);

  Builder.SetInsertPoint(Convert);
  auto *Significand = Builder.CreateOr(
      Fraction, llvm::ConstantInt::get(I64, UINT64_C(1) << FractionBits));
  auto *ShiftRight = Builder.CreateICmpULT(
      Exponent, llvm::ConstantInt::get(I64, FractionBits));
  Builder.CreateCondBr(ShiftRight, Right, Left);

  Builder.SetInsertPoint(Right);
  auto *RightValue = Builder.CreateLShr(
      Significand,
      Builder.CreateSub(llvm::ConstantInt::get(I64, FractionBits), Exponent));
  Builder.CreateBr(Exit);

  Builder.SetInsertPoint(Left);
  auto *LeftValue = Builder.CreateShl(
      Significand,
      Builder.CreateSub(Exponent, llvm::ConstantInt::get(I64, FractionBits)));
  Builder.CreateBr(Exit);

  Builder.SetInsertPoint(Exit);
  auto *Magnitude = Builder.CreatePHI(I64, 4, "magnitude");
  Magnitude->addIncoming(llvm::ConstantInt::get(I64, 0), Entry);
  Magnitude->addIncoming(Saturated, Range);
  Magnitude->addIncoming(RightValue, Right);
  Magnitude->addIncoming(LeftValue, Left);
  if (Signed) {
    auto *SignedValue = Builder.CreateSelect(
        Negative, Builder.CreateSub(llvm::ConstantInt::get(I64, 0), Magnitude),
        Magnitude);
    Builder.CreateRet(SignedValue);
  } else {
    Builder.CreateRet(Magnitude);
  }
  return Function;
}

std::string wrapperAssembly(bool Thumb) {
  const char *Mode = Thumb ? ".thumb\n" : ".arm\n";
  const char *ThumbFunction = Thumb ? ".thumb_func\n" : "";
  std::string Assembly = ".syntax unified\n.text\n";
  Assembly += Mode;
  const std::array<std::tuple<const char *, const char *, bool, bool>, 4>
      Narrow{{
          {"__aeabi_idiv", "__wasmedge_aeabi_idivmod_core", true, true},
          {"__aeabi_uidiv", "__wasmedge_aeabi_uidivmod_core", false, true},
          {"__aeabi_idivmod", "__wasmedge_aeabi_idivmod_core", true, false},
          {"__aeabi_uidivmod", "__wasmedge_aeabi_uidivmod_core", false, false},
      }};
  for (const auto &[Wrapper, Core, Signed, QuotientOnly] : Narrow) {
    const std::string Zero = std::string(".L") + Wrapper + "_zero";
    const std::string Done = std::string(".L") + Wrapper + "_done";
    Assembly +=
        ".globl " + std::string(Wrapper) + "\n.hidden " + Wrapper + "\n.type " +
        Wrapper + ", %function\n" + ThumbFunction + Wrapper +
        ":\n.fnstart\n.cantunwind\n"
        "push {r4, lr}\nsub sp, sp, #8\nstr r0, [sp]\nstr r0, [sp, #4]\n"
        "cmp r1, #0\nbeq " +
        Zero + "\nmov r2, sp\nbl " + Core + "\nldr r0, [sp]\nb " + Done + "\n" +
        Zero + ":\n";
    if (Signed)
      Assembly += "cmp r0, #0\nbeq " + Done + "_hook\nbmi " + Done +
                  "_negative\nmvn r0, #0\nlsr r0, r0, #1\nb " + Done +
                  "_hook\n" + Done +
                  "_negative:\nmov r0, #1\nlsl r0, r0, #31\n";
    else
      Assembly += "cmp r0, #0\nbeq " + Done + "_hook\nmvn r0, #0\n";
    Assembly += Done + "_hook:\nbl __aeabi_idiv0\n" + Done + ":\n";
    if (!QuotientOnly)
      Assembly += "ldr r1, [sp, #4]\n";
    Assembly += "add sp, sp, #8\npop {r4, pc}\n.fnend\n.size " +
                std::string(Wrapper) + ", . - " + Wrapper + "\n";
  }
  const std::array<std::tuple<const char *, const char *, bool>, 2> Wide{{
      {"__aeabi_ldivmod", "__wasmedge_aeabi_ldivmod_core", true},
      {"__aeabi_uldivmod", "__wasmedge_aeabi_uldivmod_core", false},
  }};
  for (const auto &[Wrapper, Core, Signed] : Wide) {
    const std::string Zero = std::string(".L") + Wrapper + "_zero";
    const std::string Done = std::string(".L") + Wrapper + "_done";
    Assembly +=
        ".globl " + std::string(Wrapper) + "\n.hidden " + Wrapper + "\n.type " +
        Wrapper + ", %function\n" + ThumbFunction + Wrapper +
        ":\n.fnstart\n.cantunwind\n"
        "push {r4, lr}\nsub sp, sp, #24\nadd r4, sp, #8\n"
        "str r4, [sp]\nadd r4, sp, #16\nstr r4, [sp, #4]\n"
        "str r0, [sp, #16]\nstr r1, [sp, #20]\norr r4, r2, r3\n"
        "cmp r4, #0\nbeq " +
        Zero + "\nbl " + Core + "\nldr r0, [sp, #8]\nldr r1, [sp, #12]\nb " +
        Done + "\n" + Zero + ":\n";
    if (Signed)
      Assembly += "orr r4, r0, r1\ncmp r4, #0\nbeq " + Done +
                  "_hook\ncmp r1, #0\nbmi " + Done +
                  "_negative\nmvn r0, #0\nmvn r1, #0\nlsr r1, r1, #1\nb " +
                  Done + "_hook\n" + Done +
                  "_negative:\nmov r0, #0\nmov r1, #1\nlsl r1, r1, #31\n";
    else
      Assembly += "orr r4, r0, r1\ncmp r4, #0\nbeq " + Done +
                  "_hook\nmvn r0, #0\nmvn r1, #0\n";
    Assembly += Done + "_hook:\nbl __aeabi_ldiv0\n" + Done +
                ":\n"
                "ldr r2, [sp, #16]\nldr r3, [sp, #20]\nadd sp, sp, #24\n"
                "pop {r4, pc}\n.fnend\n.size " +
                Wrapper + ", . - " + Wrapper + "\n";
  }
  Assembly += ".p2align 2\n";
  return Assembly;
}

} // namespace

ARMRuntimeLibcallProfile hostARMRuntimeLibcallProfile() noexcept {
  ARMRuntimeLibcallProfile Profile{};
#if defined(__arm__)
  Profile.ARM32 = true;
#endif
#if defined(__linux__)
  Profile.Linux = true;
#endif
#if defined(__ARM_EABI__)
  Profile.EABI = true;
#endif
#if defined(__ARM_ARCH) && __ARM_ARCH >= 7
  Profile.ARMv7OrLater = true;
#endif
#if defined(__ARM_FP) && __ARM_FP != 0
  Profile.VFP = true;
  Profile.FloatABI = ARMFloatABI::SoftFP;
#else
  Profile.FloatABI = ARMFloatABI::PureSoft;
#endif
#if defined(__ARM_PCS_VFP)
  Profile.FloatABI = ARMFloatABI::Hard;
#endif
  return Profile;
}

bool supportsARMRuntimeLibcalls(
    const ARMRuntimeLibcallProfile &Profile) noexcept {
  return Profile.ARM32 && Profile.Linux && Profile.EABI &&
         Profile.ARMv7OrLater && Profile.VFP &&
         Profile.FloatABI != ARMFloatABI::PureSoft;
}

Expect<void>
addARMRuntimeLibcalls(Module &LLModule,
                      const ARMRuntimeLibcallProfile &Profile) noexcept {
  if (!supportsARMRuntimeLibcalls(Profile))
    return {};

  auto &Module = *reinterpret_cast<llvm::Module *>(LLModule.unwrap());
  auto &Context = Module.getContext();
  auto *I32 = llvm::Type::getInt32Ty(Context);
  auto *I64 = llvm::Type::getInt64Ty(Context);
  auto *IDivZero = addDivZeroHook(Module, "__aeabi_idiv0", I32);
  auto *LDivZero = addDivZeroHook(Module, "__aeabi_ldiv0", I64);
  auto *IDivMod =
      addDivModCore(Module, "__wasmedge_aeabi_idivmod_core", I32, false, true);
  auto *UIDivMod = addDivModCore(Module, "__wasmedge_aeabi_uidivmod_core", I32,
                                 false, false);
  std::vector<llvm::GlobalValue *> Used{
      IDivZero,
      LDivZero,
      IDivMod,
      UIDivMod,
      addDivModCore(Module, "__wasmedge_aeabi_ldivmod_core", I64, true, true),
      addDivModCore(Module, "__wasmedge_aeabi_uldivmod_core", I64, true, false),
  };
  const std::array<std::tuple<const char *, const char *, unsigned, bool>, 4>
      IntToFP{{
          {"__aeabi_l2f", "__wasmedge_aeabi_l2f_core", 32, true},
          {"__aeabi_ul2f", "__wasmedge_aeabi_ul2f_core", 32, false},
          {"__aeabi_l2d", "__wasmedge_aeabi_l2d_core", 64, true},
          {"__aeabi_ul2d", "__wasmedge_aeabi_ul2d_core", 64, false},
      }};
  for (const auto &[Wrapper, Core, Width, Signed] : IntToFP) {
    addIntToFPCore(Module, Core, Width, Signed);
    Used.push_back(addIntToFPCore(Module, Wrapper, Width, Signed));
  }
  const std::array<std::tuple<const char *, const char *, unsigned, bool>, 4>
      FPToInt{{
          {"__aeabi_f2lz", "__wasmedge_aeabi_f2lz_core", 32, true},
          {"__aeabi_f2ulz", "__wasmedge_aeabi_f2ulz_core", 32, false},
          {"__aeabi_d2lz", "__wasmedge_aeabi_d2lz_core", 64, true},
          {"__aeabi_d2ulz", "__wasmedge_aeabi_d2ulz_core", 64, false},
      }};
  for (const auto &[Wrapper, Core, Width, Signed] : FPToInt) {
    addFPToIntCore(Module, Core, Width, Signed);
    Used.push_back(addFPToIntCore(Module, Wrapper, Width, Signed));
  }
  const std::array<std::pair<const char *, RoundingMode>, 4> Rounding{{
      {"ceil", RoundingMode::Ceil},
      {"floor", RoundingMode::Floor},
      {"trunc", RoundingMode::Trunc},
      {"roundeven", RoundingMode::RoundEven},
  }};
  for (const unsigned Width : {32U, 64U}) {
    const std::string Suffix = Width == 32 ? "f" : "";
    for (const auto &[Name, Mode] : Rounding) {
      const std::string CoreName =
          "__wasmedge_" + std::string(Name) + Suffix + "_core";
      auto *Core = addRoundingCore(Module, CoreName, Width, Mode);
      Used.push_back(Core);
      Used.push_back(addScalarWrapper(Module, std::string(Name) + Suffix, Core,
                                      Width, Profile.FloatABI, false));
    }
    for (const auto &[Name, Minimum] :
         std::array<std::pair<const char *, bool>, 2>{
             {{"fmin", true}, {"fmax", false}}}) {
      const std::string CoreName =
          "__wasmedge_" + std::string(Name) + Suffix + "_core";
      auto *Core = addMinMaxCore(Module, CoreName, Width, Minimum);
      Used.push_back(Core);
      Used.push_back(addScalarWrapper(Module, std::string(Name) + Suffix, Core,
                                      Width, Profile.FloatABI, true));
    }
  }
  llvm::appendToCompilerUsed(Module, Used);
  LLModule.setModuleInlineAsm(wrapperAssembly(
      std::string_view(LLModule.getTarget()).find("thumb") == 0));
  return {};
}

Expect<void> validateARMRuntimeLibcallsForAOT(
    const ARMRuntimeLibcallProfile &Profile) noexcept {
  if (Profile.ARM32 && Profile.Linux && Profile.EABI && Profile.ARMv7OrLater &&
      Profile.FloatABI == ARMFloatABI::PureSoft) {
    spdlog::error(
        "ARM pure-soft float ABI is unsupported for AOT compilation."sv);
    return Unexpect(ErrCode::Value::InvalidAOTConfigure);
  }
  return {};
}

} // namespace LLVM
} // namespace WasmEdge
