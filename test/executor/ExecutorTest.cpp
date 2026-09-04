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
#include "vm/component_vm.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
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
    // The component-model units run on their own VM, layered on its own
    // core executor.
    WasmEdge::VM::ComponentVM CompVM;
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
    // The component suites run every unit on the component VM.
    bool IsComponentSuite;
    TestContext(const WasmEdge::Configure &C)
        : VM(C), CompVM(C), StrictConf(C),
          IsComponentSuite(C.hasProposal(WasmEdge::Proposal::Component)) {
      VM.registerModule(SpecTestMod);
      if (C.hasProposal(WasmEdge::Proposal::Component)) {
        registerComponentHostItems();
      }
    }
    void registerComponentHostItems() {
      using namespace WasmEdge;
      using namespace WasmEdge::AST::Component;
      using CompFuncInst = Runtime::Instance::ComponentFunctionInstance;
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

      CompVM.getStoreManager().registerInstance(Host.get());
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
      CompVM.getStoreManager().registerFunction(
          "host-return-two", Owner->findFunction("host-return-two"));
      HostCompInsts.push_back(std::move(Owner));

      // Empty host instances the reference suites import by name.
      for (const auto *EmptyName :
           {"wasi", "a:b/c", "a1:b1/c", "r", "not-provided-by-the-host"}) {
        auto Empty =
            std::make_unique<Runtime::Instance::ComponentInstance>(EmptyName);
        CompVM.getStoreManager().registerInstance(Empty.get());
        HostCompInsts.push_back(std::move(Empty));
      }

      // Instance "not-provided-by-the-host2": exports an empty instance "x".
      auto Npbth2 = std::make_unique<Runtime::Instance::ComponentInstance>(
          "not-provided-by-the-host2");
      auto EmptyX = std::make_unique<Runtime::Instance::ComponentInstance>("");
      Npbth2->addComponentInstance(std::move(EmptyX));
      Npbth2->exportComponentInstance("x",
                                      Npbth2->getComponentInstanceCount() - 1);
      CompVM.getStoreManager().registerInstance(Npbth2.get());
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
      CompVM.getStoreManager().registerInstance(InstA.get());
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
      CompVM.getStoreManager().registerInstance(Demo.get());
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
      CompVM.getStoreManager().registerFunction("f", FOwner->findFunction("f"));
      HostCompInsts.push_back(std::move(FOwner));

      // Component definitions that the suites import by name. "a" is an
      // empty component. "x" defines and exports the resource type "x".
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
            CompVM.getStoreManager().registerDefinition(DefName, Comp.get());
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
    auto *TestCtx = static_cast<TestContext *>(Ctx);
    auto &VM = TestCtx->VM;
    auto &CompVM = TestCtx->CompVM;
    // A suite mixes core modules and components, so the parsed unit decides
    // which VM takes it. A parse failure is reported as-is.
    EXPECTED_TRY(auto Unit, CompVM.getLoader().parseWasmUnit(FileName));
    const bool IsComp = std::holds_alternative<
        std::unique_ptr<WasmEdge::AST::Component::Component>>(Unit);
    if (!ModName.empty()) {
      if (IsComp) {
        EXPECTED_TRY(CompVM.registerComponent(ModName, FileName));
      } else {
        return VM.registerModule(ModName, FileName);
      }
    }
    // The fallthrough keeps a named component invokable as the active unit.
    if (IsComp) {
      return CompVM.loadWasm(FileName)
          .and_then([&CompVM]() { return CompVM.validate(); })
          .and_then([&CompVM]() { return CompVM.instantiate(); });
    }
    return VM.loadWasm(FileName)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto *TestCtx = static_cast<TestContext *>(Ctx);
    // The parsed unit decides which VM takes it; a parse failure is the
    // answer either way.
    EXPECTED_TRY(auto Unit,
                 TestCtx->CompVM.getLoader().parseWasmUnit(FileName));
    if (std::holds_alternative<
            std::unique_ptr<WasmEdge::AST::Component::Component>>(Unit)) {
      return TestCtx->CompVM.loadWasm(FileName);
    }
    return TestCtx->VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TestCtx = static_cast<TestContext *>(Ctx);
    EXPECTED_TRY(auto Unit,
                 TestCtx->CompVM.getLoader().parseWasmUnit(FileName));
    if (std::holds_alternative<
            std::unique_ptr<WasmEdge::AST::Component::Component>>(Unit)) {
      auto &CompVM = TestCtx->CompVM;
      return CompVM.loadWasm(FileName).and_then(
          [&CompVM]() { return CompVM.validate(); });
    }
    auto &VM = TestCtx->VM;
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
    // Unlinkable and uninstantiable checks run strictly, so a missing import
    // fails. The shared store keeps the registered instances visible.
    auto *TestCtx = static_cast<TestContext *>(Ctx);
    EXPECTED_TRY(auto Unit,
                 TestCtx->CompVM.getLoader().parseWasmUnit(FileName));
    if (std::holds_alternative<
            std::unique_ptr<WasmEdge::AST::Component::Component>>(Unit)) {
      WasmEdge::VM::ComponentVM StrictVM(TestCtx->StrictConf,
                                         TestCtx->CompVM.getStoreManager());
      return StrictVM.loadWasm(FileName)
          .and_then([&StrictVM]() { return StrictVM.validate(); })
          .and_then([&StrictVM]() { return StrictVM.instantiate(); });
    }
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
    auto &CompVM = static_cast<TestContext *>(Ctx)->CompVM;
    if (!ModName.empty()) {
      return CompVM.execute(ModName, Field, Params);
    }
    return CompVM.execute(Field, Params);
  };
  T.onCompInstanceFromDef =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const AST::Component::Component &Comp) -> Expect<void> {
    auto &CompVM = static_cast<TestContext *>(Ctx)->CompVM;
    return CompVM.registerComponent(ModName, Comp);
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

// Reader of the coredump custom section content.
class CoredumpReader {
public:
  CoredumpReader(WasmEdge::Span<const WasmEdge::Byte> C) noexcept
      : Content(C) {}

  uint8_t readByte() noexcept {
    if (Pos >= Content.size()) {
      Failed = true;
      return 0;
    }
    return Content[Pos++];
  }

  uint32_t readU32() noexcept {
    uint32_t Result = 0;
    uint32_t Shift = 0;
    while (Pos < Content.size()) {
      const uint8_t B = Content[Pos++];
      Result |= static_cast<uint32_t>(B & 0x7FU) << Shift;
      if ((B & 0x80U) == 0) {
        return Result;
      }
      Shift += 7;
    }
    Failed = true;
    return 0;
  }

  std::string readName() noexcept {
    const uint32_t Size = readU32();
    if (Pos + Size > Content.size()) {
      Failed = true;
      return {};
    }
    std::string Result(reinterpret_cast<const char *>(&Content[Pos]), Size);
    Pos += Size;
    return Result;
  }

  // Read a fixed width little endian integer of the wasmgdb dialect.
  template <typename T> T readFixed() noexcept {
    static_assert(std::is_integral_v<T>);
    if (Pos + sizeof(T) > Content.size()) {
      Failed = true;
      return 0;
    }
    T Result = 0;
    std::memcpy(&Result, &Content[Pos], sizeof(Result));
    Pos += sizeof(T);
    return Result;
  }

  int64_t readS64() noexcept {
    int64_t Result = 0;
    uint32_t Shift = 0;
    while (Pos < Content.size()) {
      const uint8_t B = Content[Pos++];
      Result |= static_cast<int64_t>(B & 0x7FU) << Shift;
      Shift += 7;
      if ((B & 0x80U) == 0) {
        if (Shift < 64 && (B & 0x40U) != 0) {
          Result |= -(INT64_C(1) << Shift);
        }
        return Result;
      }
    }
    Failed = true;
    return 0;
  }

  // Read one `value` and return its type tag. The canonical encoding of the
  // format uses LEB128 integers, wasmgdb expects fixed width integers.
  uint8_t readValue(bool ForWasmgdb, int64_t &Out) noexcept {
    const uint8_t Type = readByte();
    switch (Type) {
    case 0x01:
      Out = 0;
      return Type;
    case 0x7F:
      Out = ForWasmgdb ? readFixed<int32_t>() : readS64();
      return Type;
    case 0x7E:
      Out = ForWasmgdb ? readFixed<int64_t>() : readS64();
      return Type;
    case 0x7D:
      readFixed<int32_t>();
      Out = 0;
      return Type;
    case 0x7C:
      readFixed<int64_t>();
      Out = 0;
      return Type;
    default:
      Failed = true;
      return Type;
    }
  }

  bool isFailed() const noexcept { return Failed; }
  bool isEnd() const noexcept { return Pos == Content.size(); }

private:
  WasmEdge::Span<const WasmEdge::Byte> Content;
  size_t Pos = 0;
  bool Failed = false;
};

const AST::CustomSection *findCustomSection(const AST::Module &Mod,
                                            std::string_view Name) noexcept {
  for (const auto &Sec : Mod.getCustomSections()) {
    if (Sec.getName() == Name) {
      return &Sec;
    }
  }
  return nullptr;
}

// Restore the working directory when leaving the scope.
class ScopedWorkingDirectory {
public:
  ScopedWorkingDirectory(const std::filesystem::path &Path)
      : Origin(std::filesystem::current_path()) {
    std::filesystem::current_path(Path);
  }
  ~ScopedWorkingDirectory() {
    std::error_code Error;
    std::filesystem::current_path(Origin, Error);
  }

private:
  std::filesystem::path Origin;
};

// (module
//   (memory (export "mem") 1)
//   (global $g (mut i32) (i32.const 42))
//   (global $h f64 (f64.const 3.5))
//   (func $inner (param i32) (result i32)
//     (local i64)
//     i32.const 7
//     local.get 0
//     i32.const 0
//     i32.store
//     drop
//     local.get 0)
//   (func (export "access_out_of_bounds") (result i32)
//     i32.const 16
//     i32.const 0xcafe
//     i32.store
//     i32.const 0x4522f0
//     call $inner))
std::array<WasmEdge::Byte, 147> CoredumpWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x03, 0x02, 0x00,
    0x01, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x12, 0x02, 0x7f, 0x01, 0x41,
    0x2a, 0x0b, 0x7c, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
    0x40, 0x0b, 0x07, 0x1e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x14,
    0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0x5f, 0x6f, 0x75, 0x74, 0x5f, 0x6f,
    0x66, 0x5f, 0x62, 0x6f, 0x75, 0x6e, 0x64, 0x73, 0x00, 0x01, 0x0a, 0x25,
    0x02, 0x10, 0x01, 0x01, 0x7e, 0x41, 0x07, 0x20, 0x00, 0x41, 0x00, 0x36,
    0x02, 0x00, 0x1a, 0x20, 0x00, 0x0b, 0x12, 0x00, 0x41, 0x10, 0x41, 0xfe,
    0x95, 0x03, 0x36, 0x02, 0x00, 0x41, 0xf0, 0xc5, 0x94, 0x02, 0x10, 0x00,
    0x0b, 0x00, 0x18, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00,
    0x05, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x07, 0x07, 0x02, 0x00, 0x01, 0x67,
    0x01, 0x01, 0x68};

