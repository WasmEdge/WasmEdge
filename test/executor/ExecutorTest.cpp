// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/executor/ExecutorTest.cpp - Wasm test suites --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suites: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    // Configuration copy for spawning fresh VMs in unlinkable /
    // uninstantiable checks.
    WasmEdge::Configure StrictConf;
    // Host component items the reference test suites expect ("host",
    // "host-return-two").
    std::vector<std::unique_ptr<WasmEdge::Runtime::Instance::ComponentInstance>>
        HostCompInsts;
    std::vector<std::unique_ptr<WasmEdge::AST::Component::Component>>
        HostCompDefs;
    std::unique_ptr<WasmEdge::AST::Module> HostSimpleModule;
    TestContext(const WasmEdge::Configure &C) : VM(C), StrictConf(C) {
      VM.registerModule(SpecTestMod);
      if (C.hasProposal(WasmEdge::Proposal::Component)) {
        registerComponentHostItems();
      }
    }
    void registerComponentHostItems() {
      using namespace WasmEdge;
      using namespace WasmEdge::AST::Component;
      using CompFuncInst = Runtime::Instance::Component::FunctionInstance;
      using HostRets =
          std::vector<std::pair<ComponentValVariant, ComponentValType>>;

      // Instance "host": resource1/resource2 with constructor and a static
      // assert, plus return-three. Mirrors the reference wast runner's host.
      auto Host =
          std::make_unique<Runtime::Instance::ComponentInstance>("host");
      // Drop tracking for resource1 (last-drop / drops statics).
      struct DropState {
        uint32_t Drops = 0;
        uint32_t LastDrop = 0;
      };
      auto Track = std::make_shared<DropState>();
      const uint32_t R1 = Host->addHostResourceType([Track](uint32_t Rep) {
        Track->Drops += 1;
        Track->LastDrop = Rep;
      });
      Host->exportType("resource1", R1);
      const uint32_t R2 = Host->addHostResourceType(nullptr);
      Host->exportType("resource2", R2);
      Host->exportType("resource1-again", R1);
      DefType OwnR1;
      DefValType DVT;
      DVT.setOwn(OwnTy{R1});
      OwnR1.setDefValType(std::move(DVT));
      const uint32_t OwnIdx = Host->addOwnedType(std::move(OwnR1));

      auto Ctor = std::make_unique<FuncType>(
          std::vector<LabelValType>{
              LabelValType("r", ComponentValType(ComponentTypeCode::U32))},
          std::vector<LabelValType>{LabelValType(ComponentValType(OwnIdx))});
      Host->addHostFunc(
          "[constructor]resource1",
          std::make_unique<CompFuncInst>(
              std::move(Ctor),
              [OwnIdx](
                  Span<const ComponentValVariant> Args) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(
                    makeComponentVal(OwnVal{std::get<uint32_t>(Args[0])}),
                    ComponentValType(OwnIdx));
                return R;
              },
              Host.get()));

      auto Assert = std::make_unique<FuncType>(
          std::vector<LabelValType>{
              LabelValType("r", ComponentValType(OwnIdx)),
              LabelValType("rep", ComponentValType(ComponentTypeCode::U32))},
          std::vector<LabelValType>{});
      Host->addHostFunc(
          "[static]resource1.assert",
          std::make_unique<CompFuncInst>(
              std::move(Assert),
              [](Span<const ComponentValVariant> Args) -> Expect<HostRets> {
                const auto &VC = std::get<std::shared_ptr<ValComp>>(Args[0]);
                const uint32_t Rep = std::get<OwnVal>(VC->V).Handle;
                if (Rep != std::get<uint32_t>(Args[1])) {
                  return Unexpect(ErrCode::Value::HostFuncError);
                }
                return HostRets{};
              },
              Host.get()));

      auto Three = std::make_unique<FuncType>(
          std::vector<LabelValType>{},
          std::vector<LabelValType>{
              LabelValType(ComponentValType(ComponentTypeCode::U32))});
      Host->addHostFunc(
          "return-three",
          std::make_unique<CompFuncInst>(
              std::move(Three),
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint32_t(3)},
                               ComponentValType(ComponentTypeCode::U32));
                return R;
              },
              Host.get()));

      // Named record types: x, rec, some-record.
      RecordTy XRec;
      XRec.LabelTypes.emplace_back("x",
                                   ComponentValType(ComponentTypeCode::U32));
      DefType XDT;
      DefValType XDVT;
      XDVT.setRecord(std::move(XRec));
      XDT.setDefValType(std::move(XDVT));
      const uint32_t XIdx = Host->addOwnedType(std::move(XDT));
      Host->exportType("x", XIdx);
      RecordTy RecRec;
      RecRec.LabelTypes.emplace_back("x", ComponentValType(XIdx));
      RecRec.LabelTypes.emplace_back(
          "y", ComponentValType(ComponentTypeCode::String));
      DefType RecDT;
      DefValType RecDVT;
      RecDVT.setRecord(std::move(RecRec));
      RecDT.setDefValType(std::move(RecDVT));
      const uint32_t RecIdx = Host->addOwnedType(std::move(RecDT));
      Host->exportType("rec", RecIdx);
      Host->exportType("some-record", RecIdx);

      // Nested instance: return-four.
      auto Nested = std::make_unique<Runtime::Instance::ComponentInstance>("");
      auto Four = std::make_unique<FuncType>(
          std::vector<LabelValType>{},
          std::vector<LabelValType>{
              LabelValType(ComponentValType(ComponentTypeCode::U32))});
      Nested->addHostFunc(
          "return-four",
          std::make_unique<CompFuncInst>(
              std::move(Four),
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint32_t(4)},
                               ComponentValType(ComponentTypeCode::U32));
                return R;
              },
              Nested.get()));
      const auto *NestedPtr = Nested.get();
      Host->addComponentInstance(std::move(Nested));
      Host->exportComponentInstance("nested",
                                    Host->getComponentInstanceCount() - 1);
      (void)NestedPtr;

      // Core module export: f() -> 101, g = 100.
      static const std::vector<WasmEdge::Byte> SimpleModuleWasm = {
          0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05,
          0x01, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x06,
          0x07, 0x01, 0x7f, 0x00, 0x41, 0xe4, 0x00, 0x0b, 0x07, 0x09,
          0x02, 0x01, 0x66, 0x00, 0x00, 0x01, 0x67, 0x03, 0x00, 0x0a,
          0x07, 0x01, 0x05, 0x00, 0x41, 0xe5, 0x00, 0x0b};
      if (auto Res = VM.getLoader().parseModule(SimpleModuleWasm)) {
        HostSimpleModule = std::move(*Res);
        Host->addModule(*HostSimpleModule);
        // The module index space of this host instance has exactly one
        // entry.
        Host->exportCoreModule("simple-module", 0);
      }

      // Borrow type for the resource1 methods.
      DefType BorrowDT;
      DefValType BorrowDVT;
      BorrowDVT.setBorrow(BorrowTy{R1});
      BorrowDT.setDefValType(std::move(BorrowDVT));
      const uint32_t BorIdx = Host->addOwnedType(std::move(BorrowDT));

      auto Simple = std::make_unique<FuncType>(
          std::vector<LabelValType>{
              LabelValType("self", ComponentValType(BorIdx)),
              LabelValType("rep", ComponentValType(ComponentTypeCode::U32))},
          std::vector<LabelValType>{});
      Host->addHostFunc(
          "[method]resource1.simple",
          std::make_unique<CompFuncInst>(
              std::move(Simple),
              [](Span<const ComponentValVariant> Args) -> Expect<HostRets> {
                const auto &VC = std::get<std::shared_ptr<ValComp>>(Args[0]);
                if (std::get<BorrowVal>(VC->V).Handle !=
                    std::get<uint32_t>(Args[1])) {
                  return Unexpect(ErrCode::Value::HostFuncError);
                }
                return HostRets{};
              },
              Host.get()));

      auto TakeOwn = std::make_unique<FuncType>(
          std::vector<LabelValType>{
              LabelValType("self", ComponentValType(BorIdx)),
              LabelValType("b", ComponentValType(OwnIdx))},
          std::vector<LabelValType>{});
      Host->addHostFunc(
          "[method]resource1.take-own",
          std::make_unique<CompFuncInst>(
              std::move(TakeOwn),
              [Track](
                  Span<const ComponentValVariant> Args) -> Expect<HostRets> {
                // Taking ownership drops the resource on the host side.
                const auto &VC = std::get<std::shared_ptr<ValComp>>(Args[1]);
                Track->Drops += 1;
                Track->LastDrop = std::get<OwnVal>(VC->V).Handle;
                return HostRets{};
              },
              Host.get()));

      auto TakeBorrow = std::make_unique<FuncType>(
          std::vector<LabelValType>{
              LabelValType("self", ComponentValType(BorIdx)),
              LabelValType("b", ComponentValType(BorIdx))},
          std::vector<LabelValType>{});
      Host->addHostFunc(
          "[method]resource1.take-borrow",
          std::make_unique<CompFuncInst>(
              std::move(TakeBorrow),
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                return HostRets{};
              },
              Host.get()));

      auto Drops = std::make_unique<FuncType>(
          std::vector<LabelValType>{},
          std::vector<LabelValType>{
              LabelValType(ComponentValType(ComponentTypeCode::U32))});
      Host->addHostFunc(
          "[static]resource1.drops",
          std::make_unique<CompFuncInst>(
              std::move(Drops),
              [Track](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint32_t(Track->Drops)},
                               ComponentValType(ComponentTypeCode::U32));
                return R;
              },
              Host.get()));

      auto LastDrop = std::make_unique<FuncType>(
          std::vector<LabelValType>{},
          std::vector<LabelValType>{
              LabelValType(ComponentValType(ComponentTypeCode::U32))});
      Host->addHostFunc(
          "[static]resource1.last-drop",
          std::make_unique<CompFuncInst>(
              std::move(LastDrop),
              [Track](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint32_t(Track->LastDrop)},
                               ComponentValType(ComponentTypeCode::U32));
                return R;
              },
              Host.get()));

      VM.getStoreManager().registerComponent(Host.get());
      HostCompInsts.push_back(std::move(Host));

      // Standalone host function "host-return-two".
      auto Owner = std::make_unique<Runtime::Instance::ComponentInstance>("");
      auto Two = std::make_unique<FuncType>(
          std::vector<LabelValType>{},
          std::vector<LabelValType>{
              LabelValType(ComponentValType(ComponentTypeCode::U32))});
      Owner->addHostFunc(
          "host-return-two",
          std::make_unique<CompFuncInst>(
              std::move(Two),
              [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                HostRets R;
                R.emplace_back(ComponentValVariant{uint32_t(2)},
                               ComponentValType(ComponentTypeCode::U32));
                return R;
              },
              Owner.get()));
      VM.getStoreManager().registerComponentFunction(
          "host-return-two", Owner->findFunction("host-return-two"));
      HostCompInsts.push_back(std::move(Owner));

      // Empty host instances the reference suites import by name.
      for (const auto *EmptyName :
           {"wasi", "a:b/c", "a1:b1/c", "r", "not-provided-by-the-host"}) {
        auto Empty =
            std::make_unique<Runtime::Instance::ComponentInstance>(EmptyName);
        VM.getStoreManager().registerComponent(Empty.get());
        HostCompInsts.push_back(std::move(Empty));
      }

      // Instance "not-provided-by-the-host2": exports an empty instance "x".
      auto Npbth2 = std::make_unique<Runtime::Instance::ComponentInstance>(
          "not-provided-by-the-host2");
      auto EmptyX = std::make_unique<Runtime::Instance::ComponentInstance>("");
      Npbth2->addComponentInstance(std::move(EmptyX));
      Npbth2->exportComponentInstance("x",
                                      Npbth2->getComponentInstanceCount() - 1);
      VM.getStoreManager().registerComponent(Npbth2.get());
      HostCompInsts.push_back(std::move(Npbth2));

      // Instance "a": exports the type u64 under the names "a" and "b".
      auto InstA = std::make_unique<Runtime::Instance::ComponentInstance>("a");
      DefType U64DT;
      DefValType U64DVT;
      U64DVT.setPrimValType(PrimValType::U64);
      U64DT.setDefValType(std::move(U64DVT));
      const uint32_t U64Idx = InstA->addOwnedType(std::move(U64DT));
      InstA->exportType("a", U64Idx);
      InstA->exportType("b", U64Idx);
      VM.getStoreManager().registerComponent(InstA.get());
      HostCompInsts.push_back(std::move(InstA));

      // Instance "demo:component/types": enum "baz" and record "foo".
      auto Demo = std::make_unique<Runtime::Instance::ComponentInstance>(
          "demo:component/types");
      EnumTy QuxEnum;
      QuxEnum.Labels.push_back("qux");
      DefType BazDT;
      DefValType BazDVT;
      BazDVT.setEnum(std::move(QuxEnum));
      BazDT.setDefValType(std::move(BazDVT));
      const uint32_t BazIdx = Demo->addOwnedType(std::move(BazDT));
      Demo->exportType("baz", BazIdx);
      RecordTy FooRec;
      FooRec.LabelTypes.emplace_back("bar", ComponentValType(BazIdx));
      DefType FooDT;
      DefValType FooDVT;
      FooDVT.setRecord(std::move(FooRec));
      FooDT.setDefValType(std::move(FooDVT));
      const uint32_t FooIdx = Demo->addOwnedType(std::move(FooDT));
      Demo->exportType("foo", FooIdx);
      VM.getStoreManager().registerComponent(Demo.get());
      HostCompInsts.push_back(std::move(Demo));

      // Standalone no-op host function "f".
      auto FOwner = std::make_unique<Runtime::Instance::ComponentInstance>("");
      auto FTy = std::make_unique<FuncType>(std::vector<LabelValType>{},
                                            std::vector<LabelValType>{});
      FOwner->addHostFunc(
          "f", std::make_unique<CompFuncInst>(
                   std::move(FTy),
                   [](Span<const ComponentValVariant>) -> Expect<HostRets> {
                     return HostRets{};
                   },
                   FOwner.get()));
      VM.getStoreManager().registerComponentFunction("f",
                                                     FOwner->findFunction("f"));
      HostCompInsts.push_back(std::move(FOwner));

      // Component definitions the suites import by name: "a" is an empty
      // component; "x" defines and exports the resource type "x".
      static const std::vector<WasmEdge::Byte> ComponentAWasm = {
          0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00};
      static const std::vector<WasmEdge::Byte> ComponentXWasm = {
          0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00,
          0x07, 0x04, 0x01, 0x3f, 0x7f, 0x00, 0x0b, 0x07,
          0x01, 0x00, 0x01, 0x78, 0x03, 0x00, 0x00};
      for (const auto &[DefName, DefWasm] :
           {std::make_pair("a", &ComponentAWasm),
            std::make_pair("x", &ComponentXWasm)}) {
        if (auto Res = VM.getLoader().parseWasmUnit(*DefWasm)) {
          auto &Comp =
              std::get<std::unique_ptr<AST::Component::Component>>(*Res);
          if (VM.getValidator().validate(*Comp)) {
            VM.getStoreManager().registerComponentDefinition(DefName,
                                                             Comp.get());
            HostCompDefs.push_back(std::move(Comp));
          }
        }
      }
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    // Always create VM with own Store to avoid module name conflicts
    // from built-in host modules being re-registered in a shared Store.
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          // Register the shared module under the alias name so that
          // the thread's wasm modules can import it by the expected name.
          Ctx->VM.registerModule(AliasName, *ModInst);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // registerModule only supports core wasm modules; components register
      // through registerComponent. A named unit must also stay invokable, so
      // the fallthrough instantiation below still runs for components.
      if (auto Res = VM.registerModule(ModName, FileName); Res) {
        return {};
      }
      if (auto Res = VM.registerComponent(ModName, FileName); !Res) {
        return Res;
      }
    }
    return VM.loadWasm(FileName)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTUnit, Loader.parseWasmUnit(FileName));
    if (std::holds_alternative<std::unique_ptr<AST::Module>>(ASTUnit)) {
      auto &ASTMod = std::get<std::unique_ptr<AST::Module>>(ASTUnit);
      EXPECTED_TRY(Validator.validate(*ASTMod.get()));
    } else {
      auto &ASTComp =
          std::get<std::unique_ptr<AST::Component::Component>>(ASTUnit);
      EXPECTED_TRY(Validator.validate(*ASTComp.get()));
    }
    return ASTUnit;
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    // Unlinkable / uninstantiable checks run strictly: missing imports must
    // fail rather than be synthesized. Share the store so registered
    // instances stay visible.
    auto *TestCtx = static_cast<TestContext *>(Ctx);
    WasmEdge::VM::VM StrictVM(TestCtx->StrictConf,
                              TestCtx->VM.getStoreManager());
    return StrictVM.loadWasm(FileName)
        .and_then([&StrictVM]() { return StrictVM.validate(); })
        .and_then([&StrictVM]() { return StrictVM.instantiate(); });
  };
  T.onCompInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                      const std::string &Field,
                      const std::vector<WasmEdge::ComponentValVariant> &Params)
      -> Expect<std::vector<std::pair<WasmEdge::ComponentValVariant,
                                      WasmEdge::ComponentValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      return VM.executeComponent(ModName, Field, Params);
    }
    return VM.executeComponent(Field, Params);
  };
  T.onCompInstanceFromDef =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const AST::Component::Component &Comp) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerComponent(ModName, Comp);
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in the VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

