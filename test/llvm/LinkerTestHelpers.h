// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "LinkerTestUtils.h"
#include "linker/eh_frame.h"
#include "linker/link_graph.h"

#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <gtest/gtest.h>

#include <llvm/ADT/StringExtras.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#if LLVM_VERSION_MAJOR < 14
#include <llvm/Support/TargetRegistry.h>
#else
#include <llvm/MC/TargetRegistry.h>
#endif
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/CodeGen.h>
#if LLVM_VERSION_MAJOR < 16
#include <llvm/Support/Host.h>
#else
#include <llvm/TargetParser/Host.h>
#endif
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Test {

using namespace Linker;

struct ObjectArchitectureCase {
  const char *Triple;
  Target Architecture;
};

[[maybe_unused]] inline std::string objectArchitectureCaseName(
    const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
  return architectureName(Info.param.Architecture);
}

inline llvm::TargetOptions targetOptions() {
  llvm::TargetOptions Options;
#if LLVM_VERSION_MAJOR < 16
  Options.RelaxELFRelocations = true;
#endif
  return Options;
}

inline std::string targetCPU(const llvm::Triple &Triple, std::string CPU) {
#if LLVM_VERSION_MAJOR < 15
  if (CPU == "generic" && Triple.getArch() == llvm::Triple::riscv64) {
    return "generic-rv64";
  }
#else
  static_cast<void>(Triple);
#endif
  return CPU;
}

inline std::vector<WasmEdge::Byte> makeObject(
    const llvm::Triple &Triple, bool Undefined = false, bool DLLExport = false,
    std::string FunctionName = "f0", std::string Directives = {},
    bool Hidden = false, bool HiddenData = false, std::string CPU = "generic",
    std::string Features = {}, bool UnwindTable = false, bool Optimize = false,
    bool Interruptible = false, bool Atomic = false,
    bool Representative = false, bool Exceptions = false,
    std::string ModuleAssembly = {}, bool SemanticSymbols = false,
    bool TypeWrapper = false, bool FloatingPoint = false,
    bool DefineFltused = false, bool UnusedAllocatableSections = false,
    bool DistinctTypeWrapperUnwind = false, bool UnusedFunctionUnwind = false) {
  std::string Error;
  const llvm::Target *NativeTarget = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(NativeTarget, nullptr) << Error;
  if (NativeTarget == nullptr) {
    return {};
  }
  llvm::TargetOptions Options = targetOptions();
  std::unique_ptr<llvm::TargetMachine> Machine(
      NativeTarget->createTargetMachine(
#if LLVM_VERSION_MAJOR < 21
          Triple.str(),
#else
          Triple,
#endif
          targetCPU(Triple, CPU), Features, Options, llvm::Reloc::PIC_));
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr) {
    return {};
  }
#if LLVM_VERSION_MAJOR < 18
  Machine->setOptLevel(Optimize ? llvm::CodeGenOpt::Default
                                : llvm::CodeGenOpt::None);
#else
  Machine->setOptLevel(Optimize ? llvm::CodeGenOptLevel::Default
                                : llvm::CodeGenOptLevel::None);
#endif

  llvm::LLVMContext Context;
  llvm::Module Module("object-reader-test", Context);
#if LLVM_VERSION_MAJOR < 21
  Module.setTargetTriple(Triple.str());
#else
  Module.setTargetTriple(Triple);
#endif
  Module.setDataLayout(Machine->createDataLayout());
  Module.setModuleInlineAsm(ModuleAssembly);
  auto *I32 = llvm::Type::getInt32Ty(Context);
  if (DefineFltused)
    new llvm::GlobalVariable(Module, I32, false,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(I32, 0), "_fltused");
  auto *Value = new llvm::GlobalVariable(
      Module, I32, false, llvm::GlobalValue::ExternalLinkage,
      Undefined ? nullptr : llvm::ConstantInt::get(I32, 7), "value");
  if (SemanticSymbols) {
    new llvm::GlobalVariable(Module, I32, true,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(I32, 1), "version");
    new llvm::GlobalVariable(Module, I32, true,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(I32, 2), "intrinsics");
    new llvm::GlobalVariable(
        Module, llvm::ArrayType::get(llvm::Type::getInt8Ty(Context), 1), true,
        llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantDataArray::get(Context, llvm::ArrayRef<uint8_t>{0}),
        "wasm.code");
    new llvm::GlobalVariable(Module, I32, true,
                             llvm::GlobalValue::ExternalLinkage,
                             llvm::ConstantInt::get(I32, 1), "wasm.size");
  }
  if (HiddenData) {
    Value->setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  auto *Zero = new llvm::GlobalVariable(Module, I32, false,
                                        llvm::GlobalValue::InternalLinkage,
                                        llvm::ConstantInt::get(I32, 0), "zero");
  Zero->setAlignment(alignment(16));
  if (UnusedAllocatableSections) {
    auto *UnusedData = new llvm::GlobalVariable(
        Module, I32, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(I32, UINT32_C(0x6E6F6367)), "unused_data");
    UnusedData->setVisibility(llvm::GlobalValue::HiddenVisibility);
    UnusedData->setSection(Triple.isOSBinFormatMachO()  ? "__TEXT,__unused_data"
                           : Triple.isOSBinFormatCOFF() ? ".rdata$unused"
                                                        : ".data.unused");
    UnusedData->setAlignment(alignment(4));
  }
  auto *F0 = llvm::Function::Create(llvm::FunctionType::get(I32, false),
                                    llvm::GlobalValue::ExternalLinkage,
                                    FunctionName, Module);
  F0->addFnAttr(llvm::Attribute::NoUnwind);
  if (UnwindTable) {
#if LLVM_VERSION_MAJOR < 15
    F0->addFnAttr(llvm::Attribute::UWTable);
#else
    F0->setUWTableKind(llvm::UWTableKind::Sync);
#endif
  }
  if (TypeWrapper) {
    auto *T0 = llvm::Function::Create(llvm::FunctionType::get(I32, false),
                                      llvm::GlobalValue::ExternalLinkage, "t0",
                                      Module);
#if LLVM_VERSION_MAJOR < 15
    T0->addFnAttr(llvm::Attribute::UWTable);
#else
    T0->setUWTableKind(llvm::UWTableKind::Sync);
#endif
    llvm::IRBuilder<> WrapperBuilder(
        llvm::BasicBlock::Create(Context, "entry", T0));
    if (DistinctTypeWrapperUnwind) {
      auto *Slot = WrapperBuilder.CreateAlloca(I32);
      WrapperBuilder.CreateStore(llvm::ConstantInt::get(I32, 0), Slot, true);
      WrapperBuilder.CreateRet(WrapperBuilder.CreateLoad(I32, Slot, true));
    } else {
      WrapperBuilder.CreateRet(llvm::ConstantInt::get(I32, 0));
    }
  }
  if (UnusedAllocatableSections) {
    auto *UnusedFunction = llvm::Function::Create(
        llvm::FunctionType::get(I32, false), llvm::GlobalValue::ExternalLinkage,
        "unused_function", Module);
    UnusedFunction->setVisibility(llvm::GlobalValue::HiddenVisibility);
    UnusedFunction->setSection(Triple.isOSBinFormatMachO()
                                   ? "__TEXT,__unused_text"
                               : Triple.isOSBinFormatCOFF() ? ".text$unused"
                                                            : ".text.unused");
    UnusedFunction->addFnAttr(llvm::Attribute::NoInline);
    if (UnusedFunctionUnwind) {
#if LLVM_VERSION_MAJOR < 15
      UnusedFunction->addFnAttr(llvm::Attribute::UWTable);
#else
      UnusedFunction->setUWTableKind(llvm::UWTableKind::Sync);
#endif
    } else {
      UnusedFunction->addFnAttr(llvm::Attribute::NoUnwind);
    }
    llvm::IRBuilder<> UnusedBuilder(
        llvm::BasicBlock::Create(Context, "entry", UnusedFunction));
    UnusedBuilder.CreateRet(llvm::ConstantInt::get(I32, UINT32_C(0x5A17)));
  }
  F0->setVisibility(Hidden ? llvm::GlobalValue::HiddenVisibility
                           : llvm::GlobalValue::DefaultVisibility);
  if (DLLExport) {
    F0->setDLLStorageClass(llvm::GlobalValue::DLLExportStorageClass);
  }
  llvm::IRBuilder<> Builder(llvm::BasicBlock::Create(Context, "entry", F0));
  auto *Loaded = Builder.CreateLoad(I32, Value);
  if (Atomic) {
    Loaded->setAtomic(llvm::AtomicOrdering::SequentiallyConsistent);
    Loaded->setAlignment(alignment(4));
  }
  llvm::Value *Result = Loaded;
  if (FloatingPoint) {
    auto *Double = Builder.CreateSIToFP(Result, Builder.getDoubleTy());
    Double = Builder.CreateFAdd(
        Double, llvm::ConstantFP::get(Builder.getDoubleTy(), 0.5));
    Result = Builder.CreateFPToSI(Double, I32);
  }
  if (Representative) {
    constexpr uint64_t LinearMemorySize = 64;
    constexpr unsigned VectorLanes = 4;
    auto *MemoryType =
        llvm::ArrayType::get(llvm::Type::getInt8Ty(Context), LinearMemorySize);
    auto *Memory = new llvm::GlobalVariable(
        Module, MemoryType, false, llvm::GlobalValue::InternalLinkage,
        llvm::ConstantAggregateZero::get(MemoryType), "memory");
    Memory->setAlignment(alignment(16));
    auto *MemoryAddress = Builder.CreateInBoundsGEP(
        MemoryType, Memory,
        {llvm::ConstantInt::get(I32, 0), llvm::ConstantInt::get(I32, 8)});
    auto *MemoryValue =
        Builder.CreateLoad(llvm::Type::getInt8Ty(Context), MemoryAddress, true);
    Builder.CreateStore(MemoryValue, MemoryAddress, true);

    auto *VectorType =
#if LLVM_VERSION_MAJOR < 11
        llvm::VectorType::get(I32, VectorLanes);
#else
        llvm::VectorType::get(I32, VectorLanes, false);
#endif
    auto *Vector = new llvm::GlobalVariable(
        Module, VectorType, false, llvm::GlobalValue::InternalLinkage,
        llvm::ConstantAggregateZero::get(VectorType), "vector");
    Vector->setAlignment(alignment(16));
    auto *VectorValue = Builder.CreateLoad(VectorType, Vector, true);
    auto *VectorResult = Builder.CreateAdd(VectorValue, VectorValue);
    Builder.CreateStore(VectorResult, Vector, true);

    auto *Direct = llvm::Function::Create(llvm::FunctionType::get(I32, false),
                                          llvm::GlobalValue::InternalLinkage,
                                          "direct", Module);
    Direct->addFnAttr(llvm::Attribute::NoUnwind);
    Direct->addFnAttr(llvm::Attribute::NoInline);
    Direct->setSection(Triple.isOSBinFormatMachO() ? "__TEXT,__text"
                                                   : ".text.direct");
    llvm::IRBuilder<> DirectBuilder(
        llvm::BasicBlock::Create(Context, "entry", Direct));
    DirectBuilder.CreateRet(llvm::ConstantInt::get(I32, 3));
    auto *Table = new llvm::GlobalVariable(Module, Direct->getType(), true,
                                           llvm::GlobalValue::InternalLinkage,
                                           Direct, "table");
    auto *Indirect = Builder.CreateLoad(Direct->getType(), Table, true);
    Result = Builder.CreateAdd(Result, Builder.CreateCall(Direct));
    Result = Builder.CreateAdd(
        Result, Builder.CreateCall(Direct->getFunctionType(), Indirect));
  }
  if (Interruptible) {
    auto *Poll = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getVoidTy(Context), false),
        llvm::GlobalValue::InternalLinkage, "poll", Module);
    Poll->addFnAttr(llvm::Attribute::NoUnwind);
    Poll->setSection(Triple.isOSBinFormatMachO() ? "__TEXT,__text"
                                                 : ".text.poll");
    llvm::IRBuilder<> PollBuilder(
        llvm::BasicBlock::Create(Context, "entry", Poll));
    PollBuilder.CreateRetVoid();
    Builder.CreateCall(Poll);
  }
  if (Exceptions) {
    F0->removeFnAttr(llvm::Attribute::NoUnwind);
    auto *PersonalityType = llvm::FunctionType::get(I32, true);
    auto *Personality = llvm::Function::Create(
        PersonalityType, llvm::GlobalValue::InternalLinkage, "personality",
        Module);
    llvm::IRBuilder<> PersonalityBuilder(
        llvm::BasicBlock::Create(Context, "entry", Personality));
    PersonalityBuilder.CreateRet(llvm::ConstantInt::get(I32, 0));
    F0->setPersonalityFn(Personality);
    auto *MayThrow = llvm::Function::Create(
        llvm::FunctionType::get(llvm::Type::getVoidTy(Context), false),
        llvm::GlobalValue::InternalLinkage, "may_throw", Module);
    MayThrow->addFnAttr(llvm::Attribute::NoInline);
    llvm::IRBuilder<> ThrowBuilder(
        llvm::BasicBlock::Create(Context, "entry", MayThrow));
    ThrowBuilder.CreateStore(llvm::ConstantInt::get(I32, 1), Value, true);
    ThrowBuilder.CreateRetVoid();
    auto *Normal = llvm::BasicBlock::Create(Context, "normal", F0);
    auto *Unwind = llvm::BasicBlock::Create(Context, "unwind", F0);
    Builder.CreateInvoke(MayThrow, Normal, Unwind);
    Builder.SetInsertPoint(Normal);
    Builder.CreateRet(Result);
    Builder.SetInsertPoint(Unwind);
    auto *LandingPadType = llvm::StructType::get(
#if LLVM_VERSION_MAJOR < 15
        llvm::Type::getInt8PtrTy(Context), I32);
#else
        llvm::PointerType::getUnqual(Context), I32);