// Trap the module in an empty directory and parse the generated coredump.
void runCoredump(bool ForWasmgdb, AST::Module &Output) {
  const auto TempDir = std::filesystem::temp_directory_path() /
                       std::filesystem::u8path("wasmedge-coredump-"s +
                                               std::to_string(ForWasmgdb));
  std::error_code Error;
  std::filesystem::remove_all(TempDir, Error);
  ASSERT_TRUE(std::filesystem::create_directories(TempDir, Error));

  {
    ScopedWorkingDirectory Guard(TempDir);
    WasmEdge::Configure Conf;
    Conf.getRuntimeConfigure().setEnableCoredump(true);
    Conf.getRuntimeConfigure().setCoredumpWasmgdb(ForWasmgdb);
    WasmEdge::VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(CoredumpWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    const auto Result = VM.execute("access_out_of_bounds"sv);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::MemoryOutOfBounds);
  }

  std::vector<std::filesystem::path> Dumps;
  for (const auto &Entry : std::filesystem::directory_iterator(TempDir)) {
    if (Entry.path().filename().string().rfind("coredump."s, 0) == 0) {
      Dumps.push_back(Entry.path());
    }
  }
  ASSERT_EQ(Dumps.size(), 1U);

  // The coredump must be a well-formed WASM module.
  WasmEdge::Configure LoadConf;
  WasmEdge::Loader::Loader LoadEngine(LoadConf);
  auto Mod = LoadEngine.parseModule(Dumps[0].u8string());
  ASSERT_TRUE(Mod);
  Output = std::move(**Mod);
  std::filesystem::remove_all(TempDir, Error);
}

