// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include <llvm/Support/JSON.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/elf_writer.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/macho_writer.h"
#include "linker/native_linker.h"
#include "linker/object_reader.h"
#include "linker/pe_writer.h"
#include "linker/relocation.h"
#include "linker/universal_wasm_writer.h"
#include "linker/writer.h"

#include "aot/version.h"
#include "common/hexstr.h"
#include "common/spdlog.h"
#include "loader/loader.h"
#include "loader/shared_library.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <gtest/gtest.h>

#include <spdlog/sinks/ostream_sink.h>

#include <llvm/BinaryFormat/COFF.h>
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
#if LLVM_VERSION_MAJOR >= 13
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif
#if LLVM_VERSION_MAJOR >= 19
#include <llvm/MC/MCELFExtras.h>
#endif
#include <llvm/Object/COFF.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/CodeGen.h>
#if LLVM_VERSION_MAJOR >= 16
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace {

using namespace WasmEdge::LLVM::Linker;

TEST(LinkerHostTest, UsesAOTArchitectureContract) {
#if defined(__x86_64__) || defined(_M_X64)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::X86_64);
#elif defined(__aarch64__) || defined(_M_ARM64)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::AArch64);
#elif defined(__riscv) && __riscv_xlen == 64
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::RISCV64);
#elif defined(__arm__) && __ARM_ARCH == 7
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::ARMv7);
#elif defined(_M_ARM) && _M_ARM == 7
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::ARMv7);
#elif defined(__s390x__)
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::S390X);
#else
  EXPECT_EQ(WasmEdge::AOT::kHostArchitecture,
            WasmEdge::AOT::Architecture::Unsupported);
#endif
}

struct ObjectArchitectureCase {
  const char *Triple;
  Target Architecture;
};

struct LinuxRelocationCase {
  const char *Triple;
  Target Architecture;
  const char *TunedCPU;
  const char *Features;
  std::set<uint64_t> SupportedTypes;
  std::set<uint64_t> RequiredTypes;
};

struct FailingRelocationCase {
  const char *Triple;
  Target Architecture;
  Endianness WrongEndian;
  uint32_t AbsoluteType;
  uint8_t Width;
  const char *Features;
};

struct PortableRelocationCase {
  const char *Triple;
  Target Architecture;
  ObjectFormat Format;
  uint32_t Type;
};

struct PortableObjectCase {
  const char *Triple;
  Target Architecture;
  std::set<uint32_t> Types;
};

std::string architectureName(Target Architecture) {
  switch (Architecture) {
  case Target::ARM:
    return "ARM";
  case Target::X86_64:
    return "X86_64";
  case Target::AArch64:
    return "AArch64";
  case Target::RISCV64:
    return "RISCV64";
  case Target::S390X:
    return "S390X";
  }
  return "Unknown";
}

[[maybe_unused]] std::string objectArchitectureCaseName(
    const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string linuxRelocationCaseName(
    const testing::TestParamInfo<LinuxRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string failingRelocationCaseName(
    const testing::TestParamInfo<FailingRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string portableRelocationCaseName(
    const testing::TestParamInfo<PortableRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string
portableObjectCaseName(const testing::TestParamInfo<PortableObjectCase> &Info) {
  return architectureName(Info.param.Architecture);
}

std::optional<uint8_t> expectedELFPatchWidth(Target Architecture,
                                             uint32_t Type) {
  constexpr uint8_t NoBytes = 0;
  constexpr uint8_t WordBytes = 4;
  constexpr uint8_t DoubleWordBytes = 8;
  switch (Architecture) {
  case Target::ARM:
    switch (Type) {
    case llvm::ELF::R_ARM_NONE:
      return NoBytes;
    case llvm::ELF::R_ARM_ABS32:
    case llvm::ELF::R_ARM_REL32:
    case llvm::ELF::R_ARM_THM_CALL:
    case llvm::ELF::R_ARM_CALL:
    case llvm::ELF::R_ARM_JUMP24:
    case llvm::ELF::R_ARM_PREL31:
      return WordBytes;
    default:
      return std::nullopt;
    }
  case Target::AArch64:
    switch (Type) {
    case llvm::ELF::R_AARCH64_ABS64:
    case llvm::ELF::R_AARCH64_PREL64:
      return DoubleWordBytes;
    case llvm::ELF::R_AARCH64_PREL32:
    case llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21:
    case llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_JUMP26:
    case llvm::ELF::R_AARCH64_CALL26:
    case llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC:
      return WordBytes;
    default:
      return std::nullopt;
    }
  case Target::RISCV64:
    switch (Type) {
    case llvm::ELF::R_RISCV_64:
    case llvm::ELF::R_RISCV_CALL:
    case llvm::ELF::R_RISCV_CALL_PLT:
      return DoubleWordBytes;
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
    case llvm::ELF::R_RISCV_PCREL_LO12_S:
    case llvm::ELF::R_RISCV_32_PCREL:
    case llvm::ELF::R_RISCV_ADD32:
    case llvm::ELF::R_RISCV_SUB32:
      return WordBytes;
    case llvm::ELF::R_RISCV_RELAX:
      return NoBytes;
    default:
      return std::nullopt;
    }
  case Target::S390X:
    switch (Type) {
    case llvm::ELF::R_390_64:
      return DoubleWordBytes;
    case llvm::ELF::R_390_PC32:
    case llvm::ELF::R_390_PC32DBL:
    case llvm::ELF::R_390_PLT32DBL:
      return WordBytes;
    default:
      return std::nullopt;
    }
  default:
    return std::nullopt;
  }
}

bool expectedELFPCRelative(Target Architecture, uint32_t Type) {
  switch (Architecture) {
  case Target::ARM:
    return Type == llvm::ELF::R_ARM_REL32 ||
           Type == llvm::ELF::R_ARM_THM_CALL || Type == llvm::ELF::R_ARM_CALL ||
           Type == llvm::ELF::R_ARM_JUMP24 || Type == llvm::ELF::R_ARM_PREL31;
  case Target::AArch64:
    return Type == llvm::ELF::R_AARCH64_PREL64 ||
           Type == llvm::ELF::R_AARCH64_PREL32 ||
           Type == llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21 ||
           Type == llvm::ELF::R_AARCH64_JUMP26 ||
           Type == llvm::ELF::R_AARCH64_CALL26;
  case Target::RISCV64:
    return Type == llvm::ELF::R_RISCV_CALL ||
           Type == llvm::ELF::R_RISCV_CALL_PLT ||
           Type == llvm::ELF::R_RISCV_PCREL_HI20 ||
           Type == llvm::ELF::R_RISCV_PCREL_LO12_I ||
           Type == llvm::ELF::R_RISCV_PCREL_LO12_S ||
           Type == llvm::ELF::R_RISCV_32_PCREL;
  case Target::S390X:
    return Type == llvm::ELF::R_390_PC32 || Type == llvm::ELF::R_390_PC32DBL ||
           Type == llvm::ELF::R_390_PLT32DBL;
  default:
    return Type == llvm::ELF::R_X86_64_PC32 ||
           Type == llvm::ELF::R_X86_64_PLT32 ||
           Type == llvm::ELF::R_X86_64_GOTPCRELX ||
           Type == llvm::ELF::R_X86_64_REX_GOTPCRELX;
  }
}

void initializeLLVMTargets() {
  static const bool Initialized = [] {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    return true;
  }();
  (void)Initialized;
}

const llvm::Target *lookupLLVMTarget(const llvm::Triple &Triple,
                                     std::string &Error) {
  initializeLLVMTargets();
  llvm::Triple LookupTriple = Triple;
  return llvm::TargetRegistry::lookupTarget("", LookupTriple, Error);
}

#define REQUIRE_LLVM_TARGET(TRIPLE)                                            \
  do {                                                                         \
    const llvm::Triple RequiredTriple(TRIPLE);                                 \
    std::string TargetError;                                                   \
    if (lookupLLVMTarget(RequiredTriple, TargetError) == nullptr) {            \
      GTEST_SKIP() << RequiredTriple.str() << ": " << TargetError;             \
    }                                                                          \
  } while (false)

bool hasRelocationHandler(Target Architecture) {
  switch (Architecture) {
  case Target::X86_64:
    return WASMEDGE_LINKER_HAS_X86_64;
  case Target::AArch64:
    return WASMEDGE_LINKER_HAS_AARCH64;
  case Target::ARM:
    return WASMEDGE_LINKER_HAS_ARM;
  case Target::RISCV64:
    return WASMEDGE_LINKER_HAS_RISCV64;
  case Target::S390X:
    return WASMEDGE_LINKER_HAS_S390X;
  }
  return false;
}

#define REQUIRE_RELOCATION_HANDLER(TARGET)                                     \
  do {                                                                         \
    if (!hasRelocationHandler(TARGET)) {                                       \
      GTEST_SKIP() << architectureName(TARGET)                                 \
                   << " relocation handler is not compiled";                   \
    }                                                                          \
  } while (false)

std::vector<WasmEdge::Byte> makeObject(
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
  llvm::TargetOptions Options;
  std::unique_ptr<llvm::TargetMachine> Machine(
      NativeTarget->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
          Triple,
#else
          Triple.str(),
#endif
          CPU, Features, Options, llvm::Reloc::PIC_));
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr) {
    return {};
  }
#if LLVM_VERSION_MAJOR >= 18
  Machine->setOptLevel(Optimize ? llvm::CodeGenOptLevel::Default
                                : llvm::CodeGenOptLevel::None);
#else
  Machine->setOptLevel(Optimize ? llvm::CodeGenOpt::Default
                                : llvm::CodeGenOpt::None);
#endif

  llvm::LLVMContext Context;
  llvm::Module Module("object-reader-test", Context);
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(Triple);
#else
  Module.setTargetTriple(Triple.str());
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
  }
  if (HiddenData) {
    Value->setVisibility(llvm::GlobalValue::HiddenVisibility);
  }
  auto *Zero = new llvm::GlobalVariable(Module, I32, false,
                                        llvm::GlobalValue::InternalLinkage,
                                        llvm::ConstantInt::get(I32, 0), "zero");
  Zero->setAlignment(llvm::Align(16));
  if (UnusedAllocatableSections) {
    auto *UnusedData = new llvm::GlobalVariable(
        Module, I32, false, llvm::GlobalValue::ExternalLinkage,
        llvm::ConstantInt::get(I32, UINT32_C(0x6E6F6367)), "unused_data");
    UnusedData->setVisibility(llvm::GlobalValue::HiddenVisibility);
    UnusedData->setSection(Triple.isOSBinFormatMachO()  ? "__TEXT,__unused_data"
                           : Triple.isOSBinFormatCOFF() ? ".rdata$unused"
                                                        : ".data.unused");
    UnusedData->setAlignment(llvm::Align(4));
  }
  auto *F0 = llvm::Function::Create(llvm::FunctionType::get(I32, false),
                                    llvm::GlobalValue::ExternalLinkage,
                                    FunctionName, Module);
  F0->addFnAttr(llvm::Attribute::NoUnwind);
  if (UnwindTable) {
#if LLVM_VERSION_MAJOR >= 14
    F0->setUWTableKind(llvm::UWTableKind::Sync);
#else
    F0->addFnAttr(llvm::Attribute::UWTable);
#endif
  }
  if (TypeWrapper) {
    auto *T0 = llvm::Function::Create(llvm::FunctionType::get(I32, false),
                                      llvm::GlobalValue::ExternalLinkage, "t0",
                                      Module);
#if LLVM_VERSION_MAJOR >= 14
    T0->setUWTableKind(llvm::UWTableKind::Sync);
#else
    T0->addFnAttr(llvm::Attribute::UWTable);
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
#if LLVM_VERSION_MAJOR >= 14
      UnusedFunction->setUWTableKind(llvm::UWTableKind::Sync);
#else
      UnusedFunction->addFnAttr(llvm::Attribute::UWTable);
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
    Loaded->setAlignment(llvm::Align(4));
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
    Memory->setAlignment(llvm::Align(16));
    auto *MemoryAddress = Builder.CreateInBoundsGEP(
        MemoryType, Memory,
        {llvm::ConstantInt::get(I32, 0), llvm::ConstantInt::get(I32, 8)});
    auto *MemoryValue =
        Builder.CreateLoad(llvm::Type::getInt8Ty(Context), MemoryAddress, true);
    Builder.CreateStore(MemoryValue, MemoryAddress, true);

    auto *VectorType =
#if LLVM_VERSION_MAJOR >= 11
        llvm::VectorType::get(I32, VectorLanes, false);
#else
        llvm::VectorType::get(I32, VectorLanes);
#endif
    auto *Vector = new llvm::GlobalVariable(
        Module, VectorType, false, llvm::GlobalValue::InternalLinkage,
        llvm::ConstantAggregateZero::get(VectorType), "vector");
    Vector->setAlignment(llvm::Align(16));
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
#if LLVM_VERSION_MAJOR >= 15
        llvm::PointerType::getUnqual(Context), I32);
#else
        llvm::Type::getInt8PtrTy(Context), I32);
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
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<WasmEdge::Byte>(Storage.begin(), Storage.end());
}

uint64_t read64le(const std::vector<WasmEdge::Byte> &Bytes, size_t Offset) {
  uint64_t Value = 0;
  for (size_t I = 0; I < 8; ++I) {
    Value |= static_cast<uint64_t>(Bytes[Offset + I]) << (I * 8);
  }
  return Value;
}

uint32_t read32le(const std::vector<WasmEdge::Byte> &Bytes, size_t Offset) {
  uint32_t Value = 0;
  for (size_t I = 0; I < 4; ++I) {
    Value |= static_cast<uint32_t>(Bytes[Offset + I]) << (I * 8);
  }
  return Value;
}

std::optional<std::vector<uint32_t>>
machOUnwindFunctionStarts(const std::vector<WasmEdge::Byte> &Content) {
  if (Content.size() < 28)
    return std::nullopt;
  const size_t Index = read32le(Content, 20);
  const uint32_t IndexCount = read32le(Content, 24);
  if (IndexCount == 0 || Index > Content.size() ||
      IndexCount > (Content.size() - Index) / 12)
    return std::nullopt;
  std::vector<uint32_t> Result;
  for (uint32_t I = 0; I + 1 < IndexCount; ++I) {
    const uint32_t Base = read32le(Content, Index + I * 12);
    const size_t Page = read32le(Content, Index + I * 12 + 4);
    if (Page > Content.size() || Content.size() - Page < 8)
      return std::nullopt;
    const uint32_t Kind = read32le(Content, Page);
    uint16_t Entries = 0;
    uint16_t Count = 0;
    std::memcpy(&Entries, Content.data() + Page + 4, sizeof(Entries));
    std::memcpy(&Count, Content.data() + Page + 6, sizeof(Count));
    const size_t Width = Kind == 2 ? 8 : Kind == 3 ? 4 : 0;
    if (Width == 0 || Entries > Content.size() - Page ||
        Count > (Content.size() - Page - Entries) / Width)
      return std::nullopt;
    for (uint16_t J = 0; J < Count; ++J) {
      const uint32_t Entry = read32le(Content, Page + Entries + J * Width);
      Result.push_back(Kind == 2 ? Entry
                                 : Base + (Entry & UINT32_C(0x00FFFFFF)));
    }
  }
  return Result;
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

RawCompactUnwindInventory
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
    auto Name = Section.getName();
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

void write64le(std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
               uint64_t Value) {
  for (size_t I = 0; I < 8; ++I) {
    Bytes[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
  }
}

void write32le(std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
               uint32_t Value) {
  for (size_t I = 0; I < 4; ++I) {
    Bytes[Offset + I] = static_cast<WasmEdge::Byte>(Value >> (I * 8));
  }
}

struct MachOEHFrameObject {
  size_t Content = 0;
  std::vector<size_t> Relocations;
};

MachOEHFrameObject
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
    auto Name = Section.getName();
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

std::vector<WasmEdge::Byte> makeAArch64MachOEHFrameObject() {
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

void clearMachOSectionRelocations(std::vector<WasmEdge::Byte> &Bytes,
                                  std::string_view Name);
std::vector<WasmEdge::Byte> makeAssemblyObject(const llvm::Triple &Triple,
                                               std::string Assembly,
                                               std::string Features,
                                               bool HardFloat);

X86_64MachOEHFrameFixture makeX86_64MachOEHFrameFixture(bool Raw,
                                                        bool Alias = false,
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
    auto Name = Section.getName();
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

void clearMachOSectionRelocations(std::vector<WasmEdge::Byte> &Bytes,
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

void clearMachOSectionInstructionAttributes(std::vector<WasmEdge::Byte> &Bytes,
                                            std::string_view Name) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  constexpr size_t SectionFlagsOffset = 64;
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  bool Cleared = false;
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
        if (SectionName.substr(0, SectionName.find('\0')) != Name)
          continue;
        const uint32_t Flags =
            read32le(Bytes, SectionOffset + SectionFlagsOffset);
        write32le(Bytes, SectionOffset + SectionFlagsOffset,
                  Flags & ~(llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                            llvm::MachO::S_ATTR_SOME_INSTRUCTIONS));
        Cleared = true;
      }
    }
    CommandOffset += CommandSize;
  }
  ASSERT_TRUE(Cleared);
}

void renameMachOSection(std::vector<WasmEdge::Byte> &Bytes,
                        std::string_view OldName, std::string_view NewName) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  ASSERT_LE(NewName.size(), 16U);
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  bool Renamed = false;
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
        if (SectionName.substr(0, SectionName.find('\0')) != OldName)
          continue;
        std::fill_n(Bytes.begin() + static_cast<ptrdiff_t>(SectionOffset), 16,
                    WasmEdge::Byte{0});
        std::copy(NewName.begin(), NewName.end(),
                  Bytes.begin() + static_cast<ptrdiff_t>(SectionOffset));
        Renamed = true;
      }
    }
    CommandOffset += CommandSize;
  }
  ASSERT_TRUE(Renamed);
}

size_t machOSectionHeaderOffset(const std::vector<WasmEdge::Byte> &Bytes,
                                std::string_view Name) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
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
          return SectionOffset;
      }
    }
    CommandOffset += CommandSize;
  }
  ADD_FAILURE() << "generated object has no " << Name << " section";
  return 0;
}

struct X86_64MachOSectionRelocation {
  size_t Field = 0;
  size_t Relocation = 0;
  int64_t Raw = 0;
  uint64_t TargetOffset = 0;
  uint64_t TargetSize = 0;
  uint32_t Type = 0;
  bool SymbolLess = false;
  bool SectionTarget = false;
};

std::vector<WasmEdge::Byte> makeX86_64MachOSectionOffsetObject() {
  constexpr std::string_view Assembly = R"(
.text
cmpq %rax, Ldata(%rip)
ret
.data
.space 13
Ldata:
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("x86_64-apple-macosx"),
                            std::string(Assembly), {}, false);
}

X86_64MachOSectionRelocation
x86_64MachOSectionRelocation(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-relocations.o"));
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
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__text")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {};
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getSymbol() != MachO->symbol_end())
        continue;
      const auto Target =
          MachO->getRelocationSection(Relocation.getRawDataRefImpl());
      if (Target == MachO->section_end())
        continue;
      const uint32_t Type = static_cast<uint32_t>(Relocation.getType());
      const int64_t Suffix = Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                          : 0;
      const auto ContentOffset =
          reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
          Bytes.data();
      EXPECT_GE(ContentOffset, 0);
      const size_t Field = static_cast<size_t>(ContentOffset) +
                           static_cast<size_t>(Relocation.getOffset());
      const int64_t Raw = static_cast<int32_t>(read32le(Bytes, Field));
      const int64_t TargetOffset =
          Raw + static_cast<int64_t>(Section.getAddress()) +
          static_cast<int64_t>(Relocation.getOffset()) + 4 + Suffix -
          static_cast<int64_t>(Target->getAddress());
      EXPECT_GE(TargetOffset, 0);
      return {Field,
              Header.reloff + Relocation.getRawDataRefImpl().d.b * 8,
              Raw,
              static_cast<uint64_t>(TargetOffset),
              Target->getSize(),
              Type,
              true,
              true};
    }
  }
  ADD_FAILURE() << "generated object has no x86_64 section relocation";
  return {};
}

class ScopedLoggingCallback {
public:
  explicit ScopedLoggingCallback(std::string &Diagnostic)
      : Logger(spdlog::default_logger()), Level(spdlog::get_level()) {
    WasmEdge::Log::setLoggingCallback(
        [&](const spdlog::details::log_msg &Value) {
          Diagnostic.assign(Value.payload.data(), Value.payload.size());
        });
    WasmEdge::Log::setErrorLoggingLevel();
  }

  ~ScopedLoggingCallback() {
    spdlog::set_default_logger(Logger);
    spdlog::set_level(Level);
  }

private:
  std::shared_ptr<spdlog::logger> Logger;
  spdlog::level::level_enum Level;
};

struct CompactUnwindObjectOffsets {
  size_t Content;
  uint32_t Count;
};

CompactUnwindObjectOffsets
compactUnwindObjectOffsets(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
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
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__compact_unwind")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {};
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    const auto Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    return {Content, Header.nreloc};
  }
  ADD_FAILURE() << "generated object has no compact unwind section";
  return {};
}

size_t compactUnwindRelocationFileOffset(std::vector<WasmEdge::Byte> &Bytes,
                                         uint64_t FieldOffset) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return 0;
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return 0;
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__compact_unwind")
      continue;
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() == FieldOffset)
        return Header.reloff + Relocation.getRawDataRefImpl().d.b * 8;
    }
  }
  ADD_FAILURE() << "no compact unwind relocation at " << FieldOffset;
  return 0;
}

std::vector<uint32_t>
ehFrameFDEOffsets(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {};
  }
  std::vector<uint32_t> Result;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() >= 8)
        Result.push_back(static_cast<uint32_t>(Relocation.getOffset() - 8));
    }
  }
  std::sort(Result.begin(), Result.end());
  Result.erase(std::unique(Result.begin(), Result.end()), Result.end());
  return Result;
}

size_t elf64RelocationSectionHeader(const std::vector<WasmEdge::Byte> &Bytes) {
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
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (Name &&
#if LLVM_VERSION_MAJOR >= 16
        (Name->starts_with(".rela") || Name->starts_with(".rel"))) {
#else
        (Name->startswith(".rela") || Name->startswith(".rel"))) {
#endif
      Index = Section.getIndex();
      break;
    }
  }
  EXPECT_NE(Index, 0U);
  return static_cast<size_t>(read64le(Bytes, 40) + Index * 64);
}

#if LLVM_VERSION_MAJOR >= 19
std::vector<WasmEdge::Byte> makeX86_64CrelObject() {
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto Offset = read64le(Bytes, Header + 24);
  const auto Info = read64le(Bytes, Offset + 8);
  const auto Addend = static_cast<int64_t>(read64le(Bytes, Offset + 16));
  const std::array<llvm::ELF::Elf_Crel<true>, 1> Relocations{{
      {read64le(Bytes, Offset), static_cast<uint32_t>(Info >> 32),
       static_cast<uint32_t>(Info), Addend},
  }};
  llvm::SmallVector<char, 16> Encoded;
  llvm::raw_svector_ostream Stream(Encoded);
  llvm::ELF::encodeCrel<true>(
      Stream, Relocations, [](const auto &Relocation) { return Relocation; });
  std::copy(Encoded.begin(), Encoded.end(), Bytes.data() + Offset);
  Bytes[Header + 4] = 0x14;
  Bytes[Header + 5] = 0x00;
  Bytes[Header + 6] = 0x00;
  Bytes[Header + 7] = 0x40;
  write64le(Bytes, Header + 32, Encoded.size());
  write64le(Bytes, Header + 56, 1);
  return Bytes;
}
#endif

std::vector<WasmEdge::Byte> makeNativeObject(bool Undefined = false) {
  return makeObject(llvm::Triple(llvm::sys::getDefaultTargetTriple()),
                    Undefined);
}

std::vector<WasmEdge::Byte> makeUnusedNativeObject() {
  return makeObject(llvm::Triple(llvm::sys::getDefaultTargetTriple()), false,
                    false, "f0", {}, false, true, "generic", {}, true, false,
                    false, false, false, false, {}, true, false, false, false,
                    true);
}

std::vector<WasmEdge::Byte> makeSemanticObject(const llvm::Triple &Triple,
                                               bool Undefined = false) {
  return makeObject(Triple, Undefined, false, "f0", {}, false, false, "generic",
                    {}, false, false, false, false, false, false, {}, true);
}

std::vector<WasmEdge::Byte> makeAssemblyObject(const llvm::Triple &Triple,
                                               std::string Assembly,
                                               std::string Features = {},
                                               bool HardFloat = false) {
  std::string Error;
  const llvm::Target *Target = lookupLLVMTarget(Triple, Error);
  EXPECT_NE(Target, nullptr) << Error;
  if (Target == nullptr) {
    return {};
  }
  llvm::TargetOptions Options;
  if (HardFloat) {
    Options.FloatABIType = llvm::FloatABI::Hard;
    Options.MCOptions.ABIName = "aapcs-vfp";
  }
  std::unique_ptr<llvm::TargetMachine> Machine(Target->createTargetMachine(
#if LLVM_VERSION_MAJOR >= 21
      Triple,
#else
      Triple.str(),
#endif
      "generic", Features, Options, llvm::Reloc::PIC_));
  EXPECT_NE(Machine, nullptr);
  if (Machine == nullptr) {
    return {};
  }
  llvm::LLVMContext Context;
  llvm::Module Module("x86-relocation-test", Context);
#if LLVM_VERSION_MAJOR >= 21
  Module.setTargetTriple(Triple);
#else
  Module.setTargetTriple(Triple.str());
#endif
  Module.setDataLayout(Machine->createDataLayout());
  Module.setModuleInlineAsm(std::move(Assembly));
  llvm::SmallVector<char, 0> Storage;
  llvm::raw_svector_ostream Stream(Storage);
  llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR >= 18
  const auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
  const auto FileType = llvm::CGFT_ObjectFile;
#endif
  EXPECT_FALSE(Machine->addPassesToEmitFile(Passes, Stream, nullptr, FileType));
  Passes.run(Module);
  return std::vector<WasmEdge::Byte>(Storage.begin(), Storage.end());
}

struct AArch64MachOAddendFixture {
  std::vector<WasmEdge::Byte> Bytes;
  size_t Content = 0;
  size_t RelocationCount = 0;
  std::vector<size_t> Relocations;
};

AArch64MachOAddendFixture makeAArch64MachOAddendFixture() {
  auto Bytes = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.text
.globl _entry
.globl _target
.globl _data
_entry:
  bl _target+4
  adrp x0, _data@PAGE+4097
  add x0, x0, _data@PAGEOFF+4097
  ret
.section __TEXT,__target,regular,pure_instructions
_target:
  ret
.section __DATA,__data
_data:
  .quad 0
.subsections_via_symbols
)");
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm64-addend.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {std::move(Bytes), 0, 0, {}};
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return {std::move(Bytes), 0, 0, {}};
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__text")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {std::move(Bytes), 0, 0, {}};
    AArch64MachOAddendFixture Result;
    Result.Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    Result.RelocationCount = machOSectionHeaderOffset(Bytes, "__text") + 60;
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      Result.Relocations.push_back(Header.reloff +
                                   Relocation.getRawDataRefImpl().d.b * 8);
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_ADDEND ||
          Relocation.getOffset() != 0)
        continue;
      const size_t Record = Result.Relocations.back();
      const uint32_t Word = read32le(Bytes, Record + 4);
      write32le(Bytes, Record + 4,
                (Word & UINT32_C(0xFF000000)) | UINT32_C(0x00FFFFFC));
    }
    Result.Bytes = std::move(Bytes);
    return Result;
  }
  ADD_FAILURE() << "generated object has no text section";
  return {std::move(Bytes), 0, 0, {}};
}

std::vector<WasmEdge::Byte> makeAliasedAArch64MachODwarfObject(
    std::string_view FunctionSection = "__text") {
  const std::string Assembly = ".section __TEXT," +
                               std::string(FunctionSection) +
                               R"(,regular,pure_instructions
.globl _compact_alias
.globl _fde_alias
_compact_alias:
_fde_alias:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Laliased_cie:
.long Laliased_cie_end-Laliased_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Laliased_cie_end:
Laliased_fde:
.long Laliased_fde_end-Laliased_fde-4
.long Laliased_fde+4-Laliased_cie
.quad _fde_alias-.
.quad 4
.byte 0
.p2align 3
Laliased_fde_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _compact_alias
.long 4
.long 0x03000018
.quad 0
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), Assembly);
}

std::vector<WasmEdge::Byte> makeRegularMachOFunctionSectionObject() {
  auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, {}, false, false, false, false, true, false, true);
  renameMachOSection(Bytes, "__unused_text", "__custom_text");
  clearMachOSectionInstructionAttributes(Bytes, "__custom_text");
  return Bytes;
}

std::vector<WasmEdge::Byte> makeSectionTargetAArch64MachODwarfObject() {
  constexpr std::string_view Assembly = R"(
.text
nop
.globl _section_target
_section_target:
Lsection_target:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lsection_cie:
.long Lsection_cie_end-Lsection_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Lsection_cie_end:
Lsection_fde:
.long Lsection_fde_end-Lsection_fde-4
.long Lsection_fde+4-Lsection_cie
.quad Lsection_target-.
.quad 4
.byte 0
.p2align 3
Lsection_fde_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _section_target
.long 4
.long 0x03000018
.quad 0
.quad 0
)";
  auto Bytes = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"),
                                  std::string(Assembly));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-target.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return Bytes;
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return Bytes;
  uint32_t TextOrdinal = 0;
  uint32_t Ordinal = 0;
  for (const auto &Section : MachO->sections()) {
    ++Ordinal;
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (Name && *Name == "__text")
      TextOrdinal = Ordinal;
  }
  EXPECT_NE(TextOrdinal, 0U);
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const size_t FileOffset =
          Header.reloff + Relocation.getRawDataRefImpl().d.b * 8 + 4;
      const uint32_t Word = read32le(Bytes, FileOffset);
      write32le(Bytes, FileOffset, (Word & UINT32_C(0xF7000000)) | TextOrdinal);
      const size_t Content = static_cast<size_t>(
          reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
          Bytes.data());
      write64le(Bytes, Content + Relocation.getOffset(),
                read64le(Bytes, Content + Relocation.getOffset()) + 4);
    }
  }
  return Bytes;
}

void setMachOEHFrameInitialLocation(std::vector<WasmEdge::Byte> &Bytes,
                                    uint64_t Value) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "malformed-section-target.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    auto Contents = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents));
    const size_t Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() == llvm::MachO::ARM64_RELOC_UNSIGNED)
        write64le(Bytes, Content + Relocation.getOffset(), Value);
    }
    return;
  }
  FAIL() << "generated object has no EH frame section";
}

std::vector<WasmEdge::Byte>
makeMultipleFDEAArch64MachODwarfObject(uint32_t Encoding) {
  std::string Assembly = R"(
.text
.globl _multiple_fde
_multiple_fde:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lmultiple_cie:
.long Lmultiple_cie_end-Lmultiple_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Lmultiple_cie_end:
Lmultiple_fde0:
.long Lmultiple_fde0_end-Lmultiple_fde0-4
.long Lmultiple_fde0+4-Lmultiple_cie
.quad _multiple_fde-.
.quad 4
.byte 0
.p2align 3
Lmultiple_fde0_end:
Lmultiple_fde1:
.long Lmultiple_fde1_end-Lmultiple_fde1-4
.long Lmultiple_fde1+4-Lmultiple_cie
.quad _multiple_fde-.
.quad 4
.byte 0
.p2align 3
Lmultiple_fde1_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _multiple_fde
.long 4
.long )";
  Assembly += std::to_string(Encoding);
  Assembly += R"(
.quad 0
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), Assembly);
}

std::vector<WasmEdge::Byte> makeX86_64AssemblyObject(std::string Assembly) {
  return makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                            std::move(Assembly));
}

std::vector<WasmEdge::Byte> makeX86_64ELFReaderFixture() {
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
  std::vector<WasmEdge::Byte> Bytes;
  WasmEdge::convertHexStrToBytes(Hex, Bytes, 0);
  return Bytes;
}

Target nativeTarget() {
#if defined(__x86_64__) || defined(_M_X64)
  return Target::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
  return Target::AArch64;
#elif defined(__arm__) || defined(_M_ARM)
  return Target::ARM;
#elif defined(__riscv) && __riscv_xlen == 64
  return Target::RISCV64;
#elif defined(__s390x__)
  return Target::S390X;
#else
#error Unsupported test host
#endif
}

struct AOTMetadata {
  using SectionTuple =
      std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>;

  uint32_t Version;
  uint8_t OS;
  uint8_t Arch;
  uint64_t VersionAddress;
  uint64_t IntrinsicsAddress;
  std::vector<uintptr_t> Types;
  std::vector<uintptr_t> Codes;
  std::vector<SectionTuple> Sections;
};

AOTMetadata parseAOTMetadata(const std::filesystem::path &Path) {
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  auto Module = Loader.parseModule(Path);
  EXPECT_TRUE(Module);
  if (!Module) {
    return {};
  }
  const auto &AOT = (*Module)->getAOTSection();
  return {AOT.getVersion(),           AOT.getOSType(),
          AOT.getArchType(),          AOT.getVersionAddress(),
          AOT.getIntrinsicsAddress(), AOT.getTypesAddress(),
          AOT.getCodesAddress(),      AOT.getSections()};
}

class LinkerOutputTest : public testing::Test {
protected:
  void SetUp() override {
    Directory =
        std::filesystem::temp_directory_path() /
        ("WasmEdgeLinker-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(Directory));
  }

  void TearDown() override {
    if (std::getenv("WASMEDGE_KEEP_LINKER_FIXTURE") == nullptr)
      std::filesystem::remove_all(Directory);
  }

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
    auto Data = Compiler.compile(**Module);
    EXPECT_TRUE(Data);
    if (!Data) {
      return {};
    }
    const auto Original = std::filesystem::current_path();
    std::filesystem::current_path(Directory);
    const auto Result = CodeGen.codegen(Wasm, std::move(*Data), Output);
    std::filesystem::current_path(Original);
    EXPECT_TRUE(Result);
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

  std::filesystem::path Directory;
};

class UnsupportedObjectArchitectureTest
    : public LinkerOutputTest,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

#if defined(__x86_64__) || defined(_M_X64)
class NonHostObjectFormatTest
    : public LinkerOutputTest,
      public testing::WithParamInterface<ObjectArchitectureCase> {};
#endif

class GeneratedMachOLinkerTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

class GeneratedWindowsLinkerTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

class MSVCFltusedPolicyTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

TEST_F(LinkerOutputTest, UniversalWasmWriterSerializesLoaderSchema) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("golden.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0x20, 0, {1, 2, 3, 4}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 3, 0x28, 4, {5, 6, 7}});
  auto Bss =
      Graph.addSection(Section{".bss", SectionKind::BSS, 16, 9, 0x30, 0, {}});
  auto Unwind = Graph.addSection(Section{".eh_frame",
                                         SectionKind::Unwind,
                                         8,
                                         2,
                                         0x40,
                                         7,
                                         {8, 9},
                                         SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && Data && Bss && Unwind);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 2, 1, true}));
  ASSERT_TRUE(
      Graph.addSymbol(Symbol{"f0", *Text, 2, 1, true, {}, false, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "golden.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  auto Module = Loader.parseModule(Output);
  ASSERT_TRUE(Module);
  const auto &AOT = (*Module)->getAOTSection();
  EXPECT_EQ(AOT.getVersion(), WasmEdge::AOT::kBinaryVersion);
#if WASMEDGE_OS_LINUX
  EXPECT_EQ(AOT.getOSType(), 1U);
#elif WASMEDGE_OS_MACOS
  EXPECT_EQ(AOT.getOSType(), 2U);
#elif WASMEDGE_OS_WINDOWS
  EXPECT_EQ(AOT.getOSType(), 3U);
#endif
#if defined(__x86_64__) || defined(_M_X64)
  EXPECT_EQ(AOT.getArchType(), 1U);
#elif defined(__aarch64__) || defined(_M_ARM64)
  EXPECT_EQ(AOT.getArchType(), 2U);
#elif defined(__riscv) && __riscv_xlen == 64
  EXPECT_EQ(AOT.getArchType(), 3U);
#elif defined(__arm__) || defined(_M_ARM)
  EXPECT_EQ(AOT.getArchType(), 4U);
#elif defined(__s390x__)
  EXPECT_EQ(AOT.getArchType(), 5U);
#endif
  EXPECT_EQ(AOT.getVersionAddress(), 0x29U);
  EXPECT_EQ(AOT.getIntrinsicsAddress(), 0x2AU);
  EXPECT_TRUE(AOT.getTypesAddress().empty());
  EXPECT_EQ(AOT.getCodesAddress(), (std::vector<uintptr_t>{0x23}));
  EXPECT_EQ(AOT.getSections().size(), 4U);
  EXPECT_EQ(
      AOT.getSections()[0],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          1, 0x20, 4, {1, 2, 3, 4}}));
  EXPECT_EQ(
      AOT.getSections()[1],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          2, 0x28, 3, {5, 6, 7}}));
  EXPECT_EQ(
      AOT.getSections()[2],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          3, 0x30, 9, {}}));
  EXPECT_EQ(
      AOT.getSections()[3],
      (std::tuple<uint8_t, uint64_t, uint64_t, std::vector<WasmEdge::Byte>>{
          4, 0x40, 2, {8, 9}}));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterMergesSameKindSectionsAndGaps) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("merge.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{".text.a", SectionKind::Text, 1, 2, 0x10, 0, {1, 2}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".text.b", SectionKind::Text, 8, 2, 0x18, 2, {3, 4}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".data.a", SectionKind::Data, 1, 2, 0x1000, 4, {5, 6}}));
  ASSERT_TRUE(Graph.addSection(
      Section{".data.b", SectionKind::Data, 4, 1, 0x1004, 6, {7}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", 2, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", 3, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "merged.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  ASSERT_EQ(Metadata.Sections.size(), 2U);
  EXPECT_EQ(Metadata.Sections[0],
            (decltype(Metadata.Sections)::value_type{
                1, 0x10, 0x0A, {1, 2, 0, 0, 0, 0, 0, 0, 3, 4}}));
  EXPECT_EQ(Metadata.Sections[1], (decltype(Metadata.Sections)::value_type{
                                      2, 0x1000, 5, {5, 6, 0, 0, 7}}));
}

TEST_F(LinkerOutputTest, UniversalWasmWriterClassifiesUnwindByPurpose) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("unwind-kinds.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x1000, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(Section{".ARM.exidx",
                                       SectionKind::Unwind,
                                       4,
                                       4,
                                       0x2000,
                                       0,
                                       {1, 2, 3, 4},
                                       SectionPurpose::ARMExidx}));
  ASSERT_TRUE(Graph.addSection(Section{".eh_frame",
                                       SectionKind::Unwind,
                                       4,
                                       4,
                                       0x3000,
                                       0,
                                       {0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Graph.addSection(
      Section{".unwind", SectionKind::Unwind, 4, 4, 0x4000, 0, {5, 6, 7, 8}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "unwind-kinds.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto Exidx = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        const auto Address = std::get<1>(Value);
        const auto &Content = std::get<3>(Value);
        return Address <= 0x2000 && 0x2004 <= Address + Content.size() &&
               std::equal(Content.begin() + (0x2000 - Address),
                          Content.begin() + (0x2004 - Address),
                          std::array<WasmEdge::Byte, 4>{1, 2, 3, 4}.begin());
      });
  const auto EHFrame = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) { return std::get<1>(Value) == 0x3000; });
  const auto Generic = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        const auto Address = std::get<1>(Value);
        const auto &Content = std::get<3>(Value);
        return Address <= 0x4000 && 0x4004 <= Address + Content.size() &&
               std::equal(Content.begin() + (0x4000 - Address),
                          Content.begin() + (0x4004 - Address),
                          std::array<WasmEdge::Byte, 4>{5, 6, 7, 8}.begin());
      });
  ASSERT_NE(Exidx, Metadata.Sections.end());
  ASSERT_NE(EHFrame, Metadata.Sections.end());
  ASSERT_NE(Generic, Metadata.Sections.end());
  EXPECT_EQ(std::get<0>(*Exidx), 2U);
  EXPECT_EQ(std::get<0>(*EHFrame), 4U);
  EXPECT_EQ(std::get<0>(*Generic), 2U);
}

LinkGraph makeUniversalCOFFPDataGraph(size_t FirstSize = 12,
                                      size_t SecondSize = 12) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  EXPECT_TRUE(Graph.beginInput("runtime-functions.obj"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 16, 64, 0x100, 0,
                               std::vector<WasmEdge::Byte>(64)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 2, 0x200, 0, {0, 0}});
  EXPECT_TRUE(Graph.addSection(Section{".xdata", SectionKind::Unwind, 4, 8,
                                       0x300, 0, std::vector<WasmEdge::Byte>(8),
                                       SectionPurpose::XData}));
  EXPECT_TRUE(Graph.addSection(
      Section{".pdata$a", SectionKind::Unwind, 4, FirstSize, 0x400, 0,
              std::vector<WasmEdge::Byte>(FirstSize), SectionPurpose::PData}));
  EXPECT_TRUE(Graph.addSection(
      Section{".pdata$b", SectionKind::Unwind, 4, SecondSize, 0x500, 0,
              std::vector<WasmEdge::Byte>(SecondSize), SectionPurpose::PData}));
  EXPECT_TRUE(Text && Data);
  EXPECT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  return Graph;
}

TEST_F(LinkerOutputTest, UniversalWasmWriterCompactsAndSortsCOFFPData) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  auto Graph = makeUniversalCOFFPDataGraph();
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 0, 0x120);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 4, 0x130);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 8, 0x304);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 0, 0x100);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 4, 0x110);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 8, 0x300);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "sorted-pdata.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; });
  ASSERT_NE(PData, Metadata.Sections.end());
  EXPECT_EQ(std::get<1>(*PData), 0x400U);
  EXPECT_EQ(std::get<2>(*PData), 24U);
  ASSERT_EQ(std::get<3>(*PData).size(), 24U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 0), 0x100U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 12), 0x120U);
  const auto XData = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        return std::get<1>(Value) <= 0x300 &&
               0x308 <= std::get<1>(Value) + std::get<3>(Value).size();
      });
  ASSERT_NE(XData, Metadata.Sections.end());
  EXPECT_EQ(std::get<0>(*XData), 2U);
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPlacesCOFFPDataAtLowestAddressWhenInsertedReverse) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  auto Graph = makeUniversalCOFFPDataGraph();
  auto &First = const_cast<Section &>(Graph.sections()[3]);
  auto &Second = const_cast<Section &>(Graph.sections()[4]);
  std::swap(First.Address, Second.Address);
  write32le(First.Content, 0, 0x120);
  write32le(First.Content, 4, 0x130);
  write32le(First.Content, 8, 0x304);
  write32le(Second.Content, 0, 0x100);
  write32le(Second.Content, 4, 0x110);
  write32le(Second.Content, 8, 0x300);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "reverse-pdata.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; });
  ASSERT_NE(PData, Metadata.Sections.end());
  EXPECT_EQ(std::get<1>(*PData), 0x400U);
  EXPECT_EQ(std::get<2>(*PData), 24U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 0), 0x100U);
  EXPECT_EQ(read32le(std::get<3>(*PData), 12), 0x120U);
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsMalformedCOFFPData) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  for (auto Graph :
       {makeUniversalCOFFPDataGraph(13, 12), makeUniversalCOFFPDataGraph()}) {
    if (Graph.sections()[3].VirtualSize == 12) {
      for (const SectionId Id : {SectionId{3}, SectionId{4}}) {
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 0,
                  0x100);
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 4,
                  0x120);
        write32le(const_cast<Section &>(Graph.sections()[Id]).Content, 8,
                  0x300);
      }
    }
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsOverlappingCOFFPData) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto Graph = makeUniversalCOFFPDataGraph();
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 0, 0x100);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 4, 0x130);
  write32le(const_cast<Section &>(Graph.sections()[3]).Content, 8, 0x300);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 0, 0x120);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 4, 0x140);
  write32le(const_cast<Section &>(Graph.sections()[4]).Content, 8, 0x304);
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterDoesNotNormalizeNonPDataCOFFUnwind) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("non-pdata.obj"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x100, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(Section{".xdata",
                                       SectionKind::Unwind,
                                       1,
                                       3,
                                       0x200,
                                       0,
                                       {1, 2, 3},
                                       SectionPurpose::XData}));
  ASSERT_TRUE(Graph.addSection(Section{".eh_frame",
                                       SectionKind::Unwind,
                                       1,
                                       3,
                                       0x300,
                                       0,
                                       {4, 5, 6},
                                       SectionPurpose::EHFrame}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "non-pdata.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  EXPECT_TRUE(
      std::none_of(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; }));
  const auto DataSection = std::find_if(
      Metadata.Sections.begin(), Metadata.Sections.end(),
      [](const auto &Value) {
        return std::get<0>(Value) == 2 && std::get<1>(Value) <= 0x200 &&
               0x303 <= std::get<1>(Value) + std::get<3>(Value).size();
      });
  ASSERT_NE(DataSection, Metadata.Sections.end());
}

TEST_F(LinkerOutputTest, UniversalWasmWriterTreatsELFPDataAsOrdinaryData) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  LinkGraph Graph(nativeTarget(), nativeTarget() == Target::S390X
                                      ? Endianness::Big
                                      : Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("ordinary-pdata.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x100, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".pdata", SectionKind::ReadOnly, 1, 4, 0x200, 0, {1, 2, 3, 4}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "ordinary-pdata.wasm";

  ASSERT_TRUE(UniversalWasmWriter::write(Graph, TinyWasm, Output));
  const auto Metadata = parseAOTMetadata(Output);
  EXPECT_TRUE(
      std::none_of(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) { return std::get<0>(Value) == 4; }));
  const auto PData =
      std::find_if(Metadata.Sections.begin(), Metadata.Sections.end(),
                   [](const auto &Value) {
                     const auto Address = std::get<1>(Value);
                     const auto &Content = std::get<3>(Value);
                     return std::get<0>(Value) == 2 && Address <= 0x200 &&
                            0x204 <= Address + Content.size();
                   });
  ASSERT_NE(PData, Metadata.Sections.end());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsAppliedGraphWithRebases) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("rebases.o"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 8, 8, 0x1000,
                                       0, std::vector<WasmEdge::Byte>(8)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x2000, 8, {0, 0}});
  ASSERT_TRUE(Text && Data);
  auto Version = Graph.addSymbol(Symbol{"version", *Data, 0, 1, true});
  ASSERT_TRUE(Version);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(
      Graph.addRelocation(Relocation{*Text, 0, llvm::ELF::R_X86_64_64, *Version,
                                     0, false, ObjectFormat::ELF, 8}));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_FALSE(Graph.rebases().empty());
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  std::string Diagnostic;
  ScopedLoggingCallback Capture(Diagnostic);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
  EXPECT_NE(Diagnostic.find("section='.text' id=0"), std::string::npos);
  EXPECT_NE(Diagnostic.find("offset=0 width=8"), std::string::npos);
  EXPECT_NE(Diagnostic.find("format=ELF type=1"), std::string::npos);
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsMixedCOFFImageBases) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("mixed-image-base.obj"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 1, PEImageBase + 0x100, 0, {0}});
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 2, PEImageBase + 0x200, 0, {0, 0}});
  ASSERT_TRUE(
      Graph.addSection(Section{".bss", SectionKind::BSS, 1, 1, 0x300, 0, {}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsSectionAddressOverflow) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("section-overflow.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 1, 0x100, 0, {0}});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 2, 0x200, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".bss", SectionKind::BSS, 1, 2, UINT64_MAX, 0, {}}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, UniversalWasmWriterRejectsPDataAddressOverflow) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("pdata-overflow.obj"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 1, 0x20,
                                       PEImageBase + 0x100, 0,
                                       std::vector<WasmEdge::Byte>(0x20)});
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 2, PEImageBase + 0x200, 0, {0, 0}});
  ASSERT_TRUE(Graph.addSection(
      Section{".xdata", SectionKind::Unwind, 1, 4, PEImageBase + 0x300, 0,
              std::vector<WasmEdge::Byte>(4), SectionPurpose::XData}));
  std::vector<WasmEdge::Byte> PData(12);
  write32le(PData, 0, 0x100);
  write32le(PData, 4, 0x120);
  write32le(PData, 8, 0x300);
  ASSERT_TRUE(Graph.addSection(Section{".pdata", SectionKind::Unwind, 1, 12,
                                       UINT64_MAX - 5, 0, std::move(PData),
                                       SectionPurpose::PData}));
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_TRUE(Bytes.empty());
}

TEST(LinkerWriterTest, RejectsInvalidSemanticSymbolTables) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto MakeGraph = [] {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    EXPECT_TRUE(Graph.beginInput("symbols.o"));
    EXPECT_TRUE(
        Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
    EXPECT_TRUE(Graph.addSection(
        Section{".data", SectionKind::Data, 1, 3, 1, 1, {1, 2, 3}}));
    return Graph;
  };
  auto Write = [&](LinkGraph &Graph) {
    EXPECT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    return UniversalWasmWriter::write(Graph, EmptyWasm, Output);
  };

  auto Missing = MakeGraph();
  EXPECT_FALSE(Write(Missing));

  auto LocalShadow = MakeGraph();
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"local_version", 1, 0, 1, false}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"version", 1, 1, 1, true}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"intrinsics", 1, 2, 1, true}));
  ASSERT_TRUE(LocalShadow.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  EXPECT_TRUE(Write(LocalShadow));

  auto Duplicate = MakeGraph();
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Duplicate.addSymbol(Symbol{"other", 0, 0, 1, true, "f0"}));
  EXPECT_FALSE(Write(Duplicate));

  auto Sparse = MakeGraph();
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(Sparse.addSymbol(Symbol{"f2", 0, 0, 1, true}));
  EXPECT_FALSE(Write(Sparse));

  auto NonCanonical = MakeGraph();
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"version", 1, 0, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"f0", 0, 0, 1, true}));
  ASSERT_TRUE(NonCanonical.addSymbol(Symbol{"f01", 0, 0, 1, true}));
  EXPECT_TRUE(Write(NonCanonical));

  LinkGraph Darwin(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Darwin.beginInput("symbols.o"));
  ASSERT_TRUE(
      Darwin.addSection(Section{"__text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  ASSERT_TRUE(Darwin.addSection(
      Section{"__data", SectionKind::Data, 1, 2, 1, 1, {1, 2}}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_version", 1, 0, 1, true}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_intrinsics", 1, 1, 1, true}));
  ASSERT_TRUE(Darwin.addSymbol(Symbol{"_f0", 0, 0, 1, true}));
  EXPECT_TRUE(Write(Darwin));
}

TEST(LinkerWriterTest, RejectsSemanticIndicesBeyondVectorCapacity) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Capacity = std::vector<uint64_t>().max_size();
  for (const char Prefix : {'t', 'f'}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("oversized-symbol.o"));
    auto Text =
        Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 1, 2, 1, 1, {1, 2}});
    ASSERT_TRUE(Text && Data);
    ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
    ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
    ASSERT_TRUE(Graph.addSymbol(Symbol{
        std::string(1, Prefix) + std::to_string(Capacity), *Text, 0, 1, true}));
    ASSERT_TRUE(applyRelocations(Graph));
    std::vector<WasmEdge::Byte> Bytes;
    Writer Output(Bytes);
    bool Written = true;

    EXPECT_NO_THROW(Written = static_cast<bool>(
                        UniversalWasmWriter::write(Graph, EmptyWasm, Output)));
    EXPECT_FALSE(Written);
    EXPECT_TRUE(Bytes.empty());
  }
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPreservesDestinationOnValidationFailure) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  constexpr std::array<WasmEdge::Byte, 8> InvalidWasm{0x01, 0x61, 0x73, 0x6D,
                                                      0x01, 0x00, 0x00, 0x00};
  const std::vector<WasmEdge::Byte> Original{9, 8, 7, 6};
  const auto Output = Directory / "preserved.wasm";
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("invalid-graph.o"));

  for (const auto Wasm : {WasmEdge::Span<const WasmEdge::Byte>(InvalidWasm),
                          WasmEdge::Span<const WasmEdge::Byte>(EmptyWasm)}) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      ASSERT_TRUE(File);
      File.write(reinterpret_cast<const char *>(Original.data()),
                 static_cast<std::streamsize>(Original.size()));
    }
    EXPECT_FALSE(UniversalWasmWriter::write(Graph, Wasm, Output));
    EXPECT_EQ(readFile(Output), Original);
  }
}

TEST_F(LinkerOutputTest,
       UniversalWasmWriterPropagatesOpenFailureAfterValidation) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("valid.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  auto Data =
      Graph.addSection(Section{".data", SectionKind::Data, 1, 2, 1, 1, {1, 2}});
  ASSERT_TRUE(Text && Data);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"version", *Data, 0, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"intrinsics", *Data, 1, 1, true}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"f0", *Text, 0, 1, true}));
  ASSERT_TRUE(applyRelocations(Graph));
  const auto Output = Directory / "missing" / "output.wasm";

  EXPECT_FALSE(UniversalWasmWriter::write(Graph, EmptyWasm, Output));
  EXPECT_FALSE(std::filesystem::exists(Output));
}

TEST_F(LinkerOutputTest, UniversalCodegenExecutesTinyFixture) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      WasmEdge::CompilerConfigure::OutputFormat::Wasm);
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator Validator(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  WasmEdge::VM::VM VM(Conf);
  auto Module = Loader.parseModule(TinyWasm);
  ASSERT_TRUE(Module);
  ASSERT_TRUE(Validator.validate(**Module));
  auto Data = Compiler.compile(**Module);
  ASSERT_TRUE(Data);
  const auto Output = Directory / "execute.wasm";

  ASSERT_TRUE(CodeGen.codegen(TinyWasm, std::move(*Data), Output));
  ASSERT_TRUE(VM.loadWasm(Output));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto Result = VM.execute("f");
  ASSERT_TRUE(Result);
  ASSERT_EQ(Result->size(), 1U);
  EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 7U);
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsUnsupportedOutputAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Existing{1, 2, 3};
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Existing.data()),
               Existing.size());
  }
  const auto Object = makeNativeObject();
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};

#if WASMEDGE_OS_LINUX
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::MachO));
#else
  EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output, OutputKind::ELF));
#endif
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Existing.begin(), Existing.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, ReportsRealObjectDiagnosticOnceAtBoundary) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object = makeNativeObject(true);
  auto ReadResult = ObjectReader::read(Object, nativeTarget());
  ASSERT_FALSE(ReadResult);
  EXPECT_EQ(ReadResult.error().Message, "undefined symbol");
  EXPECT_EQ(ReadResult.error().SymbolName, "value");
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result =
      NativeLinker::link(Object, EmptyWasm, Directory / "diagnostic.wasm",
                         OutputKind::UniversalWasm);

  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MalformedSection);
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_NE(Logs[0].find("native linker: undefined symbol"), std::string::npos);
  EXPECT_NE(Logs[0].find("symbol name='value'"), std::string::npos);
}

#if WASMEDGE_OS_LINUX && (defined(__x86_64__) || defined(_M_X64))
TEST_F(LinkerOutputTest, ReportsRealRelocationFailureOnceAtBoundary) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  auto Object = makeNativeObject();
  const auto Header = elf64RelocationSectionHeader(Object);
  const auto RelocationOffset = read64le(Object, Header + 24);
  write64le(Object, RelocationOffset + 16, static_cast<uint64_t>(INT64_MAX));
  auto ReadResult = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(ReadResult);
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result = NativeLinker::link(
      Object, EmptyWasm, Directory / "relocation-diagnostic.wasm",
      OutputKind::UniversalWasm);

  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::AOTNotImpl);
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_NE(Logs[0].find("native linker:"), std::string::npos);
  EXPECT_NE(Logs[0].find("unsupported"), std::string::npos);
  EXPECT_NE(Logs[0].find("section="), std::string::npos);
  EXPECT_NE(Logs[0].find("symbol="), std::string::npos);
  EXPECT_NE(Logs[0].find("relocation="), std::string::npos);
  EXPECT_NE(Logs[0].find("offset="), std::string::npos);
}
#endif

#if WASMEDGE_OS_LINUX
TEST_F(LinkerOutputTest, NativeAOTWriterLoadsAndExecutesWithoutImports) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "native.so";
  const auto SecondOutput = Directory / "native-second.so";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.so", true);
  ASSERT_FALSE(std::filesystem::exists(Directory / "seed.so"));

  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::ELF));
  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, SecondOutput, OutputKind::ELF));
  EXPECT_EQ(readFile(Output), readFile(SecondOutput));
  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *ELF =
      llvm::dyn_cast<llvm::object::ELFObjectFileBase>(Image->getBinary());
  ASSERT_NE(ELF, nullptr);
  std::set<std::string> DynamicSymbols;
  for (const auto &Symbol : ELF->getDynamicSymbolIterators()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (!Name->empty())
      DynamicSymbols.emplace(Name->str());
    auto Flags = Symbol.getFlags();
    ASSERT_TRUE(static_cast<bool>(Flags));
    EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U);
  }
  for (const char *Name :
       {"f0", "version", "intrinsics", "wasm.code", "wasm.size"})
    EXPECT_TRUE(DynamicSymbols.count(Name)) << Name;

  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  EXPECT_TRUE(Library->get<uint32_t>("version"));
  EXPECT_TRUE(Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
      "intrinsics"));
  EXPECT_TRUE(Library->get<uint8_t>("wasm.code"));
  EXPECT_TRUE(Library->get<uint32_t>("wasm.size"));
  EXPECT_EQ(execute(Output), 7U);
}
#endif

#if WASMEDGE_OS_MACOS
TEST_F(LinkerOutputTest, ReportsFailedMachOLinkerStage) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Output = Directory / "failed.dylib";
  std::vector<std::string> Logs;
  const auto OriginalLogger = spdlog::default_logger();
  const auto OriginalLevel = spdlog::get_level();
  WasmEdge::Log::setLoggingCallback([&](const spdlog::details::log_msg &Value) {
    Logs.emplace_back(Value.payload.data(), Value.payload.size());
  });
  WasmEdge::Log::setErrorLoggingLevel();

  const auto Result =
      NativeLinker::link({}, EmptyWasm, Output, OutputKind::MachO);
  spdlog::set_default_logger(OriginalLogger);
  spdlog::set_level(OriginalLevel);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MalformedSection);
  EXPECT_FALSE(std::filesystem::exists(Output));
  ASSERT_EQ(Logs.size(), 1U);
  EXPECT_EQ(Logs[0], "native linker: empty object buffer");
}

TEST_F(LinkerOutputTest, NativeMachOWriterLoadsAndExecutesSignedLibrary) {
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "native.dylib";
  const auto Object =
      compileTinyObject(TinyWasm, Directory / "seed.wasm", true);
  auto Input = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(Input);
  const bool HasCompact = !Input->compactUnwind().empty();
  const bool NeedsDwarf =
      std::any_of(Input->compactUnwind().begin(), Input->compactUnwind().end(),
                  [](const auto &Record) { return Record.FDE.has_value(); });
  const bool HasDwarfReferences = !Input->ehFrameReferences().empty();
  const bool HasInputEHFrame =
      std::any_of(Input->sections().begin(), Input->sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::EHFrame;
                  });
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::MachO));

  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *MachO =
      llvm::dyn_cast<llvm::object::MachOObjectFile>(Image->getBinary());
  ASSERT_NE(MachO, nullptr);
  std::set<std::string> Symbols;
  std::set<std::string> Sections;
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    Sections.emplace(Name->str());
  }
  EXPECT_EQ(Sections.count("__unwind_info") != 0, HasCompact);
  EXPECT_EQ(Sections.count("__eh_frame") != 0,
            HasInputEHFrame &&
                (!HasCompact || NeedsDwarf || HasDwarfReferences));
  EXPECT_FALSE(Sections.count("__compact_unwind"));
  for (const auto &Symbol : MachO->symbols()) {
    auto Name = Symbol.getName();
    auto Flags = Symbol.getFlags();
    ASSERT_TRUE(Name && Flags);
    EXPECT_EQ(*Flags & llvm::object::SymbolRef::SF_Undefined, 0U);
    Symbols.emplace(Name->str());
  }
  for (const char *Name :
       {"_f0", "_version", "_intrinsics", "_wasm.code", "_wasm.size"})
    EXPECT_TRUE(Symbols.count(Name)) << Name;
  const auto Bytes = readFile(Output);
  size_t Command = sizeof(llvm::MachO::mach_header_64);
  bool HasDyldInfo = false;
  auto Read32 = [&](size_t Offset) {
    uint32_t Value = 0;
    std::memcpy(&Value, Bytes.data() + Offset, sizeof(Value));
    return Value;
  };
  for (uint32_t I = 0; I < Read32(16); ++I) {
    const uint32_t Type = Read32(Command);
    const uint32_t Size = Read32(Command + 4);
    ASSERT_GE(Size, 8U);
    if (Type == llvm::MachO::LC_DYLD_INFO_ONLY) {
      HasDyldInfo = true;
      for (const size_t Offset : {size_t{16}, size_t{20}, size_t{24},
                                  size_t{28}, size_t{32}, size_t{36}})
        EXPECT_EQ(Read32(Command + Offset), 0U);
    }
    Command += Size;
  }
  EXPECT_TRUE(HasDyldInfo);

  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  {
    auto Version = Library->get<uint32_t>("version");
    auto Intrinsics =
        Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
            "intrinsics");
    auto F0 = Library->get<WasmEdge::Executable::Wrapper>("f0");
    auto WasmCode = Library->get<uint8_t>("wasm.code");
    auto WasmSize = Library->get<uint32_t>("wasm.size");
    EXPECT_TRUE(Version);
    EXPECT_TRUE(Intrinsics);
    EXPECT_TRUE(F0);
    EXPECT_TRUE(WasmCode);
    EXPECT_TRUE(WasmSize);
  }
  EXPECT_EQ(execute(Output), 7U);
  Library->unload();
  EXPECT_FALSE(Library->get<uint32_t>("version"));
}
#endif

#if WASMEDGE_OS_WINDOWS
TEST_F(LinkerOutputTest, NativePEWriterLoadsAndExecutesWithoutImports) {
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "native.dll";
  const auto Object =
      compileTinyObject(TinyWasm, Directory / "seed.wasm", true);
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Output, OutputKind::PE));
  auto Image = llvm::object::ObjectFile::createObjectFile(Output.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *PE =
      llvm::dyn_cast<llvm::object::COFFObjectFile>(Image->getBinary());
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  EXPECT_EQ(PE->getPE32PlusHeader()->AddressOfEntryPoint, 0U);
  auto Library = std::make_shared<WasmEdge::Loader::SharedLibrary>();
  ASSERT_TRUE(Library->load(Output));
  EXPECT_TRUE(Library->get<uint32_t>("version"));
  EXPECT_TRUE(Library->get<const WasmEdge::Executable::IntrinsicsTable *>(
      "intrinsics"));
  EXPECT_TRUE(Library->get<WasmEdge::Executable::Wrapper>("f0"));
  EXPECT_TRUE(Library->get<uint8_t>("wasm.code"));
  EXPECT_TRUE(Library->get<uint32_t>("wasm.size"));
  EXPECT_EQ(execute(Output), 7U);
}
#endif

TEST_F(LinkerOutputTest, NativeLinkerCreatesNoNativeTemporary) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "output.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  ASSERT_TRUE(std::filesystem::is_regular_file(Output));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerReplacesExistingOutputAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "replace.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const std::array<WasmEdge::Byte, 4> Sentinel{1, 2, 3, 4};
  {
    std::ofstream File(Output, std::ios_base::binary);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
#if !WASMEDGE_OS_WINDOWS
  constexpr std::filesystem::perms SentinelMode =
      std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
      std::filesystem::perms::group_read;
  std::filesystem::permissions(Output, SentinelMode,
                               std::filesystem::perm_options::replace);
#endif

  ASSERT_TRUE(
      NativeLinker::link(Object, TinyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_NE(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  EXPECT_EQ(execute(Output), 7U);
#if !WASMEDGE_OS_WINDOWS
  EXPECT_EQ(std::filesystem::status(Output).permissions() &
                std::filesystem::perms::mask,
            SentinelMode);
#endif
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsBadObjectsAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Existing{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  for (const auto &Object :
       {std::vector<WasmEdge::Byte>{0, 1, 2}, makeNativeObject(true)}) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      File.write(reinterpret_cast<const char *>(Existing.data()),
                 Existing.size());
    }
    EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output,
                                    OutputKind::UniversalWasm));
    EXPECT_EQ(readFile(Output),
              (std::vector<WasmEdge::Byte>(Existing.begin(), Existing.end())));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsWindowsCOFFAtomically) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto COFF = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                       ".text\nretq\n");
  auto Concatenated = COFF;
  Concatenated.insert(Concatenated.end(), COFF.begin(), COFF.end());
  std::vector<std::vector<WasmEdge::Byte>> Invalid;
#if !WASMEDGE_OS_WINDOWS
  Invalid.push_back(COFF);
#endif
  Invalid.push_back(Concatenated);

  for (const auto &Object : Invalid) {
    {
      std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
      File.write(reinterpret_cast<const char *>(Sentinel.data()),
                 Sentinel.size());
      ASSERT_TRUE(File);
    }
    EXPECT_FALSE(NativeLinker::link(Object, EmptyWasm, Output,
                                    OutputKind::UniversalWasm));
    EXPECT_EQ(readFile(Output),
              (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsLinuxELFOnWindowsAtomically) {
#if WASMEDGE_OS_WINDOWS
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto ELF = makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                                      ".text\nretq\n");
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(ELF, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
#else
  GTEST_SKIP() << "Linux ELF is the host object format";
#endif
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsConcatenatedHostObjectAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Native =
      makeSemanticObject(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
  auto Concatenated = Native;
  Concatenated.insert(Concatenated.end(), Native.begin(), Native.end());
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(NativeLinker::link(Concatenated, EmptyWasm, Output,
                                  OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsArchiveAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const std::string ArchiveMagic = "!<arch>\n";
  const std::vector<WasmEdge::Byte> Archive(ArchiveMagic.begin(),
                                            ArchiveMagic.end());
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(NativeLinker::link(Archive, EmptyWasm, Output,
                                  OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsUndefinedHostObjectAtomically) {
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object = makeSemanticObject(
      llvm::Triple(llvm::sys::getDefaultTargetTriple()), true);
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

#if WASMEDGE_OS_LINUX && (defined(__x86_64__) || defined(_M_X64))
TEST_P(UnsupportedObjectArchitectureTest,
       NativeLinkerRejectsUnsupportedInputsAtomically) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object = makeSemanticObject(llvm::Triple(Test.Triple));
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

INSTANTIATE_TEST_SUITE_P(
    NonHostArchitectures, UnsupportedObjectArchitectureTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-unknown-linux-gnu", Target::AArch64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      switch (Info.index) {
      case 0:
        return std::string("MachO_X86_64");
      case 1:
        return std::string("COFF_X86_64");
      default:
        return architectureName(Info.param.Architecture);
      }
    });
#endif

TEST_F(LinkerOutputTest, RetainsUnusedAllocatableSections) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Object = makeUnusedNativeObject();
  auto Graph = ObjectReader::read(Object, nativeTarget());
  ASSERT_TRUE(Graph);
  SectionId UnusedTextSection = InvalidSectionId;
  SectionId UnusedDataSection = InvalidSectionId;
  uint64_t UnusedDataOffset = 0;
  uint64_t UnusedDataSize = 0;
  for (const auto &Symbol : Graph->symbols()) {
    if (!Symbol.Exported && !Symbol.Global)
      continue;
    if (Symbol.Name == "unused_function" || Symbol.Name == "_unused_function")
      UnusedTextSection = Symbol.Section;
    if (Symbol.Name == "unused_data" || Symbol.Name == "_unused_data") {
      UnusedDataSection = Symbol.Section;
      UnusedDataOffset = Symbol.Offset;
      UnusedDataSize = Symbol.Size;
    }
  }
  ASSERT_LT(UnusedTextSection, Graph->sections().size());
  ASSERT_LT(UnusedDataSection, Graph->sections().size());
  const auto &UnusedText = Graph->sections()[UnusedTextSection].Content;
  const std::vector<WasmEdge::Byte> UnusedData{0x67, 0x63, 0x6F, 0x6E};
  EXPECT_EQ(Graph->sections()[UnusedTextSection].Kind, SectionKind::Text);
  EXPECT_TRUE(Graph->sections()[UnusedDataSection].Kind == SectionKind::Data ||
              Graph->sections()[UnusedDataSection].Kind ==
                  SectionKind::ReadOnly);
  ASSERT_FALSE(UnusedText.empty());
  auto Contains = [](const auto &Bytes, const auto &Marker) {
    return std::search(Bytes.begin(), Bytes.end(), Marker.begin(),
                       Marker.end()) != Bytes.end();
  };
  const auto &UnusedDataContent = Graph->sections()[UnusedDataSection].Content;
  ASSERT_EQ(UnusedDataSize, UnusedData.size());
  ASSERT_LE(UnusedDataOffset, UnusedDataContent.size());
  ASSERT_LE(UnusedDataSize, UnusedDataContent.size() - UnusedDataOffset);
  using Difference = std::vector<WasmEdge::Byte>::difference_type;
  ASSERT_LE(UnusedDataOffset,
            static_cast<uint64_t>(std::numeric_limits<Difference>::max()));
  EXPECT_TRUE(std::equal(UnusedData.begin(), UnusedData.end(),
                         UnusedDataContent.begin() +
                             static_cast<Difference>(UnusedDataOffset)));

  if (Graph->format() == ObjectFormat::MachO) {
    ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
    ASSERT_TRUE(layout(*Graph, 0, 0x4000));
    EXPECT_TRUE(normalizeMachOEHFrame(*Graph));
    EXPECT_TRUE(validateMachOEHFrameCoverage(*Graph));
  }

  const auto Universal = Directory / "retained.wasm";
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Universal,
                                 OutputKind::UniversalWasm));
  const auto Metadata = parseAOTMetadata(Universal);
  bool UniversalText = false;
  bool UniversalData = false;
  for (const auto &Section : Metadata.Sections) {
    const auto &Content = std::get<3>(Section);
    UniversalText |= Contains(Content, UnusedText);
    UniversalData |= Contains(Content, UnusedData);
  }
  EXPECT_TRUE(UniversalText);
  EXPECT_TRUE(UniversalData);

#if WASMEDGE_OS_LINUX
  const auto Native = Directory / "retained.so";
  ASSERT_TRUE(NativeLinker::link(Object, TinyWasm, Native, OutputKind::ELF));
  auto Image = llvm::object::ObjectFile::createObjectFile(Native.string());
  ASSERT_TRUE(static_cast<bool>(Image));
  bool NativeText = false;
  bool NativeData = false;
  for (const auto &Section : Image->getBinary()->sections()) {
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> Bytes(Content->bytes_begin(),
                                            Content->bytes_end());
    const llvm::object::ELFSectionRef ELFSection(Section);
    NativeText |= Contains(Bytes, UnusedText) &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_ALLOC) != 0 &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_EXECINSTR) != 0;
    NativeData |= Contains(Bytes, UnusedData) &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_ALLOC) != 0 &&
                  (ELFSection.getFlags() & llvm::ELF::SHF_WRITE) != 0;
  }
  EXPECT_TRUE(NativeText);
  EXPECT_TRUE(NativeData);
#endif
}

TEST_F(LinkerOutputTest, NativeLinkerRejectsInvalidWasmFramingAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "malformed.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const std::array<std::vector<WasmEdge::Byte>, 5> Invalid{{
      {0x01, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x02, 0x00, 0x00, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00},
      {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x80, 0x80, 0x80,
       0x80, 0x10},
  }};
  for (const auto &Wasm : Invalid) {
    EXPECT_FALSE(
        NativeLinker::link(Object, Wasm, Output, OutputKind::UniversalWasm));
    EXPECT_FALSE(std::filesystem::exists(Output));
    expectNoTemporaryFiles();
  }
}

TEST_F(LinkerOutputTest, NativeLinkerDoesNotSemanticallyReparseWasm) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  constexpr std::array<WasmEdge::Byte, 14> DuplicateTypeSections{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00};
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  const auto Output = Directory / "validated-input.wasm";

  ASSERT_TRUE(NativeLinker::link(Object, DuplicateTypeSections, Output,
                                 OutputKind::UniversalWasm));
  const auto Bytes = readFile(Output);
  ASSERT_GE(Bytes.size(), DuplicateTypeSections.size());
  EXPECT_TRUE(std::equal(DuplicateTypeSections.begin(),
                         DuplicateTypeSections.end(), Bytes.begin()));
}

#if defined(__x86_64__) || defined(_M_X64)
TEST_P(NonHostObjectFormatTest,
       NativeLinkerRejectsNonHostObjectFormatsAtomically) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object = makeObject(llvm::Triple(Test.Triple), false, false, "f0",
                                 {}, false, false, "generic", {}, false, false,
                                 false, false, false, false, {}, true);
  const auto Output = Directory / (std::string(Test.Triple) + ".wasm");
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_FALSE(std::filesystem::exists(Output));
  expectNoTemporaryFiles();
}

#if WASMEDGE_OS_LINUX
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "MachO_X86_64" : "COFF_X86_64";
    });
#elif WASMEDGE_OS_MACOS
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-unknown-linux-gnu", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "ELF_X86_64" : "COFF_X86_64";
    });
#elif WASMEDGE_OS_WINDOWS
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-unknown-linux-gnu", Target::X86_64},
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "ELF_X86_64" : "MachO_X86_64";
    });
#endif
#endif

TEST_F(LinkerOutputTest,
       NativeLinkerPublishesConcurrentlyWithoutTempSurvivors) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::promise<void> Start;
  const auto Ready = Start.get_future().share();
  auto Link = [&](const char *Name) {
    Ready.wait();
    return NativeLinker::link(Object, TinyWasm, Directory / Name,
                              OutputKind::UniversalWasm);
  };
  auto First = std::async(std::launch::async, Link, "first.wasm");
  auto Second = std::async(std::launch::async, Link, "second.wasm");
  Start.set_value();

  EXPECT_TRUE(First.get());
  EXPECT_TRUE(Second.get());
  EXPECT_EQ(parseAOTMetadata(Directory / "first.wasm").Sections,
            parseAOTMetadata(Directory / "second.wasm").Sections);
  expectNoTemporaryFiles();
}

TEST_F(LinkerOutputTest, NativeLinkerPublishesConcurrentlyToSameDestination) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Output = Directory / "shared.wasm";
  const auto Object = compileTinyObject(TinyWasm, Directory / "seed.wasm");
  std::promise<void> Start;
  const auto Ready = Start.get_future().share();
  auto Link = [&] {
    Ready.wait();
    return NativeLinker::link(Object, TinyWasm, Output,
                              OutputKind::UniversalWasm);
  };
  auto First = std::async(std::launch::async, Link);
  auto Second = std::async(std::launch::async, Link);
  Start.set_value();

  EXPECT_TRUE(First.get());
  EXPECT_TRUE(Second.get());
  EXPECT_EQ(execute(Output), 7U);
  expectNoTemporaryFiles();
}

static_assert(sizeof(Target) == sizeof(uint8_t));
static_assert(sizeof(Endianness) == sizeof(uint8_t));
static_assert(sizeof(SectionKind) == sizeof(uint8_t));
static_assert(sizeof(ObjectFormat) == sizeof(uint8_t));
static_assert(std::is_same_v<SectionId, uint32_t>);
static_assert(std::is_same_v<SymbolId, uint32_t>);
static_assert(
    std::is_same_v<decltype(Section::Content), std::vector<WasmEdge::Byte>>);
static_assert(std::is_same_v<decltype(Symbol::Section), SectionId>);
static_assert(
    noexcept(std::declval<LinkGraph &>().swap(std::declval<LinkGraph &>())));

TEST(LinkGraphTest, OwnsContentAndUsesStableIds) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  std::vector<uint8_t> Content{0x48, 0x89, 0xE5};
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 16, 3, 0, 0, Content});
  ASSERT_TRUE(Text);
  EXPECT_EQ(*Text, 0U);

  Content[0] = 0;
  ASSERT_EQ(Graph.sections().size(), 1U);
  EXPECT_EQ(Graph.sections()[*Text].Content[0], 0x48U);

  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 4, 0, 0, {1, 2, 3, 4}});
  ASSERT_TRUE(Data);
  EXPECT_EQ(*Data, 1U);
  EXPECT_EQ(*Text, 0U);

  auto Entry = Graph.addSymbol(Symbol{"entry", *Text, 0, 3, true});
  ASSERT_TRUE(Entry);
  EXPECT_EQ(*Entry, 0U);
  EXPECT_EQ(Graph.target(), Target::X86_64);
  EXPECT_EQ(Graph.endianness(), Endianness::Little);
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsSecondInputObject) {
  LinkGraph Graph(Target::ARM, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("first.o"));
  auto Result = Graph.beginInput("second.o");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "link graph accepts exactly one input object");
}

TEST(LinkGraphTest, RejectsInvalidSectionAlignment) {
  LinkGraph Graph(Target::AArch64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Zero = Graph.addSection(Section{"zero", SectionKind::Data, 0, 0});
  ASSERT_FALSE(Zero);
  EXPECT_EQ(Zero.error().SectionName, "zero");

  auto NonPowerOfTwo =
      Graph.addSection(Section{"three", SectionKind::Data, 3, 0});
  ASSERT_FALSE(NonPowerOfTwo);
  EXPECT_EQ(NonPowerOfTwo.error().Message,
            "section alignment must be a non-zero power of two");
}

TEST(LinkGraphTest, RejectsContentBeyondSectionVirtualSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Result =
      Graph.addSection(Section{".data", SectionKind::Data, 1, 1, 0, 0, {1, 2}});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SectionName, ".data");
  EXPECT_EQ(Result.error().Message,
            "section content exceeds section virtual size");
}

TEST(LinkGraphTest, RejectsDuplicateSymbolDefinitions) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(Graph.addSymbol(Symbol{"same", *Text, 0, 1, false}));

  auto Duplicate = Graph.addSymbol(Symbol{"same", *Text, 1, 1, false});
  ASSERT_FALSE(Duplicate);
  EXPECT_EQ(Duplicate.error().SymbolName, "same");
  EXPECT_EQ(Duplicate.error().Message, "duplicate symbol definition");
}

TEST(LinkGraphTest, RejectsDuplicateSymbolsAcrossGrowthCopiesAndMoves) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 1, 1});
  ASSERT_TRUE(Bss);

  constexpr uint32_t SymbolCount = 20000;
  for (uint32_t I = 0; I < SymbolCount; ++I) {
    auto Added = Graph.addSymbol(
        Symbol{"symbol" + std::to_string(I), *Bss, 0, 0, false});
    ASSERT_TRUE(Added);
    ASSERT_EQ(*Added, I);
  }
  EXPECT_FALSE(Graph.addSymbol(Symbol{"symbol0", *Bss, 0, 0, false}));
  EXPECT_FALSE(Graph.addSymbol(Symbol{"symbol19999", *Bss, 0, 0, false}));
  EXPECT_TRUE(Graph.addSymbol(Symbol{"missing", *Bss, 0, 0, false}));

  LinkGraph Copied = Graph;
  EXPECT_FALSE(Copied.addSymbol(Symbol{"symbol12345", *Bss, 0, 0, false}));

  LinkGraph Moved = std::move(Graph);
  EXPECT_FALSE(Moved.addSymbol(Symbol{"symbol5432", *Bss, 0, 0, false}));
}

TEST(LinkGraphTest, ResetsMovedFromGraphForReuse) {
  const auto Populate = [](LinkGraph &Graph, std::string_view Input) {
    EXPECT_TRUE(Graph.beginInput(Input));
    auto Text = Graph.addSection(Section{
        ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
    EXPECT_TRUE(Text);
    if (!Text)
      return;
    auto Target = Graph.addSymbol(Symbol{"target", *Text, 0, 4, false});
    EXPECT_TRUE(Target);
    if (!Target)
      return;
    EXPECT_TRUE(Graph.addRelocation(
        Relocation{*Text, 0, 2, *Target, 0, false, ObjectFormat::ELF, 4}));
    EXPECT_TRUE(Graph.addRebase(Rebase{*Text, 4, 0, 0, 4}));
    EXPECT_TRUE(Graph.validate());
  };

  LinkGraph Source(Target::X86_64, Endianness::Little);
  Populate(Source, "source.o");
  LinkGraph Moved(std::move(Source));
  EXPECT_TRUE(Moved.validate());
  EXPECT_FALSE(Source.validate());
  EXPECT_TRUE(Source.sections().empty());
  EXPECT_TRUE(Source.symbols().empty());
  EXPECT_TRUE(Source.relocations().empty());
  EXPECT_TRUE(Source.rebases().empty());
  Populate(Source, "reused-constructor-source.o");

  LinkGraph Assigned(Target::X86_64, Endianness::Little);
  Populate(Assigned, "discarded.o");
  Assigned = std::move(Source);
  EXPECT_TRUE(Assigned.validate());
  EXPECT_FALSE(Source.validate());
  EXPECT_TRUE(Source.sections().empty());
  EXPECT_TRUE(Source.symbols().empty());
  EXPECT_TRUE(Source.relocations().empty());
  EXPECT_TRUE(Source.rebases().empty());
  Populate(Source, "reused-assignment-source.o");
}

TEST(LinkGraphTest, CopyAndSelfAssignmentPreserveCoherentGraph) {
  LinkGraph Source(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Source.beginInput("source.o"));
  auto Text = Source.addSection(Section{
      ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Source.addSymbol(Symbol{"target", *Text, 0, 4, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Source.addRelocation(
      Relocation{*Text, 0, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Source.addRebase(Rebase{*Text, 4, 0, 0, 4}));

  LinkGraph Destination(Target::ARM, Endianness::Big, ObjectFormat::COFF);
  ASSERT_TRUE(Destination.beginInput("destination.o"));
  ASSERT_TRUE(Destination.addSection(
      Section{"old", SectionKind::Data, 1, 1, 0, 0, {1}}));
  Destination = Source;
  EXPECT_TRUE(Destination.validate());
  EXPECT_EQ(Destination.target(), Source.target());
  EXPECT_EQ(Destination.endianness(), Source.endianness());
  EXPECT_EQ(Destination.format(), Source.format());
  EXPECT_EQ(Destination.sections().size(), Source.sections().size());
  EXPECT_EQ(Destination.symbols().size(), Source.symbols().size());
  EXPECT_EQ(Destination.relocations().size(), Source.relocations().size());
  EXPECT_EQ(Destination.rebases().size(), Source.rebases().size());
  EXPECT_FALSE(Destination.addSymbol(Symbol{"target", *Text, 0, 4, false}));

  auto &Alias = Source;
  Source = Alias;
  EXPECT_TRUE(Source.validate());
  EXPECT_FALSE(Source.addRelocation(
      Relocation{*Text, 2, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  EXPECT_FALSE(Source.addRebase(Rebase{*Text, 3, 0, 0, 4}));
}

TEST(LinkGraphTest, RejectsUndefinedSymbols) {
  LinkGraph Graph(Target::S390X, Endianness::Big);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Result =
      Graph.addSymbol(Symbol{"missing", InvalidSectionId, 0, 0, false});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SymbolName, "missing");
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_TRUE(Graph.symbols().empty());
}

TEST(LinkGraphTest, RejectsZeroInputObjects) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "link graph requires one input object");
}

TEST(LinkGraphTest, RejectsInvalidSectionIds) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));

  auto Result = Graph.addSymbol(Symbol{"bad", 7, 0, 0, false});
  ASSERT_FALSE(Result);
  ASSERT_TRUE(Result.error().Section);
  EXPECT_EQ(*Result.error().Section, 7U);
  EXPECT_EQ(Result.error().Message, "invalid section ID");
}

TEST(LinkGraphTest, RejectsSymbolsBeyondSectionVirtualSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 8, 8});
  ASSERT_TRUE(Bss);

  auto Result = Graph.addSymbol(Symbol{"too_large", *Bss, 7, 2, false});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().SymbolName, "too_large");
  EXPECT_EQ(Result.error().Offset, 7U);
  EXPECT_EQ(Result.error().Message,
            "symbol extends beyond section virtual size");
}

TEST(LinkGraphTest, StoresRelocationsAndRebases) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 4, false});
  ASSERT_TRUE(TargetSymbol);

  Relocation Stored{*Text, 0, 42, *TargetSymbol, -4};
  Stored.PatchSize = 4;
  ASSERT_TRUE(Graph.addRelocation(Stored));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Text, 2, 7, 8}));
  ASSERT_EQ(Graph.relocations().size(), 1U);
  EXPECT_EQ(Graph.relocations()[0].Symbol, *TargetSymbol);
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(Graph.rebases()[0].Addend, 8);
}

TEST(LinkGraphTest, RejectsOverlappingRelocationsRegardlessOfOrder) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Text = Graph.addSection(Section{
        ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
    ASSERT_TRUE(Text);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
    ASSERT_TRUE(TargetSymbol);
    Relocation First{*Text, 1, 2, *TargetSymbol, 0};
    First.PatchSize = 4;
    Relocation Second{*Text, 3, 2, *TargetSymbol, 0};
    Second.PatchSize = 4;
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Second : First));
    auto Result = Graph.addRelocation(Reverse ? First : Second);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
  }
}

TEST(LinkGraphTest, AllowsUnsortedAdjacentRelocations) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{".text",
                                       SectionKind::Text,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  for (const uint64_t Offset : {UINT64_C(8), UINT64_C(0), UINT64_C(4)}) {
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, Offset, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  }
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsRelocationOverlappingLaterSuccessor) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text",
              SectionKind::Text,
              1,
              16,
              0,
              0,
              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 12, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));

  auto Result = Graph.addRelocation(
      Relocation{*Text, 0, 1, *TargetSymbol, 0, false, ObjectFormat::ELF, 8});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, AllowsZeroWidthMetadataOverlapInEitherOrder) {
  for (const bool MetadataFirst : {false, true}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Text = Graph.addSection(
        Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Text);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    Relocation Patch{*Text,
                     0,
                     llvm::ELF::R_RISCV_PCREL_HI20,
                     *TargetSymbol,
                     0,
                     false,
                     ObjectFormat::ELF,
                     4};
    Relocation Metadata{*Text, 0,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
                        0,     false, ObjectFormat::ELF,        NoPatch};
    ASSERT_TRUE(Graph.addRelocation(MetadataFirst ? Metadata : Patch));
    ASSERT_TRUE(Graph.addRelocation(MetadataFirst ? Patch : Metadata));
    EXPECT_TRUE(Graph.validate());
  }
}

TEST(LinkGraphTest, AllowsTwoZeroWidthMetadataRecordsAtOneSite) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Metadata{*Text, 0,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
                      0,     false, ObjectFormat::ELF,        NoPatch};
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, AllowsNoPatchRecordsAmongUnsortedRelocations) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  const Relocation Metadata{
      *Text, 2,     llvm::ELF::R_RISCV_RELAX, *TargetSymbol,
      0,     false, ObjectFormat::ELF,        NoPatch};
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(Metadata));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, AllowsOnlyCompleteRISCVSymbolDifferencePairs) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("difference.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    const Relocation Add{*Data, 0,     llvm::ELF::R_RISCV_ADD32, *TargetSymbol,
                         0,     false, ObjectFormat::ELF,        4};
    const Relocation Sub{*Data, 0,     llvm::ELF::R_RISCV_SUB32, *TargetSymbol,
                         0,     false, ObjectFormat::ELF,        4};
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Add));
    ASSERT_TRUE(Graph.addRelocation(Reverse ? Add : Sub));
    EXPECT_TRUE(Graph.validate());
    auto Duplicate = Graph.addRelocation(Reverse ? Sub : Add);
    ASSERT_FALSE(Duplicate);
    EXPECT_EQ(Duplicate.error().Message, "overlapping relocation patches");
  }

  LinkGraph Unmatched(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Unmatched.beginInput("unmatched.o"));
  auto Data = Unmatched.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  auto TargetSymbol = Unmatched.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Unmatched.addRelocation(
      Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                 ObjectFormat::ELF, 4}));
  auto Valid = Unmatched.validate();
  ASSERT_FALSE(Valid);
  EXPECT_EQ(Valid.error().Message, "unpaired RISC-V symbol difference");

  LinkGraph WrongWidth(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(WrongWidth.beginInput("width.o"));
  Data = WrongWidth.addSection(Section{
      ".data", SectionKind::Data, 4, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  TargetSymbol = WrongWidth.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  auto Added = WrongWidth.addRelocation(
      Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                 ObjectFormat::ELF, 8});
  ASSERT_FALSE(Added);
  EXPECT_EQ(Added.error().Message, "invalid relocation patch size");
  EXPECT_TRUE(WrongWidth.relocations().empty());
}

TEST(LinkGraphTest, RejectsEveryRISCVSymbolDifferenceTriplePermutation) {
  const std::array<std::array<uint32_t, 3>, 6> Cases{{
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_SUB32},
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_ADD32},
      {llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_ADD32,
       llvm::ELF::R_RISCV_SUB32},
      {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_SUB32,
       llvm::ELF::R_RISCV_SUB32},
  }};
  for (const auto &Types : Cases) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("difference.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    bool Rejected = false;
    for (const uint32_t Type : Types) {
      auto Result = Graph.addRelocation(Relocation{
          *Data, 0, Type, *TargetSymbol, 0, false, ObjectFormat::ELF, 4});
      if (!Result) {
        EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
        Rejected = true;
        break;
      }
    }
    EXPECT_TRUE(Rejected);
    EXPECT_LT(Graph.relocations().size(), 3U);
  }
}

void expectELFPCRelativeRelocations(
    Target Architecture,
    WasmEdge::Span<const std::pair<uint32_t, bool>> Cases) {
  for (const auto &Test : Cases) {
    EXPECT_EQ(
        relocationIsPCRelative(ObjectFormat::ELF, Architecture, Test.first),
        Test.second);
  }
}

TEST(LinkGraphTest, ClassifiesCanonicalARMELFPCRelativeRelocations) {
  const std::array<std::pair<uint32_t, bool>, 4> Cases{{
      {llvm::ELF::R_ARM_REL32, true},
      {llvm::ELF::R_ARM_THM_CALL, true},
      {llvm::ELF::R_ARM_PREL31, true},
      {llvm::ELF::R_ARM_ABS32, false},
  }};
  expectELFPCRelativeRelocations(Target::ARM, Cases);
}

TEST(LinkGraphTest, ClassifiesCanonicalAArch64ELFPCRelativeRelocations) {
  const std::array<std::pair<uint32_t, bool>, 4> Cases{{
      {llvm::ELF::R_AARCH64_PREL64, true},
      {llvm::ELF::R_AARCH64_CALL26, true},
      {llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21, true},
      {llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC, false},
  }};
  expectELFPCRelativeRelocations(Target::AArch64, Cases);
}

TEST(LinkGraphTest, ClassifiesCanonicalRISCV64ELFPCRelativeRelocations) {
  const std::array<std::pair<uint32_t, bool>, 6> Cases{{
      {llvm::ELF::R_RISCV_CALL_PLT, true},
      {llvm::ELF::R_RISCV_PCREL_LO12_S, true},
      {llvm::ELF::R_RISCV_32_PCREL, true},
      {llvm::ELF::R_RISCV_ADD32, false},
      {llvm::ELF::R_RISCV_SUB32, false},
      {llvm::ELF::R_RISCV_RELAX, false},
  }};
  expectELFPCRelativeRelocations(Target::RISCV64, Cases);
}

TEST(LinkGraphTest, ClassifiesCanonicalS390XELFPCRelativeRelocations) {
  const std::array<std::pair<uint32_t, bool>, 3> Cases{{
      {llvm::ELF::R_390_PC32, true},
      {llvm::ELF::R_390_PLT32DBL, true},
      {llvm::ELF::R_390_64, false},
  }};
  expectELFPCRelativeRelocations(Target::S390X, Cases);
}

TEST(LinkGraphTest, ClassifiesCanonicalX86_64COFFPCRelativeRelocation) {
  EXPECT_TRUE(relocationIsPCRelative(ObjectFormat::COFF, Target::X86_64, 4));
}

TEST(LinkGraphTest, EnforcesCanonicalX86RelocationPatchSizes) {
  struct Case {
    ObjectFormat Format;
    uint32_t Type;
    uint8_t Width;
  };
  const std::array<Case, 7> Cases{{
      {ObjectFormat::ELF, 1, 8},
      {ObjectFormat::ELF, 2, 4},
      {ObjectFormat::ELF, 4, 4},
      {ObjectFormat::ELF, 41, 4},
      {ObjectFormat::ELF, 42, 4},
      {ObjectFormat::MachO, 1, 4},
      {ObjectFormat::COFF, 4, 4},
  }};
  for (const auto &Test : Cases) {
    for (const uint8_t Width :
         {uint8_t{0}, uint8_t{1}, uint8_t{4}, uint8_t{8}}) {
      LinkGraph Graph(Target::X86_64, Endianness::Little);
      ASSERT_TRUE(Graph.beginInput("input.o"));
      auto Text = Graph.addSection(
          Section{".text",
                  SectionKind::Text,
                  1,
                  16,
                  0,
                  0,
                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
      ASSERT_TRUE(Text);
      auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
      ASSERT_TRUE(TargetSymbol);
      Relocation Value{*Text, 0, Test.Type, *TargetSymbol, 0};
      Value.Format = Test.Format;
      Value.PatchSize = Width;
      auto Result = Graph.addRelocation(Value);
      EXPECT_EQ(static_cast<bool>(Result), Width == Test.Width);
      if (!Result) {
        EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
      }
    }
  }
}

TEST(LinkGraphTest, RejectsUnsupportedX86RelocationBeforeRangeValidation) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Unknown{*Text, 0, 0xFFFF, *TargetSymbol, 0};
  Unknown.PatchSize = 1;
  auto Result = Graph.addRelocation(Unknown);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "unsupported relocation patch size");
}

TEST(LinkGraphTest, WrongPatchSizeCannotBypassOverlapDetection) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text",
              SectionKind::Text,
              1,
              16,
              0,
              0,
              {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation First{*Text, 0, 1, *TargetSymbol, 0};
  First.PatchSize = 8;
  ASSERT_TRUE(Graph.addRelocation(First));
  Relocation Bypass{*Text, 7, 2, *TargetSymbol, 0};
  Bypass.PatchSize = 1;
  auto Result = Graph.addRelocation(Bypass);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
}

TEST(LinkGraphTest, ValidationRejectsMutatedRelocationPatchSize) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Value{*Text, 0, 1, *TargetSymbol, 0};
  Value.PatchSize = 8;
  ASSERT_TRUE(Graph.addRelocation(Value));
  const_cast<std::vector<Relocation> &>(Graph.relocations())[0].PatchSize = 1;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "invalid relocation patch size");
}

TEST(LinkGraphTest, ValidationRejectsMutatedNonAdjacentRelocationOverlap) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{.Name = ".text",
              .Kind = SectionKind::Text,
              .Alignment = 1,
              .VirtualSize = 12,
              .Content = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  for (const uint64_t Offset : {UINT64_C(0), UINT64_C(4), UINT64_C(8)}) {
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, Offset, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  }

  const_cast<std::vector<Relocation> &>(Graph.relocations())[2].Offset = 2;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, ValidationRejectsMutatedRISCVSymbolDifferenceTriple) {
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  const Relocation Add{*Data, 0,     llvm::ELF::R_RISCV_ADD32, *TargetSymbol,
                       0,     false, ObjectFormat::ELF,        4};
  const Relocation Sub{*Data, 0,     llvm::ELF::R_RISCV_SUB32, *TargetSymbol,
                       0,     false, ObjectFormat::ELF,        4};
  ASSERT_TRUE(Graph.addRelocation(Add));
  ASSERT_TRUE(Graph.addRelocation(Sub));
  const_cast<std::vector<Relocation> &>(Graph.relocations()).push_back(Add);

  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping relocation patches");
}

TEST(LinkGraphTest, RejectsOverlappingRebasesRegardlessOfOrder) {
  for (const bool Reverse : {false, true}) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("input.o"));
    auto Data = Graph.addSection(Section{".data",
                                         SectionKind::Data,
                                         1,
                                         12,
                                         0,
                                         0,
                                         {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
    ASSERT_TRUE(Data);
    const Rebase First{*Data, 1, 1, 0, 8};
    const Rebase Second{*Data, 7, 1, 0, 4};
    ASSERT_TRUE(Graph.addRebase(Reverse ? Second : First));
    auto Result = Graph.addRebase(Reverse ? First : Second);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
  }
}

TEST(LinkGraphTest, AllowsUnsortedAdjacentRebases) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(Section{".data",
                                       SectionKind::Data,
                                       1,
                                       12,
                                       0,
                                       0,
                                       {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  for (const uint64_t Offset : {UINT64_C(8), UINT64_C(0), UINT64_C(4)})
    ASSERT_TRUE(Graph.addRebase(Rebase{*Data, Offset, 1, 0, 4}));
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, ValidationRejectsMutatedNonAdjacentRebaseOverlap) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(
      Section{.Name = ".data",
              .Kind = SectionKind::Data,
              .Alignment = 1,
              .VirtualSize = 12,
              .Content = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  for (const uint64_t Offset : {UINT64_C(0), UINT64_C(4), UINT64_C(8)})
    ASSERT_TRUE(Graph.addRebase(Rebase{*Data, Offset, 1, 0, 4}));

  const_cast<std::vector<Rebase> &>(Graph.rebases())[2].Offset = 2;
  auto Result = Graph.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
}

TEST(LinkGraphTest, ValidatesLargeSymbolAndRelocationGraph) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("stress.o"));
  constexpr uint32_t RecordCount = 20000;
  std::vector<WasmEdge::Byte> Content(RecordCount * 4);
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 4, Content.size(), 0, 0, std::move(Content)});
  ASSERT_TRUE(Data);
  for (uint32_t I = 0; I < RecordCount; ++I) {
    auto SymbolValue = Graph.addSymbol(
        Symbol{"stress" + std::to_string(I), *Data, I * 4, 0, false});
    ASSERT_TRUE(SymbolValue);
    ASSERT_TRUE(Graph.addRelocation(Relocation{*Data, I * 4, 2, *SymbolValue, 0,
                                               false, ObjectFormat::ELF, 4}));
  }

  EXPECT_TRUE(Graph.validate());
  EXPECT_FALSE(Graph.addSymbol(Symbol{"stress19999", *Data, 0, 0, false}));
}

TEST(LinkGraphTest, RejectsRebaseOverlappingLaterSuccessor) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 24, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(Data);
  ASSERT_TRUE(Graph.addRebase(Rebase{*Data, 8, 1, 0, 4}));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Data, 20, 1, 0, 4}));

  auto Result = Graph.addRebase(Rebase{*Data, 0, 1, 0, 12});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "overlapping rebase patches");
}

TEST(LinkGraphTest, PatchIndexesSurviveGraphCopiesAndMoves) {
  LinkGraph Original(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Original.beginInput("input.o"));
  auto First = Original.addSection(Section{
      ".first", SectionKind::Data, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  auto Second = Original.addSection(Section{
      ".second", SectionKind::Data, 1, 8, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}});
  ASSERT_TRUE(First && Second);
  auto TargetSymbol = Original.addSymbol(Symbol{"target", *First, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Original.addRelocation(
      Relocation{*First, 0, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Original.addRebase(Rebase{*First, 4, 1, 0, 4}));

  LinkGraph Copied = Original;
  EXPECT_FALSE(Copied.addRelocation(
      Relocation{*First, 2, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  EXPECT_FALSE(Copied.addRebase(Rebase{*First, 6, 1, 0, 2}));
  EXPECT_TRUE(Copied.addRelocation(Relocation{*Second, 0, 2, *TargetSymbol, 0,
                                              false, ObjectFormat::ELF, 4}));
  EXPECT_TRUE(Copied.addRebase(Rebase{*Second, 0, 1, 0, 4}));

  LinkGraph Moved = std::move(Original);
  EXPECT_FALSE(Moved.addRelocation(
      Relocation{*First, 2, 2, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  EXPECT_FALSE(Moved.addRebase(Rebase{*First, 6, 1, 0, 2}));
}

TEST(LinkGraphTest, RejectsInvalidPatchSections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);

  auto RelocationResult = Graph.addRelocation(
      Relocation{InvalidSectionId, 0, 42, *TargetSymbol, 0});
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Message, "invalid section ID");
  auto RebaseResult = Graph.addRebase(Rebase{InvalidSectionId, 0, 7, 0});
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Message, "invalid section ID");
}

TEST(LinkGraphTest, RejectsPatchOffsetsOutsideSectionContent) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 4, 0, 0, {0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);

  Relocation Outside{*Text, 1, 42, *TargetSymbol, 0};
  Outside.PatchSize = 4;
  auto RelocationResult = Graph.addRelocation(Outside);
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Offset, 1U);
  EXPECT_EQ(RelocationResult.error().Message,
            "relocation offset is outside section content");
  auto RebaseResult = Graph.addRebase(Rebase{*Text, 1, 7, 0});
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Offset, 1U);
  EXPECT_EQ(RebaseResult.error().Message,
            "rebase offset is outside section content");
}

TEST(LinkGraphTest, ProvidesSectionPlacementMutators) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 1, 5, 0, 0, {0, 0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Mutable{*Text, 0, 42, *TargetSymbol, 0};
  Mutable.PatchSize = 4;
  ASSERT_TRUE(Graph.addRelocation(Mutable));
  ASSERT_TRUE(Graph.addRebase(Rebase{*Text, 4, 7, 0}));

  ASSERT_TRUE(Graph.setSectionAddress(*Text, 64));
  ASSERT_TRUE(Graph.setSectionFileOffset(*Text, 32));
  const_cast<std::vector<Relocation> &>(Graph.relocations())[0].Addend = 8;
  const_cast<std::vector<Rebase> &>(Graph.rebases())[0].Addend = 16;
  EXPECT_EQ(Graph.sections()[*Text].Address, 64U);
  EXPECT_EQ(Graph.sections()[*Text].FileOffset, 32U);
  EXPECT_EQ(Graph.sections()[*Text].Content[0], 0U);
  EXPECT_EQ(Graph.relocations()[0].Addend, 8);
  EXPECT_EQ(Graph.rebases()[0].Addend, 16);

  EXPECT_FALSE(Graph.setSectionAddress(InvalidSectionId, 0));
  EXPECT_FALSE(Graph.setSectionFileOffset(InvalidSectionId, 0));
}

TEST(LinkGraphTest, ValidatesMutatedSectionInvariants) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{".text", SectionKind::Text, 1, 2, 0, 0, {0}});
  ASSERT_TRUE(Text);

  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[*Text].Alignment = 0;
  auto AlignmentResult = Graph.validate();
  ASSERT_FALSE(AlignmentResult);
  EXPECT_EQ(AlignmentResult.error().Message,
            "section alignment must be a non-zero power of two");

  Sections[*Text].Alignment = 1;
  Sections[*Text].VirtualSize = 0;
  auto SizeResult = Graph.validate();
  ASSERT_FALSE(SizeResult);
  EXPECT_EQ(SizeResult.error().Message,
            "section content exceeds section virtual size");
}

TEST(LinkGraphTest, ValidatesMutatedPatchOffsets) {
  LinkGraph RelocationGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(RelocationGraph.beginInput("input.o"));
  auto Text = RelocationGraph.addSection(
      Section{".text", SectionKind::Text, 1, 4, 0, 0, {0, 0, 0, 0}});
  ASSERT_TRUE(Text);
  auto TargetSymbol =
      RelocationGraph.addSymbol(Symbol{"target", *Text, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  Relocation Mutable{*Text, 0, 42, *TargetSymbol, 0};
  Mutable.PatchSize = 4;
  ASSERT_TRUE(RelocationGraph.addRelocation(Mutable));
  const_cast<std::vector<Relocation> &>(RelocationGraph.relocations())[0]
      .Offset = 1;
  auto RelocationResult = RelocationGraph.validate();
  ASSERT_FALSE(RelocationResult);
  EXPECT_EQ(RelocationResult.error().Message,
            "relocation offset is outside section content");

  LinkGraph RebaseGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(RebaseGraph.beginInput("input.o"));
  Text = RebaseGraph.addSection(
      Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}});
  ASSERT_TRUE(Text);
  ASSERT_TRUE(RebaseGraph.addRebase(Rebase{*Text, 0, 7, 0}));
  const_cast<std::vector<Rebase> &>(RebaseGraph.rebases())[0].Offset = 1;
  auto RebaseResult = RebaseGraph.validate();
  ASSERT_FALSE(RebaseResult);
  EXPECT_EQ(RebaseResult.error().Message,
            "rebase offset is outside section content");
}

TEST(LinkGraphTest, SymbolIdsRemainStableAcrossVectorGrowth) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Bss = Graph.addSection(Section{".bss", SectionKind::BSS, 1, 64});
  ASSERT_TRUE(Bss);

  auto First = Graph.addSymbol(Symbol{"first", *Bss, 0, 1, false});
  ASSERT_TRUE(First);
  for (uint32_t I = 1; I < 64; ++I) {
    ASSERT_TRUE(Graph.addSymbol(
        Symbol{"symbol" + std::to_string(I), *Bss, I, 1, false}));
  }

  EXPECT_EQ(*First, 0U);
  EXPECT_EQ(Graph.symbols()[*First].Name, "first");
}

TEST(LinkGraphTest, SectionOffsetsDefaultToZero) {
  Section Value{".data", SectionKind::Data, 4};
  EXPECT_EQ(Value.Address, 0U);
  EXPECT_EQ(Value.FileOffset, 0U);
  EXPECT_EQ(Value.VirtualSize, 0U);
}

TEST(LinkGraphTest, StoresValidOrderedCompactUnwindRecords) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       32,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       0x1000});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 8, 8, 0, 0, {}});
  auto ReadOnly = Graph.addSection(
      Section{"__const", SectionKind::ReadOnly, 8, 8, 0, 0, {}});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          8,
                                          8,
                                          0,
                                          0,
                                          {},
                                          SectionPurpose::EHFrame});
  ASSERT_TRUE(Text && Data && ReadOnly && EHFrame);
  auto First = Graph.addSymbol(Symbol{"first", *Text, 0, 0, false});
  auto Second = Graph.addSymbol(Symbol{"second", *Text, 16, 0, false});
  auto Personality = Graph.addSymbol(Symbol{"personality", *Data, 0, 0, false});
  auto LSDA = Graph.addSymbol(Symbol{"lsda", *ReadOnly, 0, 0, false});
  auto FDE = Graph.addSymbol(Symbol{"fde", *EHFrame, 0, 0, false});
  ASSERT_TRUE(First && Second && Personality && LSDA && FDE);

  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*First, 16, 0x02000000, *Personality, *LSDA, *FDE}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 8, 0x03000000, {}, {}, {}}));
  ASSERT_EQ(Graph.compactUnwind().size(), 2U);
  EXPECT_EQ(Graph.compactUnwind()[0].Function, *First);
  EXPECT_EQ(Graph.compactUnwind()[1].Encoding, 0x03000000U);
  EXPECT_TRUE(Graph.validate());
}

TEST(LinkGraphTest, RejectsInvalidCompactUnwindFunctionsAndRanges) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       16,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       UINT64_MAX - 7});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 4, 16, 0, 0, {}});
  ASSERT_TRUE(Text && Data);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 8, 0, false});
  auto NonText = Graph.addSymbol(Symbol{"data", *Data, 0, 0, false});
  ASSERT_TRUE(Function && NonText);
  const auto Reject = [&](CompactUnwindRecord Record,
                          std::string_view Message) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  Reject({InvalidSymbolId, 1, 0, {}, {}, {}},
         "invalid compact unwind function symbol ID");
  Reject({*NonText, 1, 0, {}, {}, {}},
         "compact unwind function must reference text section");
  Reject({*Function, 0, 0, {}, {}, {}},
         "compact unwind function length must be non-zero");
  Reject({*Function, 9, 0, {}, {}, {}},
         "compact unwind function range exceeds text section");
  Reject({*Function, 8, 0, {}, {}, {}},
         "compact unwind function address overflows");
  EXPECT_TRUE(Graph.compactUnwind().empty());
}

TEST(LinkGraphTest, RejectsInvalidCompactUnwindOptionalSymbols) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"__text", SectionKind::Text, 4, 8, 0, 0, {}});
  auto BSS =
      Graph.addSection(Section{"__bss", SectionKind::BSS, 4, 8, 0, 0, {}});
  auto Unwind = Graph.addSection(
      Section{"__unwind", SectionKind::Unwind, 4, 8, 0, 0, {}});
  auto Data =
      Graph.addSection(Section{"__data", SectionKind::Data, 4, 8, 0, 0, {}});
  auto ReadOnly = Graph.addSection(
      Section{"__const", SectionKind::ReadOnly, 4, 8, 0, 0, {}});
  auto EHFrame = Graph.addSection(Section{"__eh_frame",
                                          SectionKind::Unwind,
                                          4,
                                          8,
                                          0,
                                          0,
                                          {},
                                          SectionPurpose::EHFrame});
  auto Empty =
      Graph.addSection(Section{"__empty", SectionKind::Data, 4, 0, 0, 0, {}});
  ASSERT_TRUE(Text && BSS && Unwind && Data && ReadOnly && EHFrame && Empty);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 0, false});
  auto BSSSymbol = Graph.addSymbol(Symbol{"bss", *BSS, 0, 0, false});
  auto WrongFDE = Graph.addSymbol(Symbol{"fde", *Unwind, 0, 0, false});
  auto PersonalityEnd =
      Graph.addSymbol(Symbol{"personality_end", *Data, 8, 0, false});
  auto LSDAEnd = Graph.addSymbol(Symbol{"lsda_end", *ReadOnly, 8, 0, false});
  auto FDEEnd = Graph.addSymbol(Symbol{"fde_end", *EHFrame, 8, 0, false});
  auto EmptySymbol = Graph.addSymbol(Symbol{"empty", *Empty, 0, 0, false});
  ASSERT_TRUE(Function && BSSSymbol && WrongFDE && PersonalityEnd && LSDAEnd &&
              FDEEnd && EmptySymbol);
  const auto Reject = [&](CompactUnwindRecord Record,
                          std::string_view Message) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  Reject({*Function, 1, 0, InvalidSymbolId, {}, {}},
         "invalid compact unwind personality symbol ID");
  Reject({*Function, 1, 0, {}, InvalidSymbolId, {}},
         "invalid compact unwind LSDA symbol ID");
  Reject({*Function, 1, 0, {}, {}, InvalidSymbolId},
         "invalid compact unwind FDE symbol ID");
  Reject({*Function, 1, 0, *BSSSymbol, {}, {}},
         "compact unwind personality must reference a non-BSS section");
  Reject({*Function, 1, 0, {}, *BSSSymbol, {}},
         "compact unwind LSDA must reference a non-BSS section");
  Reject({*Function, 1, 0, {}, {}, *WrongFDE},
         "compact unwind FDE must reference an EH frame section");

  const auto RejectOutside = [&](CompactUnwindRecord Record, SymbolId Symbol,
                                 SectionId Section, std::string_view SymbolName,
                                 std::string_view SectionName) {
    auto Result = Graph.addCompactUnwind(Record);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message,
              "compact unwind symbol is outside section storage");
    EXPECT_EQ(Result.error().Symbol, Symbol);
    EXPECT_EQ(Result.error().Section, Section);
    EXPECT_EQ(Result.error().Offset, Graph.symbols()[Symbol].Offset);
    EXPECT_EQ(Result.error().SymbolName, SymbolName);
    EXPECT_EQ(Result.error().SectionName, SectionName);
  };
  RejectOutside({*Function, 1, 0, *PersonalityEnd, {}, {}}, *PersonalityEnd,
                *Data, "personality_end", "__data");
  RejectOutside({*Function, 1, 0, {}, *LSDAEnd, {}}, *LSDAEnd, *ReadOnly,
                "lsda_end", "__const");
  RejectOutside({*Function, 1, 0, {}, {}, *FDEEnd}, *FDEEnd, *EHFrame,
                "fde_end", "__eh_frame");
  RejectOutside({*Function, 1, 0, *EmptySymbol, {}, {}}, *EmptySymbol, *Empty,
                "empty", "__empty");
}

TEST(LinkGraphTest, RejectsUnorderedAndOverlappingCompactUnwindRecords) {
  const auto MakeGraph = [] {
    LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
    EXPECT_TRUE(Graph.beginInput("input.o"));
    EXPECT_TRUE(Graph.addSection(Section{"__text",
                                         SectionKind::Text,
                                         4,
                                         32,
                                         0,
                                         0,
                                         {},
                                         SectionPurpose::Default,
                                         0x1000}));
    EXPECT_TRUE(Graph.addSymbol(Symbol{"first", 0, 0, 0, false}));
    EXPECT_TRUE(Graph.addSymbol(Symbol{"second", 0, 8, 0, false}));
    return Graph;
  };
  for (const auto &Second : {CompactUnwindRecord{0, 1, 0, {}, {}, {}},
                             CompactUnwindRecord{1, 1, 0, {}, {}, {}},
                             CompactUnwindRecord{1, 8, 0, {}, {}, {}}}) {
    auto Graph = MakeGraph();
    const CompactUnwindRecord First =
        Second.Function == 1 && Second.Length == 8
            ? CompactUnwindRecord{0, 16, 0, {}, {}, {}}
            : CompactUnwindRecord{1, 1, 0, {}, {}, {}};
    ASSERT_TRUE(Graph.addCompactUnwind(First));
    auto Result = Graph.addCompactUnwind(Second);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message,
              Second.Function == 0 ? "compact unwind records are unordered"
              : Second.Length == 1
                  ? "duplicate compact unwind function address"
                  : "overlapping compact unwind function ranges");
  }
}

TEST(LinkGraphTest, RejectsCompactUnwindMutationAfterRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"__text",
                                       SectionKind::Text,
                                       4,
                                       16,
                                       0,
                                       0,
                                       {},
                                       SectionPurpose::Default,
                                       0x1000}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"first", 0, 0, 0, false}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"second", 0, 8, 0, false}));
  ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{0, 8, 0, {}, {}, {}}));
  ASSERT_TRUE(Graph.addCompactUnwind(CompactUnwindRecord{1, 8, 0, {}, {}, {}}));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Late = Graph.addCompactUnwind(CompactUnwindRecord{1, 8, 0, {}, {}, {}});
  ASSERT_FALSE(Late);
  EXPECT_EQ(Late.error().Message, "link graph relocations already applied");
}

TEST(LinkGraphTest, RejectsCompactUnwindForNonMachOGraphs) {
  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::ELF);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"function", 0, 0, 0, false}));
  auto Result =
      Graph.addCompactUnwind(CompactUnwindRecord{0, 1, 0, {}, {}, {}});
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "compact unwind requires a Mach-O link graph");
}

TEST(LayoutTest, GroupsAndAlignsSections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 8, 4, 0, 0, {1, 2, 3, 4}});
  auto Unwind = Graph.addSection(
      Section{".eh_frame", SectionKind::Unwind, 4, 3, 0, 0, {1, 2, 3}});
  auto ReadOnly = Graph.addSection(
      Section{".rodata", SectionKind::ReadOnly, 2, 2, 0, 0, {1, 2}});
  auto TextZ = Graph.addSection(
      Section{".text.z", SectionKind::Text, 4, 3, 0, 0, {1, 2, 3}});
  auto TextA =
      Graph.addSection(Section{".text.a", SectionKind::Text, 8, 1, 0, 0, {1}});
  ASSERT_TRUE(Data && Unwind && ReadOnly && TextZ && TextA);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*TextA].Address, 0U);
  EXPECT_EQ(Graph.sections()[*TextA].FileOffset, 0U);
  EXPECT_EQ(Graph.sections()[*TextZ].Address, 4U);
  EXPECT_EQ(Graph.sections()[*TextZ].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*ReadOnly].Address, 8U);
  EXPECT_EQ(Graph.sections()[*ReadOnly].FileOffset, 8U);
  EXPECT_EQ(Graph.sections()[*Unwind].Address, 12U);
  EXPECT_EQ(Graph.sections()[*Unwind].FileOffset, 12U);
  EXPECT_EQ(Graph.sections()[*Data].Address, 16U);
  EXPECT_EQ(Graph.sections()[*Data].FileOffset, 16U);
}

TEST(LayoutTest, BssConsumesVirtualButNotFileSpace) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto First =
      Graph.addSection(Section{"a", SectionKind::Data, 1, 3, 0, 0, {1, 2, 3}});
  auto Second =
      Graph.addSection(Section{"z", SectionKind::Data, 4, 1, 0, 0, {1}});
  auto Bss = Graph.addSection(Section{"b", SectionKind::BSS, 8, 9});
  ASSERT_TRUE(First && Second && Bss);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*First].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Second].Address, 4U);
  EXPECT_EQ(Graph.sections()[*Second].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*Bss].Address, 8U);
  EXPECT_EQ(Graph.sections()[*Bss].FileOffset, 0U);
  EXPECT_EQ(Graph.sections()[*Bss].Address + Graph.sections()[*Bss].VirtualSize,
            17U);

  ASSERT_TRUE(Graph.setSectionFileOffset(*Bss, 123));
  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*Second].FileOffset, 4U);
  EXPECT_EQ(Graph.sections()[*Bss].FileOffset, 0U);
}

TEST(LayoutTest, IsIndependentOfInsertionOrderForUniqueNames) {
  auto MakeGraph = [](bool Reverse) {
    LinkGraph Graph(Target::X86_64, Endianness::Little);
    EXPECT_TRUE(Graph.beginInput("input.o"));
    Section A{"a", SectionKind::ReadOnly, 4, 2, 0, 0, {1, 2}};
    Section Z{"z", SectionKind::ReadOnly, 8, 3, 0, 0, {1, 2, 3}};
    EXPECT_TRUE(Graph.addSection(Reverse ? Z : A));
    EXPECT_TRUE(Graph.addSection(Reverse ? A : Z));
    return Graph;
  };
  auto First = MakeGraph(false);
  auto Second = MakeGraph(true);

  ASSERT_TRUE(layout(First));
  ASSERT_TRUE(layout(Second));
  for (const auto &Section : First.sections()) {
    const auto Other = std::find_if(
        Second.sections().begin(), Second.sections().end(),
        [&](const auto &Value) { return Value.Name == Section.Name; });
    ASSERT_NE(Other, Second.sections().end());
    EXPECT_EQ(Other->Address, Section.Address);
    EXPECT_EQ(Other->FileOffset, Section.FileOffset);
  }
}

TEST(LayoutTest, OrdersDuplicateNamesByOriginalOrdinal) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto First =
      Graph.addSection(Section{"same", SectionKind::Text, 1, 2, 0, 0, {1, 2}});
  auto Second =
      Graph.addSection(Section{"same", SectionKind::Text, 4, 1, 0, 0, {1}});
  ASSERT_TRUE(First && Second);

  ASSERT_TRUE(layout(Graph));
  EXPECT_EQ(Graph.sections()[*First].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Second].Address, 4U);
}

TEST(LayoutTest, AppliesNonzeroImageBase) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text =
      Graph.addSection(Section{"text", SectionKind::Text, 16, 2, 0, 0, {1, 2}});
  ASSERT_TRUE(Text);

  ASSERT_TRUE(layout(Graph, 0x1003));
  EXPECT_EQ(Graph.sections()[*Text].Address, 0x1010U);
  EXPECT_EQ(Graph.sections()[*Text].FileOffset, 0U);
}

TEST(LayoutTest, AlignsUniversalPermissionGroupsToPages) {
  constexpr uint64_t PageSize = 4096;
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto Text = Graph.addSection(Section{".text", SectionKind::Text, 16, 32, 0, 0,
                                       std::vector<WasmEdge::Byte>(32)});
  auto Data = Graph.addSection(Section{".data", SectionKind::Data, 8, 8, 0, 0,
                                       std::vector<WasmEdge::Byte>(8)});
  ASSERT_TRUE(Text && Data);

  ASSERT_TRUE(layout(Graph, 0, PageSize));
  EXPECT_EQ(Graph.sections()[*Text].Address, 0U);
  EXPECT_EQ(Graph.sections()[*Data].Address, PageSize);
}

TEST(LayoutTest, IsIdempotent) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(
      Section{"text", SectionKind::Text, 8, 3, 0, 0, {1, 2, 3}}));
  ASSERT_TRUE(Graph.addSection(Section{"bss", SectionKind::BSS, 16, 7}));
  ASSERT_TRUE(layout(Graph, 0x1000));
  const auto Sections = Graph.sections();

  ASSERT_TRUE(layout(Graph, 0x1000));
  EXPECT_EQ(Graph.sections()[0].Address, Sections[0].Address);
  EXPECT_EQ(Graph.sections()[0].FileOffset, Sections[0].FileOffset);
  EXPECT_EQ(Graph.sections()[1].Address, Sections[1].Address);
  EXPECT_EQ(Graph.sections()[1].FileOffset, Sections[1].FileOffset);
}

TEST(LayoutTest, RejectsAlignmentOverflow) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"text", SectionKind::Text, 2, 0}));

  auto Result = layout(Graph, std::numeric_limits<uint64_t>::max());
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "section address alignment overflows");
  EXPECT_EQ(Result.error().SectionName, "text");
}

TEST(LayoutTest, RejectsSectionSizeAndImageBaseOverflow) {
  LinkGraph SizeGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(SizeGraph.beginInput("input.o"));
  ASSERT_TRUE(SizeGraph.addSection(Section{
      "first", SectionKind::Text, 1, std::numeric_limits<uint64_t>::max()}));
  ASSERT_TRUE(SizeGraph.addSection(Section{"second", SectionKind::Text, 1, 1}));
  auto SizeResult = layout(SizeGraph);
  ASSERT_FALSE(SizeResult);
  EXPECT_EQ(SizeResult.error().Message, "section virtual size overflows");
  EXPECT_EQ(SizeResult.error().SectionName, "second");

  LinkGraph BaseGraph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(BaseGraph.beginInput("input.o"));
  ASSERT_TRUE(BaseGraph.addSection(Section{"text", SectionKind::Text, 1, 3}));
  auto BaseResult = layout(BaseGraph, std::numeric_limits<uint64_t>::max() - 1);
  ASSERT_FALSE(BaseResult);
  EXPECT_EQ(BaseResult.error().Message, "section virtual size overflows");
  EXPECT_EQ(BaseResult.error().SectionName, "text");
}

TEST(LayoutTest, RejectsInvalidMutatedGraphBeforeLayout) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"text", SectionKind::Text, 1, 0}));
  auto &Sections = const_cast<std::vector<Section> &>(Graph.sections());
  Sections[0].Alignment = 0;

  auto Result = layout(Graph);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "section alignment must be a non-zero power of two");
}

TEST(LayoutTest, RetainsAllSectionsIncludingEmptySections) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  ASSERT_TRUE(Graph.addSection(Section{"empty", SectionKind::Text, 8, 0}));
  ASSERT_TRUE(Graph.addSection(
      Section{"unused", SectionKind::ReadOnly, 4, 1, 0, 0, {1}}));
  ASSERT_TRUE(Graph.addSection(Section{"zero", SectionKind::BSS, 16, 0}));

  ASSERT_TRUE(layout(Graph));
  ASSERT_EQ(Graph.sections().size(), 3U);
  EXPECT_EQ(Graph.sections()[0].Address, 0U);
  EXPECT_EQ(Graph.sections()[1].Address, 0U);
  EXPECT_EQ(Graph.sections()[2].Address, 16U);
}

TEST(RelocationFieldTest, ReadsAndWritesUnsignedBoundariesWithExactBytes) {
  struct Case {
    uint8_t Width;
    uint64_t Maximum;
  };
  const std::array<Case, 4> Cases{
      {{1, UINT8_MAX}, {2, UINT16_MAX}, {4, UINT32_MAX}, {8, UINT64_MAX}}};
  for (const auto Endian : {Endianness::Little, Endianness::Big}) {
    for (const auto &Test : Cases) {
      for (const uint64_t Value : {UINT64_C(0), Test.Maximum}) {
        std::array<WasmEdge::Byte, 10> Bytes{};
        ASSERT_TRUE(
            Internal::writeUnsigned(Bytes, 1, Test.Width, Endian, Value));
        for (uint8_t I = 0; I < Test.Width; ++I) {
          EXPECT_EQ(Bytes[1 + I], Value == 0 ? 0 : 0xFF);
        }
        auto Read = Internal::readUnsigned(Bytes, 1, Test.Width, Endian);
        ASSERT_TRUE(Read);
        EXPECT_EQ(*Read, Value);
      }
    }
  }
  std::array<WasmEdge::Byte, 4> Bytes{};
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(Bytes, (std::array<WasmEdge::Byte, 4>{0x78, 0x56, 0x34, 0x12}));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Big,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(Bytes, (std::array<WasmEdge::Byte, 4>{0x12, 0x34, 0x56, 0x78}));
}

TEST(RelocationFieldTest, ReadsAndWritesSignedBoundariesInEitherEndianness) {
  struct Case {
    uint8_t Width;
    int64_t Minimum;
    int64_t Maximum;
  };
  const std::array<Case, 4> Cases{{{1, INT8_MIN, INT8_MAX},
                                   {2, INT16_MIN, INT16_MAX},
                                   {4, INT32_MIN, INT32_MAX},
                                   {8, INT64_MIN, INT64_MAX}}};
  for (const auto &Test : Cases) {
    for (const auto Endian : {Endianness::Little, Endianness::Big}) {
      for (const auto Value : {Test.Minimum, Test.Maximum}) {
        std::array<WasmEdge::Byte, 10> Bytes{};
        ASSERT_TRUE(Internal::writeSigned(Bytes, 1, Test.Width, Endian, Value));
        for (uint8_t I = 0; I < Test.Width; ++I) {
          const bool SignByte =
              Endian == Endianness::Little ? I == Test.Width - 1 : I == 0;
          const uint8_t Expected = Value == Test.Minimum
                                       ? SignByte ? 0x80 : 0x00
                                   : SignByte ? 0x7F
                                              : 0xFF;
          EXPECT_EQ(Bytes[1 + I], Expected);
        }
        auto Read = Internal::readSigned(Bytes, 1, Test.Width, Endian);
        ASSERT_TRUE(Read);
        EXPECT_EQ(*Read, Value);
      }
    }
  }
}

TEST(RelocationFieldTest, AllowsMisalignmentAndRejectsBoundsAndOverflow) {
  std::array<WasmEdge::Byte, 8> Bytes{};
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 1, 4, Endianness::Little,
                                      UINT32_C(0x12345678)));
  EXPECT_EQ(*Internal::readUnsigned(Bytes, 1, 4, Endianness::Little),
            UINT32_C(0x12345678));
  EXPECT_FALSE(Internal::readUnsigned(Bytes, 0, 3, Endianness::Little));
  EXPECT_FALSE(
      Internal::readUnsigned(Bytes, UINT64_MAX, 1, Endianness::Little));
  for (const uint8_t Width : std::array<uint8_t, 4>{1, 2, 4, 8}) {
    const uint64_t Offset = Bytes.size() - Width + 1;
    EXPECT_FALSE(
        Internal::readUnsigned(Bytes, Offset, Width, Endianness::Little));
    EXPECT_FALSE(Internal::readSigned(Bytes, Offset, Width, Endianness::Big));
    EXPECT_FALSE(
        Internal::writeUnsigned(Bytes, Offset, Width, Endianness::Little, 0));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, Offset, Width, Endianness::Big, 0));
  }
  EXPECT_FALSE(Internal::writeUnsigned(Bytes, 0, 1, Endianness::Little, 256));
  EXPECT_FALSE(Internal::writeSigned(Bytes, 0, 1, Endianness::Little, 128));
  EXPECT_FALSE(Internal::writeSigned(Bytes, 0, 1, Endianness::Little, -129));
  for (const uint8_t Width : std::array<uint8_t, 3>{1, 2, 4}) {
    const uint8_t Bits =
        static_cast<uint8_t>(static_cast<uint32_t>(Width) * 8U);
    EXPECT_FALSE(Internal::writeUnsigned(Bytes, 0, Width, Endianness::Little,
                                         UINT64_C(1) << Bits));
    const int64_t Maximum = (INT64_C(1) << (Bits - 1)) - 1;
    const int64_t Minimum = -(INT64_C(1) << (Bits - 1));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, 0, Width, Endianness::Big, Maximum + 1));
    EXPECT_FALSE(
        Internal::writeSigned(Bytes, 0, Width, Endianness::Big, Minimum - 1));
  }
}

LinkGraph makeRelocationGraph(
    uint32_t Type, int64_t Addend, bool Implicit = false,
    uint64_t TargetAddress = 0x1100, uint64_t PatchAddress = 0x1000,
    ObjectFormat Format = ObjectFormat::ELF,
    std::vector<WasmEdge::Byte> Bytes = std::vector<WasmEdge::Byte>(24)) {
  LinkGraph Graph(Target::X86_64, Endianness::Little);
  EXPECT_TRUE(Graph.beginInput("input.o"));
  auto Patch =
      Graph.addSection(Section{".text", SectionKind::Text, 1, Bytes.size(),
                               PatchAddress, 0, std::move(Bytes)});
  auto TargetSection = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, TargetAddress, 0, {0}});
  EXPECT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  EXPECT_TRUE(TargetSymbol);
  Relocation Value{*Patch, 1, Type, *TargetSymbol, Addend, Implicit, Format};
  Value.PatchSize = Type == 1 ? 8 : 4;
  EXPECT_TRUE(Graph.addRelocation(Value));
  return Graph;
}

void expectGraphStateEquals(const LinkGraph &Actual,
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
    EXPECT_EQ(Left.PCRelative, Right.PCRelative);
    EXPECT_EQ(Left.External, Right.External);
    EXPECT_EQ(Left.Scattered, Right.Scattered);
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

TEST(RelocationTest, AppliesX86_64AbsoluteAndRecordsOneRebase) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makeRelocationGraph(1, 5);
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 8,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1105U);
  ASSERT_EQ(Graph.rebases().size(), 1U);
  EXPECT_EQ(Graph.rebases()[0].Section, 0U);
  EXPECT_EQ(Graph.rebases()[0].Offset, 1U);
  EXPECT_EQ(Graph.rebases()[0].Width, 8U);
  EXPECT_EQ(Graph.rebases()[0].Type, 1U);
  EXPECT_EQ(Graph.rebases()[0].Format, ObjectFormat::ELF);
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, ReportsCompiledOutKnownTargetAsUnsupported) {
  const std::array<std::pair<Target, Endianness>, 5> Architectures{{
      {Target::X86_64, Endianness::Little},
      {Target::AArch64, Endianness::Little},
      {Target::ARM, Endianness::Little},
      {Target::RISCV64, Endianness::Little},
      {Target::S390X, Endianness::Big},
  }};
  const auto Missing = std::find_if(
      Architectures.begin(), Architectures.end(),
      [](const auto &Value) { return !hasRelocationHandler(Value.first); });
  if (Missing == Architectures.end())
    GTEST_SKIP() << "all relocation handlers are compiled";
  LinkGraph Graph(Missing->first, Missing->second);
  ASSERT_TRUE(Graph.beginInput("compiled-out.o"));
  ASSERT_TRUE(
      Graph.addSection(Section{".text", SectionKind::Text, 1, 1, 0, 0, {0}}));
  const auto Snapshot = Graph;
  auto Result = applyRelocations(Graph);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
  EXPECT_EQ(Result.error().Message, "unsupported relocation target");
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsBigEndianX86_64WithoutMutation) {
  LinkGraph Graph(Target::X86_64, Endianness::Big);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  auto DataSection = Graph.addSection(Section{
      ".data", SectionKind::Data, 1, 8, 0x1000, 0, {1, 2, 3, 4, 5, 6, 7, 8}});
  ASSERT_TRUE(DataSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *DataSection, 0, 8, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{*DataSection, 0, 1, *TargetSymbol,
                                             0, false, ObjectFormat::ELF, 8}));
  const auto Content = Graph.sections()[0].Content;
  EXPECT_FALSE(applyRelocations(Graph));
  EXPECT_EQ(Graph.sections()[0].Content, Content);
  EXPECT_TRUE(Graph.rebases().empty());
  EXPECT_FALSE(Graph.relocationsApplied());
}

TEST(RelocationTest, ChecksX86_64AbsoluteOverflowAndUnderflow) {
  for (const auto &[Address, Addend] :
       std::array<std::pair<uint64_t, int64_t>, 2>{
           {{0, -1}, {UINT64_MAX, 1}}}) {
    auto Graph = makeRelocationGraph(1, Addend, false, Address);
    const auto Content = Graph.sections()[0].Content;
    EXPECT_FALSE(applyRelocations(Graph));
    EXPECT_EQ(Graph.sections()[0].Content, Content);
    EXPECT_TRUE(Graph.rebases().empty());
  }
}

TEST(RelocationTest, ChecksPC32AndPLT32SignedBoundaries) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    int64_t Displacement;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{{INT32_MIN, true},
                                   {INT32_MAX, true},
                                   {static_cast<int64_t>(INT32_MIN) - 1, false},
                                   {static_cast<int64_t>(INT32_MAX) + 1, false},
                                   {-256, true},
                                   {256, true}}};
  constexpr uint64_t PatchAddress = UINT64_C(0x100000000);
  constexpr uint64_t Place = PatchAddress + 1;
  for (const uint32_t Type : {2U, 4U}) {
    for (const auto &Test : Cases) {
      const uint64_t Target =
          Test.Displacement < 0
              ? Place - static_cast<uint64_t>(-Test.Displacement)
              : Place + static_cast<uint64_t>(Test.Displacement);
      auto Graph = makeRelocationGraph(Type, 0, false, Target, PatchAddress);
      auto Result = applyRelocations(Graph);
      EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
      if (Test.Accepted) {
        auto Value = Internal::readSigned(Graph.sections()[0].Content, 1, 4,
                                          Endianness::Little);
        ASSERT_TRUE(Value);
        EXPECT_EQ(*Value, Test.Displacement);
      }
    }
  }
}

TEST(RelocationTest, AppliesExplicitAndImplicitPC32AndPLT32) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    uint32_t Type;
    int64_t Addend;
    bool Implicit;
    uint64_t Target;
    int32_t Expected;
  };
  const std::array<Case, 4> Cases{{{2, -4, false, 0x1100, 0xFB},
                                   {4, -4, false, 0x0F00, -0x105},
                                   {2, 0, true, 0x1100, 0xFB},
                                   {4, 0, true, 0x0F00, -0x105}}};
  for (const auto &Test : Cases) {
    auto Graph = [&] {
      std::vector<WasmEdge::Byte> Bytes(24);
      if (Test.Implicit) {
        EXPECT_TRUE(Internal::writeSigned(
            WasmEdge::Span<WasmEdge::Byte>(Bytes).subspan(1), 0, 4,
            Endianness::Little, -4));
      }
      return makeRelocationGraph(Test.Type, Test.Addend, Test.Implicit,
                                 Test.Target, 0x1000, ObjectFormat::ELF,
                                 std::move(Bytes));
    }();
    ASSERT_TRUE(applyRelocations(Graph));
    auto Value = Internal::readSigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Test.Expected);
  }
}

TEST(RelocationTest, AppliesMachOSignedSuffixBiasExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    uint32_t Type;
    int64_t Suffix;
  };
  const std::array<Case, 4> Cases{{
      {llvm::MachO::X86_64_RELOC_SIGNED, 0},
      {llvm::MachO::X86_64_RELOC_SIGNED_1, 1},
      {llvm::MachO::X86_64_RELOC_SIGNED_2, 2},
      {llvm::MachO::X86_64_RELOC_SIGNED_4, 4},
  }};
  for (const auto &Test : Cases) {
    for (const uint64_t Target : {UINT64_C(0x1100), UINT64_C(0x0F00)}) {
      LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
      ASSERT_TRUE(Graph.beginInput("signed.o"));
      auto Patch = Graph.addSection(
          Section{"__text", SectionKind::Text, 1, 4, 0x1000, 0, {0, 0, 0, 0}});
      auto TargetSection = Graph.addSection(
          Section{"__target", SectionKind::Text, 1, 1, Target, 0, {0}});
      ASSERT_TRUE(Patch && TargetSection);
      auto TargetSymbol =
          Graph.addSymbol(Symbol{"_target", *TargetSection, 0, 1, false});
      ASSERT_TRUE(TargetSymbol);
      ASSERT_TRUE(Graph.addRelocation(
          Relocation{*Patch, 0, Test.Type, *TargetSymbol, 0, true,
                     ObjectFormat::MachO, 4, true, false, false}));
      ASSERT_TRUE(applyRelocations(Graph));
      auto Value = Internal::readSigned(Graph.sections()[*Patch].Content, 0, 4,
                                        Endianness::Little);
      ASSERT_TRUE(Value);
      EXPECT_EQ(*Value, static_cast<int64_t>(Target) - 0x1000 - 4)
          << Test.Suffix;

      LinkGraph Explicit(Target::X86_64, Endianness::Little,
                         ObjectFormat::MachO);
      ASSERT_TRUE(Explicit.beginInput("explicit-signed.o"));
      auto ExplicitPatch = Explicit.addSection(
          Section{"__text", SectionKind::Text, 1, 4, 0x1000, 0, {0, 0, 0, 0}});
      auto ExplicitTarget = Explicit.addSection(
          Section{"__target", SectionKind::Text, 1, 1, Target, 0, {0}});
      ASSERT_TRUE(ExplicitPatch && ExplicitTarget);
      auto ExplicitSymbol =
          Explicit.addSymbol(Symbol{"_target", *ExplicitTarget, 0, 1, false});
      ASSERT_TRUE(ExplicitSymbol);
      ASSERT_TRUE(Explicit.addRelocation(
          Relocation{*ExplicitPatch, 0, Test.Type, *ExplicitSymbol, 7, false,
                     ObjectFormat::MachO, 4, true, false, false}));
      ASSERT_TRUE(applyRelocations(Explicit));
      auto ExplicitValue =
          Internal::readSigned(Explicit.sections()[*ExplicitPatch].Content, 0,
                               4, Endianness::Little);
      ASSERT_TRUE(ExplicitValue);
      EXPECT_EQ(*ExplicitValue,
                static_cast<int64_t>(Target) + 7 - 0x1000 - 4 - Test.Suffix);
    }
  }
}

TEST(RelocationTest, RelocatesGeneratedMachOSignedSuffixExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Original = makeObject(llvm::Triple("x86_64-apple-macosx"));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Original.data()),
                          Original.size()),
          "signed.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
    llvm::object::DataRefImpl SectionRef;
    SectionRef.d.a = Ref.d.a;
    RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
    break;
  }
  ASSERT_NE(RelocationOffset, 0U);
  for (const uint32_t Type :
       {llvm::MachO::X86_64_RELOC_SIGNED_1, llvm::MachO::X86_64_RELOC_SIGNED_2,
        llvm::MachO::X86_64_RELOC_SIGNED_4}) {
    auto Object = Original;
    uint32_t Word = 0;
    std::memcpy(&Word, Object.data() + RelocationOffset + 4, sizeof(Word));
    Word = (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28);
    std::memcpy(Object.data() + RelocationOffset + 4, &Word, sizeof(Word));
    auto Graph = ObjectReader::read(Object, Target::X86_64);
    ASSERT_TRUE(Graph) << Type;
    ASSERT_EQ(Graph->relocations().size(), 1U);
    EXPECT_EQ(Graph->relocations()[0].Type, Type);
    ASSERT_TRUE(layout(*Graph, 0, 4096));
    const auto Relocation = Graph->relocations()[0];
    ASSERT_TRUE(applyRelocations(*Graph));
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t S =
        Graph->sections()[Symbol.Section].Address + Symbol.Offset;
    const uint64_t P =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    auto Value =
        Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                             Relocation.Offset, 4, Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, static_cast<int64_t>(S) - static_cast<int64_t>(P) - 4);
  }
}

TEST(RelocationTest, DecodesImplicitAddendForX86_64Absolute) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  std::vector<WasmEdge::Byte> Bytes(24);
  ASSERT_TRUE(
      Internal::writeSigned(WasmEdge::Span<WasmEdge::Byte>(Bytes).subspan(1), 0,
                            8, Endianness::Little, -5));
  auto Graph = makeRelocationGraph(1, 0, true, 0x1100, 0x1000,
                                   ObjectFormat::ELF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 8,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x10FBU);
}

TEST(RelocationTest, RejectsOverflowAndDoesNotPartiallyModifyGraph) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makeRelocationGraph(1, 5);
  auto FarSection = Graph.addSection(Section{
      ".far", SectionKind::Data, 1, 1, UINT64_C(0x8000000000000001), 0, {0}});
  ASSERT_TRUE(FarSection);
  auto SecondSymbol =
      Graph.addSymbol(Symbol{"second", *FarSection, 0, 1, false});
  ASSERT_TRUE(SecondSymbol);
  ASSERT_TRUE(Graph.addRelocation(Relocation{0, 12, 1, *SecondSymbol, INT64_MAX,
                                             false, ObjectFormat::ELF, 8}));
  const auto Snapshot = Graph;
  auto Result = applyRelocations(Graph);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("overflows"), std::string::npos);
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_TRUE(Result.error().RelocationType);
  EXPECT_TRUE(Result.error().Offset);
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsGeneratedRebaseOverlappingExistingRebase) {
  auto Graph = makeRelocationGraph(1, 5);
  ASSERT_TRUE(Graph.addRebase(Rebase{0, 4, 1, 0, 8}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsInvalidGraphUnsupportedTargetTypeAndFormat) {
  auto InvalidSection = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(InvalidSection.relocations())[0]
      .Section = InvalidSectionId;
  EXPECT_FALSE(applyRelocations(InvalidSection));

  auto InvalidSymbol = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(InvalidSymbol.relocations())[0].Symbol =
      InvalidSymbolId;
  EXPECT_FALSE(applyRelocations(InvalidSymbol));

  auto UnsupportedType = makeRelocationGraph(2, 0);
  auto &UnsupportedTypeValue =
      const_cast<std::vector<Relocation> &>(UnsupportedType.relocations())[0];
  UnsupportedTypeValue.Type = 0xFFFF;
  auto UnsupportedTypeResult = applyRelocations(UnsupportedType);
  ASSERT_FALSE(UnsupportedTypeResult);
  EXPECT_EQ(UnsupportedTypeResult.error().Kind, DiagnosticKind::Malformed);
  auto UnsupportedFormat = makeRelocationGraph(2, -4);
  const_cast<std::vector<Relocation> &>(UnsupportedFormat.relocations())[0]
      .Format = ObjectFormat::COFF;
  EXPECT_FALSE(applyRelocations(UnsupportedFormat));
}

TEST(RelocationTest, RelaxesX86_64RexGotpcrelxForDefinedSymbol) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  std::vector<WasmEdge::Byte> Content(24);
  Content[0] = 0x48;
  Content[1] = 0x8B;
  Content[2] = 0x05;
  auto Graph = makeRelocationGraph(42, -4, false, 0x1100, 0x1000,
                                   ObjectFormat::ELF, std::move(Content));
  auto &RelocationValue =
      const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
  RelocationValue.PatchSize = 4;
  RelocationValue.Offset = 3;
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(Graph.sections()[0].Content[1], 0x8DU);
  auto Value = Internal::readSigned(Graph.sections()[0].Content, 3, 4,
                                    Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0xF9);
}

TEST(RelocationTest, ValidatesGotpcrelxInstructionAndAddend) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    uint32_t Type;
    int64_t Addend;
    std::array<WasmEdge::Byte, 3> Prefix;
    uint64_t Offset;
    bool Accepted;
  };
  const std::array<Case, 7> Cases{{
      {41, -4, {0, 0x8B, 0x05}, 3, true},
      {42, -4, {0x48, 0x8B, 0x05}, 3, true},
      {42, 0, {0x48, 0x8B, 0x05}, 3, false},
      {42, -4, {0x48, 0x89, 0x05}, 3, false},
      {42, -4, {0x48, 0x8B, 0x04}, 3, false},
      {42, -4, {0x90, 0x8B, 0x05}, 3, false},
      {41, -4, {0x8B, 0x05, 0x90}, 2, true},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    std::copy(Test.Prefix.begin(), Test.Prefix.end(), Bytes.begin());
    auto Graph =
        makeRelocationGraph(Test.Type, Test.Addend, false, 0x1100, 0x1000,
                            ObjectFormat::ELF, std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = Test.Offset;
    RelocationValue.PatchSize = 4;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
  }
}

TEST(RelocationTest, RelaxesExactGotpcrelxIndirectCallAndJump) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    WasmEdge::Byte ModRM;
    std::array<WasmEdge::Byte, 6> ExpectedPrefixAndPatch;
  };
  const std::array<Case, 2> Cases{{
      {0x15, {0x67, 0xE8, 0xFA, 0x00, 0x00, 0x00}},
      {0x25, {0xE9, 0xFB, 0x00, 0x00, 0x00, 0x90}},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    Bytes[0] = 0xFF;
    Bytes[1] = Test.ModRM;
    auto Graph = makeRelocationGraph(41, -4, false, 0x1100, 0x1000,
                                     ObjectFormat::ELF, std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = 2;
    RelocationValue.PatchSize = 4;
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_TRUE(std::equal(Test.ExpectedPrefixAndPatch.begin(),
                           Test.ExpectedPrefixAndPatch.end(),
                           Graph.sections()[0].Content.begin()));
  }
}

TEST(RelocationTest, RelaxesGotpcrelxIndirectBranchesAboveFourGiB) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  struct Case {
    WasmEdge::Byte ModRM;
    std::array<WasmEdge::Byte, 6> Expected;
  };
  const std::array<Case, 2> Cases{{
      {0x15, {0x67, 0xE8, 0xFA, 0x00, 0x00, 0x00}},
      {0x25, {0xE9, 0xFB, 0x00, 0x00, 0x00, 0x90}},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(24);
    Bytes[0] = 0xFF;
    Bytes[1] = Test.ModRM;
    auto Graph = makeRelocationGraph(41, -4, false, UINT64_C(0x100000100),
                                     UINT64_C(0x100000000), ObjectFormat::ELF,
                                     std::move(Bytes));
    auto &RelocationValue =
        const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
    RelocationValue.Offset = 2;
    RelocationValue.PatchSize = 4;
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_TRUE(std::equal(Test.Expected.begin(), Test.Expected.end(),
                           Graph.sections()[0].Content.begin()));
  }
}

TEST(RelocationTest, RejectsGotpcrelxIndirectBranchDisplacementOverflow) {
  std::vector<WasmEdge::Byte> Bytes(24);
  Bytes[0] = 0xFF;
  Bytes[1] = 0x15;
  auto Graph = makeRelocationGraph(41, -4, false, UINT64_C(0x80001007), 0x1000,
                                   ObjectFormat::ELF, std::move(Bytes));
  auto &RelocationValue =
      const_cast<std::vector<Relocation> &>(Graph.relocations())[0];
  RelocationValue.Offset = 2;
  RelocationValue.PatchSize = 4;
  EXPECT_FALSE(applyRelocations(Graph));
}

TEST(RelocationTest, RejectsMutationAndLayoutAfterRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  auto Graph = makeRelocationGraph(2, -4);
  auto EHFrame = Graph.addSection(
      Section{".eh_frame", SectionKind::Unwind, 8, 8, 0, 0,
              std::vector<WasmEdge::Byte>(8), SectionPurpose::EHFrame});
  ASSERT_TRUE(EHFrame);
  ASSERT_TRUE(applyRelocations(Graph));
  const auto ExpectRelocated = [](const auto &Result) {
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, "link graph relocations already applied");
  };
  ExpectRelocated(Graph.beginInput("again.o"));
  ExpectRelocated(Graph.addSection(Section{"new", SectionKind::Data, 1, 0}));
  ExpectRelocated(Graph.addSymbol(Symbol{"new", 0, 0, 0, false}));
  ExpectRelocated(Graph.addRelocation(Relocation{0, 8, 2, 0, -4}));
  ExpectRelocated(Graph.addRebase(Rebase{0, 8, 1, 0, 8}));
  const auto EHFrameReferences = Graph.ehFrameReferences();
  ExpectRelocated(Graph.addEHFrameReference(EHFrameReference{*EHFrame, 0, 0}));
  EXPECT_EQ(Graph.ehFrameReferences().size(), EHFrameReferences.size());
  ExpectRelocated(Graph.setSectionAddress(0, 3));
  ExpectRelocated(Graph.setSectionFileOffset(0, 3));
  auto LayoutResult = layout(Graph);
  ASSERT_FALSE(LayoutResult);
  EXPECT_EQ(LayoutResult.error().Message, "cannot layout relocated link graph");
}

TEST(RelocationTest, ReadsLayoutsAndRelocatesX86_64ObjectEndToEnd) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object = makeX86_64ELFReaderFixture();
  ASSERT_EQ(Object.size(), 848U);
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto Text =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const Section &Value) { return Value.Name == ".text"; });
  ASSERT_NE(Text, Graph->sections().end());
  ASSERT_GE(Text->Content.size(), 7U);
  EXPECT_EQ(Text->Content[1], 0x8DU);
  auto Displacement =
      Internal::readSigned(Text->Content, 3, 4, Endianness::Little);
  ASSERT_TRUE(Displacement);
  const auto &Relocation = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  EXPECT_EQ(*Displacement,
            static_cast<int64_t>(Graph->sections()[Symbol.Section].Address +
                                 Symbol.Offset) -
                static_cast<int64_t>(Text->Address + Relocation.Offset) - 4);
}

LinkGraph makeELFRelocationGraph(
    Target Architecture, Endianness Endian, uint32_t Type, uint8_t Width,
    uint64_t TargetAddress, uint64_t PatchAddress = 0x1000, uint64_t Offset = 0,
    int64_t Addend = 0, bool Implicit = false,
    std::vector<WasmEdge::Byte> Bytes = std::vector<WasmEdge::Byte>(16),
    ObjectFormat Format = ObjectFormat::ELF) {
  LinkGraph Graph(Architecture, Endian, Format);
  EXPECT_TRUE(Graph.beginInput("input.o"));
  auto Patch =
      Graph.addSection(Section{".text", SectionKind::Text, 4, Bytes.size(),
                               PatchAddress, 0, std::move(Bytes)});
  auto TargetSection = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, TargetAddress, 0, {0}});
  EXPECT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  EXPECT_TRUE(TargetSymbol);
  EXPECT_TRUE(Graph.addRelocation(Relocation{
      *Patch, Offset, Type, *TargetSymbol, Addend, Implicit, Format, Width}));
  return Graph;
}

void setTargetThumb(LinkGraph &Graph, bool Thumb = true) {
  ASSERT_FALSE(Graph.symbols().empty());
  const_cast<Symbol &>(Graph.symbols().back()).Thumb = Thumb;
}

TEST(ARMRelocationTest, AppliesDataRelocationsAndPreservesPrel31TopBit) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  auto Absolute = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, 2, 4, 0x2000, 0x1000, 0, 0, true, [] {
        std::vector<WasmEdge::Byte> Bytes(16);
        EXPECT_TRUE(Internal::writeSigned(Bytes, 0, 4, Endianness::Little, -4));
        return Bytes;
      }());
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            0x1FFCU);
  ASSERT_EQ(Absolute.rebases().size(), 1U);
  EXPECT_EQ(Absolute.rebases()[0].Width, 4U);

  auto Relative = makeELFRelocationGraph(Target::ARM, Endianness::Little, 3, 4,
                                         0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Relative));
  EXPECT_EQ(*Internal::readSigned(Relative.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);

  std::vector<WasmEdge::Byte> PrelBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(PrelBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x80000004)));
  auto Prel =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 42, 4, 0x1100,
                             0x1000, 0, 0, true, std::move(PrelBytes));
  ASSERT_TRUE(applyRelocations(Prel));
  EXPECT_EQ(*Internal::readUnsigned(Prel.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x80000104));
}

TEST(ARMRelocationTest, AppliesThumbStateToDataAddresses) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  for (const auto Type : {llvm::ELF::R_ARM_ABS32, llvm::ELF::R_ARM_REL32}) {
    auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little, Type,
                                        4, 0x2000, 0x1000);
    setTargetThumb(Graph);
    ASSERT_TRUE(applyRelocations(Graph));
    const auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                              Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Type == llvm::ELF::R_ARM_ABS32 ? 0x2001U : 0x1001U);
  }
}

TEST(ARMRelocationTest, ConvertsCrossStateCallsAndRejectsJump) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> ArmBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(ArmBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xEB000000)));
  auto ArmCall = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                        llvm::ELF::R_ARM_CALL, 4, 0x1004,
                                        0x1000, 0, 0, false, ArmBytes);
  setTargetThumb(ArmCall);
  ASSERT_TRUE(applyRelocations(ArmCall));
  EXPECT_EQ(*Internal::readUnsigned(ArmCall.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xFA000001));

  auto ArmJump = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_JUMP24, 4, 0x1004,
      0x1000, 0, 0, false, std::move(ArmBytes));
  setTargetThumb(ArmJump);
  const auto JumpSnapshot = ArmJump;
  auto JumpResult = applyRelocations(ArmJump);
  ASSERT_FALSE(JumpResult);
  EXPECT_EQ(JumpResult.error().Kind, DiagnosticKind::Unsupported);
  expectGraphStateEquals(ArmJump, JumpSnapshot);

  std::vector<WasmEdge::Byte> ThumbBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(ThumbBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto ThumbCall = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
      0x1000, 0, 0, false, std::move(ThumbBytes));
  ASSERT_TRUE(applyRelocations(ThumbCall));
  EXPECT_EQ(*Internal::readUnsigned(ThumbCall.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xE802F000));

  std::vector<WasmEdge::Byte> MisalignedBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(MisalignedBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto Misaligned = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1002,
      0x1000, 0, 0, false, std::move(MisalignedBytes));
  EXPECT_FALSE(applyRelocations(Misaligned));
}

TEST(ARMRelocationTest, NormalizesArmBlxToBlForArmTarget) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFA000000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_CALL, 4, 0x1004, 0x1000,
                                      0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEB000001));
}

TEST(ARMRelocationTest, DecodesArmBlxImplicitHBit) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFB000000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_CALL, 4, 0x1006, 0x1000,
                                      0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEB000002));
}

TEST(ARMRelocationTest, NormalizesThumbBlxToBlForThumbTarget) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xE800F000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
                                      0x1000, 0, 0, true, std::move(Bytes));
  setTargetThumb(Graph);
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xF802F000));
}

TEST(ARMRelocationTest, EncodesArmCallAndChecksRangeAlignmentAndEndianness) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  for (const auto &[Delta, Accepted] :
       std::array<std::pair<int64_t, bool>, 6>{{{4, true},
                                                {-4, true},
                                                {33554428, true},
                                                {-33554432, true},
                                                {2, false},
                                                {33554432, false}}}) {
    const uint64_t Patch = UINT64_C(0x4000000);
    const uint64_t Target = Delta < 0 ? Patch - static_cast<uint64_t>(-Delta)
                                      : Patch + static_cast<uint64_t>(Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0xEB000000)));
    auto Graph =
        makeELFRelocationGraph(Target::ARM, Endianness::Little, 28, 4, Target,
                               Patch, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Accepted);
    if (!Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }

  auto Big = makeELFRelocationGraph(Target::ARM, Endianness::Big, 3, 4, 0x1100);
  const auto Snapshot = Big;
  EXPECT_FALSE(applyRelocations(Big));
  expectGraphStateEquals(Big, Snapshot);
}

TEST(ARMRelocationTest, DecodesImplicitBranchAddendFromImm24) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xEBFFFFFE)));
  auto Graph =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 28, 4, 0x1004,
                             0x1000, 0, 0, true, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xEBFFFFFF));
}

TEST(ARMRelocationTest, EncodesThumbCallBoundariesAndImplicitAddend) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  constexpr uint32_t ThumbBl = UINT32_C(0xF800F000);
  constexpr uint64_t PatchAddress = UINT64_C(0x2000000);
  struct Case {
    int64_t Delta;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{
      {-INT64_C(16777216), true},
      {INT64_C(16777214), true},
      {-INT64_C(16777218), false},
      {INT64_C(16777216), false},
      {-2, true},
      {1, false},
  }};
  for (const auto &Test : Cases) {
    const uint64_t TargetAddress =
        Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                       : PatchAddress + static_cast<uint64_t>(Test.Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, ThumbBl));
    auto Graph = makeELFRelocationGraph(
        Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4,
        TargetAddress, PatchAddress, 0, 0, false, std::move(Bytes));
    setTargetThumb(Graph);
    const auto Snapshot = Graph;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }

  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFFFFF7FF)));
  auto Implicit = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                         llvm::ELF::R_ARM_THM_CALL, 4, 0x1002,
                                         0x1000, 0, 0, true, std::move(Bytes));
  setTargetThumb(Implicit);
  ASSERT_TRUE(applyRelocations(Implicit));
  EXPECT_EQ(*Internal::readUnsigned(Implicit.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xF800F000));
}

TEST(ARMRelocationTest, RejectsMalformedThumbCallInstructionAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x8000F000)));
  auto Graph = makeELFRelocationGraph(Target::ARM, Endianness::Little,
                                      llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
                                      0x1000, 0, 0, false, std::move(Bytes));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);

  std::vector<WasmEdge::Byte> MisalignedBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(MisalignedBytes, 1, 4, Endianness::Little,
                                      UINT32_C(0xF800F000)));
  auto Misaligned = makeELFRelocationGraph(
      Target::ARM, Endianness::Little, llvm::ELF::R_ARM_THM_CALL, 4, 0x1004,
      0x1000, 1, 0, false, std::move(MisalignedBytes));
  const auto MisalignedSnapshot = Misaligned;
  EXPECT_FALSE(applyRelocations(Misaligned));
  expectGraphStateEquals(Misaligned, MisalignedSnapshot);
}

TEST(ARMRelocationTest, ReadsGeneratedThumbAndCantUnwindObject) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.thumb
.section .text,"ax",%progbits
.globl caller
.thumb_func
.type caller,%function
caller:
.fnstart
  bl target
  bx lr
.cantunwind
.fnend
.section .text.target,"ax",%progbits
.thumb_func
.type target,%function
target:
  bx lr
)",
                         "+thumb-mode");
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(ObjectBytes.data()),
                          ObjectBytes.size()),
          "thumb.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  std::set<uint64_t> Types;
  for (const auto &Section : (*Object)->sections()) {
    for (const auto &Relocation : Section.relocations()) {
      Types.insert(Relocation.getType());
    }
  }
  EXPECT_EQ(Types, (std::set<uint64_t>{llvm::ELF::R_ARM_THM_CALL,
                                       llvm::ELF::R_ARM_PREL31}));
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  const auto TargetSymbol =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Value) { return Value.Name == "target"; });
  ASSERT_NE(TargetSymbol, Graph->symbols().end());
  EXPECT_TRUE(TargetSymbol->Thumb);
  EXPECT_EQ(TargetSymbol->Offset & 1, 0U);
  const auto Exidx = std::find_if(
      Graph->sections().begin(), Graph->sections().end(),
      [](const auto &Section) { return Section.Name == ".ARM.exidx"; });
  ASSERT_NE(Exidx, Graph->sections().end());
  EXPECT_EQ(Exidx->Kind, SectionKind::Unwind);
  EXPECT_FALSE(Exidx->Content.empty());
  ASSERT_TRUE(Exidx->LinkedSection);
  EXPECT_EQ(Graph->sections()[*Exidx->LinkedSection].Name, ".text");
  const auto ExidxId =
      static_cast<SectionId>(Exidx - Graph->sections().begin());
  EXPECT_TRUE(std::any_of(Graph->relocations().begin(),
                          Graph->relocations().end(), [&](const auto &Value) {
                            return Value.Section == ExidxId &&
                                   Value.Type == llvm::ELF::R_ARM_PREL31;
                          }));
}

TEST(ARMRelocationTest, RelocatesGeneratedThumbAndCantUnwindObject) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabihf"),
                         R"(.syntax unified
.thumb
.section .text,"ax",%progbits
.globl caller
.thumb_func
.type caller,%function
caller:
.fnstart
  bl target
  bx lr
.cantunwind
.fnend
.section .text.target,"ax",%progbits
.thumb_func
.type target,%function
target:
  bx lr
)",
                         "+thumb-mode");
  auto Graph = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  EXPECT_TRUE(applyRelocations(*Graph));
}

TEST(ARMRelocationTest, RejectsGeneratedPersonalityImport) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes =
      makeObject(llvm::Triple("armv7-unknown-linux-gnueabihf"), false, false,
                 "f0", {}, true, true, "generic", "+thumb-mode", true);
  auto Result = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_EQ(Result.error().SymbolName, "__aeabi_unwind_cpp_pr0");
}

TEST(ARMRelocationTest, RejectsGeneratedDivisionImport) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto ObjectBytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), R"(.syntax unified
.thumb
.text
.globl divide
.thumb_func
divide:
  bl __aeabi_uldivmod
  bx lr
)",
      "+thumb-mode");
  auto Result = ObjectReader::read(ObjectBytes, Target::ARM);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "undefined symbol");
  EXPECT_EQ(Result.error().SymbolName, "__aeabi_uldivmod");
}

TEST(ARMRelocationTest, AcceptsNoneOnlyAtZeroWidth) {
  REQUIRE_RELOCATION_HANDLER(Target::ARM);
  auto None =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 0, 0, 0x1000);
  ASSERT_TRUE(applyRelocations(None));
}

TEST(ARMRelocationTest, RejectsUnsupportedAtomically) {
  auto Unsupported =
      makeELFRelocationGraph(Target::ARM, Endianness::Little, 2, 4, 0x1100);
  const_cast<std::vector<Relocation> &>(Unsupported.relocations())[0].Type = 99;
  const auto Snapshot = Unsupported;
  auto Result = applyRelocations(Unsupported);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Malformed);
  expectGraphStateEquals(Unsupported, Snapshot);
}

TEST(AArch64RelocationTest, AppliesAbsolutePrelAndCallRelocations) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  auto Absolute = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                         0x101, 8, 0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Little),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  auto Prel = makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x105,
                                     4, 0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Prel));
  EXPECT_EQ(*Internal::readSigned(Prel.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);

  for (const auto &[Delta, Accepted] :
       std::array<std::pair<int64_t, bool>, 6>{{{4, true},
                                                {-4, true},
                                                {134217724, true},
                                                {-134217728, true},
                                                {2, false},
                                                {134217728, false}}}) {
    const uint64_t Patch = UINT64_C(0x20000000);
    const uint64_t Target = Delta < 0 ? Patch - static_cast<uint64_t>(-Delta)
                                      : Patch + static_cast<uint64_t>(Delta);
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x94000000)));
    auto Graph =
        makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x11B, 4,
                               Target, Patch, 0, 0, false, std::move(Bytes));
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Accepted);
  }
}

TEST(AArch64RelocationTest, EncodesPageAndScaledLow12Relocations) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  std::vector<WasmEdge::Byte> AdrBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(AdrBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x90000000)));
  auto Adr = makeELFRelocationGraph(Target::AArch64, Endianness::Little, 0x113,
                                    4, 0x201000, 0x1FFFFC, 0, 0, false,
                                    std::move(AdrBytes));
  ASSERT_TRUE(applyRelocations(Adr));
  EXPECT_EQ(*Internal::readUnsigned(Adr.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xD0000000));

  struct Case {
    uint32_t Type;
    uint32_t Instruction;
    uint64_t Address;
    uint32_t Expected;
    bool Accepted;
  };
  const std::array<Case, 10> Cases{{
      {0x115, 0x91000000, 0x1ABC, 0x912AF000, true},
      {0x116, 0x39000000, 0x1ABC, 0x392AF000, true},
      {0x11C, 0x79000000, 0x1ABC, 0x79157800, true},
      {0x11D, 0xB9000000, 0x1ABC, 0xB90ABC00, true},
      {0x11E, 0xF9000000, 0x1AB8, 0xF9055C00, true},
      {0x12B, 0x3D800000, 0x1AB0, 0x3D82AC00, true},
      {0x11E, 0xF9000000, 0x1ABC, 0, false},
      {0x11E, 0xD503201F, 0x1AB8, 0, false},
      {0x11E, 0xBD400000, 0x1AB8, 0, false},
      {0x11E, 0x3DC00000, 0x1AB8, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        Test.Type, 4, Test.Address, 0x1000, 0,
                                        0, false, std::move(Bytes));
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, EncodesMachOPageOff12InstructionClasses) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  struct Case {
    uint32_t Instruction;
    uint64_t Address;
    uint32_t Expected;
    bool Accepted;
  };
  const std::array<Case, 8> Cases{{
      {0x39000000, 0x1ABC, 0x392AF000, true},
      {0x79000000, 0x1ABC, 0x79157800, true},
      {0xB9000000, 0x1ABC, 0xB90ABC00, true},
      {0xF9000000, 0x1AB8, 0xF9055C00, true},
      {0x3D800000, 0x1AB0, 0x3D82AC00, true},
      {0xF9000000, 0x1ABC, 0, false},
      {0x3D800000, 0x1AB8, 0, false},
      {0xD503201F, 0x1ABC, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        Test.Instruction));
    auto Graph = makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                                        llvm::MachO::ARM64_RELOC_PAGEOFF12, 4,
                                        Test.Address, 0x1000, 0, 0, false,
                                        std::move(Bytes), ObjectFormat::MachO);
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, AppliesSignedMachOUnsignedImplicitAddends) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  struct Case {
    uint64_t Target;
    int64_t Addend;
    bool Accepted;
    uint64_t Expected;
  };
  const std::array<Case, 4> Cases{{
      {0x2000, -1, true, 0x1FFF},
      {UINT64_C(0x8000000000001000), INT64_MIN, true, 0x1000},
      {0, -1, false, 0},
      {UINT64_MAX, 1, false, 0},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(
        Internal::writeSigned(Bytes, 0, 8, Endianness::Little, Test.Addend));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little, llvm::MachO::ARM64_RELOC_UNSIGNED,
        8, Test.Target, 0x1000, 0, Test.Addend == 1 ? 1 : 0, Test.Addend != 1,
        std::move(Bytes), ObjectFormat::MachO);
    auto Result = applyRelocations(Graph);
    EXPECT_EQ(static_cast<bool>(Result), Test.Accepted);
    if (Test.Accepted) {
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 8,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(AArch64RelocationTest, RejectsShiftedAddLow12InstructionAtomically) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x91400000)));
  auto Graph = makeELFRelocationGraph(
      Target::AArch64, Endianness::Little, llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
      4, 0x1ABC, 0x1000, 0, 0, false, std::move(Bytes));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(AArch64RelocationTest, AppliesLoadStore64Low12ToDRegisterInstruction) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0xFD400000)));
  auto Graph =
      makeELFRelocationGraph(Target::AArch64, Endianness::Little,
                             llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC, 4, 0x1AB8,
                             0x1000, 0, 0, false, std::move(Bytes));

  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0xFD455C00));
}

TEST(AArch64RelocationTest, ChecksAdrpPageDeltaBoundaries) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr uint64_t PageSize = UINT64_C(1) << 12;
  constexpr uint64_t PatchAddress = UINT64_C(0x200000000);
  struct Case {
    int64_t Pages;
    bool Accepted;
  };
  const std::array<Case, 4> Cases{{
      {-INT64_C(1048576), true},
      {INT64_C(1048575), true},
      {-INT64_C(1048577), false},
      {INT64_C(1048576), false},
  }};
  for (const auto &Test : Cases) {
    const uint64_t TargetAddress =
        Test.Pages < 0
            ? PatchAddress - static_cast<uint64_t>(-Test.Pages) * PageSize
            : PatchAddress + static_cast<uint64_t>(Test.Pages) * PageSize;
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x90000000)));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little,
        llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21, 4, TargetAddress,
        PatchAddress + PageSize - 4, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
    }
  }
}

TEST(AArch64RelocationTest, EncodesAdrpAddAndLoadPairsExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr uint64_t PatchAddress = UINT64_C(0x1000);
  constexpr uint64_t TargetAddress = UINT64_C(0x3AB8);
  for (const auto &[LowType, LowInstruction, ExpectedLow] :
       std::array<std::tuple<uint32_t, uint32_t, uint32_t>, 2>{{
           {llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC, UINT32_C(0x91000000),
            UINT32_C(0x912AE000)},
           {llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC, UINT32_C(0xF9400000),
            UINT32_C(0xF9455C00)},
       }}) {
    LinkGraph Graph(Target::AArch64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("pair.o"));
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x90000000)));
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                        LowInstruction));
    auto Text = Graph.addSection(Section{".text", SectionKind::Text, 4, 8,
                                         PatchAddress, 0, std::move(Bytes)});
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 8, 8, TargetAddress, 0, {0}});
    ASSERT_TRUE(Text && Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 8, false});
    ASSERT_TRUE(TargetSymbol);
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Text, 0, llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
                   *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    ASSERT_TRUE(Graph.addRelocation(Relocation{
        *Text, 4, LowType, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                      Endianness::Little),
              UINT32_C(0xD0000000));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                      Endianness::Little),
              ExpectedLow);
  }
}

TEST(AArch64RelocationTest, RelocatesGeneratedAdrpLow12PairsExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  REQUIRE_LLVM_TARGET("aarch64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeAssemblyObject(llvm::Triple("aarch64-unknown-linux-gnu"),
                         R"(.text
.globl f
.type f,%function
f:
  adrp x0, data
  add x0, x0, :lo12:data
  adrp x1, data
  ldr x1, [x1, :lo12:data]
  ret
.data
.p2align 3
data:
  .xword 0
)"),
      Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 4U);
  const std::array<uint32_t, 4> Types{{
      llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
      llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
      llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
      llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC,
  }};
  for (size_t I = 0; I < Types.size(); ++I) {
    EXPECT_EQ(Graph->relocations()[I].Type, Types[I]);
  }
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &First = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[First.Symbol];
  const uint64_t TargetAddress =
      Graph->sections()[Symbol.Section].Address + Symbol.Offset;
  const uint32_t Low12 = static_cast<uint32_t>(TargetAddress & 0xFFF);
  const auto &Text = Graph->sections()[First.Section];
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 4, 4, Endianness::Little),
            UINT32_C(0x91000000) | (Low12 << 10));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 12, 4, Endianness::Little),
            UINT32_C(0xF9400021) | ((Low12 >> 3) << 10));
}

uint64_t decodeAdrpTargetPage(uint32_t Instruction, uint64_t PatchAddress) {
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

TEST(AArch64RelocationTest, DecodesCOFFImplicitAddendsExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  struct Case {
    uint32_t Type;
    uint32_t Initial;
    uint64_t Target;
    uint64_t Patch;
    uint32_t Expected;
  };
  const std::array<Case, 5> Cases{{
      {llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, 5, ImageBase + 0x2000,
       ImageBase + 0x1000, 0x2005},
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 0x94000002, ImageBase + 0x1100,
       ImageBase + 0x1000, 0x94000042},
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 0x97FFFFFF, ImageBase + 0x1100,
       ImageBase + 0x1000, 0x9400003F},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A, 0x91001400,
       ImageBase + 0x1FFC, ImageBase + 0x1000, 0x91000400},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 0xF9400800,
       ImageBase + 0x1FF8, ImageBase + 0x1000, 0xF9400400},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, Test.Initial));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little, Test.Type, 4, Test.Target,
        Test.Patch, 0, 0, true, std::move(Bytes), ObjectFormat::COFF);
    ASSERT_TRUE(applyRelocations(Graph));
    EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                      Endianness::Little),
              Test.Expected)
        << Test.Type;
  }

  std::vector<WasmEdge::Byte> SIMD(8);
  ASSERT_TRUE(Internal::writeUnsigned(SIMD, 0, 4, Endianness::Little,
                                      UINT32_C(0x3DC00400)));
  auto SIMDGraph = makeELFRelocationGraph(
      Target::AArch64, Endianness::Little,
      llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 4, ImageBase + 0x1FF0,
      ImageBase + 0x1000, 0, 0, true, std::move(SIMD), ObjectFormat::COFF);
  ASSERT_TRUE(applyRelocations(SIMDGraph));
  EXPECT_EQ(*Internal::readUnsigned(SIMDGraph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x3DC00000));
}

TEST(AArch64RelocationTest, ResolvesCOFFPageBaseImplicitByteAddends) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  struct Case {
    uint32_t Initial;
    uint64_t Target;
    uint64_t Patch;
    uint64_t ExpectedPage;
    bool Accepted;
  };
  const std::array<Case, 8> Cases{{
      {UINT32_C(0xB0000000), ImageBase + 0x5000, ImageBase + 0x1000,
       ImageBase + 0x5000, true},
      {UINT32_C(0xF0FFFFE0), ImageBase + 0x5001, ImageBase + 0x1000,
       ImageBase + 0x5000, true},
      {UINT32_C(0xB0000000), ImageBase + (UINT64_C(1048575) << 12), ImageBase,
       ImageBase + (UINT64_C(1048575) << 12), true},
      {UINT32_C(0xF0FFFFE0), ImageBase - (UINT64_C(1048576) << 12) + 1,
       ImageBase, ImageBase - (UINT64_C(1048576) << 12), true},
      {UINT32_C(0xB0008000), ImageBase + (UINT64_C(1048575) << 12), ImageBase,
       0, false},
      {UINT32_C(0xF0FF7FE0), ImageBase - (UINT64_C(1048576) << 12), ImageBase,
       0, false},
      {UINT32_C(0xB0000000), UINT64_MAX, ImageBase, 0, false},
      {UINT32_C(0xF0FFFFE0), 0, ImageBase, 0, false},
  }};
  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(
        Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little, Test.Initial));
    auto Graph = makeELFRelocationGraph(
        Target::AArch64, Endianness::Little,
        llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21, 4, Test.Target, Test.Patch,
        0, 0, true, std::move(Bytes), ObjectFormat::COFF);
    const auto Snapshot = Graph;
    ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
        << Test.Initial;
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
      continue;
    }
    const auto Instruction = Internal::readUnsigned(Graph.sections()[0].Content,
                                                    0, 4, Endianness::Little);
    ASSERT_TRUE(Instruction);
    EXPECT_EQ(
        decodeAdrpTargetPage(static_cast<uint32_t>(*Instruction), Test.Patch),
        Test.ExpectedPage);
  }
}

TEST(AArch64RelocationTest, RejectsMalformedCOFFImplicitInstructions) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  for (const auto Type : {llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A,
                          llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L}) {
    std::vector<WasmEdge::Byte> Bytes(8);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0xD503201F)));
    auto Graph =
        makeELFRelocationGraph(Target::AArch64, Endianness::Little, Type, 4,
                               UINT64_C(0x180002000), UINT64_C(0x180001000), 0,
                               0, true, std::move(Bytes), ObjectFormat::COFF);
    const auto Snapshot = Graph;
    EXPECT_FALSE(applyRelocations(Graph)) << Type;
    expectGraphStateEquals(Graph, Snapshot);
  }
}

TEST(AArch64RelocationTest, RelocatesGeneratedCOFFAddendsExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  const auto Object =
      makeAssemblyObject(llvm::Triple("aarch64-pc-windows-msvc"), R"(
.text
.globl f
.globl target
f:
  adrp x0, data+4097
  add x0, x0, :lo12:data+4097
  adrp x1, data-1
  add x1, x1, :lo12:data-1
  bl target+8
target:
  ret
.data
.p2align 4
.globl data
data:
  .xword 0
)");
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 4U);
  ASSERT_TRUE(PEWriter::layout(*Graph));
  const auto Before =
      Graph->sections()[Graph->relocations()[0].Section].Content;
  EXPECT_TRUE(std::any_of(Before.begin(), Before.end(),
                          [](WasmEdge::Byte Value) { return Value != 0; }));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &Text = Graph->sections()[Graph->relocations()[0].Section];
  const auto DataSymbol =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "data"; });
  ASSERT_NE(DataSymbol, Graph->symbols().end());
  const uint64_t DataAddress =
      Graph->sections()[DataSymbol->Section].Address + DataSymbol->Offset;
  const auto FirstADRP =
      Internal::readUnsigned(Text.Content, 0, 4, Endianness::Little);
  const auto SecondADRP =
      Internal::readUnsigned(Text.Content, 8, 4, Endianness::Little);
  ASSERT_TRUE(FirstADRP && SecondADRP);
  EXPECT_EQ(
      decodeAdrpTargetPage(static_cast<uint32_t>(*FirstADRP), Text.Address),
      (DataAddress + 4097) & ~UINT64_C(0xFFF));
  EXPECT_EQ(decodeAdrpTargetPage(static_cast<uint32_t>(*SecondADRP),
                                 Text.Address + 8),
            (DataAddress - 1) & ~UINT64_C(0xFFF));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 4, 4, Endianness::Little),
            UINT32_C(0x91000000) | (((DataAddress + 4097) & 0xFFF) << 10));
  EXPECT_EQ(*Internal::readUnsigned(Text.Content, 12, 4, Endianness::Little),
            UINT32_C(0x91000021) | (((DataAddress - 1) & 0xFFF) << 10));
}

TEST(RISCVRelocationTest, AppliesAbsoluteCallAndUnwindRelocations) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  auto Absolute = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 2,
                                         8, 0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Little),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  std::vector<WasmEdge::Byte> CallBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000097)));
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x000080E7)));
  auto Call = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 18, 8,
                                     0x11234, 0x10000, 0, 0, false,
                                     std::move(CallBytes));
  ASSERT_TRUE(applyRelocations(Call));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001097));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x234080E7));

  auto Unwind = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 57,
                                       4, 0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(Unwind));
  EXPECT_EQ(*Internal::readSigned(Unwind.sections()[0].Content, 0, 4,
                                  Endianness::Little),
            -0xFC);
}

TEST(RISCVRelocationTest, ReadsGeneratedSymbolDifferenceObject) {
#if LLVM_VERSION_MAJOR == 12
  GTEST_SKIP() << "LLVM 12's RISC-V assembler cannot represent cross-section "
                  "symbol differences; direct relocation logic is tested "
                  "separately";
#endif
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  constexpr std::string_view Assembly = R"(
    .pushsection .rodata.symbol_begin,"a",@progbits
  begin:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_end,"a",@progbits
  end:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_difference,"a",@progbits
    .globl symbol_difference
  symbol_difference:
    .word end - begin
    .popsection
  )";
  const auto Object =
      makeObject(llvm::Triple("riscv64-unknown-linux-gnu"), false, false, "f0",
                 {}, true, true, "generic-rv64", "+a", false, false, false,
                 false, false, false, Assembly.data());
  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    if (Relocation.Type == llvm::ELF::R_RISCV_ADD32 ||
        Relocation.Type == llvm::ELF::R_RISCV_SUB32) {
      Types.insert(Relocation.Type);
      EXPECT_EQ(Relocation.PatchSize, 4U);
      EXPECT_FALSE(Relocation.PCRelative);
    }
  }
  EXPECT_EQ(Types, (std::set<uint32_t>{llvm::ELF::R_RISCV_ADD32,
                                       llvm::ELF::R_RISCV_SUB32}));
}

TEST(RISCVRelocationTest, ReadsGeneratedByteSymbolDifferenceObject) {
#if LLVM_VERSION_MAJOR == 12
  GTEST_SKIP() << "LLVM 12's RISC-V assembler cannot represent cross-section "
                  "symbol differences; direct relocation logic is tested "
                  "separately";
#endif
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  constexpr std::string_view Assembly = R"(
    .pushsection .rodata.symbol_begin,"a",@progbits
  begin:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_end,"a",@progbits
  end:
    .byte 0
    .popsection
    .pushsection .rodata.symbol_difference,"a",@progbits
  symbol_difference:
    .byte end - begin
    .popsection
  )";
  const auto Object =
      makeObject(llvm::Triple("riscv64-unknown-linux-gnu"), false, false, "f0",
                 {}, true, true, "generic-rv64", "+a", false, false, false,
                 false, false, false, Assembly.data());
  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    if (Relocation.Type == llvm::ELF::R_RISCV_ADD8 ||
        Relocation.Type == llvm::ELF::R_RISCV_SET8 ||
        Relocation.Type == llvm::ELF::R_RISCV_SUB8) {
      Types.insert(Relocation.Type);
      EXPECT_EQ(Relocation.PatchSize, 1U);
      EXPECT_FALSE(Relocation.PCRelative);
    }
  }
  EXPECT_TRUE(Types == (std::set<uint32_t>{llvm::ELF::R_RISCV_ADD8,
                                           llvm::ELF::R_RISCV_SUB8}) ||
              Types == (std::set<uint32_t>{llvm::ELF::R_RISCV_SET8,
                                           llvm::ELF::R_RISCV_SUB8}));
}

TEST(RISCVRelocationTest, AppliesSymbolDifferencePairsModulo32InEitherOrder) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  struct Case {
    uint64_t AddOffset;
    uint64_t SubOffset;
    uint32_t Expected;
  };
  const std::array<Case, 2> Cases{{
      {31, 4, UINT32_C(0x00000015)},
      {4, 31, UINT32_C(0xFFFFFFDF)},
  }};
  for (const auto &Test : Cases) {
    for (const bool Reverse : {false, true}) {
      LinkGraph Graph(Target::RISCV64, Endianness::Little);
      ASSERT_TRUE(Graph.beginInput("difference.o"));
      std::vector<WasmEdge::Byte> Bytes(4);
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                          UINT32_C(0xFFFFFFF0)));
      auto Field = Graph.addSection(Section{".data", SectionKind::Data, 4, 4,
                                            0x1000, 0, std::move(Bytes)});
      auto Symbols = Graph.addSection(Section{".symbols", SectionKind::Data, 1,
                                              32, UINT64_MAX - 31, 0,
                                              std::vector<WasmEdge::Byte>(32)});
      ASSERT_TRUE(Field && Symbols);
      auto AddSymbol =
          Graph.addSymbol(Symbol{"add", *Symbols, Test.AddOffset, 0, false});
      auto SubSymbol =
          Graph.addSymbol(Symbol{"sub", *Symbols, Test.SubOffset, 0, false});
      ASSERT_TRUE(AddSymbol && SubSymbol);
      const Relocation Add{*Field, 0,     llvm::ELF::R_RISCV_ADD32, *AddSymbol,
                           7,      false, ObjectFormat::ELF,        4};
      const Relocation Sub{*Field, 0,     llvm::ELF::R_RISCV_SUB32, *SubSymbol,
                           -3,     false, ObjectFormat::ELF,        4};
      ASSERT_TRUE(Graph.addRelocation(Reverse ? Sub : Add));
      ASSERT_TRUE(Graph.addRelocation(Reverse ? Add : Sub));
      ASSERT_TRUE(applyRelocations(Graph));
      EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[*Field].Content, 0, 4,
                                        Endianness::Little),
                Test.Expected);
    }
  }
}

TEST(RISCVRelocationTest, RejectsMalformedSymbolDifferencePairsAtomically) {
  for (const uint32_t SecondType :
       {llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_32_PCREL}) {
    LinkGraph Graph(Target::RISCV64, Endianness::Little);
    ASSERT_TRUE(Graph.beginInput("malformed.o"));
    auto Data = Graph.addSection(
        Section{".data", SectionKind::Data, 4, 4, 0x1000, 0, {1, 2, 3, 4}});
    ASSERT_TRUE(Data);
    auto TargetSymbol = Graph.addSymbol(Symbol{"symbol", *Data, 0, 0, false});
    ASSERT_TRUE(TargetSymbol);
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Data, 0, llvm::ELF::R_RISCV_ADD32, *TargetSymbol, 0, false,
                   ObjectFormat::ELF, 4}));
    auto Second = Graph.addRelocation(Relocation{
        *Data, 0, SecondType, *TargetSymbol, 0, false, ObjectFormat::ELF, 4});
    EXPECT_FALSE(Second);
    const auto Snapshot = Graph;
    EXPECT_FALSE(applyRelocations(Graph));
    expectGraphStateEquals(Graph, Snapshot);
  }
}

TEST(RISCVRelocationTest, PairsPcrelLowWithMarkedHighSiteAndAllowsRelax) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(12);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 12, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, 23, *TargetSymbol, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, 24, *HighSite, 0, false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, 51, *TargetSymbol, 0, false, ObjectFormat::ELF, 0}));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001297));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x23428293));
}

TEST(RISCVRelocationTest, PairsPcrelLowBeforeHighRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_I, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001297));
  EXPECT_EQ(*Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x23428293));
}

TEST(RISCVRelocationTest, RejectsMalformedHighBeforeMutation) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  LinkGraph Graph(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Graph.beginInput("input.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x00028293)));
  auto Text = Graph.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, 1, 1, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Graph.addSymbol(Symbol{"target", *Data, 0, 1, false});
  auto HighSite = Graph.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_I, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RISCVRelocationTest, RejectsMissingLowPairAndInvalidOpcodes) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  std::vector<WasmEdge::Byte> Bytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000293)));
  auto Missing =
      makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 24, 4, 0x1000,
                             0x1000, 0, 0, false, std::move(Bytes));
  EXPECT_FALSE(applyRelocations(Missing));

  auto BadCall = makeELFRelocationGraph(Target::RISCV64, Endianness::Little, 18,
                                        8, 0x1100, 0x1000);
  EXPECT_FALSE(applyRelocations(BadCall));
}

TEST(RISCVRelocationTest, EncodesCallPltAndPcrelLo12SExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  std::vector<WasmEdge::Byte> CallBytes(16);
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000097)));
  ASSERT_TRUE(Internal::writeUnsigned(CallBytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x000080E7)));
  auto Call = makeELFRelocationGraph(
      Target::RISCV64, Endianness::Little, llvm::ELF::R_RISCV_CALL_PLT, 8,
      0x11234, 0x10000, 0, 0, false, std::move(CallBytes));
  ASSERT_TRUE(applyRelocations(Call));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 0, 4,
                                    Endianness::Little),
            UINT32_C(0x00001097));
  EXPECT_EQ(*Internal::readUnsigned(Call.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x234080E7));

  LinkGraph Store(Target::RISCV64, Endianness::Little);
  ASSERT_TRUE(Store.beginInput("store.o"));
  std::vector<WasmEdge::Byte> Bytes(8);
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                      UINT32_C(0x00000297)));
  ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                      UINT32_C(0x0052A023)));
  auto Text = Store.addSection(
      Section{".text", SectionKind::Text, 4, 8, 0x1000, 0, std::move(Bytes)});
  auto Data = Store.addSection(
      Section{".data", SectionKind::Data, 4, 4, 0x2234, 0, {0}});
  ASSERT_TRUE(Text && Data);
  auto TargetSymbol = Store.addSymbol(Symbol{"target", *Data, 0, 4, false});
  auto HighSite = Store.addSymbol(Symbol{"high", *Text, 0, 0, false});
  ASSERT_TRUE(TargetSymbol && HighSite);
  ASSERT_TRUE(Store.addRelocation(
      Relocation{*Text, 0, llvm::ELF::R_RISCV_PCREL_HI20, *TargetSymbol, 0,
                 false, ObjectFormat::ELF, 4}));
  ASSERT_TRUE(Store.addRelocation(
      Relocation{*Text, 4, llvm::ELF::R_RISCV_PCREL_LO12_S, *HighSite, 0, false,
                 ObjectFormat::ELF, 4}));
  ASSERT_TRUE(applyRelocations(Store));
  EXPECT_EQ(*Internal::readUnsigned(Store.sections()[0].Content, 4, 4,
                                    Endianness::Little),
            UINT32_C(0x2252AA23));
}

TEST(RISCVRelocationTest, ChecksRoundedHighSignedRange) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  constexpr uint64_t PatchAddress = UINT64_C(0x200000000);
  struct Case {
    int64_t Delta;
    bool Accepted;
    int64_t RoundedDelta;
  };
  const std::array<Case, 4> Cases{{
      {-INT64_C(0x80000800), true, -INT64_C(0x80000000)},
      {-INT64_C(0x80000801), false, 0},
      {INT64_C(0x7FFFF7FF), true, INT64_C(0x7FFFF000)},
      {INT64_C(0x7FFFF800), false, 0},
  }};
  for (const uint32_t Type :
       {llvm::ELF::R_RISCV_CALL, llvm::ELF::R_RISCV_CALL_PLT}) {
    for (const auto &Test : Cases) {
      std::vector<WasmEdge::Byte> Bytes(16);
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                          UINT32_C(0x00000097)));
      ASSERT_TRUE(Internal::writeUnsigned(Bytes, 4, 4, Endianness::Little,
                                          UINT32_C(0x000080E7)));
      const uint64_t TargetAddress =
          Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                         : PatchAddress + static_cast<uint64_t>(Test.Delta);
      auto Graph = makeELFRelocationGraph(Target::RISCV64, Endianness::Little,
                                          Type, 8, TargetAddress, PatchAddress,
                                          0, 0, false, std::move(Bytes));
      const auto Snapshot = Graph;
      ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
          << Type << " " << Test.Delta;
      if (!Test.Accepted) {
        expectGraphStateEquals(Graph, Snapshot);
        continue;
      }
      const auto High = Internal::readUnsigned(Graph.sections()[0].Content, 0,
                                               4, Endianness::Little);
      const auto Low = Internal::readUnsigned(Graph.sections()[0].Content, 4, 4,
                                              Endianness::Little);
      ASSERT_TRUE(High && Low);
      const int64_t HighDelta = static_cast<int32_t>(
          static_cast<uint32_t>(*High) & UINT32_C(0xFFFFF000));
      const int64_t LowDelta =
          static_cast<int32_t>(static_cast<uint32_t>(*Low) &
                               UINT32_C(0xFFF00000)) >>
          20;
      EXPECT_EQ(PatchAddress + HighDelta + LowDelta, TargetAddress);
    }
  }

  for (const auto &Test : Cases) {
    std::vector<WasmEdge::Byte> Bytes(16);
    ASSERT_TRUE(Internal::writeUnsigned(Bytes, 0, 4, Endianness::Little,
                                        UINT32_C(0x00000297)));
    const uint64_t TargetAddress =
        Test.Delta < 0 ? PatchAddress - static_cast<uint64_t>(-Test.Delta)
                       : PatchAddress + static_cast<uint64_t>(Test.Delta);
    auto Graph = makeELFRelocationGraph(
        Target::RISCV64, Endianness::Little, llvm::ELF::R_RISCV_PCREL_HI20, 4,
        TargetAddress, PatchAddress, 0, 0, false, std::move(Bytes));
    const auto Snapshot = Graph;
    ASSERT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted)
        << Test.Delta;
    if (!Test.Accepted) {
      expectGraphStateEquals(Graph, Snapshot);
      continue;
    }
    const auto High = Internal::readUnsigned(Graph.sections()[0].Content, 0, 4,
                                             Endianness::Little);
    ASSERT_TRUE(High);
    const int64_t HighDelta = static_cast<int32_t>(
        static_cast<uint32_t>(*High) & UINT32_C(0xFFFFF000));
    EXPECT_EQ(PatchAddress + HighDelta,
              static_cast<uint64_t>(static_cast<int64_t>(PatchAddress) +
                                    Test.RoundedDelta));
  }
}

TEST(S390XRelocationTest, AppliesBigEndianAbsoluteAndPcRelativeRelocations) {
  REQUIRE_RELOCATION_HANDLER(Target::S390X);
  auto Absolute = makeELFRelocationGraph(Target::S390X, Endianness::Big, 22, 8,
                                         0x2000, 0x1000, 0, -8);
  ASSERT_TRUE(applyRelocations(Absolute));
  EXPECT_EQ(*Internal::readUnsigned(Absolute.sections()[0].Content, 0, 8,
                                    Endianness::Big),
            0x1FF8U);
  ASSERT_EQ(Absolute.rebases().size(), 1U);

  auto PC32 = makeELFRelocationGraph(Target::S390X, Endianness::Big, 5, 4,
                                     0x0F00, 0x1000, 0, 4);
  ASSERT_TRUE(applyRelocations(PC32));
  EXPECT_EQ(
      *Internal::readSigned(PC32.sections()[0].Content, 0, 4, Endianness::Big),
      -0xFC);
}

void expectGeneratedRebaseOverlapRejected(Target Architecture,
                                          Endianness Endian, uint32_t Type,
                                          uint8_t Width) {
  if (!hasRelocationHandler(Architecture))
    GTEST_SKIP() << architectureName(Architecture)
                 << " relocation handler is not compiled";
  auto Graph =
      makeELFRelocationGraph(Architecture, Endian, Type, Width, 0x2000);
  ASSERT_TRUE(Graph.addRebase(Rebase{0, 0, Type, 0, Width}));
  const auto Snapshot = Graph;
  EXPECT_FALSE(applyRelocations(Graph));
  expectGraphStateEquals(Graph, Snapshot);
}

TEST(RelocationTest, RejectsGeneratedARMRebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::ARM, Endianness::Little, 2, 4);
}

TEST(RelocationTest, RejectsGeneratedAArch64RebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::AArch64, Endianness::Little,
                                       0x101, 8);
}

TEST(RelocationTest, RejectsGeneratedRISCV64RebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::RISCV64, Endianness::Little, 2,
                                       8);
}

TEST(RelocationTest, RejectsGeneratedS390XRebaseOverlap) {
  expectGeneratedRebaseOverlapRejected(Target::S390X, Endianness::Big, 22, 8);
}

TEST(RelocationTest, GeneratedRebaseIndexPreservesOverlapSemantics) {
  const std::array<Rebase, 3> Existing{{
      {0, 8, 1, 0, 8},
      {0, UINT64_MAX - 7, 1, 0, 8},
      {1, 0, 1, 0, 8},
  }};
  Internal::RebaseIntervalIndex Index(Existing);

  EXPECT_TRUE(Index.insert(0, 0, 8));
  EXPECT_TRUE(Index.insert(0, 16, 8));
  EXPECT_FALSE(Index.insert(0, 7, 1));
  EXPECT_FALSE(Index.insert(0, 15, 2));
  EXPECT_FALSE(Index.insert(0, UINT64_MAX, 1));
  EXPECT_TRUE(Index.insert(1, 8, 8));
}

void expectManyAdjacentAbsoluteRelocations(Target Architecture,
                                           Endianness Endian, uint32_t Type,
                                           uint8_t Width) {
  if (!hasRelocationHandler(Architecture))
    GTEST_SKIP() << architectureName(Architecture)
                 << " relocation handler is not compiled";
  constexpr uint32_t RelocationCount = 20000;
  LinkGraph Graph(Architecture, Endian);
  ASSERT_TRUE(Graph.beginInput("many-absolute.o"));
  auto Data = Graph.addSection(
      Section{".data", SectionKind::Data, Width,
              static_cast<uint64_t>(RelocationCount) * Width, 0x1000, 0,
              std::vector<WasmEdge::Byte>(static_cast<size_t>(RelocationCount) *
                                          Width)});
  auto TargetSection = Graph.addSection(
      Section{".target", SectionKind::Data, 1, 1, 0x2000, 0, {0}});
  ASSERT_TRUE(Data && TargetSection);
  auto TargetSymbol =
      Graph.addSymbol(Symbol{"target", *TargetSection, 0, 1, false});
  ASSERT_TRUE(TargetSymbol);
  for (uint32_t I = 0; I < RelocationCount; ++I) {
    ASSERT_TRUE(Graph.addRelocation(
        Relocation{*Data, static_cast<uint64_t>(I) * Width, Type, *TargetSymbol,
                   0, false, ObjectFormat::ELF, Width}));
  }

  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_EQ(Graph.rebases().size(), RelocationCount);
  for (uint32_t I = 0; I < RelocationCount; ++I) {
    EXPECT_EQ(Graph.rebases()[I].Section, *Data);
    EXPECT_EQ(Graph.rebases()[I].Offset, static_cast<uint64_t>(I) * Width);
    EXPECT_EQ(Graph.rebases()[I].Width, Width);
  }
}

TEST(RelocationTest, AppliesManyAdjacentX86_64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::X86_64, Endianness::Little,
                                        llvm::ELF::R_X86_64_64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentARMAbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::ARM, Endianness::Little,
                                        llvm::ELF::R_ARM_ABS32, 4);
}

TEST(RelocationTest, AppliesManyAdjacentAArch64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::AArch64, Endianness::Little,
                                        llvm::ELF::R_AARCH64_ABS64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentRISCV64AbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::RISCV64, Endianness::Little,
                                        llvm::ELF::R_RISCV_64, 8);
}

TEST(RelocationTest, AppliesManyAdjacentS390XAbsoluteRelocations) {
  expectManyAdjacentAbsoluteRelocations(Target::S390X, Endianness::Big,
                                        llvm::ELF::R_390_64, 8);
}

TEST(S390XRelocationTest, ChecksDoubledDisplacementRangeAndAlignment) {
  REQUIRE_RELOCATION_HANDLER(Target::S390X);
  struct Case {
    int64_t Delta;
    bool Accepted;
  };
  const std::array<Case, 6> Cases{{
      {-INT64_C(4294967296), true},
      {INT64_C(4294967294), true},
      {-INT64_C(4294967298), false},
      {INT64_C(4294967296), false},
      {2, true},
      {1, false},
  }};
  constexpr uint64_t Patch = UINT64_C(0x200000000);
  for (const uint32_t Type : {19U, 20U}) {
    for (const auto &Test : Cases) {
      const uint64_t Target = Test.Delta < 0
                                  ? Patch - static_cast<uint64_t>(-Test.Delta)
                                  : Patch + static_cast<uint64_t>(Test.Delta);
      auto Graph = makeELFRelocationGraph(Target::S390X, Endianness::Big, Type,
                                          4, Target, Patch);
      EXPECT_EQ(static_cast<bool>(applyRelocations(Graph)), Test.Accepted);
    }
  }

  auto Little =
      makeELFRelocationGraph(Target::S390X, Endianness::Little, 5, 4, 0x1100);
  EXPECT_FALSE(applyRelocations(Little));
}

class GeneratedLinuxObjectTest
    : public testing::Test,
      public testing::WithParamInterface<LinuxRelocationCase> {};

TEST_P(GeneratedLinuxObjectTest, ReadsAndRelocatesGeneratedLinuxObjects) {
  const auto &Test = GetParam();
  if (!hasRelocationHandler(Test.Architecture))
    GTEST_SKIP() << architectureName(Test.Architecture)
                 << " relocation handler is not compiled";
  REQUIRE_LLVM_TARGET(Test.Triple);
  std::set<uint64_t> GeneratedTypes;
  for (const bool Optimize : {false, true}) {
    for (const bool Tuned : {false, true}) {
      for (const bool Interruptible : {false, true}) {
        const bool Exceptions = Test.Architecture != Target::ARM;
        const auto ObjectBytes = makeObject(
            llvm::Triple(Test.Triple), false, false, "representative", {}, true,
            true, Tuned ? Test.TunedCPU : "generic", Test.Features, false,
            Optimize, Interruptible, true, true, Exceptions,
            Test.Architecture == Target::RISCV64
                ? ".pushsection .rodata.symbol_begin,\"a\",@progbits\n"
                  "inventory_begin:\n.byte 0\n.popsection\n"
                  ".pushsection .rodata.symbol_end,\"a\",@progbits\n"
                  "inventory_end:\n.byte 0\n.popsection\n"
                  ".pushsection .rodata.symbol_difference,\"a\",@progbits\n"
                  ".word inventory_end - inventory_begin\n"
                  ".popsection\n"
                : "");
        auto Object =
            llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
                llvm::StringRef(
                    reinterpret_cast<const char *>(ObjectBytes.data()),
                    ObjectBytes.size()),
                "inventory.o"));
        ASSERT_TRUE(static_cast<bool>(Object));
        for (const auto &Section : (*Object)->sections()) {
          for (const auto &Relocation : Section.relocations()) {
            ASSERT_LE(Relocation.getType(), UINT32_MAX);
            GeneratedTypes.insert(Relocation.getType());
            EXPECT_TRUE(Test.SupportedTypes.count(Relocation.getType()) != 0)
                << Test.Triple << " unexpected relocation "
                << Relocation.getType();
            ASSERT_TRUE(expectedELFPatchWidth(
                Test.Architecture, static_cast<uint32_t>(Relocation.getType())))
                << Test.Triple << " relocation " << Relocation.getType();
          }
        }
        auto Graph = ObjectReader::read(ObjectBytes, Test.Architecture);
        ASSERT_TRUE(Graph) << Test.Triple;
        for (const std::string_view Name :
             {"memory", "vector", "table", "direct"}) {
          EXPECT_TRUE(std::any_of(
              Graph->symbols().begin(), Graph->symbols().end(),
              [&](const auto &Value) { return Value.Name == Name; }))
              << Test.Triple << " missing representative " << Name;
        }
        if (Exceptions) {
          EXPECT_TRUE(std::any_of(Graph->sections().begin(),
                                  Graph->sections().end(),
                                  [](const auto &Value) {
                                    return Value.Kind == SectionKind::Unwind;
                                  }))
              << Test.Triple << " missing landingpad unwind data";
        }
        for (const auto &Relocation : Graph->relocations()) {
          EXPECT_EQ(expectedELFPatchWidth(Test.Architecture, Relocation.Type),
                    Relocation.PatchSize);
          EXPECT_EQ(Relocation.PCRelative,
                    expectedELFPCRelative(Test.Architecture, Relocation.Type))
              << Test.Triple << " relocation " << Relocation.Type;
        }
        ASSERT_TRUE(layout(*Graph, 0x1000)) << Test.Triple;
        const auto Snapshot = *Graph;
        auto Applied = applyRelocations(*Graph);
        EXPECT_TRUE(Applied) << Test.Triple;
        if (!Applied) {
          expectGraphStateEquals(*Graph, Snapshot);
        }
      }
    }
  }
  for (const auto Type : Test.RequiredTypes) {
    EXPECT_TRUE(GeneratedTypes.count(Type) != 0)
        << Test.Triple << " missing core relocation " << Type;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedLinuxObjectTest,
    testing::Values(
        LinuxRelocationCase{"armv7-unknown-linux-gnueabihf",
                            Target::ARM,
                            "cortex-a8",
                            "",
                            {llvm::ELF::R_ARM_ABS32, llvm::ELF::R_ARM_REL32,
                             llvm::ELF::R_ARM_CALL, llvm::ELF::R_ARM_PREL31},
                            {llvm::ELF::R_ARM_REL32, llvm::ELF::R_ARM_CALL,
                             llvm::ELF::R_ARM_PREL31}},
        LinuxRelocationCase{
            "aarch64-unknown-linux-gnu",
            Target::AArch64,
            "cortex-a53",
            "",
            {llvm::ELF::R_AARCH64_ABS64, llvm::ELF::R_AARCH64_PREL64,
             llvm::ELF::R_AARCH64_PREL32, llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
             llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_CALL26,
             llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC},
            {llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
             llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_CALL26}},
        LinuxRelocationCase{
            "riscv64-unknown-linux-gnu",
            Target::RISCV64,
            "generic-rv64",
            "+a",
            {llvm::ELF::R_RISCV_64, llvm::ELF::R_RISCV_CALL_PLT,
             llvm::ELF::R_RISCV_PCREL_HI20, llvm::ELF::R_RISCV_PCREL_LO12_I,
             llvm::ELF::R_RISCV_PCREL_LO12_S, llvm::ELF::R_RISCV_32_PCREL,
             llvm::ELF::R_RISCV_ADD32, llvm::ELF::R_RISCV_SUB32},
            {llvm::ELF::R_RISCV_CALL_PLT, llvm::ELF::R_RISCV_PCREL_HI20,
             llvm::ELF::R_RISCV_PCREL_LO12_I, llvm::ELF::R_RISCV_ADD32,
             llvm::ELF::R_RISCV_SUB32}},
        LinuxRelocationCase{
            "s390x-unknown-linux-gnu",
            Target::S390X,
            "z13",
            "",
            {llvm::ELF::R_390_PC32, llvm::ELF::R_390_PC32DBL,
             llvm::ELF::R_390_PLT32DBL, llvm::ELF::R_390_64},
            {llvm::ELF::R_390_PC32DBL, llvm::ELF::R_390_PLT32DBL}}),
    linuxRelocationCaseName);

class FailingGeneratedRelocationTest
    : public testing::Test,
      public testing::WithParamInterface<FailingRelocationCase> {};

TEST_P(FailingGeneratedRelocationTest,
       RejectsWrongEndiannessAndArchitectureMismatch) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto ObjectBytes =
      makeObject(llvm::Triple(Test.Triple), false, false, "representative", {},
                 true, true, "generic", Test.Features, false, false, true, true,
                 true, Test.Architecture != Target::ARM);
  LinkGraph WrongEndianGraph(Test.Architecture, Test.WrongEndian);
  ASSERT_TRUE(WrongEndianGraph.beginInput("wrong-endian.o"));
  const auto WrongEndianSnapshot = WrongEndianGraph;
  EXPECT_FALSE(applyRelocations(WrongEndianGraph)) << Test.Triple;
  expectGraphStateEquals(WrongEndianGraph, WrongEndianSnapshot);

  const auto Mismatch =
      Test.Architecture == Target::ARM ? Target::AArch64 : Target::ARM;
  EXPECT_FALSE(ObjectReader::read(ObjectBytes, Mismatch)) << Test.Triple;
}

TEST_P(FailingGeneratedRelocationTest,
       RejectsFailingGeneratedRelocationAtomically) {
  const auto &Test = GetParam();
  if (!hasRelocationHandler(Test.Architecture))
    GTEST_SKIP() << architectureName(Test.Architecture)
                 << " relocation handler is not compiled";
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto ObjectBytes =
      makeObject(llvm::Triple(Test.Triple), false, false, "representative", {},
                 true, true, "generic", Test.Features, false, false, true, true,
                 true, Test.Architecture != Target::ARM);
  auto Graph = ObjectReader::read(ObjectBytes, Test.Architecture);
  ASSERT_TRUE(Graph) << Test.Triple;
  ASSERT_TRUE(layout(*Graph, 0x1000));
  auto Patch = Graph->addSection(
      Section{".failure", SectionKind::Data, Test.Width, Test.Width, 0, 0,
              std::vector<WasmEdge::Byte>(Test.Width)});
  auto TargetSection = Graph->addSection(
      Section{".failure.target", SectionKind::Data, 1, 0, UINT64_MAX});
  ASSERT_TRUE(Patch && TargetSection);
  auto TargetSymbol =
      Graph->addSymbol(Symbol{"failure.target", *TargetSection, 0, 0, false});
  ASSERT_TRUE(TargetSymbol);
  ASSERT_TRUE(Graph->addRelocation(Relocation{*Patch, 0, Test.AbsoluteType,
                                              *TargetSymbol, 1, false,
                                              ObjectFormat::ELF, Test.Width}));
  const auto Snapshot = *Graph;
  EXPECT_FALSE(applyRelocations(*Graph)) << Test.Triple;
  expectGraphStateEquals(*Graph, Snapshot);
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, FailingGeneratedRelocationTest,
    testing::Values(
        FailingRelocationCase{"armv7-unknown-linux-gnueabihf", Target::ARM,
                              Endianness::Big, llvm::ELF::R_ARM_ABS32, 4, ""},
        FailingRelocationCase{"aarch64-unknown-linux-gnu", Target::AArch64,
                              Endianness::Big, llvm::ELF::R_AARCH64_ABS64, 8,
                              ""},
        FailingRelocationCase{"riscv64-unknown-linux-gnu", Target::RISCV64,
                              Endianness::Big, llvm::ELF::R_RISCV_64, 8, "+a"},
        FailingRelocationCase{"s390x-unknown-linux-gnu", Target::S390X,
                              Endianness::Little, llvm::ELF::R_390_64, 8, ""}),
    failingRelocationCaseName);

TEST(RelocationTest, AppliesGeneratedELF64PC32AndPLT32RelocationsExactly) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Graph = ObjectReader::read(
      makeX86_64AssemblyObject(
          ".data\n.globl target_data\ntarget_data:\n.quad 0\n"
          ".globl absolute\nabsolute:\n.quad target_data + 5\n"
          ".text\n.globl caller\ncaller:\n"
          "movl target_data(%rip), %eax\ncall target_func\nret\n"
          ".section .text.target,\"ax\",@progbits\n"
          ".globl target_func\ntarget_func:\nret\n"),
      Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 3U);
  std::array<bool, 3> Seen{};
  for (const auto &Relocation : Graph->relocations()) {
    ASSERT_TRUE(Relocation.Type == 1 || Relocation.Type == 2 ||
                Relocation.Type == 4);
    Seen[Relocation.Type == 1 ? 0 : Relocation.Type == 2 ? 1 : 2] = true;
    EXPECT_EQ(Relocation.Format, ObjectFormat::ELF);
    EXPECT_EQ(Relocation.PatchSize, Relocation.Type == 1 ? 8U : 4U);
    EXPECT_EQ(Relocation.PCRelative, Relocation.Type != 1);
    EXPECT_FALSE(Relocation.AddendIsImplicit);
    EXPECT_EQ(Relocation.Addend, Relocation.Type == 1 ? 5 : -4);
  }
  EXPECT_EQ(Seen, (std::array<bool, 3>{true, true, true}));
  ASSERT_TRUE(layout(*Graph, 0x1000));
  struct ExpectedPatch {
    SectionId Section;
    uint64_t Offset;
    uint8_t Width;
    uint64_t Bits;
  };
  std::vector<ExpectedPatch> Expected;
  for (const auto &Relocation : Graph->relocations()) {
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t S =
        Graph->sections()[Symbol.Section].Address + Symbol.Offset;
    const uint64_t P =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    Expected.push_back(ExpectedPatch{
        Relocation.Section, Relocation.Offset,
        static_cast<uint8_t>(Relocation.Type == 1 ? 8 : 4),
        Relocation.Type == 1 ? S + static_cast<uint64_t>(Relocation.Addend)
                             : static_cast<uint32_t>(static_cast<int64_t>(S) +
                                                     Relocation.Addend -
                                                     static_cast<int64_t>(P))});
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Patch : Expected) {
    auto Value =
        Internal::readUnsigned(Graph->sections()[Patch.Section].Content,
                               Patch.Offset, Patch.Width, Endianness::Little);
    ASSERT_TRUE(Value);
    EXPECT_EQ(*Value, Patch.Bits);
  }
  ASSERT_EQ(Graph->rebases().size(), 1U);
  EXPECT_EQ(Graph->rebases()[0].Width, 8U);
}

class PortableRelocationTest
    : public testing::Test,
      public testing::WithParamInterface<PortableRelocationCase> {};

TEST_P(PortableRelocationTest,
       ReadsLayoutsAndRelocatesCurrentMachOAndCOFFForms) {
  const auto &Test = GetParam();
  REQUIRE_RELOCATION_HANDLER(Test.Architecture);
  REQUIRE_LLVM_TARGET(Test.Triple);
  const llvm::Triple Triple(Test.Triple);
  auto Graph = ObjectReader::read(makeObject(Triple), Test.Architecture);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  EXPECT_EQ(Graph->relocations()[0].Format, Test.Format);
  EXPECT_EQ(Graph->relocations()[0].Type, Test.Type);
  EXPECT_EQ(Graph->relocations()[0].PatchSize, 4U);
  EXPECT_TRUE(Graph->relocations()[0].PCRelative);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  const auto &Relocation = Graph->relocations()[0];
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  const int64_t S = static_cast<int64_t>(
      Graph->sections()[Symbol.Section].Address + Symbol.Offset);
  const int64_t P = static_cast<int64_t>(
      Graph->sections()[Relocation.Section].Address + Relocation.Offset);
  auto Before =
      Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                           Relocation.Offset, 4, Endianness::Little);
  ASSERT_TRUE(Before);
  ASSERT_TRUE(applyRelocations(*Graph));
  auto After =
      Internal::readSigned(Graph->sections()[Relocation.Section].Content,
                           Relocation.Offset, 4, Endianness::Little);
  ASSERT_TRUE(After);
  EXPECT_EQ(*After, S + *Before - P - 4);
}

INSTANTIATE_TEST_SUITE_P(MachO, PortableRelocationTest,
                         testing::Values(PortableRelocationCase{
                             "x86_64-apple-macosx", Target::X86_64,
                             ObjectFormat::MachO, 1}),
                         portableRelocationCaseName);
INSTANTIATE_TEST_SUITE_P(COFF, PortableRelocationTest,
                         testing::Values(PortableRelocationCase{
                             "x86_64-pc-windows-msvc", Target::X86_64,
                             ObjectFormat::COFF, 4}),
                         portableRelocationCaseName);

void expectPortableRelocationForms(
    ObjectFormat Format, Target Architecture,
    WasmEdge::Span<const std::tuple<uint32_t, uint8_t, bool>> Cases) {
  for (const auto &[Type, Width, PCRelative] : Cases) {
    EXPECT_EQ(relocationPatchSize(Format, Architecture, Type, 4), Width);
    EXPECT_EQ(relocationIsPCRelative(Format, Architecture, Type), PCRelative);
  }
}

TEST(RelocationTest, RecognizesObservedX86_64MachORelocationForms) {
  const std::array<std::tuple<uint32_t, uint8_t, bool>, 3> Cases{{
      {llvm::MachO::X86_64_RELOC_SIGNED, 4, true},
      {llvm::MachO::X86_64_RELOC_SIGNED_4, 4, true},
      {llvm::MachO::X86_64_RELOC_BRANCH, 4, true},
  }};
  expectPortableRelocationForms(ObjectFormat::MachO, Target::X86_64, Cases);
}

TEST(RelocationTest, RecognizesObservedAArch64MachORelocationForms) {
  const std::array<std::tuple<uint32_t, uint8_t, bool>, 3> Cases{{
      {llvm::MachO::ARM64_RELOC_BRANCH26, 4, true},
      {llvm::MachO::ARM64_RELOC_PAGE21, 4, true},
      {llvm::MachO::ARM64_RELOC_PAGEOFF12, 4, false},
  }};
  expectPortableRelocationForms(ObjectFormat::MachO, Target::AArch64, Cases);
}

TEST(ObjectReaderTest, PreservesAArch64MachOSectionPageOffRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  constexpr uint32_t TargetOffset = 24;
  auto Object = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.text
.globl _entry
_entry:
  adrp x0, Ldata@PAGE
  add x0, x0, Ldata@PAGEOFF
  adrp x1, Ldata@PAGE
  ldr x1, [x1, Ldata@PAGEOFF]
  ret
.data
Ldata:
  .space 32
)");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "pageoff.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  std::vector<std::pair<size_t, size_t>> PageOffs;
  uint32_t TargetOrdinal = 0;
  for (const auto &Section : MachO->sections()) {
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    auto Contents = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents));
    const size_t Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Object.data());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_PAGEOFF12)
        continue;
      const auto Symbol = Relocation.getSymbol();
      ASSERT_NE(Symbol, MachO->symbol_end());
      auto TargetSection = Symbol->getSection();
      ASSERT_TRUE(static_cast<bool>(TargetSection));
      ASSERT_NE(*TargetSection, MachO->section_end());
      PageOffs.emplace_back(
          Content + static_cast<size_t>(Relocation.getOffset()),
          Header.reloff + Relocation.getRawDataRefImpl().d.b * 8 + 4);
      TargetOrdinal = static_cast<uint32_t>((*TargetSection)->getIndex());
    }
  }
  ASSERT_EQ(PageOffs.size(), 2U);
  ASSERT_NE(TargetOrdinal, 0U);
  for (const auto &[Field, Metadata] : PageOffs) {
    uint32_t Word = read32le(Object, Metadata);
    Word &= ~UINT32_C(0x08FFFFFF);
    Word |= TargetOrdinal;
    write32le(Object, Metadata, Word);

    uint32_t Instruction = read32le(Object, Field);
    const bool LoadStore =
        (Instruction & UINT32_C(0x3B000000)) == UINT32_C(0x39000000);
    const unsigned Scale = LoadStore ? (Instruction >> 30) & 3 : 0;
    ASSERT_EQ(TargetOffset & ((UINT32_C(1) << Scale) - 1), 0U);
    Instruction &= ~UINT32_C(0x003FFC00);
    Instruction |= (TargetOffset >> Scale) << 10;
    write32le(Object, Field, Instruction);
  }

  Parsed = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                      Object.size()),
      "section-pageoff.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  size_t SectionPageOffs = 0;
  for (const auto &Section : (*Parsed)->sections())
    for (const auto &Relocation : Section.relocations())
      SectionPageOffs +=
          Relocation.getType() == llvm::MachO::ARM64_RELOC_PAGEOFF12 &&
          Relocation.getSymbol() == (*Parsed)->symbol_end();
  ASSERT_EQ(SectionPageOffs, 2U);

  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  ASSERT_TRUE(layout(*Graph, 0x4000, 0x4000));
  ASSERT_TRUE(applyRelocations(*Graph));
  size_t Applied = 0;
  for (const auto &Relocation : Graph->relocations()) {
    if (Relocation.Type != llvm::MachO::ARM64_RELOC_PAGEOFF12)
      continue;
    EXPECT_EQ(Relocation.Addend, TargetOffset);
    EXPECT_FALSE(Relocation.AddendIsImplicit);
    const auto Instruction =
        Internal::readUnsigned(Graph->sections()[Relocation.Section].Content,
                               Relocation.Offset, 4, Endianness::Little);
    ASSERT_TRUE(Instruction);
    const bool LoadStore =
        (*Instruction & UINT32_C(0x3B000000)) == UINT32_C(0x39000000);
    const unsigned Scale = LoadStore ? (*Instruction >> 30) & 3 : 0;
    const uint64_t Reconstructed = ((*Instruction & UINT32_C(0x003FFC00)) >> 10)
                                   << Scale;
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    EXPECT_EQ(Reconstructed,
              (Graph->sections()[Symbol.Section].Address + TargetOffset) &
                  UINT64_C(0xFFF));
    ++Applied;
  }
  EXPECT_EQ(Applied, 2U);
}

TEST(ObjectReaderTest, ConsumesAArch64MachOAddendPairs) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Fixture = makeAArch64MachOAddendFixture();
  ASSERT_EQ(Fixture.Relocations.size(), 6U);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 3U);
  const std::array<std::pair<uint32_t, int64_t>, 3> Expected{{
      {llvm::MachO::ARM64_RELOC_BRANCH26, -4},
      {llvm::MachO::ARM64_RELOC_PAGE21, 4097},
      {llvm::MachO::ARM64_RELOC_PAGEOFF12, 4097},
  }};
  for (const auto &Entry : Expected) {
    const uint32_t Type = Entry.first;
    const int64_t Addend = Entry.second;
    const auto Relocation =
        std::find_if(Graph->relocations().begin(), Graph->relocations().end(),
                     [&](const auto &Value) { return Value.Type == Type; });
    ASSERT_NE(Relocation, Graph->relocations().end()) << Type;
    EXPECT_EQ(Relocation->Addend, Addend) << Type;
    EXPECT_FALSE(Relocation->AddendIsImplicit) << Type;
  }

  ASSERT_TRUE(layout(*Graph, 0x4000, 0x4000));
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Relocation : Graph->relocations()) {
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t Target = Graph->sections()[Symbol.Section].Address +
                            Symbol.Offset + Relocation.Addend;
    const uint64_t Patch =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    const auto Word =
        Internal::readUnsigned(Graph->sections()[Relocation.Section].Content,
                               Relocation.Offset, 4, Endianness::Little);
    ASSERT_TRUE(Word);
    if (Relocation.Type == llvm::MachO::ARM64_RELOC_BRANCH26) {
      const uint32_t Immediate =
          static_cast<uint32_t>(*Word) & UINT32_C(0x03FFFFFF);
      int64_t Delta = static_cast<int64_t>(Immediate);
      if ((Immediate & UINT32_C(0x02000000)) != 0)
        Delta -= INT64_C(1) << 26;
      Delta *= 4;
      EXPECT_EQ(static_cast<uint64_t>(static_cast<int64_t>(Patch) + Delta),
                Target);
    } else if (Relocation.Type == llvm::MachO::ARM64_RELOC_PAGE21) {
      EXPECT_EQ(decodeAdrpTargetPage(static_cast<uint32_t>(*Word), Patch),
                Target & ~UINT64_C(0xFFF));
    } else {
      EXPECT_EQ((static_cast<uint32_t>(*Word) >> 10) & UINT32_C(0xFFF),
                Target & UINT64_C(0xFFF));
    }
  }
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOAddendPairs) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Original = makeAArch64MachOAddendFixture();
  ASSERT_EQ(Original.Relocations.size(), 6U);
  const auto SetType = [](std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                          uint32_t Type) {
    const uint32_t Word = read32le(Bytes, Offset + 4);
    write32le(Bytes, Offset + 4, (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28));
  };
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message,
                         DiagnosticKind Kind = DiagnosticKind::Malformed) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
    EXPECT_EQ(Result.error().Kind, Kind);
  };

  auto MissingSuccessor = Original.Bytes;
  write32le(MissingSuccessor, Original.RelocationCount, 1);
  Reject(std::move(MissingSuccessor), "addend relocation lacks successor");

  auto WrongSuccessor = Original.Bytes;
  SetType(WrongSuccessor, Original.Relocations[1],
          llvm::MachO::ARM64_RELOC_UNSIGNED);
  Reject(std::move(WrongSuccessor), "addend relocation successor",
         DiagnosticKind::Unsupported);

  auto MalformedAddendMetadata = Original.Bytes;
  write32le(MalformedAddendMetadata, Original.Relocations[0] + 4,
            read32le(MalformedAddendMetadata, Original.Relocations[0] + 4) |
                UINT32_C(1) << 24);
  Reject(std::move(MalformedAddendMetadata),
         "malformed addend relocation metadata");

  auto MalformedSuccessorMetadata = Original.Bytes;
  write32le(MalformedSuccessorMetadata, Original.Relocations[1] + 4,
            read32le(MalformedSuccessorMetadata, Original.Relocations[1] + 4) &
                ~(UINT32_C(1) << 27));
  Reject(std::move(MalformedSuccessorMetadata),
         "malformed addend relocation successor");

  auto MismatchedAddress = Original.Bytes;
  write32le(MismatchedAddress, Original.Relocations[0],
            read32le(MismatchedAddress, Original.Relocations[0]) + 4);
  Reject(std::move(MismatchedAddress), "addend relocation addresses differ");

  auto EmbeddedAddend = Original.Bytes;
  write32le(EmbeddedAddend, Original.Content,
            read32le(EmbeddedAddend, Original.Content) | 1);
  Reject(std::move(EmbeddedAddend),
         "addend relocation conflicts with embedded addend");
}

TEST(RelocationTest, RecognizesObservedX86_64COFFRelocationForms) {
  const std::array<std::tuple<uint32_t, uint8_t, bool>, 2> Cases{{
      {llvm::COFF::IMAGE_REL_AMD64_REL32, 4, true},
      {llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 4, false},
  }};
  expectPortableRelocationForms(ObjectFormat::COFF, Target::X86_64, Cases);
}

TEST(RelocationTest, RecognizesObservedAArch64COFFRelocationForms) {
  const std::array<std::tuple<uint32_t, uint8_t, bool>, 5> Cases{{
      {llvm::COFF::IMAGE_REL_ARM64_BRANCH26, 4, true},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21, 4, true},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A, 4, false},
      {llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L, 4, false},
      {llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, 4, false},
  }};
  expectPortableRelocationForms(ObjectFormat::COFF, Target::AArch64, Cases);
}

TEST(RelocationTest, DecodesCOFFAddr32NBImplicitAddendAsUnsigned) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  constexpr uint32_t Addend = UINT32_C(0x80000000);
  std::vector<WasmEdge::Byte> Bytes(24);
  std::memcpy(Bytes.data() + 1, &Addend, sizeof(Addend));
  auto Graph = makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0,
                                   true, ImageBase + 0x2000, ImageBase + 0x1000,
                                   ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, static_cast<uint64_t>(Addend) + 0x2000);
}

TEST(RelocationTest, WrapsCOFFAddr32NBAt32Bits) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  constexpr uint32_t Addend = UINT32_C(0xFFFFF000);
  std::vector<WasmEdge::Byte> Bytes(24);
  std::memcpy(Bytes.data() + 1, &Addend, sizeof(Addend));
  auto Graph = makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0,
                                   true, ImageBase + 0x2000, ImageBase + 0x1000,
                                   ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1000U);
}

TEST(RelocationTest, AppliesCOFFAddr32NBToUniversalOffsets) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  constexpr uint32_t Addend = UINT32_C(0xFFFFF000);
  std::vector<WasmEdge::Byte> Bytes(24);
  std::memcpy(Bytes.data() + 1, &Addend, sizeof(Addend));
  auto Graph =
      makeRelocationGraph(llvm::COFF::IMAGE_REL_AMD64_ADDR32NB, 0, true, 0x2000,
                          0x1000, ObjectFormat::COFF, std::move(Bytes));
  ASSERT_TRUE(applyRelocations(Graph));
  auto Value = Internal::readUnsigned(Graph.sections()[0].Content, 1, 4,
                                      Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, 0x1000U);
}

TEST(RelocationTest, AppliesAArch64COFFAddr32NBFromPEBasedUniversalLayout) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  constexpr uint64_t ImageBase = UINT64_C(0x180000000);
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::COFF);
  ASSERT_TRUE(Graph.beginInput("arm64-universal.obj"));
  auto Text = Graph.addSection(Section{
      ".text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}});
  auto PData = Graph.addSection(Section{".pdata", SectionKind::Unwind, 4, 8, 0,
                                        0, std::vector<WasmEdge::Byte>(8),
                                        SectionPurpose::PData});
  ASSERT_TRUE(Text && PData);
  auto Function = Graph.addSymbol(Symbol{"function", *Text, 0, 4, false});
  ASSERT_TRUE(Function);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*PData, 0, llvm::COFF::IMAGE_REL_ARM64_ADDR32NB, *Function, 0,
                 true, ObjectFormat::COFF, 4}));
  write32le(const_cast<Section &>(Graph.sections()[*PData]).Content, 4,
            (1U << 2) | 1U);

  ASSERT_TRUE(layout(Graph, ImageBase, 4096));
  ASSERT_TRUE(applyRelocations(Graph));
  EXPECT_EQ(read32le(Graph.sections()[*PData].Content, 0),
            Graph.sections()[*Text].Address - ImageBase);
  auto RuntimeFunctions = normalizePERuntimeFunctions(Graph, ImageBase);
  ASSERT_TRUE(RuntimeFunctions);
  ASSERT_EQ(RuntimeFunctions->size(), 8U);
  EXPECT_EQ(read32le(*RuntimeFunctions, 0),
            Graph.sections()[*Text].Address - ImageBase);
}

TEST(RelocationTest, AcceptsX86_64MachOAbsoluteAndRejectsGOTForm) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                llvm::MachO::X86_64_RELOC_UNSIGNED, 8),
            8);
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                   llvm::MachO::X86_64_RELOC_UNSIGNED, 4));
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::X86_64,
                                   llvm::MachO::X86_64_RELOC_GOT, 4));
}

TEST(RelocationTest, AcceptsAArch64MachOAbsoluteAndRejectsGOTForm) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                llvm::MachO::ARM64_RELOC_UNSIGNED, 8),
            8);
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                   llvm::MachO::ARM64_RELOC_UNSIGNED, 4));
  EXPECT_FALSE(relocationPatchSize(ObjectFormat::MachO, Target::AArch64,
                                   llvm::MachO::ARM64_RELOC_POINTER_TO_GOT, 8));
}

TEST(RelocationTest, RejectsFourByteAArch64MachOUnsignedWithoutOverwrite) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unsigned.o"));
  std::vector<WasmEdge::Byte> Bytes{0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
  auto Patch = Graph.addSection(
      Section{"__data", SectionKind::Data, 4, 8, 0x1000, 0, Bytes});
  auto TargetSection =
      Graph.addSection(Section{"__target", SectionKind::Data, 8, 8, 0x2000, 0,
                               std::vector<WasmEdge::Byte>(8)});
  ASSERT_TRUE(Patch && TargetSection);
  auto Target = Graph.addSymbol(Symbol{"target", *TargetSection, 0, 8, false});
  ASSERT_TRUE(Target);
  EXPECT_FALSE(Graph.addRelocation(
      Relocation{*Patch, 0, llvm::MachO::ARM64_RELOC_UNSIGNED, *Target, 0,
                 false, ObjectFormat::MachO, 4}));
  EXPECT_EQ(Graph.sections()[*Patch].Content, Bytes);
}

TEST(ObjectReaderTest, RejectsFourByteAArch64MachOUnsigned) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Object = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.data
.globl _pointer
_pointer:
.long _target
.long 0x44332211
.text
.globl _target
_target:
ret
)");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unsigned32.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool Found = false;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Contents = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents));
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&**Parsed);
      const auto Raw = MachO->getRelocation(Relocation.getRawDataRefImpl());
      EXPECT_EQ(MachO->getAnyRelocationLength(Raw), 2U);
      ASSERT_GE(Contents->size(), Relocation.getOffset() + 8);
      EXPECT_EQ(static_cast<uint8_t>((*Contents)[Relocation.getOffset() + 4]),
                0x11U);
      Found = true;
    }
  }
  ASSERT_TRUE(Found);
  EXPECT_FALSE(ObjectReader::read(Object, Target::AArch64));
}

TEST(RelocationTest, AcceptsX86_64COFFAbsolute) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::COFF, Target::X86_64,
                                llvm::COFF::IMAGE_REL_AMD64_ADDR64, 8),
            8);
}

TEST(RelocationTest, AcceptsAArch64COFFAbsolute) {
  EXPECT_EQ(relocationPatchSize(ObjectFormat::COFF, Target::AArch64,
                                llvm::COFF::IMAGE_REL_ARM64_ADDR64, 8),
            8);
}

class PortableObjectTest
    : public testing::Test,
      public testing::WithParamInterface<PortableObjectCase> {};

TEST_P(PortableObjectTest, ReadsRealisticPortableObjectsWithoutPersonality) {
  const auto &Test = GetParam();
  REQUIRE_RELOCATION_HANDLER(Test.Architecture);
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Bytes = makeObject(llvm::Triple(Test.Triple));
  auto Graph = ObjectReader::read(Bytes, Test.Architecture);
  ASSERT_TRUE(Graph) << Test.Triple;
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    Types.insert(Relocation.Type);
  }
  EXPECT_EQ(Types, Test.Types) << Test.Triple;
  EXPECT_TRUE(std::none_of(Graph->sections().begin(), Graph->sections().end(),
                           [](const auto &Section) {
                             return Section.Purpose ==
                                    SectionPurpose::CompactUnwind;
                           }));
  ASSERT_TRUE(layout(*Graph, 0x4000, 0x4000));
  EXPECT_TRUE(applyRelocations(*Graph)) << Test.Triple;
}

INSTANTIATE_TEST_SUITE_P(
    MachO, PortableObjectTest,
    testing::Values(PortableObjectCase{"x86_64-apple-macosx",
                                       Target::X86_64,
                                       {llvm::MachO::X86_64_RELOC_SIGNED}},
                    PortableObjectCase{"arm64-apple-macosx",
                                       Target::AArch64,
                                       {llvm::MachO::ARM64_RELOC_PAGE21,
                                        llvm::MachO::ARM64_RELOC_PAGEOFF12}}),
    portableObjectCaseName);
INSTANTIATE_TEST_SUITE_P(
    COFF, PortableObjectTest,
    testing::Values(PortableObjectCase{"x86_64-pc-windows-msvc",
                                       Target::X86_64,
                                       {llvm::COFF::IMAGE_REL_AMD64_REL32}},
                    PortableObjectCase{
                        "aarch64-pc-windows-msvc",
                        Target::AArch64,
                        {llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                         llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A}}),
    portableObjectCaseName);

class GeneratedMachOEHFrameTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

TEST_P(GeneratedMachOEHFrameTest, NormalizesGeneratedMachOFrames) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  const auto Object = makeObject(llvm::Triple(Triple), false, false, "f0", {},
                                 false, false, "generic", {}, true, false,
                                 false, false, false, false, {}, false, true);
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph)) << Triple;
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph)) << Triple;
  constexpr uint64_t FirstBase = UINT64_C(0x100000000);
  constexpr uint64_t SecondBase = UINT64_C(0x700000000);
  const auto First = machOEHFrameStarts(*Graph, FirstBase);
  const auto Second = machOEHFrameStarts(*Graph, SecondBase);
  ASSERT_TRUE(First) << Triple;
  ASSERT_TRUE(Second) << Triple;
  ASSERT_EQ(First->size(), Second->size());
  EXPECT_EQ(First->size(), 2U) << Triple;
  for (size_t I = 0; I < First->size(); ++I)
    EXPECT_EQ((*First)[I] - FirstBase, (*Second)[I] - SecondBase) << Triple;
  for (const std::string_view Name : {"_f0", "_t0"}) {
    const auto Symbol =
        std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                     [&](const auto &Value) { return Value.Name == Name; });
    ASSERT_NE(Symbol, Graph->symbols().end()) << Triple << " " << Name;
    const uint64_t Address =
        FirstBase + Graph->sections()[Symbol->Section].Address + Symbol->Offset;
    EXPECT_NE(std::find(First->begin(), First->end(), Address), First->end())
        << Triple << " " << Name;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOEHFrameTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx", Target::AArch64}),
    objectArchitectureCaseName);

TEST(MachOEHFrameTest, RelocationOwnsFDEAddressAndAppliesAddend) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  auto &RelocationValue =
      const_cast<Relocation &>(Graph->relocations().front());
  ASSERT_EQ(RelocationValue.Offset, Fixture.Field);
  RelocationValue.Addend = 1;
  RelocationValue.AddendIsImplicit = false;
  auto &EH = const_cast<Section &>(Graph->sections()[RelocationValue.Section]);
  ASSERT_TRUE(Internal::writeUnsigned(EH.Content, Fixture.Field, 8,
                                      Endianness::Little,
                                      UINT64_C(0x7FFFFFFFFFFFFFFF)));

  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  const auto &Symbol = Graph->symbols()[RelocationValue.Symbol];
  EXPECT_EQ(Starts->front(),
            Graph->sections()[Symbol.Section].Address + Symbol.Offset + 1);
}

TEST(MachOEHFrameTest, RejectsNonPointerWidthFDERelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  const_cast<Relocation &>(Graph->relocations().front()).PatchSize = 4;
  EXPECT_FALSE(normalizeMachOEHFrame(*Graph));
}

class GeneratedMachOCompactUnwindTest
    : public testing::Test,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

TEST_P(GeneratedMachOCompactUnwindTest,
       InventoriesObservedLLVMGeneratedMachORecords) {
  struct Fixture {
    const char *Name;
    bool UnwindTable;
    bool Optimize;
    bool Representative;
    bool Exceptions;
    bool TypeWrapper;
  };
  constexpr std::array<Fixture, 6> Fixtures{{
      {"leaf", false, false, false, false, false},
      {"default-unwind", true, false, false, false, false},
      {"optimized-unwind", true, true, false, false, false},
      {"representative-mixed", true, true, true, false, false},
      {"type-wrapper-compact-only", true, false, false, false, true},
      {"exceptions-mixed-zero-compact-personality-lsda", true, false, false,
       true, false},
  }};
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  REQUIRE_LLVM_TARGET(Triple);
  for (const auto &Fixture : Fixtures) {
    const auto Inventory = collectCompactUnwindInventory(
        makeObject(llvm::Triple(Triple), false, false, "f0", {}, false, false,
                   "generic", {}, Fixture.UnwindTable, Fixture.Optimize, false,
                   false, Fixture.Representative, Fixture.Exceptions, {}, false,
                   Fixture.TypeWrapper));
    if (std::string_view(Triple) == "x86_64-apple-macosx") {
      EXPECT_FALSE(Inventory.HasCompactUnwind) << Fixture.Name;
      EXPECT_TRUE(Inventory.Records.empty()) << Fixture.Name;
      EXPECT_TRUE(Inventory.Relocations.empty()) << Fixture.Name;
      EXPECT_EQ(Inventory.HasEHFrame, Fixture.UnwindTable) << Fixture.Name;
      continue;
    }
    if (!Fixture.UnwindTable) {
      EXPECT_FALSE(Inventory.HasCompactUnwind) << Fixture.Name;
      EXPECT_TRUE(Inventory.Records.empty()) << Fixture.Name;
      EXPECT_TRUE(Inventory.Relocations.empty()) << Fixture.Name;
      EXPECT_FALSE(Inventory.HasEHFrame) << Fixture.Name;
      continue;
    }
    const size_t ExpectedCount = Fixture.TypeWrapper  ? 2U
                                 : Fixture.Exceptions ? 3U
                                                      : 1U;
    ASSERT_TRUE(Inventory.HasCompactUnwind) << Fixture.Name;
    ASSERT_EQ(Inventory.Records.size(), ExpectedCount) << Fixture.Name;
    ASSERT_EQ(Inventory.Relocations.size(), ExpectedCount) << Fixture.Name;
    const bool MixedCompactAndDwarf =
        Fixture.Representative || Fixture.Exceptions;
    EXPECT_EQ(Inventory.HasEHFrame, MixedCompactAndDwarf) << Fixture.Name;
    uint64_t Function = 0;
    std::set<uint64_t> RelocationOffsets;
    std::set<std::string> RelocationSymbols;
    for (size_t I = 0; I < Inventory.Records.size(); ++I) {
      const auto &Record = Inventory.Records[I];
      EXPECT_EQ(Record.Function, Function) << Fixture.Name << " " << I;
      EXPECT_GT(Record.Length, 0U) << Fixture.Name << " " << I;
      EXPECT_EQ(Record.Encoding,
                I == 0 && (Fixture.Representative || Fixture.Exceptions)
                    ? UINT32_C(0x03000000)
                    : UINT32_C(0x02000000))
          << Fixture.Name << " " << I;
      EXPECT_EQ(Record.Personality, 0U) << Fixture.Name << " " << I;
      EXPECT_EQ(Record.LSDA, 0U) << Fixture.Name << " " << I;
      Function += Record.Length;
    }
    for (const auto &Relocation : Inventory.Relocations) {
      EXPECT_EQ(Relocation.Type,
                static_cast<uint64_t>(llvm::MachO::ARM64_RELOC_UNSIGNED))
          << Fixture.Name;
      EXPECT_EQ(Relocation.Offset % sizeof(RawCompactUnwindRecord), 0U)
          << Fixture.Name;
      RelocationOffsets.emplace(Relocation.Offset);
      RelocationSymbols.emplace(Relocation.Symbol);
    }
    std::set<uint64_t> ExpectedOffsets;
    for (size_t I = 0; I < ExpectedCount; ++I)
      ExpectedOffsets.emplace(I * sizeof(RawCompactUnwindRecord));
    EXPECT_EQ(RelocationOffsets, ExpectedOffsets) << Fixture.Name;
    EXPECT_EQ(RelocationSymbols, (std::set<std::string>{""})) << Fixture.Name;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOCompactUnwindTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx", Target::AArch64}),
    objectArchitectureCaseName);

LinkGraph makeCompactUnwindGraph(
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

TEST(CompactUnwindTest, ConvertsAArch64FramelessAndSemanticCoverage) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02002000);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  EXPECT_EQ(Starts->front(), Graph.sections().front().Address);
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x0C, 31, 32, 0x08, 30};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
  ASSERT_GE(EH->Content.size(), 4U);
  EXPECT_EQ(
      std::vector<WasmEdge::Byte>(EH->Content.end() - 4, EH->Content.end()),
      std::vector<WasmEdge::Byte>(4));
}

TEST(CompactUnwindTest, ConvertsX86RBPFrame) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x01010001);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 1U);
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x0C, 6, 16, 0x90, 1, 0x86, 2, 0x83, 3};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
}

TEST(CompactUnwindTest, ConvertsMultipleOrderedRecords) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x02000000, "_t0");
  auto Second = Graph.addSymbol(Symbol{"_f0", 0, 16, 16, true, {}, true});
  ASSERT_TRUE(Second);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 16, 0x02001000, {}, {}, {}}));
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 2U);
  EXPECT_TRUE(std::is_sorted(Starts->begin(), Starts->end()));
}

void expectUnsupportedCompactUnwindRejectedAtomically(Target Architecture,
                                                      uint32_t Encoding) {
  auto Graph = makeCompactUnwindGraph(Architecture, Encoding);
  const auto Sections = Graph.sections();
  const auto References = Graph.ehFrameReferences();
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), Sections.size());
  EXPECT_EQ(Graph.ehFrameReferences().size(), References.size());
  EXPECT_EQ(Graph.sections().front().Content, Sections.front().Content);
}

TEST(CompactUnwindTest, RejectsUnsupportedX86_64RecordAtomically) {
  expectUnsupportedCompactUnwindRejectedAtomically(Target::X86_64, 0x01010009);
}

TEST(CompactUnwindTest, RejectsUnsupportedAArch64RecordsAtomically) {
  expectUnsupportedCompactUnwindRejectedAtomically(Target::AArch64, 0x02000020);
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x42000000);
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), 1U);

  auto MissingFDE = makeCompactUnwindGraph(Target::AArch64, 0x03000000);
  EXPECT_FALSE(compactUnwindToEHFrame(MissingFDE));
  EXPECT_EQ(MissingFDE.sections().size(), 1U);

  for (const bool HasLSDA : {false, true}) {
    LinkGraph Optional(Target::AArch64, Endianness::Little,
                       ObjectFormat::MachO);
    ASSERT_TRUE(Optional.beginInput("optional.o"));
    auto Text = Optional.addSection(
        Section{"__text", SectionKind::Text, 4, 16, 0, 0,
                std::vector<WasmEdge::Byte>(16), SectionPurpose::Default});
    auto Data = Optional.addSection(
        Section{"__data", SectionKind::Data, 8, 8, 0, 0,
                std::vector<WasmEdge::Byte>(8), SectionPurpose::Default});
    ASSERT_TRUE(Text && Data);
    auto Function =
        Optional.addSymbol(Symbol{"_f0", *Text, 0, 16, true, {}, true});
    auto Value = Optional.addSymbol(Symbol{"optional", *Data, 0, 8, false});
    ASSERT_TRUE(Function && Value);
    ASSERT_TRUE(Optional.addCompactUnwind(CompactUnwindRecord{
        *Function,
        16,
        0x02000000,
        HasLSDA ? std::optional<SymbolId>{} : std::optional<SymbolId>{*Value},
        HasLSDA ? std::optional<SymbolId>{*Value} : std::optional<SymbolId>{},
        {}}));
    EXPECT_FALSE(compactUnwindToEHFrame(Optional));
    EXPECT_EQ(Optional.sections().size(), 2U);
  }
}

TEST(CompactUnwindTest, PreservesExistingDwarfFallbackAndSynthesizesMissing) {
  auto Existing = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Existing));
  const auto ExistingEH =
      std::find_if(Existing.sections().begin(), Existing.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(ExistingEH, Existing.sections().end());
  ASSERT_EQ(Existing.ehFrameReferences().size(), 1U);

  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("mixed.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 4, 64, 0, 0,
                                       std::vector<WasmEdge::Byte>(64),
                                       SectionPurpose::Default, 0x1000});
  auto EH = Graph.addSection(*ExistingEH);
  ASSERT_TRUE(Text && EH);
  auto First = Graph.addSymbol(Symbol{"_t0", *Text, 0, 16, true, {}, true});
  auto Second = Graph.addSymbol(Symbol{"_f0", *Text, 16, 16, true, {}, true});
  ASSERT_TRUE(First && Second);
  ASSERT_TRUE(Graph.addEHFrameReference(EHFrameReference{
      *EH, Existing.ehFrameReferences().front().Offset, *First}));
  auto FDE = Graph.addSymbol(
      Symbol{"fallback_fde", *EH,
             Existing.ehFrameReferences().front().Offset - 8, 0, false});
  ASSERT_TRUE(FDE);
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*First, 16, 0x03000000, {}, {}, *FDE}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Second, 16, 0x02000000, {}, {}, {}}));
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(std::count_if(Graph.sections().begin(), Graph.sections().end(),
                          [](const auto &Section) {
                            return Section.Purpose == SectionPurpose::EHFrame;
                          }),
            1);
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
  const auto Starts = machOEHFrameStarts(Graph, 0);
  ASSERT_TRUE(Starts);
  EXPECT_EQ(Starts->size(), 2U);
}

TEST_F(LinkerOutputTest, WritesConvertedCompactUnwindToUniversalWasm) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(__aarch64__) &&       \
    !defined(_M_ARM64)
  GTEST_SKIP() << "compact unwind conversion requires x86_64 or AArch64";
#else
#if defined(__x86_64__) || defined(_M_X64)
  constexpr const char *Triple = "x86_64-apple-macosx";
  constexpr std::string_view Instructions = "retq\n";
  constexpr uint32_t Encoding = UINT32_C(0x01000000);
  constexpr WasmEdge::Byte AOTArchitecture = 1;
  const auto Architecture = Target::X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
  constexpr const char *Triple = "arm64-apple-macosx";
  constexpr std::string_view Instructions = "ret\n";
  constexpr uint32_t Encoding = UINT32_C(0x02000000);
  constexpr WasmEdge::Byte AOTArchitecture = 2;
  const auto Architecture = Target::AArch64;
#endif
  REQUIRE_LLVM_TARGET(Triple);
  constexpr std::array<WasmEdge::Byte, 34> TinyWasm{
      0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7F, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01, 0x66,
      0x00, 0x00, 0x0A, 0x06, 0x01, 0x04, 0x00, 0x41, 0x07, 0x0B};
  const auto Object = makeAssemblyObject(
      llvm::Triple(Triple),
      ".text\n.globl _f0\n_f0:\n" + std::string(Instructions) +
          ".globl _t0\n_t0:\n" + std::string(Instructions) +
          ".section __DATA,__const\n.globl _version\n_version:\n.long 1\n"
          ".globl _intrinsics\n_intrinsics:\n.long 2\n"
          ".section __LD,__compact_unwind,regular,debug\n.p2align 3\n"
          ".quad _f0\n.long 1\n.long " +
          std::to_string(Encoding) +
          "\n.quad 0\n.quad 0\n.quad _t0\n.long 1\n.long " +
          std::to_string(Encoding) + "\n.quad 0\n.quad 0\n");
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 2U);
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 2U);
  for (const std::string_view Name : {"_f0", "_t0"}) {
    const auto Symbol =
        std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                     [&](const auto &Value) { return Value.Name == Name; });
    ASSERT_NE(Symbol, Graph->symbols().end()) << Name;
    const uint64_t Address =
        Graph->sections()[Symbol->Section].Address + Symbol->Offset;
    EXPECT_NE(std::find(Starts->begin(), Starts->end(), Address), Starts->end())
        << Name;
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  ASSERT_TRUE(Graph->rebases().empty());

  const auto Output = Directory / "compact-unwind.wasm";
  ASSERT_TRUE(UniversalWasmWriter::write(*Graph, TinyWasm, Output));
  const auto Bytes = readFile(Output);
  ASSERT_GT(Bytes.size(), TinyWasm.size());
  EXPECT_TRUE(std::equal(TinyWasm.begin(), TinyWasm.end(), Bytes.begin()));
  size_t Cursor = TinyWasm.size();
  ASSERT_EQ(Bytes[Cursor++], 0U);
  const auto ReadULEB = [&](size_t &Offset) {
    uint64_t Value = 0;
    unsigned Shift = 0;
    while (Offset < Bytes.size()) {
      const WasmEdge::Byte Part = Bytes[Offset++];
      Value |= static_cast<uint64_t>(Part & 0x7F) << Shift;
      if ((Part & 0x80) == 0)
        return Value;
      Shift += 7;
    }
    return UINT64_MAX;
  };
  const uint64_t PayloadSize = ReadULEB(Cursor);
  ASSERT_EQ(Cursor + PayloadSize, Bytes.size());
  const uint64_t NameSize = ReadULEB(Cursor);
  ASSERT_EQ(
      std::string_view(reinterpret_cast<const char *>(Bytes.data() + Cursor),
                       NameSize),
      "wasmedge");
  Cursor += NameSize;
  EXPECT_EQ(ReadULEB(Cursor), WasmEdge::AOT::kBinaryVersion);
  static_cast<void>(Bytes[Cursor++]);
  EXPECT_EQ(Bytes[Cursor++], AOTArchitecture);
  for (unsigned I = 0; I < 2; ++I)
    static_cast<void>(ReadULEB(Cursor));
  for (unsigned List = 0; List < 2; ++List) {
    const uint64_t Count = ReadULEB(Cursor);
    for (uint64_t I = 0; I < Count; ++I)
      static_cast<void>(ReadULEB(Cursor));
  }
  const uint64_t SectionCount = ReadULEB(Cursor);
  std::vector<WasmEdge::Byte> Unwind;
  for (uint64_t I = 0; I < SectionCount; ++I) {
    const WasmEdge::Byte Kind = Bytes[Cursor++];
    static_cast<void>(ReadULEB(Cursor));
    static_cast<void>(ReadULEB(Cursor));
    const uint64_t ContentSize = ReadULEB(Cursor);
    ASSERT_LE(ContentSize, Bytes.size() - Cursor);
    const auto NativeContentSize = static_cast<size_t>(ContentSize);
    if (Kind == 4)
      Unwind.assign(Bytes.data() + Cursor,
                    Bytes.data() + Cursor + NativeContentSize);
    Cursor += NativeContentSize;
  }
  ASSERT_FALSE(Unwind.empty());
  EXPECT_EQ(std::vector<WasmEdge::Byte>(Unwind.end() - 4, Unwind.end()),
            std::vector<WasmEdge::Byte>(4));
#endif
}

TEST(GeneratedMachOLinkerTest, LinksMixedRawDwarfAndCompactX86_64Unwind) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true, false, true);
  const auto Input = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Input.Content, 0U);
  ASSERT_TRUE(Input.Relocations.empty());
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->ehFrameReferences()[0].Symbol].Name, "_f0");
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->compactUnwind()[0].Function].Name, "_t0");

  ASSERT_TRUE(Graph->pruneUnreferencedMachOEHFrame());
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->ehFrameReferences()[0].Symbol].Name, "_f0");
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  EXPECT_EQ(Graph->symbols()[Graph->compactUnwind()[0].Function].Name, "_t0");
  EXPECT_TRUE(std::any_of(Graph->sections().begin(), Graph->sections().end(),
                          [](const auto &Section) {
                            return Section.Purpose == SectionPurpose::EHFrame;
                          }));
  ASSERT_TRUE(reserveMachOUnwindInfo(*Graph));
  ASSERT_TRUE(MachOWriter::layout(*Graph));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));
  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  ASSERT_EQ(Starts->size(), 1U);
  const auto F0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f0"; });
  const auto T0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  ASSERT_NE(F0, Graph->symbols().end());
  ASSERT_NE(T0, Graph->symbols().end());
  const uint64_t F0Address =
      Graph->sections()[F0->Section].Address + F0->Offset;
  const uint64_t T0Address =
      Graph->sections()[T0->Section].Address + T0->Offset;
  EXPECT_NE(std::find(Starts->begin(), Starts->end(), F0Address),
            Starts->end());
  EXPECT_EQ(
      Graph->sections()[Graph->symbols()[Graph->compactUnwind()[0].Function]
                            .Section]
              .Address +
          Graph->symbols()[Graph->compactUnwind()[0].Function].Offset,
      T0Address);
  ASSERT_TRUE(applyRelocations(*Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(*Graph));
  const auto Unwind = std::find_if(
      Graph->sections().begin(), Graph->sections().end(),
      [](const auto &S) { return S.Purpose == SectionPurpose::UnwindInfo; });
  ASSERT_NE(Unwind, Graph->sections().end());
  const auto CompactStarts = machOUnwindFunctionStarts(Unwind->Content);
  ASSERT_TRUE(CompactStarts);
  EXPECT_NE(std::find(CompactStarts->begin(), CompactStarts->end(), T0Address),
            CompactStarts->end());

  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(*Graph, Output));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "mixed-x86_64.dylib"));
  ASSERT_TRUE(static_cast<bool>(Object));
  std::set<std::string> Sections;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    Sections.emplace(Name->str());
  }
  EXPECT_TRUE(Sections.count("__eh_frame"));
  EXPECT_TRUE(Sections.count("__unwind_info"));
  EXPECT_FALSE(Sections.count("__compact_unwind"));
}

TEST_P(GeneratedMachOLinkerTest, LinksGeneratedMacOSObjects) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object =
      makeObject(llvm::Triple(Triple), false, false, "f0", {}, false, false,
                 "generic", {}, true, false, false, false, false, false, {},
                 true, true, false, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "generated.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  std::set<std::string> InputSections;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    InputSections.emplace(Name->str());
  }
  EXPECT_EQ(InputSections.count("__compact_unwind") != 0,
            Architecture == Target::AArch64)
      << Triple;
  EXPECT_EQ(InputSections.count("__eh_frame") != 0,
            Architecture == Target::X86_64)
      << Triple;
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const bool HasCompact = !Graph->compactUnwind().empty();
  const bool NeedsDwarf =
      std::any_of(Graph->compactUnwind().begin(), Graph->compactUnwind().end(),
                  [](const auto &Record) { return Record.FDE.has_value(); });
  const bool HasInputEHFrame =
      std::any_of(Graph->sections().begin(), Graph->sections().end(),
                  [](const auto &Section) {
                    return Section.Purpose == SectionPurpose::EHFrame;
                  });
  if (HasCompact) {
    ASSERT_TRUE(Graph->pruneUnreferencedMachOEHFrame()) << Triple;
  }
  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(reserveMachOUnwindInfo(*Graph)) << Triple;
  }
  ASSERT_TRUE(MachOWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph)) << Triple;
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  if (!Graph->compactUnwind().empty()) {
    ASSERT_TRUE(populateMachOUnwindInfo(*Graph)) << Triple;
    const auto Unwind =
        std::find_if(Graph->sections().begin(), Graph->sections().end(),
                     [](const auto &Section) {
                       return Section.Purpose == SectionPurpose::UnwindInfo;
                     });
    ASSERT_NE(Unwind, Graph->sections().end()) << Triple;
  }
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(MachOWriter::write(*Graph, Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "generated.dylib"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  bool HasEHFrame = false;
  std::vector<WasmEdge::Byte> FinalUnwind;
  for (const auto &Section : (*Image)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    HasEHFrame |= *Name == "__eh_frame";
    if (*Name == "__unwind_info") {
      auto Content = Section.getContents();
      ASSERT_TRUE(static_cast<bool>(Content));
      FinalUnwind.assign(Content->bytes_begin(), Content->bytes_end());
    }
  }
  EXPECT_EQ(HasEHFrame, HasInputEHFrame && (!HasCompact || NeedsDwarf))
      << Triple;
  EXPECT_EQ(!FinalUnwind.empty(), HasCompact) << Triple;
  if (HasCompact) {
    const auto FunctionOffsets = machOUnwindFunctionStarts(FinalUnwind);
    ASSERT_TRUE(FunctionOffsets) << Triple;
    for (const std::string_view Name : {"_f0", "_t0"}) {
      const auto Symbol =
          std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                       [&](const auto &Value) { return Value.Name == Name; });
      ASSERT_NE(Symbol, Graph->symbols().end()) << Triple << " " << Name;
      const auto Address = static_cast<uint32_t>(
          Graph->sections()[Symbol->Section].Address + Symbol->Offset);
      EXPECT_NE(
          std::find(FunctionOffsets->begin(), FunctionOffsets->end(), Address),
          FunctionOffsets->end())
          << Triple << " " << Name;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedMachOLinkerTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"arm64-apple-macosx", Target::AArch64}),
    objectArchitectureCaseName);

TEST_P(GeneratedWindowsLinkerTest, LinksGeneratedWindowsObjects) {
  const auto &Test = GetParam();
  const auto Triple = Test.Triple;
  const auto Architecture = Test.Architecture;
  REQUIRE_RELOCATION_HANDLER(Architecture);
  REQUIRE_LLVM_TARGET(Triple);
  const auto Object = makeObject(llvm::Triple(Triple), false, false, "f0",
                                 "/EXPORT:f0 /EXPORT:value", false, false,
                                 "generic", {}, true, true, false, false, true);
  auto Graph = ObjectReader::read(Object, Architecture);
  ASSERT_TRUE(Graph) << Triple;
  const std::set<uint32_t> Allowed =
      Architecture == Target::X86_64
          ? std::set<uint32_t>{llvm::COFF::IMAGE_REL_AMD64_ADDR64,
                               llvm::COFF::IMAGE_REL_AMD64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_AMD64_REL32,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_1,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_2,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_3,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_4,
                               llvm::COFF::IMAGE_REL_AMD64_REL32_5}
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L};
  const std::set<uint32_t> Core =
      Architecture == Target::X86_64
          ? std::set<uint32_t>{llvm::COFF::IMAGE_REL_AMD64_ADDR64,
                               llvm::COFF::IMAGE_REL_AMD64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_AMD64_REL32}
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21};
  const std::set<uint32_t> Expected =
      Architecture == Target::X86_64
          ? Core
          : std::set<uint32_t>{llvm::COFF::IMAGE_REL_ARM64_ADDR64,
                               llvm::COFF::IMAGE_REL_ARM64_ADDR32NB,
                               llvm::COFF::IMAGE_REL_ARM64_BRANCH26,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                               llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L};
  std::set<uint32_t> Actual;
  for (const auto &Relocation : Graph->relocations()) {
    Actual.insert(Relocation.Type);
    EXPECT_EQ(Relocation.PatchSize,
              Relocation.Type == llvm::COFF::IMAGE_REL_AMD64_ADDR64 ||
                      Relocation.Type == llvm::COFF::IMAGE_REL_ARM64_ADDR64
                  ? 8
                  : 4);
  }
  EXPECT_TRUE(std::includes(Allowed.begin(), Allowed.end(), Actual.begin(),
                            Actual.end()))
      << Triple;
  EXPECT_TRUE(
      std::includes(Actual.begin(), Actual.end(), Core.begin(), Core.end()))
      << Triple;
  EXPECT_EQ(Actual, Expected) << Triple;
  auto RawObject =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "generated.obj"));
  ASSERT_TRUE(static_cast<bool>(RawObject));
  for (const auto &Symbol : (*RawObject)->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    EXPECT_NE(*Name, "_fltused");
    EXPECT_NE(*Name, "_DllMainCRTStartup");
  }
  ASSERT_TRUE(PEWriter::layout(*Graph)) << Triple;
  ASSERT_TRUE(applyRelocations(*Graph)) << Triple;
  ASSERT_FALSE(Graph->rebases().empty()) << Triple;
  for (const auto &Rebase : Graph->rebases()) {
    EXPECT_EQ(Rebase.Width, 8U);
    EXPECT_EQ(Rebase.Type,
              Architecture == Target::X86_64
                  ? static_cast<uint32_t>(llvm::COFF::IMAGE_REL_AMD64_ADDR64)
                  : static_cast<uint32_t>(llvm::COFF::IMAGE_REL_ARM64_ADDR64));
  }
  std::vector<WasmEdge::Byte> Bytes;
  Writer Output(Bytes);
  ASSERT_TRUE(PEWriter::write(*Graph, "generated.dll", Output)) << Triple;
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                      Bytes.size()),
      "generated.dll"));
  ASSERT_TRUE(static_cast<bool>(Image))
      << Triple << " " << llvm::toString(Image.takeError());
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  auto Read = [&](size_t Offset, uint8_t Width) {
    uint64_t Result = 0;
    for (uint8_t I = 0; I < Width; ++I)
      Result |= static_cast<uint64_t>(Bytes[Offset + I]) << (I * 8);
    return Result;
  };
  const uint32_t HeaderOffset = static_cast<uint32_t>(Read(0x3C, 4));
  const size_t Optional = HeaderOffset + 24;
  EXPECT_EQ(Read(Optional + 16, 4), 0U);
  for (const size_t Directory : {size_t{1}, size_t{12}})
    EXPECT_EQ(Read(Optional + 112 + Directory * 8, 8), 0U);
  std::set<std::string> Exports;
  for (const auto &Export : PE->export_directories()) {
    llvm::StringRef Name;
    ASSERT_FALSE(Export.getSymbolName(Name));
    Exports.emplace(Name.str());
  }
  EXPECT_EQ(Exports, (std::set<std::string>{"f0", "value"}));
  EXPECT_FALSE(Exports.count("_fltused"));
  EXPECT_FALSE(Exports.count("_DllMainCRTStartup"));
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedWindowsLinkerTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-pc-windows-msvc", Target::AArch64}),
    objectArchitectureCaseName);

TEST(PEWriterTest, DiscardsUnreferencedCompilerRequiredFltusedMarker) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Object =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0",
                 "/EXPORT:f0", false, false, "generic", {}, true, true, false,
                 false, false, false, {}, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "floating.obj"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool HasUndefinedFltused = false;
  for (const auto &Symbol : (*Parsed)->symbols()) {
    auto Name = Symbol.getName();
    auto Flags = Symbol.getFlags();
    ASSERT_TRUE(Name && Flags);
    HasUndefinedFltused |=
        *Name == "_fltused" &&
        (*Flags & llvm::object::SymbolRef::SF_Undefined) != 0;
  }
  EXPECT_TRUE(HasUndefinedFltused);
  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
  auto Graph =
      ObjectReader::read(Object, Target::X86_64,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::none_of(
      Graph->symbols().begin(), Graph->symbols().end(), [](const auto &Symbol) {
        return Symbol.Name == "_fltused" || Symbol.Name == "_DllMainCRTStartup";
      }));
  const auto ResolvedObject =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0",
                 "/EXPORT:f0", false, false, "generic", {}, true, true, false,
                 false, false, false, {}, false, false, true, true);
  auto Resolved = ObjectReader::read(ResolvedObject, Target::X86_64);
  ASSERT_TRUE(Resolved);
  const auto TextContent = [](const LinkGraph &Value) {
    std::vector<std::vector<WasmEdge::Byte>> Result;
    for (const auto &Section : Value.sections())
      if (Section.Kind == SectionKind::Text)
        Result.push_back(Section.Content);
    return Result;
  };
  EXPECT_EQ(TextContent(*Graph), TextContent(*Resolved));
  const auto RealSymbols = [](const LinkGraph &Value) {
    std::set<std::string> Result;
    for (const auto &Symbol : Value.symbols())
      if (Symbol.Name != "_fltused")
        Result.emplace(Symbol.Name);
    return Result;
  };
  EXPECT_EQ(RealSymbols(*Graph), RealSymbols(*Resolved));
  ASSERT_EQ(Graph->relocations().size(), Resolved->relocations().size());
  for (size_t I = 0; I < Graph->relocations().size(); ++I) {
    const auto &Left = Graph->relocations()[I];
    const auto &Right = Resolved->relocations()[I];
    EXPECT_EQ(Left.Section, Right.Section);
    EXPECT_EQ(Left.Offset, Right.Offset);
    EXPECT_EQ(Left.Type, Right.Type);
    EXPECT_EQ(Left.Addend, Right.Addend);
    EXPECT_EQ(Graph->symbols()[Left.Symbol].Name,
              Resolved->symbols()[Right.Symbol].Name);
  }
  ASSERT_TRUE(PEWriter::layout(*Graph));
  ASSERT_TRUE(applyRelocations(*Graph));
  EXPECT_TRUE(std::none_of(Graph->relocations().begin(),
                           Graph->relocations().end(), [&](const auto &Rel) {
                             return Graph->symbols()[Rel.Symbol].Name ==
                                    "_fltused";
                           }));
  std::vector<WasmEdge::Byte> ImageBytes;
  Writer ImageWriter(ImageBytes);
  ASSERT_TRUE(PEWriter::write(*Graph, "floating.dll", ImageWriter));
  auto Image = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(ImageBytes.data()),
                      ImageBytes.size()),
      "floating.dll"));
  ASSERT_TRUE(static_cast<bool>(Image));
  const auto *PE = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Image);
  ASSERT_NE(PE, nullptr);
  EXPECT_EQ(PE->getPE32PlusHeader()->AddressOfEntryPoint, 0U);
  EXPECT_EQ(PE->import_directory_begin(), PE->import_directory_end());
  auto Read = [&](size_t Offset, uint8_t Width) {
    uint64_t Result = 0;
    for (uint8_t I = 0; I < Width; ++I)
      Result |= static_cast<uint64_t>(ImageBytes[Offset + I]) << (I * 8);
    return Result;
  };
  const uint32_t PEOffset = static_cast<uint32_t>(Read(0x3C, 4));
  const size_t Optional = PEOffset + 24;
  for (const size_t Directory : {size_t{1}, size_t{12}})
    EXPECT_EQ(Read(Optional + 112 + Directory * 8, 8), 0U);
  std::set<std::string> Exports;
  for (const auto &Export : PE->export_directories()) {
    llvm::StringRef Name;
    ASSERT_FALSE(Export.getSymbolName(Name));
    Exports.emplace(Name.str());
  }
  EXPECT_EQ(Exports, (std::set<std::string>{"f0"}));
}

TEST(ObjectReaderTest, SelectsNativeFltusedPolicyFromCOFFObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto COFF =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(Marker));

  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::COFF),
            ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::ELF),
            ObjectReaderInputPolicy::Strict);
  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::MachO),
            ObjectReaderInputPolicy::Strict);
  EXPECT_TRUE(ObjectReader::read(
      COFF, Target::X86_64,
      Internal::nativeObjectInputPolicy(ObjectFormat::COFF)));
}

TEST_P(MSVCFltusedPolicyTest, AllowsOnlyUnreferencedMarker) {
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object =
      makeObject(llvm::Triple(Test.Triple), false, false, "f0", {}, false,
                 false, "generic", {}, false, false, false, false, false, false,
                 std::string(Marker));
  EXPECT_FALSE(ObjectReader::read(Object, Test.Architecture)) << Test.Triple;
  auto Graph =
      ObjectReader::read(Object, Test.Architecture,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  ASSERT_TRUE(Graph) << Test.Triple;
  EXPECT_TRUE(std::none_of(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_fltused"; }));
}

TEST(ObjectReaderTest, RejectsUnresolvedMinGWFltusedUnderDefaultPolicy) {
  REQUIRE_LLVM_TARGET("x86_64-w64-windows-gnu");
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto GNUCOFF =
      makeObject(llvm::Triple("x86_64-w64-windows-gnu"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(Marker));
  EXPECT_FALSE(ObjectReader::read(GNUCOFF, Target::X86_64));
}

TEST_P(MSVCFltusedPolicyTest, RejectsReferencedMarker) {
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  constexpr std::string_view Referenced = R"(
.data
.p2align 3
.quad _fltused
)";
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object =
      makeObject(llvm::Triple(Test.Triple), false, false, "f0", {}, false,
                 false, "generic", {}, false, false, false, false, false, false,
                 std::string(Marker) + std::string(Referenced));
  EXPECT_FALSE(
      ObjectReader::read(Object, Test.Architecture,
                         ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused))
      << Test.Triple;
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, MSVCFltusedPolicyTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-pc-windows-msvc", Target::AArch64}),
    objectArchitectureCaseName);

TEST(ObjectReaderTest, DoesNotApplyMSVCFltusedPolicyToELF) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto ELF =
      makeObject(llvm::Triple("x86_64-unknown-linux-gnu"), false, false, "f0",
                 {}, false, false, "generic", {}, false, false, false, false,
                 false, false, ".globl _fltused\n.quad _fltused\n");
  EXPECT_FALSE(ObjectReader::read(
      ELF, Target::X86_64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(ObjectReaderTest, RejectsUnrelatedMSVCUndefinedSymbolWithFltusedPolicy) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto OtherUndefined =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), true);
  EXPECT_FALSE(ObjectReader::read(
      OtherUndefined, Target::X86_64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(ObjectReaderTest, ReadsAArch64MSVCFloatingPointObjectWithFltusedPolicy) {
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  const auto ARM64FP =
      makeObject(llvm::Triple("aarch64-pc-windows-msvc"), false, false, "f0",
                 {}, false, false, "generic", {}, true, true, false, false,
                 false, false, {}, false, false, true);
  auto ParsedARM64 =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(ARM64FP.data()),
                          ARM64FP.size()),
          "arm64-floating.obj"));
  ASSERT_TRUE(static_cast<bool>(ParsedARM64));
  for (const auto &Symbol : (*ParsedARM64)->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    EXPECT_NE(*Name, "_fltused");
  }
  EXPECT_TRUE(ObjectReader::read(
      ARM64FP, Target::AArch64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(EHFrameTest, AcceptsUnwindlessGraphWithoutSemanticFunctions) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unwindless.o"));
  ASSERT_TRUE(Graph.addSection(Section{
      "__text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}}));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));

  EXPECT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_TRUE(validateMachOEHFrameCoverage(Graph));
}

TEST(EHFrameTest, RejectsUnwindlessSemanticFunction) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("unwindless.o"));
  ASSERT_TRUE(Graph.addSection(Section{
      "__text", SectionKind::Text, 4, 4, 0, 0, {0xC0, 0x03, 0x5F, 0xD6}}));
  ASSERT_TRUE(Graph.addSymbol(Symbol{"_f0", 0, 0, 4, true, {}, true}));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));

  EXPECT_TRUE(normalizeMachOEHFrame(Graph));
  EXPECT_FALSE(validateMachOEHFrameCoverage(Graph));
}

TEST(EHFrameTest, RequiresTypeWrapperCoverage) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Object = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                           "f0", {}, false, false, "generic", {}, true, false,
                           false, false, false, false, {}, false, true);
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(compactUnwindToEHFrame(*Graph));
  ASSERT_TRUE(layout(*Graph, 0, 0x4000));
  ASSERT_TRUE(normalizeMachOEHFrame(*Graph));
  ASSERT_TRUE(validateMachOEHFrameCoverage(*Graph));

  const auto Starts = machOEHFrameStarts(*Graph, 0);
  ASSERT_TRUE(Starts);
  const auto T0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  const auto F0 =
      std::find_if(Graph->symbols().begin(), Graph->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f0"; });
  ASSERT_NE(T0, Graph->symbols().end());
  ASSERT_NE(F0, Graph->symbols().end());
  const uint64_t T0Address =
      Graph->sections()[T0->Section].Address + T0->Offset;
  const uint64_t F0Address =
      Graph->sections()[F0->Section].Address + F0->Offset;
  ASSERT_NE(std::find(Starts->begin(), Starts->end(), T0Address),
            Starts->end());
  ASSERT_NE(std::find(Starts->begin(), Starts->end(), F0Address),
            Starts->end());

  const auto EH =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph->sections().end());
  const SectionId EHId = static_cast<SectionId>(EH - Graph->sections().begin());
  auto Content = Graph->sections()[EHId].Content;
  bool Mutated = false;
  for (size_t Offset = 0; Offset + 16 <= Content.size();) {
    uint32_t Length = 0;
    std::memcpy(&Length, Content.data() + Offset, sizeof(Length));
    if (Length == 0)
      break;
    uint32_t CIE = 0;
    std::memcpy(&CIE, Content.data() + Offset + 4, sizeof(CIE));
    if (CIE != 0) {
      const size_t Field = Offset + 8;
      int64_t Delta = 0;
      std::memcpy(&Delta, Content.data() + Field, sizeof(Delta));
      if (static_cast<int64_t>(Graph->sections()[EHId].Address + Field) +
              Delta ==
          static_cast<int64_t>(T0Address)) {
        Delta = static_cast<int64_t>(F0Address -
                                     (Graph->sections()[EHId].Address + Field));
        std::memcpy(Content.data() + Field, &Delta, sizeof(Delta));
        Mutated = true;
        break;
      }
    }
    Offset += Length + 4;
  }
  ASSERT_TRUE(Mutated);
  LinkGraph Corrupted(Graph->target(), Graph->endianness(), Graph->format());
  ASSERT_TRUE(Corrupted.beginInput("corrupted-eh-frame"));
  for (SectionId I = 0; I < Graph->sections().size(); ++I) {
    auto SectionValue = Graph->sections()[I];
    if (I == EHId)
      SectionValue.Content = std::move(Content);
    ASSERT_TRUE(Corrupted.addSection(std::move(SectionValue)));
  }
  for (const auto &Symbol : Graph->symbols())
    ASSERT_TRUE(Corrupted.addSymbol(Symbol));
  for (const auto &Relocation : Graph->relocations())
    ASSERT_TRUE(Corrupted.addRelocation(Relocation));
  for (const auto &Rebase : Graph->rebases())
    ASSERT_TRUE(Corrupted.addRebase(Rebase));
  for (const auto &Reference : Graph->ehFrameReferences())
    ASSERT_TRUE(Corrupted.addEHFrameReference(Reference));
  EXPECT_FALSE(validateMachOEHFrameCoverage(Corrupted));
}

TEST(EHFrameTest, CollapsesAliasedSemanticFunctionAddresses) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Object = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                           "f0", {}, false, false, "generic", {}, true, false,
                           false, false, false, false, {}, false, true);
  auto Covered = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Covered);
  ASSERT_TRUE(compactUnwindToEHFrame(*Covered));
  ASSERT_TRUE(layout(*Covered, 0, 0x4000));
  auto &Symbols = const_cast<std::vector<Symbol> &>(Covered->symbols());
  const auto T0 =
      std::find_if(Symbols.begin(), Symbols.end(),
                   [](const auto &Symbol) { return Symbol.Name == "_t0"; });
  ASSERT_NE(T0, Symbols.end());
  const SectionId T0Section = T0->Section;
  const uint64_t T0Offset = T0->Offset;
  const uint64_t T0Size = T0->Size;
  Symbols.push_back(
      Symbol{"_t1", T0Section, T0Offset, T0Size, true, std::nullopt, true});
  ASSERT_TRUE(normalizeMachOEHFrame(*Covered));
  EXPECT_TRUE(validateMachOEHFrameCoverage(*Covered));
  Symbols.back().Offset = T0Offset + 1;
  EXPECT_FALSE(validateMachOEHFrameCoverage(*Covered));
}

TEST(EHFrameTest, DecodesStrictInt64SLEB128) {
  const std::array<WasmEdge::Byte, 10> Max{0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                           0xFF, 0xFF, 0xFF, 0xFF, 0x00};
  const std::array<WasmEdge::Byte, 10> Min{0x80, 0x80, 0x80, 0x80, 0x80,
                                           0x80, 0x80, 0x80, 0x80, 0x7F};
  auto MaxValue = Internal::decodeSLEB128(Max);
  auto MinValue = Internal::decodeSLEB128(Min);
  ASSERT_TRUE(MaxValue);
  ASSERT_TRUE(MinValue);
  EXPECT_EQ(*MaxValue, INT64_MAX);
  EXPECT_EQ(*MinValue, INT64_MIN);
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7E}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 11>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}));
  EXPECT_FALSE(Internal::decodeSLEB128(std::array<WasmEdge::Byte, 10>{
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
}

TEST(EHFrameTest, ChecksResolvedAddressArithmetic) {
  EXPECT_EQ(Internal::resolveMachOFDEAddress(0, 0, 0, INT64_MAX),
            static_cast<uint64_t>(INT64_MAX));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(UINT64_MAX, 0, 0, INT64_MIN),
            static_cast<uint64_t>(INT64_MAX));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(UINT64_MAX - 10, 4, 3, 2),
            UINT64_MAX - 1);
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(UINT64_MAX - 10, 8, 3, 1));
  EXPECT_EQ(Internal::resolveMachOFDEAddress(5, 4, 3, -2), 10U);
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(0, 0, 1, -2));
  EXPECT_FALSE(Internal::resolveMachOFDEAddress(0, 0, 0, INT64_MIN));
}

TEST(EHFrameTest, RejectsMalformedRecordsAtomically) {
  LinkGraph Graph(Target::AArch64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("malformed.o"));
  ASSERT_TRUE(Graph.addSection(Section{"__eh_frame",
                                       SectionKind::Unwind,
                                       8,
                                       8,
                                       0,
                                       0,
                                       {4, 0, 0, 0, 0, 0, 0, 0},
                                       SectionPurpose::EHFrame}));
  const auto Before = Graph.sections()[0].Content;
  EXPECT_FALSE(normalizeMachOEHFrame(Graph));
  EXPECT_EQ(Graph.sections()[0].Content, Before);
}

TEST(EHFrameTest, ReportsFDEInitialLocationFields) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAArch64MachOEHFrameObject();
  const auto Object = machOEHFrameObject(Bytes);
  ASSERT_NE(Object.Content, 0U);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "eh-frame.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> Frame(Content->bytes_begin(),
                                            Content->bytes_end());
    auto Fields = machOEHFrameFields(Frame, Target::AArch64);
    ASSERT_TRUE(Fields);
    ASSERT_EQ(Fields->size(), 1U);
    return;
  }
  FAIL() << "generated object has no EH frame section";
}

TEST(EHFrameTest, RejectsReferenceOutsideFDEInitialLocationField) {
  auto Source = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Source));
  ASSERT_EQ(Source.ehFrameReferences().size(), 1U);
  auto &References =
      const_cast<std::vector<EHFrameReference> &>(Source.ehFrameReferences());
  References.front().Offset = 8;
  const auto Before = Source.sections().back().Content;
  EXPECT_FALSE(normalizeMachOEHFrame(Source));
  EXPECT_EQ(Source.sections().back().Content, Before);

  auto WrongSection = makeCompactUnwindGraph(Target::AArch64, 0x02000000);
  ASSERT_TRUE(compactUnwindToEHFrame(WrongSection));
  auto &WrongReferences = const_cast<std::vector<EHFrameReference> &>(
      WrongSection.ehFrameReferences());
  WrongReferences.front().Section = 0;
  EXPECT_FALSE(normalizeMachOEHFrame(WrongSection));
}

TEST(ObjectReaderTest, ParsesPairedAArch64MachOEHFrameReference) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAArch64MachOEHFrameObject();
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  const auto &Reference = Graph->ehFrameReferences().front();
  auto Fields = machOEHFrameFields(Graph->sections()[Reference.Section].Content,
                                   Target::AArch64);
  ASSERT_TRUE(Fields);
  EXPECT_EQ(*Fields, (std::set<size_t>{Reference.Offset}));
  EXPECT_EQ(Graph->sections()[Reference.Section].Purpose,
            SectionPurpose::EHFrame);
  EXPECT_EQ(Graph->symbols()[Reference.Symbol].Name, "_f0");
  EXPECT_TRUE(std::none_of(
      Graph->relocations().begin(), Graph->relocations().end(),
      [&](const auto &Value) { return Value.Section == Reference.Section; }));
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOEHFramePairs) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Original = makeAArch64MachOEHFrameObject();
  const auto Object = machOEHFrameObject(Original);
  ASSERT_EQ(Object.Relocations.size(), 2U);
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message,
                         DiagnosticKind Kind = DiagnosticKind::Malformed) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
    EXPECT_EQ(Result.error().Kind, Kind);
  };
  const auto SetType = [](std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                          uint32_t Type) {
    auto Word = read32le(Bytes, Offset + 4);
    write32le(Bytes, Offset + 4, (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28));
  };

  auto OrphanSubtractor = Original;
  SetType(OrphanSubtractor, Object.Relocations[1],
          llvm::MachO::ARM64_RELOC_SUBTRACTOR);
  Reject(std::move(OrphanSubtractor), "subtractor is not followed by unsigned");

  auto OrphanUnsigned = Original;
  SetType(OrphanUnsigned, Object.Relocations[0],
          llvm::MachO::ARM64_RELOC_UNSIGNED);
  Reject(std::move(OrphanUnsigned), "unsigned relocation lacks subtractor");

  auto WrongOrder = Original;
  SetType(WrongOrder, Object.Relocations[0], llvm::MachO::ARM64_RELOC_UNSIGNED);
  SetType(WrongOrder, Object.Relocations[1],
          llvm::MachO::ARM64_RELOC_SUBTRACTOR);
  Reject(std::move(WrongOrder), "unsigned relocation lacks subtractor");

  auto MismatchedOffset = Original;
  write32le(MismatchedOffset, Object.Relocations[1], 25);
  Reject(std::move(MismatchedOffset), "pair addresses differ");

  auto UnsupportedSymbol = Original;
  auto SubtractorWord = read32le(UnsupportedSymbol, Object.Relocations[0] + 4);
  auto UnsignedWord = read32le(UnsupportedSymbol, Object.Relocations[1] + 4);
  write32le(UnsupportedSymbol, Object.Relocations[0] + 4,
            (SubtractorWord & UINT32_C(0xFF000000)) |
                (UnsignedWord & UINT32_C(0x00FFFFFF)));
  Reject(std::move(UnsupportedSymbol), "subtractor symbol",
         DiagnosticKind::Unsupported);

  auto NonFDEField = Original;
  write32le(NonFDEField, Object.Relocations[0], 8);
  write32le(NonFDEField, Object.Relocations[1], 8);
  Reject(std::move(NonFDEField), "not an FDE initial-location field");
}

TEST(ObjectReaderTest, PreservesX86MachOEHFrameRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::any_of(Graph->relocations().begin(),
                          Graph->relocations().end(), [&](const auto &Value) {
                            return Graph->sections()[Value.Section].Purpose ==
                                       SectionPurpose::EHFrame &&
                                   Value.Type ==
                                       llvm::MachO::X86_64_RELOC_UNSIGNED;
                          }));
  EXPECT_TRUE(Graph->ehFrameReferences().empty());
}

TEST(ObjectReaderTest, AssociatesRawX86_64MachOFDEReference) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_TRUE(Object.Relocations.empty());
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  const auto &Reference = Graph->ehFrameReferences().front();
  ASSERT_LT(Reference.Section, Graph->sections().size());
  ASSERT_LT(Reference.Symbol, Graph->symbols().size());
  EXPECT_EQ(Graph->sections()[Reference.Section].Purpose,
            SectionPurpose::EHFrame);
  auto Fields = machOEHFrameFields(Graph->sections()[Reference.Section].Content,
                                   Target::X86_64);
  ASSERT_TRUE(Fields);
  EXPECT_EQ(*Fields, (std::set<size_t>{Reference.Offset}));
  EXPECT_EQ(Reference.Offset, Fixture.Field);
  EXPECT_EQ(Graph->symbols()[Reference.Symbol].Name, "_f1");
}

TEST(ObjectReaderTest, RejectsStrictRawX86_64MachOFDETargets) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Reject = [](X86_64MachOEHFrameFixture Fixture, uint64_t Value,
                         std::string_view Message) {
    const auto Object = machOEHFrameObject(Fixture.Bytes);
    ASSERT_NE(Object.Content, 0U);
    write64le(Fixture.Bytes, Object.Content + Fixture.Field, Value);
    auto Result = ObjectReader::read(Fixture.Bytes, Target::X86_64);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  const auto Fixture = makeX86_64MachOEHFrameFixture(true);
  Reject(Fixture, Fixture.DataAddress + 1 - Fixture.FieldAddress,
         "unresolved x86_64 Mach-O FDE target");
  Reject(Fixture, UINT64_C(0x8000000000000000),
         "x86_64 Mach-O FDE target address overflows");
  Reject(Fixture, Fixture.DataAddress - Fixture.FieldAddress,
         "x86_64 Mach-O FDE target is not a text symbol");
}

TEST(ObjectReaderTest, RejectsMalformedRawX86_64MachOEHFrame) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  write32le(Fixture.Bytes, Object.Content, 1);
  auto Result = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "malformed x86_64 Mach-O EH frame");
}

TEST(ObjectReaderTest, SkipsRelocatedX86_64MachOFDEReference) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(false);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_EQ(Object.Relocations.size(), 1U);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  const auto Relocation =
      std::find_if(Graph->relocations().begin(), Graph->relocations().end(),
                   [&](const auto &Value) {
                     return Graph->sections()[Value.Section].Purpose ==
                                SectionPurpose::EHFrame &&
                            Value.Offset == Fixture.Field;
                   });
  ASSERT_NE(Relocation, Graph->relocations().end());
  EXPECT_TRUE(std::none_of(Graph->ehFrameReferences().begin(),
                           Graph->ehFrameReferences().end(),
                           [&](const auto &Reference) {
                             return Reference.Section == Relocation->Section &&
                                    Reference.Offset == Relocation->Offset;
                           }));
}

TEST(ObjectReaderTest, SelectsDeterministicRawX86_64MachOFDEAlias) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true, true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_TRUE(Object.Relocations.empty());
  auto First = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  auto Second = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(First);
  ASSERT_TRUE(Second);
  const auto AssertAliasedSymbols = [](const LinkGraph &Graph) {
    const auto F1 =
        std::find_if(Graph.symbols().begin(), Graph.symbols().end(),
                     [](const auto &Symbol) { return Symbol.Name == "_f1"; });
    const auto Alias = std::find_if(
        Graph.symbols().begin(), Graph.symbols().end(),
        [](const auto &Symbol) { return Symbol.Name == "_f1_alias"; });
    ASSERT_NE(F1, Graph.symbols().end());
    ASSERT_NE(Alias, Graph.symbols().end());
    ASSERT_EQ(Graph.sections()[F1->Section].Kind, SectionKind::Text);
    ASSERT_EQ(Graph.sections()[Alias->Section].Kind, SectionKind::Text);
    EXPECT_EQ(Graph.sections()[F1->Section].InputAddress + F1->Offset,
              Graph.sections()[Alias->Section].InputAddress + Alias->Offset);
  };
  AssertAliasedSymbols(*First);
  AssertAliasedSymbols(*Second);
  ASSERT_EQ(First->ehFrameReferences().size(), 1U);
  ASSERT_EQ(Second->ehFrameReferences().size(), 1U);
  const auto &FirstReference = First->ehFrameReferences().front();
  const auto &SecondReference = Second->ehFrameReferences().front();
  ASSERT_LT(FirstReference.Symbol, First->symbols().size());
  ASSERT_LT(SecondReference.Symbol, Second->symbols().size());
  const auto &FirstSymbol = First->symbols()[FirstReference.Symbol];
  const auto &SecondSymbol = Second->symbols()[SecondReference.Symbol];
  EXPECT_EQ(FirstSymbol.Name, SecondSymbol.Name);
  EXPECT_NE(FirstSymbol.Name, "$section");
  const auto F1 =
      std::find_if(First->symbols().begin(), First->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f1"; });
  ASSERT_NE(F1, First->symbols().end());
  EXPECT_EQ(First->sections()[FirstSymbol.Section].InputAddress +
                FirstSymbol.Offset,
            First->sections()[F1->Section].InputAddress + F1->Offset);
}

TEST(ObjectReaderTest, ParsesCompactOnlyAArch64MachOUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto CompactOnly =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(CompactOnly, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  EXPECT_EQ(Graph->symbols()[Record.Function].Name, "_f0");
  EXPECT_GT(Record.Length, 0U);
  EXPECT_EQ(Record.Encoding, 0x02000000U);
  EXPECT_FALSE(Record.Personality);
  EXPECT_FALSE(Record.LSDA);
  EXPECT_FALSE(Record.FDE);
  EXPECT_TRUE(std::none_of(Graph->sections().begin(), Graph->sections().end(),
                           [](const Section &Value) {
                             return Value.Purpose ==
                                    SectionPurpose::CompactUnwind;
                           }));
}

TEST(ObjectReaderTest, ParsesCompactUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  EXPECT_FALSE(Graph->compactUnwind().empty());
}

TEST(ObjectReaderTest, ResolvesCompactUnwindTargetOffsets) {
  auto Local =
      Internal::resolveCompactUnwindTargetOffset(false, 0x1000, 0, 0x1018);
  auto ExternalNegative = Internal::resolveCompactUnwindTargetOffset(
      true, 0x1000, 0x20, UINT64_MAX - 7);
  ASSERT_TRUE(Local);
  ASSERT_TRUE(ExternalNegative);
  EXPECT_EQ(*Local, 0x18U);
  EXPECT_EQ(*ExternalNegative, 0x18U);
  EXPECT_FALSE(
      Internal::resolveCompactUnwindTargetOffset(false, 0x1000, 0, 0x0FFF));
  EXPECT_FALSE(Internal::resolveCompactUnwindTargetOffset(
      true, 0, UINT64_MAX, static_cast<uint64_t>(INT64_MAX)));
}

TEST(ObjectReaderTest, ValidatesDecodedAArch64CompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> AArch64{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Records =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, AArch64);
  ASSERT_TRUE(Records);
  ASSERT_EQ(Records->size(), 1U);
  EXPECT_EQ((*Records)[0].Length, 8U);
}

TEST(ObjectReaderTest, ValidatesDecodedX86_64CompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> X86_64{
      {{0, llvm::MachO::X86_64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Records =
      Internal::parseCompactUnwindSection(Target::X86_64, Content, X86_64);
  ASSERT_TRUE(Records);
  ASSERT_EQ(Records->size(), 1U);
  EXPECT_EQ((*Records)[0].Encoding, 0x02000000U);
}

TEST(ObjectReaderTest, RejectsDecodedARMCompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> AArch64{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  EXPECT_FALSE(
      Internal::parseCompactUnwindSection(Target::ARM, Content, AArch64));
}

TEST(ObjectReaderTest, DiagnosesMalformedDecodedCompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  const auto Diagnose = [&](Internal::CompactUnwindRelocation Relocation) {
    auto Result = Internal::parseCompactUnwindSection(
        Target::AArch64, Content,
        WasmEdge::Span<const Internal::CompactUnwindRelocation>(&Relocation,
                                                                1));
    EXPECT_FALSE(Result);
    return Result.error().Message;
  };
  EXPECT_EQ(
      Diagnose({8, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}),
      "unsupported compact unwind relocation field");
  EXPECT_EQ(
      Diagnose({0, llvm::MachO::ARM64_RELOC_UNSIGNED, 4, false, false, false}),
      "unsupported compact unwind relocation");
  auto Missing =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, {});
  ASSERT_FALSE(Missing);
  EXPECT_EQ(Missing.error().Message,
            "compact unwind record lacks function relocation");
  write64le(Content, 16, 1);
  std::array<Internal::CompactUnwindRelocation, 1> Function{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Unrelocated =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, Function);
  ASSERT_FALSE(Unrelocated);
  EXPECT_EQ(Unrelocated.error().Message,
            "compact unwind target lacks relocation");
}

TEST(ObjectReaderTest, AssociatesZeroDwarfInputByFunctionFDE) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Mixed =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  clearMachOSectionRelocations(Mixed, "__text");
  const auto Offsets = compactUnwindObjectOffsets(Mixed);
  ASSERT_NE(Offsets.Content, 0U);
  EXPECT_EQ(read32le(Mixed, Offsets.Content + 12), UINT32_C(0x03000000));
  const auto FDEOffsets = ehFrameFDEOffsets(Mixed);
  ASSERT_FALSE(FDEOffsets.empty());
  EXPECT_GT(FDEOffsets.front(), 0U);
  auto Graph = ObjectReader::read(Mixed, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_GT(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, 0U);
}

TEST(ObjectReaderTest, AssociatesExactNonzeroDwarfOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Mixed =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  clearMachOSectionRelocations(Mixed, "__text");
  const auto Offsets = compactUnwindObjectOffsets(Mixed);
  ASSERT_NE(Offsets.Content, 0U);
  std::optional<LinkGraph> Graph;
  uint32_t FDEOffset = 0;
  for (const uint32_t Candidate : ehFrameFDEOffsets(Mixed)) {
    auto CandidateObject = Mixed;
    write32le(CandidateObject, Offsets.Content + 12,
              UINT32_C(0x03000000) | Candidate);
    auto Parsed = ObjectReader::read(CandidateObject, Target::AArch64);
    if (!Parsed)
      continue;
    ASSERT_FALSE(Graph);
    FDEOffset = Candidate;
    Graph.emplace(std::move(*Parsed));
  }
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, FDEOffset);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, AssociatesDwarfFDEThroughEquivalentMachOSymbols) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAliasedAArch64MachODwarfObject();
  const auto Inventory = collectCompactUnwindInventory(Bytes);
  ASSERT_TRUE(Inventory.HasCompactUnwind);
  ASSERT_TRUE(Inventory.HasEHFrame);
  ASSERT_EQ(Inventory.Records.size(), 1U);
  EXPECT_EQ(Inventory.Records[0].Encoding, UINT32_C(0x03000018));

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  ASSERT_TRUE(Record.FDE);
  EXPECT_EQ(Graph->symbols()[*Record.FDE].Offset, 24U);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, AssociatesSectionTargetDwarfFDEAtFunctionOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeSectionTargetAArch64MachODwarfObject();
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-target.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  bool SectionTarget = false;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() != 32 ||
          Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const auto Symbol = Relocation.getSymbol();
      SectionTarget = Symbol == (*Object)->symbol_end();
    }
  }
  ASSERT_TRUE(SectionTarget);

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  const auto &Function = Graph->symbols()[Graph->compactUnwind()[0].Function];
  EXPECT_EQ(Function.Offset, 4U);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, 24U);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, RejectsOverflowingSectionTargetDwarfFDEAddend) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeSectionTargetAArch64MachODwarfObject();
  setMachOEHFrameInitialLocation(Bytes, UINT64_C(0x8000000000000000));
  auto Result = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("DWARF FDE target address overflows"),
            std::string::npos);
}

TEST(ObjectReaderTest, SelectsEncodedDwarfFDEFromMultipleCandidates) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeMultipleFDEAArch64MachODwarfObject(UINT32_C(0x03000000));
  const auto FDEs = ehFrameFDEOffsets(Bytes);
  ASSERT_EQ(FDEs.size(), 2U);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_NE(Offsets.Content, 0U);
  write32le(Bytes, Offsets.Content + 12, UINT32_C(0x03000000) | FDEs.back());

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset,
            FDEs.back());
}

TEST(ObjectReaderTest, RejectsAmbiguousZeroOffsetDwarfFDE) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes =
      makeMultipleFDEAArch64MachODwarfObject(UINT32_C(0x03000000));
  ASSERT_EQ(ehFrameFDEOffsets(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, ClassifiesMachOCustomFunctionSectionAsText) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeRegularMachOFunctionSectionObject();
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "custom-section.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  bool PriorReadOnly = false;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__custom_text")
      continue;
    const auto *MachO =
        llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
    ASSERT_NE(MachO, nullptr);
    const uint32_t Flags =
        MachO->getSection64(Section.getRawDataRefImpl()).flags;
    PriorReadOnly = !Section.isText() &&
                    (Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                              llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) == 0;
  }
  ASSERT_TRUE(PriorReadOnly);
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  ASSERT_LT(Function->Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Function->Section].Name, "__custom_text");
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
  const auto DataObject =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, {}, false, false, false, false, true);
  auto DataGraph = ObjectReader::read(DataObject, Target::AArch64);
  ASSERT_TRUE(DataGraph);
  const auto Data = std::find_if(
      DataGraph->symbols().begin(), DataGraph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_data"; });
  ASSERT_NE(Data, DataGraph->symbols().end());
  EXPECT_NE(DataGraph->sections()[Data->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, ClassifiesUnusedMachOInstructionSectionAsText) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Object =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, true, "generic", {}, false, false, false, false, false,
                 false, {}, true, false, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unused-instruction.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  bool HasInstructionSection = false;
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__unused_text")
      continue;
    const uint32_t Flags =
        MachO->getSection64(Section.getRawDataRefImpl()).flags;
    HasInstructionSection =
        !Section.isText() &&
        (Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                  llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) != 0;
  }
  ASSERT_TRUE(HasInstructionSection);
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  ASSERT_LT(Function->Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, Classifies32BitMachOInstructionSectionAsText) {
  REQUIRE_LLVM_TARGET("armv7-apple-ios");
  const auto Object =
      makeAssemblyObject(llvm::Triple("armv7-apple-ios"),
                         ".section __TEXT,__custom,regular,pure_instructions\n"
                         ".globl _function\n_function:\nbx lr\n");
  auto Graph = ObjectReader::read(Object, Target::ARM);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, ResolvesX86_64MachOSectionTargetRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".text\nnop\nLtarget:\nret\n.data\n.globl _pointer\n_pointer:\n.quad "
      "Ltarget\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  const auto &TargetSymbol = Graph->symbols()[Relocation.Symbol];
  ASSERT_LT(TargetSymbol.Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[TargetSymbol.Section].Kind, SectionKind::Text);
  EXPECT_EQ(Relocation.Addend, 1);
  EXPECT_FALSE(Relocation.AddendIsImplicit);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  auto Value = Internal::readUnsigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, Graph->sections()[TargetSymbol.Section].Address +
                        TargetSymbol.Offset + 1);
}

TEST(ObjectReaderTest, NormalizesX86_64MachOAbsoluteSectionRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".data\n.globl _pointer\n_pointer:\n.quad Lstring\n"
      ".section __TEXT,__cstring,cstring_literals\n.space 7\nLstring:\n"
      ".asciz \"value\"\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  ASSERT_LT(Symbol.Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Symbol.Section].Name, "__cstring");
  EXPECT_EQ(Relocation.Addend, 7);
  EXPECT_FALSE(Relocation.AddendIsImplicit);

  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  auto Value = Internal::readUnsigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, Graph->sections()[Symbol.Section].Address + 7);
}

TEST(ObjectReaderTest, PreservesExternalX86_64MachOPCRelativeRelocation) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".text\n.globl _entry\n_entry:\ncallq _external\nretq\n"
      ".section __TEXT,__other,regular,pure_instructions\n"
      ".globl _external\n_external:\nretq\n");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "external-relocation.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  bool SymbolTarget = false;
  for (const auto &Section : MachO->sections()) {
    for (const auto &Relocation : Section.relocations()) {
      const auto Symbol = Relocation.getSymbol();
      ASSERT_NE(Symbol, MachO->symbol_end());
      auto Name = Symbol->getName();
      ASSERT_TRUE(static_cast<bool>(Name));
      EXPECT_EQ(*Name, "_external");
      SymbolTarget = true;
    }
  }
  ASSERT_TRUE(SymbolTarget);
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  EXPECT_EQ(Graph->symbols()[Relocation.Symbol].Name, "_external");
  EXPECT_EQ(Relocation.Type, llvm::MachO::X86_64_RELOC_BRANCH);
  EXPECT_EQ(Relocation.Addend, 0);
  EXPECT_TRUE(Relocation.AddendIsImplicit);
  EXPECT_TRUE(Relocation.External);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  auto Value = Internal::readSigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  const uint64_t PatchEnd = Graph->sections()[Relocation.Section].Address +
                            Relocation.Offset + Relocation.PatchSize;
  uint64_t Resolved = 0;
  if (*Value < 0) {
    const uint64_t Magnitude =
        static_cast<uint64_t>(-(*Value + 1)) + UINT64_C(1);
    ASSERT_LE(Magnitude, PatchEnd);
    Resolved = PatchEnd - Magnitude;
  } else {
    const uint64_t Displacement = static_cast<uint64_t>(*Value);
    ASSERT_LE(Displacement, UINT64_MAX - PatchEnd);
    Resolved = PatchEnd + Displacement;
  }
  EXPECT_EQ(Resolved,
            Graph->sections()[Symbol.Section].Address + Symbol.Offset);
}

TEST(ObjectReaderTest, NormalizesX86_64MachOSectionRelocationOffsets) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  constexpr std::string_view Assembly = R"(
.section __TEXT,__text,regular,pure_instructions
.globl _entry
_entry:
cmpb $0, Ldata(%rip)
cmpw $256, Ldata(%rip)
cmpl $65536, Ldata(%rip)
cmpq %rax, Ldata(%rip)
callq Ltarget
ret
.section __TEXT,__other,regular,pure_instructions
.space 9
Ltarget:
ret
.data
.space 13
Ldata:
.quad 0
)";
  const auto Bytes = makeAssemblyObject(llvm::Triple("x86_64-apple-macosx"),
                                        std::string(Assembly));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-relocations.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);

  struct RawRelocation {
    uint64_t Offset;
    uint32_t Type;
    int64_t Value;
    uint64_t TargetOffset;
    std::string TargetSection;
  };
  std::vector<RawRelocation> RawRelocations;
  std::set<std::string> SymbolNames;
  for (const auto &Symbol : MachO->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    SymbolNames.insert(Name->str());
  }
  EXPECT_EQ(SymbolNames.count("Ldata"), 0U);
  EXPECT_EQ(SymbolNames.count("Ltarget"), 0U);

  std::set<uint32_t> Types;
  for (const auto &Section : MachO->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__text")
      continue;
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> SectionBytes(Content->bytes_begin(),
                                                   Content->bytes_end());
    for (const auto &Relocation : Section.relocations()) {
      const auto Type = static_cast<uint32_t>(Relocation.getType());
      ASSERT_TRUE(Type == llvm::MachO::X86_64_RELOC_SIGNED ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_1 ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ||
                  Type == llvm::MachO::X86_64_RELOC_BRANCH);
      EXPECT_EQ(Relocation.getSymbol(), MachO->symbol_end());
      const auto TargetSection =
          MachO->getRelocationSection(Relocation.getRawDataRefImpl());
      ASSERT_NE(TargetSection, MachO->section_end());
      auto TargetName = TargetSection->getName();
      ASSERT_TRUE(static_cast<bool>(TargetName));
      const int64_t Suffix = Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                          : 0;
      auto Value = Internal::readSigned(SectionBytes, Relocation.getOffset(), 4,
                                        Endianness::Little);
      ASSERT_TRUE(Value);
      const int64_t TargetOffset =
          *Value + static_cast<int64_t>(Section.getAddress()) +
          static_cast<int64_t>(Relocation.getOffset()) + 4 + Suffix -
          static_cast<int64_t>(TargetSection->getAddress());
      const uint64_t ExpectedOffset =
          Type == llvm::MachO::X86_64_RELOC_BRANCH ? 9 : 13;
      EXPECT_EQ(TargetOffset, static_cast<int64_t>(ExpectedOffset));
      RawRelocations.push_back({Relocation.getOffset(), Type, *Value,
                                ExpectedOffset, TargetName->str()});
      Types.insert(Type);
    }
  }
  ASSERT_EQ(RawRelocations.size(), 5U);
  EXPECT_EQ(std::count_if(RawRelocations.begin(), RawRelocations.end(),
                          [](const auto &Value) {
                            return Value.Type ==
                                   llvm::MachO::X86_64_RELOC_BRANCH;
                          }),
            1);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_1), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_2), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_4), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_BRANCH), 0U);

  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), RawRelocations.size());
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    SCOPED_TRACE(testing::Message()
                 << "type=" << Raw.Type << " offset=" << Raw.Offset
                 << " raw=" << Raw.Value << " target=" << Raw.TargetSection
                 << "+" << Raw.TargetOffset);
    EXPECT_FALSE(Relocation->AddendIsImplicit);
    EXPECT_EQ(Relocation->Addend, static_cast<int64_t>(Raw.TargetOffset));
    ASSERT_LT(Relocation->Symbol, Graph->symbols().size());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    EXPECT_EQ(Symbol.Offset, 0U);
    ASSERT_LT(Symbol.Section, Graph->sections().size());
    EXPECT_EQ(Graph->sections()[Symbol.Section].Name, Raw.TargetSection);
  }

  ASSERT_TRUE(layout(*Graph, 0x1000, 0x1000));
  const auto OrderedDelta = [](uint64_t TargetAddress, uint64_t PatchAddress) {
    return TargetAddress >= PatchAddress
               ? std::pair{true, TargetAddress - PatchAddress}
               : std::pair{false, PatchAddress - TargetAddress};
  };
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    const auto &PatchSection = Graph->sections()[Relocation->Section];
    const auto &TargetSection = Graph->sections()[Symbol.Section];
    EXPECT_NE(
        OrderedDelta(TargetSection.Address, PatchSection.Address),
        OrderedDelta(TargetSection.InputAddress, PatchSection.InputAddress));
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    const int64_t S =
        static_cast<int64_t>(Graph->sections()[Symbol.Section].Address);
    const int64_t P = static_cast<int64_t>(
        Graph->sections()[Relocation->Section].Address + Relocation->Offset);
    const int64_t Suffix = Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                           : Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                           : Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                            : 0;
    auto Patched =
        Internal::readSigned(Graph->sections()[Relocation->Section].Content,
                             Relocation->Offset, 4, Endianness::Little);
    ASSERT_TRUE(Patched);
    EXPECT_EQ(*Patched, S + Relocation->Addend - P - 4 - Suffix);
    EXPECT_EQ(P + *Patched + 4 + Suffix,
              S + static_cast<int64_t>(Raw.TargetOffset));
  }
}

TEST(ObjectReaderTest, RejectsInvalidX86_64MachOSectionRelocationOffsets) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Original = makeX86_64MachOSectionOffsetObject();
  const auto Relocation = x86_64MachOSectionRelocation(Original);
  ASSERT_NE(Relocation.Field, 0U);
  ASSERT_NE(Relocation.Relocation, 0U);
  ASSERT_GT(Relocation.TargetSize, Relocation.TargetOffset);
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message) {
    auto Result = ObjectReader::read(Bytes, Target::X86_64);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };
  const auto SetTargetOffset = [&](std::vector<WasmEdge::Byte> Bytes,
                                   int64_t Offset) {
    const int64_t Delta =
        Offset - static_cast<int64_t>(Relocation.TargetOffset);
    write32le(Bytes, Relocation.Field,
              static_cast<uint32_t>(Relocation.Raw + Delta));
    return Bytes;
  };

  Reject(SetTargetOffset(Original, -1),
         "x86_64 Mach-O section relocation target is negative");
  Reject(SetTargetOffset(Original, static_cast<int64_t>(Relocation.TargetSize)),
         "x86_64 Mach-O section relocation target is outside its section");
  Reject(SetTargetOffset(Original,
                         static_cast<int64_t>(Relocation.TargetSize + 1)),
         "x86_64 Mach-O section relocation target is outside its section");

  auto Overflow = Original;
  const size_t TextHeader = machOSectionHeaderOffset(Overflow, "__text");
  ASSERT_NE(TextHeader, 0U);
  write64le(Overflow, TextHeader + 32, UINT64_MAX);
  Reject(std::move(Overflow),
         "x86_64 Mach-O section relocation offset overflows");
}

TEST(ObjectReaderTest, RejectsUnsupportedX86_64MachOSectionNormalization) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Unsupported = makeX86_64MachOSectionOffsetObject();
  const auto Relocation = x86_64MachOSectionRelocation(Unsupported);
  ASSERT_NE(Relocation.Relocation, 0U);
  auto Word = read32le(Unsupported, Relocation.Relocation + 4);
  Word &= ~UINT32_C(0xF0000000);
  Word |= llvm::MachO::X86_64_RELOC_GOT << 28;
  write32le(Unsupported, Relocation.Relocation + 4, Word);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Unsupported.data()),
                          Unsupported.size()),
          "unsupported-section-relocation.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  const auto Section = MachO->section_begin();
  ASSERT_NE(Section, MachO->section_end());
  const auto InputRelocation = Section->relocation_begin();
  ASSERT_NE(InputRelocation, Section->relocation_end());
  EXPECT_EQ(InputRelocation->getType(), llvm::MachO::X86_64_RELOC_GOT);
  EXPECT_EQ(InputRelocation->getSymbol(), MachO->symbol_end());
  EXPECT_NE(MachO->getRelocationSection(InputRelocation->getRawDataRefImpl()),
            MachO->section_end());
  auto Result = ObjectReader::read(Unsupported, Target::X86_64);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "unsupported PC-relative x86_64 Mach-O section relocation type");
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
}

TEST(CompactUnwindTest, ConvertsSparseX86RBPRegisterSlots) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x01040081);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  const auto EH =
      std::find_if(Graph.sections().begin(), Graph.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph.sections().end());
  const std::vector<WasmEdge::Byte> CFI{0x83, 6, 0x8C, 4};
  EXPECT_NE(std::search(EH->Content.begin(), EH->Content.end(), CFI.begin(),
                        CFI.end()),
            EH->Content.end());
}

TEST(CompactUnwindTest, ConvertsAArch64FramePairs) {
  auto Graph = makeCompactUnwindGraph(Target::AArch64, 0x04000303);
  ASSERT_TRUE(compactUnwindToEHFrame(Graph));
  const auto &Bytes = Graph.sections().back().Content;
  const std::vector<WasmEdge::Byte> CFI{
      0x0C, 29,   16, 0x9E, 1,    0x9D, 2, 0x93, 3,  0x94, 4,    0x95, 5, 0x96,
      6,    0x05, 72, 7,    0x05, 73,   8, 0x05, 74, 9,    0x05, 75,   10};
  EXPECT_NE(std::search(Bytes.begin(), Bytes.end(), CFI.begin(), CFI.end()),
            Bytes.end());
}

TEST(CompactUnwindTest, ConvertsAArch64FramelessPairs) {
  const auto DecodeRules = [](WasmEdge::Span<const WasmEdge::Byte> Bytes,
                              uint64_t StackSize) {
    std::map<uint32_t, int64_t> Rules;
    std::vector<WasmEdge::Byte> Prefix{0x0C, 31};
    uint64_t Value = StackSize;
    do {
      WasmEdge::Byte Part = static_cast<WasmEdge::Byte>(Value & 0x7F);
      Value >>= 7;
      Prefix.push_back(Part | (Value == 0 ? 0 : 0x80));
    } while (Value != 0);
    Prefix.insert(Prefix.end(), {0x08, 30});
    auto Cursor =
        std::search(Bytes.begin(), Bytes.end(), Prefix.begin(), Prefix.end());
    EXPECT_NE(Cursor, Bytes.end());
    if (Cursor == Bytes.end())
      return Rules;
    Cursor += static_cast<ptrdiff_t>(Prefix.size());
    while (Cursor != Bytes.end()) {
      uint32_t Register = 0;
      if ((*Cursor & 0xC0) == 0x80) {
        Register = *Cursor++ & 0x3F;
      } else if (*Cursor++ == 0x05 && Cursor != Bytes.end()) {
        Register = *Cursor++;
      } else {
        break;
      }
      if (Cursor == Bytes.end())
        break;
      const uint64_t Factor = *Cursor++;
      Rules.emplace(Register, -8 * static_cast<int64_t>(Factor));
    }
    return Rules;
  };
  for (const auto &[Encoding, Expected] :
       std::array<std::pair<uint32_t, std::map<uint32_t, int64_t>>, 2>{
           {{{0x0200400F},
             {{19, 0},
              {20, -8},
              {21, -16},
              {22, -24},
              {23, -32},
              {24, -40},
              {25, -48},
              {26, -56}}},
            {{0x02021010}, {{27, 0}, {28, -8}}}}}) {
    auto Graph = makeCompactUnwindGraph(Target::AArch64, Encoding);
    ASSERT_TRUE(compactUnwindToEHFrame(Graph));
    const auto &Bytes = Graph.sections().back().Content;
    EXPECT_EQ(DecodeRules(Bytes, ((Encoding >> 12) & 0xFFF) * 16), Expected);
  }
  auto Reserved = makeCompactUnwindGraph(Target::AArch64, 0x02001020);
  EXPECT_FALSE(validateCompactUnwind(Reserved));
}

void expectNotFunctionStartFlagPreserved(Target Architecture,
                                         uint32_t Encoding) {
  auto Graph = makeCompactUnwindGraph(Architecture, Encoding);
  ASSERT_TRUE(validateCompactUnwind(Graph));
  ASSERT_TRUE(reserveMachOUnwindInfo(Graph));
  ASSERT_TRUE(layout(Graph, 0, 0x4000));
  ASSERT_TRUE(applyRelocations(Graph));
  ASSERT_TRUE(populateMachOUnwindInfo(Graph));
  const auto Unwind = std::find_if(
      Graph.sections().begin(), Graph.sections().end(),
      [](const auto &S) { return S.Purpose == SectionPurpose::UnwindInfo; });
  ASSERT_NE(Unwind, Graph.sections().end());
  const std::array<WasmEdge::Byte, 4> Encoded{
      0, 0,
      Architecture == Target::X86_64 ? WasmEdge::Byte{2} : WasmEdge::Byte{0},
      0x82};
  EXPECT_NE(std::search(Unwind->Content.begin(), Unwind->Content.end(),
                        Encoded.begin(), Encoded.end()),
            Unwind->Content.end());
}

TEST(CompactUnwindTest, PreservesAArch64NotFunctionStartFlag) {
  REQUIRE_RELOCATION_HANDLER(Target::AArch64);
  expectNotFunctionStartFlagPreserved(Target::AArch64, 0x82000000);
}

TEST(CompactUnwindTest, PreservesX86_64NotFunctionStartFlag) {
  REQUIRE_RELOCATION_HANDLER(Target::X86_64);
  expectNotFunctionStartFlagPreserved(Target::X86_64, 0x82020000);
}

TEST(CompactUnwindTest, ConvertsX86FramelessModes) {
  auto Immediate = makeCompactUnwindGraph(Target::X86_64, 0x02360804);
  ASSERT_TRUE(compactUnwindToEHFrame(Immediate));
  const std::vector<WasmEdge::Byte> ImmediateCFI{0x0C, 7,    0xB0, 3,    0x90,
                                                 1,    0x86, 2,    0x83, 3};
  EXPECT_NE(std::search(Immediate.sections().back().Content.begin(),
                        Immediate.sections().back().Content.end(),
                        ImmediateCFI.begin(), ImmediateCFI.end()),
            Immediate.sections().back().Content.end());

  std::vector<WasmEdge::Byte> Text(64);
  const std::array<WasmEdge::Byte, 5> Instruction{0x48, 0x81, 0xEC, 0x48, 0x38};
  std::copy(Instruction.begin(), Instruction.end(), Text.begin() + 2);
  auto Indirect = makeCompactUnwindGraph(Target::X86_64, 0x03056804, "_f0",
                                         std::move(Text));
  ASSERT_TRUE(compactUnwindToEHFrame(Indirect));
}

TEST(CompactUnwindTest, ValidatesAllDocumentedAArch64ModesEarly) {
  for (const uint32_t Encoding : {UINT32_C(0x04000F1F), UINT32_C(0x02001001)}) {
    auto Graph = makeCompactUnwindGraph(Target::AArch64, Encoding);
    EXPECT_TRUE(validateCompactUnwind(Graph));
  }
}

TEST(CompactUnwindTest, RejectsMalformedX86_64FramelessEncodings) {
  for (const uint32_t Encoding :
       {UINT32_C(0x02001C00), UINT32_C(0x02000406), UINT32_C(0x03056804)}) {
    auto Graph = makeCompactUnwindGraph(Target::X86_64, Encoding);
    EXPECT_FALSE(validateCompactUnwind(Graph));
    EXPECT_EQ(Graph.sections().size(), 1U);
  }
  auto Truncated = makeCompactUnwindGraph(Target::X86_64, 0x030E6804);
  EXPECT_FALSE(validateCompactUnwind(Truncated));
}

TEST(CompactUnwindTest, RejectsMalformedAArch64FramelessEncoding) {
  auto Reserved = makeCompactUnwindGraph(Target::AArch64, 0x02001020);
  EXPECT_FALSE(validateCompactUnwind(Reserved));
}

TEST(CompactUnwindTest, AcceptsX86DwarfFallbackRelocationAssociation) {
  auto Existing = makeCompactUnwindGraph(Target::X86_64, 0x01000000);
  ASSERT_TRUE(compactUnwindToEHFrame(Existing));
  const auto EH =
      std::find_if(Existing.sections().begin(), Existing.sections().end(),
                   [](const auto &Section) {
                     return Section.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Existing.sections().end());

  LinkGraph Graph(Target::X86_64, Endianness::Little, ObjectFormat::MachO);
  ASSERT_TRUE(Graph.beginInput("x86-dwarf.o"));
  auto Text = Graph.addSection(Section{"__text", SectionKind::Text, 1, 16, 0, 0,
                                       std::vector<WasmEdge::Byte>(16)});
  auto Frame = Graph.addSection(*EH);
  ASSERT_TRUE(Text && Frame);
  auto Function = Graph.addSymbol(Symbol{"_f0", *Text, 0, 16, true, {}, true});
  auto FDE = Graph.addSymbol(Symbol{"fde", *Frame, 16, 0, false});
  ASSERT_TRUE(Function && FDE);
  ASSERT_TRUE(Graph.addRelocation(
      Relocation{*Frame, 24, llvm::MachO::X86_64_RELOC_UNSIGNED, *Function, 0,
                 false, ObjectFormat::MachO, 8, false, true, false}));
  ASSERT_TRUE(Graph.addCompactUnwind(
      CompactUnwindRecord{*Function, 16, 0x04000000, {}, {}, *FDE}));
  EXPECT_TRUE(compactUnwindToEHFrame(Graph));
}

TEST(CompactUnwindTest, RejectsX86DwarfFallbackWithoutExactAssociation) {
  auto Graph = makeCompactUnwindGraph(Target::X86_64, 0x04000000);
  const auto Sections = Graph.sections();
  EXPECT_FALSE(compactUnwindToEHFrame(Graph));
  EXPECT_EQ(Graph.sections().size(), Sections.size());
  EXPECT_EQ(Graph.sections().front().Content, Sections.front().Content);
}

TEST(ObjectReaderTest, RejectsWrongDwarfCompactUnwindOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_NE(Offsets.Content, 0U);
  EXPECT_EQ(read32le(Bytes, Offsets.Content + 12) & UINT32_C(0x0F000000),
            UINT32_C(0x03000000));
  write32le(Bytes, Offsets.Content + 12, UINT32_C(0x03000001));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, LeavesX86_64MachOEHOnlyUnchanged) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto EHOnly =
      makeObject(llvm::Triple("x86_64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(EHOnly, Target::X86_64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(Graph->compactUnwind().empty());
  EXPECT_TRUE(std::any_of(Graph->sections().begin(), Graph->sections().end(),
                          [](const Section &Value) {
                            return Value.Purpose == SectionPurpose::EHFrame;
                          }));
}

TEST(ObjectReaderTest, AppendsELFEHFrameTerminator) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object = makeX86_64ELFReaderFixture();
  ASSERT_EQ(Object.size(), 848U);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unwind.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  uint64_t InputSize = 0;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = Section.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name == ".eh_frame")
      InputSize = Section.getSize();
  }
  ASSERT_NE(InputSize, 0U);

  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  const auto EH =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Value) {
                     return Value.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph->sections().end());
  EXPECT_EQ(EH->VirtualSize, InputSize + 4);
  ASSERT_EQ(EH->Content.size(), InputSize + 4);
  EXPECT_EQ(
      std::vector<WasmEdge::Byte>(EH->Content.end() - 4, EH->Content.end()),
      std::vector<WasmEdge::Byte>(4));
}

TEST(ObjectReaderTest, TreatsELFPDataNameAsOrdinaryData) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object =
      makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                         ".section .pdata,\"a\",@progbits\n.byte 1, 2, 3, 4\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  const auto PData =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Value) { return Value.Name == ".pdata"; });
  ASSERT_NE(PData, Graph->sections().end());
  EXPECT_EQ(PData->Purpose, SectionPurpose::Default);
  EXPECT_EQ(PData->Kind, SectionKind::ReadOnly);
}

TEST(ObjectReaderTest, ParsesAssembledX86_64MachOCompactUnwind) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  constexpr std::string_view CompactUnwind = R"(
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _f0
.long 1
.long 0x01000000
.quad 0
.quad 0
)";
  const auto Bytes =
      makeObject(llvm::Triple("x86_64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(CompactUnwind));
  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  EXPECT_EQ(Graph->symbols()[Record.Function].Name, "_f0");
  EXPECT_EQ(Record.Length, 1U);
  EXPECT_EQ(Record.Encoding, 0x01000000U);
  EXPECT_FALSE(Record.Personality);
  EXPECT_FALSE(Record.LSDA);
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOCompactUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto MakeCompact = [] {
    return makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0",
                      {}, false, false, "generic", {}, true);
  };
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
  };

  constexpr std::string_view BadSizeAssembly = R"(
.section __LD,__compact_unwind,regular,debug
.space 31
)";
  auto BadSize =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(BadSizeAssembly));
  Reject(std::move(BadSize), "compact unwind size is not a multiple of 32");

  auto MissingFunction = MakeCompact();
  auto Offsets = compactUnwindObjectOffsets(MissingFunction);
  const auto FunctionRelocation =
      compactUnwindRelocationFileOffset(MissingFunction, 0);
  ASSERT_NE(FunctionRelocation, 0U);
  write32le(MissingFunction, FunctionRelocation, 16);
  Reject(std::move(MissingFunction),
         "compact unwind record lacks function relocation");

  auto UnsupportedField = MakeCompact();
  Offsets = compactUnwindObjectOffsets(UnsupportedField);
  const auto UnsupportedRelocation =
      compactUnwindRelocationFileOffset(UnsupportedField, 0);
  ASSERT_NE(UnsupportedRelocation, 0U);
  write32le(UnsupportedField, UnsupportedRelocation, 8);
  Reject(std::move(UnsupportedField),
         "unsupported compact unwind relocation field");

  auto ZeroLength = MakeCompact();
  Offsets = compactUnwindObjectOffsets(ZeroLength);
  write32le(ZeroLength, Offsets.Content + 8, 0);
  Reject(std::move(ZeroLength),
         "compact unwind function length must be non-zero");

  auto BadTarget = MakeCompact();
  Offsets = compactUnwindObjectOffsets(BadTarget);
  write64le(BadTarget, Offsets.Content, UINT64_MAX);
  Reject(std::move(BadTarget), "compact unwind target is outside its section");

  for (const size_t Field : {16U, 24U}) {
    auto UnrelocatedTarget = MakeCompact();
    Offsets = compactUnwindObjectOffsets(UnrelocatedTarget);
    write64le(UnrelocatedTarget, Offsets.Content + Field, 1);
    Reject(std::move(UnrelocatedTarget),
           "compact unwind target lacks relocation");
  }
}

TEST(ObjectReaderTest, RejectsDuplicateAArch64MachOCompactUnwindRelocation) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                          "f0", {}, false, false, "generic", {}, true, false,
                          false, false, false, false, {}, false, true);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_GE(Offsets.Count, 2U);
  const auto SecondFunctionRelocation =
      compactUnwindRelocationFileOffset(Bytes, 32);
  ASSERT_NE(SecondFunctionRelocation, 0U);
  write32le(Bytes, SecondFunctionRelocation, 0);
  auto Result = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_FALSE(Result);
  EXPECT_NE(
      Result.error().Message.find("duplicate compact unwind relocation field"),
      std::string::npos);
}

TEST(RelocationTest, RejectsX86_64MachOPortablePersonalityObject) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  EXPECT_FALSE(
      ObjectReader::read(makeObject(llvm::Triple("x86_64-apple-macosx"), false,
                                    false, "f0", {}, false, false, "generic",
                                    {}, true, false, false, false, false, true),
                         Target::X86_64));
}

TEST(RelocationTest, RejectsAArch64COFFPortablePersonalityObject) {
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("aarch64-pc-windows-msvc"), false, false, "f0",
                 {}, false, false, "generic", {}, true, false, false, false,
                 false, true),
      Target::AArch64));
}

TEST(ObjectReaderTest, RejectsMalformedBytes) {
  const std::vector<WasmEdge::Byte> Bytes{0x01, 0x02, 0x03, 0x04};
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, NormalizesZeroSectionAlignment) {
  EXPECT_EQ(Internal::normalizeSectionAlignment(0), 1U);
  EXPECT_EQ(Internal::normalizeSectionAlignment(16), 16U);
}

TEST(ObjectReaderTest, ParsesCOFFExportDirectives) {
  auto Exports = Internal::parseCOFFExports(
      " /DEFAULTLIB:libcmt /EXPORT:f0 /EXPORT:data,DATA "
      "/EXPORT:ordinal,NONAME /EXPORT:alias=real "
      "/EXPORT:\"wasm.code\",DATA /EXPORT:\"public.name\"=\"real.name\"");
  ASSERT_TRUE(Exports);
  EXPECT_EQ(Exports->at("alias"), "real");
  EXPECT_EQ(Exports->at("data"), "data");
  EXPECT_EQ(Exports->at("f0"), "f0");
  EXPECT_EQ(Exports->at("ordinal"), "ordinal");
  EXPECT_EQ(Exports->at("public.name"), "real.name");
  EXPECT_EQ(Exports->at("wasm.code"), "wasm.code");
}

TEST(ObjectReaderTest, RejectsMalformedCOFFExportDirectives) {
  EXPECT_FALSE(Internal::parseCOFFExports("/EXPORT:"));
  EXPECT_FALSE(Internal::parseCOFFExports("\"/EXPORT:f0"));
}

TEST(ObjectReaderTest, RejectsConcatenatedCOFFObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  auto Concatenated = Object;
  Concatenated.insert(Concatenated.end(), Object.begin(), Object.end());
  auto PaddedConcatenated = Object;
  PaddedConcatenated.insert(PaddedConcatenated.end(), 16, 0);
  PaddedConcatenated.insert(PaddedConcatenated.end(), Object.begin(),
                            Object.end());
  auto Padded = Object;
  Padded.insert(Padded.end(), 16, 0);

  EXPECT_FALSE(ObjectReader::read(Concatenated, Target::X86_64));
  EXPECT_FALSE(ObjectReader::read(PaddedConcatenated, Target::X86_64));
  EXPECT_TRUE(ObjectReader::read(Padded, Target::X86_64));
}

TEST(ObjectReaderTest, FindsUnalignedTrailingCOFFAfterLargeZeroPadding) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  auto Concatenated = Object;
  constexpr size_t PaddingSize = 1024 * 1024;
  Concatenated.reserve(Object.size() * 2 + PaddingSize + 1);
  Concatenated.insert(Concatenated.end(), PaddingSize, 0);
  if (Concatenated.size() % 2 == 0)
    Concatenated.push_back(0);
  ASSERT_EQ(Concatenated.size() % 2, 1U);
  Concatenated.insert(Concatenated.end(), Object.begin(), Object.end());

  EXPECT_FALSE(ObjectReader::read(Concatenated, Target::X86_64));
}

TEST(ObjectReaderTest, FindsEveryRecognizedCOFFSignatureAsTrailingObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  struct Signature {
    std::string_view Name;
    uint16_t Machine;
    unsigned MinimumLLVM;
  };
  constexpr std::array<Signature, 14> Signatures{{
      {"Unknown", 0x0000, 12},
      {"PowerPC", 0x01F0, 12},
      {"Alpha", 0x0183, 12},
      {"Alpha64", 0x0184, 12},
      {"R4000", 0x0166, 12},
      {"M68K", 0x0150, 12},
      {"I386", 0x014C, 12},
      {"ARMNT", 0x01C4, 12},
      {"PARISC", 0x0290, 12},
      {"M68KWindows", 0x0268, 12},
      {"AMD64", 0x8664, 12},
      {"ARM64", 0xAA64, 12},
      {"ARM64EC", 0xA641, 21},
      {"ARM64X", 0xA64E, 21},
  }};
  for (const auto &Signature : Signatures) {
    auto Trailing = Object;
    Trailing[0] = static_cast<WasmEdge::Byte>(Signature.Machine);
    Trailing[1] = static_cast<WasmEdge::Byte>(Signature.Machine >> 8);
    const llvm::StringRef Data(reinterpret_cast<const char *>(Trailing.data()),
                               Trailing.size());
    if (LLVM_VERSION_MAJOR < Signature.MinimumLLVM) {
      EXPECT_NE(llvm::identify_magic(Data), llvm::file_magic::coff_object)
          << Signature.Name;
      continue;
    }
    EXPECT_EQ(llvm::identify_magic(Data), llvm::file_magic::coff_object)
        << Signature.Name;
    auto Parsed = llvm::object::ObjectFile::createObjectFile(
        llvm::MemoryBufferRef(Data, "signature.obj"));
    ASSERT_TRUE(static_cast<bool>(Parsed)) << Signature.Name;

    auto Concatenated = Object;
    Concatenated.reserve(Object.size() + Trailing.size());
    Concatenated.insert(Concatenated.end(), Trailing.begin(), Trailing.end());
    EXPECT_FALSE(ObjectReader::read(Concatenated, Target::X86_64))
        << Signature.Name;
  }
}

TEST(ObjectReaderTest, RejectsConcatenatedELF32ObjectWithFlags) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  const auto Object = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"), ".text\nbx lr\n");
  ASSERT_EQ(Object[llvm::ELF::EI_CLASS], llvm::ELF::ELFCLASS32);
  ASSERT_NE(read32le(Object, 36), 0U);
  auto Concatenated = Object;
  Concatenated.insert(Concatenated.end(), Object.begin(), Object.end());

  EXPECT_FALSE(ObjectReader::read(Concatenated, Target::ARM));
}

TEST(ObjectReaderTest, AcceptsNullSymbolRISCVRelaxRelocation) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto Object =
      makeAssemblyObject(llvm::Triple("riscv64-unknown-linux-gnu"),
                         ".text\n.reloc 0, R_RISCV_RELAX, 0\nret\n");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "relax.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool HasNullRelax = false;
  for (const auto &Section : (*Parsed)->sections())
    for (const auto &Relocation : Section.relocations())
      HasNullRelax |= Relocation.getType() == llvm::ELF::R_RISCV_RELAX &&
                      Relocation.getSymbol() == (*Parsed)->symbol_end();
  ASSERT_TRUE(HasNullRelax);

  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::none_of(Graph->relocations().begin(),
                           Graph->relocations().end(), [](const auto &Value) {
                             return Value.Type == llvm::ELF::R_RISCV_RELAX;
                           }));
}

TEST(ObjectReaderTest,
     IgnoresUnusedCOFFTablePointersWhenFindingTrailingObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "test.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Parsed);
  ASSERT_NE(COFF, nullptr);
  bool Mutated = false;
  for (const auto &Section : COFF->sections()) {
    const auto *Header = COFF->getCOFFSection(Section);
    if (Header->NumberOfRelocations != 0 || Header->NumberOfLinenumbers != 0)
      continue;
    const auto Offset = reinterpret_cast<const WasmEdge::Byte *>(Header) -
                        reinterpret_cast<const WasmEdge::Byte *>(Object.data());
    write32le(Object, static_cast<size_t>(Offset) + 24, UINT32_MAX);
    write32le(Object, static_cast<size_t>(Offset) + 28, UINT32_MAX);
    Mutated = true;
    break;
  }
  ASSERT_TRUE(Mutated);
  auto Concatenated = Object;
  Concatenated.insert(Concatenated.end(), Object.begin(), Object.end());

  EXPECT_FALSE(ObjectReader::read(Concatenated, Target::X86_64));
}

TEST(ObjectReaderTest, IgnoresCOFFVirtualSectionRawSizeForTrailingObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  const auto Trailing = Object;
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "test.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Parsed);
  ASSERT_NE(COFF, nullptr);
  bool Mutated = false;
  for (const auto &Section : COFF->sections()) {
    const auto *Header = COFF->getCOFFSection(Section);
    if (Header->PointerToRawData != 0)
      continue;
    const auto Offset = reinterpret_cast<const WasmEdge::Byte *>(Header) -
                        reinterpret_cast<const WasmEdge::Byte *>(Object.data());
    write32le(Object, static_cast<size_t>(Offset) + 16, UINT32_MAX);
    Mutated = true;
    break;
  }
  ASSERT_TRUE(Mutated);
  EXPECT_TRUE(ObjectReader::read(Object, Target::X86_64));

  Object.insert(Object.end(), Trailing.begin(), Trailing.end());
  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
}

TEST(ObjectReaderTest, ValidatesCOFFRelocationOverflowConvention) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  constexpr uint32_t RelocationCount = UINT16_MAX;
  constexpr size_t RelocationSize = 10;
  auto MakeObject = [] {
    auto Object = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                     ".text\nretq\n");
    auto Parsed =
        llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
            llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                            Object.size()),
            "test.o"));
    EXPECT_TRUE(static_cast<bool>(Parsed));
    if (!Parsed) {
      llvm::consumeError(Parsed.takeError());
      return std::pair{std::move(Object), size_t{0}};
    }
    const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Parsed);
    EXPECT_NE(COFF, nullptr);
    if (COFF == nullptr || COFF->section_begin() == COFF->section_end())
      return std::pair{std::move(Object), size_t{0}};
    const auto *Header = COFF->getCOFFSection(*COFF->section_begin());
    const auto Section = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Header) - Object.data());
    Object.resize(read32le(Object, 8));
    write32le(Object, 12, 0);
    write32le(Object, Section + 36, 0);
    return std::pair{std::move(Object), Section};
  };

  auto [MismatchedFlag, MismatchedSection] = MakeObject();
  ASSERT_NE(MismatchedSection, 0U);
  write32le(MismatchedFlag, MismatchedSection + 36,
            llvm::COFF::IMAGE_SCN_LNK_NRELOC_OVFL);
  EXPECT_FALSE(ObjectReader::read(MismatchedFlag, Target::X86_64));

  auto [SmallExtended, ExtendedSection] = MakeObject();
  ASSERT_NE(ExtendedSection, 0U);
  const uint32_t ExtendedTable = static_cast<uint32_t>(SmallExtended.size());
  SmallExtended.resize(SmallExtended.size() + RelocationCount * RelocationSize);
  write32le(SmallExtended, ExtendedSection + 24, ExtendedTable);
  SmallExtended[ExtendedSection + 32] = 0xFF;
  SmallExtended[ExtendedSection + 33] = 0xFF;
  write32le(SmallExtended, ExtendedSection + 36,
            llvm::COFF::IMAGE_SCN_LNK_NRELOC_OVFL);
  write32le(SmallExtended, ExtendedTable, RelocationCount);
  EXPECT_FALSE(ObjectReader::read(SmallExtended, Target::X86_64));

  auto [Ordinary, OrdinarySection] = MakeObject();
  ASSERT_NE(OrdinarySection, 0U);
  const auto Trailing = Ordinary;
  const uint32_t OrdinaryTable = static_cast<uint32_t>(Ordinary.size());
  Ordinary.resize(Ordinary.size() + RelocationCount * RelocationSize);
  write32le(Ordinary, OrdinarySection + 24, OrdinaryTable);
  Ordinary[OrdinarySection + 32] = 0xFF;
  Ordinary[OrdinarySection + 33] = 0xFF;
  EXPECT_TRUE(ObjectReader::read(Ordinary, Target::X86_64));
  Ordinary.insert(Ordinary.end(), Trailing.begin(), Trailing.end());
  EXPECT_FALSE(ObjectReader::read(Ordinary, Target::X86_64));
}

TEST(ObjectReaderTest,
     IgnoresUnusedCOFFSymbolPointerWhenFindingTrailingObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Object = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                   ".text\nretq\n");
  const auto Trailing = Object;
  Object.resize(read32le(Object, 8));
  write32le(Object, 8, UINT32_MAX);
  write32le(Object, 12, 0);
  EXPECT_TRUE(ObjectReader::read(Object, Target::X86_64));

  Object.insert(Object.end(), Trailing.begin(), Trailing.end());

  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
}

TEST(ObjectReaderTest, IncludesZeroSymbolCOFFStringTableInStructuralExtent) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Object = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                   ".text\nretq\n");
  const auto Embedded = Object;
  const auto SymbolTable = read32le(Object, 8);
  Object.resize(SymbolTable);
  write32le(Object, 12, 0);
  const uint32_t StringTableSize =
      static_cast<uint32_t>(sizeof(uint32_t) + Embedded.size() + 1);
  Object.resize(SymbolTable + StringTableSize);
  write32le(Object, SymbolTable, StringTableSize);
  std::copy(Embedded.begin(), Embedded.end(),
            Object.begin() + SymbolTable + sizeof(uint32_t));
  EXPECT_TRUE(ObjectReader::read(Object, Target::X86_64));

  Object.insert(Object.end(), Embedded.begin(), Embedded.end());
  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsConcatenatedCOFFBigObj) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Normal = makeAssemblyObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                   ".text\nretq\n");
  const uint16_t SectionCount =
      static_cast<uint16_t>(Normal[2] | static_cast<uint16_t>(Normal[3]) << 8);
  const uint32_t SymbolTable = read32le(Normal, 8);
  const uint32_t SymbolCount = read32le(Normal, 12);
  ASSERT_GT(SymbolCount, 0U);
  ASSERT_LE(SymbolCount,
            (Normal.size() - static_cast<size_t>(SymbolTable)) / 18);
  const size_t StringTable =
      SymbolTable + static_cast<size_t>(SymbolCount) * 18;
  constexpr size_t HeaderGrowth = 36;
  constexpr size_t SymbolGrowth = 2;
  const size_t BigSymbolTable = SymbolTable + HeaderGrowth;
  std::vector<WasmEdge::Byte> BigObj(Normal.size() + HeaderGrowth +
                                     SymbolCount * SymbolGrowth);
  using Difference = decltype(Normal)::difference_type;
  ASSERT_LE(Normal.size(),
            static_cast<size_t>(std::numeric_limits<Difference>::max()));
  ASSERT_LE(BigObj.size(),
            static_cast<size_t>(std::numeric_limits<Difference>::max()));
  std::copy(Normal.begin() + 20,
            Normal.begin() + static_cast<Difference>(SymbolTable),
            BigObj.begin() + 56);
  BigObj[2] = 0xFF;
  BigObj[3] = 0xFF;
  BigObj[4] = 2;
  BigObj[6] = Normal[0];
  BigObj[7] = Normal[1];
  std::copy(std::begin(llvm::COFF::BigObjMagic),
            std::end(llvm::COFF::BigObjMagic), BigObj.begin() + 12);
  write32le(BigObj, 44, SectionCount);
  write32le(BigObj, 48, static_cast<uint32_t>(BigSymbolTable));
  write32le(BigObj, 52, SymbolCount);
  for (uint16_t I = 0; I < SectionCount; ++I) {
    const size_t Section = 56 + static_cast<size_t>(I) * 40;
    for (const size_t Pointer : {size_t{20}, size_t{24}, size_t{28}}) {
      const uint32_t Value = read32le(BigObj, Section + Pointer);
      if (Value != 0)
        write32le(BigObj, Section + Pointer, Value + HeaderGrowth);
    }
  }
  for (uint32_t I = 0; I < SymbolCount;) {
    const size_t Input = SymbolTable + static_cast<size_t>(I) * 18;
    const size_t Output = BigSymbolTable + static_cast<size_t>(I) * 20;
    std::copy_n(Normal.begin() + static_cast<Difference>(Input), 12,
                BigObj.begin() + static_cast<Difference>(Output));
    const int16_t Section =
        static_cast<int16_t>(static_cast<uint16_t>(Normal[Input + 12]) |
                             static_cast<uint16_t>(Normal[Input + 13]) << 8);
    write32le(BigObj, Output + 12, static_cast<uint32_t>(Section));
    std::copy_n(Normal.begin() + static_cast<Difference>(Input + 14), 4,
                BigObj.begin() + static_cast<Difference>(Output + 16));
    const uint8_t AuxCount = Normal[Input + 17];
    ASSERT_LE(AuxCount, SymbolCount - I - 1);
    for (uint8_t J = 0; J < AuxCount; ++J) {
      std::copy_n(
          Normal.begin() + static_cast<Difference>(Input + (J + 1) * 18), 18,
          BigObj.begin() + static_cast<Difference>(Output + (J + 1) * 20));
    }
    I += 1 + AuxCount;
  }
  std::copy(Normal.begin() + static_cast<Difference>(StringTable), Normal.end(),
            BigObj.begin() +
                static_cast<Difference>(BigSymbolTable +
                                        static_cast<size_t>(SymbolCount) * 20));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(BigObj.data()),
                          BigObj.size()),
          "bigobj.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Parsed);
  ASSERT_NE(COFF, nullptr);
  ASSERT_NE(COFF->getCOFFBigObjHeader(), nullptr);
  EXPECT_EQ(COFF->getNumberOfSymbols(), SymbolCount);
  ASSERT_TRUE(ObjectReader::read(BigObj, Target::X86_64));
  const auto Trailing = BigObj;
  BigObj.insert(BigObj.end(), Trailing.begin(), Trailing.end());

  EXPECT_FALSE(ObjectReader::read(BigObj, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsMalformedCOFFStructuralMetadata) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Object = makeObject(llvm::Triple("x86_64-pc-windows-msvc"));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "test.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *COFF = llvm::dyn_cast<llvm::object::COFFObjectFile>(&**Parsed);
  ASSERT_NE(COFF, nullptr);
  const auto Section = COFF->section_begin();
  ASSERT_NE(Section, COFF->section_end());
  const auto *Header = COFF->getCOFFSection(*Section);
  const auto Offset = reinterpret_cast<const WasmEdge::Byte *>(Header) -
                      reinterpret_cast<const WasmEdge::Byte *>(Object.data());
  write32le(Object, static_cast<size_t>(Offset) + 24, UINT32_MAX);
  Object[static_cast<size_t>(Offset) + 32] = 1;
  Object[static_cast<size_t>(Offset) + 33] = 0;

  EXPECT_FALSE(ObjectReader::read(Object, Target::X86_64));
}

TEST(ObjectReaderTest, NormalizesX86_64ELFRelaObject) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  auto Result = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->target(), Target::X86_64);
  EXPECT_EQ(Result->endianness(), Endianness::Little);
  EXPECT_FALSE(Result->sections().empty());
  const auto Text =
      std::find_if(Result->sections().begin(), Result->sections().end(),
                   [](const Section &Value) { return Value.Name == ".text"; });
  ASSERT_NE(Text, Result->sections().end());
  EXPECT_EQ(Text->Kind, SectionKind::Text);
  EXPECT_EQ(Text->VirtualSize, Text->Content.size());
  EXPECT_GT(Text->Alignment, 0U);
  const auto Bss = std::find_if(
      Result->sections().begin(), Result->sections().end(),
      [](const Section &Value) { return Value.Kind == SectionKind::BSS; });
  ASSERT_NE(Bss, Result->sections().end());
  EXPECT_TRUE(Bss->Content.empty());
  EXPECT_GE(Bss->VirtualSize, 4U);
  EXPECT_GE(Bss->Alignment, 16U);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  const auto TextId = static_cast<SectionId>(Text - Result->sections().begin());
  const auto F0Id = static_cast<SymbolId>(F0 - Result->symbols().begin());
  EXPECT_EQ(F0->Section, TextId);
  EXPECT_EQ(F0->Offset, 0U);
  EXPECT_TRUE(F0->Exported);
  EXPECT_GT(F0->Size, 0U);
  ASSERT_FALSE(Result->relocations().empty());
  const auto &Relocation = Result->relocations()[0];
  EXPECT_EQ(Relocation.Section, TextId);
  EXPECT_LT(Relocation.Offset, Text->Content.size());
  EXPECT_NE(Relocation.Type, 0U);
  EXPECT_LT(Relocation.Symbol, Result->symbols().size());
  EXPECT_EQ(Relocation.Offset, 3U);
  EXPECT_EQ(Relocation.Type, 42U);
  EXPECT_EQ(Relocation.Format, ObjectFormat::ELF);
  EXPECT_EQ(Relocation.PatchSize, 4U);
  EXPECT_TRUE(Relocation.PCRelative);
  EXPECT_EQ(Result->symbols()[Relocation.Symbol].Name, "value");
  EXPECT_EQ(Relocation.Addend, -4);
  EXPECT_FALSE(Relocation.AddendIsImplicit);
  EXPECT_NE(Relocation.Symbol, F0Id);
}

TEST(ObjectReaderTest, ReadsCOFFExportsFromDirectives) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, true),
      Target::X86_64);
  ASSERT_TRUE(Result);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  EXPECT_TRUE(F0->Exported);
}

TEST(ObjectReaderTest, NormalizesRenamedCOFFExport) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Result =
      ObjectReader::read(makeObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                    false, false, "real", "/EXPORT:alias=real"),
                         Target::X86_64);
  ASSERT_TRUE(Result);
  const auto Real =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "real"; });
  ASSERT_NE(Real, Result->symbols().end());
  EXPECT_TRUE(Real->Exported);
  ASSERT_TRUE(Real->ExportName);
  EXPECT_EQ(*Real->ExportName, "alias");
}

TEST(ObjectReaderTest, ExportsMachOExternalDefinedSymbols) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-apple-macosx")), Target::X86_64);
  ASSERT_TRUE(Result);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "_f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  EXPECT_TRUE(F0->Exported);
  ASSERT_EQ(Result->relocations().size(), 1U);
  EXPECT_TRUE(Result->relocations()[0].PCRelative);
  EXPECT_EQ(Result->relocations()[0].PatchSize, 4U);
  EXPECT_TRUE(Result->relocations()[0].External);
  EXPECT_FALSE(Result->relocations()[0].Scattered);
}

TEST(ObjectReaderTest, RejectsArm64eMachOSubtypes) {
  constexpr size_t MachOCPUSubtypeOffset = 8;
  constexpr uint32_t CPU_SUBTYPE_ARM64_ALL = 0;
  constexpr uint32_t CPU_SUBTYPE_ARM64E = 2;
  constexpr uint32_t CPU_SUBTYPE_ARM64E_PTRAUTH_VERSION_1 = 0x81000002;
  constexpr uint32_t CPU_SUBTYPE_LIB64 = 0x80000000;
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Generic = makeObject(llvm::Triple("arm64-apple-macosx"));
  ASSERT_GE(Generic.size(), MachOCPUSubtypeOffset + sizeof(uint32_t));
  ASSERT_EQ(read32le(Generic, MachOCPUSubtypeOffset), CPU_SUBTYPE_ARM64_ALL);
  EXPECT_TRUE(ObjectReader::read(Generic, Target::AArch64));
  auto GenericWithCapabilities = Generic;
  ASSERT_TRUE(Internal::writeUnsigned(
      GenericWithCapabilities, MachOCPUSubtypeOffset, 4, Endianness::Little,
      CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_ARM64_ALL));
  EXPECT_TRUE(ObjectReader::read(GenericWithCapabilities, Target::AArch64));
  for (const uint32_t Subtype :
       {CPU_SUBTYPE_ARM64E, CPU_SUBTYPE_ARM64E_PTRAUTH_VERSION_1,
        CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_ARM64E}) {
    auto Arm64e = Generic;
    ASSERT_TRUE(Internal::writeUnsigned(Arm64e, MachOCPUSubtypeOffset, 4,
                                        Endianness::Little, Subtype));
    auto Result = ObjectReader::read(Arm64e, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find("including arm64e"),
              std::string::npos);
  }
}

TEST(ObjectReaderTest, AcceptsX86_64MachOSubtypeControl) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto X86 = makeObject(llvm::Triple("x86_64-apple-macosx"));
  EXPECT_TRUE(ObjectReader::read(X86, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsMalformedX86_64MachOSignedMetadata) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Bytes = makeObject(llvm::Triple("x86_64-apple-macosx"));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "test.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() != Section.relocation_end()) {
      const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
      llvm::object::DataRefImpl SectionRef;
      SectionRef.d.a = Ref.d.a;
      RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
      break;
    }
  }
  ASSERT_NE(RelocationOffset, 0U);
  for (const uint32_t Mask : {UINT32_C(1) << 24, UINT32_C(1) << 25}) {
    auto Malformed = Bytes;
    uint32_t Word = 0;
    std::memcpy(&Word, Malformed.data() + RelocationOffset + 4, sizeof(Word));
    Word ^= Mask;
    std::memcpy(Malformed.data() + RelocationOffset + 4, &Word, sizeof(Word));
    EXPECT_FALSE(ObjectReader::read(Malformed, Target::X86_64));
  }
  auto Scattered = Bytes;
  uint32_t AddressWord = 0;
  std::memcpy(&AddressWord, Scattered.data() + RelocationOffset,
              sizeof(AddressWord));
  AddressWord |= UINT32_C(1) << 31;
  std::memcpy(Scattered.data() + RelocationOffset, &AddressWord,
              sizeof(AddressWord));
  EXPECT_FALSE(ObjectReader::read(Scattered, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsScatteredAArch64MachORelocation) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  EXPECT_FALSE(
      Internal::supportsMachORelocationMetadata(Target::AArch64, true));
  auto Bytes = makeObject(llvm::Triple("arm64-apple-macosx"));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm64.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
    llvm::object::DataRefImpl SectionRef;
    SectionRef.d.a = Ref.d.a;
    RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
    break;
  }
  ASSERT_NE(RelocationOffset, 0U);
  uint32_t Address = 0;
  std::memcpy(&Address, Bytes.data() + RelocationOffset, sizeof(Address));
  Address |= UINT32_C(1) << 31;
  std::memcpy(Bytes.data() + RelocationOffset, &Address, sizeof(Address));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, DoesNotExportHiddenMachOSymbols) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Result =
      ObjectReader::read(makeObject(llvm::Triple("x86_64-apple-macosx"), false,
                                    false, "hidden", {}, true),
                         Target::X86_64);
  ASSERT_TRUE(Result);
  const auto Hidden =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "_hidden"; });
  ASSERT_NE(Hidden, Result->symbols().end());
  EXPECT_FALSE(Hidden->Exported);
}

TEST(ObjectReaderTest, RejectsELFRelocationWithInvalidSectionLink) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  Bytes[Header + 40] = 0xFF;
  Bytes[Header + 41] = 0xFF;
  Bytes[Header + 42] = 0xFF;
  Bytes[Header + 43] = 0x7F;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsELFRelocationWithZeroEntrySize) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 56, 0);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsTruncatedELFRelocationTable) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 32, read64le(Bytes, Header + 32) - 1);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsELFRelocationWithInvalidSymbolIndex) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  write64le(Bytes, RelocationOffset + 8, UINT64_C(0xFFFFFFFF00000000));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, PreservesGraphRelocationDiagnosticFields) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  write64le(Bytes, RelocationOffset, UINT64_MAX);

  auto Result = ObjectReader::read(Bytes, Target::X86_64);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "relocation offset is outside section content");
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_TRUE(Result.error().RelocationType);
  EXPECT_EQ(Result.error().Offset, UINT64_MAX);
}

TEST(ObjectReaderTest, ClassifiesUnsupportedELFRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  const auto Info = read64le(Bytes, RelocationOffset + 8);
  write64le(Bytes, RelocationOffset + 8,
            (Info & UINT64_C(0xFFFFFFFF00000000)) | UINT32_MAX);

  auto Result = ObjectReader::read(Bytes, Target::X86_64);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "unsupported relocation patch size");
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_EQ(Result.error().RelocationType, UINT32_MAX);
}

#if LLVM_VERSION_MAJOR >= 19
TEST(ObjectReaderTest, NormalizesELFCrelRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Result = ObjectReader::read(makeX86_64CrelObject(), Target::X86_64);
  ASSERT_TRUE(Result);
  ASSERT_EQ(Result->relocations().size(), 1U);
  EXPECT_EQ(Result->relocations()[0].Offset, 3U);
  EXPECT_EQ(Result->relocations()[0].Type, 42U);
  EXPECT_EQ(Result->relocations()[0].Addend, -4);
  EXPECT_FALSE(Result->relocations()[0].AddendIsImplicit);
}

TEST(ObjectReaderTest, RejectsTruncatedELFCrelRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeX86_64CrelObject();
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 32, 1);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}
#endif

TEST(ObjectReaderTest, MarksELFRelAddendsImplicit) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("armv7-unknown-linux-gnueabihf"), false, false,
                 "f0", {}, false, true),
      Target::ARM);
  ASSERT_TRUE(Result);
  ASSERT_FALSE(Result->relocations().empty());
  EXPECT_TRUE(Result->relocations()[0].AddendIsImplicit);
  EXPECT_EQ(Result->relocations()[0].Addend, 0);
}

TEST(ObjectReaderTest, PreservesARMHardFloatMetadata) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeAssemblyObject(
      llvm::Triple("armv7-unknown-linux-gnueabihf"),
      ".syntax unified\n.text\n.globl f0\nf0:\n bx lr\n", "", true);
  ASSERT_GE(Bytes.size(), 40U);
  const uint32_t HardFloatFlags =
      llvm::ELF::EF_ARM_EABI_VER5 | llvm::ELF::EF_ARM_ABI_FLOAT_HARD;
  std::memcpy(Bytes.data() + 36, &HardFloatFlags, sizeof(HardFloatFlags));
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_EABIMASK,
            llvm::ELF::EF_ARM_EABI_VER5);
  EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_ABI_FLOAT_HARD,
            llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
}

TEST(ObjectReaderTest, PreservesRISCVArchitectureFlags) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto Bytes = makeAssemblyObject(
      llvm::Triple("riscv64-unknown-linux-gnu"),
      ".option rvc\n.text\n.globl f0\nf0:\n c.nop\n ret\n", "+c,+f,+d");
  auto Graph = ObjectReader::read(Bytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  const uint32_t Expected =
      llvm::ELF::EF_RISCV_RVC | llvm::ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
  EXPECT_EQ(Graph->elfFlags(), Expected);
}

TEST(ObjectReaderTest, WritesRISCVArchitectureFlags) {
  REQUIRE_RELOCATION_HANDLER(Target::RISCV64);
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto Bytes = makeAssemblyObject(
      llvm::Triple("riscv64-unknown-linux-gnu"),
      ".option rvc\n.text\n.globl f0\nf0:\n c.nop\n ret\n", "+c,+f,+d");
  auto Graph = ObjectReader::read(Bytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  const uint32_t Expected =
      llvm::ELF::EF_RISCV_RVC | llvm::ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
  ASSERT_TRUE(ELFWriter::layout(*Graph));
  ASSERT_TRUE(applyRelocations(*Graph));
  std::vector<WasmEdge::Byte> OutputBytes;
  Writer Output(OutputBytes);
  ASSERT_TRUE(ELFWriter::write(*Graph, Output));
  EXPECT_EQ(read64le(OutputBytes, 48) & UINT32_MAX, Expected);
}

TEST(ObjectReaderTest, RejectsNonRelocatableELFObject) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  ASSERT_GT(Bytes.size(), 18U);
  Bytes[16] = 3;
  Bytes[17] = 0;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
  Bytes[16] = 2;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUnsupportedObjectFormat) {
  REQUIRE_LLVM_TARGET("wasm32-unknown-unknown");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("wasm32-unknown-unknown")), Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUnsupportedArchitecture) {
  REQUIRE_LLVM_TARGET("i386-unknown-linux-gnu");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("i386-unknown-linux-gnu")), Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUndefinedExternal) {
  EXPECT_FALSE(ObjectReader::read(makeNativeObject(true), nativeTarget()));
}

TEST(ObjectReaderTest, RejectsTargetMismatch) {
  const Target Other =
      nativeTarget() == Target::X86_64 ? Target::AArch64 : Target::X86_64;
  EXPECT_FALSE(ObjectReader::read(makeNativeObject(), Other));
}

TEST(ObjectReaderTest, RejectsEmptyAndArchiveBuffers) {
  EXPECT_FALSE(ObjectReader::read({}, nativeTarget()));
  const std::string Archive = "!<arch>\n";
  EXPECT_FALSE(ObjectReader::read(
      WasmEdge::Span<const WasmEdge::Byte>(
          reinterpret_cast<const WasmEdge::Byte *>(Archive.data()),
          Archive.size()),
      nativeTarget()));
}

} // namespace