#endif
    auto *LandingPad = Builder.CreateLandingPad(LandingPadType, 0);
    LandingPad->setCleanup(true);
    Builder.CreateRet(Result);
  } else {
    Builder.CreateRet(Result);
  }
  if (!Directives.empty()) {
    Module.setModuleInlineAsm(".section .drectve\n.ascii \" " + Directives +
                              "\"");
  }

  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
  const auto FileType = ObjectFileType;
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<WasmEdge::Byte>(Storage.begin(), Storage.end());
}

inline uint64_t read64le(const std::vector<WasmEdge::Byte> &Bytes,
                         size_t Offset) {
  uint64_t Value = 0;
  for (size_t I = 0; I < 8; ++I) {
    Value |= static_cast<uint64_t>(Bytes[Offset + I]) << (I * 8);
  }
  return Value;
}

inline uint32_t read32le(const std::vector<WasmEdge::Byte> &Bytes,
                         size_t Offset) {
  uint32_t Value = 0;
  for (size_t I = 0; I < 4; ++I) {
    Value |= static_cast<uint32_t>(Bytes[Offset + I]) << (I * 8);
  }
  return Value;
}

struct RawCompactUnwindRecord {
  uint64_t Function;
  uint32_t Length;
  uint32_t Encoding;
  uint64_t Personality;
  uint64_t LSDA;
};