TEST(AsyncRunWsmFile, InterruptTest) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(AsyncWasm, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(AsyncWasm, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(AsyncExecute, InterruptTest) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AsyncWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(AsyncInvoke, InterruptTest) {
  WasmEdge::Configure Conf;
  WasmEdge::Loader::Loader LoadEngine(Conf);
  WasmEdge::Validator::Validator ValidEngine(Conf);
  WasmEdge::Executor::Executor ExecEngine(Conf);
  WasmEdge::Runtime::StoreManager Store;

  auto AST = LoadEngine.parseModule(AsyncWasm);
  ASSERT_TRUE(AST);
  ASSERT_TRUE(ValidEngine.validate(**AST));
  auto Module = ExecEngine.instantiateModule(Store, **AST);
  ASSERT_TRUE(Module);
  auto FuncInst = (*Module)->findFuncExports("_start");
  ASSERT_NE(FuncInst, nullptr);
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = ExecEngine.asyncInvoke(FuncInst, {}, {});
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = ExecEngine.asyncInvoke(FuncInst, {}, {});
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(VM, MultipleVM) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM1(Conf);
  WasmEdge::VM::VM VM2(Conf);
  std::array<WasmEdge::Byte, 36> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0a, 0x01, 0x06, 0x5f, 0x73,
      0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b};
  ASSERT_TRUE(VM1.loadWasm(Wasm));
  ASSERT_TRUE(VM1.validate());
  ASSERT_TRUE(VM1.instantiate());
  ASSERT_TRUE(VM2.loadWasm(Wasm));
  ASSERT_TRUE(VM2.validate());
  ASSERT_TRUE(VM2.instantiate());
  auto Result1 = VM1.execute("_start");
  auto Result2 = VM2.execute("_start");
  EXPECT_TRUE(Result1);
  EXPECT_TRUE(Result2);
}

TEST(Coredump, generateCoredump) {
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setEnableCoredump(true);
  Conf.getRuntimeConfigure().setCoredumpWasmgdb(false);
  WasmEdge::VM::VM VM(Conf);
  std::array<WasmEdge::Byte, 70> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
      0x1e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x14, 0x61, 0x63, 0x63,
      0x65, 0x73, 0x73, 0x5f, 0x6f, 0x75, 0x74, 0x5f, 0x6f, 0x66, 0x5f, 0x62,
      0x6f, 0x75, 0x6e, 0x64, 0x73, 0x00, 0x00, 0x0a, 0x0d, 0x01, 0x0b, 0x00,
      0x41, 0xf0, 0xa2, 0x04, 0x41, 0x00, 0x36, 0x02, 0x00, 0x0b};
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.execute("access_out_of_bounds");
  bool FindCoredump = false;
  for (const auto &Entry : std::filesystem::directory_iterator("./")) {
    if (Entry.path().string().find("coredump.") != std::string::npos) {
      FindCoredump = true;
      break;
    }
  }
  EXPECT_TRUE(FindCoredump);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
