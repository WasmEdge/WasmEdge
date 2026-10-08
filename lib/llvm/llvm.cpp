// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "llvm.h"

#include <llvm/IR/GlobalValue.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#if LLVM_VERSION_MAJOR < 12 || WASMEDGE_OS_WINDOWS
#include <llvm/ExecutionEngine/Orc/Core.h>
#endif
#if LLVM_VERSION_MAJOR < 12
#include <llvm/Target/TargetMachine.h>
#endif
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/Support/CBindingWrapping.h>
#include <llvm/Support/Error.h>
#if LLVM_VERSION_MAJOR < 15
#else
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#endif
#if LLVM_VERSION_MAJOR < 15
#elif LLVM_VERSION_MAJOR < 22
#include <llvm/ExecutionEngine/Orc/DebugObjectManagerPlugin.h>
#include <llvm/ExecutionEngine/Orc/EPCDebugObjectRegistrar.h>
#else
#include <llvm/ExecutionEngine/Orc/Debugging/DebuggerSupport.h>
#endif
#if LLVM_VERSION_MAJOR < 15
#else
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h>
#include <llvm/ExecutionEngine/Orc/TargetProcess/JITLoaderGDB.h>
#if LLVM_VERSION_MAJOR < 18
#include <llvm/ExecutionEngine/Orc/DebuggerSupportPlugin.h>
#else
#include <llvm/ExecutionEngine/Orc/Debugging/DebuggerSupportPlugin.h>
#endif
#if LLVM_VERSION_MAJOR < 17
#else
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h>
#endif
#endif
#if LLVM_VERSION_MAJOR < 10
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/Host.h>
#endif
#if LLVM_VERSION_MAJOR < 18
#include <llvm/IR/Instructions.h>
#include <llvm/Support/CBindingWrapping.h>
#endif

#if WASMEDGE_OS_WINDOWS
#include "common/spdlog.h"
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/Process.h>
#include <system/winapi.h>
#endif

namespace llvm {
#if WASMEDGE_OS_WINDOWS
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::ExecutionSession,
                                   LLVMOrcExecutionSessionRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::ObjectLayer, LLVMOrcObjectLayerRef)
#endif
#if LLVM_VERSION_MAJOR < 11
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::ThreadSafeContext,
                                   LLVMOrcThreadSafeContextRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::JITDylib, LLVMOrcJITDylibRef)
#endif
#if LLVM_VERSION_MAJOR < 12
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::LLJITBuilder, LLVMOrcLLJITBuilderRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::JITTargetMachineBuilder,
                                   LLVMOrcJITTargetMachineBuilderRef)
#endif
#if LLVM_VERSION_MAJOR < 13
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::ThreadSafeModule,
                                   LLVMOrcThreadSafeModuleRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::IRTransformLayer,
                                   LLVMOrcIRTransformLayerRef)
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::MaterializationResponsibility,
                                   LLVMOrcMaterializationResponsibilityRef)
#endif
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(orc::LLJIT, LLVMOrcLLJITRef)
} // namespace llvm

#if LLVM_VERSION_MAJOR < 18
LLVMTailCallKind LLVMGetTailCallKind(LLVMValueRef Call) {
  return static_cast<LLVMTailCallKind>(
      llvm::unwrap<llvm::CallInst>(Call)->getTailCallKind());
}

void LLVMSetTailCallKind(LLVMValueRef Call, LLVMTailCallKind kind) {
  llvm::unwrap<llvm::CallInst>(Call)->setTailCallKind(
      static_cast<llvm::CallInst::TailCallKind>(kind));
}
#endif
#if LLVM_VERSION_MAJOR < 10
LLVMBool LLVMGetWeak(LLVMValueRef CmpXchgInst) noexcept {
  return llvm::unwrap<llvm::AtomicCmpXchgInst>(CmpXchgInst)->isWeak();
}

void LLVMSetWeak(LLVMValueRef CmpXchgInst, LLVMBool IsWeak) noexcept {
  llvm::unwrap<llvm::AtomicCmpXchgInst>(CmpXchgInst)->setWeak(IsWeak);
}
#endif