static_assert(sizeof(RawCompactUnwindRecord) == 32);

struct RawCompactUnwindRelocation {
  uint64_t Offset;
  uint64_t Type;
  std::string Symbol;
};

struct RawCompactUnwindInventory {
  std::vector<RawCompactUnwindRecord> Records;
  std::vector<RawCompactUnwindRelocation> Relocations;
  bool HasCompactUnwind = false;
  bool HasEHFrame = false;
};

inline RawCompactUnwindInventory
collectCompactUnwindInventory(const std::vector<WasmEdge::Byte> &Bytes) {
  RawCompactUnwindInventory Result;
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return Result;
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return Result;
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name)
      continue;
    Result.HasEHFrame |= *Name == "__eh_frame";
    if (*Name != "__compact_unwind")
      continue;
    Result.HasCompactUnwind = true;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      continue;
    const std::vector<WasmEdge::Byte> Content(Contents->bytes_begin(),
                                              Contents->bytes_end());
    EXPECT_EQ(Content.size() % sizeof(RawCompactUnwindRecord), 0U);
    if (Content.size() % sizeof(RawCompactUnwindRecord) != 0)
      continue;
    for (size_t Offset = 0; Offset < Content.size();
         Offset += sizeof(RawCompactUnwindRecord)) {
      Result.Records.push_back(
          {read64le(Content, Offset), read32le(Content, Offset + 8),
           read32le(Content, Offset + 12), read64le(Content, Offset + 16),
           read64le(Content, Offset + 24)});
    }
    for (const auto &Relocation : Section.relocations()) {
      std::string SymbolName;
      const auto Symbol = Relocation.getSymbol();
      if (Symbol != MachO->symbol_end()) {
        auto RelocationName = Symbol->getName();
        EXPECT_TRUE(static_cast<bool>(RelocationName));
        if (RelocationName)
          SymbolName = RelocationName->str();
      }
      Result.Relocations.push_back(
          {Relocation.getOffset(), Relocation.getType(), SymbolName});
    }
  }
  return Result;
}

