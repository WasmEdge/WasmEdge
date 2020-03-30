// SPDX-License-Identifier: Apache-2.0
#include "compiler/library.h"
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>

namespace {
static const constexpr uint32_t kPageSize = 65536;
}

namespace SSVM {
namespace Compiler {

class Library::Engine {
private:
  llvm::orc::ThreadSafeContext TSCtx;
  std::unique_ptr<llvm::orc::LLLazyJIT> JIT;

public:
  Engine()
      : TSCtx(std::make_unique<llvm::LLVMContext>()),
        JIT(llvm::cantFail(llvm::orc::LLLazyJITBuilder().create())) {
    static_cast<llvm::orc::RTDyldObjectLinkingLayer &>(
        JIT->getObjLinkingLayer())
        .setNotifyLoaded(
            [GDBListener =
                 llvm::JITEventListener::createGDBRegistrationListener()](
                llvm::JITEventListener::ObjectKey K,
                const llvm::object::ObjectFile &Obj,
                const llvm::RuntimeDyld::LoadedObjectInfo &L) {
              GDBListener->notifyObjectLoaded(K, Obj, L);
            });
  }

  llvm::LLVMContext &getContext() { return *TSCtx.getContext(); }

  llvm::Error addModule(std::unique_ptr<llvm::Module> Module) {
    return JIT->addIRModule(
        llvm::orc::ThreadSafeModule(std::move(Module), TSCtx));
  }

  llvm::Error defineAbsolute(llvm::StringRef Name,
                             llvm::JITEvaluatedSymbol Address) {
    return JIT->defineAbsolute(Name, Address);
  }

  llvm::Expected<llvm::JITEvaluatedSymbol> lookup(llvm::StringRef Name) {
    return JIT->lookup(Name);
  }
};

Library::Library() : ExecutionEngine(nullptr) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  ExecutionEngine = new Engine;
  MemoryPtr = Memory.data();
}

llvm::LLVMContext &Library::getContext() {
  return ExecutionEngine->getContext();
}

void Library::setModule(std::unique_ptr<llvm::Module> Module) {
  llvm::cantFail(ExecutionEngine->addModule(std::move(Module)));

  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "memset",
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&std::memset),
                               llvm::JITSymbolFlags::Exported |
                                   llvm::JITSymbolFlags::Callable)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "memcpy",
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&std::memcpy),
                               llvm::JITSymbolFlags::Exported |
                                   llvm::JITSymbolFlags::Callable)));

  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "$trap",
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&trapProxy),
                               llvm::JITSymbolFlags::Exported |
                                   llvm::JITSymbolFlags::Callable)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "$memory.size",
      llvm::JITEvaluatedSymbol(
          llvm::pointerToJITTargetAddress(&memorySizeProxy),
          llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "$memory.grow",
      llvm::JITEvaluatedSymbol(
          llvm::pointerToJITTargetAddress(&memoryGrowProxy),
          llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "$lib.ctx",
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(this),
                               llvm::JITSymbolFlags::Exported)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      "$memory",
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&MemoryPtr),
                               llvm::JITSymbolFlags::Exported)));
}

Library::~Library() noexcept { delete ExecutionEngine; }

ErrCode Library::execute() {
  using namespace std::literals;
  return execute("_start"s);
}

ErrCode Library::execute(const std::string &FuncName) {
  if (auto Function = ExecutionEngine->lookup("$ctor")) {
    reinterpret_cast<void (*)()>(Function->getAddress())();
  } else {
    llvm::errs() << Function.takeError() << '\n';
  }
  if (auto Function = ExecutionEngine->lookup(FuncName)) {
    try {
      reinterpret_cast<void (*)()>(Function->getAddress())();
    } catch (const ErrCode &Status) {
      return Status;
    }
  } else {
    llvm::errs() << Function.takeError() << '\n';
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

void Library::trap(ErrCode Status) {
  throw Status;
}

uint32_t Library::memorySize() {
  const uint32_t Size = Memory.size() / kPageSize;
  return Size;
}

uint32_t Library::memoryGrow(uint32_t NewSize) {
  const uint32_t OldSize = Memory.size() / kPageSize;
  Memory.resize(Memory.size() + NewSize * kPageSize);
  MemoryPtr = Memory.data();
  return OldSize;
}

void Library::terminate() { throw ErrCode::Terminated; }

ErrCode Library::setHostFunction(std::unique_ptr<HostFunction> Func,
                                 const std::string &ModName,
                                 const std::string &FuncName) {
  const std::string FullName = ModName + '.' + FuncName;
  const std::string FullCtxName = FullName + ".ctx";
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      FullName,
      llvm::JITEvaluatedSymbol(
          llvm::pointerToJITTargetAddress(Func->getFunction()),
          llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable)));
  llvm::cantFail(ExecutionEngine->defineAbsolute(
      FullCtxName,
      llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(Func.get()),
                               llvm::JITSymbolFlags::Exported)));

  HostFuncs.emplace_back(std::move(Func));
  return ErrCode::Success;
}

} // namespace Compiler
} // namespace SSVM