namespace WasmEdge::LLVM {

void Value::setDSOLocal(bool Local) noexcept {
  llvm::cast<llvm::GlobalValue>(reinterpret_cast<llvm::Value *>(Ref))
      ->setDSOLocal(Local);
}

void Value::eliminateUnreachableBlocks() noexcept {
  llvm::EliminateUnreachableBlocks(
      *llvm::cast<llvm::Function>(reinterpret_cast<llvm::Value *>(Ref)));
}

bool SectionIterator::isText() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isText();
}

bool SectionIterator::isData() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isData();
}

bool SectionIterator::isBSS() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isBSS();
}

bool SectionIterator::isPData() const noexcept {
#if WASMEDGE_OS_WINDOWS
  using namespace std::literals;
  return ".pdata"sv == getName();
#else
  return false;
#endif
}

bool SectionIterator::isEHFrame() const noexcept {
#if WASMEDGE_OS_LINUX
  using namespace std::literals;
  return ".eh_frame"sv == getName();
#elif WASMEDGE_OS_MACOS
  using namespace std::literals;
  return "__eh_frame"sv == getName();
#else
  return false;
#endif
}

bool SectionIterator::isVirtual() const noexcept {
  auto *S = reinterpret_cast<const llvm::object::section_iterator *>(Ref);
  return (*S)->isVirtual();
}

#if WASMEDGE_OS_WINDOWS
namespace {
class DefaultMMapper final : public llvm::SectionMemoryManager::MemoryMapper {
public:
  llvm::sys::MemoryBlock allocateMappedMemory(
      llvm::SectionMemoryManager::AllocationPurpose /*Purpose*/,
      size_t NumBytes, const llvm::sys::MemoryBlock *const NearBlock,
      unsigned Flags, std::error_code &EC) override {
    return llvm::sys::Memory::allocateMappedMemory(NumBytes, NearBlock, Flags,
                                                   EC);
  }
  std::error_code protectMappedMemory(const llvm::sys::MemoryBlock &Block,
                                      unsigned Flags) override {
    return llvm::sys::Memory::protectMappedMemory(Block, Flags);
  }

  std::error_code releaseMappedMemory(llvm::sys::MemoryBlock &M) override {
    return llvm::sys::Memory::releaseMappedMemory(M);
  }
};

class ContiguousSectionMemoryManager : public llvm::RTDyldMemoryManager {
public:
  explicit ContiguousSectionMemoryManager(
      llvm::SectionMemoryManager::MemoryMapper *UnownedMM = nullptr)
      : MMapper(UnownedMM), OwnedMMapper(nullptr) {
    if (!MMapper) {
      OwnedMMapper = std::make_unique<DefaultMMapper>();
      MMapper = OwnedMMapper.get();
    }
  }

  ~ContiguousSectionMemoryManager() noexcept override {
    using namespace std::literals;
    if (Preallocated.allocatedSize() != 0) {
      auto EC = MMapper->releaseMappedMemory(Preallocated);
      if (EC) {
        spdlog::error("releaseMappedMemory failed with error: {}"sv,
                      EC.message());
      }
    }
  }

  bool needsToReserveAllocationSpace() override { return true; }

  void reserveAllocationSpace(uintptr_t CodeSize, llvm::Align CodeAlign,
                              uintptr_t RODataSize, llvm::Align RODataAlign,
                              uintptr_t RWDataSize,
                              llvm::Align RWDataAlign) override {
    using namespace std::literals;
    assuming(Preallocated.allocatedSize() == 0);

    static const size_t PageSize = llvm::sys::Process::getPageSizeEstimate();
    assuming(CodeAlign.value() <= PageSize);
    assuming(RODataAlign.value() <= PageSize);
    assuming(RWDataAlign.value() <= PageSize);
    CodeSize = roundUpTo(CodeSize + CodeAlign.value(), PageSize);
    RODataSize = roundUpTo(RODataSize + RODataAlign.value(), PageSize);
    RWDataSize = roundUpTo(RWDataSize + RWDataAlign.value(), PageSize);
    const uintptr_t TotalSize =
        CodeSize + RODataSize + RWDataSize + PageSize * 3;

    std::error_code EC;
    Preallocated = MMapper->allocateMappedMemory(
        llvm::SectionMemoryManager::AllocationPurpose::Code, TotalSize, nullptr,
        llvm::sys::Memory::MF_READ | llvm::sys::Memory::MF_WRITE, EC);
    if (EC) {
      spdlog::error("allocateMappedMemory failed with error: {}"sv,
                    EC.message());
      return;
    }

    auto base = reinterpret_cast<std::uintptr_t>(Preallocated.base());
    CodeMem = CodeFree =
        llvm::sys::MemoryBlock(reinterpret_cast<void *>(base), CodeSize);
    base += CodeSize;
    RODataMem = RODataFree =
        llvm::sys::MemoryBlock(reinterpret_cast<void *>(base), RODataSize);
    base += RODataSize;
    RWDataMem = RWDataFree =
        llvm::sys::MemoryBlock(reinterpret_cast<void *>(base), RWDataSize);
  }

  uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment,
                               unsigned /*SectionID*/,
                               llvm::StringRef /*SectionName*/,
                               bool IsReadOnly) override {
    if (IsReadOnly) {
      return Allocate(RODataFree, Size, Alignment);
    } else {
      return Allocate(RWDataFree, Size, Alignment);
    }
  }

  uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                               unsigned /*SectionID*/,
                               llvm::StringRef /*SectionName*/) override {
    return Allocate(CodeFree, Size, Alignment);
  }

  bool finalizeMemory(std::string *ErrMsg) override {
    std::error_code EC;

    EC = MMapper->protectMappedMemory(CodeMem, llvm::sys::Memory::MF_READ |
                                                   llvm::sys::Memory::MF_EXEC);
    if (EC) {
      if (ErrMsg) {
        *ErrMsg = EC.message();
      }
      return true;
    }
    EC = MMapper->protectMappedMemory(RODataMem, llvm::sys::Memory::MF_READ);
    if (EC) {
      if (ErrMsg) {
        *ErrMsg = EC.message();
      }
      return true;
    }

    llvm::sys::Memory::InvalidateInstructionCache(CodeMem.base(),
                                                  CodeMem.allocatedSize());
    return false;
  }

private:
  llvm::sys::MemoryBlock Preallocated;

  // Sections must be in the order code < rodata < rwdata.
  llvm::sys::MemoryBlock CodeMem;
  llvm::sys::MemoryBlock RODataMem;
  llvm::sys::MemoryBlock RWDataMem;

  llvm::sys::MemoryBlock CodeFree;
  llvm::sys::MemoryBlock RODataFree;
  llvm::sys::MemoryBlock RWDataFree;

  llvm::SectionMemoryManager::MemoryMapper *MMapper;
  std::unique_ptr<llvm::SectionMemoryManager::MemoryMapper> OwnedMMapper;

  uint8_t *Allocate(llvm::sys::MemoryBlock &FreeBlock, std::uintptr_t Size,
                    unsigned alignment) {
    using namespace std::literals;
    const auto Base = reinterpret_cast<uintptr_t>(FreeBlock.base());
    const auto Start = roundUpTo(Base, alignment);
    const uintptr_t PaddedSize = (Start - Base) + Size;
    if (PaddedSize > FreeBlock.allocatedSize()) {
      spdlog::error("Failed to satisfy suballocation request for {}"sv, Size);
      return nullptr;
    }
    FreeBlock =
        llvm::sys::MemoryBlock(reinterpret_cast<void *>(Base + PaddedSize),
                               FreeBlock.allocatedSize() - PaddedSize);
    return reinterpret_cast<uint8_t *>(Start);
  }

  static uintptr_t roundUpTo(uintptr_t Value, uintptr_t Divisor) noexcept {
    return ((Value + (Divisor - 1)) / Divisor) * Divisor;
  }
};

// Register stack unwind info for JIT functions
class Win64EHManager : public ContiguousSectionMemoryManager {
  using Base = ContiguousSectionMemoryManager;
  uint64_t CodeAddress = 0;

public:
  ~Win64EHManager() noexcept override {}

  uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID,
                               llvm::StringRef SectionName) override {
    using namespace std::literals;
    const auto Allocated =
        Base::allocateCodeSection(Size, Alignment, SectionID, SectionName);
    if (SectionName == llvm::StringRef(".text"sv)) {
      CodeAddress = reinterpret_cast<uint64_t>(Allocated);
    }
    return Allocated;
  }

  void registerEHFrames(uint8_t *Addr, uint64_t /*LoadAddr*/,
                        size_t Size) noexcept override {
    using namespace std::literals;
    winapi::RUNTIME_FUNCTION_ *const FunctionTable =
        reinterpret_cast<winapi::RUNTIME_FUNCTION_ *>(Addr);
    const uint32_t EntryCount =
        static_cast<uint32_t>(Size / sizeof(winapi::RUNTIME_FUNCTION_));
    if (EntryCount == 0)
      return;
    // Calculate the object image base address by assuming that the address of
    // the first function is equal to the address of the code section.
    const auto ImageBase = CodeAddress - FunctionTable[0].BeginAddress;
    winapi::RtlAddFunctionTable(FunctionTable, EntryCount, ImageBase);
    EHFrames.push_back({Addr, Size});
  }
  void deregisterEHFrames() noexcept override {
    using namespace std::literals;
    for (auto &Frame : EHFrames) {
      winapi::RtlDeleteFunctionTable(
          reinterpret_cast<winapi::RUNTIME_FUNCTION_ *>(Frame.Addr));
    }
    EHFrames.clear();
  }
};
} // namespace

LLVMOrcLLJITBuilderRef OrcLLJIT::getBuilder() noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  const LLVMOrcLLJITBuilderRef Builder = LLVMOrcCreateLLJITBuilder();
  LLVMOrcLLJITBuilderSetObjectLinkingLayerCreator(
      Builder,
      [](void *, LLVMOrcExecutionSessionRef ES, const char *) noexcept {
        auto Layer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(
            *unwrap(ES), [](
#if LLVM_VERSION_MAJOR < 21
#else
                             const llvm::MemoryBuffer &
#endif
                         ) { return std::make_unique<Win64EHManager>(); });
        Layer->setOverrideObjectFlagsWithResponsibilityFlags(true);
        Layer->setAutoClaimResponsibilityForObjectSymbols(true);
        return wrap(static_cast<llvm::orc::ObjectLayer *>(Layer.release()));
      },
      nullptr);
  return Builder;
}
#elif LLVM_VERSION_MAJOR < 10
namespace {
class EHFrameDeregisteringMemoryManager : public llvm::SectionMemoryManager {
public:
  ~EHFrameDeregisteringMemoryManager() noexcept override {
    deregisterEHFrames();
  }
};
} // namespace

LLVMOrcLLJITBuilderRef OrcLLJIT::getBuilder() noexcept {
  using llvm::unwrap;
  const LLVMOrcLLJITBuilderRef Builder = LLVMOrcCreateLLJITBuilder();
  if (auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost()) {
    llvm::StringMap<bool> FeatureMap;
    llvm::sys::getHostCPUFeatures(FeatureMap);
    for (auto &Feature : FeatureMap) {
      JTMB->getFeatures().AddFeature(Feature.first(), Feature.second);
    }
    JTMB->setCPU(llvm::sys::getHostCPUName().str());
    unwrap(Builder)->setJITTargetMachineBuilder(std::move(*JTMB));
  } else {
    llvm::consumeError(JTMB.takeError());
  }
  unwrap(Builder)->setObjectLinkingLayerCreator(
      [](llvm::orc::ExecutionSession &ES) {
        return std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(ES, []() {
          return std::make_unique<EHFrameDeregisteringMemoryManager>();
        });
      });
  return Builder;
}
#else
LLVMOrcLLJITBuilderRef OrcLLJIT::getBuilder() noexcept { return nullptr; }
#endif