inline void write64le(std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                      uint64_t Value) {
  for (size_t I = 0; I < 8; ++I) {
    Bytes[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
  }
}

inline void write32le(std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                      uint32_t Value) {
  for (size_t I = 0; I < 4; ++I) {
    Bytes[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
  }
}

struct MachOEHFrameObject {
  size_t Content = 0;
  std::vector<size_t> Relocations;
};

inline MachOEHFrameObject
machOEHFrameObject(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "eh-frame.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {};
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return {};
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {};
    MachOEHFrameObject Result;
    Result.Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations())
      Result.Relocations.push_back(Header.reloff +
                                   Relocation.getRawDataRefImpl().d.b * 8);
    return Result;
  }
  ADD_FAILURE() << "generated object has no EH frame section";
  return {};
}

inline std::vector<WasmEdge::Byte> makeAArch64MachOEHFrameObject() {
  constexpr std::string_view Frame = R"(
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lcie:
.long Lcie_end-Lcie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Lcie_end:
Lfde:
.long Lfde_end-Lfde-4
.long Lfde+4-Lcie
.quad _f0-.
.quad 4
.byte 0
.p2align 3
Lfde_end:
.long 0
.text
)";
  return makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                    false, false, "generic", {}, false, false, false, false,
                    false, false, std::string(Frame));
}

