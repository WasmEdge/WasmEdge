// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "llvm/compiler.h"

#include "compiler/context.h"
#include "compiler/function_compiler.h"

#include "aot/version.h"
#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"
#include "system/allocator.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <system_error>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

struct RAIICleanup {
  RAIICleanup(LLVM::Compiler::CompileContext *&ContextRef,
              LLVM::Compiler::CompileContext *NewContext)
      : Context(ContextRef) {
    Context = NewContext;
  }
  ~RAIICleanup() { Context = nullptr; }
  LLVM::Compiler::CompileContext *&Context;
};

// Translate Compiler::OptimizationLevel to llvm::PassBuilder version
#if LLVM_VERSION_MAJOR >= 13
static inline const char *
toLLVMLevel(WasmEdge::CompilerConfigure::OptimizationLevel Level) noexcept {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return "default<O0>,function(tailcallelim)";
  case OL::O1:
    return "default<O1>,function(tailcallelim)";
  case OL::O2:
    return "default<O2>";
  case OL::O3:
    return "default<O3>";
  case OL::Os:
    return "default<Os>";
  case OL::Oz:
    return "default<Oz>";
  default:
    assumingUnreachable();
  }
}
#else
static inline std::pair<unsigned int, unsigned int>
toLLVMLevel(WasmEdge::CompilerConfigure::OptimizationLevel Level) noexcept {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return {0, 0};
  case OL::O1:
    return {1, 0};
  case OL::O2:
    return {2, 0};
  case OL::O3:
    return {3, 0};
  case OL::Os:
    return {2, 1};
  case OL::Oz:
    return {2, 2};
  default:
    assumingUnreachable();
  }
}
#endif

static inline LLVMCodeGenOptLevel toLLVMCodeGenLevel(
    WasmEdge::CompilerConfigure::OptimizationLevel Level) noexcept {
  using OL = WasmEdge::CompilerConfigure::OptimizationLevel;
  switch (Level) {
  case OL::O0:
    return LLVMCodeGenLevelNone;
  case OL::O1:
    return LLVMCodeGenLevelLess;
  case OL::O2:
    return LLVMCodeGenLevelDefault;
  case OL::O3:
    return LLVMCodeGenLevelAggressive;
  case OL::Os:
    return LLVMCodeGenLevelDefault;
  case OL::Oz:
    return LLVMCodeGenLevelDefault;
  default:
    assumingUnreachable();
  }
}
} // namespace