TEST(Coredump, Sections) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(false, Mod));

  ASSERT_EQ(Mod.getMemorySection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getMemorySection().getContent()[0].getLimit().getMin(), 1U);

  // The runs of zeros of the linear memory are skipped, only the marker
  // written by the module is dumped.
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  const auto &Seg = Mod.getDataSection().getContent()[0];
  EXPECT_EQ(Seg.getMode(), AST::DataSegment::DataMode::Active);
  EXPECT_EQ(Seg.getIdx(), 0U);
  ASSERT_EQ(Seg.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getNum().get<int32_t>(), 16);
  ASSERT_EQ(Seg.getData().size(), 2U);
  EXPECT_EQ(Seg.getData()[0], 0xFE);
  EXPECT_EQ(Seg.getData()[1], 0xCA);

  // The globals are dumped as constant globals holding their current values.
  ASSERT_EQ(Mod.getGlobalSection().getContent().size(), 2U);
  const auto &GlobI32 = Mod.getGlobalSection().getContent()[0];
  EXPECT_EQ(GlobI32.getGlobalType().getValMut(), ValMut::Const);
  EXPECT_EQ(GlobI32.getGlobalType().getValType().getCode(), TypeCode::I32);
  ASSERT_EQ(GlobI32.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(GlobI32.getExpr().getInstrs()[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(GlobI32.getExpr().getInstrs()[0].getNum().get<int32_t>(), 42);
  const auto &GlobF64 = Mod.getGlobalSection().getContent()[1];
  EXPECT_EQ(GlobF64.getGlobalType().getValMut(), ValMut::Const);
  ASSERT_EQ(GlobF64.getExpr().getInstrs().size(), 2U);
  EXPECT_EQ(GlobF64.getExpr().getInstrs()[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(GlobF64.getExpr().getInstrs()[0].getNum().get<double>(),
                   3.5);

  // core ::= customsec(0x0 executable-name:name)
  const auto *Core = findCustomSection(Mod, "core"sv);
  ASSERT_NE(Core, nullptr);
  CoredumpReader CoreReader(Core->getContent());
  EXPECT_EQ(CoreReader.readByte(), 0x00);
  CoreReader.readName();
  EXPECT_FALSE(CoreReader.isFailed());
  EXPECT_TRUE(CoreReader.isEnd());

  // coremodules ::= customsec(vec(0x0 module-name:name))
  const auto *CoreModules = findCustomSection(Mod, "coremodules"sv);
  ASSERT_NE(CoreModules, nullptr);
  CoredumpReader ModuleReader(CoreModules->getContent());
  ASSERT_EQ(ModuleReader.readU32(), 1U);
  EXPECT_EQ(ModuleReader.readByte(), 0x00);
  ModuleReader.readName();
  EXPECT_FALSE(ModuleReader.isFailed());
  EXPECT_TRUE(ModuleReader.isEnd());

  // coreinstances ::= customsec(vec(0x0 moduleidx memories:vec globals:vec))
  const auto *CoreInstances = findCustomSection(Mod, "coreinstances"sv);
  ASSERT_NE(CoreInstances, nullptr);
  CoredumpReader InstanceReader(CoreInstances->getContent());
  ASSERT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_EQ(InstanceReader.readByte(), 0x00);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  ASSERT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  ASSERT_EQ(InstanceReader.readU32(), 2U);
  EXPECT_EQ(InstanceReader.readU32(), 0U);
  EXPECT_EQ(InstanceReader.readU32(), 1U);
  EXPECT_FALSE(InstanceReader.isFailed());
  EXPECT_TRUE(InstanceReader.isEnd());
}

TEST(Coredump, StackFrames) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(false, Mod));

  const auto *CoreStack = findCustomSection(Mod, "corestack"sv);
  ASSERT_NE(CoreStack, nullptr);
  CoredumpReader Reader(CoreStack->getContent());
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readName(), "main"s);

  // The frames are listed from the youngest one to the oldest one, therefore
  // the trapping `$inner` comes before its caller.
  ASSERT_EQ(Reader.readU32(), 2U);

  int64_t Value = 0;
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);
  // The code offset is relative to the first instruction of the function body,
  // the trapping `i32.store` of `$inner` follows three 2 byte instructions.
  EXPECT_EQ(Reader.readU32(), 6U);
  // `$inner` has one i32 parameter and one i64 local.
  ASSERT_EQ(Reader.readU32(), 2U);
  EXPECT_EQ(Reader.readValue(false, Value), 0x7F);
  EXPECT_EQ(Value, INT64_C(0x4522f0));
  EXPECT_EQ(Reader.readValue(false, Value), 0x7E);
  EXPECT_EQ(Value, INT64_C(0));
  // The operand stack values are untyped in the interpreter, hence they are
  // dumped as missing values.
  ASSERT_EQ(Reader.readU32(), 1U);
  EXPECT_EQ(Reader.readValue(false, Value), 0x01);

  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 1U);
  // The `call` of the caller follows a 2 byte, a 4 byte, a 3 byte and a 5 byte
  // instruction.
  EXPECT_EQ(Reader.readU32(), 14U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);

  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
}

