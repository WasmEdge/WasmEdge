// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "loader/serialize.h"

namespace WasmEdge {
namespace Executor {

Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp) {
  auto Res = instantiate(StoreMgr, Comp);
  if (!Res) {
    return Unexpect(Res);
  }
  return Res;
}
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp,
                               std::string_view Name) {
  auto Res = instantiate(StoreMgr, Comp, Name);
  if (!Res) {
    return Unexpect(Res);
  }
  return Res;
}

/// Instantiate a WASM Module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiateModule(Runtime::StoreManager &StoreMgr,
                            const AST::Module &Mod) {
  if (auto Res = instantiate(StoreMgr, Mod)) {
    return Res;
  } else {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
}

/// Register a named WASM module. See "include/executor/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const AST::Module &Mod, std::string_view Name) {
  if (auto Res = instantiate(StoreMgr, Mod, Name)) {
    return Res;
  } else {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
}

/// Register an instantiated module. See "include/executor/executor.h".
Expect<void>
Executor::registerModule(Runtime::StoreManager &StoreMgr,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  if (auto Res = StoreMgr.registerModule(&ModInst); !Res) {
    spdlog::error(ErrCode::Value::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::ModuleNameConflict);
  }
  return {};
}
Expect<void> Executor::registerComponent(
    Runtime::StoreManager &StoreMgr,
    const Runtime::Instance::ComponentInstance &CompInst) {
  if (auto Res = StoreMgr.registerComponent(&CompInst); !Res) {
    spdlog::error(ErrCode::Value::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return Unexpect(ErrCode::Value::ModuleNameConflict);
  }
  return {};
}

/// Register a host function which will be invoked before calling a
/// host function.
Expect<void> Executor::registerPreHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPreHost(HostData, HostFunc);
  return {};
}

/// Register a host function which will be invoked after calling a
/// host function.
Expect<void> Executor::registerPostHostFunction(
    void *HostData = nullptr, std::function<void(void *)> HostFunc = nullptr) {
  HostFuncHelper.setPostHost(HostData, HostFunc);
  return {};
}

// Invoke function. See "include/executor/executor.h".
Expect<std::vector<std::pair<ValVariant, ValType>>>
Executor::invoke(const Runtime::Instance::FunctionInstance *FuncInst,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Matching arguments and function type.
  const auto &FuncType = FuncInst->getFuncType();
  const auto &PTypes = FuncType.getParamTypes();
  const auto &RTypes = FuncType.getReturnTypes();
  // The defined type list may be empty if the function is an independent
  // function instance, that is, the module instance will be nullptr. For this
  // case, all of value types are number types or abstract heap types.
  //
  // If a function belongs to component instance, we should totally get
  // converted type, so should no need type list.
  WasmEdge::Span<const WasmEdge::AST::SubType *const> TypeList = {};
  if (FuncInst->getModule()) {
    TypeList = FuncInst->getModule()->getTypeList();
  }
  if (!AST::TypeMatcher::matchTypes(TypeList, ParamTypes, PTypes)) {
    spdlog::error(ErrCode::Value::FuncSigMismatch);
    spdlog::error(ErrInfo::InfoMismatch(
        PTypes, RTypes, std::vector(ParamTypes.begin(), ParamTypes.end()),
        RTypes));
    return Unexpect(ErrCode::Value::FuncSigMismatch);
  }

  // Check the reference value validation.
  for (uint32_t I = 0; I < ParamTypes.size(); ++I) {
    if (ParamTypes[I].isRefType() && (!ParamTypes[I].isNullableRefType() &&
                                      Params[I].get<RefVariant>().isNull())) {
      spdlog::error(ErrCode::Value::NonNullRequired);
      spdlog::error("    Cannot pass a null reference as argument of {}.",
                    ParamTypes[I]);
      return Unexpect(ErrCode::Value::NonNullRequired);
    }
  }

  Runtime::StackManager StackMgr;

  // Call runFunction.
  if (auto Res = runFunction(StackMgr, *FuncInst, Params); !Res) {
    return Unexpect(Res);
  }

  // Get return values.
  std::vector<std::pair<ValVariant, ValType>> Returns(RTypes.size());
  for (uint32_t I = 0; I < RTypes.size(); ++I) {
    auto Val = StackMgr.pop();
    const auto &RType = RTypes[RTypes.size() - I - 1];
    if (RType.isRefType()) {
      // For the reference type cases of the return values, they should be
      // transformed into abstract heap types due to the opaque of type indices.
      auto &RefType = Val.get<RefVariant>().getType();
      if (RefType.isExternalized()) {
        // First handle the forced externalized value type case.
        RefType = ValType(TypeCode::Ref, TypeCode::ExternRef);
      }
      if (!RefType.isAbsHeapType()) {
        // The instance must not be nullptr because the null references are
        // already dynamic typed into the top abstract heap type.
        auto *Inst =
            Val.get<RefVariant>().getPtr<Runtime::Instance::CompositeBase>();
        assuming(Inst);
        // The ModInst may be nullptr only in the independent host function
        // instance. Therefore the module instance here must not be nullptr
        // because the independent host function instance cannot be imported and
        // be referred by instructions.
        const auto *ModInst = Inst->getModule();
        auto *DefType = *ModInst->getType(RefType.getTypeIndex());
        RefType =
            ValType(RefType.getCode(), DefType->getCompositeType().expand());
      }
      // Should use the value type from the reference here due to the dynamic
      // typing rule of the null references.
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RefType);
    } else {
      // For the number type cases of the return values, the unused bits should
      // be erased due to the security issue.
      cleanNumericVal(Val, RType);
      Returns[RTypes.size() - I - 1] = std::make_pair(Val, RType);
    }
  }

  // After execution, the value stack size should be 0.
  assuming(StackMgr.size() == 0);
  return Returns;
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
Executor::asyncInvoke(const Runtime::Instance::FunctionInstance *FuncInst,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (Executor::*FPtr)(
      const Runtime::Instance::FunctionInstance *, Span<const ValVariant>,
      Span<const ValType>) = &Executor::invoke;
  return {FPtr, *this, FuncInst, std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

void Executor::generateCoredump(Runtime::StackManager &StackMgr) {
  const Configure Conf;
  Loader::Serializer Ser(Conf);
  const auto *CurrentInstance = StackMgr.getModule();
  if (CurrentInstance == nullptr) {
    return;
  }

  auto Core = collectProcessInformation();
  // auto DataSection = collectDataSection(StackMgr);
  auto MemSec = Executor::collectMemory(StackMgr);
  // auto Globals = collectGlobals(StackMgr);
  auto CoreStack = collectCoreStack(StackMgr);
  // auto CoreInstances = collectCoreInstances(StackMgr);
  // auto CoreModules = collectCoreModules(StackMgr);

  //  final file generation
  AST::Module Mod{};
  std::vector<Byte> &Magic = Mod.getMagic();
  Magic.insert(Magic.begin(), {0x00, 0x61, 0x73, 0x6d});

  std::vector<Byte> &Version = Mod.getVersion();
  Version.insert(Version.begin(), {0x01, 0x00, 0x00, 0x00});

  Mod.getCustomSections().emplace_back(Core);
  // Mod.getCustomSections().emplace_back(CoreModules);
  // Mod.getCustomSections().emplace_back(CoreInstances);
  Mod.getCustomSections().emplace_back(CoreStack);
  // Mod.getDataSection() = std::move(DataSection);
  Mod.getMemorySection() = std::move(MemSec);
  // TODO globals works with DWARF data, is this section needed?
  //  Mod.getGlobalSection() = std::move(Globals);
  auto Res = Ser.serializeModule(Mod);
  std::ofstream File("coredump.wasm", std::ios::out | std::ios::binary);
  if (File.is_open()) {
    File.write(reinterpret_cast<const char *>(Res->data()), Res->size());
    File.close();
  } else {
    // TODO change to assuming
    //  Handle error opening file
    throw std::ios_base::failure("Failed");
  }
}

AST::CustomSection Executor::collectProcessInformation() {
  spdlog::info("Constructing Core");
  // AST node
  AST::CustomSection Core;
  Core.setName("core");

  // Trying to insert this in leads to wasmgdb
  // saying it is an unsupported process-type
  // std::string Name = "a.wasm";
  // std::vector<Byte> Data(Name.begin(), Name.end());
  // if (Data.size() != 0) { // case where the module has an empty name field
  //   Content.insert(Content.end(), Data.begin(), Data.end());
  // } else {
  //   Content.insert(Content.begin(), {0x00, 0x00});
  // }
  auto &Content = Core.getContent();
  Content.insert(Content.begin(), {0x00, 0x00});

  return Core;
}

AST::MemorySection Executor::collectMemory(Runtime::StackManager &StackMgr) {
  spdlog::info("Collecting memory section");

  AST::MemorySection MemSec;
  const auto *CurrentInstance = StackMgr.getModule();
  auto MemInstances = CurrentInstance->getMemoryInstances();
  std::cout << MemInstances.size() << "\n";
  std::cout << MemInstances[0]->getMemoryType().getLimit().hasMax() << "\n";
  std::cout << MemInstances[0]->getMemoryType().getLimit().getMax() << "\n";
  std::cout << MemInstances[0]->getMemoryType().getLimit().getMin() << "\n";
  MemInstances[0]->getMemoryType().getLimit();
  // In the current version of WebAssembly, at most one memory may be defined
  // or imported in a single module, and all constructs implicitly reference
  // this memory.
  //
  // If hasMax, set Max to that.
  // Else set whatever Min exists

  return MemSec;
}

// Data section
// TODO malformed section
AST::DataSection Executor::collectDataSection(Runtime::StackManager &StackMgr) {
  spdlog::info("Collecting Memories");

  AST::DataSection Datasec;

  const auto *CurrentInstance = StackMgr.getModule();
  auto &DataInstances = CurrentInstance->getOwnedDataInstances();

  AST::DataSegment Seg;
  auto &Content = Seg.getData();

  for (auto &Iter : DataInstances) {
    Content.insert(Content.end(), Iter->getData().begin(),
                   Iter->getData().end());
  }
  if (Content.size() == 0) {
    std::cout << "empty\n";
    std::cout << Content.max_size();
    Content.insert(Seg.getData().end(), 0x00);
  }
  std::cout << Content.size();
  Datasec.getContent().push_back(Seg);
  return Datasec;
}

// corestack   ::= customsec(thread-info vec(frame))
// thread-info ::= 0x0 thread-name:name
// frame       ::= 0x0 funcidx:u32 codeoffset:u32
//                     locals:vec(value) stack:vec(value)
AST::CustomSection Executor::collectCoreStack(Runtime::StackManager &StackMgr) {
  spdlog::info("Constructing CoreStack");

  AST::CustomSection CoreStack;
  CoreStack.setName("corestack");

  auto Frames = StackMgr.getAllFrames();
  std::reverse(Frames.begin(), Frames.end());
  auto Size = Frames.size();

  // TODO get thread info
  auto &Content = CoreStack.getContent();
  Content.push_back(0x00);
  Content.push_back(0x04);

  std::string str = "main";
  Content.insert(Content.end(), str.begin(), str.end());
  for (size_t i = 0; i < Size; i++) {
    if (Frames[i].Module == nullptr) // guard
      continue;

    auto Funcidx = Frames[i].From->getTargetIndex(); // function index
    // TODO which to use?
    //  auto Funcidx = Item.From->getTargetIndex(); // function index
    auto Codeoffset = 0;
    // auto Codeoffset = 0; // only if codeoffset is unknown?
    auto LocalsN = Frames[i].Locals;
    std::cout << Frames[i].From->getOffset() << "\n";

    auto Locals = StackMgr.getRangeSpan(Frames[i].VPos, Frames[i].VPos);
    auto Stack =
        StackMgr.getTopSpan(StackMgr.size() - LocalsN); // TODO is this correct?

    Content.push_back(Funcidx);
    Content.push_back(Codeoffset);
    //
    // // TODO get the number type before inserting in the bytes
    // Is that even required?
    for (auto &Iter : Locals) {
      Content.push_back(Iter.unwrap());
    }
    for (auto &Iter : Stack) {
      Content.push_back(Iter.unwrap());
    }
  }

  return CoreStack;
}

// AST::GlobalSection collectGlobals(Runtime::StackManager &StackMgr) {
//   spdlog::info("Collecting Globals");
//   AST::GlobalSection Globals;
//   const auto *const CurrentInstance = StackMgr.getModule();
//
//   auto &GlobalInstances = CurrentInstance->getOwnedGlobalInstances();
//   auto &Content = Globals.getContent();
//
//   for (auto &Iter : GlobalInstances) {
//     AST::GlobalSegment Seg;
//     auto &Type = Seg.getGlobalType();
//     Type = Iter->getGlobalType();
//     Content.push_back(Seg);
//   }
//
//   std::cout << Content.size();
//
//   return Globals;
// }
} // namespace Executor
} // namespace WasmEdge
