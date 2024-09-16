// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "loader/serialize.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <variant>

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
  const Configure Config;
  Loader::Serializer Ser(Config);
  const auto *CurrentInstance = StackMgr.getModule();
  if (CurrentInstance == nullptr) {
    return;
  }

  auto Core = collectProcessInformation();
  // auto DataSection = collectDataSection(StackMgr);
  auto MemSec = collectMemory(StackMgr);
  auto Globals = collectGlobals(StackMgr);
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
  Mod.getGlobalSection() = std::move(Globals);
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

  // TODO add check here to see if it is multimemory
  auto &Content = MemSec.getContent();
  Content.push_back(MemInstances[0]->getMemoryType());
  // In the current version of WebAssembly, at most one memory may be defined
  // or imported in a single module, and all constructs implicitly reference
  // this memory.

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
  // TODO reverse correctly using rbegin/rend
  std::reverse(Frames.begin(), Frames.end());
  auto FramesSize = Frames.size();

  auto &Content = CoreStack.getContent();

  // The following denotes the length of the thread-name.
  Content.push_back(0x00);
  Content.push_back(0x04);

  std::string ThreadName = "main";
  Content.insert(Content.end(), ThreadName.begin(), ThreadName.end());

  // This denotes the number of frames in the section
  Content.push_back(static_cast<Byte>(FramesSize - 1));
  for (size_t I = 0; I < FramesSize; I++) {
    if (Frames[I].Module == nullptr) // guard
      continue;

    // each frame's start is denoted by this
    Content.push_back(0x00);
    auto Funcidx = Frames[I].From->getTargetIndex(); // function index
    // TODO calculate offset properly
    auto Codeoffset = Frames[I].From->getOffset();

    uint32_t Lstart = Frames[I].VPos - Frames[I].Locals;
    uint32_t Lend = Frames[I].VPos;

    uint32_t Vstart = Frames[I].VPos;
    // if it is the first frame i.e the top frame, we set it to valuestack size
    uint32_t Vend =
        (I > 0) ? Frames[I - 1].VPos - Frames[I - 1].Locals : StackMgr.size();

    uint32_t Lsize = Lend - Lstart;
    uint32_t Vsize = Vend - Vstart;

    auto Locals = StackMgr.getRangeSpan(Lstart, Lsize);
    auto Stacks = StackMgr.getRangeSpan(Vstart, Vsize);

    Content.push_back(static_cast<Byte>(Funcidx));
    Content.push_back(static_cast<Byte>(Codeoffset));
    // TODO map values correctly to their binary encoding
    // XXX Using 0x7E for now

    Content.push_back(Frames[I].Locals);
    Content.push_back(Vsize);
    for (auto &Iter : Locals) {
      Content.push_back(0x7E);
      auto Value = Iter.unwrap();

      std::vector<Byte> ValueBytes(4);
      std::memcpy(ValueBytes.data(), &Value, sizeof(int64_t));

      // Create the final byte vector
      Content.insert(Content.end(), ValueBytes.begin(), ValueBytes.end());
    }
    for (auto &Iter : Stacks) {
      // TODO why is this parsed incorrectly by wasmgdb as 126?
      Content.push_back(0x7E);
      auto Value = Iter.unwrap();

      std::vector<Byte> ValueBytes(4);
      std::memcpy(ValueBytes.data(), &Value, sizeof(int64_t));

      // Create the Locals byte vector
      Content.insert(Content.end(), ValueBytes.begin(), ValueBytes.end());
    }
  }

  return CoreStack;
}

AST::GlobalSection Executor::collectGlobals(Runtime::StackManager &StackMgr) {
  spdlog::info("Collecting Globals");
  AST::GlobalSection Globals;
  const auto *const CurrentInstance = StackMgr.getModule();

  auto &GlobalInstances = CurrentInstance->getOwnedGlobalInstances();
  auto &Content = Globals.getContent();

  for (auto &Iter : GlobalInstances) {
    AST::GlobalSegment Seg;
    Iter->getValue();
    Seg.getGlobalType() = Iter->getGlobalType();
    // TODO get the init expression here, for now this is dummy expression
    Seg.getExpr().getInstrs() = {
        WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};

    Content.push_back(Seg);
  }

  return Globals;
}
} // namespace Executor
} // namespace WasmEdge