struct X86_64MachOEHFrameFixture {
  std::vector<WasmEdge::Byte> Bytes;
  size_t Field = 0;
  uint64_t FieldAddress = 0;
  uint64_t DataAddress = 0;
};

inline void clearMachOSectionRelocations(std::vector<WasmEdge::Byte> &Bytes,
                                         std::string_view Name);
inline std::vector<WasmEdge::Byte>
makeAssemblyObject(const llvm::Triple &Triple, std::string Assembly,
                   std::string Features, bool HardFloat);

inline X86_64MachOEHFrameFixture
makeX86_64MachOEHFrameFixture(bool Raw, bool Alias = false,
                              bool Mixed = false) {
  std::string Assembly = Mixed ? R"(
.text
.globl _t0
_t0:
ret
.globl _f0
_f0:
ret
)"
                               : R"(
.text
.globl _f0
_f0:
ret
.globl _f1
_f1:
ret
)";
  if (Alias && !Mixed) {
    Assembly += R"(
.globl _f1_alias
.set _f1_alias, _f1
)";
  }
  Assembly += R"(
.data
.globl _d0
_d0:
.quad 0
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lx86_cie:
.long Lx86_cie_end-Lx86_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 1
.byte 0x78
.byte 16
.byte 1
.byte 0x10
.p2align 3
Lx86_cie_end:
Lx86_fde:
.long Lx86_fde_end-Lx86_fde-4
.long 28
)";
  Assembly += Mixed ? ".quad _f0\n" : ".quad _f1\n";
  Assembly += R"(
.quad 1
.byte 0
.p2align 3
Lx86_fde_end:
.long 0
)";
  if (Mixed) {
    Assembly += R"(
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _t0
.long 1
.long 0x01000000
.quad 0
.quad 0
)";
  }
  auto Bytes = makeAssemblyObject(llvm::Triple("x86_64-apple-macosx"),
                                  std::move(Assembly), {}, false);
  const auto EH = machOEHFrameObject(Bytes);
  EXPECT_NE(EH.Content, 0U);
  EXPECT_EQ(EH.Relocations.size(), 1U);

  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "x86-eh-frame.o"));
  EXPECT_TRUE(static_cast<bool>(Parsed));
  if (!Parsed) {
    llvm::consumeError(Parsed.takeError());
    return {std::move(Bytes), 0, 0, 0};
  }
  uint64_t EHAddress = 0;
  uint64_t TargetAddress = 0;
  uint64_t DataAddress = 0;
  std::set<size_t> Fields;
  std::set<uint64_t> Relocations;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    EHAddress = Section.getAddress();
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      continue;
    const std::vector<WasmEdge::Byte> Frame(Contents->bytes_begin(),
                                            Contents->bytes_end());
    auto ParsedFields = machOEHFrameFields(Frame, Target::X86_64);
    EXPECT_TRUE(ParsedFields.has_value());
    if (ParsedFields)
      Fields = std::move(*ParsedFields);
    for (const auto &Relocation : Section.relocations())
      Relocations.insert(Relocation.getOffset());
  }
  for (const auto &Symbol : (*Parsed)->symbols()) {
    auto Name = Symbol.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || (*Name != (Mixed ? "_f0" : "_f1") && *Name != "_d0"))
      continue;
    auto Address = Symbol.getAddress();
    EXPECT_TRUE(static_cast<bool>(Address));
    if (!Address)
      continue;
    if (*Name == "_d0")
      DataAddress = *Address;
    else
      TargetAddress = *Address;
  }
  EXPECT_EQ(Fields.size(), 1U);
  EXPECT_NE(TargetAddress, 0U);
  EXPECT_NE(DataAddress, 0U);
  const size_t Field = Fields.empty() ? 0 : *Fields.begin();
  EXPECT_NE(Relocations.find(Field), Relocations.end());
  if (Raw && Fields.size() == 1 && TargetAddress != 0) {
    const int64_t Delta = static_cast<int64_t>(TargetAddress) -
                          static_cast<int64_t>(EHAddress + Field);
    write64le(Bytes, EH.Content + Field, static_cast<uint64_t>(Delta));
    clearMachOSectionRelocations(Bytes, "__eh_frame");
  }
  return {std::move(Bytes), Field, EHAddress + Field, DataAddress};
}