namespace WasmEdge {
namespace LLVM {

Expect<void> Compiler::checkConfigure() noexcept {
  // Note: Although the Exception Handling and Memory64 proposals are not
  // implemented in AOT yet, we should not trap here because the default
  // configuration has become WASM 3.0, which contains these proposals.
  if (Conf.hasProposal(Proposal::ExceptionHandling)) {
    spdlog::warn("Proposal Exception Handling is not yet supported in WasmEdge "
                 "AOT/JIT. The compilation will be trapped when related data "
                 "structure or instructions found in WASM."sv);
  }
  if (Conf.hasProposal(Proposal::Annotations)) {
    spdlog::error(ErrCode::Value::InvalidAOTConfigure);
    spdlog::error("    Proposal Custom Annotation Syntax is not yet supported "
                  "in WasmEdge AOT/JIT."sv);
    return Unexpect(ErrCode::Value::InvalidAOTConfigure);
  }
  return {};
}

Expect<void> Compiler::optimize(LLVM::Module &LLModule,
                                LLVM::TargetMachine &TM) noexcept {
  spdlog::info("optimize start"sv);
  auto Triple = LLModule.getTarget();
  auto [TheTarget, ErrorMessage] = LLVM::Target::getFromTriple(Triple);
  if (ErrorMessage) {
    spdlog::error("getFromTriple failed:{}"sv, ErrorMessage.string_view());
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  std::string CPUName;
#if defined(__riscv) && __riscv_xlen == 64
  CPUName = "generic-rv64"s;
#else
  if (!Conf.getCompilerConfigure().isGenericBinary()) {
    CPUName = LLVM::getHostCPUName().string_view();
  } else {
    CPUName = "generic"s;
  }
#endif

  // On RISC-V we use generic-rv64 as the CPU, so also use default
  // features; host features under QEMU can be inconsistent (e.g.
  // zvl*b without v) which LLVM >= 20 rejects.
  TM = LLVM::TargetMachine::create(
      TheTarget, Triple, CPUName.c_str(),
#if defined(__riscv) && __riscv_xlen == 64
      "",
#else
      LLVM::getHostCPUFeatures().unwrap(),
#endif
      toLLVMCodeGenLevel(Conf.getCompilerConfigure().getOptimizationLevel()),
      LLVMRelocPIC, LLVMCodeModelDefault);

#if LLVM_VERSION_MAJOR >= 13
  auto PBO = LLVM::PassBuilderOptions::create();
  if (auto Error = PBO.runPasses(
          LLModule,
          toLLVMLevel(Conf.getCompilerConfigure().getOptimizationLevel()),
          TM)) {
    spdlog::error("{}"sv, Error.message().string_view());
  }
#else
  auto FP = LLVM::PassManager::createForModule(LLModule);
  auto MP = LLVM::PassManager::create();

  TM.addAnalysisPasses(MP);
  TM.addAnalysisPasses(FP);
  {
    auto PMB = LLVM::PassManagerBuilder::create();
    auto [OptLevel, SizeLevel] =
        toLLVMLevel(Conf.getCompilerConfigure().getOptimizationLevel());
    PMB.setOptLevel(OptLevel);
    PMB.setSizeLevel(SizeLevel);
    PMB.populateFunctionPassManager(FP);
    PMB.populateModulePassManager(MP);
  }
  switch (Conf.getCompilerConfigure().getOptimizationLevel()) {
  case CompilerConfigure::OptimizationLevel::O0:
  case CompilerConfigure::OptimizationLevel::O1:
    FP.addTailCallEliminationPass();
    break;
  default:
    break;
  }

  FP.initializeFunctionPassManager();
  for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
    FP.runFunctionPassManager(Fn);
  }
  FP.finalizeFunctionPassManager();
  MP.runPassManager(LLModule);
#endif

  spdlog::info("optimize done"sv);
  return {};
}

// Initialize the LLVM module held by the data for compilation: set the
// target triple and the PIC level, and return the LLVM context.
static LLVM::Context initLLVMModule(LLVM::Data &D) noexcept {
  auto LLContext = D.extract().getLLContext();
  LLVM::Core::init(LLContext.unwrap());
  auto &LLModule = D.extract().LLModule;
  LLModule.setTarget(LLVM::getDefaultTargetTriple().unwrap());
  LLModule.addFlag(LLVMModuleFlagBehaviorError, "PIC Level"sv, 2);
  return LLContext;
}

Expect<Data> Compiler::compile(const AST::Module &Module) noexcept {
  // Check that the module is validated.
  if (unlikely(!Module.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    return Unexpect(ErrCode::Value::NotValidated);
  }

  std::unique_lock Lock(Mutex);
  spdlog::info("compile start"sv);

  LLVM::Data D;
  auto LLContext = initLLVMModule(D);
  auto &LLModule = D.extract().LLModule;

  CompileContext NewContext(LLContext, LLModule,
                            Conf.getCompilerConfigure().isGenericBinary());
  RAIICleanup Cleanup(Context, &NewContext);
  Context->addVersionGlobal();

  // Compile all sections and the function declarations.
  compileSections(Module, false);
  // Compile all function bodies.
  const auto DefinedCount = Module.getDefinedFuncCount();
  for (uint32_t I = 0; I < DefinedCount; ++I) {
    EXPECTED_TRY(compileFunctionBody(I));
  }
  // Compile ExportSection.
  compile(Module.getExportSection());
  // StartSection is not required for compilation.

  spdlog::info("verify start"sv);
  LLModule.verify(LLVMPrintMessageAction);

  auto &TM = D.extract().TM;
  EXPECTED_TRY(optimize(LLModule, TM));

  // Set initializer for constant value
  Context->finalizeIntrinsicsTable();
  return Expect<Data>{std::move(D)};
}

void Compiler::compile(const AST::TypeSection &TypeSec,
                       bool DeclarationsOnly) noexcept {
  auto WrapperTy =
      LLVM::Type::getFunctionType(Context->VoidTy,
                                  {Context->ExecCtxPtrTy, Context->Int8PtrTy,
                                   Context->Int8PtrTy, Context->Int8PtrTy},
                                  false);
  auto SubTypes = TypeSec.getContent();
  const auto Size = SubTypes.size();
  if (Size == 0) {
    return;
  }
  Context->CompositeTypes.reserve(Size);
  Context->FunctionWrappers.reserve(Size);

  auto SetFuncAttributes = [&](auto FDecl) {
    FDecl.setVisibility(LLVMProtectedVisibility);
    FDecl.setDSOLocal(true);
    FDecl.setDLLStorageClass(LLVMDLLExportStorageClass);
    FDecl.addFnAttr(Context->NoStackArgProbe);
    FDecl.addFnAttr(Context->StrictFP);
    FDecl.addFnAttr(Context->UWTable);
    FDecl.addParamAttr(0, Context->ReadOnly);
    FDecl.addParamAttr(0, Context->NoAlias);
    FDecl.addParamAttr(1, Context->NoAlias);
    FDecl.addParamAttr(2, Context->NoAlias);
    FDecl.addParamAttr(3, Context->NoAlias);
  };

  // Iterate and compile types.
  for (size_t I = 0; I < Size; ++I) {
    const auto &CompType = SubTypes[I].getCompositeType();
    const auto Name = fmt::format("t{}"sv, Context->CompositeTypes.size());
    if (CompType.isFunc()) {
      // Check that the function type is unique.
      {
        bool Unique = true;
        for (size_t J = 0; J < I; ++J) {
          if (Context->CompositeTypes[J] &&
              Context->CompositeTypes[J]->isFunc()) {
            const auto &OldFuncType = Context->CompositeTypes[J]->getFuncType();
            if (OldFuncType == CompType.getFuncType()) {
              Unique = false;
              Context->CompositeTypes.push_back(Context->CompositeTypes[J]);
              if (DeclarationsOnly) {
                auto FDecl = Context->LLModule.get().addFunction(
                    WrapperTy, LLVMExternalLinkage, Name.c_str());
                SetFuncAttributes(FDecl);
                Context->FunctionWrappers.push_back(FDecl);
              } else {
                auto F = Context->FunctionWrappers[J];
                Context->FunctionWrappers.push_back(F);
                auto A = Context->LLModule.get().addAlias(WrapperTy, F,
                                                          Name.c_str());
                A.setLinkage(LLVMExternalLinkage);
                A.setVisibility(LLVMProtectedVisibility);
                A.setDSOLocal(true);
                A.setDLLStorageClass(LLVMDLLExportStorageClass);
              }
              break;
            }
          }
        }
        if (!Unique) {
          continue;
        }
      }

      // Create Wrapper
      auto F = Context->LLModule.get().addFunction(
          WrapperTy, LLVMExternalLinkage, Name.c_str());
      {
        SetFuncAttributes(F);

        if (!DeclarationsOnly) {
          LLVM::Builder Builder(Context->LLContext);
          Builder.positionAtEnd(
              LLVM::BasicBlock::create(Context->LLContext, F, "entry"));

          auto FTy = toLLVMType(Context->LLContext, Context->ExecCtxPtrTy,
                                CompType.getFuncType());
          auto RTy = FTy.getReturnType();
          std::vector<LLVM::Type> FPTy(FTy.getNumParams());
          FTy.getParamTypes(FPTy);

          const size_t ArgCount = FPTy.size() - 1;
          auto ExecCtxPtr = F.getFirstParam();
          auto RawFunc = LLVM::FunctionCallee{
              FTy, Builder.createBitCast(ExecCtxPtr.getNextParam(),
                                         FTy.getPointerTo())};
          auto RawArgs = ExecCtxPtr.getNextParam().getNextParam();
          auto RawRets = RawArgs.getNextParam();

          std::vector<LLVM::Value> Args;
          Args.reserve(FTy.getNumParams());
          Args.push_back(ExecCtxPtr);
          for (size_t J = 0; J < ArgCount; ++J) {
            Args.push_back(Builder.createValuePtrLoad(
                FPTy[J + 1], RawArgs, Context->Int8Ty, J * LLVM::kValSize));
          }

          auto Ret = Builder.createCall(RawFunc, Args);
          if (RTy.isVoidTy()) {
            // nothing to do
          } else if (RTy.isStructTy()) {
            auto Rets = unpackStruct(Builder, Ret);
            Builder.createArrayPtrStore(Rets, RawRets, Context->Int8Ty,
                                        LLVM::kValSize);
          } else {
            Builder.createValuePtrStore(Ret, RawRets, Context->Int8Ty);
          }
          Builder.createRetVoid();
        }
      }
      // Copy wrapper, param and return lists to module instance.
      Context->FunctionWrappers.push_back(F);
    } else {
      // Non function type case. Create empty wrapper.
      auto F = Context->LLModule.get().addFunction(
          WrapperTy, LLVMExternalLinkage, Name.c_str());
      {
        SetFuncAttributes(F);

        if (!DeclarationsOnly) {
          LLVM::Builder Builder(Context->LLContext);
          Builder.positionAtEnd(
              LLVM::BasicBlock::create(Context->LLContext, F, "entry"));
          Builder.createRetVoid();
        }
      }
      Context->FunctionWrappers.push_back(F);
    }
    Context->CompositeTypes.push_back(&CompType);
  }
}

void Compiler::compile(const AST::ImportSection &ImportSec) noexcept {
  // Iterate and compile import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    // Get data from import description.
    const auto &ExtType = ImpDesc.getExternalType();

    // Add the imports to the module instance.
    switch (ExtType) {
    case ExternalType::Function: // Function type index
    {
      const auto FuncID = static_cast<uint32_t>(Context->Functions.size());
      // Get the function type index in module.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      assuming(TypeIdx < Context->CompositeTypes.size());
      assuming(Context->CompositeTypes[TypeIdx]->isFunc());
      const auto &FuncType = Context->CompositeTypes[TypeIdx]->getFuncType();
      auto FTy =
          toLLVMType(Context->LLContext, Context->ExecCtxPtrTy, FuncType);
      auto RTy = FTy.getReturnType();
      auto F =
          LLVM::FunctionCallee{FTy, Context->LLModule.get().addFunction(
                                        FTy, LLVMInternalLinkage,
                                        fmt::format("f{}"sv, FuncID).c_str())};
      F.Fn.setDSOLocal(true);
      F.Fn.addFnAttr(Context->NoStackArgProbe);
      F.Fn.addFnAttr(Context->StrictFP);
      F.Fn.addFnAttr(Context->UWTable);
      F.Fn.addParamAttr(0, Context->ReadOnly);
      F.Fn.addParamAttr(0, Context->NoAlias);

      LLVM::Builder Builder(Context->LLContext);
      Builder.positionAtEnd(
          LLVM::BasicBlock::create(Context->LLContext, F.Fn, "entry"));

      const auto ArgSize = FuncType.getParamTypes().size();
      const auto RetSize =
          RTy.isVoidTy() ? 0 : FuncType.getReturnTypes().size();

      LLVM::Value Args = Builder.createArray(ArgSize, LLVM::kValSize);
      LLVM::Value Rets = Builder.createArray(RetSize, LLVM::kValSize);

      auto Arg = F.Fn.getFirstParam();
      for (unsigned I = 0; I < ArgSize; ++I) {
        Arg = Arg.getNextParam();
        Builder.createValuePtrStore(Arg, Args, Context->Int8Ty,
                                    I * LLVM::kValSize);
      }

      Builder.createCall(
          Context->getIntrinsic(
              Builder, Executable::Intrinsics::kCall,
              LLVM::Type::getFunctionType(
                  Context->VoidTy,
                  {Context->Int32Ty, Context->Int8PtrTy, Context->Int8PtrTy},
                  false)),
          {Context->LLContext.getInt32(FuncID), Args, Rets});

      if (RetSize == 0) {
        Builder.createRetVoid();
      } else if (RetSize == 1) {
        Builder.createRet(
            Builder.createValuePtrLoad(RTy, Rets, Context->Int8Ty));
      } else {
        Builder.createAggregateRet(Builder.createArrayPtrLoad(
            RetSize, RTy, Rets, Context->Int8Ty, LLVM::kValSize));
      }

      Context->Functions.emplace_back(TypeIdx, F, nullptr);
      Context->ImportCount++;
      break;
    }
    case ExternalType::Table: // Table type
    {
      // Get table address type. External type checked in validation.
      const auto &TabType = ImpDesc.getExternalTableType();
      const auto AddrType = TabType.getLimit().getAddrType();
      auto Type = toLLVMType(Context->LLContext, AddrType);
      Context->TableAddrTypes.push_back(Type);
      break;
    }
    case ExternalType::Memory: // Memory type
    {
      // Get memory address type. External type checked in validation.
      const auto &MemType = ImpDesc.getExternalMemoryType();
      const auto AddrType = MemType.getLimit().getAddrType();
      auto Type = toLLVMType(Context->LLContext, AddrType);
      Context->MemoryAddrTypes.push_back(Type);
      break;
    }
    case ExternalType::Global: // Global type
    {
      // Get global type. External type checked in validation.
      const auto &GlobType = ImpDesc.getExternalGlobalType();
      const auto &ValType = GlobType.getValType();
      auto Type = toLLVMType(Context->LLContext, ValType);
      Context->Globals.push_back(Type);
      break;
    }
    case ExternalType::Tag: // Tag type
    {
      // TODO: EXCEPTION - implement the AOT.
      break;
    }
    default:
      assumingUnreachable();
    }
  }
}

void Compiler::compile(const AST::ExportSection &) noexcept {}

void Compiler::compile(const AST::GlobalSection &GlobalSec) noexcept {
  for (const auto &GlobalSeg : GlobalSec.getContent()) {
    const auto &ValType = GlobalSeg.getGlobalType().getValType();
    auto Type = toLLVMType(Context->LLContext, ValType);
    Context->Globals.push_back(Type);
  }
}

void Compiler::compile(const AST::MemorySection &MemorySec,
                       const AST::DataSection &) noexcept {
  for (const auto &MemType : MemorySec.getContent()) {
    const auto AddrType = MemType.getLimit().getAddrType();
    auto Type = toLLVMType(Context->LLContext, AddrType);
    Context->MemoryAddrTypes.push_back(Type);
  }
}

void Compiler::compile(const AST::TableSection &TableSec,
                       const AST::ElementSection &) noexcept {
  for (const auto &TableSeg : TableSec.getContent()) {
    const auto AddrType = TableSeg.getTableType().getLimit().getAddrType();
    auto Type = toLLVMType(Context->LLContext, AddrType);
    Context->TableAddrTypes.push_back(Type);
  }
}

void Compiler::compileSections(const AST::Module &Module,
                               bool DeclarationsOnly) noexcept {
  // Compile Function Types
  compile(Module.getTypeSection(), DeclarationsOnly);
  // Compile ImportSection
  compile(Module.getImportSection());
  // Compile GlobalSection
  compile(Module.getGlobalSection());
  // Compile MemorySection (MemorySec, DataSec)
  compile(Module.getMemorySection(), Module.getDataSection());
  // Compile TableSection (TableSec, ElemSec)
  compile(Module.getTableSection(), Module.getElementSection());
  // Create function declarations without compiling bodies. (FunctionSec,
  // CodeSec)
  compileFunctionDeclarations(Module.getFunctionSection(),
                              Module.getCodeSection());
}

void Compiler::compileFunctionDeclarations(
    const AST::FunctionSection &FunctionSec,
    const AST::CodeSection &CodeSec) noexcept {
  const auto &TypeIdxs = FunctionSec.getContent();
  const auto &CodeSegs = CodeSec.getContent();
  assuming(TypeIdxs.size() == CodeSegs.size());

  for (size_t I = 0; I < CodeSegs.size(); ++I) {
    const auto &TypeIdx = TypeIdxs[I];
    const auto &Code = CodeSegs[I];
    assuming(TypeIdx < Context->CompositeTypes.size());
    assuming(Context->CompositeTypes[TypeIdx]->isFunc());
    const auto &FuncType = Context->CompositeTypes[TypeIdx]->getFuncType();
    const auto FuncID = Context->Functions.size();
    auto FTy = toLLVMType(Context->LLContext, Context->ExecCtxPtrTy, FuncType);
    LLVM::FunctionCallee F = {FTy, Context->LLModule.get().addFunction(
                                       FTy, LLVMExternalLinkage,
                                       fmt::format("f{}"sv, FuncID).c_str())};
    F.Fn.setVisibility(LLVMProtectedVisibility);
    F.Fn.setDSOLocal(true);
    F.Fn.setDLLStorageClass(LLVMDLLExportStorageClass);
    F.Fn.addFnAttr(Context->NoStackArgProbe);
    F.Fn.addFnAttr(Context->StrictFP);
    F.Fn.addFnAttr(Context->UWTable);
    F.Fn.addParamAttr(0, Context->ReadOnly);
    F.Fn.addParamAttr(0, Context->NoAlias);

    Context->Functions.emplace_back(TypeIdx, F, &Code);
  }
}

Expect<void> Compiler::compileFunctionBody(uint32_t LocalFuncIndex) noexcept {
  // Find the function in the Functions list
  // LocalFuncIndex is relative to the defined functions (not imports)
  uint32_t GlobalFuncIndex = Context->ImportCount + LocalFuncIndex;
  if (GlobalFuncIndex >= Context->Functions.size()) {
    spdlog::error("[lazy-jit]: function index {} out of range"sv,
                  LocalFuncIndex);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  auto &[T, F, Code] = Context->Functions[GlobalFuncIndex];
  if (!Code) {
    spdlog::error("[lazy-jit]: cannot compile import function {}"sv,
                  LocalFuncIndex);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  // Check if already compiled (function has basic blocks)
  if (F.Fn.countBasicBlocks() > 0) {
    spdlog::debug("[lazy-jit]: function {} already compiled"sv, LocalFuncIndex);
    return {};
  }

  spdlog::debug("[lazy-jit]: compiling function {}"sv, LocalFuncIndex);

  std::vector<ValType> Locals;
  for (const auto &Local : Code->getLocals()) {
    for (unsigned I = 0; I < Local.first; ++I) {
      Locals.push_back(Local.second);
    }
  }

  FunctionCompiler FC(
      *Context, F, Locals, Conf.getCompilerConfigure().isInterruptible(),
      Conf.getStatisticsConfigure().isInstructionCounting(),
      Conf.getStatisticsConfigure().isCostMeasuring(),
      Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT);
  auto Type = Context->resolveBlockType(T);
  EXPECTED_TRY(FC.compile(*Code, std::move(Type)));
  F.Fn.eliminateUnreachableBlocks();

  return {};
}

Expect<LLVM::Data>
LLVM::Compiler::compileInfrastructure(const AST::Module &Module) noexcept {
  // Check the module is validated.
  if (unlikely(!Module.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    return Unexpect(ErrCode::Value::NotValidated);
  }

  std::unique_lock Lock(Mutex);
  spdlog::info("[lazy-jit]: compile infrastructure start"sv);

  Data D;
  auto LLContext = initLLVMModule(D);
  auto &LLModule = D.extract().LLModule;

  CompileContext NewContext(LLContext, LLModule,
                            Conf.getCompilerConfigure().isGenericBinary());
  RAIICleanup Cleanup(Context, &NewContext);
  Context->addVersionGlobal();

  // Compile all sections and the function declarations without bodies.
  compileSections(Module, false);
  // Compile ExportSection
  compile(Module.getExportSection());

  // Set initializer for constant value
  Context->finalizeIntrinsicsTable();
  LLModule.verify(LLVMPrintMessageAction);

  spdlog::info("[lazy-jit]: infrastructure compilation done"sv);

  return Expect<Data>{std::move(D)};
}

Expect<LLVM::Data>
Compiler::compileFunctions(Data &&LLData, const AST::Module &Module,
                           Span<const uint32_t> LocalFuncIndices) noexcept {
  if (unlikely(!Module.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    return Unexpect(ErrCode::Value::NotValidated);
  }
  if (unlikely(LocalFuncIndices.empty())) {
    spdlog::error("[lazy-jit]: compileFunctions with empty index list"sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  std::unique_lock Lock(Mutex);
  std::vector<uint32_t> Sorted(LocalFuncIndices.begin(),
                               LocalFuncIndices.end());
  std::sort(Sorted.begin(), Sorted.end());
  Sorted.erase(std::unique(Sorted.begin(), Sorted.end()), Sorted.end());

  spdlog::debug("[lazy-jit]: compile functions batch ({}) start"sv,
                Sorted.size());

  // Each batch starts from a fresh module sharing the same thread-safe
  // context: on success the previous batch module was consumed by the JIT,
  // and after a failed batch the leftover module must be discarded so its
  // declarations are not re-added on top of themselves.
  LLData.extract().resetModule();
  auto LLContext = initLLVMModule(LLData);
  auto &LLModule = LLData.extract().LLModule;

  CompileContext NewContext(LLContext, LLModule,
                            Conf.getCompilerConfigure().isGenericBinary());
  RAIICleanup Cleanup(Context, &NewContext);

  // Emit the type wrappers as external declarations resolved against the
  // infrastructure module, then declare the functions and compile the
  // requested bodies.
  compileSections(Module, true);

  for (uint32_t FuncIndex : Sorted) {
    EXPECTED_TRY(compileFunctionBody(FuncIndex));
  }

  spdlog::info("[lazy-jit]: verify batch ({} funcs) start"sv, Sorted.size());
  LLModule.verify(LLVMPrintMessageAction);
  spdlog::info("[lazy-jit]: verify batch ({} funcs) done"sv, Sorted.size());

  auto &TM = LLData.extract().TM;
  EXPECTED_TRY(optimize(LLModule, TM));

  spdlog::debug("[lazy-jit]: compile functions batch ({}) done"sv,
                Sorted.size());
  return Expect<Data>{std::move(LLData)};
}

} // namespace LLVM
} // namespace WasmEdge