#if LLVM_VERSION_MAJOR < 15 || WASMEDGE_OS_WINDOWS
#else
namespace {
llvm::Error addMachODebuggerSupport(llvm::orc::LLJIT &J,
                                    llvm::orc::ObjectLinkingLayer &Linking) {
  auto &ES = J.getExecutionSession();
  auto &ProcessJD = ES.createBareJITDylib("<wasmedge-debugger-support>");
#if LLVM_VERSION_MAJOR < 17
  const llvm::JITEvaluatedSymbol AllocAction(
      llvm::pointerToJITTargetAddress(
          &llvm_orc_registerJITLoaderGDBAllocAction),
      llvm::JITSymbolFlags::Exported);
#else
  const llvm::orc::ExecutorSymbolDef AllocAction(
      llvm::orc::ExecutorAddr::fromPtr(
          &llvm_orc_registerJITLoaderGDBAllocAction),
      llvm::JITSymbolFlags::Exported);
#endif
  if (auto Err = ProcessJD.define(llvm::orc::absoluteSymbols(
          {{J.mangleAndIntern("llvm_orc_registerJITLoaderGDBAllocAction"),
            AllocAction}}))) {
    return Err;
  }
  auto Plugin = llvm::orc::GDBJITDebugInfoRegistrationPlugin::Create(
      ES, ProcessJD, J.getTargetTriple());
  if (!Plugin) {
    return Plugin.takeError();
  }
  Linking.addPlugin(std::move(*Plugin));
  return llvm::Error::success();
}
} // namespace
#endif

Error OrcLLJIT::enableDebuggerSupport() noexcept {
#if WASMEDGE_OS_WINDOWS
  return Error(llvm::wrap(llvm::make_error<llvm::StringError>(
      "not supported on Windows", llvm::inconvertibleErrorCode())));
#elif WASMEDGE_OS_MACOS && LLVM_VERSION_MAJOR < 13
  return Error(llvm::wrap(llvm::make_error<llvm::StringError>(
      "unknown JIT object layer", llvm::inconvertibleErrorCode())));
#else
  auto &J = *llvm::unwrap(Ref);
  auto &Layer = J.getObjLinkingLayer();
#if LLVM_VERSION_MAJOR < 11
  static_cast<llvm::orc::RTDyldObjectLinkingLayer &>(Layer).setNotifyLoaded(
      [](llvm::orc::VModuleKey K, const llvm::object::ObjectFile &Obj,
         const llvm::RuntimeDyld::LoadedObjectInfo &Info) {
        llvm::JITEventListener::createGDBRegistrationListener()
            ->notifyObjectLoaded(K, Obj, Info);
      });
  return Error();
#elif LLVM_VERSION_MAJOR < 13
  static_cast<llvm::orc::RTDyldObjectLinkingLayer &>(Layer)
      .registerJITEventListener(
          *llvm::JITEventListener::createGDBRegistrationListener());
  return Error();
#else
  if (auto *RTDyld =
          llvm::dyn_cast<llvm::orc::RTDyldObjectLinkingLayer>(&Layer)) {
    RTDyld->registerJITEventListener(
        *llvm::JITEventListener::createGDBRegistrationListener());
    return Error();
  }
#if LLVM_VERSION_MAJOR < 15
  return Error(llvm::wrap(llvm::make_error<llvm::StringError>(
      "unknown JIT object layer", llvm::inconvertibleErrorCode())));
#else
  auto *Linking = llvm::dyn_cast<llvm::orc::ObjectLinkingLayer>(&Layer);
  if (Linking && J.getTargetTriple().isOSBinFormatMachO()) {
    return Error(llvm::wrap(addMachODebuggerSupport(J, *Linking)));
  }
#if LLVM_VERSION_MAJOR < 22
  if (!Linking) {
    return Error(llvm::wrap(llvm::make_error<llvm::StringError>(
        "unknown JIT object layer", llvm::inconvertibleErrorCode())));
  }
  auto &ES = J.getExecutionSession();
  auto Registrar = std::make_unique<llvm::orc::EPCDebugObjectRegistrar>(
      ES,
      llvm::orc::ExecutorAddr::fromPtr(&llvm_orc_registerJITLoaderGDBWrapper));
#if LLVM_VERSION_MAJOR < 17
  Linking->addPlugin(std::make_unique<llvm::orc::DebugObjectManagerPlugin>(
      ES, std::move(Registrar)));
#else
  Linking->addPlugin(std::make_unique<llvm::orc::DebugObjectManagerPlugin>(
      ES, std::move(Registrar), true, true));
#endif
  return Error();
#else
  return Error(llvm::wrap(llvm::orc::enableDebuggerSupport(J)));
#endif
#endif
#endif
#endif
}

} // namespace WasmEdge::LLVM