inline void clearMachOSectionRelocations(std::vector<WasmEdge::Byte> &Bytes,
                                         std::string_view Name) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  constexpr size_t SectionRelocationCountOffset = 60;
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  for (uint32_t I = 0; I < CommandCount; ++I) {
    const uint32_t Command = read32le(Bytes, CommandOffset);
    const uint32_t CommandSize = read32le(Bytes, CommandOffset + 4);
    if (Command == llvm::MachO::LC_SEGMENT_64) {
      const uint32_t SectionCount = read32le(Bytes, CommandOffset + 64);
      for (uint32_t J = 0; J < SectionCount; ++J) {
        const size_t SectionOffset =
            CommandOffset + SegmentCommandSize + J * SectionSize;
        const std::string_view SectionName(
            reinterpret_cast<const char *>(Bytes.data() + SectionOffset), 16);
        if (SectionName.substr(0, SectionName.find('\0')) == Name)
          write32le(Bytes, SectionOffset + SectionRelocationCountOffset, 0);
      }
    }
    CommandOffset += CommandSize;
  }
}

inline size_t
elf64RelocationSectionHeader(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "test.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return 0;
  }
  uint64_t Index = 0;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (Name &&
#if LLVM_VERSION_MAJOR < 16
        (Name->startswith(".rela") || Name->startswith(".rel"))) {
#else
        (Name->starts_with(".rela") || Name->starts_with(".rel"))) {
#endif
      Index = Section.getIndex();
      break;
    }
  }
  EXPECT_NE(Index, 0U);
  return static_cast<size_t>(read64le(Bytes, 40) + Index * 64);
}

inline std::vector<WasmEdge::Byte> makeNativeObject(bool Undefined = false) {
  return makeObject(llvm::Triple(llvm::Triple::normalize(
                        llvm::sys::getDefaultTargetTriple())),
                    Undefined);
}

inline std::vector<WasmEdge::Byte>
makeSemanticObject(const llvm::Triple &Triple, bool Undefined = false) {
  return makeObject(Triple, Undefined, false, "f0", {}, false, false, "generic",
                    {}, false, false, false, false, false, false, {}, true);
}

inline std::vector<WasmEdge::Byte>
makeAssemblyObject(const llvm::Triple &Triple, std::string Assembly,
                   std::string Features = {}, bool HardFloat = false) {
  std::string Error;
  const llvm::Target *Target = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(Target, nullptr) << Error;
  if (Target == nullptr) {
    return {};
  }
  llvm::TargetOptions Options = targetOptions();
  if (HardFloat) {
    Options.FloatABIType = llvm::FloatABI::Hard;
    Options.MCOptions.ABIName = "aapcs-vfp";
  }
#if LLVM_VERSION_MAJOR < 15
  if (Triple.getArch() == llvm::Triple::riscv64 &&
      Features.find("+d") != std::string::npos) {
    Options.MCOptions.ABIName = "lp64d";
  }
#endif
  std::unique_ptr<llvm::TargetMachine> Machine(Target->createTargetMachine(
#if LLVM_VERSION_MAJOR < 21
      Triple.str(),
#else
      Triple,
#endif
      targetCPU(Triple, "generic"), Features, Options, llvm::Reloc::PIC_));
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr) {
    return {};
  }
  llvm::LLVMContext Context;
  llvm::Module Module("x86-relocation-test", Context);
#if LLVM_VERSION_MAJOR < 21
  Module.setTargetTriple(Triple.str());
#else
  Module.setTargetTriple(Triple);
#endif
  Module.setDataLayout(Machine->createDataLayout());
  Module.setModuleInlineAsm(std::move(Assembly));
  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
  const auto FileType = ObjectFileType;
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<WasmEdge::Byte>(Storage.begin(), Storage.end());
}