TEST(Coredump, Wasmgdb) {
  AST::Module Mod;
  ASSERT_NO_FATAL_FAILURE(runCoredump(true, Mod));

  // The wasmgdb parser only reads the first data segment and pads it with the
  // offset, hence the whole memory must be dumped as a single segment.
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  const auto &Seg = Mod.getDataSection().getContent()[0];
  EXPECT_EQ(Seg.getExpr().getInstrs()[0].getNum().get<int32_t>(), 0);
  ASSERT_EQ(Seg.getData().size(), UINT32_C(65536));
  EXPECT_EQ(Seg.getData()[16], 0xFE);
  EXPECT_EQ(Seg.getData()[17], 0xCA);

  const auto *CoreStack = findCustomSection(Mod, "corestack"sv);
  ASSERT_NE(CoreStack, nullptr);
  CoredumpReader Reader(CoreStack->getContent());
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readName(), "main"s);
  ASSERT_EQ(Reader.readU32(), 2U);

  // The wasmgdb parser expects both vector sizes before the values and never
  // reads the operand stack, hence the operand stack must be empty.
  int64_t Value = 0;
  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 6U);
  ASSERT_EQ(Reader.readU32(), 2U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readValue(true, Value), 0x7F);
  EXPECT_EQ(Value, INT64_C(0x4522f0));
  EXPECT_EQ(Reader.readValue(true, Value), 0x7E);
  EXPECT_EQ(Value, INT64_C(0));

  EXPECT_EQ(Reader.readByte(), 0x00);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 1U);
  EXPECT_EQ(Reader.readU32(), 14U);
  EXPECT_EQ(Reader.readU32(), 0U);
  EXPECT_EQ(Reader.readU32(), 0U);

  EXPECT_FALSE(Reader.isFailed());
  EXPECT_TRUE(Reader.isEnd());
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