#if LLVM_VERSION_MAJOR < 12 && WASMEDGE_OS_WINDOWS
void LLVMOrcLLJITBuilderSetObjectLinkingLayerCreator(
    LLVMOrcLLJITBuilderRef Builder,
    LLVMOrcLLJITBuilderObjectLinkingLayerCreatorFunction F,
    void *Ctx) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  unwrap(Builder)->setObjectLinkingLayerCreator(
      [=](llvm::orc::ExecutionSession &ES, const llvm::Triple &TT) {
        auto TTStr = TT.str();
        return std::unique_ptr<llvm::orc::ObjectLayer>(
            unwrap(F(Ctx, wrap(&ES), TTStr.c_str())));
      });
}
#endif
#if LLVM_VERSION_MAJOR < 11
LLVMOrcThreadSafeContextRef LLVMOrcCreateNewThreadSafeContext() noexcept {
  using llvm::wrap;
  return wrap(
      new llvm::orc::ThreadSafeContext(std::make_unique<llvm::LLVMContext>()));
}

LLVMContextRef
LLVMOrcThreadSafeContextGetContext(LLVMOrcThreadSafeContextRef TSCtx) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  return wrap(unwrap(TSCtx)->getContext());
}

void LLVMOrcDisposeThreadSafeContext(
    LLVMOrcThreadSafeContextRef TSCtx) noexcept {
  delete llvm::unwrap(TSCtx);
}

LLVMOrcThreadSafeModuleRef
LLVMOrcCreateNewThreadSafeModule(LLVMModuleRef M,
                                 LLVMOrcThreadSafeContextRef TSCtx) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  return wrap(new llvm::orc::ThreadSafeModule(
      std::unique_ptr<llvm::Module>(unwrap(M)), *unwrap(TSCtx)));
}

void LLVMOrcDisposeThreadSafeModule(LLVMOrcThreadSafeModuleRef TSM) noexcept {
  delete llvm::unwrap(TSM);
}

LLVMOrcLLJITBuilderRef LLVMOrcCreateLLJITBuilder() noexcept {
  return llvm::wrap(new llvm::orc::LLJITBuilder());
}

void LLVMOrcDisposeLLJITBuilder(LLVMOrcLLJITBuilderRef Builder) noexcept {
  delete llvm::unwrap(Builder);
}

void LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(
    LLVMOrcLLJITBuilderRef Builder,
    LLVMOrcJITTargetMachineBuilderRef JTMB) noexcept {
  using llvm::unwrap;
  unwrap(Builder)->setJITTargetMachineBuilder(std::move(*unwrap(JTMB)));
  delete unwrap(JTMB);
}

LLVMErrorRef LLVMOrcCreateLLJIT(LLVMOrcLLJITRef *Result,
                                LLVMOrcLLJITBuilderRef Builder) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  if (!Builder) {
    Builder = LLVMOrcCreateLLJITBuilder();
  }
  auto J = unwrap(Builder)->create();
  LLVMOrcDisposeLLJITBuilder(Builder);
  if (!J) {
    *Result = nullptr;
    return wrap(J.takeError());
  }
  *Result = wrap(J->release());
  return LLVMErrorSuccess;
}

LLVMErrorRef LLVMOrcDisposeLLJIT(LLVMOrcLLJITRef J) noexcept {
  delete llvm::unwrap(J);
  return LLVMErrorSuccess;
}

LLVMOrcJITDylibRef LLVMOrcLLJITGetMainJITDylib(LLVMOrcLLJITRef J) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  return wrap(&unwrap(J)->getMainJITDylib());
}

LLVMErrorRef
LLVMOrcLLJITAddLLVMIRModule(LLVMOrcLLJITRef J, LLVMOrcJITDylibRef JD,
                            LLVMOrcThreadSafeModuleRef TSM) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  std::unique_ptr<llvm::orc::ThreadSafeModule> TmpTSM(unwrap(TSM));
  return wrap(unwrap(J)->addIRModule(*unwrap(JD), std::move(*TmpTSM)));
}