inline std::vector<WasmEdge::Byte> makeX86_64ELFReaderFixture() {
  constexpr std::string_view Hex =
      "7f454c4602010100000000000000000001003e0001000000000000000000000000000000"
      "00000000500100000000000000000000400000000000400008000100"
      "488d05000000008b00c30000070000001400000000000000017a5200017810011b0c0708"
      "90010000140000001c000000000000000a0000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000300020000000000"
      "00000000000000000000000037000000120002000000000000000000"
      "0a000000000000000c000000110004000000000000000000040000000000000003000000"
      "000000000200000003000000fcffffffffffffff2000000000000000"
      "02000000010000000000000000000000002e72656c612e746578740076616c7565002e72"
      "656c612e65685f6672616d65002e737472746162002e73796d746162"
      "002e64617461006630000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000002100000003000000000000000000000000000000"
      "0000000010010000000000003a000000000000000000000000000000"
      "010000000000000000000000000000000600000001000000060000000000000000000000"
      "0000000040000000000000000a000000000000000000000000000000"
      "040000000000000000000000000000000100000004000000000000000000000000000000"
      "00000000e00000000000000018000000000000000700000002000000"
      "080000000000000018000000000000003100000001000000030000000000000000000000"
      "000000004c0000000000000004000000000000000000000000000000"
      "040000000000000000000000000000001700000001000070020000000000000000000000"
      "00000000500000000000000030000000000000000000000000000000"
      "080000000000000000000000000000001200000004000000000000000000000000000000"
      "00000000f80000000000000018000000000000000700000005000000"
      "080000000000000018000000000000002900000002000000000000000000000000000000"
      "00000000800000000000000060000000000000000100000002000000"
      "08000000000000001800000000000000";
  const std::string Bytes =
      llvm::fromHex(llvm::StringRef(Hex.data(), Hex.size()));
  return std::vector<WasmEdge::Byte>(Bytes.begin(), Bytes.end());
}

class LinkerOutputTest : public testing::Test {
protected:
  void SetUp() override {
    TempDirectory.emplace("WasmEdgeLinker-");
    if (std::getenv("WASMEDGE_KEEP_LINKER_FIXTURE") != nullptr)
      TempDirectory->keep();
    Directory = TempDirectory->path();
  }

  void TearDown() override { TempDirectory.reset(); }

  std::vector<WasmEdge::Byte>
  readFile(const std::filesystem::path &Path) const {
    std::ifstream Input(Path, std::ios_base::binary | std::ios_base::ate);
    EXPECT_TRUE(Input);
    const auto Size = Input.tellg();
    EXPECT_GE(Size, 0);
    std::vector<WasmEdge::Byte> Result(static_cast<size_t>(Size));
    Input.seekg(0);
    EXPECT_TRUE(Input.read(reinterpret_cast<char *>(Result.data()), Size));
    return Result;
  }

  void expectNoTemporaryFiles() const {
    for (const auto &Entry : std::filesystem::directory_iterator(Directory)) {
      EXPECT_EQ(Entry.path().extension(), ".wasm");
    }
  }

  std::vector<WasmEdge::Byte>
  compileTinyObject(WasmEdge::Span<const WasmEdge::Byte> Wasm,
                    const std::filesystem::path &Output,
                    bool Native = false) const {
    WasmEdge::Configure Conf;
    Conf.getCompilerConfigure().setOutputFormat(
        Native ? WasmEdge::CompilerConfigure::OutputFormat::Native
               : WasmEdge::CompilerConfigure::OutputFormat::Wasm);
    Conf.getCompilerConfigure().setDumpIR(true);
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator Validator(Conf);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    WasmEdge::LLVM::CodeGen CodeGen(Conf);
    auto Module = Loader.parseModule(Wasm);
    EXPECT_TRUE(Module);
    if (!Module) {
      return {};
    }
    EXPECT_TRUE(Validator.validate(**Module));
    auto Data =
        Compiler.compile(**Module, WasmEdge::LLVM::Compiler::Consumer::CodeGen);
    EXPECT_TRUE(Data);
    if (!Data) {
      return {};
    }
    {
      ScopedCurrentPath CurrentPath(Directory);
      EXPECT_TRUE(CodeGen.codegen(Wasm, std::move(*Data), Output));
    }
    auto Object = readFile(Directory / "wasm.o");
    std::filesystem::remove(Directory / "wasm.o");
    std::filesystem::remove(Directory / "wasm.ll");
    std::filesystem::remove(Directory / "wasm-opt.ll");
    if (Native)
      std::filesystem::remove(Output);
    return Object;
  }

  uint32_t execute(const std::filesystem::path &Path) const {
    WasmEdge::Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
    WasmEdge::VM::VM VM(Conf);
    EXPECT_TRUE(VM.loadWasm(Path));
    EXPECT_TRUE(VM.validate());
    EXPECT_TRUE(VM.instantiate());
    auto Result = VM.execute("f");
    EXPECT_TRUE(Result);
    if (!Result || Result->size() != 1) {
      return 0;
    }
    return (*Result)[0].first.get<uint32_t>();
  }

  std::optional<ScopedTempDirectory> TempDirectory;
  std::filesystem::path Directory;
};

inline void expectGraphStateEquals(const LinkGraph &Actual,
                                   const LinkGraph &Expected) {
  EXPECT_EQ(Actual.target(), Expected.target());
  EXPECT_EQ(Actual.endianness(), Expected.endianness());
  EXPECT_EQ(Actual.relocationsApplied(), Expected.relocationsApplied());
  ASSERT_EQ(Actual.sections().size(), Expected.sections().size());
  for (size_t I = 0; I < Actual.sections().size(); ++I) {
    const auto &Left = Actual.sections()[I];
    const auto &Right = Expected.sections()[I];
    EXPECT_EQ(Left.Name, Right.Name);
    EXPECT_EQ(Left.Kind, Right.Kind);
    EXPECT_EQ(Left.Alignment, Right.Alignment);
    EXPECT_EQ(Left.VirtualSize, Right.VirtualSize);
    EXPECT_EQ(Left.Address, Right.Address);
    EXPECT_EQ(Left.FileOffset, Right.FileOffset);
    EXPECT_EQ(Left.Content, Right.Content);
  }
  ASSERT_EQ(Actual.symbols().size(), Expected.symbols().size());
  for (size_t I = 0; I < Actual.symbols().size(); ++I) {
    const auto &Left = Actual.symbols()[I];
    const auto &Right = Expected.symbols()[I];
    EXPECT_EQ(Left.Name, Right.Name);
    EXPECT_EQ(Left.Section, Right.Section);
    EXPECT_EQ(Left.Offset, Right.Offset);
    EXPECT_EQ(Left.Size, Right.Size);
    EXPECT_EQ(Left.Exported, Right.Exported);
    EXPECT_EQ(Left.ExportName, Right.ExportName);
  }
  ASSERT_EQ(Actual.relocations().size(), Expected.relocations().size());
  for (size_t I = 0; I < Actual.relocations().size(); ++I) {
    const auto &Left = Actual.relocations()[I];
    const auto &Right = Expected.relocations()[I];
    EXPECT_EQ(Left.Section, Right.Section);
    EXPECT_EQ(Left.Offset, Right.Offset);
    EXPECT_EQ(Left.Type, Right.Type);
    EXPECT_EQ(Left.Symbol, Right.Symbol);
    EXPECT_EQ(Left.Addend, Right.Addend);
    EXPECT_EQ(Left.AddendIsImplicit, Right.AddendIsImplicit);
    EXPECT_EQ(Left.Format, Right.Format);
    EXPECT_EQ(Left.PatchSize, Right.PatchSize);
  }
  ASSERT_EQ(Actual.rebases().size(), Expected.rebases().size());
  for (size_t I = 0; I < Actual.rebases().size(); ++I) {
    const auto &Left = Actual.rebases()[I];
    const auto &Right = Expected.rebases()[I];
    EXPECT_EQ(Left.Section, Right.Section);
    EXPECT_EQ(Left.Offset, Right.Offset);
    EXPECT_EQ(Left.Type, Right.Type);
    EXPECT_EQ(Left.Addend, Right.Addend);
    EXPECT_EQ(Left.Width, Right.Width);
    EXPECT_EQ(Left.Format, Right.Format);
  }
}

inline uint64_t decodeAdrpTargetPage(uint32_t Instruction,
                                     uint64_t PatchAddress) {
  constexpr uint64_t PageSize = UINT64_C(1) << 12;
  const uint32_t Raw =
      ((Instruction >> 29) & 3) | ((Instruction >> 3) & UINT32_C(0x1FFFFC));
  const int64_t Pages = Raw >= (UINT32_C(1) << 20)
                            ? static_cast<int64_t>(Raw) - (INT64_C(1) << 21)
                            : static_cast<int64_t>(Raw);
  const uint64_t PatchPage = PatchAddress & ~(PageSize - 1);
  const uint64_t Magnitude =
      static_cast<uint64_t>(Pages < 0 ? -(Pages + 1) : Pages) +
      static_cast<uint64_t>(Pages < 0);
  return Pages < 0 ? PatchPage - Magnitude * PageSize
                   : PatchPage + Magnitude * PageSize;
}

inline LinkGraph makeCompactUnwindGraph(
    Target Architecture, uint32_t Encoding, std::string Name = "_f0",
    std::vector<WasmEdge::Byte> Content = std::vector<WasmEdge::Byte>(64)) {
  LinkGraph Graph(Architecture, Endianness::Little, ObjectFormat::MachO);
  EXPECT_TRUE(Graph.beginInput("compact.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 4,
                                       Content.size(), 0, 0, std::move(Content),
                                       SectionPurpose::Default, 0x1000});
  EXPECT_TRUE(Text);
  auto Function =
      Graph.addSymbol(Symbol{std::move(Name), *Text, 0, 16, true, {}, true});
  EXPECT_TRUE(Function);
  EXPECT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Function, 16, Encoding, {}, {}, {}}));
  return Graph;
}

} // namespace Test
} // namespace LLVM
} // namespace WasmEdge