LLVMErrorRef LLVMOrcLLJITLookup(LLVMOrcLLJITRef J,
                                LLVMOrcJITTargetAddress *Result,
                                const char *Name) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  auto Sym = unwrap(J)->lookup(Name);
  if (!Sym) {
    *Result = 0;
    return wrap(Sym.takeError());
  }
  *Result = Sym->getAddress();
  return LLVMErrorSuccess;
}
#endif
#if LLVM_VERSION_MAJOR < 12
LLVMOrcJITTargetMachineBuilderRef
LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(LLVMTargetMachineRef TM) {
  using llvm::wrap;
  auto *TemplateTM = reinterpret_cast<llvm::TargetMachine *>(TM);
  auto JTMB = std::make_unique<llvm::orc::JITTargetMachineBuilder>(
      TemplateTM->getTargetTriple());
  JTMB->setCPU(TemplateTM->getTargetCPU().str())
      .setRelocationModel(TemplateTM->getRelocationModel())
      .setCodeModel(TemplateTM->getCodeModel())
      .setCodeGenOptLevel(TemplateTM->getOptLevel());
#if LLVM_VERSION_MAJOR < 10
  JTMB->getFeatures() =
      llvm::SubtargetFeatures(TemplateTM->getTargetFeatureString());
  JTMB->getOptions() = TemplateTM->Options;
#else
  JTMB->setFeatures(TemplateTM->getTargetFeatureString())
      .setOptions(TemplateTM->Options);
#endif
  LLVMDisposeTargetMachine(TM);
  return wrap(JTMB.release());
}

LLVMOrcResourceTrackerRef
LLVMOrcJITDylibCreateResourceTracker(LLVMOrcJITDylibRef JD) noexcept {
  return reinterpret_cast<LLVMOrcResourceTrackerRef>(JD);
}

void LLVMOrcReleaseResourceTracker(LLVMOrcResourceTrackerRef) noexcept {}

LLVMErrorRef LLVMOrcResourceTrackerRemove(LLVMOrcResourceTrackerRef) noexcept {
  return LLVMErrorSuccess;
}

LLVMErrorRef
LLVMOrcLLJITAddLLVMIRModuleWithRT(LLVMOrcLLJITRef J,
                                  LLVMOrcResourceTrackerRef RT,
                                  LLVMOrcThreadSafeModuleRef TSM) noexcept {
  return LLVMOrcLLJITAddLLVMIRModule(
      J, reinterpret_cast<LLVMOrcJITDylibRef>(RT), TSM);
}
#endif
#if LLVM_VERSION_MAJOR < 11
#elif LLVM_VERSION_MAJOR < 13
LLVMOrcIRTransformLayerRef
LLVMOrcLLJITGetIRTransformLayer(LLVMOrcLLJITRef J) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  return wrap(&(unwrap(J)->getIRTransformLayer()));
}
void LLVMOrcIRTransformLayerSetTransform(
    LLVMOrcIRTransformLayerRef IRTransformLayer,
    LLVMOrcIRTransformLayerTransformFunction TransformFunction,
    void *Ctx) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
  unwrap(IRTransformLayer)
      ->setTransform([=](llvm::orc::ThreadSafeModule TSM,
                         llvm::orc::MaterializationResponsibility &R)
                         -> llvm::Expected<llvm::orc::ThreadSafeModule> {
        LLVMOrcThreadSafeModuleRef TSMRef =
            wrap(new llvm::orc::ThreadSafeModule(std::move(TSM)));
        if (LLVMErrorRef Err = TransformFunction(Ctx, &TSMRef, wrap(&R))) {
          return unwrap(Err);
        }
        return std::move(*unwrap(TSMRef));
      });
}
#endif
#if LLVM_VERSION_MAJOR < 13
LLVMErrorRef
LLVMOrcThreadSafeModuleWithModuleDo(LLVMOrcThreadSafeModuleRef TSM,
                                    LLVMOrcGenericIRModuleOperationFunction F,
                                    void *Ctx) noexcept {
  using llvm::unwrap;
  using llvm::wrap;
#if LLVM_VERSION_MAJOR < 10
  auto Lock = unwrap(TSM)->getContextLock();
  return F(Ctx, wrap(unwrap(TSM)->getModule()));
#else
  return wrap(unwrap(TSM)->withModuleDo(
      [&](llvm::Module &M) { return unwrap(F(Ctx, wrap(&M))); }));
#endif
}
#endif
